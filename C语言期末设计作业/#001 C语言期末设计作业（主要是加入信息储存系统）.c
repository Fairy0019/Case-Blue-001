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

// 全局变量
int viewMode = 0;   // 0=自动 1=白方在下 2=黑方在下
char board[BOARD_SIZE][BOARD_SIZE];
int whiteToMove = 1;
int whiteCastleKingSide = 1;
int whiteCastleQueenSide = 1;
int blackCastleKingSide = 1;
int blackCastleQueenSide = 1;
int enPassantValid = 0;
int enPassantRow = -1;
int enPassantCol = -1;

User currentUser = {0}; // 当前登录用户，若未登录则 username[0] == '\0'

#define MAX_MOVES 200                 // 一局棋最多记录200步
char currentGameMoves[MAX_MOVES][10]; // 存放每一步，例如 "e2e4"
int moveCount;                        // 当前对局已走的步数

// 辅助函数
int isWhite(char piece) { return piece >= 'A' && piece <= 'Z'; }
int isBlack(char piece) { return piece >= 'a' && piece <= 'z'; }
int onBoard(int r, int c) { return r>=0 && r<BOARD_SIZE && c>=0 && c<BOARD_SIZE; }

void trimNewline(char *str)
{
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n')
        str[len - 1] = '\0';
}

int userExists(const char *username)
{
    FILE *fp = fopen("users.txt", "r");
    if (!fp)
        return 0; // 文件不存在则肯定不存在
    char line[100];
    while (fgets(line, sizeof(line), fp))
    {
        char name[MAX_USERNAME];
        sscanf(line, "%s", name);
        if (strcmp(name, username) == 0)
        {
            fclose(fp);
            return 1;
        }
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

    if (userExists(username))
    {
        printf("用户名已存在！\n");
        return;
    }

    printf("密码: ");
    fgets(pwd1, MAX_PASSWORD, stdin);
    trimNewline(pwd1);
    printf("确认密码: ");
    fgets(pwd2, MAX_PASSWORD, stdin);
    trimNewline(pwd2);

    if (strcmp(pwd1, pwd2) != 0)
    {
        printf("两次密码不一致！\n");
        return;
    }

    // 保存到文件 users.txt
    FILE *fp = fopen("users.txt", "a");
    if (!fp)
    {
        printf("无法打开用户文件！\n");
        return;
    }
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
    if (!fp)
    {
        printf("用户文件不存在，请先注册。\n");
        return 0;
    }
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
    if (currentUser.username[0] == '\0')
    {
        printf("请先登录！\n");
        return;
    }
    char oldPwd[MAX_PASSWORD], newPwd1[MAX_PASSWORD], newPwd2[MAX_PASSWORD];
    printf("\n===== 修改密码 =====\n");
    printf("原密码: ");
    fgets(oldPwd, MAX_PASSWORD, stdin);
    trimNewline(oldPwd);

    if (strcmp(oldPwd, currentUser.password) != 0)
    {
        printf("原密码错误！\n");
        return;
    }

    printf("新密码: ");
    fgets(newPwd1, MAX_PASSWORD, stdin);
    trimNewline(newPwd1);
    printf("确认新密码: ");
    fgets(newPwd2, MAX_PASSWORD, stdin);
    trimNewline(newPwd2);

    if (strcmp(newPwd1, newPwd2) != 0)
    {
        printf("两次新密码不一致！\n");
        return;
    }

    // 更新文件：读取所有用户，修改当前用户，重写文件
    FILE *fp = fopen("users.txt", "r");
    if (!fp)
    {
        printf("无法打开用户文件！\n");
        return;
    }
    FILE *tmp = fopen("users.tmp", "w");
    if (!tmp)
    {
        fclose(fp);
        printf("临时文件创建失败！\n");
        return;
    }

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
        else
        {
            fputs(line, tmp);
        }
    }
    fclose(fp);
    fclose(tmp);

    remove("users.txt");
    rename("users.tmp", "users.txt");

    if (updated)
        printf("密码修改成功！\n");
    else
        printf("修改失败，用户不存在？\n");
}

