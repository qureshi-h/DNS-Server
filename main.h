#ifndef MAIN_H
#define MAIN_H

#include <inttypes.h>
#include "helper1.h"

char* get_time();
void print_log(FILE* file, char* mode, question_t* question, answer_t* answer);
void print_ip(FILE* file, answer_t* answer);
int get_client_socket();
uint8_t* get_query(int socket_fd, int* new_socket);
int get_server_socket(char* nodename, char* server_port);

#endif

