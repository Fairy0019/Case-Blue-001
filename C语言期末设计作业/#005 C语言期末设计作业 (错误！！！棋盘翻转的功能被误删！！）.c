#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_USERNAME 20
#define MAX_PASSWORD 20

typedef struct
{
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} User;

#define BOARD_SIZE 8

// 全局变量 - 棋盘始终白方在下（不翻转）
char board[BOARD_SIZE][BOARD_SIZE];
int whiteToMove = 1;
int whiteCastleKingSide = 1;
int whiteCastleQueenSide = 1;
int blackCastleKingSide = 1;
int blackCastleQueenSide = 1;
int enPassantValid = 0;
int enPassantRow = -1;
int enPassantCol = -1;

User currentUser = {0};

#define MAX_MOVES 200
char currentGameMoves[MAX_MOVES][10];
int moveCount;

// 函数原型声明
void initBoard(void);
int parseSquare(const char *token, int *row, int *col);
void printBoard(void);
void printInstructions(void);
int executeMove(int sr, int sc, int dr, int dc, int white);
int isKingInCheck(int white);
int hasLegalMove(int white);
void saveGameMoves(const char *result);
void saveUserStats(int win, int loss, int draw, int quit);
void replayGame(int gameIndex);

// 辅助函数
int isWhite(char piece) { return piece >= 'A' && piece <= 'Z'; }
int isBlack(char piece) { return piece >= 'a' && piece <= 'z'; }
int onBoard(int r, int c) { return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE; }

void trimNewline(char *str)
{
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
        str[len - 1] = '\0';
}

int userExists(const char *username)
{
    FILE *fp = fopen("users.txt", "r");
    if (!fp) return 0;
    char line[100];
    while (fgets(line, sizeof(line), fp))
    {
        char name[MAX_USERNAME];
        sscanf(line, "%s", name);
        if (strcmp(name, username) == 0) { fclose(fp); return 1; }
    }
    fclose(fp);
    return 0;
}

void registerUser()
{
    char username[MAX_USERNAME], pwd1[MAX_PASSWORD], pwd2[MAX_PASSWORD];
    printf("\n===== 注册新账户 =====\n");
    printf("用户名: ");
    fgets(username, MAX_USERNAME, stdin);
    trimNewline(username);
    if (userExists(username)) { printf("用户名已存在！\n"); return; }
    printf("密码: ");
    fgets(pwd1, MAX_PASSWORD, stdin);
    trimNewline(pwd1);
    printf("确认密码: ");
    fgets(pwd2, MAX_PASSWORD, stdin);
    trimNewline(pwd2);
    if (strcmp(pwd1, pwd2) != 0) { printf("两次密码不一致！\n"); return; }
    FILE *fp = fopen("users.txt", "a");
    if (!fp) { printf("无法打开用户文件！\n"); return; }
    fprintf(fp, "%s %s\n", username, pwd1);
    fclose(fp);
    printf("注册成功！\n");
}

int login()
{
    char username[MAX_USERNAME], password[MAX_PASSWORD];
    printf("\n===== 用户登录 =====\n");
    printf("用户名: ");
    fgets(username, MAX_USERNAME, stdin);
    trimNewline(username);
    printf("密码: ");
    fgets(password, MAX_PASSWORD, stdin);
    trimNewline(password);
    FILE *fp = fopen("users.txt", "r");
    if (!fp) { printf("用户文件不存在，请先注册。\n"); return 0; }
    char line[100];
    while (fgets(line, sizeof(line), fp))
    {
        char u[MAX_USERNAME], p[MAX_PASSWORD];
        sscanf(line, "%s %s", u, p);
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0)
        {
            strcpy(currentUser.username, username);
            strcpy(currentUser.password, password);
            fclose(fp);
            printf("登录成功！欢迎 %s\n", username);
            return 1;
        }
    }
    fclose(fp);
    printf("用户名或密码错误！\n");
    return 0;
}

