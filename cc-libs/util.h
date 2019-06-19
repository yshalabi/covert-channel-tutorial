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

#ifdef DEBUG
#define debug(...) do{			\
	fprintf(stdout, ##__VA_ARGS__); \
} while(0)
#else
#define debug(...)
#endif

#ifdef MAP_HUGETLB
#define HUGEPAGES MAP_HUGETLB
#else
#define HUGEPAGES 0
#endif

#define ADDR_PTR uint64_t
#define CYCLES uint32_t

struct state {
	ADDR_PTR addr;
	int interval;
	int wait_cycles_between_measurements;
};

typedef enum _channel {
    PrimeProbe = 0,
    FlushReload,
    L1DPrimeProbe
} Channel;

struct Node {
    ADDR_PTR addr;
    struct Node *next;
};

/*
 * Execution config of the program, with the variables
 * that we need to pass around the various functions.
 */
struct config {
    char *buffer;
    struct Node *addr_set;
    uint64_t cache_region;
    uint64_t interval;
    uint64_t prime_period;
    uint64_t access_period;
    uint64_t probe_period;
    uint64_t miss_threshold;
    char *shared_filename;
    bool benchmark_mode;        // sender only
    Channel channel;
};

uint64_t measure_one_block_access_time(ADDR_PTR addr);
void clflush(ADDR_PTR addr);
uint64_t rdtsc();
CYCLES rdtscp(void);

uint64_t get_time();
uint64_t cc_sync();

uint64_t print_pid();
void print_help();

int ipow(int base, int exp);

char *string_to_binary(char *s);

char *conv_msg(char *data, int size, char *msg);


uint64_t get_cache_slice_set_index(ADDR_PTR virt_addr);
uint64_t get_L3_cache_set_index(ADDR_PTR virt_addr);
// uint64_t get_hugepage_cache_set_index(ADDR_PTR virt_addr);
void *allocate_buffer(uint64_t size);

void append_string_to_linked_list(struct Node **head, ADDR_PTR addr);

void init_default(struct config *config, int argc, char **argv);


// =======================================
// Machine Configuration
// =======================================

// Hugepage
#define HUGEPAGE_BITS 21
#define HUGEPAGE_SIZE (1 << HUGEPAGE_BITS)
#define HUGEPAGE_MASK (HUGEPAGE_SIZE - 1)

// Cache
#define CACHE_LINESIZE      64
#define LOG_CACHE_LINESIZE  6

// L1
#define LOG_CACHE_SETS_L1   6
#define CACHE_SETS_L1       64
#define CACHE_SETS_L1_MASK  (CACHE_SETS_L1 - 1)
#define CACHE_WAYS_L1       8
#define CACHE_WAYS_L2       8

// LLC

#define LOG_CACHE_SETS_L3   15
#define CACHE_SETS_L3       32768
#define CACHE_SETS_L3_MASK  (CACHE_SETS_L3 - 1)
#define CACHE_WAYS_L3       20

// =======================================
// Covert Channel Default Configuration
// =======================================

// Access period of 0x00050000 seems too be sufficient for i3-metal
#define CHANNEL_DEFAULT_INTERVAL        0x000f0000
#define CHANNEL_DEFAULT_PERIOD          0x00050000
#define CHANNEL_DEFAULT_REGION          0x0
#define CHANNEL_SYNC_TIMEMASK           0x003fffff
#define CHANNEL_SYNC_JITTER             0x4000
#define CHANNEL_L3_MISS_THRESHOLD       220
#define CHANNEL_L2_MISS_THRESHOLD       150 	// not used
#define CHANNEL_L1_MISS_THRESHOLD       84
#define MAX_BUFFER_LEN                  1024

// TODO: following parameters need to be verified
#define CHANNEL_FR_DEFAULT_INTERVAL     0x00008000 // (1<<15)
#define CHANNEL_FR_DEFAULT_PERIOD       0x00000800 // (1<<11)

#endif
