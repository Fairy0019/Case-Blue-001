#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_USERNAME 20
#define MAX_PASSWORD 20
#define BOARD_SIZE   8
#define MAX_MOVES    200

// ---------- 游戏状态结构体 ----------
typedef struct {
    char board[BOARD_SIZE][BOARD_SIZE];
    int whiteToMove;
    int whiteCastleKingSide, whiteCastleQueenSide;
    int blackCastleKingSide, blackCastleQueenSide;
    int enPassantValid;
    int enPassantRow, enPassantCol;
    int moveCount;
    char currentGameMoves[MAX_MOVES][10];
} GameState;

// ---------- 用户 ----------
typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} User;

// ---------- 未完成对局记录 ----------
typedef struct {
    long startPos;               // 文件中的起始位置（保留但暂未使用）
    char moves[MAX_MOVES][10];
    int moveCount;
    char result[50];
} UnfinishedGame;

// ---------- 全局变量 ----------
GameState game;
int viewMode = 1;        // 1=白方在下 2=黑方在下
User currentUser = {0};

// ---------- 函数原型 ----------
void initBoard(void);
int parseSquare(const char *token, int *row, int *col);
int parseMoveString(const char *moveStr, char *src, char *dst, char *promote);
void printBoard(void);
void printInstructions(void);
void toggleViewMode(void);
int executeMove(int sr, int sc, int dr, int dc, int white, int silentPromotion);
int isKingInCheck(int white);
int hasLegalMove(int white);
void saveGameMoves(const char *result);
void saveUserStats(int win, int loss, int draw, int quit);
void replayGame(int gameIndex);
void saveCurrentMove(const char *moveStr, int stepNo);
void checkAndResumeUnfinishedGame(void);
int loadGameFromMoves(const char *moves[], int moveCnt);
void continueGame(void);
void newGame(void);
void manageUnfinishedGames(void);

int isWhite(char piece) { return piece >= 'A' && piece <= 'Z'; }
int isBlack(char piece) { return piece >= 'a' && piece <= 'z'; }
int onBoard(int r, int c) { return r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE; }
void trimNewline(char *str) {
    size_t len = strlen(str);
    if (len && str[len-1] == '\n') str[len-1] = '\0';
}

// ---------- 用户管理 ----------
int userExists(const char *username) {
    FILE *fp = fopen("users.txt", "r");
    if (!fp) return 0;
    char line[100];
    while (fgets(line, sizeof(line), fp)) {
        char name[MAX_USERNAME];
        sscanf(line, "%s", name);
        if (strcmp(name, username) == 0) { fclose(fp); return 1; }
    }
    fclose(fp);
    return 0;
}

void registerUser() {
    char username[MAX_USERNAME], pwd1[MAX_PASSWORD], pwd2[MAX_PASSWORD];
    printf("\n===== 注册新账户 =====\n用户名: ");
    fgets(username, MAX_USERNAME, stdin);
    trimNewline(username);
    if (userExists(username)) {
        printf("用户名已存在！\n");
        return;
    }
    printf("密码: ");
    fgets(pwd1, MAX_PASSWORD, stdin);
    trimNewline(pwd1);
    printf("确认密码: ");
    fgets(pwd2, MAX_PASSWORD, stdin);
    trimNewline(pwd2);
    if (strcmp(pwd1, pwd2) != 0) {
        printf("两次密码不一致！\n");
        return;
    }
    FILE *fp = fopen("users.txt", "a");
    if (!fp) { printf("无法打开用户文件！\n"); return; }
    fprintf(fp, "%s %s\n", username, pwd1);
    fclose(fp);
    printf("注册成功！\n");
}

