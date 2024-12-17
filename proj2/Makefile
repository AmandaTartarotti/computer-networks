CC=gcc

CFLAGS= -c -Wall

all: client

client: clientTCP.o aux_functions.o
	$(CC) clientTCP.o aux_functions.o -o client

clientTCP.o : clientTCP.c
	$(CC) $(CFLAGS) clientTCP.c

aux_functions.o : aux_functions.c
	$(CC) $(CFLAGS) aux_functions.c

clean:
	rm -rf *o client