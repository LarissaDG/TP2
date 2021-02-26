/*void * UDP_thread(void *data){
    printf("Entrou na etapa 4\n");

    udp_data *valor = (udp_data*) data;
    udp_file_msg dados;

    memset(&dados, 0, sizeof(dados));
    
    printf("Tam udp_file_msg %ld\n",sizeof(udp_file_msg));
    size_t count;
    socklen_t client_struct_length = sizeof(valor->client_addr_UDP);

    // Receive client's message:
    while (1){
        if((count = recvfrom(valor->udp_socket,&dados, sizeof(dados),0,(struct sockaddr*)
            &valor->client_addr_UDP, &client_struct_length)) < 0){
            logexit("receive UDP");
        }

        printf("Count %ld\n",count );
        logexit("receive UDP");

        imprime_pacote_UDP(dados); 
        getchar();
        if(count <= 0){
            break;
        }
    }

    printf("Estou aki\n");

    pthread_exit(EXIT_SUCCESS);
     
}*/

void UDP_connection(udp_data valor){

    udp_file_msg dados;

    memset(&dados, 0, sizeof(dados));
    
    printf("Tam udp_file_msg %ld\n",sizeof(udp_file_msg));
    size_t count;
    socklen_t client_struct_length = sizeof(valor.client_addr_UDP);

    // Receive client's message:
    while (1){
        if((count = recvfrom(valor.udp_socket,&dados, sizeof(dados),0,(struct sockaddr*)
            &valor.client_addr_UDP, &client_struct_length)) < 0){
            logexit("receive UDP");
        }

        imprime_pacote_UDP(dados); 
        getchar();
        if(count <= 0){
            break;
        }
    }

    printf("Estou aki\n");
}

void cria_mensagem(unsigned short int msg_id,struct sockaddr_in6 server_addr_UDP,struct client_data *cdata, udp_data valor){

    if(msg_id == 1){
        //Recebe a mensagem Hello
        mini_msg sms;
        recv(cdata->csock, &sms, sizeof(sms), 0);
        printf("Mensagem recebida: Hello\n");

        //Cria a mensagem Connection
        connection_msg conex;
        conex.msg_id = 2;
        conex.udp_port = server_addr_UDP.sin6_port;
        printf("PORTA: %u\n", conex.udp_port);
        send(cdata->csock, &conex, sizeof(conex), 0);
        printf("Mensagem enviada: Connection\n");
    }

    if(msg_id == 3){
        //Recebe a mensagem Info File
        printf("Mensagem recebida: Info File\n");

        info_file_msg info;
        int receive = recv(cdata->csock, &info, sizeof(info), 0);
        printf("ID msg %u\nNome do arquivo %s\nTamanho do arquivo %u\n Bytes lidos %d \n",
            info.msg_id,info.nome_arquivo,info.tamanho_arquivo,receive);

        //Cria mensagem Ok
        mini_msg sms;
        sms.msg_id = 4;
        send(cdata->csock, &sms, sizeof(sms), 0);
        printf("Mensagem enviada: OK\n");
        

        //Cria thread UDP
        UDP_connection(valor);
        //udp_data *data = &valor;
        //pthread_t tid;
        //pthread_create(&tid, NULL,UDP_thread,data);

    }
}

//Funcoes de rede

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

