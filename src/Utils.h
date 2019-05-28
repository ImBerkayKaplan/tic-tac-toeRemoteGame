// This file is created to hold all the common methods and variables for the tictactoeServer.c and the tictactoeClient.c

#ifndef UTILS_H_
#define UTILS_H_

// The number of columns and rows that is in a tictactoe board is defined here as well as the number of games that can connect to the server
#define ROWS 3
#define COLUMNS 3
#define GAME 2

struct sock{
    int sockfd;
    struct sockaddr_in address;
};

extern int version;
extern int gameInProgress;
extern int gameComplete;
extern int gameError;
extern int tryAgain;
extern int newGame;
extern int move;
extern int timeoutLimit;
extern int bufferSize;
extern int numOfMaxClients;
extern int outOfResource;
extern int malformedRequest;
extern int draw;
extern int iWin;
extern int youWin;


int checkwin(char board[ROWS][COLUMNS]);
int getChoice();
int initSharedState(char board[ROWS][COLUMNS]);
int choiceValid(char board[ROWS][COLUMNS], int choice);
int findGame(int clientNumber, struct sockaddr_in serverAddress, int gameInProgress[GAME], int gameNumber[GAME], struct sockaddr_in clientAddrs[GAME]);
int errorChecking(int isClient, char *buffer, char board[ROWS][COLUMNS]);
int serverTimeouts(int gameIndex, time_t timeSinceLastValid[GAME], char boards[GAME][ROWS][COLUMNS], int allGameStatus[GAME], int allGameNumber[GAME], char buffer[bufferSize], struct sockaddr_in clientAddrs[GAME], int skt);

void sendError(int errorType, char buffer[bufferSize], struct sockaddr_in serverAddress, int skt);
void serverTurn(char buffer[bufferSize], char boards[GAME][ROWS][COLUMNS], int allGameStatus[GAME], time_t timeSinceLastValid[GAME], int gameIndex, struct sockaddr_in serverAddress, int skt);
void clientHandshake(char *buffer, char *gameNumber, int client, struct sockaddr_in address);
void argumentsAreValid(char *portNumberString, char *ipAddress, int numOfArgs, int thisIsClient);
void print_board(char board[ROWS][COLUMNS]);

struct sock createSocket(char *portNumber, char *ipAddress, int thisIsClient);

#endif
