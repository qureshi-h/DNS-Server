#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
#include <arpa/inet.h>

#define HEADER_COUNT 7
#define TIMESTAMP_LEN 24

char* get_time();

int main(int argc, char* argv[]) {
	
	FILE* log_file = fopen("./dns_svr.log", "w");

	uint16_t header[HEADER_COUNT];

	fread(header, sizeof(*header), HEADER_COUNT, stdin);

	for (int i = 0; i < HEADER_COUNT; i++) {
		printf("%u ", ntohs(header[i]));
	}
	

	fprintf(log_file, "%s ", get_time());	
	uint8_t label_size;
	char* label;

	fread(&label_size, sizeof(label_size), 1, stdin);
	while (label_size) {
	
		label = (char*)malloc(sizeof(*label) * (label_size + 1));
		fread(label, sizeof(*label), label_size, stdin);
		label[label_size] ='\0';
		printf("%s.", label);
		fread(&label_size, sizeof(label_size), 1, stdin);
	}
	return 0;
}

char* get_time() {

	time_t timer = time(NULL);
	struct tm* tm_info = localtime(&timer);
	char* timestamp = (char*)malloc(sizeof(*timestamp) * (TIMESTAMP_LEN + 1));

	strftime(timestamp, TIMESTAMP_LEN + 1, "%Y-%m-%dT%H:%M:%S+0000", tm_info);
	return timestamp;

}

