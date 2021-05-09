#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include "main.h"

#define HEADER_COUNT 7
#define TIMESTAMP_LEN 24
#define IP_ADDRESS_SIZE 2

int main(int argc, char* argv[]) {

    FILE* log_file = fopen("./dns_svr.log", "w");

    header_t* header = get_header();
    printf("%d ", header->size);
    printf("%u ", header->id);
    printf("%u ", header->flags);
    printf("%u ", header->qd_count);
    printf("%u ", header->an_count);
    printf("%u ", header->ns_count);
    printf("%u \n", header->ar_count);

    question_t* question = NULL;
    if (header->qd_count) {
        question = get_question();
        printf("%s %u %u\n", question->q_name, question->q_type, question->q_class);
    }

    answer_t* answer = NULL;
    if (header->an_count) {
        answer = get_answer();
        printf("%u %u %u %u ", answer->name, answer->type, answer->class, answer->ttl);
        for (int i = 0; i < answer->rd_length; i++) {
            printf("%u:", answer->rd_data[i]);
        }
    }  
  
    int flag = 0;
    fprintf(log_file, "%s %s is at ", get_time(), question->q_name);
    for (int i = 0; i < answer->rd_length; i++) {
        if (answer->rd_data[i]) {
            fprintf(log_file, "%x", answer->rd_data[i]);
            
            if (i != answer->rd_length - 1)
                fprintf(log_file, ":");
        }
        else if (!flag) {
            fprintf(log_file, ":");
            flag++;      
	}  
    } 

    printf("\n");
    return 0;
}

header_t* get_header() {

    uint16_t buffer[HEADER_COUNT];
    fread(buffer, sizeof(*buffer), HEADER_COUNT, stdin);

    header_t* header = (header_t*)malloc(sizeof(*header));
    header->size = ntohs(buffer[0]);
    header->id = ntohs(buffer[1]);
    header->flags = ntohs(buffer[2]);
    header->qd_count = ntohs(buffer[3]);
    header->an_count = ntohs(buffer[4]);
    header->ns_count = ntohs(buffer[5]);
    header->ar_count = ntohs(buffer[6]);

    return header;
}

question_t* get_question() {

    uint8_t label_size;

    question_t* question = (question_t*)malloc(sizeof(*question));
    question->q_name = (char*)malloc(sizeof(*(question->q_name)));
    question->q_name[0] = '\0';
    question->q_name_size = 0;

    fread(&label_size, sizeof(label_size), 1, stdin);
    while (label_size) {

        question->q_name = (char*)realloc(question->q_name,
            sizeof(*(question->q_name)) * (label_size + question->q_name_size + 1));
        fread(question->q_name + question->q_name_size, sizeof(*(question->q_name)),
            label_size, stdin);
        question->q_name_size += label_size + 1;
        question->q_name[question->q_name_size - 1] = '.';
        fread(&label_size, sizeof(label_size), 1, stdin);
    }

    question->q_name[question->q_name_size - 1] = '\0';

    fread(&(question->q_type), sizeof(question->q_type), 1, stdin);
    fread(&(question->q_class), sizeof(question->q_class), 1, stdin);
    
    question->q_type = ntohs(question->q_type);
    question->q_class = ntohs(question->q_class);

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
    answer->rd_length = ntohs(answer->rd_length) / IP_ADDRESS_SIZE;

    answer->rd_data = (uint16_t*)malloc(sizeof(*(answer->rd_data)) * answer->rd_length);
    fread(answer->rd_data, sizeof(*(answer->rd_data)), answer->rd_length, stdin);
    for (int i = 0; i < answer->rd_length; i++) {
        answer->rd_data[i] = ntohs(answer->rd_data[i]);
    }

    return answer;
}

char* get_time() {

    time_t timer = time(NULL);
    struct tm* tm_info = localtime(&timer);
    char* timestamp = (char*)malloc(sizeof(*timestamp) * (TIMESTAMP_LEN + 1));

    strftime(timestamp, TIMESTAMP_LEN + 1, "%Y-%m-%dT%H:%M:%S+0000", tm_info);
    return timestamp;

}

