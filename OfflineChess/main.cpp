#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <array>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdlib>

struct Move {
    int from = -1;
    int to = -1;
    char promotion = 0;
    bool castle = false;
    bool enPassant = false;
};

std::array<char, 64> board;
bool whiteTurn = true;
int selected = -1;
int enPassantSquare = -1;
bool whiteKingMoved = false;
bool blackKingMoved = false;
bool whiteRookAMoved = false;
bool whiteRookHMoved = false;
bool blackRookAMoved = false;
bool blackRookHMoved = false;
bool gameOver = false;
std::string statusText = "White to move";
std::vector<Move> legalMoves;

const int boardX = 28;
const int boardY = 28;
const int squareSize = 82;

bool isWhite(char piece) {
    return piece >= 'A' && piece <= 'Z';
}

bool isBlack(char piece) {
    return piece >= 'a' && piece <= 'z';
}

bool sameSide(char a, char b) {
    if (a == '.' || b == '.') return false;
    return isWhite(a) == isWhite(b);
}

int rowOf(int square) {
    return square / 8;
}

int colOf(int square) {
    return square % 8;
}

bool inside(int row, int col) {
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

void resetGame() {
    std::string start =
        "rnbqkbnr"
        "pppppppp"
        "........"
        "........"
        "........"
        "........"
        "PPPPPPPP"
        "RNBQKBNR";

    for (int i = 0; i < 64; i++) {
        board[i] = start[i];
    }

    whiteTurn = true;
    selected = -1;
    enPassantSquare = -1;
    whiteKingMoved = false;
    blackKingMoved = false;
    whiteRookAMoved = false;
    whiteRookHMoved = false;
    blackRookAMoved = false;
    blackRookHMoved = false;
    gameOver = false;
    statusText = "White to move";
    legalMoves.clear();
}

bool squareAttacked(const std::array<char, 64>& state, int square, bool byWhite) {
    int row = rowOf(square);
    int col = colOf(square);

    int pawnRow = row + (byWhite ? 1 : -1);

    for (int dc : {-1, 1}) {
        int c = col + dc;

        if (inside(pawnRow, c)) {
            char piece = state[pawnRow * 8 + c];

            if (piece == (byWhite ? 'P' : 'p')) {
                return true;
            }
        }
    }

    const int knightMoves[8][2] = {
        {-2,-1}, {-2,1}, {-1,-2}, {-1,2},
        {1,-2}, {1,2}, {2,-1}, {2,1}
    };

    for (auto& move : knightMoves) {
        int r = row + move[0];
        int c = col + move[1];

        if (inside(r, c) && state[r * 8 + c] == (byWhite ? 'N' : 'n')) {
            return true;
        }
    }

    const int bishopDirs[4][2] = {
        {-1,-1}, {-1,1}, {1,-1}, {1,1}
    };

    for (auto& dir : bishopDirs) {
        int r = row + dir[0];
        int c = col + dir[1];

        while (inside(r, c)) {
            char piece = state[r * 8 + c];

            if (piece != '.') {
                if (piece == (byWhite ? 'B' : 'b') || piece == (byWhite ? 'Q' : 'q')) {
                    return true;
                }

                break;
            }

            r += dir[0];
            c += dir[1];
        }
    }

    const int rookDirs[4][2] = {
        {-1,0}, {1,0}, {0,-1}, {0,1}
    };

    for (auto& dir : rookDirs) {
        int r = row + dir[0];
        int c = col + dir[1];

        while (inside(r, c)) {
            char piece = state[r * 8 + c];

            if (piece != '.') {
                if (piece == (byWhite ? 'R' : 'r') || piece == (byWhite ? 'Q' : 'q')) {
                    return true;
                }

                break;
            }

            r += dir[0];
            c += dir[1];
        }
    }

    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) continue;

            int r = row + dr;
            int c = col + dc;

            if (inside(r, c) && state[r * 8 + c] == (byWhite ? 'K' : 'k')) {
                return true;
            }
        }
    }

    return false;
}

bool kingInCheck(const std::array<char, 64>& state, bool white) {
    char king = white ? 'K' : 'k';

    for (int i = 0; i < 64; i++) {
        if (state[i] == king) {
            return squareAttacked(state, i, !white);
        }
    }

    return true;
}