void logout()
{
    if (currentUser.username[0] == '\0')
    {
        printf("尚未登录任何账户。\n");
    }
    else
    {
        printf("用户 %s 已退出登录。\n", currentUser.username);
        currentUser.username[0] = '\0';
        currentUser.password[0] = '\0';
    }
}

void loadUserStats()
{
    if (currentUser.username[0] == '\0')
        return;
    char filename[50];
    sprintf(filename, "stats_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        printf("\n暂无战绩记录。\n");
        return;
    }
    char line[100];
    if (fgets(line, sizeof(line), fp))
    {
        printf("\n=== %s 的战绩 ===\n%s", currentUser.username, line);
    }
    else
    {
        printf("\n暂无战绩记录。\n");
    }
    fclose(fp);
}

void saveUserStats(int win, int loss, int draw, int quit)
{
    char filename[50];
    sprintf(filename, "stats_%s.txt", currentUser.username);
    int old_win = 0, old_loss = 0, old_draw = 0, old_quit = 0;
    FILE *fp = fopen(filename, "r");
    if (fp)
    {
        fscanf(fp, "赢:%d 输:%d 平:%d 中途退出:%d", &old_win, &old_loss, &old_draw, &old_quit);
        fclose(fp);
    }
    old_win += win;
    old_loss += loss;
    old_draw += draw;
    old_quit += quit;
    fp = fopen(filename, "w");
    if (fp)
    {
        fprintf(fp, "赢:%d 输:%d 平:%d 中途退出:%d\n", old_win, old_loss, old_draw, old_quit);
        fclose(fp);
    }
}

void saveGameMoves(const char *result)
{
    if (moveCount == 0)
        return; // 没有走棋就不保存
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "a");
    if (!fp)
    {
        printf("无法保存棋谱！\n");
        return;
    }
    fprintf(fp, "[GAME_START]\n");
    for (int i = 0; i < moveCount; i++)
    {
        fprintf(fp, "%d. %s\n", i + 1, currentGameMoves[i]);
    }
    fprintf(fp, "[RESULT] %s\n", result);
    fprintf(fp, "[GAME_END]\n\n");
    fclose(fp);
}

void showStats()
{
    if (currentUser.username[0] == '\0')
    {
        printf("请先登录！\n");
        return;
    }
    char filename[50];
    sprintf(filename, "stats_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        printf("暂无战绩记录。\n");
        return;
    }
    char line[100];
    if (fgets(line, sizeof(line), fp))
    {
        printf("\n=== %s 的战绩 ===\n%s", currentUser.username, line);
    }
    else
    {
        printf("暂无战绩记录。\n");
    }
    fclose(fp);
}

void listGames()
{
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        printf("没有找到任何历史对局。\n");
        return;
    }
    char line[200];
    int gameNum = 1;
    int inGame = 0;
    char result[50] = "";
    printf("\n=== 历史对局列表 ===\n");
    while (fgets(line, sizeof(line), fp))
    {
        if (strstr(line, "[GAME_START]"))
        {
            inGame = 1;
            result[0] = '\0';
        }
        else if (strstr(line, "[RESULT]") && inGame)
        {
            sscanf(line, "[RESULT] %[^\n]", result);
        }
        else if (strstr(line, "[GAME_END]") && inGame)
        {
            printf("%d. 结果: %s\n", gameNum++, result);
            inGame = 0;
        }
    }
    fclose(fp);
    if (gameNum == 1)
        printf("暂无对局记录。\n");
}

