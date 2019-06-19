
#include "util.h"

/* Measure the time it takes to access a block with virtual address addr. */
extern inline __attribute__((always_inline))
uint64_t measure_one_block_access_time(ADDR_PTR addr) {
    uint64_t cycles;

    asm volatile("mov %1, %%r8\n\t"
            "lfence\n\t"
            "rdtsc\n\t"
            "lfence\n\t"
            "mov %%eax, %%edi\n\t"
            "mov (%%r8), %%r8\n\t"
            "lfence\n\t"
            "rdtsc\n\t"
            "lfence\n\t"
            "sub %%edi, %%eax\n\t"
    : "=a"(cycles) /*output*/
    : "r"(addr)
    : "r8", "edi");

    return cycles;
}

/*
 * CLFlushes the given address.
 */
extern inline __attribute__((always_inline))
void clflush(ADDR_PTR addr) {
    asm volatile ("clflush (%0)"::"r"(addr));
}

extern inline __attribute__((always_inline))
uint64_t rdtsc() {
    uint64_t a, d;
    asm volatile ("lfence");
    asm volatile ("rdtsc" : "=a" (a), "=d" (d));
    asm volatile ("lfence");
    return (d << 32) | a;
}

extern inline __attribute__((always_inline))
CYCLES rdtscp(void) {
	CYCLES cycles;
	asm volatile ("rdtscp"
	: /* outputs */ "=a" (cycles));

	return cycles;
}

inline uint64_t get_time() {
    // can be a choice of channel?
    return rdtsc();
}

extern inline __attribute__((always_inline))
uint64_t cc_sync() {
    while((get_time() & CHANNEL_SYNC_TIMEMASK) > CHANNEL_SYNC_JITTER) {}
    return get_time();
}

/*
 * Computes base to the exp.
 */
int ipow(int base, int exp)
{
    int result = 1;
    while (exp) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }

    return result;
}

/*
 * Returns the 11 bits used index a LLC set in a slice of a given address.
 */
uint64_t get_cache_slice_set_index(ADDR_PTR virt_addr) {
    // return (virt_addr >> LOG_CACHE_LINESIZE) & CACHE_SETS_L1_MASK;
    return (virt_addr >> LOG_CACHE_LINESIZE) & (2048-1);
}

uint64_t get_L3_cache_set_index(ADDR_PTR virt_addr) {
    return (virt_addr >> LOG_CACHE_LINESIZE) & CACHE_SETS_L3_MASK;
    // return (virt_addr >> LOG_CACHE_LINESIZE) & (2048-1);
}

/*
 * Returns the 15 physical bits of a given virtual address in a hugepage.
 */
// uint64_t get_hugepage_cache_set_index(ADDR_PTR virt_addr)
// {
//     return (virt_addr & HUGEPAGE_MASK) >> LOG_CACHE_LINESIZE;
// }


/*
 * Allocate a buffer of the size as passed-in
 * returns the pointer to the buffer
 */
void *allocate_buffer(uint64_t size) {
    void *buffer = MAP_FAILED;
#ifdef HUGEPAGES
    buffer = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE|HUGEPAGES, -1, 0);
#endif

    if (buffer == MAP_FAILED) {
        fprintf(stderr, "allocating non-hugepages\n");
        buffer = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE, -1, 0);
    }
    if (buffer == MAP_FAILED) {
        fprintf(stderr, "Failed to allocate buffer!\n");
        exit(-1);
    }

    return buffer;
}

/*
 * Convert a given ASCII string to a binary string.
 * From:
 * https://stackoverflow.com/questions/41384262/convert-string-to-binary-in-c
 */
char *string_to_binary(char *s)
{
    if (s == NULL) return 0; /* no input string */

    size_t len = strlen(s) - 1;

    // Each char is one byte (8 bits) and + 1 at the end for null terminator
    char *binary = malloc(len * 8 + 1);
    binary[0] = '\0';

    for (size_t i = 0; i < len; ++i) {
        char ch = s[i];
        for (int j = 7; j >= 0; --j) {
            if (ch & (1 << j)) {
                strcat(binary, "1");
            } else {
                strcat(binary, "0");
            }
        }
    }

    return binary;
}

/*
 * Convert 8 bit data stream into character and return
 */
char *conv_msg(char *data, int size, char *msg) {
    for (int i = 0; i < size; i++) {
        char _tmp = 0;

        for (int j = i * 8; j < ((i + 1) * 8); j++) {
            _tmp = (_tmp << 1) + data[j] - '0';
        }

        msg[i] = _tmp;
    }

    msg[size] = '\0';

    return msg;
}

/*
 * Appends the given string to the linked list which is pointed to by the given head
 */
void append_string_to_linked_list(struct Node **head, ADDR_PTR addr)
{
    struct Node *current = *head;

    // Create the new node to append to the linked list
    struct Node *new_node = malloc(sizeof(*new_node));
    new_node->addr = addr;
    new_node->next = NULL;

    // If the linked list is empty, just make the head to be this new node
    if (current == NULL)
        *head = new_node;

        // Otherwise, go till the last node and append the new node after it
    else {
        while (current->next != NULL)
            current = current->next;

        current->next = new_node;
    }
}

uint64_t print_pid() {
    uint64_t pid = getpid();
    printf("Process ID: %lu\n", pid);
    return pid;
}

void print_help() {
    // TODO
}

void init_default(struct config *config, int argc, char **argv) {

    config->buffer = NULL;
    config->addr_set = NULL;

    // Cache region specifies the targeted set
    config->cache_region = CHANNEL_DEFAULT_REGION;
    // Interval specifies the time used to send a single bit
    config->interval = CHANNEL_DEFAULT_INTERVAL;

    // Prime+Probe specific paramters:
    config->access_period = CHANNEL_DEFAULT_PERIOD;
    config->prime_period = CHANNEL_DEFAULT_PERIOD;


    config->benchmark_mode = false;

    config->channel = PrimeProbe;

    // Parse the command line flags
    //      -d is used to enable the debug prints
    //      -i is used to specify a custom value for the time interval
    //      -w is used to specify a custom number of wait time between two probes
    int option;
    while ((option = getopt(argc, argv, "di:a:r:c:")) != -1) {
        switch (option) {
            case 'i':
                config->interval = atoi(optarg);
                break;
            case 'r':
                config->cache_region = atoi(optarg);
                break;
            case 'a':
                config->access_period = atoi(optarg);
                break;
            case 'c':
                // value 0,1,2 to select channel
                config->channel = atoi(optarg);
                break;
            case '?':
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                print_help();
                exit(1);
            default:
                print_help();
                exit(1);
        }
    }

    if (config->channel == PrimeProbe) {
        config->miss_threshold = CHANNEL_L3_MISS_THRESHOLD;
        if (config->interval < config->prime_period + config->access_period) {
            fprintf(stderr, "ERROR: P+P channel bit interval too short!\n");
            exit(-1);
        }
        else {
            config->probe_period = config->interval - config->prime_period - config->access_period;
        }
    }

    debug("prime %u access %u probe %u\n", config->prime_period, config->access_period, config->probe_period);

}
