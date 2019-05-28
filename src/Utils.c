#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "Utils.h"

int version = 5;
int gameInProgress = 0;
int gameComplete = 1;
int gameError = 2;
int tryAgain = 5;
int newGame = 0;
int move = 1;
int timeoutLimit = 60;
int bufferSize = 1000;
int outOfResource = 1;
int malformedRequest = 2;
int draw = 1;
int iWin = 2;
int youWin = 3;

// Brute force print out the board and all the squares/values 
void print_board(char board[ROWS][COLUMNS]){
    printf("\n\n\n\tCurrent TicTacToe Game\n\n");
    printf("Player 1 (X)  -  Player 2 (O)\n\n\n");
    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", board[0][0], board[0][1], board[0][2]);
    printf("_____|_____|_____\n");
    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", board[1][0], board[1][1], board[1][2]);
    printf("_____|_____|_____\n");
    printf("     |     |     \n");
    printf("  %c  |  %c  |  %c \n", board[2][0], board[2][1], board[2][2]);
    printf("     |     |     \n\n");
}

// Initialize the shared state AKA the board
int initSharedState(char board[ROWS][COLUMNS]){
    
    int i, j, count = 1;
    for (i=0;i<3;i++)
        for (j=0;j<3;j++)
        {
            board[i][j] = count + '0';
            count++;
        }
    return 0;
}

// Brute force check to see if someone won, or if there is a draw. Returns 0 if the game is over and -1 if the game should continue
int checkwin(char board[ROWS][COLUMNS]){
    if (board[0][0] == board[0][1] && board[0][1] == board[0][2] ) // row matches
        return 1;
    else if (board[1][0] == board[1][1] && board[1][1] == board[1][2] ) // row matches
        return 1;
    else if (board[2][0] == board[2][1] && board[2][1] == board[2][2] ) // row matches
        return 1;
    else if (board[0][0] == board[1][0] && board[1][0] == board[2][0] ) // column
        return 1;
    else if (board[0][1] == board[1][1] && board[1][1] == board[2][1] ) // column
        return 1;
    else if (board[0][2] == board[1][2] && board[1][2] == board[2][2] ) // column
        return 1;
    else if (board[0][0] == board[1][1] && board[1][1] == board[2][2] ) // diagonal
        return 1;
    else if (board[2][0] == board[1][1] && board[1][1] == board[0][2] ) // diagonal
        return 1;
    else if (board[0][0] != '1' && board[0][1] != '2' && board[0][2] != '3' &&
    board[1][0] != '4' && board[1][1] != '5' && board[1][2] != '6' &&
    board[2][0] != '7' && board[2][1] != '8' && board[2][2] != '9')
        return 0; // Return of 0 means game over
    else
        return  - 1; // return of -1 means keep playing
}

/* Create a single socket with the address and the timeout
 *
 * @param portNumber: the port number to be used
 * @param ipAddress: the ip address to be used. This is NULL if thisIsClient = 0
 * @param thisIsClient: indicates if this socket is created for the client
 *
 * @returns result: The created socket
*/
struct sock createSocket(char *portNumber, char *ipAddress, int thisIsClient){

    // Create the socket and do error checking
    struct sock result;
    int sockd;
    if(!(sockd = socket(AF_INET, SOCK_DGRAM,0))){
        fprintf(stderr,"Socket could not be defined. Program exiting.\n");
        exit(1);
    }

    // Set the variables of the struct address
    struct sockaddr_in sockAddress;
    sockAddress.sin_family = AF_INET;
    sockAddress.sin_port = htons((uint16_t) strtol(portNumber, NULL, 10));

    // Depending on the client and the server, set the variables of the sockaddr_in address
    if(thisIsClient){
        sockAddress.sin_addr.s_addr = inet_addr(ipAddress);
    }else{
        printf("Starting on port %s\n", portNumber);
        sockAddress.sin_addr.s_addr = INADDR_ANY;
    }

