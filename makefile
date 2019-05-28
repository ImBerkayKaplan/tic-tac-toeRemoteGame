GCC=gcc
SOURCE=src
C=*.c
MAIN=main
CLIENT=tictactoeClient
SERVER=tictactoeServer
UTILS=Utils.c
CFLAGS  = -g -Wall

all: project

project:
	@$(GCC) -o $(CLIENT) $(SOURCE)/$(CLIENT).c $(SOURCE)/$(UTILS)
	@$(GCC) -o $(SERVER) $(SOURCE)/$(SERVER).c $(SOURCE)/$(UTILS)

clean:
	@rm $(CLIENT)
	@rm $(SERVER)
