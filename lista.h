#ifndef LISTA_H
#define LISTA_H

#define TAGSZ 500

typedef struct celula *apontador;

typedef struct celula{
  char data[TAGSZ];
  apontador prox;
}celula;

typedef struct lista{
  apontador primeiro;
  int tam;
}lista;


void CriaListaVazia(lista*);
int VerificaVazia(lista);
void InsereFinal(lista*, char*);
void ImprimeLista(lista);
void RemoveItem(lista*, char*);
void DesalocaLista(lista*);
int BuscaItem(lista, char*);

void DesalocaCelula(celula*);

#endif