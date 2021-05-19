#ifndef MAIN_H
#define MAIN_H

#include <inttypes.h>

typedef struct header {

    uint16_t size;
    uint16_t id;
    uint16_t flags;
    uint16_t qd_count;
    uint16_t an_count;
    uint16_t ns_count;
    uint16_t ar_count;
} header_t;

typedef struct question {

    char* q_name;
    int q_name_size;
    uint16_t q_type;
    uint16_t q_class;
} question_t;

typedef struct answer {
    
    uint16_t name;
    uint16_t type;
    uint16_t class;
    int ttl;
    uint16_t rd_length;
    uint8_t* rd_data;
} answer_t;

char* get_time();
header_t* get_header(uint16_t* buffer, int* pos);
question_t* get_question(uint8_t* buffer, int* pos);
answer_t* get_answer();
void print_log(FILE* file, char* mode, question_t* question, answer_t* answer);
void print_ip(FILE* file, answer_t* answer);
int get_client_socket();
uint8_t* get_query(int socket_fd);
int get_server_socket(char* nodename, char* server_port);

#endif

