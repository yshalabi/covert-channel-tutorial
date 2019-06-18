#include "util.h"

int global_offset = 0;

/*
 * Parses the arguments and flags of the program and initializes the struct state
 * with those parameters (or the default ones if no custom flags are given).
 */
int init_state(struct state *state, int argc, char **argv)
{
	if (argc<2) {
		return -1;
	}   

	// These numbers may need to be tuned up to the specific machine in use
	// NOTE: Make sure that interval is the same in both sender and receiver
	state->interval = (uint32_t) 1<<15;
	state->wait_cycles_between_measurements = 0;

	int offset = 0;
	if (argc>2) {
		offset = 64*atoi(argv[2]);
	}

	int inFile = open(argv[1],O_RDONLY);
	if (inFile == -1) {
		printf("Failed to Open File\n");
		return -1;
	}	

	int size = 4096;
	void *mapaddr = mmap(NULL, size, PROT_READ, MAP_SHARED, inFile, 0);
	printf("File mapped at %p\n", mapaddr);

	if (mapaddr == (void*) -1 ) {
		printf("Failed to Map Address\n");
		return -1;
	}
		
	state->addr = (ADDR_PTR) mapaddr + offset; 	
	printf("Address Flushing = %lx\n",state->addr);

	return 0;
}

/*
 * Detects a bit by repeatedly measuring the access time of the addresses in the
 * probing set and counting the number of misses for the clock length of state->interval.
 *
 * If the the first_bit argument is true, relax the strict definition of "one" and try to
 * sync with the sender.
 */
bool detect_bit(struct state *state, bool first_bit)
{
	uint32_t mask = ((uint32_t) 1<<20) - 1;
	uint32_t threshold = (uint32_t) 1<<8;
	while((rdtscp() & mask) > threshold);

	int misses = 0;
	int hits = 0;
	int total_measurements = 0;

	// This is high because the misses caused by clflush
	// usually cause an access time larger than 150 cycles
	int misses_time_threshold = 70;

	CYCLES start_t = rdtscp();
	while ((rdtscp() - start_t) < state->interval) {
		CYCLES time = measure_one_block_access_time(state->addr); 

		// When the access time is larger than 1000 cycles,
		// it is usually due to a disk miss. We exclude such misses
		// because they are not caused by clflush.
		if (time < 1000) {
			total_measurements++;
			if (time > misses_time_threshold) {
				misses++;
			} else {
				hits++;
			}
		}

		// Busy loop to give time to the sender to flush the cache
		CYCLES wait_t = rdtscp();
		while((rdtscp() - wait_t) < state->wait_cycles_between_measurements &&
			       (rdtscp() - start_t) < state->interval);
	}

	bool ret =  misses > (float) total_measurements / 2.0;
	return ret;
}

// This is the only hardcoded variable which defines the max size of a message
// to be the same as the max size of the message in the starter code of the sender.
static const int max_buffer_len = 128 * 8;

int main(int argc, char **argv)
{
    // Initialize state and local variables
    struct state state;
    init_state(&state, argc, argv);
    char msg_ch[max_buffer_len + 1];
    int flip_sequence = 4;
    bool first_time = true;
    bool current;
    bool previous = true;

    printf("Press enter to begin listening ");
    getchar();
    while (1) {
        current = detect_bit(&state, first_time);

        // This receiving loop is a sort of finite state machine.
        // Once again, it would be easier to explain how it works
        // in a whiteboard, but here is an attempt to give an idea:
        //
        // Starting from the base state, it first looks for a sequence
        // of bits of the form "1010" (ref: flip_sequence variable).
        //
        // The first 1 is used to synchronize, the following ones are
        // used to make sure that the synchronization was right.
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
        // message receiving mode and restarts from the base state.
        if (flip_sequence == 0 && current == 1 && previous == 1) {
            int binary_msg_len = 0;
            int strike_zeros = 0;
            for (int i = 0; i < max_buffer_len; i++) {
                binary_msg_len++;

                if (detect_bit(&state, first_time)) {
                    msg_ch[i] = '1';
                    strike_zeros = 0;

                } else {
                    msg_ch[i] = '0';
                    if (++strike_zeros >= 8 && i % 8 == 0) {
                        break;
                    }
                }
            }

            msg_ch[binary_msg_len - 8] = '\0';

            int ascii_msg_len = binary_msg_len / 8;
            char msg[ascii_msg_len];
            printf("> %s\n", conv_char(msg_ch, ascii_msg_len, msg));
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