void addSlidingMoves(std::vector<Move>& moves, int from, const int dirs[][2], int count) {
    int row = rowOf(from);
    int col = colOf(from);

    for (int i = 0; i < count; i++) {
        int r = row + dirs[i][0];
        int c = col + dirs[i][1];

        while (inside(r, c)) {
            int to = r * 8 + c;

            if (board[to] == '.') {
                moves.push_back({from, to});
            } else {
                if (!sameSide(board[from], board[to])) {
                    moves.push_back({from, to});
                }

                break;
            }

            r += dirs[i][0];
            c += dirs[i][1];
        }
    }
}

std::vector<Move> pseudoMoves(bool white) {
    std::vector<Move> moves;

    for (int from = 0; from < 64; from++) {
        char piece = board[from];

        if (piece == '.' || isWhite(piece) != white) continue;

        int row = rowOf(from);
        int col = colOf(from);
        char lower = static_cast<char>(tolower(piece));

        if (lower == 'p') {
            int direction = white ? -1 : 1;
            int startRow = white ? 6 : 1;
            int promotionRow = white ? 0 : 7;
            int r = row + direction;

            if (inside(r, col) && board[r * 8 + col] == '.') {
                int to = r * 8 + col;
                char promotion = '\0';

                if (r == promotionRow) {
                    promotion = white ? 'Q' : 'q';
                }

                moves.push_back({from, to, promotion});

                int doubleRow = row + direction * 2;

                if (row == startRow && board[doubleRow * 8 + col] == '.') {
                    moves.push_back({from, doubleRow * 8 + col});
                }
            }

            for (int dc : {-1, 1}) {
                int c = col + dc;

                if (!inside(r, c)) continue;

                int to = r * 8 + c;

                if (board[to] != '.' && !sameSide(piece, board[to])) {
                    char promotion = '\0';

                if (r == promotionRow) {
                    promotion = white ? 'Q' : 'q';
                }

                moves.push_back({from, to, promotion});
                } else if (to == enPassantSquare) {
                    Move move;
                    move.from = from;
                    move.to = to;
                    move.enPassant = true;
                    moves.push_back(move);
                }
            }
        }

        else if (lower == 'n') {
            const int knightMoves[8][2] = {
                {-2,-1}, {-2,1}, {-1,-2}, {-1,2},
                {1,-2}, {1,2}, {2,-1}, {2,1}
            };

            for (auto& move : knightMoves) {
                int r = row + move[0];
                int c = col + move[1];

                if (!inside(r, c)) continue;

                int to = r * 8 + c;

                if (board[to] == '.' || !sameSide(piece, board[to])) {
                    moves.push_back({from, to});
                }
            }
        }

        else if (lower == 'b') {
            const int dirs[4][2] = {
                {-1,-1}, {-1,1}, {1,-1}, {1,1}
            };

            addSlidingMoves(moves, from, dirs, 4);
        }

        else if (lower == 'r') {
            const int dirs[4][2] = {
                {-1,0}, {1,0}, {0,-1}, {0,1}
            };

            addSlidingMoves(moves, from, dirs, 4);
        }

        else if (lower == 'q') {
            const int dirs[8][2] = {
                {-1,-1}, {-1,1}, {1,-1}, {1,1},
                {-1,0}, {1,0}, {0,-1}, {0,1}
            };

            addSlidingMoves(moves, from, dirs, 8);
        }

        else if (lower == 'k') {
            for (int dr = -1; dr <= 1; dr++) {
                for (int dc = -1; dc <= 1; dc++) {
                    if (dr == 0 && dc == 0) continue;

                    int r = row + dr;
                    int c = col + dc;

                    if (!inside(r, c)) continue;

                    int to = r * 8 + c;

                    if (board[to] == '.' || !sameSide(piece, board[to])) {
                        moves.push_back({from, to});
                    }
                }
            }

            // castle checks are kept here so illegal through-check castles never get added
            if (white && from == 60 && !whiteKingMoved && !kingInCheck(board, true)) {
                if (!whiteRookHMoved && board[61] == '.' && board[62] == '.' &&
                    board[63] == 'R' && !squareAttacked(board, 61, false) && !squareAttacked(board, 62, false)) {
                    Move move{60, 62};
                    move.castle = true;
                    moves.push_back(move);
                }

                if (!whiteRookAMoved && board[59] == '.' && board[58] == '.' && board[57] == '.' &&
                    board[56] == 'R' && !squareAttacked(board, 59, false) && !squareAttacked(board, 58, false)) {
                    Move move{60, 58};
                    move.castle = true;
                    moves.push_back(move);
                }
            }

            if (!white && from == 4 && !blackKingMoved && !kingInCheck(board, false)) {
                if (!blackRookHMoved && board[5] == '.' && board[6] == '.' &&
                    board[7] == 'r' && !squareAttacked(board, 5, true) && !squareAttacked(board, 6, true)) {
                    Move move{4, 6};
                    move.castle = true;
                    moves.push_back(move);
                }

                if (!blackRookAMoved && board[3] == '.' && board[2] == '.' && board[1] == '.' &&
                    board[0] == 'r' && !squareAttacked(board, 3, true) && !squareAttacked(board, 2, true)) {
                    Move move{4, 2};
                    move.castle = true;
                    moves.push_back(move);
                }
            }
        }
    }

    return moves;
}

