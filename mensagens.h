#include <stdio.h>
#include <stdlib.h>
#include "common.h"

//Usada pelo Hello, OK e Fim
typedef struct {
    unsigned short int msg_id;
} mini_msg;

typedef struct {
    unsigned short int msg_id;
    unsigned udp_port;
} connection_msg;

typedef struct {
    unsigned short int msg_id;
    char nome_arquivo[15];
    unsigned tamanho_arquivo;
} info_file_msg;

typedef struct {
    unsigned short int msg_id;
    unsigned num_sequencia;
} ack_msg;

//UDP
typedef struct {
    unsigned short int msg_id;
    unsigned num_sequencia;
    unsigned short int payload_size;
    char payload [BUFSZ];
} udp_file_msg;


