#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <arpa/inet.h>

#include "main.h"

#define HEADER_COUNT 7
#define TIMESTAMP_LEN 24

int main(int argc, char* argv[]) {

    FILE* log_file = fopen("./dns_svr.log", "w");

    header_t* header = get_header();
    printf("%d ", header->size);
    printf("%u ", header->id);
    printf("%u ", header->flags);
    printf("%u ", header->qd_count);
    printf("%u ", header->an_count);
    printf("%u ", header->ns_count);
    printf("%u ", header->ar_count);


    fprintf(log_file, "%s ", get_time());
    uint8_t label_size;
    char* label;

    fread(&label_size, sizeof(label_size), 1, stdin);
    while (label_size) {

        label = (char*)malloc(sizeof(*label) * (label_size + 1));
        fread(label, sizeof(*label), label_size, stdin);
        label[label_size] = '\0';
        printf("%s.", label);
        fread(&label_size, sizeof(label_size), 1, stdin);
    }
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

char* get_time() {

    time_t timer = time(NULL);
    struct tm* tm_info = localtime(&timer);
    char* timestamp = (char*)malloc(sizeof(*timestamp) * (TIMESTAMP_LEN + 1));

    strftime(timestamp, TIMESTAMP_LEN + 1, "%Y-%m-%dT%H:%M:%S+0000", tm_info);
    return timestamp;

}
