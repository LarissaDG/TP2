all:
	gcc -Wall -c common.c
	gcc -Wall -c lista.c
	gcc -Wall client.c common.o -lpthread -o cliente
	gcc -Wall server.c common.o lista.o -lpthread -o servidor

clean:
	rm common.o client server server-mt
