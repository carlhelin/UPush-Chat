CC = gcc
CFLAGS = -g -std=gnu11

all: upush_server upush_client

upush_client: upush_client.o
	$(CC) $(CFLAGS) upush_client.o -o upush_client

upush_client.o : upush_client.c
	$(CC) $(CFLAGS) -c upush_client.c

upush_server: upush_server.o
	$(CC) $(CFLAGS) upush_server.o -o upush_server

upush_server.o: upush_server.c
	$(CC) $(CFLAGS) -c upush_server.c

.PHONY: clean

run:
	./upush_server

clean:
	rm *.o upush_client upush_server