int login() {
    char username[MAX_USERNAME], password[MAX_PASSWORD];
    printf("\n===== 用户登录 =====\n用户名: ");
    fgets(username, MAX_USERNAME, stdin);
    trimNewline(username);
    printf("密码: ");
    fgets(password, MAX_PASSWORD, stdin);
    trimNewline(password);
    FILE *fp = fopen("users.txt", "r");
    if (!fp) { printf("用户文件不存在，请先注册。\n"); return 0; }
    char line[100];
    while (fgets(line, sizeof(line), fp)) {
        char u[MAX_USERNAME], p[MAX_PASSWORD];
        sscanf(line, "%s %s", u, p);
        if (strcmp(u, username) == 0 && strcmp(p, password) == 0) {
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

void changePassword() {
    if (currentUser.username[0] == '\0') {
        printf("请先登录！\n");
        return;
    }
    char oldPwd[MAX_PASSWORD], newPwd1[MAX_PASSWORD], newPwd2[MAX_PASSWORD];
    printf("\n===== 修改密码 =====\n原密码: ");
    fgets(oldPwd, MAX_PASSWORD, stdin);
    trimNewline(oldPwd);
    if (strcmp(oldPwd, currentUser.password) != 0) {
        printf("原密码错误！\n");
        return;
    }
    printf("新密码: ");
    fgets(newPwd1, MAX_PASSWORD, stdin);
    trimNewline(newPwd1);
    printf("确认新密码: ");
    fgets(newPwd2, MAX_PASSWORD, stdin);
    trimNewline(newPwd2);
    if (strcmp(newPwd1, newPwd2) != 0) {
        printf("两次新密码不一致！\n");
        return;
    }
    FILE *fp = fopen("users.txt", "r");
    if (!fp) { printf("无法打开用户文件！\n"); return; }
    FILE *tmp = fopen("users.tmp", "w");
    if (!tmp) { fclose(fp); printf("临时文件创建失败！\n"); return; }
    char line[100];
    int updated = 0;
    while (fgets(line, sizeof(line), fp)) {
        char u[MAX_USERNAME], p[MAX_PASSWORD];
        sscanf(line, "%s %s", u, p);
        if (strcmp(u, currentUser.username) == 0) {
            fprintf(tmp, "%s %s\n", currentUser.username, newPwd1);
            strcpy(currentUser.password, newPwd1);
            updated = 1;
        } else fputs(line, tmp);
    }
    fclose(fp); fclose(tmp);
    remove("users.txt");
    rename("users.tmp", "users.txt");
    printf(updated ? "密码修改成功！\n" : "修改失败，用户不存在？\n");
}

void logout() {
    if (currentUser.username[0] == '\0')
        printf("尚未登录任何账户。\n");
    else {
        printf("用户 %s 已退出登录。\n", currentUser.username);
        currentUser.username[0] = '\0';
        currentUser.password[0] = '\0';
    }
}

// ---------- 战绩与棋谱持久化 ----------
void saveUserStats(int win, int loss, int draw, int quit) {
    char filename[50];
    sprintf(filename, "stats_%s.txt", currentUser.username);
    int old_win = 0, old_loss = 0, old_draw = 0, old_quit = 0;
    FILE *fp = fopen(filename, "r");
    if (fp) {
        fscanf(fp, "赢:%d 输:%d 平:%d 中途退出:%d", &old_win, &old_loss, &old_draw, &old_quit);
        fclose(fp);
    }
    old_win += win; old_loss += loss; old_draw += draw; old_quit += quit;
    fp = fopen(filename, "w");
    if (fp) {
        fprintf(fp, "赢:%d 输:%d 平:%d 中途退出:%d\n", old_win, old_loss, old_draw, old_quit);
        fclose(fp);
    }
}

void showStats() {
    if (currentUser.username[0] == '\0') {
        printf("请先登录！\n");
        return;
    }
    char filename[50];
    sprintf(filename, "stats_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("玩家 %s 暂无战绩记录。\n", currentUser.username);
        return;
    }
    char line[100];
    if (fgets(line, sizeof(line), fp))
        printf("\n=== 玩家 %s 的战绩 ===\n%s", currentUser.username, line);
    else
        printf("玩家 %s 暂无战绩记录。\n", currentUser.username);
    fclose(fp);
}

void saveCurrentMove(const char *moveStr, int stepNo) {
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "a");
    if (!fp) return;
    setbuf(fp, NULL);
    if (stepNo == 1) fprintf(fp, "[GAME_START]\n");
    fprintf(fp, "%d. %s\n", stepNo, moveStr);
    fflush(fp);
    fclose(fp);
}

void saveGameMoves(const char *result) {
    if (game.moveCount == 0) return;
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "a");
    if (!fp) { printf("无法保存棋谱！\n"); return; }
    setbuf(fp, NULL);
    fprintf(fp, "[RESULT] %s\n", result);
    fprintf(fp, "[GAME_END]\n\n");
    fflush(fp);
    fclose(fp);
}

void listGames() {
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "r");
    if (!fp) { printf("没有找到任何历史对局。\n"); return; }
    char line[200];
    int gameNum = 1, inGame = 0;
    char result[50] = "";
    printf("\n=== 历史对局列表 ===\n");
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "[GAME_START]")) { inGame = 1; result[0] = '\0'; }
        else if (strstr(line, "[RESULT]") && inGame) sscanf(line, "[RESULT] %[^\n]", result);
        else if (strstr(line, "[GAME_END]") && inGame) {
            printf("%d. 结果: %s\n", gameNum++, result);
            inGame = 0;
        }
    }
    fclose(fp);
    if (gameNum == 1) printf("暂无对局记录。\n");
}

