#include "common.h"
#include "lista.h"

#include <pthread.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/queue.h>

#include <sys/socket.h>
#include <sys/types.h>

#define MAX_CLIENTES 255

//Entreada tipo do protocolo e o porto

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

void processa_entrada(struct client_data *cliente, char* buf, int cid, char* dest, struct sockaddr_in6 server_addr_UDP){
    unsigned porta = 0;
    if(isInvalido(cliente,buf,sizeof(buf))){
        strcpy(dest,"Invalid caracter");
    }
    if(buf[0] == '1'){
        printf("Mensagem recebida: Hello\n");
        porta = ntohl(server_addr_UDP.sin6_port);
        determina_etapa(2,porta,dest);
        return ;
    }
    if(buf[0] == '3'){
        printf("Mensagem recebida: info file\n");
        determina_etapa(4, porta, dest);
        return ;
    }
    if(buf[0] == '6'){
        printf("Mensagem recebida: file\n");
        determina_etapa(7, porta, dest);
        return ;
    }
    if(buf[0] == '5'){
        printf("Mensagem recebida: FIM\n");
        strcpy(dest,"fim");
        return ;
    }
    strcpy(dest,"nada");
}



//Funcoes de rede

//Retorna 1 se sucesso e 0 caso contrario
int send_msg(char *output, struct client_data *cdata){
    int count = send(cdata->csock, output, strlen(output) + 1, 0);
    return count == (strlen(output) + 1);
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

    char buf[BUFSZ];
    char dest[BUFSZ];
    memset(buf, 0, BUFSZ);
    size_t count;

    /* CRIA UDP*/
    //Cria socket UDP
    int udp_socket;
    struct sockaddr_in6 server_addr_UDP;
    udp_socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(udp_socket < 0){
        logexit("Soquete UDP");
    }

    //Seta porto e IP do UDP
    server_addr_UDP.sin6_family = AF_INET6;
    server_addr_UDP.sin6_port = htons(2000);
    //printf("Funfou\n");
    
    //UDP bind
    if(bind(udp_socket, (struct sockaddr*)&server_addr_UDP, sizeof(server_addr_UDP)) < 0){
        //printf("Couldn't bind to the port\n");
        logexit("bind UDP");
    }
    //printf("Done with binding\n");

    // FIM UDP
    
    
    while(1){
        memset(buf, 0, BUFSZ);
        count = recv(cdata->csock, buf, BUFSZ - 1, 0);
        //printf("%s\n",buf);
        if(count > 0){
            processa_entrada(cdata,buf, cid, dest, server_addr_UDP);
            if(strcmp(dest,"Invalid caracter")==0) break;
            if(strcmp(dest,"fim")==0){
                printf("Fim\n");
                exit(1);
            }
            if(strcmp(dest,"nada")==0){
                logexit("Mensagens");
            }
            count = send(cdata->csock, dest, strlen(dest) + 1, 0);
            if(count != strlen(dest) + 1) break;
        }
        else break;

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