void changePassword()
{
    if (currentUser.username[0] == '\0') { printf("请先登录！\n"); return; }
    char oldPwd[MAX_PASSWORD], newPwd1[MAX_PASSWORD], newPwd2[MAX_PASSWORD];
    printf("\n===== 修改密码 =====\n");
    printf("原密码: ");
    fgets(oldPwd, MAX_PASSWORD, stdin);
    trimNewline(oldPwd);
    if (strcmp(oldPwd, currentUser.password) != 0) { printf("原密码错误！\n"); return; }
    printf("新密码: ");
    fgets(newPwd1, MAX_PASSWORD, stdin);
    trimNewline(newPwd1);
    printf("确认新密码: ");
    fgets(newPwd2, MAX_PASSWORD, stdin);
    trimNewline(newPwd2);
    if (strcmp(newPwd1, newPwd2) != 0) { printf("两次新密码不一致！\n"); return; }
    FILE *fp = fopen("users.txt", "r");
    if (!fp) { printf("无法打开用户文件！\n"); return; }
    FILE *tmp = fopen("users.tmp", "w");
    if (!tmp) { fclose(fp); printf("临时文件创建失败！\n"); return; }
    char line[100];
    int updated = 0;
    while (fgets(line, sizeof(line), fp))
    {
        char u[MAX_USERNAME], p[MAX_PASSWORD];
        sscanf(line, "%s %s", u, p);
        if (strcmp(u, currentUser.username) == 0)
        {
            fprintf(tmp, "%s %s\n", currentUser.username, newPwd1);
            strcpy(currentUser.password, newPwd1);
            updated = 1;
        }
        else fputs(line, tmp);
    }
    fclose(fp); fclose(tmp);
    remove("users.txt");
    rename("users.tmp", "users.txt");
    if (updated) printf("密码修改成功！\n");
    else printf("修改失败，用户不存在？\n");
}

void logout()
{
    if (currentUser.username[0] == '\0') printf("尚未登录任何账户。\n");
    else
    {
        printf("用户 %s 已退出登录。\n", currentUser.username);
        currentUser.username[0] = '\0';
        currentUser.password[0] = '\0';
    }
}

void loadUserStats()
{
    if (currentUser.username[0] == '\0') return;
    char filename[50];
    sprintf(filename, "stats_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "r");
    if (!fp) { printf("\n暂无战绩记录。\n"); return; }
    char line[100];
    if (fgets(line, sizeof(line), fp)) printf("\n=== %s 的战绩 ===\n%s", currentUser.username, line);
    else printf("\n暂无战绩记录。\n");
    fclose(fp);
}

void saveUserStats(int win, int loss, int draw, int quit)
{
    char filename[50];
    sprintf(filename, "stats_%s.txt", currentUser.username);
    int old_win = 0, old_loss = 0, old_draw = 0, old_quit = 0;
    FILE *fp = fopen(filename, "r");
    if (fp) { fscanf(fp, "赢:%d 输:%d 平:%d 中途退出:%d", &old_win, &old_loss, &old_draw, &old_quit); fclose(fp); }
    old_win += win; old_loss += loss; old_draw += draw; old_quit += quit;
    fp = fopen(filename, "w");
    if (fp) { fprintf(fp, "赢:%d 输:%d 平:%d 中途退出:%d\n", old_win, old_loss, old_draw, old_quit); fclose(fp); }
}

void saveGameMoves(const char *result)
{
    if (moveCount == 0) return;
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "a");
    if (!fp) { printf("无法保存棋谱！\n"); return; }
    fprintf(fp, "[GAME_START]\n");
    for (int i = 0; i < moveCount; i++) fprintf(fp, "%d. %s\n", i + 1, currentGameMoves[i]);
    fprintf(fp, "[RESULT] %s\n", result);
    fprintf(fp, "[GAME_END]\n\n");
    fclose(fp);
}

