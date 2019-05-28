#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "Utils.h"

int tictactoe(char board[ROWS][COLUMNS], struct sock client);

int main(int argc, char *argv[]){

    // Check if the arguments are valid
    argumentsAreValid(argv[1], argv[2], argc, 1);

    // Create the socket and the board or print an error message
    struct sock client = createSocket(argv[1], argv[2], 1);
    char board[ROWS][COLUMNS];

    initSharedState(board); // Initialize the 'game' board
    tictactoe(board, client); // call the 'game'
    close(client.sockfd); // Close the connection when the game is finished
    return 0;
}

int tictactoe(char board[ROWS][COLUMNS], struct sock client){

    // Necessary variables to get the input, print the board, and send or recieve data with buffer
    int row, column, choice; // Row, column, choice and the rc for error checking for socket operations
    socklen_t  len; // Length of the socket
    char buffer[bufferSize], gameNumber; // The buffer as an array and the game number from the server
    memset(buffer, 0, bufferSize);
    struct sockaddr_in clientAddress = client.address;

    // Initiate the handshake and after that, all requests will be moves
    clientHandshake(buffer, &gameNumber, client.sockfd, client.address);

    // Loop until game is over
    do{

        // Print the board
        print_board(board);

        // Prompt the user for input until the position is valid
        do{
            printf("\nEnter a number:  ");
            choice = getChoice();
        }while(!choiceValid(board, choice));

        // Prepare the package and send it to the server
        memset(buffer, 0, bufferSize);
        buffer[0] = version;
        buffer[1] = choice;
        buffer[2] = (char) gameInProgress;
        buffer[3] = 1; //NOTE: Currently don't support modifier bit. Sending to be compliant
        buffer[4] = move;
        buffer[5] = gameNumber;
        sendto(client.sockfd, buffer, bufferSize, 0, (const struct sockaddr *) &clientAddress, sizeof(clientAddress));

        // Calculate the position, mark the board as X, and print the board
        row = (((buffer[1]) - 1) / ROWS);
        column = ((buffer[1]) - 1) % COLUMNS;
        board[row][column] = 'X';
        print_board(board);

        /* after a move, check to see if someone won! (or if there is a draw */
        if(checkwin(board) == 1){
            printf("==>\aYou win\n");
            break;
        }
        else if(!checkwin(board)){
            printf("==>\aGame draw\n");
            break;
        }

        // Receive from the server and get the size of the address
        printf("\nWaiting for player 2 to enter a position...\n");
        memset (buffer, 0, bufferSize);
        len = sizeof(clientAddress);
        if(recvfrom(client.sockfd, buffer, bufferSize, 0, (struct sockaddr *) &clientAddress, &len) <= 0){
            printf("The other side had an issue, exiting...\n");
            exit(1);
        }

        // Check for any errors from the server
        errorChecking(1, buffer, board);

        // Mark the server's position in the board
        row = (((buffer[1])-1) / ROWS);
        column = ((buffer[1])-1) % COLUMNS;
        board[row][column] = 'O';

        /* after a move, check to see if someone won! or if there is a draw */
        if(checkwin(board) == 1){
            printf("==>\aPlayer 2 wins\n");
            break;
        }
        else if(!checkwin(board)){
            printf("==>\aGame draw\n");
            break;
        }

    }while (checkwin(board) ==  - 1); // -1 means no one won

    /* print out the board again */
    print_board(board);
    return 0;
}