void replayGame(int gameIndex)
{
    char filename[50];
    sprintf(filename, "games_%s.txt", currentUser.username);
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        printf("无法打开棋谱文件。\n");
        return;
    }
    char line[200];
    int curGame = 0;
    int inTarget = 0;
    char moves[MAX_MOVES][10];
    int moveCnt = 0;
    char result[50] = "";
    while (fgets(line, sizeof(line), fp))
    {
        if (strstr(line, "[GAME_START]"))
        {
            curGame++;
            if (curGame == gameIndex)
            {
                inTarget = 1;
                moveCnt = 0;
                result[0] = '\0';
                continue;
            }
        }
        if (inTarget)
        {
            if (strstr(line, "[RESULT]"))
            {
                sscanf(line, "[RESULT] %[^\n]", result);
            }
            else if (strstr(line, "[GAME_END]"))
            {
                break;
            }
            else
            {
                char move[10];
                if (sscanf(line, "%*d. %s", move) == 1)
                {
                    if (moveCnt < MAX_MOVES)
                    {
                        strcpy(moves[moveCnt++], move);
                    }
                }
            }
        }
    }
    fclose(fp);
    if (!inTarget || moveCnt == 0)
    {
        printf("未找到指定对局或对局为空。\n");
        return;
    }
    printf("\n开始回放第 %d 局（结果：%s）\n", gameIndex, result);
    printf("按回车键播放下一步，输入 q 退出回放。\n");
    // 备份当前棋盘状态
    char backupBoard[BOARD_SIZE][BOARD_SIZE];
    int bWhiteToMove = whiteToMove;
    int bWCK = whiteCastleKingSide, bWCQ = whiteCastleQueenSide;
    int bBCK = blackCastleKingSide, bBCQ = blackCastleQueenSide;
    int bEPV = enPassantValid, bEPR = enPassantRow, bEPC = enPassantCol;
    memcpy(backupBoard, board, sizeof(board));
    // 重新初始化棋盘
    initBoard();
    // 逐步执行
    for (int i = 0; i < moveCnt; i++)
    {
        char src[3] = {moves[i][0], moves[i][1], '\0'};
        char dst[3] = {moves[i][2], moves[i][3], '\0'};
        int sr, sc, dr, dc;
        if (!parseSquare(src, &sr, &sc) || !parseSquare(dst, &dr, &dc))
        {
            printf("棋谱解析错误，停止回放。\n");
            break;
        }
        if (!executeMove(sr, sc, dr, dc, (i % 2 == 0)))
        {
            printf("回放时移动失败！棋谱可能有误。\n");
            break;
        }
        printBoard();
        printf("第 %d 步：%s\n", i + 1, moves[i]);
        char ch = getchar();
        if (ch == 'q' || ch == 'Q')
        {
            printf("用户中断回放。\n");
            break;
        }
        while (getchar() != '\n')
            ;
    }
    // 恢复备份
    memcpy(board, backupBoard, sizeof(board));
    whiteToMove = bWhiteToMove;
    whiteCastleKingSide = bWCK;
    whiteCastleQueenSide = bWCQ;
    blackCastleKingSide = bBCK;
    blackCastleQueenSide = bBCQ;
    enPassantValid = bEPV;
    enPassantRow = bEPR;
    enPassantCol = bEPC;
    printf("回放结束。\n");
}

void initBoard() {
    const char *start[8] = {
        "rnbqkbnr", "pppppppp", "........", "........",
        "........", "........", "PPPPPPPP", "RNBQKBNR"
    };
    for (int r=0; r<BOARD_SIZE; ++r)
        for (int c=0; c<BOARD_SIZE; ++c)
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
    if (file<'a' || file>'h' || rank<'1' || rank>'8') return 0;
    *col = file - 'a';
    *row = 8 - (rank - '0');
    return 1;
}

int isFriendlyPiece(char piece, int white) {
    if (piece=='.') return 0;
    return white ? isWhite(piece) : isBlack(piece);
}
int isEnemyPiece(char piece, int white) {
    if (piece=='.') return 0;
    return white ? isBlack(piece) : isWhite(piece);
}