void showStats()
{
    if (currentUser.username[0] == '\0') { printf("请先登录！\n"); return; }
    char filename[50];
    sprintf(filename, "stats_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "r");
    if (!fp) { printf("玩家 %s 暂无战绩记录。\n", currentUser.username); return; }
    char line[100];
    if (fgets(line, sizeof(line), fp)) printf("\n=== 玩家 %s 的战绩 ===\n%s", currentUser.username, line);
    else printf("玩家 %s 暂无战绩记录。\n", currentUser.username);
    fclose(fp);
}

void listGames()
{
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "r");
    if (!fp) { printf("没有找到任何历史对局。\n"); return; }
    char line[200];
    int gameNum = 1, inGame = 0;
    char result[50] = "";
    printf("\n=== 历史对局列表 ===\n");
    while (fgets(line, sizeof(line), fp))
    {
        if (strstr(line, "[GAME_START]")) { inGame = 1; result[0] = '\0'; }
        else if (strstr(line, "[RESULT]") && inGame) sscanf(line, "[RESULT] %[^\n]", result);
        else if (strstr(line, "[GAME_END]") && inGame) { printf("%d. 结果: %s\n", gameNum++, result); inGame = 0; }
    }
    fclose(fp);
    if (gameNum == 1) printf("暂无对局记录。\n");
}

void replayGame(int gameIndex)
{
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "r");
    if (!fp) { printf("无法打开棋谱文件。\n"); return; }
    char line[200];
    int curGame = 0, inTarget = 0, moveCnt = 0;
    char moves[MAX_MOVES][10];
    char result[50] = "";
    while (fgets(line, sizeof(line), fp))
    {
        if (strstr(line, "[GAME_START]"))
        {
            curGame++;
            if (curGame == gameIndex) { inTarget = 1; moveCnt = 0; result[0] = '\0'; continue; }
        }
        if (inTarget)
        {
            if (strstr(line, "[RESULT]")) sscanf(line, "[RESULT] %[^\n]", result);
            else if (strstr(line, "[GAME_END]")) break;
            else
            {
                char move[10];
                if (sscanf(line, "%*d. %s", move) == 1 && moveCnt < MAX_MOVES) strcpy(moves[moveCnt++], move);
            }
        }
    }
    fclose(fp);
    if (!inTarget || moveCnt == 0) { printf("未找到指定对局或对局为空。\n"); return; }
    printf("\n开始回放第 %d 局（结果：%s）\n", gameIndex, result);
    printf("按回车键播放下一步，输入 q 退出回放。\n");
    char backupBoard[BOARD_SIZE][BOARD_SIZE];
    int bWhiteToMove = whiteToMove, bWCK = whiteCastleKingSide, bWCQ = whiteCastleQueenSide;
    int bBCK = blackCastleKingSide, bBCQ = blackCastleQueenSide;
    int bEPV = enPassantValid, bEPR = enPassantRow, bEPC = enPassantCol;
    memcpy(backupBoard, board, sizeof(board));
    initBoard();
    for (int i = 0; i < moveCnt; i++)
    {
        char src[3] = {moves[i][0], moves[i][1], '\0'};
        char dst[3] = {moves[i][2], moves[i][3], '\0'};
        int sr, sc, dr, dc;
        if (!parseSquare(src, &sr, &sc) || !parseSquare(dst, &dr, &dc)) { printf("棋谱解析错误，停止回放。\n"); break; }
        char piece = board[sr][sc];
        if (piece == '.') { printf("回放错误：起始格无棋子！\n"); break; }
        board[dr][dc] = piece;
        board[sr][sc] = '.';
        if (tolower(piece) == 'k' && abs(dc - sc) == 2)
        {
            int isWhiteMove = (i % 2 == 0);
            if (isWhiteMove)
            {
                if (dc == 6) { board[7][5] = 'R'; board[7][7] = '.'; }
                else if (dc == 2) { board[7][3] = 'R'; board[7][0] = '.'; }
            }
            else
            {
                if (dc == 6) { board[0][5] = 'r'; board[0][7] = '.'; }
                else if (dc == 2) { board[0][3] = 'r'; board[0][0] = '.'; }
            }
        }
        if (tolower(piece) == 'p' && (dr == 0 || dr == 7)) board[dr][dc] = (i % 2 == 0) ? 'Q' : 'q';
        if (tolower(piece) == 'p' && abs(dc - sc) == 1 && board[dr][dc] == '.')
        {
            int captureRow = (i % 2 == 0) ? dr + 1 : dr - 1;
            if (captureRow >= 0 && captureRow < BOARD_SIZE) board[captureRow][dc] = '.';
        }
        printBoard();
        printf("第 %d 步：%s\n", i + 1, moves[i]);
        char input[10];
        fgets(input, sizeof(input), stdin);
        if (input[0] == 'q' || input[0] == 'Q') { printf("用户中断回放。\n"); break; }
    }
    memcpy(board, backupBoard, sizeof(board));
    whiteToMove = bWhiteToMove;
    whiteCastleKingSide = bWCK; whiteCastleQueenSide = bWCQ;
    blackCastleKingSide = bBCK; blackCastleQueenSide = bBCQ;
    enPassantValid = bEPV; enPassantRow = bEPR; enPassantCol = bEPC;
    printf("对局结果：%s\n", result);
    printf("回放结束。\n");
}

