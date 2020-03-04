
all: tcp_app upd_app

tcp_app: tcp_client tcp_server

upd_app: upd_client upd_server

tcp_client: tcpclient.c
	gcc -o tcpclient.out tcpclient.c

upd_client: udpclient.c
	gcc -o udpclient.out udpclient.c

tcp_server: tcpserver.c
	gcc -o tcpserver.out tcpserver.c

upd_server: udpserver.c
	gcc -o udpserver.out udpserver.c

clean:
	rm *.out *.o -f
