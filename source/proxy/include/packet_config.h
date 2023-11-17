#ifndef CLIENT_PACKET_CONFIG_H
#define CLIENT_PACKET_CONFIG_H

#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <printf.h>
#include <arpa/inet.h>

uint8_t                     window_size;

typedef struct header
{
    uint32_t                    seq_number;
    uint32_t                    ack_number;
    uint8_t                     flags;
    uint8_t                     window_size;
    struct timeval              tv;
    struct sockaddr_storage     src_ip;
    struct sockaddr_storage     dst_ip;
} header;

typedef struct packet
{
    struct header   hd;
    char            data[512];
} packet;

typedef struct sent_packet
{
    struct packet   pt;
    uint8_t         is_packet_full;
} sent_packet;

int                 create_window(struct sent_packet **window, uint8_t cmd_line_window_size, uint8_t *first_empty_packet);
int                 first_packet_ring_buffer(struct sent_packet *window, uint8_t *first_empty_packet);
int                 remove_packet_from_window(struct sent_packet *window, struct packet *pt);

#endif //CLIENT_PACKET_CONFIG_H
