#ifndef CLIENT_PACKET_CONFIG_H
#define CLIENT_PACKET_CONFIG_H

#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <printf.h>
#include <arpa/inet.h>

#define DATA_SIZE 512

typedef struct header
{
    uint32_t                    seq_number;
    uint32_t                    ack_number;
    uint8_t                     flags;
    uint8_t                     window_size;
    uint16_t                    checksum;
    struct timeval              tv;
} header;

typedef struct packet
{
    struct header   hd;
    char            data[DATA_SIZE];
} packet;

#endif //CLIENT_PACKET_CONFIG_H