void initBoard() {
    const char *start[8] = { "rnbqkbnr", "pppppppp", "........", "........", "........", "........", "PPPPPPPP", "RNBQKBNR" };
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            board[r][c] = start[r][c];
    whiteToMove = 1;
    whiteCastleKingSide = whiteCastleQueenSide = 1;
    blackCastleKingSide = blackCastleQueenSide = 1;
    enPassantValid = 0;
    enPassantRow = enPassantCol = -1;
}

int parseSquare(const char *token, int *row, int *col) {
    if (strlen(token) < 2) return 0;
    char file = token[0], rank = token[1];
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') return 0;
    *col = file - 'a';
    *row = 8 - (rank - '0');
    return 1;
}

int isFriendlyPiece(char piece, int white) {
    if (piece == '.') return 0;
    return white ? isWhite(piece) : isBlack(piece);
}
int isEnemyPiece(char piece, int white) {
    if (piece == '.') return 0;
    return white ? isBlack(piece) : isWhite(piece);
}

int clearPath(int sr, int sc, int dr, int dc) {
    int drStep = (dr > sr) ? 1 : (dr < sr) ? -1 : 0;
    int dcStep = (dc > sc) ? 1 : (dc < sc) ? -1 : 0;
    int r = sr + drStep, c = sc + dcStep;
    while (r != dr || c != dc) {
        if (board[r][c] != '.') return 0;
        r += drStep; c += dcStep;
    }
    return 1;
}

int canPawnAttack(int sr, int sc, int dr, int dc, int white) {
    int direction = white ? -1 : 1;
    return dr == sr + direction && abs(dc - sc) == 1;
}

int canMovePawn(int sr, int sc, int dr, int dc, int white) {
    int direction = white ? -1 : 1;
    char target = board[dr][dc];
    int startRow = white ? 6 : 1;
    if (sc == dc && target == '.') {
        if (dr == sr + direction) return 1;
        if (sr == startRow && dr == sr + 2 * direction && board[sr + direction][sc] == '.') return 1;
    }
    if (abs(dc - sc) == 1 && dr == sr + direction) {
        if (isEnemyPiece(target, white)) return 1;
        if (enPassantValid && dr == enPassantRow && dc == enPassantCol) return 1;
    }
    return 0;
}

int canMoveKnight(int sr, int sc, int dr, int dc) {
    int drAbs = abs(dr - sr), dcAbs = abs(dc - sc);
    return (drAbs == 2 && dcAbs == 1) || (drAbs == 1 && dcAbs == 2);
}
int canMoveBishop(int sr, int sc, int dr, int dc) {
    if (abs(dr - sr) != abs(dc - sc)) return 0;
    return clearPath(sr, sc, dr, dc);
}
int canMoveRook(int sr, int sc, int dr, int dc) {
    if (sr != dr && sc != dc) return 0;
    return clearPath(sr, sc, dr, dc);
}
int canMoveQueen(int sr, int sc, int dr, int dc) {
    return canMoveBishop(sr, sc, dr, dc) || canMoveRook(sr, sc, dr, dc);
}
int squareAttacked(int row, int col, int byWhite);
int isKingInCheck(int white);