int clearPath(int sr, int sc, int dr, int dc) {
    int drStep = (dr>sr) ? 1 : (dr<sr) ? -1 : 0;
    int dcStep = (dc>sc) ? 1 : (dc<sc) ? -1 : 0;
    int r = sr+drStep, c = sc+dcStep;
    while (r!=dr || c!=dc) {
        if (board[r][c]!='.') return 0;
        r += drStep; c += dcStep;
    }
    return 1;
}

int canPawnAttack(int sr, int sc, int dr, int dc, int white) {
    int direction = white ? -1 : 1;
    return dr == sr+direction && abs(dc-sc)==1;
}

int canMovePawn(int sr, int sc, int dr, int dc, int white) {
    int direction = white ? -1 : 1;
    char target = board[dr][dc];
    int startRow = white ? 6 : 1;
    if (sc==dc && target=='.') {
        if (dr == sr+direction) return 1;
        if (sr==startRow && dr==sr+2*direction && board[sr+direction][sc]=='.') return 1;
    }
    if (abs(dc-sc)==1 && dr==sr+direction) {
        if (isEnemyPiece(target, white)) return 1;
        if (enPassantValid && dr==enPassantRow && dc==enPassantCol) return 1;
    }
    return 0;
}

int canMoveKnight(int sr, int sc, int dr, int dc) {
    int drAbs = abs(dr-sr), dcAbs = abs(dc-sc);
    return (drAbs==2 && dcAbs==1) || (drAbs==1 && dcAbs==2);
}

int canMoveBishop(int sr, int sc, int dr, int dc) {
    if (abs(dr-sr) != abs(dc-sc)) return 0;
    return clearPath(sr,sc,dr,dc);
}

int canMoveRook(int sr, int sc, int dr, int dc) {
    if (sr!=dr && sc!=dc) return 0;
    return clearPath(sr,sc,dr,dc);
}

int canMoveQueen(int sr, int sc, int dr, int dc) {
    return canMoveBishop(sr,sc,dr,dc) || canMoveRook(sr,sc,dr,dc);
}

// 前向声明（因为 squareAttacked 和 isKingInCheck 互相调用，但这里顺序已调好）
int squareAttacked(int row, int col, int byWhite);
int isKingInCheck(int white);

int canCastle(int sr, int sc, int dr, int dc, int white) {
    if (isKingInCheck(white)) return 0;
    if (white) {
        if (sr!=7 || sc!=4) return 0;
        if (dr==7 && dc==6) {
            if (!whiteCastleKingSide) return 0;
            if (board[7][5]!='.' || board[7][6]!='.') return 0;
            if (squareAttacked(7,5,0) || squareAttacked(7,6,0)) return 0;
            if (board[7][7]!='R') return 0;
            return 1;
        }
        if (dr==7 && dc==2) {
            if (!whiteCastleQueenSide) return 0;
            if (board[7][3]!='.' || board[7][2]!='.' || board[7][1]!='.') return 0;
            if (squareAttacked(7,3,0) || squareAttacked(7,2,0)) return 0;
            if (board[7][0]!='R') return 0;
            return 1;
        }
    } else {
        if (sr!=0 || sc!=4) return 0;
        if (dr==0 && dc==6) {
            if (!blackCastleKingSide) return 0;
            if (board[0][5]!='.' || board[0][6]!='.') return 0;
            if (squareAttacked(0,5,1) || squareAttacked(0,6,1)) return 0;
            if (board[0][7]!='r') return 0;
            return 1;
        }
        if (dr==0 && dc==2) {
            if (!blackCastleQueenSide) return 0;
            if (board[0][3]!='.' || board[0][2]!='.' || board[0][1]!='.') return 0;
            if (squareAttacked(0,3,1) || squareAttacked(0,2,1)) return 0;
            if (board[0][0]!='r') return 0;
            return 1;
        }
    }
    return 0;
}

