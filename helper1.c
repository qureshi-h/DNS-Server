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

#include "helper.h"

/*
Parse the buffer to extract the header elements into a header struct
*/
header_t* get_header(uint16_t* buffer, int* pos) {

    header_t* header = (header_t*)malloc(sizeof(*header));
    assert(header);

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

/*
Parses the buffer to extract the question elements and stores them in a struct
*/
question_t* get_question(uint8_t* buffer, int* pos) {

    uint8_t label_size;

    question_t* question = (question_t*)malloc(sizeof(*question));
    assert(question);

    question->q_name = (char*)malloc(sizeof(*(question->q_name)));
    assert(question->q_name);

    question->q_name[0] = '\0';
    question->q_name_size = 0;

    label_size = buffer[(*pos)++];
    while (label_size) {

        question->q_name = (char*)realloc(question->q_name,
            sizeof(*(question->q_name)) * (label_size + question->q_name_size + 1));
        assert(question->q_name);

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

/*
Parses the buffer to extract the question elements and stores them in a struct
*/
answer_t* get_answer(uint16_t* buffer) {

    answer_t* answer = (answer_t*)malloc(sizeof(*answer));
    assert(answer);

    int pos = 0;
    answer->name = ntohs(buffer[pos]);
    answer->type = ntohs(buffer[++pos]);
    answer->class = ntohs(buffer[++pos]);
    answer->ttl = ntohs(*(uint32_t*)(buffer + pos + 1));
    answer->rd_length = ntohs(buffer[pos += 3]);

    answer->rd_data = (uint8_t*)malloc(sizeof(*(answer->rd_data)) * answer->rd_length);
    assert(answer->rd_data);
    memcpy(answer->rd_data, buffer + pos + 1, answer->rd_length);

    return answer;
}

/*
returns the value of r_code bit from the flag byte
*/
uint8_t get_r_code(uint8_t flags) {
    return flags & 15;
}
