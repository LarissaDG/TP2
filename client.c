#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <math.h>

#include "common.h"
#include "mensagens.h"

//Variáveis globais
// Socket
int s = 0;
unsigned port = 0;
udp_file_msg *pacote;
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
	int i,num_pacotes = 0;
	float aux;

	long tam = get_tam_arquivo(nome,len);

	//Vê o número de pacotes
	aux = (float)tam/(float)BUFSZ;
	num_pacotes = ceil(aux);

	pacote = (udp_file_msg*) calloc(num_pacotes,sizeof(udp_file_msg));

	printf("num_pacotes: %d\n", num_pacotes);

	//Abre o arquivo
	FILE *arquivo = fopen(nome,"rb");
	if(arquivo == NULL ){
		printf("Erro na abertura do arquivo\n");
	}

	//Coloco o ponteiro no inicio do arquivo:
	fseek(arquivo,0,SEEK_SET);

	//Lê 1000 bytes
	for(i=0;i<num_pacotes;i++){
		pacote[i].msg_id = 6;
		pacote[i].num_sequencia = i;
		memset(&pacote[i].payload,0,sizeof(pacote[i].payload));
		fread(pacote[i].payload,sizeof(pacote[i].payload),1,arquivo);
		//printf("payload: %s\n",pacote.payload[i]);
		pacote[i].payload_size = strlen(pacote[i].payload);
		//printf("tamanho: %ld\n",strlen(pacote.payload[i]));
	}

	//Imprime
	for(i=0;i<num_pacotes;i++){
		imprime_pacote_UDP(pacote[i]);	
	}

	fclose(arquivo);
}

void cria_mensagem(unsigned short int msg_id,char nome[],int tam){

	//Cria a mensagem Hello
	if(msg_id == 0){//Talvez tenha que trocar o valor de comparação
		mini_msg sms;
		sms.msg_id = 1;
		send(s, &sms, sizeof(sms), 0);
		printf("Mensagem enviada: Hello\n");
	}

	//Cria a mensagem Info file
	if(msg_id == 2){
		//Recebe a mensagem Connection
		printf("Mensagem recebida: Connection\n");

		connection_msg conex;
		int receive = recv(s, &conex, sizeof(conex),0);
		printf("ID msg %u\nUDP port %u\nBytes lidos %d \n",conex.msg_id,conex.udp_port,receive);
		port = ntohs(conex.udp_port);
		printf("PORTA: %u\n", port);

		//Cria a mensagem Info File
		info_file_msg info;
		info.msg_id = 3;
		strcpy(info.nome_arquivo,nome);
		info.tamanho_arquivo = get_tam_arquivo(nome,tam);
		send(s, &info, sizeof(info), 0); 
		printf("Mensagem enviada: Info File\n");
	}

	//Cria a mensagem Dados
	if(msg_id == 4){
		printf("Mensagem recebida: Ok\n");
		mini_msg sms;
        recv(s, &sms, sizeof(sms), 0);
        printf("Mensagem recebida: Hello\n");

		//Cria a mensagem de dados
        segmenta_arquivo(nome,tam);
        
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
        printf("Mini msg %u Bytes lidos %d \n",sms.msg_id, receive);
        if(receive > 0){
			cria_mensagem(sms.msg_id,nome,strlen(nome));
			fflush(stdout);
        }
    	else{
			break;
    	}
    	//getchar();
  	}
  	//printf("Saiu\n");
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
