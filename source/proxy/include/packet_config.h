#ifndef CLIENT_PACKET_CONFIG_H
#define CLIENT_PACKET_CONFIG_H

#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <printf.h>
#include <arpa/inet.h>

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


#endif //CLIENT_PACKET_CONFIG_H
