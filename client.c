#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

#include "common.h"
#include "mensagens.h"

#define TIMEOUT 2*CLOCKS_PER_SEC
#define WINDOW 4

typedef struct{
	ack_msg ack;
	clock_t tempo;
	int recebido;
}controle;

//Variáveis globais
// Socket
int s = 0, sock=0;
struct sockaddr_in6 server_addr,client_addr;
udp_file_msg *pacote;
int num_pacotes=0;
//Exit flag
int flag = 0;

//Entradas Servidor e o porto
void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port> <file>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511 arquivo.doc\n", argv[0]);
	exit(EXIT_FAILURE);
}

void is_nome_val(char nomeArquivo[], int tam){
	int i=0, count=0,num=0;
	if(tam > 15){
		logexit("Nome não permitido");
	}
	//Percorre a string contando o numero de '.'
	char * pos = strchr(nomeArquivo,'.');
	if(pos == NULL){
		logexit("Nome não permitido");
	}
	for(i = pos-nomeArquivo;i<tam;i++){
		if(nomeArquivo[i]==' '){
			logexit("Nome não permitido");
		}
		if(nomeArquivo[i]=='.'){
			count ++;
		}
		else if(isalnum(nomeArquivo[i])){
			num ++;
		}
	}
	if(count != 1 || num != 3){
		logexit("Nome não permitido");
	}
}

long get_tam_arquivo(char nomeArquivo[], int tam){
	FILE *arquivo = fopen(nomeArquivo, "r");
	if (arquivo != NULL) {
		fseek(arquivo, 0, SEEK_END);
		long posicao = ftell(arquivo);
		fclose(arquivo);
		return posicao;
	}
	else{
		logexit("Arquivo inexistente");
		return -1;
	}
}

void imprime_pacote_UDP(udp_file_msg pacote){
    printf("Msg id: %u\n",pacote.msg_id);
    printf("Pacote id: %u\n",pacote.num_sequencia);
    printf("Payload size: %u\n",pacote.payload_size);
    printf("payload: %s\n",pacote.payload);
    printf("\n\n");
}


void segmenta_arquivo(char nome[],int len){
	int i;
	float aux;

	long tam = get_tam_arquivo(nome,len);
	printf("**tamanho_arquivo** %ld\n", tam);
	//Vê o número de pacotes
	aux = (float)tam/(float)BUFSZ;
	printf("**aux %f = %ld/%d ** \n", aux,tam,BUFSZ);
	num_pacotes = ceil(aux);
	printf("**Numero de pacotes %d **\n", num_pacotes);

	pacote = (udp_file_msg*) calloc(num_pacotes,sizeof(udp_file_msg));

	//printf("num_pacotes: %d\n", num_pacotes);

	//Abre o arquivo
	FILE *arquivo = fopen(nome,"rb");
	if(arquivo == NULL ){
		logexit("Erro na abertura do arquivo");
	}

	//Coloco o ponteiro no inicio do arquivo:
	fseek(arquivo,0,SEEK_SET);

	//Lê 1000 bytes
	for(i=0;i<num_pacotes;i++){
		pacote[i].msg_id = 6;
		pacote[i].num_sequencia = i;
		memset(&pacote[i].payload,0,sizeof(pacote[i].payload));
		fread(pacote[i].payload,sizeof(pacote[i].payload),1,arquivo);
		//TODO: Mudar o jeito de indicar o payload size
		pacote[i].payload_size = strlen(pacote[i].payload);
	}

	//Imprime
	/*for(i=0;i<num_pacotes;i++){
		imprime_pacote_UDP(pacote[i]);	
	}*/

	fclose(arquivo);
}

int checa_controle(controle *ctrl){
	int i, count=0;
	for(i=0;i<num_pacotes;i++){
		if(ctrl[i].recebido == 1){
			count++;
		}
	}
	if(count == num_pacotes-1){
		return 1;
	}
	return 0;
}

