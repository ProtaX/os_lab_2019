
all: client server

dbg: client-dbg server-dbg

client-dbg: libnetfac client.c
	gcc -o client-dbg.out client.c -I./libnetfac -L./ -lpthread -lm -lnetfac -g -O0

server-dbg: libnetfac server.c
	gcc -o server-dbg.out  server.c -I./libnetfac -L./ -lpthread -lm -lnetfac -g -O0

client: libnetfac client.c
	gcc -o client.out client.c -I./libnetfac -L./ -lpthread -lm -lnetfac -std=c99

server: libnetfac server.c
	gcc -o server.out server.c -I./libnetfac -L./ -lpthread -lm -lnetfac -std=c99

libnetfac: ./libnetfac/netfac.c ./libnetfac/netfac.h
	gcc -o ./libnetfac/libnetfac.o -c ./libnetfac/netfac.c -I./libnetfac -std=c99
	ar rcs ./libnetfac.a ./libnetfac/libnetfac.o

clean:
	rm -r *.o *.a *.out -f