int canMoveKing(int sr, int sc, int dr, int dc) {
    int drAbs = abs(dr-sr), dcAbs = abs(dc-sc);
    if (drAbs==0 && dcAbs==2)
        return canCastle(sr,sc,dr,dc, isWhite(board[sr][sc]));
    return drAbs<=1 && dcAbs<=1;
}

int squareAttacked(int row, int col, int byWhite) {
    for (int r=0; r<BOARD_SIZE; ++r) {
        for (int c=0; c<BOARD_SIZE; ++c) {
            char piece = board[r][c];
            if (!isFriendlyPiece(piece, byWhite)) continue;
            int can = 0;
            char lower = tolower(piece);
            switch (lower) {
                case 'p': can = canPawnAttack(r,c,row,col,byWhite); break;
                case 'n': can = canMoveKnight(r,c,row,col); break;
                case 'b': can = canMoveBishop(r,c,row,col); break;
                case 'r': can = canMoveRook(r,c,row,col); break;
                case 'q': can = canMoveQueen(r,c,row,col); break;
                case 'k': can = canMoveKing(r,c,row,col); break;
            }
            if (can) return 1;
        }
    }
    return 0;
}

int isKingInCheck(int white) {
    int kingRow=-1, kingCol=-1;
    char king = white ? 'K' : 'k';
    for (int r=0; r<BOARD_SIZE; ++r) {
        for (int c=0; c<BOARD_SIZE; ++c) {
            if (board[r][c]==king) { kingRow=r; kingCol=c; break; }
        }
        if (kingRow!=-1) break;
    }
    if (kingRow==-1) return 1;
    return squareAttacked(kingRow, kingCol, !white);
}

int canMovePiece(int sr, int sc, int dr, int dc, int white) {
    if (!onBoard(sr,sc) || !onBoard(dr,dc)) return 0;
    char piece = board[sr][sc];
    if (piece=='.' || !isFriendlyPiece(piece,white)) return 0;
    if (isFriendlyPiece(board[dr][dc], white)) return 0;
    char lower = tolower(piece);
    switch (lower) {
        case 'p': return canMovePawn(sr,sc,dr,dc,white);
        case 'n': return canMoveKnight(sr,sc,dr,dc);
        case 'b': return canMoveBishop(sr,sc,dr,dc);
        case 'r': return canMoveRook(sr,sc,dr,dc);
        case 'q': return canMoveQueen(sr,sc,dr,dc);
        case 'k': return canMoveKing(sr,sc,dr,dc);
        default: return 0;
    }
}

