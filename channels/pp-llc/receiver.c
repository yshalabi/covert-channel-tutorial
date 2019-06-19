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


typedef enum _channel {
    PrimeProbe = 0,
    FlushReload,
    L1DPrimeProbe
} Channel;


uint64_t printPID() {
    uint64_t pid = getpid();
    printf("Process ID: %lu\n", pid);
    return pid;
}

void print_help() {
    // TODO
}

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

    if (config->channel == PrimeProbe || config->channel == L1DPrimeProbe) {
        int L1_way_stride = ipow(2, LOG_CACHE_SETS_L1 + LOG_CACHE_LINESIZE); // 4096
        uint64_t bsize = 1024 * CACHE_WAYS_L1 * L1_way_stride; // 64 * 8 * 4k = 2M

        // Allocate a buffer twice the size of the L1 cache

        printf("buffer pointer addr %p\n", config->buffer);
        // Initialize the buffer to be be the non-zero page
        for (uint32_t i = 0; i < bsize; i += 64) {
            *(config->buffer + i) = pid;
        }
        // Construct the addr_set by taking the addresses that have cache set index 0
        // There will be at least one of such addresses in our buffer.
        uint32_t addr_set_size = 0;
        for (int i = 0; i < 1024 * CACHE_WAYS_L1 * CACHE_SETS_L1; i++) {
            ADDR_PTR addr = (ADDR_PTR) (config->buffer + CACHE_LINESIZE * i);
            // both of following function should work...L3 is a more restrict set
            {
            // if (get_L3_cache_set_index(addr) == config->cache_region) {
                append_string_to_linked_list(&config->addr_set, addr);
                addr_set_size++;
            }
            // restrict the probing set to CACHE_WAYS_L1 to aviod self eviction
            if (config->channel == L1DPrimeProbe && addr_set_size >= CACHE_WAYS_L1) {
		break;
	    }
	    else if (addr_set_size >= 2 * (CACHE_WAYS_L1 + CACHE_WAYS_L2)) {
		break;
	    }
        }

        printf("Found addr_set size of %u\n", addr_set_size);
    }

}

// receiver function pointer
bool (*detect_bit)(const struct config*, bool);
/*
 * Detects a bit by repeatedly measuring the access time of the addresses in the
 * probing set and counting the number of misses for the clock length of config->interval.
 *
 * If the the first_bit argument is true, relax the strict definition of "one" and try to
 * cc_sync with the sender.
 */
// bool detect_bit(const struct config *config, bool first_bit, uint64_t start_t)
bool detect_bit_pp(const struct config *config, bool first_bit)
{
    uint64_t start_t = 0;
    // debug("time %lx\n", start_t);

    int misses = 0;
    int hits = 0;
    int total_measurements = 0;

    // miss in L3
    struct Node *current = NULL;

    // prime
    uint64_t prime_count = 0;
    do {
        current = config->addr_set;
        while (current != NULL && current->next != NULL) {
            volatile uint64_t* addr1 = (uint64_t*) current->addr;
            volatile uint64_t* addr2 = (uint64_t*) current->next->addr;
            *addr1;
            *addr2;
            *addr1;
            *addr2;
            current = current->next;
            prime_count++;
        }
    } while (1);
    // debug("prime count%lu\n", prime_count);

    // wait for sender to access

    // probe
    current = config->addr_set;
    {
        ADDR_PTR addr = current->addr;
        uint64_t time = measure_one_block_access_time(addr);

        // When the access time is larger than 1000 cycles,
        // it is usually due to a long-latency page walk.
        // We exclude such misses
        // because they are not caused by accesses from the sender.
        total_measurements += time < 800;
        misses  += (time < 800) && (time > config->miss_threshold);
        hits    += (time < 800) && (time <= config->miss_threshold);

        current = current->next;
        // debug("access time %lu\n", time);
    }


    bool ret = (misses > CACHE_WAYS_L1 / 2 - 1)? true: false;
    // FIXME: If only one set region used in a L1D, the channel is really not
    // reliable as too much noise even from stack reads and writes.
    // Mulitple regions for each channel is recommended.
    // The hardcoded 1 miss count threshold can be used for a noisy l1d-PP
    // bool ret = (misses > 1)? true: false;


    return ret;
}

// This is the only hardcoded variable which defines the max size of a message
// to be the same as the max size of the message in the starter code of the sender.
// static const int MAX_BUFFER_LEN = 128 * 8;

int main(int argc, char **argv)
{
    // Initialize config and local variables
    struct config config;
    init_config(&config, argc, argv);

    detect_bit = detect_bit_pp;


    char msg_ch[MAX_BUFFER_LEN + 1];
    int flip_sequence = 4;
    bool first_time = true;
    bool current;
    bool previous = true;

    printf("Press enter to begin listening ");
    getchar();
    while (1) {

        // cc_sync on clock edge
        uint64_t start_t = 0;
        // current = detect_bit(&config, first_time, start_t);
        current = detect_bit(&config, first_time);

        // This receiving loop is a sort of finite config machine.
        // Once again, it would be easier to explain how it works
        // in a whiteboard, but here is an attempt to give an idea:
        //
        // Starting from the base config, it first looks for a sequence
        // of bits of the form "1010" (ref: flip_sequence variable).
        //
        // The first 1 is used to cc_synchronize, the following ones are
        // used to make sure that the cc_synchronization was right.
        //
        // Once these bits have been detected, if there are other bit
        // flips, the receiver ignores them.
        //
        // In fact, as of now the sender sends more than 4 bit flips.
        // This is because sometimes the receiver may miss the first 2.
        // Thus having more still works.
        //
        // After the 1010 bits, when two consecutive 11 bits are detected,
        // the receiver will know that what follows is a message and go
        // into message receiving mode.
        //
        // Finally, when a NULL byte is received the receiver exits the
        // message receiving mode and restarts from the base config.
        if (flip_sequence == 0 && current == 1 && previous == 1) {

            uint32_t msg_len = 0, strike_zeros = 0;
            for (msg_len = 0; msg_len < MAX_BUFFER_LEN; msg_len++) {

                // uint32_t bit = detect_bit(&config, first_time, start_t);
                uint32_t bit = detect_bit(&config, first_time);
                msg_ch[msg_len] = '0' + bit;
                strike_zeros = (strike_zeros + (1-bit)) & (bit-1);
                if (strike_zeros >= 8 && ((msg_len & 0x7) == 0)) {
                    break;
                }

                start_t += config.interval;
            }

            msg_ch[msg_len - 8] = '\0';
            printf("message %s received\n", msg_ch);

            uint32_t ascii_msg_len = msg_len / 8;
            char msg[ascii_msg_len];
            if (strcmp(msg, "exit") == 0) {
                break;
            }

        } else if (flip_sequence > 0 && current != previous) {
            flip_sequence--;
            first_time = false;

        } else if (current == previous) {
            flip_sequence = 4;
            first_time = true;
        }

        previous = current;
    }

    printf("Receiver finished\n");
    return 0;
}
