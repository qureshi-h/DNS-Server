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
#define RESPONSE "response"
#define HEADER_SIZE_LENGTH 2
#define PORT "8053"
#define MAX_MSG_SIZE 512
#define QUAD_A 28

int main(int argc, char* argv[]) {

    FILE* log_file = fopen("./dns_svr.log", "w");
    int pos, temp_pos, client_socket_fd;

    uint8_t* query;
    header_t* header;
    question_t* question;
    uint8_t buffer[MAX_MSG_SIZE];

    int socket_fd = get_client_socket();
    int server_socket_fd;

    while (1) {

        pos = 0;

        query = get_query(socket_fd, &client_socket_fd);
        header = get_header((uint16_t*)query, &pos);
        question = get_question(query, &pos);

        printf("%s\n", question->q_name);
        
        server_socket_fd = get_server_socket(argv[1], argv[2]);
        print_log(log_file, QUERY, question, NULL);
       
        if (question->q_type != QUAD_A) {
            print_log(log_file, "unimplemented", NULL, NULL);
            query[4] = query[4] | 128;
            query[5] = query[5] | 4;
            assert(write(client_socket_fd, query, header->size) == header->size);
            continue;
        }

        printf("header size %u\n", header->size);
        assert(write(server_socket_fd, query, header->size) == header->size);
        
        uint16_t bytes_read = read(server_socket_fd, buffer, MAX_MSG_SIZE);
        printf("bytes %u\n", bytes_read);
        send(client_socket_fd, buffer, bytes_read, 0);

        temp_pos = 0;
        header = get_header((uint16_t*)buffer, &temp_pos);
        
	if (!header->an_count)
       	    continue;

	answer_t* answer = get_answer((uint16_t*)(buffer + pos));
        if (answer->type == QUAD_A) {
            print_log(log_file, RESPONSE, question, answer);
        }
        else {
            continue;
        }

        close(server_socket_fd);
        close(client_socket_fd);
    }
    
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

uint8_t* get_query(int socket_fd, int* new_socket) {

    if (listen(socket_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_storage client_address;
    socklen_t client_addr_size = sizeof(client_address);
    *new_socket = accept(socket_fd, (struct sockaddr*)&client_address, &client_addr_size);
    if (*new_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    int bytes_read = 0;
    uint16_t* buffer = (uint16_t*)malloc(sizeof(*buffer));
    assert(read(*new_socket, buffer, 2) == 2);

    uint16_t size = ntohs(*buffer);
    buffer = (uint16_t*)realloc(buffer, sizeof(*buffer) * MAX_MSG_SIZE);
    
    while (bytes_read != size) {
        bytes_read += read(*new_socket, buffer + 1, MAX_MSG_SIZE - 1);
    }

    return (uint8_t*)buffer;
}

int get_server_socket(char* nodename, char* server_port) {

    int status, socket_fd;
    struct addrinfo hints, *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(nodename, server_port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    socket_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (socket_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("connection %d \n", connect(socket_fd, servinfo->ai_addr, servinfo->ai_addrlen));

    freeaddrinfo(servinfo);
    return socket_fd;
}

void print_log(FILE* file, char* mode, question_t* question, answer_t* answer) {

    fprintf(file, "%s ", get_time());
    if (!strcmp(QUERY, mode))
        fprintf(file, "requested %s\n", question->q_name);
    else if (!strcmp(RESPONSE, mode)) {
        fprintf(file, "%s is at ", question->q_name);
        print_ip(file, answer);
    }
    else {
        fprintf(file, "unimplemented request\n");
    }

    fflush(file);
}

void print_ip(FILE* file, answer_t* answer) {

    char ip_address[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, answer->rd_data, ip_address, sizeof(ip_address));

    fprintf(file, "%s\n", ip_address);
    fflush(file);
}

header_t* get_header(uint16_t* buffer, int* pos) {

    header_t* header = (header_t*)malloc(sizeof(*header));
    header->size = buffer[*pos];
    header->id = ntohs(buffer[++(*pos)]);
    header->flags = ntohs(buffer[(++(*pos))]);
    header->qd_count = ntohs(buffer[(++(*pos))]);
    header->an_count = ntohs(buffer[++(*pos)]);
    header->ns_count = ntohs(buffer[++(*pos)]);
    header->ar_count = ntohs(buffer[++(*pos)]);

    ++(*pos);
    *pos *= 2;
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
        label_size = buffer[(*pos)++];
    }

    question->q_name[question->q_name_size - 1] = '\0';

    question->q_type = ntohs(*(uint16_t*)(buffer + *pos));
    question->q_class = ntohs(*(uint16_t*)(buffer + (*pos += 2)));

    *pos += 2;
    return question;
}

answer_t* get_answer(uint16_t* buffer) {

    answer_t* answer = (answer_t*)malloc(sizeof(*answer));

    int pos = 0;
    answer->name = ntohs(buffer[pos]);
    answer->type = ntohs(buffer[++pos]);
    answer->class = ntohs(buffer[++pos]);
    answer->ttl = ntohs(*(uint32_t*)(buffer + pos + 1));
    answer->rd_length = ntohs(buffer[pos += 3]);

    answer->rd_data = (uint8_t*)malloc(sizeof(*(answer->rd_data)) * answer->rd_length);
    memcpy(answer->rd_data, buffer + pos + 1, answer->rd_length);

    return answer;
}

char* get_time() {

    time_t timer = time(NULL);
    struct tm* tm_info = localtime(&timer);
    char* timestamp = (char*)malloc(sizeof(*timestamp) * (TIMESTAMP_LEN + 1));

    strftime(timestamp, TIMESTAMP_LEN + 1, "%FT%T%z", tm_info);
    return timestamp;
}

uint8_t get_r_code(uint8_t flags) {

    return flags & 15;
}