int canCastle(int sr, int sc, int dr, int dc, int white) {
    if (isKingInCheck(white)) return 0;
    if (white) {
        if (sr != 7 || sc != 4) return 0;
        if (dr == 7 && dc == 6) {
            if (!whiteCastleKingSide) return 0;
            if (board[7][5] != '.' || board[7][6] != '.') return 0;
            if (squareAttacked(7,5,0) || squareAttacked(7,6,0)) return 0;
            if (board[7][7] != 'R') return 0;
            return 1;
        }
        if (dr == 7 && dc == 2) {
            if (!whiteCastleQueenSide) return 0;
            if (board[7][3] != '.' || board[7][2] != '.' || board[7][1] != '.') return 0;
            if (squareAttacked(7,3,0) || squareAttacked(7,2,0)) return 0;
            if (board[7][0] != 'R') return 0;
            return 1;
        }
    } else {
        if (sr != 0 || sc != 4) return 0;
        if (dr == 0 && dc == 6) {
            if (!blackCastleKingSide) return 0;
            if (board[0][5] != '.' || board[0][6] != '.') return 0;
            if (squareAttacked(0,5,1) || squareAttacked(0,6,1)) return 0;
            if (board[0][7] != 'r') return 0;
            return 1;
        }
        if (dr == 0 && dc == 2) {
            if (!blackCastleQueenSide) return 0;
            if (board[0][3] != '.' || board[0][2] != '.' || board[0][1] != '.') return 0;
            if (squareAttacked(0,3,1) || squareAttacked(0,2,1)) return 0;
            if (board[0][0] != 'r') return 0;
            return 1;
        }
    }
    return 0;
}

int canMoveKing(int sr, int sc, int dr, int dc) {
    int drAbs = abs(dr - sr), dcAbs = abs(dc - sc);
    if (drAbs == 0 && dcAbs == 2) return canCastle(sr, sc, dr, dc, isWhite(board[sr][sc]));
    return drAbs <= 1 && dcAbs <= 1;
}

int squareAttacked(int row, int col, int byWhite) {
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c) {
            char piece = board[r][c];
            if (!isFriendlyPiece(piece, byWhite)) continue;
            int can = 0;
            char lower = tolower(piece);
            switch (lower) {
                case 'p': can = canPawnAttack(r, c, row, col, byWhite); break;
                case 'n': can = canMoveKnight(r, c, row, col); break;
                case 'b': can = canMoveBishop(r, c, row, col); break;
                case 'r': can = canMoveRook(r, c, row, col); break;
                case 'q': can = canMoveQueen(r, c, row, col); break;
                case 'k': can = canMoveKing(r, c, row, col); break;
            }
            if (can) return 1;
        }
    return 0;
}

int isKingInCheck(int white) {
    int kingRow = -1, kingCol = -1;
    char king = white ? 'K' : 'k';
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            if (board[r][c] == king) { kingRow = r; kingCol = c; break; }
    if (kingRow == -1) return 1;
    return squareAttacked(kingRow, kingCol, !white);
}

int canMovePiece(int sr, int sc, int dr, int dc, int white) {
    if (!onBoard(sr, sc) || !onBoard(dr, dc)) return 0;
    char piece = board[sr][sc];
    if (piece == '.' || !isFriendlyPiece(piece, white)) return 0;
    if (isFriendlyPiece(board[dr][dc], white)) return 0;
    char lower = tolower(piece);
    switch (lower) {
        case 'p': return canMovePawn(sr, sc, dr, dc, white);
        case 'n': return canMoveKnight(sr, sc, dr, dc);
        case 'b': return canMoveBishop(sr, sc, dr, dc);
        case 'r': return canMoveRook(sr, sc, dr, dc);
        case 'q': return canMoveQueen(sr, sc, dr, dc);
        case 'k': return canMoveKing(sr, sc, dr, dc);
        default: return 0;
    }
}

