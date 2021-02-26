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
#include "lista.h"


#define MAX_CLIENTES 255

//Entrada tipo do protocolo e o porto
void usage(int argc, char **argv) {
    printf("usage: %s <server port>\n", argv[0]);
    printf("example: %s 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

//Dados do cliente
struct client_data {
    int csock;
    struct sockaddr_storage storage;
    int id;
    unsigned udp_port;
};

typedef struct {
    int udp_socket;
    struct sockaddr_in6 client_addr_UDP;
}udp_data;

udp_file_msg *pacote;
int num_pacotes = 0;

struct client_data *Clientes [MAX_CLIENTES];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void inicializaClientes(){
    pthread_mutex_lock(&clients_mutex);
    for(int i=0; i < MAX_CLIENTES; i++){
        Clientes[i] = NULL;
    }
    pthread_mutex_unlock(&clients_mutex);
}

//Retorna id do cliente
int addCliente(struct client_data *cdata ){
    pthread_mutex_lock(&clients_mutex);
    int i = 0;
    for(i=0; i < MAX_CLIENTES; i++){
        if(Clientes[i] == NULL){
            Clientes[i] = cdata;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return i;
}

//Nao desaloca a cdata
void removeCliente(int cid){
    pthread_mutex_lock(&clients_mutex);
    int i = 0;
    for(i=0; i < MAX_CLIENTES; i++){
        if(Clientes[i] != NULL){
            if(Clientes[i]->id == cid){ 
                Clientes[i] = NULL;
                break;
            }   
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

//Funcoes de parsing
void removeSinal(char *buf,char *dest, int tam){
  for(int i=0; i < tam; i++){
    dest[i] = buf[i+1];
    if(dest[i] == '\n' || dest[i] == '\n'){
        dest[i] = '\0';
    }
  }
}

int isInvalido(struct client_data *cliente ,char *buf, int tam){
    int i=0;
    while(buf[i]!='\0'){
        if(!ispunct(buf[i])&&!isalnum(buf[i])&&!isspace(buf[i])){
            return 1;
        }
        i++;
    }
    return 0;
}

int isValido(char c){
    if(!ispunct(c)&&!isalnum(c)){
        return 0;
    }
    return 1;
}

void imprime_pacote_UDP(udp_file_msg pacote){
    printf("Msg id: %u\n",pacote.msg_id);
    printf("Pacote id: %u\n",pacote.num_sequencia);
    printf("Payload size: %u\n",pacote.payload_size);
    printf("payload: %s\n",pacote.payload);
    printf("\n\n");
}


void UDP_connection(udp_data valor, info_file_msg info,struct client_data *cdata){
    int expected = 0;
    ack_msg ack;
    udp_file_msg dados;
    float aux;

    memset(&dados, 0, sizeof(dados));

    //Vê o número de pacotes
    aux = (float)info.tamanho_arquivo/(float)BUFSZ;
    num_pacotes = ceil(aux);

    pacote = (udp_file_msg*) calloc(num_pacotes,sizeof(udp_file_msg));

    //printf("num_pacotes: %d\n", num_pacotes);
    
    //printf("Tam udp_file_msg %ld\n",sizeof(udp_file_msg));
    size_t count;
    socklen_t client_struct_length = sizeof(valor.client_addr_UDP);

    // Receive client's message:
    while (1){
        count = 0;
        printf("Inicio count = %ld\n", count);
        count = recvfrom(valor.udp_socket,&dados, sizeof(dados),O_NONBLOCK,(struct sockaddr*)
            &valor.client_addr_UDP, &client_struct_length);

            if(count > 0){//Manda o ack
                printf("PASS2\n");
                if(dados.num_sequencia == expected){
                    ack.msg_id = 7;
                    ack.num_sequencia = dados.num_sequencia;

                    if(send(cdata->csock, &ack, sizeof(ack), 0) < 0){
                        logexit("Envio ack falhou");
                    }
                    printf("Manda ack %d\n", ack.num_sequencia);
                    expected ++;
                }
                //imprime_pacote_UDP(dados);
                pacote[dados.num_sequencia] = dados;
                printf("IMPRIME PACOTE RECEBIDO\n");
                imprime_pacote_UDP(pacote[dados.num_sequencia]); 
                printf("Pacote %d recebido de %d \n",dados.num_sequencia+1, num_pacotes);
            }
            else{
                printf("BOBO\n");
                break;
            }
        //getchar();
        printf("FIm count = %ld\n", count);

        printf("To preso\n");
    }
    printf("Sai\n");
}

void renomeia_arquivo(char* nome, char* aux){
    int i,pos=0;

    for(i=0;i<strlen(nome);i++){
        if(nome[i] != '.'){
            aux[i] = nome[i];
        }
        else{
            aux[i] = '2';
            pos = i;//Posição do ponto
            break;
        }
    }
    for(i=pos;i<=strlen(nome);i++){
        aux[pos+1]=nome[i];
        pos++;
    }

    aux[i] = '\0';
    //printf("Novo nome do arquivo %s\n", aux);
}

void monta_arquivo(info_file_msg info){
    FILE * arquivo;
    int i;
    char novoNome[20];
    renomeia_arquivo(info.nome_arquivo,novoNome);
    if((arquivo = fopen(novoNome,"wb")) == NULL){
        logexit("Falha na criacao do arquivo");
    }
    for(i=0;i<num_pacotes;i++){
        fwrite(pacote[i].payload,pacote[i].payload_size,1,arquivo);
    }
    fclose(arquivo);
    printf(" Montou arquivo\n");
}


void cria_mensagem(unsigned short int msg_id,struct sockaddr_in6 server_addr_UDP,struct client_data *cdata, udp_data valor){

    if(msg_id == 1){
        //Recebe a mensagem Hello
        mini_msg sms;
        recv(cdata->csock, &sms, sizeof(sms), 0);
        //printf("Mensagem recebida: Hello\n");

        //Cria a mensagem Connection
        connection_msg conex;
        conex.msg_id = 2;
        conex.udp_port = server_addr_UDP.sin6_port;
        //printf("PORTA: %u\n", conex.udp_port);
        send(cdata->csock, &conex, sizeof(conex), 0);
        //printf("Mensagem enviada: Connection\n");
    }

    if(msg_id == 3){
        //Recebe a mensagem Info File
        //printf("Mensagem recebida: Info File\n");

        info_file_msg info;
        recv(cdata->csock, &info, sizeof(info), 0);
        //printf("ID msg %u\nNome do arquivo %s\nTamanho do arquivo %u\n",
        //    info.msg_id,info.nome_arquivo,info.tamanho_arquivo);

        //Cria mensagem Ok
        mini_msg sms;
        sms.msg_id = 4;
        send(cdata->csock, &sms, sizeof(sms), 0);
        //printf("Mensagem enviada: OK\n");
        

        //Cria thread UDP
        UDP_connection(valor,info,cdata);
        //printf("OIE\n");
        monta_arquivo(info);
    }
}

/*Esta função é chamada toda vez que criamos uma nova thread para um cliente, ou seja,
novo cliente implica em nova thread. O que por sua vez que o cliente é acionado a um vetor de clientes.
Ela trata de receber as mensagens dos clientes, trata-las e dar as devidas respostas. As respostas incluem:
alocar uma porta UDP para elas, criar um soquete UDP, etc.*/

void * client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);
    int cid = addCliente(cdata);
    cdata->id = cid;
    
    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

    mini_msg sms;
    memset(&sms, 0, sizeof(sms));
    size_t count;

    //CRIA UDP
    //Cria socket UDP
    int udp_socket;
    struct sockaddr_in6 server_addr_UDP, client_addr_UDP;

    if((udp_socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP))< 0){
        logexit("Soquete UDP");
    }

    memset(&server_addr_UDP, 0, sizeof(server_addr_UDP)); 
    memset(&client_addr_UDP, 0, sizeof(client_addr_UDP)); 

    //Seta porto e IP do UDP
    server_addr_UDP.sin6_family = AF_INET6;
    //TODO escolher e alocar um porto diferente por cliente
    server_addr_UDP.sin6_port = htons(2000);
    printf("Porta inicializada: %u\n",ntohs(server_addr_UDP.sin6_port));
    //printf("Funfou\n");
    
    //UDP bind
    if(bind(udp_socket, (struct sockaddr*)&server_addr_UDP, sizeof(server_addr_UDP)) < 0){
        logexit("bind UDP");
    }
    //printf("Done with binding\n");

    // FIM UDP

    //Seta struct UDP
    udp_data valor;
    valor.udp_socket = udp_socket;
    valor.client_addr_UDP = client_addr_UDP;
    
    
    while(1){
        memset(&sms, 0, sizeof(sms));
        count = recv(cdata->csock, &sms, sizeof(sms), MSG_PEEK);
        printf("Mini msg %u Bytes lidos %ld \n",sms.msg_id, count);
        if(count > 0){
            cria_mensagem(sms.msg_id, server_addr_UDP, cdata, valor);
        }
       
        else {
            break;
        }
    }


    removeCliente(cid);
    free(cdata);
    close(cdata->csock);
    pthread_exit(EXIT_SUCCESS);
}


int main(int argc, char **argv) {

    if (argc < 2) {
        usage(argc, argv);
    }
    
    inicializaClientes();

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init("v6", argv[1], &storage)) {
        usage(argc, argv);
    }

    //TCP Channel
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }


    //TCP reuso
    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    //TCP bind
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) { 
            logexit("accept");
        }

       struct client_data *cdata = malloc(sizeof(*cdata));
        if (!cdata) {
            logexit("malloc");
        }
        cdata->csock = csock;
        memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);

    }

    exit(EXIT_SUCCESS);
}

