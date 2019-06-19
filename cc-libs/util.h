
// You may only use fgets() to pull input from stdin
// You may use any print function to stdout to print 
// out chat messages
//
// You may use memory allocators and helper functions
// (e.g., rand()).  You may not use system().
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


#ifndef UTIL_H_
#define UTIL_H_

#define ADDR_PTR uint64_t
#define CYCLES uint32_t

struct state {
	ADDR_PTR addr;
	int interval;
	int wait_cycles_between_measurements;
};

struct Node {
    ADDR_PTR addr;
    struct Node *next;
};

CYCLES measure_one_block_access_time(ADDR_PTR addr);
CYCLES rdtscp(void);

void clflush(ADDR_PTR addr);

int ipow(int base, int exp);

char *string_to_binary(char *s);

char *conv_char(char *data, int size, char *msg);

uint64_t get_cache_set_index(ADDR_PTR phys_addr);

void append_string_to_linked_list(struct Node **head, ADDR_PTR addr);

// L1 properties
static const int CACHE_SETS_L1 = 64;
static const int CACHE_WAYS_L1 = 8;

// L3 properties
static const int CACHE_SETS_L3 = 8192;
static const int CACHE_WAYS_L3 = 16;
static const int CACHE_SLICES_L3 = 8;

#endif
