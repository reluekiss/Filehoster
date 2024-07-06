all: server

start:
	./server 2003 public

clean:
	@rm -rf *.o
	@rm -rf server

server: main.o
	gcc -o server $^

main.o: main.c
	gcc -c -o main.o main.c

