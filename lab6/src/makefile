
all: client server

dbg: client-dbg server-dbg

client-dbg: client.c
	gcc -o client-dbg.out -lpthread -lm -g -O0 client.c

server-dbg: server.c
	gcc -o server-dbg.out -lpthread -lm -g -O0 server.c

client: client.c
	gcc -o client.out -lpthread -lm client.c

server: server.c
	gcc -o server.out -lpthread -lm server.c

clean:
	rm *.o *.a *.out -f
