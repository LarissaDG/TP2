#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "common.h"

//Entradas Servidor e o porto
void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port> <file>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511 arquivo.doc\n", argv[0]);
	exit(EXIT_FAILURE);
}


// Socket
int s = 0;
//Exit flag
int flag = 0;


void send_msg() {
	//Mensagem HELLO
	char buffer[BUFSZ] = {};
	determina_etapa(1, buffer);
	send(s, buffer, strlen(buffer), 0);
	memset(buffer, 0, BUFSZ);
  	/*while(1) {
  		fflush(stdout);
    	fgets(buffer, BUFSZ, stdin);
      	send(s, buffer, strlen(buffer), 0);
		memset(buffer, 0, BUFSZ);
 	}*/
}

void recv_msg() {
	char buffer[BUFSZ] = {};
  	while (1) {
		int receive = recv(s, buffer, BUFSZ, 0);
    	if (receive > 0) {
      		//printf("%s", buffer);
      		if(buffer[0]=='2'){
      			 printf("Mensagem recebida: Connection\n");
      			 determina_etapa(3, buffer);
      			 send(s, buffer, strlen(buffer), 0);
				 memset(buffer, 0, BUFSZ);
      		}
      		if(buffer[0]=='4'){
      			 printf("Mensagem recebida: ok\n");
      			 determina_etapa(6, buffer);
      			 send(s, buffer, strlen(buffer), 0);
				 memset(buffer, 0, BUFSZ);
      		}
      		if(buffer[0]=='7'){
      			 printf("Mensagem recebida: Ack\n");
      			 determina_etapa(5, buffer);
      			 send(s, buffer, strlen(buffer), 0);
				 memset(buffer, 0, BUFSZ);
      		}
      		fflush(stdout);
    	} 
    	else{
			break;
    	} 
		memset(buffer, 0, sizeof(buffer));
  	}
  	//printf("Saiu\n");
  	flag = 1;
}


int main(int argc, char **argv){
	if (argc < 4) {
		usage(argc, argv);
	}
	
	//Estabelece conexão com o TCP
	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		usage(argc, argv);
	}

	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(s, addr, sizeof(storage))) {
		logexit("connect");
	}
	
	//Threads
	pthread_t send_msg_thread;
  	if(pthread_create(&send_msg_thread, NULL, (void *) send_msg, NULL) != 0){
		printf("ERROR: pthread send\n");
    	return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
  	if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg, NULL) != 0){
		printf("ERROR: pthread receive\n");
		return EXIT_FAILURE;
	}

	while (1){
		if(flag){
			break;
		}
	}

	close(s);

	return EXIT_SUCCESS;
}
