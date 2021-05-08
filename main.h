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

char* get_time();
header_t* get_header();

#endif