void cria_mensagem(unsigned short int msg_id,char nome[],int tam){

	//Cria a mensagem Hello
	if(msg_id == 0){//Talvez tenha que trocar o valor de comparação
		mini_msg sms;
		sms.msg_id = 1;
		send(s, &sms, sizeof(sms), 0);
		//printf("Mensagem enviada: Hello\n");
	}

	//Cria a mensagem Info file
	if(msg_id == 2){
		//Recebe a mensagem Connection
		//printf("Mensagem recebida: Connection\n");

		connection_msg conex;
		recv(s, &conex, sizeof(conex),0);
		//printf("ID msg %u\nUDP port %u\n\n",conex.msg_id,conex.udp_port);
		server_addr.sin6_port = conex.udp_port;
		//printf("PORTA: %u\n", ntohs(server_addr.sin6_port));

		//Cria a mensagem Info File
		info_file_msg info;
		info.msg_id = 3;
		strcpy(info.nome_arquivo,nome);
		info.tamanho_arquivo = get_tam_arquivo(nome,tam);
		printf("**tamanho_arquivo** %d\n", info.tamanho_arquivo);
		send(s, &info, sizeof(info), 0); 
		//printf("Mensagem enviada: Info File\n");
	}

	//Cria a mensagem Dados
	if(msg_id == 4){
		int count = 0;
		ack_msg ack;
		mini_msg sms;
		int primeiro=0,ultimo=0,expected = 0, aux=0;
		controle *ctrl;

		//Inicializa vetor de acks
		ctrl = (controle*)calloc(num_pacotes,sizeof(controle));

		//socklen_t server_addr_length = sizeof(server_addr);
		printf("Mensagem recebida: Ok\n");
        recv(s, &sms, sizeof(sms), 0);


		//Cria a mensagem de dados
        segmenta_arquivo(nome,tam);
        //Manda via UDP para o servidor - No momento só manda um pacote
        //imprime_pacote_UDP(pacote[i]);

        //Janela deslizante go back N
        while(1){
        	//Espera a disponibilidade do pacote
        	printf("1\n");
        	if(ultimo < num_pacotes){//TODO ver se essa ehuma boa condição
        		if(ultimo - primeiro >= WINDOW){
        			//Nada
        		}
		        if((count = sendto(sock,&pacote[ultimo],sizeof(pacote[ultimo]),0,
		        	(struct sockaddr*)&server_addr,sizeof(server_addr))) < 0){
		        	logexit("Envio UDP falhou");
		        }
		        printf("\n\n ----------PACOTE ---------\n");
		        imprime_pacote_UDP(pacote[ultimo]);
		        printf("-------------------\n");

		        printf("Pacote %d de %d \n",ultimo+1, num_pacotes);
		        ctrl[ultimo].tempo = clock();
		        ultimo ++;
        	}
        	printf("2\n");
        	if(recv(s, &ack, sizeof(ack), MSG_PEEK)){
		        if((count = recv(s, &ack, sizeof(ack), 0)) < 0){
		            logexit("receive ack");
		        } 
		       // printf("Recebi ack: %u %u\n", ack.msg_id,ack.num_sequencia);
		        if(ack.num_sequencia == expected){
		        	printf("Valor esperado: %d \n",expected);
		        	ctrl[ack.num_sequencia].ack = ack;
		        	ctrl[ack.num_sequencia].recebido = 1;
		        	ctrl[ack.num_sequencia].tempo = clock() - ctrl[ack.num_sequencia].tempo;
		        	primeiro ++;
		        	expected ++; 
		        }
	        }
	        
	        //Se o tempo acabou manda todos os pacotes que não foram recebidos 
	        if(ctrl[primeiro].tempo >= TIMEOUT){
	        	aux = primeiro;
	        	while(aux < ultimo){
		    		printf("Entra\n");
	        		if((count = sendto(sock,&pacote[aux],sizeof(pacote[aux]),0,
		        		(struct sockaddr*)&server_addr,sizeof(server_addr))) < 0){
		        		logexit("Envio UDP falhou");
		       		}
		       		printf("Reenvia pacotes\n");
			        //imprime_pacote_UDP(pacote[aux]);
			        ctrl[aux].tempo = clock();
			        aux++;	
	        	}
	        }
	        if((ultimo == num_pacotes) && checa_controle(ctrl)){
	        	return ;
	        }
    	}
	}
}

void send_msg() {
	char aux [] = "vazio";
	cria_mensagem(0,aux,strlen(aux));
}

void recv_msg(char nome[]) {
	mini_msg sms;
  	while (1) {
		memset(&sms, 0, sizeof(sms));
		int receive = recv(s, &sms, sizeof(sms), MSG_PEEK);
        //printf("Mini msg %u Bytes lidos %d \n",sms.msg_id, receive);
        if(receive > 0){
			cria_mensagem(sms.msg_id,nome,strlen(nome));
			printf("Depois do cria msg\n");
			fflush(stdout);
        }
    	else{
			break;
    	}
    	//getchar();
  	}
  	flag=1;
}


int main(int argc, char **argv){
	if (argc < 4) {
		usage(argc, argv);
	}

	is_nome_val(argv[3], strlen(argv[3]));
	
	//Estabelece conexão com o TCP
	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		usage(argc, argv);
	}

	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		logexit("socket");
	}

	//Criação socket UDP
	if((sock = socket(AF_INET6,SOCK_DGRAM,0)) < 0){
		logexit("socket UDP");
	}

	//Inicializa dados do servidor
	memset(&server_addr,0,sizeof(server_addr));
	
	server_addr.sin6_family = AF_INET6;
	inet_pton(AF_INET6,argv[1],&server_addr.sin6_addr);

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
  	if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg, argv[3]) != 0){
		printf("ERROR: pthread receive\n");
		return EXIT_FAILURE;
	}

	while (1){
		if(flag){
			break;
		}
	}

	//Finalizações
	close(s);
	free(pacote);


	return EXIT_SUCCESS;
}
