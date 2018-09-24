CC=gcc
CFLAGS=-Wall 
LDLIBS=-lpthread

all:client

client: client.o csapp.o
	$(CC) $(CFLAGS) -o client client.o csapp.o $(LDLIBS)

clean:
	    rm -f client.o csapp.o client
