#include "util.h"

/*
 * Parses the arguments and flags of the program and initializes the struct config
 * with those parameters (or the default ones if no custom flags are given).
 */
void init_config(struct config *config, int argc, char **argv) {
    uint64_t pid = print_pid();

    init_default(config, argc, argv);

    if (config->channel == PrimeProbe) {
        int L3_way_stride = ipow(2, LOG_CACHE_SETS_L3 + LOG_CACHE_LINESIZE);
        uint64_t bsize = 32 * CACHE_WAYS_L3 * L3_way_stride;

        // Allocate a buffer of the size of the LLC
        // config->buffer = malloc((size_t) bsize);
        config->buffer = allocate_buffer(bsize);
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
                if (get_cache_slice_set_index(addr) == config->cache_region) {
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
    uint64_t start_t = get_time();
    debug("time %lx\n", start_t);

    if (one) {
        // wait for receiver to prime the cache set
        while (get_time() - start_t < config->prime_period) {}

        // access
        uint64_t access_count = 0;
        struct Node *current = NULL;
        uint64_t stopTime = start_t + config->prime_period + config->access_period;
        // uint64_t stopTime = start_t + config->interval;
        do {
            current = config->addr_set;
            while (current != NULL && current->next != NULL && get_time() < stopTime) {
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
        } while (get_time() < stopTime);
        debug("access count %lu time %lx\n", access_count, get_time() - start_t);

        // wait for receiver to probe
        while (get_time() - start_t < config->interval) {}

    } else {
        while (get_time() - start_t < config->interval) {}
    }
}

int main(int argc, char **argv)
{
    // Initialize config and local variables
    struct config config;
    init_config(&config, argc, argv);
    if (config.channel == PrimeProbe) {
        send_bit = send_bit_pp;
    }
    else {
        fprintf(stderr, "This branch only supports LLC-PrimeProbe\n");
        exit(-1);
    }

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
#if 1
        // Get a message to send from the user
        printf("< ");
        char text_buf[128];
        fgets(text_buf, sizeof(text_buf), stdin);

        if (strcmp(text_buf, "exit\n") == 0) {
            sending = 0;
        }

        char *msg = string_to_binary(text_buf);
#else
        char *msg = all_one_msg;
#endif

        // If we are in benchmark mode, start measuring the time
        if (config.benchmark_mode) {
            start_t = get_time();
        }

        // Send a '10101011' byte to let the receiver detect that
        // I am about to send a start string and cc_sync
        size_t msg_len = strlen(msg);

        // cc_sync on clock edge
        uint64_t start_t =  cc_sync();

        for (int i = 0; i < 10; i++) {
            start_t = cc_sync();
            // send_bit(i % 2 == 0, &config, start_t);
            send_bit(i % 2 == 0, &config);
        }
        start_t = cc_sync();
        // send_bit(true, &config, start_t);
        send_bit(true, &config);
        start_t = cc_sync();
        // send_bit(true, &config, start_t);
        send_bit(true, &config);

        // Send the message bit by bit
        start_t = cc_sync();
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