// ---------- 扫描未完成对局（公共函数）----------
int scanUnfinishedGames(UnfinishedGame *games, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    char line[256];
    int inGame = 0, count = 0;
    long currentStartPos = -1;
    char currentResult[50] = "";
    char currentMoves[MAX_MOVES][10];
    int currentMoveCnt = 0;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, "[GAME_START]") == 0) {
            if (inGame && currentMoveCnt > 0 && count < 10) {
                UnfinishedGame *ug = &games[count];
                ug->startPos = currentStartPos;
                ug->moveCount = currentMoveCnt;
                strcpy(ug->result, currentResult);
                for (int i = 0; i < currentMoveCnt; i++) strcpy(ug->moves[i], currentMoves[i]);
                count++;
            }
            inGame = 1;
            currentStartPos = ftell(fp) - strlen(line) - 1;
            currentMoveCnt = 0;
            currentResult[0] = '\0';
        } else if (strstr(line, "[GAME_END]") && inGame) {
            inGame = 0;
        } else if (strstr(line, "[RESULT]") && inGame) {
            sscanf(line, "[RESULT] %[^\n]", currentResult);
        } else if (inGame && line[0] >= '0' && line[0] <= '9') {
            char move[10];
            if (sscanf(line, "%*d. %s", move) == 1 && currentMoveCnt < MAX_MOVES)
                strcpy(currentMoves[currentMoveCnt++], move);
        }
    }
    if (inGame && currentMoveCnt > 0 && count < 10) {
        UnfinishedGame *ug = &games[count];
        ug->startPos = currentStartPos;
        ug->moveCount = currentMoveCnt;
        strcpy(ug->result, currentResult);
        for (int i = 0; i < currentMoveCnt; i++) strcpy(ug->moves[i], currentMoves[i]);
        count++;
    }
    fclose(fp);
    return count;
}

// ---------- 标记中途退出并更新战绩 ----------
void markGameAsQuit(int moveCount) {
    int nextWhite = (moveCount % 2 == 0);
    const char *quitResult = nextWhite ? "白方中途退出" : "黑方中途退出";
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "a");
    if (fp) {
        setbuf(fp, NULL);
        fprintf(fp, "[RESULT] %s\n", quitResult);
        fprintf(fp, "[GAME_END]\n\n");
        fclose(fp);
    }
    saveUserStats(0, 0, 0, 1);
}

// ---------- 未完成对局处理 ----------
void checkAndResumeUnfinishedGame(void) {
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    UnfinishedGame games[10];
    int unfinishedCount = scanUnfinishedGames(games, filename);
    if (unfinishedCount == 0) return;

    if (unfinishedCount > 3) {
        int toAutoQuit = unfinishedCount - 3;
        printf("发现 %d 个未完成对局，最多保留3个可恢复。\n", unfinishedCount);
        printf("较早的 %d 个将自动记录为中途退出。\n", toAutoQuit);
        for (int i = 0; i < toAutoQuit; i++) {
            markGameAsQuit(games[i].moveCount);
            printf("已自动标记对局 %d 为中途退出。\n", i + 1);
        }
        unfinishedCount = 3;
        for (int i = 0; i < 3; i++) games[i] = games[toAutoQuit + i];
    }

    printf("\n========== 恢复系统 ==========\n");
    printf("发现 %d 个未完成的对局（可能是异常退出导致）：\n", unfinishedCount);
    for (int i = 0; i < unfinishedCount; i++)
        printf("%d. 已走步数：%d\n", i + 1, games[i].moveCount);
    printf("请选择要处理的对局（输入1~%d），或输入0退出恢复系统：", unfinishedCount);
    char choiceStr[10];
    fgets(choiceStr, sizeof(choiceStr), stdin);
    int choice = atoi(choiceStr);
    if (choice < 1 || choice > unfinishedCount) {
        printf("退出恢复系统。\n");
        return;
    }

    UnfinishedGame *ug = &games[choice - 1];
    printf("\n请选择对该对局的操作：\n");
    printf("  1 - 继续此对局（从断点恢复）\n");
    printf("  2 - 放弃并记录为中途退出（更新战绩）\n");
    printf("  3 - 忽略（保留文件，下次再问）\n");
    printf("请输入数字：");
    fgets(choiceStr, sizeof(choiceStr), stdin);
    int action = atoi(choiceStr);

    if (action == 1) {
        if (loadGameFromMoves((const char **)ug->moves, ug->moveCount)) {
            printf("成功恢复棋盘状态！继续游戏...\n");
            game.moveCount = ug->moveCount;
            for (int i = 0; i < game.moveCount; i++) strcpy(game.currentGameMoves[i], ug->moves[i]);
            game.whiteToMove = (game.moveCount % 2 == 0);
            continueGame();
        } else {
            printf("恢复失败，该对局标记为异常。\n");
            FILE *fix = fopen(filename, "a");
            if (fix) { fprintf(fix, "[RESULT] 恢复失败异常\n[GAME_END]\n\n"); fclose(fix); }
        }
    } else if (action == 2) {
        markGameAsQuit(ug->moveCount);
        printf("已记录为中途退出，战绩已更新。\n");
    } else if (action == 3) {
        printf("已忽略该对局，保留文件，下次启动仍会询问。\n");
    } else printf("无效输入，不做任何操作。\n");
    printf("==============================\n");
}

