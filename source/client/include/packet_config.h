#ifndef CLIENT_PACKET_CONFIG_H
#define CLIENT_PACKET_CONFIG_H

#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <printf.h>
#include <string.h>

// In global.h or a similar header file
 int can_send_packet;

uint8_t first_empty_packet;
uint8_t front_of_window;

typedef struct header
{
    uint32_t        sequence_number;
    uint32_t        acknowledgment_number;
    uint8_t         flags;
    uint8_t         window_size;
    struct timeval  tv;
} header;

typedef struct packet
{
    struct header   hd;
    char            data[512];
} packet;

typedef struct sent_packet
{
    struct packet   pt;
    uint32_t        expected_ack_number;
    struct timeval  sent_tv;
    uint8_t         has_been_acked;
} sent_packet;

int create_window(struct sent_packet **window, uint8_t window_size);
int window_empty(struct sent_packet *window, uint8_t window_size);

#endif //CLIENT_PACKET_CONFIG_H
