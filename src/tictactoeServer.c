#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "Utils.h"

int tictactoe(struct sock client);

int main(int argc, char *argv[]){

    // Check if the arguments are valid
    argumentsAreValid(argv[1], NULL, argc, 0);

    // Get the socket struct and define the board
    struct sock server = createSocket(argv[1], NULL, 0);
    tictactoe(server); // call the 'game'

    return 0;
}

int tictactoe(struct sock server){
    // Current game number incrementer
    int globalGameCount = 0;

    // Values to store all games (all game boards, client addresses, game states, game numbers, and timestamps)
    char boards[GAME][ROWS][COLUMNS];
    struct sockaddr_in clientAddrs[GAME];
    int allGameStatus[GAME];
    int allGameNumber[GAME];
    time_t timeSinceLastValid[GAME]; // Use timeSinceLastValid = time(NULL); to set


    // Initialize all boards and game status info
    int i;
    for(i=0; i < GAME; i++){
        initSharedState(boards[i]);
        allGameStatus[i] = gameComplete; // This is also the state after each game completes
    }


    // Necessary variables to get the input, print the board, and send or receive data with buffer
    int gameIndex, curClientTimedOut;
    ssize_t rc;
    char buffer[bufferSize];
    memset(buffer, 0, bufferSize);
    struct sockaddr_in serverAddress = server.address;


    // Loop forever
    do{
        //Defensive reset of game index and current client timeout status
        gameIndex = -1;
        curClientTimedOut = 0;

        // Receive buffer from the client
        memset (buffer, 0, bufferSize);
        size_t serverLen = sizeof(serverAddress);
        rc = recvfrom(server.sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*) &serverAddress, (socklen_t *)&serverLen);

        // Process only if valid
        if(rc <= 0){
            printf("Receive error! Attempting to read again\n");
            serverTimeouts(gameIndex, timeSinceLastValid, boards, allGameStatus, allGameNumber, buffer, clientAddrs, server.sockfd);
        }
        else{
            // Search active games for user(by IP and port) and game number
            gameIndex = findGame(buffer[5], serverAddress, allGameStatus, allGameNumber, clientAddrs);

            // Process user timeouts
            curClientTimedOut = serverTimeouts(gameIndex, timeSinceLastValid, boards, allGameStatus, allGameNumber, buffer, clientAddrs, server.sockfd);
            
            if(curClientTimedOut) {    // Client was timed out, game no longer valid, skip turn
                printf("--------------------------------------------\n");
                printf("Current client was timed out, skipping turn\n");
                printf("--------------------------------------------\n");
            }
            else if(gameIndex != -1) {    //Active game found
                printf("--------------------------------------------\n");
                printf("Active game %d found on board %d\n", allGameNumber[gameIndex], gameIndex);
                printf("--------------------------------------------\n");
                
                if(buffer[4] == newGame)
                {
                    printf("--------------------------------------------\n");
                    printf("'Try again' error for game %d\n", allGameNumber[gameIndex]);
                    printf("--------------------------------------------\n");
                    // send general error 'try again'
                    sendError(tryAgain, buffer, serverAddress, server.sockfd);

                    // End the game
                    allGameStatus[gameIndex] = gameComplete;
                    initSharedState(boards[gameIndex]);
                }
                else if(buffer[4] == move)
                {
                    printf("--------------------------------------------\n");
                    printf("Parsing server turn for game %d\n", allGameNumber[gameIndex]);
                    printf("--------------------------------------------\n");
                    // parse server turn (set shouldProcessTurn to 1)
                    serverTurn(buffer, boards, allGameStatus, timeSinceLastValid, gameIndex, serverAddress, server.sockfd);
                }
                else
                {
                    printf("--------------------------------------------\n");
                    printf("'Malformed/invalid' error for game %d\n", allGameNumber[gameIndex]);
                    printf("--------------------------------------------\n");
                    // send error 'malformed/invalid request'
                    sendError(malformedRequest, buffer, serverAddress, server.sockfd);

                    // End the game
                    allGameStatus[gameIndex] = gameComplete;
                    initSharedState(boards[gameIndex]);
                }
            }
            else { // No active game found
                printf("--------------------------------------------\n");
                printf("No active game found, attempting to allocate\n");
                printf("--------------------------------------------\n");
                
                if(buffer[4] == newGame){ // If new game request
                    // Defensive reset to -1
                    gameIndex = -1;
                    int i;

                    // Attempt to allocate game for user
                    for (i=0; i < GAME; i++){
                        if(allGameStatus[i] != gameInProgress){
                            gameIndex = i;
                            break;
                        }
                    }

                    if (gameIndex != -1){
                        printf("--------------------------------------------\n");
                        printf("Allocation success, new game on board %d\n", gameIndex);

                        // Clear client address structure
                        clientAddrs[gameIndex].sin_family = AF_INET;
                        clientAddrs[gameIndex].sin_port = 0;
                        clientAddrs[gameIndex].sin_addr.s_addr = 0;
                        memset(&clientAddrs[gameIndex].sin_zero, 0, sizeof(clientAddrs[gameIndex].sin_zero));

                        // Assign game values
                        memcpy(&clientAddrs[gameIndex], (struct sockaddr*)&serverAddress, sizeof(serverAddress));
                        initSharedState(boards[gameIndex]);
                        allGameNumber[gameIndex] = globalGameCount;
                        allGameStatus[gameIndex] = gameInProgress;
                        timeSinceLastValid[gameIndex] = time(NULL);

                        // Set buffer values to send
                        memset(buffer, 0, bufferSize);

                        buffer[0] = version;
                        buffer[5] = allGameNumber[gameIndex];
                        sendto(server.sockfd, buffer, bufferSize, 0, (struct sockaddr*) &serverAddress, sizeof(serverAddress));

			globalGameCount = (globalGameCount+1)%254;

                        printf("\tNew game number is %d, count now %d\n", allGameNumber[gameIndex], globalGameCount);
                        printf("--------------------------------------------\n");
                    }
                    else{
                        printf("--------------------------------------------\n");
                        printf("Allocation failure, alerting user\n");
                        printf("--------------------------------------------\n");

                        // Send error 'out of resources'
                        sendError(outOfResource, buffer, serverAddress, server.sockfd);
                    }

                }
                else{ // No game found and not a new game request
                    printf("--------------------------------------------\n");
                    printf("'Malformed/invalid' error from user w/o active game!\n");
                    printf("--------------------------------------------\n");
                    
                    // send error 'malformed/invalid request'
                    sendError(malformedRequest, buffer, serverAddress, server.sockfd);
                }

            }
        }

        // Turn has ended
        printf("--------------------------------------------\n");
        printf("Turn ending on board %d\n", gameIndex);
        printf("--------------------------------------------\n");

        printf("\nWaiting for a player to enter a position...\n\n\n");

    }while (1); // Server runs endlessly

    return 0;
}