void applyMoveTo(std::array<char, 64>& state, const Move& move) {
    char piece = state[move.from];

    state[move.to] = move.promotion ? move.promotion : piece;
    state[move.from] = '.';

    if (move.enPassant) {
        int captured = move.to + (isWhite(piece) ? 8 : -8);
        state[captured] = '.';
    }

    if (move.castle) {
        if (move.to == 62) {
            state[61] = state[63];
            state[63] = '.';
        } else if (move.to == 58) {
            state[59] = state[56];
            state[56] = '.';
        } else if (move.to == 6) {
            state[5] = state[7];
            state[7] = '.';
        } else if (move.to == 2) {
            state[3] = state[0];
            state[0] = '.';
        }
    }
}

std::vector<Move> getLegalMoves(bool white) {
    std::vector<Move> legal;

    for (const Move& move : pseudoMoves(white)) {
        auto test = board;
        applyMoveTo(test, move);

        if (!kingInCheck(test, white)) {
            legal.push_back(move);
        }
    }

    return legal;
}

void updateGameState() {
    auto moves = getLegalMoves(whiteTurn);

    if (moves.empty()) {
        gameOver = true;

        if (kingInCheck(board, whiteTurn)) {
            statusText = whiteTurn ? "Checkmate - Black wins" : "Checkmate - White wins";
        } else {
            statusText = "Stalemate";
        }

        return;
    }

    if (kingInCheck(board, whiteTurn)) {
        statusText = whiteTurn ? "White is in check" : "Black is in check";
    } else {
        statusText = whiteTurn ? "White to move" : "Black to move";
    }
}

void makeMove(const Move& move) {
    char piece = board[move.from];

    if (piece == 'K') whiteKingMoved = true;
    if (piece == 'k') blackKingMoved = true;
    if (move.from == 56 || move.to == 56) whiteRookAMoved = true;
    if (move.from == 63 || move.to == 63) whiteRookHMoved = true;
    if (move.from == 0 || move.to == 0) blackRookAMoved = true;
    if (move.from == 7 || move.to == 7) blackRookHMoved = true;

    enPassantSquare = -1;

    if (tolower(piece) == 'p' && abs(move.to - move.from) == 16) {
        enPassantSquare = (move.from + move.to) / 2;
    }

    applyMoveTo(board, move);

    selected = -1;
    legalMoves.clear();
    whiteTurn = !whiteTurn;
    updateGameState();
}

std::wstring pieceSymbol(char piece) {
    switch (piece) {
        case 'K': return L"\u2654";
        case 'Q': return L"\u2655";
        case 'R': return L"\u2656";
        case 'B': return L"\u2657";
        case 'N': return L"\u2658";
        case 'P': return L"\u2659";
        case 'k': return L"\u265A";
        case 'q': return L"\u265B";
        case 'r': return L"\u265C";
        case 'b': return L"\u265D";
        case 'n': return L"\u265E";
        case 'p': return L"\u265F";
        default: return L"";
    }
}

bool moveTarget(int square) {
    for (const Move& move : legalMoves) {
        if (move.to == square) return true;
    }

    return false;
}