int executeMove(int sr, int sc, int dr, int dc, int white) {
    if (!canMovePiece(sr, sc, dr, dc, white)) return 0;
    char piece = board[sr][sc];
    char target = board[dr][dc];
    char backup[BOARD_SIZE][BOARD_SIZE];
    int bWCK = whiteCastleKingSide, bWCQ = whiteCastleQueenSide;
    int bBCK = blackCastleKingSide, bBCQ = blackCastleQueenSide;
    int bEPV = enPassantValid, bEPR = enPassantRow, bEPC = enPassantCol;
    memcpy(backup, board, sizeof(board));

    // 易位移动车
    if (tolower(piece) == 'k' && abs(dc - sc) == 2) {
        if (white) {
            if (dc == 6) { board[7][5] = 'R'; board[7][7] = '.'; }
            else if (dc == 2) { board[7][3] = 'R'; board[7][0] = '.'; }
            whiteCastleKingSide = whiteCastleQueenSide = 0;
        } else {
            if (dc == 6) { board[0][5] = 'r'; board[0][7] = '.'; }
            else if (dc == 2) { board[0][3] = 'r'; board[0][0] = '.'; }
            blackCastleKingSide = blackCastleQueenSide = 0;
        }
    }
    // 移动车导致易位权失效
    if (tolower(piece) == 'r') {
        if (white) {
            if (sr == 7 && sc == 0) whiteCastleQueenSide = 0;
            if (sr == 7 && sc == 7) whiteCastleKingSide = 0;
        } else {
            if (sr == 0 && sc == 0) blackCastleQueenSide = 0;
            if (sr == 0 && sc == 7) blackCastleKingSide = 0;
        }
    }
    // 吃车导致易位权失效
    if (tolower(target) == 'r') {
        if (target == 'R') {
            if (dr == 7 && dc == 0) whiteCastleQueenSide = 0;
            if (dr == 7 && dc == 7) whiteCastleKingSide = 0;
        } else {
            if (dr == 0 && dc == 0) blackCastleQueenSide = 0;
            if (dr == 0 && dc == 7) blackCastleKingSide = 0;
        }
    }

    board[dr][dc] = piece;
    board[sr][sc] = '.';

    // 吃过路兵
    if (tolower(piece) == 'p' && abs(dr - sr) == 1 && abs(dc - sc) == 1 && target == '.'
        && enPassantValid && dr == enPassantRow && dc == enPassantCol) {
        int captureRow = white ? dr + 1 : dr - 1;
        board[captureRow][dc] = '.';
    }

    // 设置过路兵标志
    if (tolower(piece) == 'p' && abs(dr - sr) == 2) {
        enPassantValid = 1;
        enPassantRow = (sr + dr) / 2;
        enPassantCol = sc;
    } else {
        enPassantValid = 0;
    }

    if (isKingInCheck(white)) {
        memcpy(board, backup, sizeof(board));
        whiteCastleKingSide = bWCK; whiteCastleQueenSide = bWCQ;
        blackCastleKingSide = bBCK; blackCastleQueenSide = bBCQ;
        enPassantValid = bEPV; enPassantRow = bEPR; enPassantCol = bEPC;
        return 0;
    }

    // 兵升变（严格判断：必须是兵且到达底线）
    if (tolower(piece) == 'p' && (dr == 0 || dr == 7)) {
        // 再次确认移动的棋子是兵（以防万一）
        if (tolower(board[dr][dc]) != 'p') {
            printf("升变错误：目标格不是兵，请报告bug。\n");
            return 0;
        }
        char promote[8] = {0};
        printf("兵升变！请选择升变棋子：Q(后) R(车) N(马) B(象) : ");
        while (1) {
            fgets(promote, sizeof(promote), stdin);
            if (promote[0] == '\n') continue;
            char choice = toupper(promote[0]);
            if (choice == 'Q' || choice == 'R' || choice == 'N' || choice == 'B') {
                board[dr][dc] = white ? choice : tolower(choice);
                break;
            }
            printf("无效输入，请输入 Q, R, N, B : ");
        }
    }
    return 1;
}

