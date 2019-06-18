#include <sched.h>
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
#include "l1.h"
#include "low.h"
#include "crc16.h"
/*
 * Detects a bit by repeatedly measuring the access time of the addresses in the
 * probing set and counting the number of misses for the clock length of state->interval.
 *
 * If the the first_bit argument is true, relax the strict definition of "one" and try to
 * sync with the sender.
 */
typedef struct __attribute__((__packed__)) {
    unsigned char num;
    char msg[7];
    unsigned char ecc;
} cc_payload;
#define THRESHOLD_ONE 130
#define L1_HIT 74

bool is_valid_packet(char * p) 
{
    uint16_t crc = crc16_ccitt_init();
    for(int i =0; i < 8; i++)
        crc = crc16_ccitt_update(p[i], crc);
    crc = crc16_ccitt_finalize(crc);
    return (crc == 0);
}

#define SYNC_DELAY_FACTOR 28
#define NEXT_PERIOD_BOUNDARY (1<<(SYNC_DELAY_FACTOR+1))

void synchronize()
{
    uint64_t mask = (((uint64_t) 1) << 44)-1; 
    while(rdtscp() & mask > 0x1000)
        ;
}

int set_timings[64][256];
void measure_thresholds(uint16_t * results, int * rmap, char * dst)
{
    char payload[8] = {0,0,0,0,0,0,0,0};
    int set_threshold[64];
    for(int i = 0; i < 64; i++)
    {
        for(int j = 0; j < 256; j++) set_timings[i][j] = 0;
        for(int s = 0; s < 256; s++) {
            int cycles = results[s*64 + rmap[i]];
            if(cycles < 256 && cycles >0)
                set_timings[i][cycles]++;
        }
    }
    for(int i = 0; i < 64; i++) {
        int sum = 0;
        for(int j = 0; j < 256; j++) {
            sum+=set_timings[i][j];
            if(sum < 128)
                continue;
            set_threshold[i] = j;
            break;
        }
    }
    int set = 0;
    for(int c = 0; c < 8; c++){
        payload[c] = 0;
        for(int b = 0; b < 8; b++) {
            int i = c*8 + b;
            //printf("SET %d = %d\n", set++, set_threshold[i]);
            if(set_threshold[i] > 120)
                payload[c] |= (1<<b);
        }
    }
    dst[0] = payload[0];
    dst[1] = payload[1];
    dst[2] = payload[2];
    dst[3] = payload[3];
    dst[4] = payload[4];
    dst[5] = payload[5];
    dst[6] = payload[6];
    dst[7] = payload[7];
}

int main(int argc, char **argv)
{
    char msg[7];
    msg[6] = 0;
    uint64_t next_period;
    int i;
    l1pp_t l1 = l1_prepare();
    int nsets = l1_getmonitoredset(l1, NULL, 0);
    int *map = calloc(nsets, sizeof(int));
    l1_getmonitoredset(l1, map, nsets);

    int rmap[64];
    for(int i =0;i<64;i++) rmap[i] = -1;
    for(int i =0; i < nsets; i++) rmap[map[i]] = i;

    int samples = 256;
    uint16_t *results = calloc(samples * nsets, sizeof(uint16_t));
    for (int i = 0; i < samples * nsets; i+= 4096/sizeof(uint16_t))
        results[i] = 1;

    int next_msg = 0;
    uint16_t last_msg_num = 0xFFFF;
    char buf[128];
    char * pos = buf;
    printf("\nMsg: ");
    while(1) {

        char packet[8] = {0,0,0,0,0,0,0,0};
        synchronize();
        for(int i = 0; i < 128; i++)
        {
            uint16_t * res = results + nsets*i*2;
            l1_probe(l1, res);
            res = results + nsets*(2*i) + nsets;
            l1_bprobe(l1, res);
        }

        measure_thresholds(results,rmap, packet);
        
        if(is_valid_packet(packet) == false){
            sched_yield();
            continue;
        } 
        uint16_t msg_num = (uint16_t) packet[0];
        if(msg_num == last_msg_num)
            continue;
        last_msg_num = msg_num;
        for(int i = 2; i <6; i++ ) {
            if(packet[i] == 0)
                printf("\nMsg: ");
            printf("%c",packet[i]);
        }
    }
    return 0;
}