void drawGame(HWND window, HDC dc) {
    RECT client;
    GetClientRect(window, &client);

    HBRUSH background = CreateSolidBrush(RGB(20, 22, 28));
    FillRect(dc, &client, background);
    DeleteObject(background);

    HFONT titleFont = CreateFontW(
        30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI"
    );

    HFONT pieceFont = CreateFontW(
        54, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI Symbol"
    );

    SetBkMode(dc, TRANSPARENT);

    RECT titleRect = {720, 45, 1040, 90};
    SelectObject(dc, titleFont);
    SetTextColor(dc, RGB(235, 238, 245));
    DrawTextW(dc, L"OFFLINE CHESS", -1, &titleRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT statusRect = {720, 105, 1040, 150};
    std::wstring status(statusText.begin(), statusText.end());
    SetTextColor(dc, RGB(170, 178, 195));
    DrawTextW(dc, status.c_str(), -1, &statusRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int square = row * 8 + col;
            RECT rect = {
                boardX + col * squareSize,
                boardY + row * squareSize,
                boardX + (col + 1) * squareSize,
                boardY + (row + 1) * squareSize
            };

            COLORREF color = ((row + col) % 2 == 0)
                ? RGB(222, 226, 232)
                : RGB(83, 91, 107);

            if (square == selected) {
                color = RGB(186, 163, 83);
            }

            HBRUSH squareBrush = CreateSolidBrush(color);
            FillRect(dc, &rect, squareBrush);
            DeleteObject(squareBrush);

            if (moveTarget(square)) {
                int centerX = (rect.left + rect.right) / 2;
                int centerY = (rect.top + rect.bottom) / 2;
                HBRUSH marker = CreateSolidBrush(RGB(93, 184, 122));
                HBRUSH oldBrush = (HBRUSH)SelectObject(dc, marker);
                HPEN pen = CreatePen(PS_NULL, 0, 0);
                HPEN oldPen = (HPEN)SelectObject(dc, pen);

                Ellipse(dc, centerX - 9, centerY - 9, centerX + 9, centerY + 9);

                SelectObject(dc, oldBrush);
                SelectObject(dc, oldPen);
                DeleteObject(marker);
                DeleteObject(pen);
            }

            if (board[square] != '.') {
                SelectObject(dc, pieceFont);
                std::wstring symbol = pieceSymbol(board[square]);

                SetTextColor(dc, isWhite(board[square]) ? RGB(248, 248, 248) : RGB(12, 14, 18));
                DrawTextW(dc, symbol.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
        }
    }

    RECT info = {720, 190, 1050, 370};
    SelectObject(dc, titleFont);
    SetTextColor(dc, RGB(235, 238, 245));
    DrawTextW(dc, L"2 PLAYER LOCAL", -1, &info, DT_LEFT | DT_TOP);

    HFONT smallFont = CreateFontW(
        18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI"
    );

    SelectObject(dc, smallFont);
    SetTextColor(dc, RGB(155, 163, 180));

    RECT textRect = {720, 245, 1040, 430};
    DrawTextW(
        dc,
        L"click a piece then click a highlighted square\n\n"
        L"fully local\n"
        L"no accounts\n"
        L"no networking\n"
        L"no telemetry\n"
        L"no save files",
        -1,
        &textRect,
        DT_LEFT | DT_TOP
    );

    RECT button = {720, 520, 930, 575};
    HBRUSH buttonBrush = CreateSolidBrush(RGB(48, 53, 65));
    FillRect(dc, &button, buttonBrush);
    DeleteObject(buttonBrush);

    SetTextColor(dc, RGB(240, 242, 247));
    DrawTextW(dc, L"NEW GAME", -1, &button, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    DeleteObject(titleFont);
    DeleteObject(pieceFont);
    DeleteObject(smallFont);
}

void clickGame(HWND window, int x, int y) {
    if (x >= 720 && x <= 930 && y >= 520 && y <= 575) {
        resetGame();
        InvalidateRect(window, nullptr, FALSE);
        return;
    }

    if (gameOver) return;

    if (x < boardX || y < boardY) return;

    int col = (x - boardX) / squareSize;
    int row = (y - boardY) / squareSize;

    if (!inside(row, col)) return;

    int square = row * 8 + col;

    if (selected != -1) {
        for (const Move& move : legalMoves) {
            if (move.to == square) {
                makeMove(move);
                InvalidateRect(window, nullptr, FALSE);
                return;
            }
        }
    }

    char piece = board[square];

    if (piece != '.' && isWhite(piece) == whiteTurn) {
        selected = square;
        legalMoves.clear();

        for (const Move& move : getLegalMoves(whiteTurn)) {
            if (move.from == square) {
                legalMoves.push_back(move);
            }
        }
    } else {
        selected = -1;
        legalMoves.clear();
    }

    InvalidateRect(window, nullptr, FALSE);
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_LBUTTONDOWN: {
            int x = static_cast<short>(LOWORD(lParam));
            int y = static_cast<short>(HIWORD(lParam));
            clickGame(window, x, y);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT paint;
            HDC dc = BeginPaint(window, &paint);
            drawGame(window, dc);
            EndPaint(window, &paint);
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    resetGame();

    const wchar_t className[] = L"OfflineChessWindow";

    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&windowClass);

    HWND window = CreateWindowExW(
        0,
        className,
        L"Offline Chess",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1090,
        735,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (!window) return 0;

    ShowWindow(window, show);
    UpdateWindow(window);

    MSG message = {};

    while (GetMessageW(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}