int hasLegalMove(int white) {
    char backup[BOARD_SIZE][BOARD_SIZE];
    int bWCK = whiteCastleKingSide, bWCQ = whiteCastleQueenSide;
    int bBCK = blackCastleKingSide, bBCQ = blackCastleQueenSide;
    int bEPV = enPassantValid, bEPR = enPassantRow, bEPC = enPassantCol;
    for (int sr = 0; sr < BOARD_SIZE; ++sr)
        for (int sc = 0; sc < BOARD_SIZE; ++sc)
            if (isFriendlyPiece(board[sr][sc], white))
                for (int dr = 0; dr < BOARD_SIZE; ++dr)
                    for (int dc = 0; dc < BOARD_SIZE; ++dc)
                        if (canMovePiece(sr, sc, dr, dc, white)) {
                            memcpy(backup, board, sizeof(board));
                            int moved = executeMove(sr, sc, dr, dc, white);
                            memcpy(board, backup, sizeof(board));
                            whiteCastleKingSide = bWCK; whiteCastleQueenSide = bWCQ;
                            blackCastleKingSide = bBCK; blackCastleQueenSide = bBCQ;
                            enPassantValid = bEPV; enPassantRow = bEPR; enPassantCol = bEPC;
                            if (moved) return 1;
                        }
    return 0;
}

// 固定棋盘显示：白方在下（行1为白方底线，行8为黑方底线）
void printBoard() {
    printf("    a   b   c   d   e   f   g   h\n");
    printf("  +---+---+---+---+---+---+---+---+\n");
    for (int r = 0; r < BOARD_SIZE; ++r) {
        printf("%d |", 8 - r);
        for (int c = 0; c < BOARD_SIZE; ++c) printf(" %c |", board[r][c]);
        printf(" %d\n", 8 - r);
        printf("  +---+---+---+---+---+---+---+---+\n");
    }
    printf("    a   b   c   d   e   f   g   h\n");
}

void printInstructions() {
    printf("国际象棋游戏规则：\n");
    printf("- 每方交替移动一枚棋子。\n");
    printf("- 兵直走一格，首步可走两格；斜吃敌方棋子。\n");
    printf("- 马走日字，越过棋子。\n");
    printf("- 象走斜线，车走直线，后可走直线和斜线。\n");
    printf("- 王可走一格，不能将自己置于将军。\n");
    printf("- 王可进行王车易位：白方 e1->g1/h1 或 e1->c1，黑方 e8->g8/h8。\n");
    printf("- 吃过路兵：如果敌方兵首走两格通过你的兵，下一步可沿对角线吃掉它。\n");
    printf("- 兵升变到对方底线可升变为后、车、马或象。\n");
    printf("- 失去所有合法着法且王在将军判负；无将军但无合法着法判为僵局。\n");
    printf("输入格式示例：e2 e4 或 e7e5。输入 quit 结束游戏。\n\n");
}

