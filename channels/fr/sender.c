#include "util.h"

/*
 * Parses the arguments and flags of the program and initializes the struct state
 * with those parameters (or the default ones if no custom flags are given).
 */
int init_state(struct state *state, int argc, char **argv)
{

	if (argc<2) {
		return -1;
	}

	// This number may need to be tuned up to the specific machine in use
	// NOTE: Make sure that interval is the same in both sender and receiver
	state->interval = (uint32_t) 1<<15;

	int size = 4096;
	int offset = 0;
	if (argc>2) {
		offset = atoi(argv[2])*64; // inc. in multiples of 64 byte cache line
	}

	int inFile = open(argv[1], O_RDONLY);
	if(inFile == -1) {
		printf("Failed to Open File\n");
		return -1;
	}

	void *mapaddr = mmap(NULL,size,PROT_READ,MAP_SHARED,inFile,0);
	printf("File mapped at %p\n",mapaddr);

	if (mapaddr == (void*) -1 ) {
		printf("Failed to Map Address\n");
		return -1;
	}

	state->addr = (ADDR_PTR) mapaddr + offset;
	printf("Address Flushing = %lx\n",state->addr);

	return 0;
}

/*
 * Sends a bit to the receiver by repeatedly flushing the addresses of the eviction_set
 * for the clock length of state->interval when we are sending a one, or by doing nothing
 * for the clock length of state->interval when we are sending a zero.
 */
void send_bit(bool one, struct state *state)
{
	uint32_t mask = ((uint32_t) 1<<20) - 1;
	uint32_t threshold = (uint32_t) 1<<8;
	while((rdtscp() & mask) > threshold);

	CYCLES start_t = rdtscp();

	if (one) {
		ADDR_PTR addr = state->addr;
		while ((rdtscp() - start_t) < state->interval) {
			clflush(addr);
		}	

	} else {
		start_t = rdtscp();
		while (rdtscp() - start_t < state->interval) {}
	}
}

int main(int argc, char **argv)
{
    // Initialize state and local variables
    struct state state;
    init_state(&state, argc, argv);
    int sending = 1;

    printf("Please type a message (exit to stop).\n");
    while (sending) {

        // Get a message to send from the user
        printf("< ");
        char text_buf[128];
        fgets(text_buf, sizeof(text_buf), stdin);

        if (strcmp(text_buf, "exit\n") == 0) {
            sending = 0;
        }

        // Convert that message to binary
        char *msg = string_to_binary(text_buf);


        // Send a '10101011' byte to let the receiver detect that
        // I am about to send a start string and sync
        for (int i = 0; i < 6; i++) {
            send_bit(i % 2 == 0, &state);
        }
        send_bit(true, &state);
        send_bit(true, &state);

        // Send the message bit by bit
        size_t msg_len = strlen(msg);
        for (int ind = 0; ind < msg_len; ind++) {
            if (msg[ind] == '0') {
                send_bit(false, &state);
            } else {
                send_bit(true, &state);
            }
        }

    }

    printf("Sender finished\n");
    return 0;
}