void manageUnfinishedGames(void) {
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    UnfinishedGame games[10];
    int unfinishedCount = scanUnfinishedGames(games, filename);
    if (unfinishedCount == 0) {
        printf("没有未完成的对局。\n");
        return;
    }
    while (1) {
        printf("\n========== 未完成对局列表 ==========\n");
        for (int i = 0; i < unfinishedCount; i++)
            printf("%d. 已走步数：%d\n", i + 1, games[i].moveCount);
        printf("请选择要处理的对局（输入1~%d），或输入0返回上级菜单：", unfinishedCount);
        char choiceStr[10];
        fgets(choiceStr, sizeof(choiceStr), stdin);
        int choice = atoi(choiceStr);
        if (choice == 0) break;
        if (choice < 1 || choice > unfinishedCount) {
            printf("无效输入。\n");
            continue;
        }
        UnfinishedGame *ug = &games[choice - 1];
        printf("\n请选择对该对局的操作：\n");
        printf("  1 - 继续此对局（从断点恢复）\n");
        printf("  2 - 放弃并记录为中途退出（更新战绩）\n");
        printf("  3 - 忽略（保留文件，下次再问）\n");
        printf("请输入数字：");
        fgets(choiceStr, sizeof(choiceStr), stdin);
        int action = atoi(choiceStr);

        if (action == 1) {
            if (loadGameFromMoves((const char **)ug->moves, ug->moveCount)) {
                printf("成功恢复棋盘状态！继续游戏...\n");
                game.moveCount = ug->moveCount;
                for (int i = 0; i < game.moveCount; i++) strcpy(game.currentGameMoves[i], ug->moves[i]);
                game.whiteToMove = (game.moveCount % 2 == 0);
                continueGame();
                printf("游戏已结束，返回主菜单。\n");
                return;
            } else {
                printf("恢复失败，该对局标记为异常。\n");
                FILE *fix = fopen(filename, "a");
                if (fix) { fprintf(fix, "[RESULT] 恢复失败异常\n[GAME_END]\n\n"); fclose(fix); }
                printf("请重新进入本菜单查看最新状态。\n");
                return;
            }
        } else if (action == 2) {
            markGameAsQuit(ug->moveCount);
            printf("已记录为中途退出，战绩已更新。\n");
            printf("请重新进入本菜单查看更新后的列表。\n");
            return;
        } else if (action == 3) {
            printf("已忽略该对局，保留文件。\n");
            return;
        } else printf("无效输入，请重新选择。\n");
    }
    printf("已退出未完成对局管理。\n");
}

// ---------- 解析移动字符串（支持 "e2e4" 或 "e7e8Q"）----------
int parseMoveString(const char *moveStr, char *src, char *dst, char *promote) {
    int len = strlen(moveStr);
    if (len == 4) {
        src[0] = moveStr[0]; src[1] = moveStr[1]; src[2] = '\0';
        dst[0] = moveStr[2]; dst[1] = moveStr[3]; dst[2] = '\0';
        *promote = 0;
        return 1;
    } else if (len == 5) {
        src[0] = moveStr[0]; src[1] = moveStr[1]; src[2] = '\0';
        dst[0] = moveStr[2]; dst[1] = moveStr[3]; dst[2] = '\0';
        *promote = moveStr[4];
        return 1;
    }
    return 0;
}

// ---------- 从棋谱加载棋盘 ----------
int loadGameFromMoves(const char *moves[], int moveCnt) {
    initBoard();
    game.whiteCastleKingSide = game.whiteCastleQueenSide = 1;
    game.blackCastleKingSide = game.blackCastleQueenSide = 1;
    game.enPassantValid = 0;
    game.whiteToMove = 1;

    for (int i = 0; i < moveCnt; i++) {
        char src[8], dst[8], promoteChar;
        if (!parseMoveString(moves[i], src, dst, &promoteChar)) return 0;
        int sr, sc, dr, dc;
        if (!parseSquare(src, &sr, &sc) || !parseSquare(dst, &dr, &dc)) return 0;
        char piece = game.board[sr][sc];
        if (piece == '.') return 0;

        // 先执行移动（静默模式自动升为后）
        if (!executeMove(sr, sc, dr, dc, (i % 2 == 0), 1)) return 0;
        // 如果有升变字符，修正为目标棋子
        if (promoteChar != 0) {
            game.board[dr][dc] = promoteChar;
        }
    }
    return 1;
}

