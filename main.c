#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "main.h"

#define TIMESTAMP_LEN 24
#define QUERY "query"
#define HEADER_SIZE_LENGTH 2
#define PORT "8053"
#define MAX_MSG_SIZE 512

int main(int argc, char* argv[]) {

    FILE* log_file = fopen("./dns_svr.log", "w");
    int pos = 0;

    int client_socket_fd = get_client_socket();
    uint8_t* query = get_query(client_socket_fd);
    header_t *header = get_header(query, &pos);
    question_t *question = get_question(query, &pos);
   
    printf("%s\n", question->q_name);

    int server_socket_fd = get_server_socket(argv[1], argv[2]);
    assert(send(server_socket_fd, query, sizeof(query), 0) == sizeof(query));

    printf("sent\n");

    char buffer[MAX_MSG_SIZE];
    uint16_t bytes_read = read(server_socket_fd, buffer, MAX_MSG_SIZE);
    printf("read");
    printf("%u", bytes_read);


    return 0;
}

int get_client_socket() {

    int status, socket_fd;
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socket_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    if (bind(socket_fd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(servinfo);
    return socket_fd;
}

uint8_t* get_query(int socket_fd) {

    if (listen(socket_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_storage client_address;
    socklen_t client_addr_size = sizeof(client_address);
    int new_socket = accept(socket_fd, (struct sockaddr*)&client_address, &client_addr_size);
    if (new_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    uint16_t query_length = 0;
    assert(read(new_socket, &query_length, 2) == 2);
    
    query_length = ntohs(query_length);

    query_length -= 2;
    uint8_t* buffer = (uint8_t*)malloc(sizeof(*buffer) * query_length);
    assert(read(new_socket, buffer, query_length) == query_length);

    return buffer;
}

int get_server_socket(char* nodename, char* server_port) {

    int status, socket_fd;
    struct addrinfo hints, * servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((status = getaddrinfo(nodename, server_port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socket_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    connect(socket_fd, servinfo->ai_addr, servinfo->ai_addrlen);

    freeaddrinfo(servinfo);
    return socket_fd;
}

void print_log(FILE* file, char* mode, question_t* question, answer_t* answer) {

    fprintf(file, "%s ", get_time());
    if (!strcmp(QUERY, mode))
        fprintf(file, "requested %s", question->q_name);
    else if (answer) {
        fprintf(file, "%s is at ", question->q_name);
        print_ip(file, answer);
    }

    fflush(file);
}

void print_ip(FILE* file, answer_t* answer) {

    char ip_address[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, answer->rd_data, ip_address, sizeof(ip_address));

    fprintf(file, "%s", ip_address);

    fflush(file);
}

header_t* get_header(uint8_t* buffer, int* pos) {

    header_t* header = (header_t*)malloc(sizeof(*header));
    header->id = ntohs(buffer[*pos]);
    header->flags = ntohs(buffer[*pos += 2]);
    header->qd_count = ntohs(buffer[*pos += 2]);
    header->an_count = ntohs(buffer[*pos += 2]);
    header->ns_count = ntohs(buffer[*pos += 2]);
    header->ar_count = ntohs(buffer[*pos += 2]);

    *pos += 2;
    return header;
}

question_t* get_question(uint8_t* buffer, int* pos) {

    uint8_t label_size;

    question_t* question = (question_t*)malloc(sizeof(*question));
    question->q_name = (char*)malloc(sizeof(*(question->q_name)));
    question->q_name[0] = '\0';
    question->q_name_size = 0;

    label_size = buffer[(*pos)++];
    while (label_size) {

        question->q_name = (char*)realloc(question->q_name,
            sizeof(*(question->q_name)) * (label_size + question->q_name_size + 1));
        
        memcpy(question->q_name + question->q_name_size, buffer + *pos, label_size);
        *pos += label_size;
        
        question->q_name_size += label_size + 1;
        question->q_name[question->q_name_size - 1] = '.';
        label_size = buffer[++(*pos)];
    }

    question->q_name[question->q_name_size - 1] = '\0';

    question->q_type = ntohs(buffer[*pos]);
    question->q_class = ntohs(buffer[*pos += 2]);

    *pos += 2;

    return question;
}

answer_t* get_answer() {

    answer_t* answer = (answer_t*)malloc(sizeof(*answer));

    fread(&(answer->name), sizeof(answer->name), 1, stdin);
    answer->name = ntohs(answer->name);
    fread(&(answer->type), sizeof(answer->type), 1, stdin);
    answer->type = ntohs(answer->type);
    fread(&(answer->class), sizeof(answer->class), 1, stdin);
    answer->class = ntohs(answer->class);
    fread(&(answer->ttl), sizeof(answer->ttl), 1, stdin);
    answer->ttl = ntohs(answer->ttl);
    fread(&(answer->rd_length), sizeof(answer->rd_length), 1, stdin);
    answer->rd_length = ntohs(answer->rd_length);

    answer->rd_data = (uint8_t*)malloc(sizeof(*(answer->rd_data)) * answer->rd_length);
    fread(answer->rd_data, sizeof(*(answer->rd_data)), answer->rd_length, stdin);

    return answer;
}

char* get_time() {

    time_t timer = time(NULL);
    struct tm* tm_info = localtime(&timer);
    char* timestamp = (char*)malloc(sizeof(*timestamp) * (TIMESTAMP_LEN + 1));

    strftime(timestamp, TIMESTAMP_LEN + 1, "%FT%T%z", tm_info);
    return timestamp;

}

