#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

#define BUFSZ 500


typedef struct {
    unsigned short int msg_id;
} hello_msg;

void tostring(hello_msg sms, char buffer[], int tam);

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);

void determina_etapa(int num, char buffer[]);
