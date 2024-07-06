all: server

clean:
	@rm -rf *.o
	@rm -rf server

start:
	./server 2002 public

server: main.o
	gcc -o server $^

main.o: main.c
	gcc -c -o main.o main.c