// ---------- 核心游戏逻辑 ----------
void initBoard(void) {
    const char *start[8] = {"rnbqkbnr", "pppppppp", "........", "........", "........", "........", "PPPPPPPP", "RNBQKBNR"};
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            game.board[r][c] = start[r][c];
    game.whiteToMove = 1;
    game.whiteCastleKingSide = game.whiteCastleQueenSide = 1;
    game.blackCastleKingSide = game.blackCastleQueenSide = 1;
    game.enPassantValid = 0;
    game.enPassantRow = game.enPassantCol = -1;
    game.moveCount = 0;
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
        if (game.board[r][c] != '.') return 0;
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
    char target = game.board[dr][dc];
    int startRow = white ? 6 : 1;
    if (sc == dc && target == '.') {
        if (dr == sr + direction) return 1;
        if (sr == startRow && dr == sr + 2 * direction && game.board[sr + direction][sc] == '.') return 1;
    }
    if (abs(dc - sc) == 1 && dr == sr + direction) {
        if (isEnemyPiece(target, white)) return 1;
        if (game.enPassantValid && dr == game.enPassantRow && dc == game.enPassantCol) return 1;
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
            if (!game.whiteCastleKingSide) return 0;
            if (game.board[7][5] != '.' || game.board[7][6] != '.') return 0;
            if (squareAttacked(7,5,0) || squareAttacked(7,6,0)) return 0;
            if (game.board[7][7] != 'R') return 0;
            return 1;
        }
        if (dr == 7 && dc == 2) {
            if (!game.whiteCastleQueenSide) return 0;
            if (game.board[7][3] != '.' || game.board[7][2] != '.' || game.board[7][1] != '.') return 0;
            if (squareAttacked(7,3,0) || squareAttacked(7,2,0)) return 0;
            if (game.board[7][0] != 'R') return 0;
            return 1;
        }
    } else {
        if (sr != 0 || sc != 4) return 0;
        if (dr == 0 && dc == 6) {
            if (!game.blackCastleKingSide) return 0;
            if (game.board[0][5] != '.' || game.board[0][6] != '.') return 0;
            if (squareAttacked(0,5,1) || squareAttacked(0,6,1)) return 0;
            if (game.board[0][7] != 'r') return 0;
            return 1;
        }
        if (dr == 0 && dc == 2) {
            if (!game.blackCastleQueenSide) return 0;
            if (game.board[0][3] != '.' || game.board[0][2] != '.' || game.board[0][1] != '.') return 0;
            if (squareAttacked(0,3,1) || squareAttacked(0,2,1)) return 0;
            if (game.board[0][0] != 'r') return 0;
            return 1;
        }
    }
    return 0;
}

int canMoveKing(int sr, int sc, int dr, int dc) {
    int drAbs = abs(dr - sr), dcAbs = abs(dc - sc);
    if (drAbs == 0 && dcAbs == 2)
        return canCastle(sr, sc, dr, dc, isWhite(game.board[sr][sc]));
    return drAbs <= 1 && dcAbs <= 1;
}

int squareAttacked(int row, int col, int byWhite) {
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c) {
            char piece = game.board[r][c];
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
            if (game.board[r][c] == king) { kingRow = r; kingCol = c; break; }
    if (kingRow == -1) return 1;
    return squareAttacked(kingRow, kingCol, !white);
}

