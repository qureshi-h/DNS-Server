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
#define PORT "8053"
#define MAX_MSG_SIZE 512
#define QUAD_A 28
#define NUM_BYTES_HEADER 14


/*
Get the query from the client, forwards in to the upstream server, and 
relays the response back to the client. Keeps logs throughout the process.
*/
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
            query[5] = query[5] | 4;
            write(client_socket_fd, query, header->size);
            close(server_socket_fd);
            close(client_socket_fd);
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

/*
Creates a socket will will be used to lisen for clients
*/
int get_client_socket() {

    int status, socket_fd;
    struct addrinfo hints, * servinfo;

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

/*
accepts a tcp connection with a client, and stores and returns their query.
*/
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

    uint8_t* buffer = (uint8_t*)malloc(sizeof(*buffer) * 2);

    if (read(*new_socket, buffer, 2) != 2)
        assert(read(*new_socket, buffer + 1, 1));

    int bytes_read = 2;
    uint16_t size = ntohs(*((uint16_t*)buffer));
    printf("size %u\n", size);
    buffer = (uint8_t*)realloc(buffer, sizeof(*buffer) * size);

    while (bytes_read != size) {
        printf("%d ", bytes_read);
        bytes_read += read(*new_socket, buffer + bytes_read, size - bytes_read);
    }
    return buffer;
}

/*
Connects with the upstream server and returns the file descriptor.
*/
int get_server_socket(char* nodename, char* server_port) {

    int status, socket_fd;
    struct addrinfo hints, * servinfo;

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

/*
Prints logs into the file based on the mode.
*/
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

/*
Prints the ip address into the given file.
*/
void print_ip(FILE* file, answer_t* answer) {

    char ip_address[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, answer->rd_data, ip_address, sizeof(ip_address));

    fprintf(file, "%s\n", ip_address);
    fflush(file);
}

/*
Return the timestamp in the required format.
*/
char* get_time() {

    time_t timer = time(NULL);
    struct tm* tm_info = localtime(&timer);
    char* timestamp = (char*)malloc(sizeof(*timestamp) * (TIMESTAMP_LEN + 1));

    strftime(timestamp, TIMESTAMP_LEN + 1, "%FT%T%z", tm_info);
    return timestamp;
}