int executeMove(int sr, int sc, int dr, int dc, int white) {
    if (!canMovePiece(sr,sc,dr,dc,white)) return 0;
    char piece = board[sr][sc];
    char target = board[dr][dc];
    char backup[BOARD_SIZE][BOARD_SIZE];
    int bWCK=whiteCastleKingSide, bWCQ=whiteCastleQueenSide;
    int bBCK=blackCastleKingSide, bBCQ=blackCastleQueenSide;
    int bEPV=enPassantValid, bEPR=enPassantRow, bEPC=enPassantCol;
    memcpy(backup, board, sizeof(board));

    if (tolower(piece)=='k' && abs(dc-sc)==2) {
        if (white) {
            if (dc==6) { board[7][5]='R'; board[7][7]='.'; }
            else if (dc==2) { board[7][3]='R'; board[7][0]='.'; }
            whiteCastleKingSide=whiteCastleQueenSide=0;
        } else {
            if (dc==6) { board[0][5]='r'; board[0][7]='.'; }
            else if (dc==2) { board[0][3]='r'; board[0][0]='.'; }
            blackCastleKingSide=blackCastleQueenSide=0;
        }
    }
    if (tolower(piece)=='r') {
        if (white) {
            if (sr==7 && sc==0) whiteCastleQueenSide=0;
            if (sr==7 && sc==7) whiteCastleKingSide=0;
        } else {
            if (sr==0 && sc==0) blackCastleQueenSide=0;
            if (sr==0 && sc==7) blackCastleKingSide=0;
        }
    }
    if (tolower(target)=='r') {
        if (target=='R') {
            if (dr==7 && dc==0) whiteCastleQueenSide=0;
            if (dr==7 && dc==7) whiteCastleKingSide=0;
        } else {
            if (dr==0 && dc==0) blackCastleQueenSide=0;
            if (dr==0 && dc==7) blackCastleKingSide=0;
        }
    }

    board[dr][dc] = piece;
    board[sr][sc] = '.';

    // 吃过路兵
    if (tolower(piece)=='p' && abs(dr-sr)==1 && abs(dc-sc)==1 && target=='.'
        && enPassantValid && dr==enPassantRow && dc==enPassantCol) {
        int captureRow = white ? dr+1 : dr-1;
        board[captureRow][dc] = '.';
    }

    if (tolower(piece)=='p' && abs(dr-sr)==2) {
        enPassantValid = 1;
        enPassantRow = (sr+dr)/2;
        enPassantCol = sc;
    } else {
        enPassantValid = 0;
    }

    if (isKingInCheck(white)) {
        memcpy(board, backup, sizeof(board));
        whiteCastleKingSide=bWCK; whiteCastleQueenSide=bWCQ;
        blackCastleKingSide=bBCK; blackCastleQueenSide=bBCQ;
        enPassantValid=bEPV; enPassantRow=bEPR; enPassantCol=bEPC;
        return 0;
    }

    if (tolower(piece)=='p' && (dr==0 || dr==7))
        board[dr][dc] = white ? 'Q' : 'q';
    return 1;
}

int hasLegalMove(int white) {
    char backup[BOARD_SIZE][BOARD_SIZE];
    int bWCK=whiteCastleKingSide, bWCQ=whiteCastleQueenSide;
    int bBCK=blackCastleKingSide, bBCQ=blackCastleQueenSide;
    int bEPV=enPassantValid, bEPR=enPassantRow, bEPC=enPassantCol;
    for (int sr=0; sr<BOARD_SIZE; ++sr)
        for (int sc=0; sc<BOARD_SIZE; ++sc)
            if (isFriendlyPiece(board[sr][sc], white))
                for (int dr=0; dr<BOARD_SIZE; ++dr)
                    for (int dc=0; dc<BOARD_SIZE; ++dc)
                        if (canMovePiece(sr,sc,dr,dc,white)) {
                            memcpy(backup, board, sizeof(board));
                            int moved = executeMove(sr,sc,dr,dc,white);
                            memcpy(board, backup, sizeof(board));
                            whiteCastleKingSide=bWCK; whiteCastleQueenSide=bWCQ;
                            blackCastleKingSide=bBCK; blackCastleQueenSide=bBCQ;
                            enPassantValid=bEPV; enPassantRow=bEPR; enPassantCol=bEPC;
                            if (moved) return 1;
                        }
    return 0;
}

