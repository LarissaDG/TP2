#include "common.h"
#include "lista.h"
#include "msgs_struct.h"

#include <pthread.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/queue.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 500
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
    lista tags;
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
            printf("i = %d\n", i);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    printf("i = %d\n",i);
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

void inscrever (struct client_data *cliente, char* dest,char *buf, int tam){
  char chave [tam];
  removeSinal(buf,chave,tam);
  //Verifico se já está 
  //Se sim mandar msg
  if(BuscaItem(cliente->tags,chave)){
     strcpy(dest,"already subscribed ");
  }
  //Se não adiciono na lista
  else{
   InsereFinal(&(cliente->tags),chave);
   strcpy(dest,"subscribed "); 
  }
}

void desinscrever(struct client_data *cliente ,char* dest,char *buf, int tam){
    char chave [tam];
    removeSinal(buf,chave,tam);
    //procura se tá inscrito
    //se tá remove
    if(BuscaItem(cliente->tags,chave)){
     RemoveItem(&(cliente->tags),chave);
     strcpy(dest,"unsubscribed ");
    }
    //se não tem que mandar uma msg
    else{
       strcpy(dest,"not subscribed "); 
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

int hashtag(char **p, char buf[], int tam){
    char *q;

    //se o caracter em p for igual a null ou '\0'
    if (*p == NULL || **p == '\0'){
        return 0;
    }

    q = strchr(*p, '#');//Procura a primeira ocorrencia do caracter na string
    //Remove coisas tipo: ######
    while (q && isValido(q[1]) == 0) {
        q = strchr(q + 1, '#');
    }

    if (q) {
        size_t n = 0;

        q++;                    // skip hash sign

        //Preenche buf com a tag
        while (n + 1 < tam && isValido(*q)) {
            buf[n++] = *q++;
        }

        if (tam){
            buf[n] = '\0';
        } // terminate buffer
        *p = q;                 // remember position

        return 1;               // hashtag found
    }

    return 0;                   // nothing found
}


int isKill(struct client_data *cliente ,char *buf, int tam){
    if(strcmp(buf,"##kill\n")==0){
        return 1;
    }
    return 0;
}


//Retorna 1 se sucesso e 0 caso contrario
int send_msg(char *output, struct client_data *cdata){
    int count = send(cdata->csock, output, strlen(output) + 1, 0);
    return count == (strlen(output) + 1);
}

void publish(lista tags, char *msg, int cid){
    printf("Publish\n");
    pthread_mutex_lock(&clients_mutex);
    int i = 0;
    //Percorre clientes
    for(i=0; i < MAX_CLIENTES; i++){
        if(Clientes[i] != NULL){
            printf("Visitando cliente %d de id %d\n", i, Clientes[i]->id);
            ImprimeLista(Clientes[i]->tags);

            //CLiente que enviou a mensagem nao deve receber
            if(Clientes[i]->id == cid){
                printf("Lista do cliente que enviou\n");
                ImprimeLista(Clientes[i]->tags);
                continue;
            }
            else{
                apontador aux;
                aux = tags.primeiro;
                while(aux->prox != NULL){
                    aux = aux->prox;
                    //Checa se esse cliente tem alguma das tags
                    if(BuscaItem(Clientes[i]->tags, aux->data)){
                        send_msg(msg, Clientes[i]);
                        printf("Achou a tag %s\n", aux->data);
                        break;
                    }
                }
            }   
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}


char * confirmaInscricao(struct client_data *cliente, char* buf, int cid){
    lista tags;
    char item[BUFSZ];
    CriaListaVazia(&tags);
    printf("ConfirmaInscricao\n");
    printf("Mensagem enviada = %s", buf);

    if(isInvalido(cliente,buf,sizeof(buf))){
        char dest[BUFSZ];
        return strcpy(dest,"Invalid caracter");
    }
   
    if(isKill(cliente,buf,sizeof(buf))){
        char dest[BUFSZ];
        return strcpy(dest,"kill");
    }

    if(buf[0] == '+'){
        char dest[BUFSZ];
        inscrever(cliente,dest,buf,sizeof(buf));
        return strcat(dest,buf);
    }
    
    if(buf[0] == '-'){
        char dest[BUFSZ];
        desinscrever(cliente,dest,buf,sizeof(buf));
        return strcat(dest,buf);
    }
    char *search_str = (char*)malloc(BUFSZ*sizeof(char));
    strcpy(search_str, buf);
    
    while(hashtag(&search_str,item,BUFSZ)){
        if(strchr(item,'#')==NULL){
             InsereFinal(&tags,item);
        }
    }
    publish(tags, buf, cid);
    DesalocaLista(&tags);
    //mensagem pra enviar
    //ImprimeLista(tags);
    return "tag enviada";
}


//Funcoes de rede

void * client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);
    CriaListaVazia(&(cdata->tags));
    int cid = addCliente(cdata);
    cdata->id = cid;
    
    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    size_t count;
    
    while(1){
        memset(buf, 0, BUFSZ);
        count = recv(cdata->csock, buf, BUFSZ - 1, 0);
        if(count > 0){
            //publish or command (+, - , ##kill ...)
            char * output = confirmaInscricao(cdata,buf, cid);
            printf("Output = -%s-\n", output);
            if(strcmp(output,"Invalid caracter")==0) break;
            if(strcmp(output,"tag enviada")==0){
                continue;
            }
            if(strcmp(output,"kill")==0){
                exit(1);
            }
            count = send(cdata->csock, output, strlen(output) + 1, 0);
            if(count != strlen(output) + 1) break;
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

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

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
