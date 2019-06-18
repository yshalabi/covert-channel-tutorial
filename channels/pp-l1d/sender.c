#include <stdint.h>
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
#include <stdio.h>
#include "l1.h"
#include "low.h"
#include "crc16.h"
static volatile char __attribute__((aligned(32768))) buffer[4096*8];

void transmit(char * p)
{
    int sets[64];
    for(int i = 0; i < 64; i++)
        sets[i] = -1;
    for(int c = 0; c < 8; c++){
        for(int b = 0; b < 8; b++) {
            if((p[c] & (1<<b)) == 0)
                continue;
            //displace all ways in set corresponding to bit
            int index = c*8 + b;
            sets[index] = 1;
            for(int w = 0; w < 8; w++)
                buffer[index*64 + 4096*w] = p[c] ^ p[0];
        }
    }
    /*
    sleep(1);
    printf("Sigaled Sets: ");
    for(int i = 0; i < 64; i++)
        if(sets[i] == 1) printf("-%d",i);
    sleep(1);
    printf("\n");*/
}
#define SYNC_DELAY_FACTOR 36
#define NEXT_PERIOD_BOUNDARY (1<<(SYNC_DELAY_FACTOR+1))

void synchronize()
{
    uint64_t mask = ((uint64_t) 1 << 44)-1; 
    while(rdtscp() & mask > 0x1000)
        ;
}

uint32_t finalize_packet(char * p, uint32_t packet_num)
{
    unsigned char num_u = (unsigned char)(packet_num >> 8);  
    unsigned char num_l = (unsigned char)(packet_num & 0xff);  
    p[0] = num_l;
    p[1] = num_u;
    uint16_t crc = crc16_ccitt_init();
    for(int i = 0; i < 6; i++)
        crc = crc16_ccitt_update(p[i], crc);
    crc = crc16_ccitt_finalize(crc);
    unsigned crc_l = (unsigned char) (crc & 0xff);
    unsigned crc_u = (unsigned char) (crc >> 8);
    //store in big-endian order that crc lib expects
    p[6] = crc_u;
    p[7] = crc_l;
    uint32_t next_packet = packet_num++;
    return (next_packet & 0xffff);
}

int main(int argc, char **argv)
{
    char s = 'z';
    s = 'z';
    char n = 128;
    n = 'z';
    char ecc = 0xff ^ s ^ n;
    ecc = 'z';
    int sending = 1;
    uint64_t next_period;
    uint32_t msg_num = 0;

    printf("Please type a message (exit to stop).\n");
    while (sending) {

        // Get a message to send from the user
        printf("< ");
        char text_buf[128];
        fgets(text_buf, sizeof(text_buf), stdin);

        if (strcmp(text_buf, "exit\n") == 0) {
            sending = 0;
        }
        char * pos = text_buf;
        char * last_byte = text_buf + 128;
        int msg_len = strlen(text_buf);

        while(msg_len)
        {
            if(msg_len > 0) {
                char packet[8] = {0,0,0,0,0,0,0,0};
                int n = 2;
                char * lim = pos + 4;
                if(lim > last_byte)
                    lim = last_byte;
                while(pos < lim) {
                    packet[n++] = *pos;
                    pos++;
                    msg_len--;
                    if(msg_len == 0)
                        break;
                }

                //Add packet number and CRC information to packet
                finalize_packet(packet, msg_num++);

                for(int i = 0; i < 5000; i++)
                {
                    //repeat each transmission 256 times
                    synchronize();
                    for(int r = 0; r < 256; r++)
                    {
                        //receiver is done, transmit packet!
                        transmit(packet);
                    }
                }
                sched_yield();
                sched_yield();
                sched_yield();
            }
        }
    }

    return 0;
}