void playGame()
{
    initBoard();
    printf("当前登录玩家：%s\n", currentUser.username);
    moveCount = 0;
    printf("\n新游戏开始！\n");
    printInstructions();
    char line[64];
    while (1)
    {
        printBoard();
        printf("%s 走棋：", whiteToMove ? "白方" : "黑方");
        if (!fgets(line, sizeof(line), stdin)) break;
        // 移除换行
        line[strcspn(line, "\n")] = 0;
        char src[8] = {0}, dst[8] = {0};
        if (sscanf(line, "%7s %7s", src, dst) < 1) continue;
        if (strcmp(src, "quit") == 0 || strcmp(src, "exit") == 0)
        {
            const char *quitResult = whiteToMove ? "白方中途退出" : "黑方中途退出";
            printf("游戏结束（%s）。\n", quitResult);
            if (moveCount > 0) { saveGameMoves(quitResult); saveUserStats(0, 0, 0, 1); }
            return;
        }
        // 支持 "e2e4" 格式
        if (strlen(src) == 4 && dst[0] == '\0')
        {
            dst[0] = src[2]; dst[1] = src[3]; dst[2] = '\0';
            src[2] = '\0';
        }
        int sr, sc, dr, dc;
        if (!parseSquare(src, &sr, &sc) || !parseSquare(dst, &dr, &dc))
        { printf("无效输入，请使用 a1 到 h8 的坐标。\n"); continue; }

        if (!executeMove(sr, sc, dr, dc, whiteToMove))
        { printf("非法移动！请检查棋子颜色和走法。\n"); continue; }

        // 记录棋谱
        char moveStr[10];
        sprintf(moveStr, "%c%c%c%c", 'a'+sc, '8'-sr, 'a'+dc, '8'-dr);
        if (moveCount < MAX_MOVES) strcpy(currentGameMoves[moveCount++], moveStr);

        // 胜负判断
        if (isKingInCheck(!whiteToMove))
        {
            if (!hasLegalMove(!whiteToMove))
            {
                printBoard();
                printf("将死！%s 胜利。\n", whiteToMove ? "白方" : "黑方");
                if (whiteToMove) { saveGameMoves("白方赢"); saveUserStats(1, 0, 0, 0); }
                else { saveGameMoves("黑方赢"); saveUserStats(0, 1, 0, 0); }
                return;
            }
            printf("将军！\n");
        }
        else if (!hasLegalMove(!whiteToMove))
        {
            printBoard();
            printf("僵局，平局。\n");
            saveGameMoves("平局");
            saveUserStats(0, 0, 1, 0);
            return;
        }
        whiteToMove = !whiteToMove;
    }
    printf("游戏结束。\n");
}

int main()
{
    printf("===== 国际象棋游戏（带账户系统+战绩回放）=====\n");
    currentUser.username[0] = '\0';
    char line[64];
    while (1)
    {
        printf("\n");
        if (currentUser.username[0] == '\0')
        {
            printf("=== 主菜单（未登录）===\n");
            printf("1. 登录\n2. 注册\n3. 退出程序\n请选择：");
        }
        else
        {
            printf("=== 主菜单（当前用户：%s）===\n", currentUser.username);
            printf("1. 修改密码\n2. 退出登录\n3. 开始新游戏\n4. 游戏规则\n5. 查看战绩\n6. 历史对局\n7. 退出程序\n请选择：");
        }
        if (!fgets(line, sizeof(line), stdin)) break;
        int choice = atoi(line);
        if (currentUser.username[0] == '\0')
        {
            switch (choice)
            {
                case 1: login(); break;
                case 2: registerUser(); break;
                case 3: printf("感谢使用，再见！\n"); return 0;
                default: printf("无效选择，请输入 1、2 或 3。\n");
            }
        }
        else
        {
            switch (choice)
            {
                case 1: changePassword(); break;
                case 2: logout(); break;
                case 3: playGame(); break;
                case 4: printInstructions(); break;
                case 5: showStats(); break;
                case 6:
                {
                    listGames();
                    printf("请输入要回放的对局序号（输入0返回）：");
                    int idx;
                    if (scanf("%d", &idx) == 1 && idx > 0) replayGame(idx);
                    while (getchar() != '\n');
                    break;
                }
                case 7: printf("感谢使用，再见！\n"); return 0;
                default: printf("无效选择，请输入 1 ~ 7。\n");
            }
        }
    }
    return 0;
}

// 我发现中途直接关闭程序，就是不输入quit，直接点击叉掉。好像也不会有记录，没有txt文档记录。
// 这一点咋搞？是加入"直接取消程序，无法记录。"还是加入，即使取消了程序也能记录， 记录为"在黑/白方行动时，取消了程序进程。"？