
all: client server

dbg: client-dbg server-dbg

client-dbg: client.c libnetfac
	gcc -o client-dbg.out client.c -I./libnetfac -L./ -lpthread -lm -lnetfac -g -O0

server-dbg: server.c libnetfac
	gcc -o server-dbg.out  server.c -I./libnetfac -L./ -lpthread -lm -lnetfac -g -O0

client: client.c libnetfac
	gcc -o client.out client.c -I./libnetfac -L./ -lpthread -lm -lnetfac

server: server.c libnetfac
	gcc -o server.out server.c -I./libnetfac -L./ -lpthread -lm -lnetfac

libnetfac: ./libnetfac/netfac.c
	gcc -o ./libnetfac/libnetfac.o -c ./libnetfac/netfac.c -I./libnetfac
	ar rcs ./libnetfac.a ./libnetfac/libnetfac.o

clean:
	rm -r *.o *.a *.out -f
