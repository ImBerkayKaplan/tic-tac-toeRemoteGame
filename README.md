# tic-tac-toeRemoteGame

This program is a simple tic-tac-toe game against the computer that is a server. The users connect to the server to play a game against the computer. The server may handle more than one game at the same time. 

Currently, the maximum number of users that can connect to the server is set to two, and can be change by modifying the ```GAME``` variable that is found in Utils.h. After 60 second of inactivity, the client will be disconnected by the server. The timeout setting can be changed my modifying the ```timeoutLimit``` variable that is found in Utils.c.

## Compilation

Use the make file by simply typing ```make``` in the command line window while in the project directory. This directory will contain the “src” folder. The ```make``` command will generate two executables named tictactoeClient and tictactoeServer. 

## Execution

first, the server must be running. Type ```tictactoeServer <port-number>``` in the command line when you are in the project directory. The variable ```<port-number>``` is a positive integer that is the port number the server will run on. Afterwards, type ```tictactoeClient <port-number> <ip-address>```, where ```<port-number>``` is the port number, and ```<ip-address>``` is the ip address of the server that the client will connect to. The server can handle more than one game, and you may have another client connect to the server by following the process mentioned above.
