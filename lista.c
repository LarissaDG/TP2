#include "lista.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


void CriaListaVazia(lista* lista){
    lista->primeiro = (apontador)malloc(sizeof(celula));
    lista->tam = 0;
    lista->primeiro->prox = NULL;
}

int VerificaVazia(lista lista){
    return(lista.tam == 0);
}

int BuscaItem(lista lista, char* tag){
    apontador aux;
    aux = lista.primeiro;
    while(aux->prox != NULL){
        aux = aux->prox;
        if(strcmp(aux->data, tag) == 0) return 1;
    }
    return 0;
}

//Caso ja esteja na lista, nao faz nada
void InsereFinal(lista* lista, char *tag){
    apontador aux;
    aux = lista->primeiro;
    while(aux->prox != NULL){
        aux = aux->prox;
        if(strcmp(aux->data, tag) == 0) return;
    }

    apontador novo = (apontador)malloc(sizeof(celula));
    strcpy(novo->data, tag);
    novo->prox = NULL;

    aux->prox = novo;
    lista->tam++;
}

void RemoveItem(lista* lista, char* tag){
    apontador aux;
    aux = lista->primeiro;
    while(aux->prox != NULL){
        if(strcmp(aux->prox->data, tag) == 0){
            apontador prox = aux->prox->prox;
            DesalocaCelula(aux->prox);
            aux->prox = prox;
            lista->tam--;
        }
        aux = aux->prox;
        if(aux == NULL) break;
    }
}

void DesalocaLista(lista *lista){
    apontador aux, aux2;
    aux = lista->primeiro;
    while(aux != NULL){
        aux2 = aux->prox;
        DesalocaCelula(aux);
        aux = aux2;
    }   
}

void ImprimeLista(lista lista){
    apontador aux;
    aux = lista.primeiro->prox;
    printf("Inicio lista\n");
    while(aux != NULL){
        printf("%s\n",aux->data);
        aux = aux->prox;
    }
    printf("Fim lista\n");
}



// Celula
void DesalocaCelula(celula* celula){
    free(celula);
}