int canMovePiece(int sr, int sc, int dr, int dc, int white) {
    if (!onBoard(sr, sc) || !onBoard(dr, dc)) return 0;
    char piece = game.board[sr][sc];
    if (piece == '.' || !isFriendlyPiece(piece, white)) return 0;
    if (isFriendlyPiece(game.board[dr][dc], white)) return 0;
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

// 核心移动函数，silentPromotion=1 时自动升变为后，不询问
int executeMove(int sr, int sc, int dr, int dc, int white, int silentPromotion) {
    if (!canMovePiece(sr, sc, dr, dc, white)) return 0;
    char piece = game.board[sr][sc];
    char target = game.board[dr][dc];
    GameState backup = game;

    // 易位：移动车
    if (tolower(piece) == 'k' && abs(dc - sc) == 2) {
        if (white) {
            if (dc == 6) { game.board[7][5] = 'R'; game.board[7][7] = '.'; }
            else if (dc == 2) { game.board[7][3] = 'R'; game.board[7][0] = '.'; }
            game.whiteCastleKingSide = game.whiteCastleQueenSide = 0;
        } else {
            if (dc == 6) { game.board[0][5] = 'r'; game.board[0][7] = '.'; }
            else if (dc == 2) { game.board[0][3] = 'r'; game.board[0][0] = '.'; }
            game.blackCastleKingSide = game.blackCastleQueenSide = 0;
        }
    }
    // 更新车是否移动过（易位权限）
    if (tolower(piece) == 'r') {
        if (white) {
            if (sr == 7 && sc == 0) game.whiteCastleQueenSide = 0;
            if (sr == 7 && sc == 7) game.whiteCastleKingSide = 0;
        } else {
            if (sr == 0 && sc == 0) game.blackCastleQueenSide = 0;
            if (sr == 0 && sc == 7) game.blackCastleKingSide = 0;
        }
    }
    if (tolower(target) == 'r') {
        if (target == 'R') {
            if (dr == 7 && dc == 0) game.whiteCastleQueenSide = 0;
            if (dr == 7 && dc == 7) game.whiteCastleKingSide = 0;
        } else {
            if (dr == 0 && dc == 0) game.blackCastleQueenSide = 0;
            if (dr == 0 && dc == 7) game.blackCastleKingSide = 0;
        }
    }

    // 移动棋子
    game.board[dr][dc] = piece;
    game.board[sr][sc] = '.';

    // 吃过路兵处理
    if (tolower(piece) == 'p' && abs(dr - sr) == 1 && abs(dc - sc) == 1 && target == '.' && game.enPassantValid && dr == game.enPassantRow && dc == game.enPassantCol) {
        int captureRow = white ? dr + 1 : dr - 1;
        game.board[captureRow][dc] = '.';
    }

    // 更新吃过路兵标记
    if (tolower(piece) == 'p' && abs(dr - sr) == 2) {
        game.enPassantValid = 1;
        game.enPassantRow = (sr + dr) / 2;
        game.enPassantCol = sc;
    } else {
        game.enPassantValid = 0;
    }

    // 检查移动后己方王是否被将军
    if (isKingInCheck(white)) {
        game = backup;
        return 0;
    }

    // 兵升变
    if (tolower(piece) == 'p' && (dr == 0 || dr == 7)) {
        char promote = silentPromotion ? (white ? 'Q' : 'q') : 0;
        if (!silentPromotion) {
            printf("兵升变！请选择升变棋子：Q(后) R(车) N(马) B(象) : ");
            char input[8];
            while (1) {
                fgets(input, sizeof(input), stdin);
                if (input[0] == '\n') continue;
                char choice = toupper(input[0]);
                if (choice == 'Q' || choice == 'R' || choice == 'N' || choice == 'B') {
                    promote = white ? choice : tolower(choice);
                    break;
                }
                printf("无效输入，请输入 Q, R, N, B : ");
            }
        }
        game.board[dr][dc] = promote;
    }
    return 1;
}

int hasLegalMove(int white) {
    GameState backup = game;
    for (int sr = 0; sr < BOARD_SIZE; ++sr)
        for (int sc = 0; sc < BOARD_SIZE; ++sc)
            if (isFriendlyPiece(game.board[sr][sc], white))
                for (int dr = 0; dr < BOARD_SIZE; ++dr)
                    for (int dc = 0; dc < BOARD_SIZE; ++dc)
                        if (canMovePiece(sr, sc, dr, dc, white)) {
                            if (executeMove(sr, sc, dr, dc, white, 1)) {
                                game = backup;
                                return 1;
                            }
                            game = backup;
                        }
    return 0;
}

void printBoard(void) {
    printf("    a   b   c   d   e   f   g   h\n");
    printf("  +---+---+---+---+---+---+---+---+\n");
    int rowOrder[8];
    if (viewMode == 1)
        for (int i = 0; i < 8; i++) rowOrder[i] = i;
    else
        for (int i = 0; i < 8; i++) rowOrder[i] = 7 - i;

    for (int i = 0; i < 8; ++i) {
        int r = rowOrder[i];
        printf("%d |", 8 - r);
        for (int c = 0; c < BOARD_SIZE; ++c)
            printf(" %c |", game.board[r][c]);
        printf(" %d\n", 8 - r);
        printf("  +---+---+---+---+---+---+---+---+\n");
    }
    printf("    a   b   c   d   e   f   g   h\n");
}

void printInstructions() {
    printf("国际象棋游戏规则：\n- 每方交替移动一枚棋子。\n- 兵直走一格，首步可走两格；斜吃敌方棋子。\n- 马走日字，越过棋子。\n- 象走斜线，车走直线，后可走直线和斜线。\n- 王可走一格，不能将自己置于将军。\n- 王可进行王车易位：白方 e1->g1/h1 或 e1->c1，黑方 e8->g8/h8。\n- 吃过路兵：如果敌方兵首走两格通过你的兵，下一步可沿对角线吃掉它。\n- 兵升变到对方底线可升变为后、车、马或象。\n- 失去所有合法着法且王在将军判负；无将军但无合法着法判为僵局。\n输入格式示例：e2 e4 或 e7e5。输入 quit 结束游戏。\n\n");
}

void toggleViewMode() {
    printf("\n当前棋盘视角模式：%s\n", viewMode == 1 ? "始终白方在下" : "始终黑方在下");
    printf("请选择新模式：\n  1 - 始终白方在下\n  2 - 始终黑方在下\n输入数字：");
    char buf[16];
    if (fgets(buf, sizeof(buf), stdin)) {
        int newMode;
        if (sscanf(buf, "%d", &newMode) == 1 && (newMode == 1 || newMode == 2)) {
            viewMode = newMode;
            printf("视角模式已切换。\n");
        } else printf("无效输入，保持原模式。\n");
    }
}

// ---------- 回放（复用 executeMove，支持升变精确还原）----------
void replayGame(int gameIndex) {
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "r");
    if (!fp) { printf("无法打开棋谱文件。\n"); return; }
    char line[200];
    int curGame = 0, inTarget = 0, moveCnt = 0;
    char moves[MAX_MOVES][10];
    char result[50] = "";
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "[GAME_START]")) {
            curGame++;
            if (curGame == gameIndex) { inTarget = 1; moveCnt = 0; result[0] = '\0'; continue; }
        }
        if (inTarget) {
            if (strstr(line, "[RESULT]")) sscanf(line, "[RESULT] %[^\n]", result);
            else if (strstr(line, "[GAME_END]")) break;
            else {
                char move[10];
                if (sscanf(line, "%*d. %s", move) == 1 && moveCnt < MAX_MOVES)
                    strcpy(moves[moveCnt++], move);
            }
        }
    }
    fclose(fp);
    if (!inTarget || moveCnt == 0) {
        printf("未找到指定对局或对局为空。\n");
        return;
    }
    printf("\n开始回放第 %d 局（结果：%s）\n", gameIndex, result);
    printf("按回车键播放下一步，输入 q 并回车退出回放。\n");

    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);

    GameState savedGame = game;
    initBoard();

    for (int i = 0; i < moveCnt; i++) {
        char src[8], dst[8], promoteChar;
        if (!parseMoveString(moves[i], src, dst, &promoteChar)) {
            printf("棋谱解析错误，停止回放。\n");
            break;
        }
        int sr, sc, dr, dc;
        if (!parseSquare(src, &sr, &sc) || !parseSquare(dst, &dr, &dc)) {
            printf("棋谱解析错误，停止回放。\n");
            break;
        }
        int whiteToMoveNow = (i % 2 == 0);
        if (!executeMove(sr, sc, dr, dc, whiteToMoveNow, 1)) {
            printf("回放错误：第 %d 步移动非法！\n", i+1);
            break;
        }
        // 如果有升变字符，修正为目标棋子
        if (promoteChar != 0) {
            game.board[dr][dc] = promoteChar;
        }
        printBoard();
        printf("第 %d 步：%s\n", i+1, moves[i]);
        if (i == moveCnt - 1) {
            printf("回放结束。\n");
            break;
        }
        printf("按回车继续...");
        char input[10];
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        if (input[0] == 'q' || input[0] == 'Q') {
            printf("用户中断回放。\n");
            break;
        }
    }
    game = savedGame;
    printf("对局结果：%s\n", result);
}

