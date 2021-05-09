#ifndef MAIN_H
#define MAIN_H

#include <inttypes.h>

typedef struct header {

	int size;
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

char* get_time();
header_t* get_header();
question_t* get_question();

#endif