    // Set the timeout of the socket (Client only, server manages its own timeouts)
    if(thisIsClient){
        struct timeval timeout;
        timeout.tv_sec = timeoutLimit;
        timeout.tv_usec = 0;
        if(setsockopt(sockd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0){
            fprintf(stderr,"Setting the timeout failed. This error has occured in setsockopt() function. Program exiting.\n");
            exit(1);
        }
    }
    // If this method is called from the server, bind the socket
    if(!thisIsClient){
        if(bind(sockd, (struct sockaddr *) &sockAddress, sizeof(sockAddress)) < 0){
            fprintf(stderr,"Server could not bind to the given port number. Program exiting\n");
            exit(1);
        }
    }

    // Return the socket
    result.sockfd = sockd;
    result.address = sockAddress;
    return result;
}

// Get a game move from the user
int getChoice(){
    int choice, flush;
    if(scanf("%d", &choice) == 1){
        // flush input and return choice
        while((flush = getchar()) != EOF && flush != '\n');
        return choice;
    }else{
        // flush input and return error
        while((flush = getchar()) != EOF && flush != '\n');
        return -1;
    }
}

// Check if the choice entered by the user is valid
int choiceValid(char board[ROWS][COLUMNS], int choice){
    int row = ((choice-1) / ROWS);
    int column = (choice-1) % COLUMNS;
    if(choice > 9 || choice < 1 || board[row][column] == 'X' || board[row][column] == 'O'){
        // Position has already been selected, or choice is invalid
        return 0;
    }
    return 1;

}

/* Server function: Attempts to find the index of the user's game. Returns index if an
 * active game is found, -1 otherwise
 *
 * @param clientNumber: the user's reported game number
 * @param serverAddress: the user's sockaddr_in info
 * @param allGameStatus: array storing all game active states
 * @param allGameNumber: array storing all active game numbers
 * @param clientAddrs: array storing all client sockaddr_in values
 *
*/
int findGame(int clientNumber, struct sockaddr_in serverAddress, int allGameStatus[GAME], int allGameNumber[GAME], struct sockaddr_in clientAddrs[GAME]){
    int retPos = -1, i;
    for(i = 0; i < GAME; i++){
        // If a game is in progress with the provided game number, check the address info
        if((allGameStatus[i] == gameInProgress) && (allGameNumber[i] == clientNumber)){
            if(clientAddrs[i].sin_port == serverAddress.sin_port){
                if (clientAddrs[i].sin_addr.s_addr == serverAddress.sin_addr.s_addr){
                    retPos = i;
                    break;
                }
            }
        }
    }
    return retPos;
}

/* Server function: Make a move and send to a client
 *
 * @param buffer: The buffer received from the client
 * @param boards: The array of all current game boards
 * @param allGameStatus: The status (gameInProgress or gameComplete) of all current game boards
 * @param gameIndex: The current game's location in the global game variables
 * @param serverAddress: The client's sockaddr_in information
 * @param sockfd: The socket to send the message over
 *
*/
void serverTurn(char buffer[bufferSize], char boards[GAME][ROWS][COLUMNS], int allGameStatus[GAME], time_t timeSinceLastValid[GAME], int gameIndex, struct sockaddr_in serverAddress, int sockfd){
    int row, column, modifier;

    // Check for any errors from the server
    modifier = errorChecking(0, buffer, boards[gameIndex]);
    if (modifier){

        // Error detected, send error and end this game
        memset(buffer, 0, bufferSize);
        buffer[0] = version;
        buffer[2] = gameError;
        buffer[3] = modifier;
        sendto(sockfd, buffer, bufferSize, 0, (struct sockaddr*) &serverAddress, sizeof(serverAddress));

        allGameStatus[gameIndex] = gameComplete;
        initSharedState(boards[gameIndex]);

    }
    else{
        // If choice is valid, mark the position in the board, and print the board
        row = ((buffer[1]) - 1) / ROWS;
        column = ((buffer[1]) - 1) % COLUMNS;
        boards[gameIndex][row][column] = 'X';

        // after a move, check to see if someone won! or if there is a draw */
        if(checkwin(boards[gameIndex]) == 1){
            printf("==>\aClient wins\n");
            // set game win/loss modifier and allGameStatus, invalidate the game
            allGameStatus[gameIndex] = gameComplete;
            modifier = youWin;
            initSharedState(boards[gameIndex]);
        }
        else if(!checkwin(boards[gameIndex])){
            printf("==>\aGame draw\n");
            // set game win/loss modifier and allGameStatus, invalidate the game
            allGameStatus[gameIndex] = gameComplete;
            modifier = draw;
            initSharedState(boards[gameIndex]);
        }


        // Clear the buffer and pick the next move if game will continue
        memset(buffer, 0, bufferSize);
        int i = 0;
        if((modifier != draw) && (modifier != youWin)){
            while(i < 9 && !choiceValid(boards[gameIndex], i)){
                i++;
            }

            // Mark the board, check for a winner
            row = ((i - 1) / ROWS);
            column = (i - 1) % COLUMNS;
            boards[gameIndex][row][column] = 'O';
        }


       // after a move, check to see if someone won! or if there is a draw
        if(checkwin(boards[gameIndex]) == 1){
            printf("==>\aServer win\n");
            // set game win/loss modifier and allGameStatus, invalidate the game
            allGameStatus[gameIndex] = gameComplete;
            modifier = iWin;
            initSharedState(boards[gameIndex]);
        }
        else if(!checkwin(boards[gameIndex])){
            printf("==>\aGame draw\n");
            // set game win/loss modifier and allGameStatus, invalidate the game
            allGameStatus[gameIndex] = gameComplete;
            modifier = draw;
            initSharedState(boards[gameIndex]);
        }

        // Set choices and send variables
        buffer[0] = version;
        buffer[1] = i;
        buffer[2] = (char) allGameStatus[gameIndex];
        buffer[3] = modifier;
        buffer[4] = move;
        sendto(sockfd, buffer, bufferSize, 0, (struct sockaddr*) &serverAddress, sizeof(serverAddress));

        // Update client's timeout
        timeSinceLastValid[gameIndex] = time(NULL);
    }
}


/* Checks if the user-entered arguments are valid
 *
 * @param portNumberString: The port number to be checked if it is valid
 * @param ipAddress: The IP address to be checked if it is valid. This is null if thisIsClient = 0
 * @param numOfArgs: The number of user-entered arguments when the program is run
 * @param thisIsClient: specifies if this method is used by the client. The program will adjust itself accordingly
*/
void argumentsAreValid(char *portNumberString, char *ipAddress, int numOfArgs, int thisIsClient){

    // Check if the user entered 3 arguments and if the ip address is correct or print an error
    if(thisIsClient){
        if(numOfArgs != 3){
            fprintf(stderr,"Please enter 2 arguments as tictactoeClient <port-number> <ip-address>\n");
            exit(1);
        }
    }else{
        if(numOfArgs != 2){
            fprintf(stderr,"Please enter 1 arguments as tictactoeServer <port-number>\n");
            exit(1);
        }
    }

    // Set the variables for the port number validation
    int i=0;
    int isValid = 1;
    int portNumber = 0;

    // Check the validity of the port number
    while(i < strlen(portNumberString)){
        if(!isdigit(portNumberString[i])){
            fprintf(stderr,"Invalid port number. Program exiting.\n");
            exit(1);
        }
        i++;
    }

    // Check if the ipAddress is valid only for thisIsClient = 1
    if(thisIsClient){
        struct sockaddr_in sa;
        if(!inet_pton(AF_INET, ipAddress, &(sa.sin_addr))){
            fprintf(stderr,"IP address not valid. Program exiting.\n");
            exit(1);
        }
    }
}

/*Checks if any of the constant variables are erronous. It will print an error if one of the variables is invalid.
 *When in server mode, will return 0 if no errors found, and a modifier value for the error if found
 *
 * @param isClient: if this method is called by the server or the client. If it is client, isClient == 1, otherwise, isClient == 0
 * @param buffer: The buffer to be checked for its variables
 * @param board: The tictactoe board will be used for choiceValid
*/
int errorChecking(int isClient, char *buffer, char board[ROWS][COLUMNS]){

    int retVal = 0;

    // Check if the version number is correct
    if(buffer[0] != version){
        if(isClient){
            fprintf(stderr,"Version number wrong. Program exiting.\n");
            exit(1);
        }
        else{
            retVal = malformedRequest;
        }
    }

    // If the choice is not valid, exit the program
    if(!choiceValid(board, buffer[1])){
        if(isClient){
            fprintf(stderr,"Invalid choice. Program exiting.\n");
            exit(1);
        }
        else{
            retVal = malformedRequest;
        }
    }

    // Check if the game is continuing
    if(buffer[2] == gameError){
        if(isClient){
            fprintf(stderr,"Error occured with the game. Program exiting\n");
            exit(1);
        }
        else{
            retVal = malformedRequest;
        }

    }
    return retVal;
}

/* The client version of the method that initiates the handshake with the server
 *
 * @param buffer: The message that will be sent to the server. The constant variables will be set accordingly
 * @param gameNumber: The game number that will be received from the server
 * @param client: The socket to be used to send a message
*/
void clientHandshake(char *buffer, char *gameNumber, int client, struct sockaddr_in address){

    // Set the version and the initial newgame value
    socklen_t  len; // Length of the socket
    buffer[0] = version;
    buffer[2] = (char) gameInProgress;
    buffer[3] = 1; //NOTE: Currently don't support modifier bit. Sending to be compliant
    buffer[4] = newGame;
    sendto(client, buffer, bufferSize, 0, (const struct sockaddr *) &address, sizeof(address));

    // Receive the game number and check for the version
    memset(buffer, 0, bufferSize);
    len = sizeof(address);
    if(recvfrom(client, buffer, bufferSize, 0, (struct sockaddr *) &address, &len) <= 0){
        printf("The other side had an issue, exiting...\n");
        exit(1);
    }
    *gameNumber = buffer[5];

    // Check if the version number is correct
    if(buffer[0] != version){
        fprintf(stderr,"Version number wrong. Program exiting.\n");
        exit(1);
    }

    // Check if the server was able to accept the game
    if(buffer[2] == gameError){
	if(buffer[3] == outOfResource){
	    fprintf(stderr,"Server cannot accept more games. Program exiting.\n");
            exit(1);
	}

    	fprintf(stderr,"Error from the server side. Program exiting.\n");
        exit(1);
    }
}

/* Server function: Process timeouts from all clients. If the current client has timed out, return 1. Else return 0
 *
 * @param gameIndex
 * @param timeSinceLastValid
 * @param boards
 * @param allGameStatus
 * @param allgameNumber
 * @param buffer
 * @param clientAddrs
 * @param sockfd
 *
*/
int serverTimeouts(int gameIndex, time_t timeSinceLastValid[GAME], char boards[GAME][ROWS][COLUMNS], int allGameStatus[GAME], int allGameNumber[GAME], char buffer[bufferSize], struct sockaddr_in clientAddrs[GAME], int sockfd)
{
    int retVal = 0;

    // Check all timeouts, invalidate those who have timed out and send an error
    time_t currentTime = time(NULL);
    int i;
    for(i=0; i < GAME; i++){
        if(allGameStatus[i] == gameInProgress){
            if((currentTime - timeSinceLastValid[i]) > timeoutLimit){
                printf("--------------------------------------------\n");
                printf("Game %d timed out (delta = %g seconds)\n", allGameNumber[i], difftime(currentTime, timeSinceLastValid[i]));
                printf("--------------------------------------------\n");
                // send client timeout
                sendError(tryAgain, buffer, clientAddrs[i], sockfd);
 
                // End the game
                allGameStatus[i] = gameComplete;
                initSharedState(boards[i]);

                // If the current client has timed out, note this in retVal
                if(i == gameIndex){
                    retVal = 1;
                }
            }
        }
    }
    return retVal;
}


/* Server function: Send error to client
 *
 * @param errorType: The error code determined by our protocol
 * @param buffer: The array to send
 * @param address: The address to send the buffer
 * @param sockfd: socket descriptor
 */
void sendError(int errorType, char buffer[bufferSize], struct sockaddr_in address, int sockfd){
    memset(buffer, 0, bufferSize);

    buffer[0] = version;
    buffer[2] = gameError;
    buffer[3] = errorType;
    sendto(sockfd, buffer, bufferSize, 0, (struct sockaddr*) &address, sizeof(address));

}