// ---------- 游戏主循环 ----------
void continueGame(void) {
    char line[64];
    while (1) {
        printBoard();
        printf("%s 走棋：", game.whiteToMove ? "白方" : "黑方");
        if (!fgets(line, sizeof(line), stdin)) break;
        line[strcspn(line, "\n")] = 0;
        char src[8] = {0}, dst[8] = {0};
        if (sscanf(line, "%7s %7s", src, dst) < 1) continue;
        if (strcmp(src, "quit") == 0 || strcmp(src, "exit") == 0) {
            const char *quitResult = game.whiteToMove ? "白方中途退出" : "黑方中途退出";
            printf("游戏结束（%s）。\n", quitResult);
            if (game.moveCount > 0) {
                saveGameMoves(quitResult);
                saveUserStats(0, 0, 0, 1);
            }
            return;
        }
        if (strlen(src) == 4 && dst[0] == '\0') {
            dst[0] = src[2]; dst[1] = src[3]; dst[2] = '\0';
            src[2] = '\0';
        }

        int raw_sr, raw_sc, raw_dr, raw_dc;
        if (!parseSquare(src, &raw_sr, &raw_sc) || !parseSquare(dst, &raw_dr, &raw_dc)) {
            printf("无效输入，请使用 a1 到 h8 的坐标。\n");
            continue;
        }

        int sr = raw_sr, sc = raw_sc, dr = raw_dr, dc = raw_dc;
        if (viewMode == 2) { sr = 7 - sr; dr = 7 - dr; }

        if (!executeMove(sr, sc, dr, dc, game.whiteToMove, 0)) {
            printf("非法移动！请检查棋子颜色和走法。\n");
            continue;
        }

        // 生成移动字符串，并检测是否升变，若是则追加升变字符
        char moveStr[10];
        sprintf(moveStr, "%c%c%c%c", 'a'+raw_sc, '8'-raw_sr, 'a'+raw_dc, '8'-raw_dr);
        // 检查是否发生了升变：移动的棋子是兵，且到达底线，且目标格棋子不再是兵
        char pieceNow = game.board[dr][dc];
        if (tolower(pieceNow) != 'p' && (dr == 0 || dr == 7)) {
            // 升变发生，追加升变字符
            char tmp[10];
            sprintf(tmp, "%s%c", moveStr, pieceNow);
            strcpy(moveStr, tmp);
        }
        saveCurrentMove(moveStr, game.moveCount + 1);

        if (game.moveCount < MAX_MOVES) {
            strcpy(game.currentGameMoves[game.moveCount], moveStr);
            game.moveCount++;
        }

        if (isKingInCheck(!game.whiteToMove)) {
            if (!hasLegalMove(!game.whiteToMove)) {
                printBoard();
                printf("将死！%s 胜利。\n", game.whiteToMove ? "白方" : "黑方");
                if (game.whiteToMove) {
                    saveGameMoves("白方赢");
                    saveUserStats(1, 0, 0, 0);
                } else {
                    saveGameMoves("黑方赢");
                    saveUserStats(0, 1, 0, 0);
                }
                return;
            }
            printf("将军！\n");
        } else if (!hasLegalMove(!game.whiteToMove)) {
            printBoard();
            printf("僵局，平局。\n");
            saveGameMoves("平局");
            saveUserStats(0, 0, 1, 0);
            return;
        }
        game.whiteToMove = !game.whiteToMove;
    }
}

