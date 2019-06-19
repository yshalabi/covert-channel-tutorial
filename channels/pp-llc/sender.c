#include "util.h"
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


uint64_t printPID() {
    uint64_t pid = getpid();
    printf("Process ID: %lu\n", pid);
    return pid;
}

void print_help() {
    // TODO
}

typedef enum _channel {
    PrimeProbe = 0,
    FlushReload,
    L1DPrimeProbe
} Channel;


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

    // Flush+Reload specific paramters:
    config->shared_filename = "shared.txt";

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

    if (config->channel == PrimeProbe || config->channel == L1DPrimeProbe) {
        config->miss_threshold = config->channel == PrimeProbe?
                                 CHANNEL_L3_MISS_THRESHOLD:
                                 CHANNEL_L1_MISS_THRESHOLD;
        if (config->interval < config->prime_period + config->access_period) {
            fprintf(stderr, "ERROR: P+P channel bit interval too short!\n");
            exit(-1);
        }
        else {
            config->probe_period = config->interval - config->prime_period - config->access_period;
        }
    }

    // debug("prime %u access %u probe %u\n", config->prime_period, config->access_period, config->probe_period);
    //
    if (config->channel == FlushReload) {
        config->access_period = CHANNEL_FR_DEFAULT_INTERVAL;
        config->access_period = CHANNEL_FR_DEFAULT_PERIOD;
        config->miss_threshold = CHANNEL_L1_MISS_THRESHOLD;
        if (config->cache_region > 63) {
            fprintf(stderr, "ERROR: F+R channel region should be within a 4K page (64lines)!\n");
            exit(-1);
        }
    }

}

/*
 * Parses the arguments and flags of the program and initializes the struct config
 * with those parameters (or the default ones if no custom flags are given).
 */
void init_config(struct config *config, int argc, char **argv)
{
    uint64_t pid = printPID();

    init_default(config, argc, argv);

    if (config->channel == PrimeProbe) {
        int L3_way_stride = ipow(2, LOG_CACHE_SETS_L3 + LOG_CACHE_LINESIZE);
        uint64_t bsize = 32 * CACHE_WAYS_L3 * L3_way_stride;

        // Allocate a buffer of the size of the LLC
        // config->buffer = malloc((size_t) bsize);
        printf("buffer pointer addr %p\n", config->buffer);

        // Initialize the buffer to be be the non-zero page
        for (uint32_t i = 0; i < bsize; i += 64) {
            *(config->buffer + i) = pid;
        }

        // Construct the addr_set by taking the addresses that have cache set index 0
        uint32_t addr_set_size = 0;
        for (int set_index = 0; set_index < CACHE_SETS_L3; set_index++) {
            for (uint32_t line_index = 0; line_index < 32 * CACHE_WAYS_L3; line_index++) {
                // a simple hash to shuffle the lines in physical address space
                uint32_t stride_idx = (line_index * 167 + 13) % (32 * CACHE_WAYS_L3);
                ADDR_PTR addr = (ADDR_PTR) (config->buffer + \
                        set_index * CACHE_LINESIZE + stride_idx * L3_way_stride);
                // both of following function should work...L3 is a more restrict set
                {
                        // if (get_L3_cache_set_index(addr) == config->cache_region) {
                    append_string_to_linked_list(&config->addr_set, addr);
                    addr_set_size++;
                }
            }
        }
        printf("Found addr_set size of %u\n", addr_set_size);
    }

}

// sender function pointer
void (*send_bit)(bool, const struct config*);

/*
 * Sends a bit to the receiver by repeatedly flushing the addresses of the addr_set
 * for the clock length of config->interval when we are sending a one, or by doing nothing
 * for the clock length of config->interval when we are sending a zero.
 */
void send_bit_pp(bool one, const struct config *config)
{
    uint64_t start_t = 0;//et_time();

    if (one) {
        // wait for receiver to prime the cache set

        // access
        uint64_t access_count = 0;
        struct Node *current = NULL;
        uint64_t stopTime = start_t + config->prime_period + config->access_period;
        // uint64_t stopTime = start_t + config->interval;
        do {
            current = config->addr_set;
            while (current != NULL && current->next != NULL) {
            // while (current != NULL && get_time() < stopTime) {
                volatile uint64_t* addr1 = (uint64_t*) current->addr;
                volatile uint64_t* addr2 = (uint64_t*) current->next->addr;
                *addr1;
                *addr2;
                *addr1;
                *addr2;
                *addr1;
                *addr2;
                current = current->next;
                access_count++;
            }
        } while (1);//get_time() < stopTime);

        // wait for receiver to probe

    } else {
        ;
    }
}

int main(int argc, char **argv)
{
    // Initialize config and local variables
    struct config config;
    init_config(&config, argc, argv);
    send_bit = send_bit_pp;

    uint64_t start_t, end_t;
    int sending = 1;
    printf("Please type a message (exit to stop).\n");
    char all_one_msg[129];
    for (uint32_t i = 0; i < 120; i++)
        all_one_msg[i] = '1';
    for (uint32_t i = 120; i < 128; i++)
        all_one_msg[i] = '1';
    all_one_msg[128] = '\0';
    while (sending) {

        // Get a message to send from the user
        printf("< ");
        char text_buf[128];
        fgets(text_buf, sizeof(text_buf), stdin);

        if (strcmp(text_buf, "exit\n") == 0) {
            sending = 0;
        }

        char *msg = string_to_binary(text_buf);


        // If we are in benchmark mode, start measuring the time

        // Send a '10101011' byte to let the receiver detect that
        // I am about to send a start string and cc_sync
        size_t msg_len = strlen(msg);

        // cc_sync on clock edge
        uint64_t start_t =  0;//;w

        for (int i = 0; i < 10; i++) {
            // send_bit(i % 2 == 0, &config, start_t);
            send_bit(i % 2 == 0, &config);
        }
        // send_bit(true, &config, start_t);
        send_bit(true, &config);
        // send_bit(true, &config, start_t);
        send_bit(true, &config);

        // Send the message bit by bit
        // TODO: for longer messages it is recommended to re-sync every X bits
        for (uint32_t ind = 0; ind < msg_len; ind++) {
            if (msg[ind] == '0') {
                // send_bit(false, &config, start_t);
                send_bit(false, &config);
            } else {
                // send_bit(true, &config, start_t);
                send_bit(true, &config);
            }
            start_t += config.interval;
        }

        printf("message %s sent\n", msg);

        // If we are in benchmark mode, finish measuring the
        // time and print the bit rate.
        // if (config.benchmark_mode) {
        //     end_t = get_time();
        //     printf("Bitrate: %.2f Bytes/second\n",
        //            ((double) strlen(text_buf)) / ((double) (end_t - start_t) / CLOCKS_PER_SEC));
        // }
    }

    printf("Sender finished\n");
    return 0;
}
