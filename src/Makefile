CC=gcc
CFLAGS=-Wall -Wextra

OBJS=server.o file.o mime.o cache.o hashtable.o llist.o

all: server

server: $(OBJS)
	gcc -o $@ $^ -lcrypto -lssl

server.o: server.c

file.o: file.c file.h

mime.o: mime.c mime.h

cache.o: cache.c cache.h

hashtable.o: hashtable.c hashtable.h

llist.o: llist.c llist.h

clean:
	rm -f $(OBJS)
	rm -f server

.PHONY: all, clean