void newGame() {
    initBoard();
    printf("\n新游戏开始！\n");
    printInstructions();
    continueGame();
}

// ---------- 主函数 ----------
int main() {
    printf("===== 国际象棋游戏（带账户系统+自动恢复+翻转模式）=====\n");
    currentUser.username[0] = '\0';
    char line[64];
    while (1) {
        printf("\n");
        if (currentUser.username[0] == '\0') {
            printf("=== 主菜单（未登录）===\n");
            printf("1. 登录\n2. 注册\n3. 退出程序\n请选择：");
        } else {
            printf("=== 主菜单（当前用户：%s）===\n", currentUser.username);
            printf("1. 修改密码\n2. 退出登录\n3. 开始新游戏\n4. 游戏规则\n5. 切换棋盘翻转模式\n6. 查看战绩\n7. 历史对局\n8. 查看未完成的对局\n9. 退出程序\n请选择：");
        }
        if (!fgets(line, sizeof(line), stdin)) break;
        int choice = atoi(line);
        if (currentUser.username[0] == '\0') {
            switch (choice) {
                case 1: if (login()) checkAndResumeUnfinishedGame(); break;
                case 2: registerUser(); break;
                case 3: printf("感谢使用，再见！\n"); return 0;
                default: printf("无效选择，请输入 1、2 或 3。\n");
            }
        } else {
            switch (choice) {
                case 1: changePassword(); break;
                case 2: logout(); break;
                case 3: newGame(); break;
                case 4: printInstructions(); break;
                case 5: toggleViewMode(); break;
                case 6: showStats(); break;
                case 7:
                    listGames();
                    printf("请输入要回放的对局序号（输入0返回）：");
                    int idx;
                    if (scanf("%d", &idx) == 1 && idx > 0) replayGame(idx);
                    while (getchar() != '\n');
                    break;
                case 8: manageUnfinishedGames(); break;
                case 9: printf("感谢使用，再见！\n"); return 0;
                default: printf("无效选择，请输入 1 ~ 9。\n");
            }
        }
    }
    return 0;
}

//答辩可能会问的问题 &答案

//问：为什么不用三维数组保存历史状态来实现悔棋？
//答：“悔棋”我还没做，默认不可悔棋。（下次一定！）

//问：如何保证恢复棋盘时状态完全一致？
//答：我们记录了每一步的代数坐标，并重新执行所有合法移动（通过 executeMove 保证规则一致）。

//问：如果用户在恢复时再次异常退出怎么办？
//答：系统会再次检测到未完成对局，用户可以再次选择继续或放弃。