void printBoard() {
    printf("    a   b   c   d   e   f   g   h\n");
    printf("  +---+---+---+---+---+---+---+---+\n");
    int flip = 0;
    if (viewMode==0) flip = (whiteToMove==0);
    else if (viewMode==1) flip = 0;
    else flip = 1;
    if (!flip) {
        for (int r=0; r<BOARD_SIZE; ++r) {
            printf("%d |", 8-r);
            for (int c=0; c<BOARD_SIZE; ++c) printf(" %c |", board[r][c]);
            printf(" %d\n", 8-r);
            printf("  +---+---+---+---+---+---+---+---+\n");
        }
    } else {
        for (int r=BOARD_SIZE-1; r>=0; --r) {
            printf("%d |", r+1);
            for (int c=0; c<BOARD_SIZE; ++c) printf(" %c |", board[r][c]);
            printf(" %d\n", r+1);
            printf("  +---+---+---+---+---+---+---+---+\n");
        }
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
    printf("- 兵升变到对方底线自动升变为后。\n");
    printf("- 失去所有合法着法且王在将军判负；无将军但无合法着法判为僵局。\n");
    printf("输入格式示例：e2 e4 或 e7e5。输入 quit 结束游戏。\n\n");
}

void printMenu() {
    printf("=== 国际象棋游戏菜单 ===\n");
    printf("1. 新游戏\n");
    printf("2. 游戏规则\n");
    printf("3. 切换棋盘翻转模式\n");
    printf("4. 退出\n");
    printf("请选择：");
}

void toggleViewMode() {
    printf("\n当前棋盘视角模式：");
    if (viewMode==0) printf("自动（根据走棋方翻转）\n");
    else if (viewMode==1) printf("始终白方在下（不翻转）\n");
    else printf("始终黑方在下（强制翻转）\n");
    printf("请选择新模式：\n  0 - 自动\n  1 - 始终白方在下\n  2 - 始终黑方在下\n输入数字：");
    char buf[16];
    if (fgets(buf, sizeof(buf), stdin)) {
        int newMode;
        if (sscanf(buf, "%d", &newMode)==1 && newMode>=0 && newMode<=2) {
            viewMode = newMode;
            printf("视角模式已切换。\n");
        } else printf("无效输入，保持原模式。\n");
    }
}



void playGame()
{
    initBoard();
    moveCount = 0; // 重置步数记录
    printf("\n新游戏开始！\n");
    printInstructions();
    char line[64];
    while (1)
    {
        printBoard();
        printf("%s 走棋：", whiteToMove ? "白方" : "黑方");
        if (!fgets(line, sizeof(line), stdin))
            break;
        char src[8] = {0}, dst[8] = {0};
        if (sscanf(line, "%7s %7s", src, dst) < 1)
            continue;
        // 处理中途退出
        if (strcmp(src, "quit") == 0 || strcmp(src, "exit") == 0)
        {
            printf("游戏结束（中途退出）。\n");
            if (moveCount > 0)
            {
                saveGameMoves("中途退出");
                saveUserStats(0, 0, 0, 1); // 中途退出+1
            }
            return;
        }
        // 支持类似 "e2e4" 的输入
        if (strlen(src) == 4 && dst[0] == '\0')
        {
            dst[0] = src[2];
            dst[1] = src[3];
            dst[2] = '\0';
            src[2] = '\0';
        }
        int sr, sc, dr, dc;
        if (!parseSquare(src, &sr, &sc) || !parseSquare(dst, &dr, &dc))
        {
            printf("无效输入，请使用 a1 到 h8 的坐标。\n");
            continue;
        }

        // 记录原始代数坐标（用于保存棋谱）
        char moveStr[10];
        sprintf(moveStr, "%s%s", src, dst);

        // 计算是否翻转（与 printBoard 一致）
        int flip = 0;
        if (viewMode == 0)
            flip = (whiteToMove == 0);
        else if (viewMode == 1)
            flip = 0;
        else
            flip = 1;
        if (flip)
        {
            sr = 7 - sr;
            dr = 7 - dr;
        }

        if (!executeMove(sr, sc, dr, dc, whiteToMove))
        {
            printf("非法移动。请重新输入。\n");
            continue;
        }

        // 成功移动，记录步数
        if (moveCount < MAX_MOVES)
        {
            strcpy(currentGameMoves[moveCount], moveStr);
            moveCount++;
        }

        // 判断胜负或僵局
        if (isKingInCheck(!whiteToMove))
        {
            if (!hasLegalMove(!whiteToMove))
            {
                printBoard();
                printf("将死！%s 胜利。\n", whiteToMove ? "白方" : "黑方");
                if (whiteToMove)
                {
                    saveGameMoves("赢");
                    saveUserStats(1, 0, 0, 0);
                }
                else
                {
                    saveGameMoves("输");
                    saveUserStats(0, 1, 0, 0);
                }
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
    currentUser.username[0] = '\0'; // 初始未登录

    char line[64];
    while (1)
    {
        printf("\n");
        if (currentUser.username[0] == '\0')
        {
            // 未登录菜单
            printf("=== 主菜单（未登录）===\n");
            printf("1. 登录\n");
            printf("2. 注册\n");
            printf("3. 退出程序\n");
            printf("请选择：");
        }
        else
        {
            // 已登录菜单
            printf("=== 主菜单（当前用户：%s）===\n", currentUser.username);
            printf("1. 修改密码\n");
            printf("2. 退出登录\n");
            printf("3. 开始新游戏\n");
            printf("4. 游戏规则\n");
            printf("5. 切换棋盘翻转模式\n");
            printf("6. 查看战绩\n");
            printf("7. 历史对局\n");
            printf("8. 退出程序\n");
            printf("请选择：");
        }

        if (!fgets(line, sizeof(line), stdin))
            break;
        int choice = atoi(line);

        if (currentUser.username[0] == '\0')
        {
            // 未登录处理
            switch (choice)
            {
            case 1:
                login();
                break;
            case 2:
                registerUser();
                break;
            case 3:
                printf("感谢使用，再见！\n");
                return 0;
            default:
                printf("无效选择，请输入 1、2 或 3。\n");
            }
        }
        else
        {
            // 已登录处理
            switch (choice)
            {
            case 1:
                changePassword();
                break;
            case 2:
                logout();
                break;
            case 3:
                playGame();
                break;
            case 4:
                printInstructions();
                break;
            case 5:
                toggleViewMode();
                break;
            case 6:
                showStats();
                break;
            case 7:
            {
                listGames();
                printf("请输入要回放的对局序号（输入0返回）：");
                int idx;
                if (scanf("%d", &idx) == 1 && idx > 0)
                {
                    replayGame(idx);
                }
                while (getchar() != '\n')
                    ; // 清空输入缓冲区
                break;
            }
            case 8:
                printf("感谢使用，再见！\n");
                return 0;
            default:
                printf("无效选择，请输入 1 ~ 8。\n");
            }
        }
    }
    return 0;
}

// 问题：默认升变为后？需要提供选择
// 问题：是否支持悔棋？（需要保存历史状态）

// 回放示例：
// === 历史对局列表 ===
// 1. 结果: 中途退出
// 请输入要回放的对局序号（输入0返回）：1

// 开始回放第 1 局（结果：中途退出）
// 按回车键播放下一步，输入 q 退出回放。
//     a   b   c   d   e   f   g   h
//   +---+---+---+---+---+---+---+---+
// 8 | r | n | b | q | k | b | n | r | 8
//   +---+---+---+---+---+---+---+---+
// 7 | p | p | p | p | p | p | p | p | 7
//   +---+---+---+---+---+---+---+---+
// 6 | . | . | . | . | . | . | . | . | 6
//   +---+---+---+---+---+---+---+---+
// 5 | . | . | . | . | . | . | . | . | 5
//   +---+---+---+---+---+---+---+---+
// 4 | . | . | . | . | P | . | . | . | 4
//   +---+---+---+---+---+---+---+---+
// 3 | . | . | . | . | . | . | . | . | 3
//   +---+---+---+---+---+---+---+---+
// 2 | P | P | P | P | . | P | P | P | 2
//   +---+---+---+---+---+---+---+---+
// 1 | R | N | B | Q | K | B | N | R | 1
//   +---+---+---+---+---+---+---+---+
//     a   b   c   d   e   f   g   h
// 第 1 步：e2e4

// 回放时移动失败！棋谱可能有误。
// 回放结束。
// 很显然失败了，因为我们在保存棋谱时没有考虑到翻转模式导致的坐标变化。需要在保存棋谱时统一使用不翻转的坐标（即始终以白方视角记录），这样回放时才能正确解析和执行。