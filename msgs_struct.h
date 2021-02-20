typedef struct {
    unsigned short int msg_id;
} hello_msg;

typedef struct {
    unsigned short int msg_id;
    unsigned udp_port;
} connection_msg;