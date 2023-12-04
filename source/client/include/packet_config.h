#ifndef CLIENT_PACKET_CONFIG_H
#define CLIENT_PACKET_CONFIG_H

#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <printf.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include "protocol.h"
#include "server_config.h"

#define DATA_SIZE 512

uint8_t                     first_empty_packet;
uint8_t                     first_unacked_packet;
uint8_t                     is_window_available;
uint8_t                     window_size;

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

typedef struct sent_packet
{
    struct packet   pt;
    uint32_t        expected_ack_number;
    uint8_t         is_packet_full;
} sent_packet;

int                 create_window(struct sent_packet **window, uint8_t window_size, struct fsm_error *err);
int                 window_empty(struct sent_packet *window);
int                 first_packet_ring_buffer(struct sent_packet *window);
int                 first_unacked_ring_buffer(struct sent_packet *window);
int                 send_packet(int sockfd, struct sockaddr_storage *addr,
                                struct sent_packet *window, struct packet *pt,
                                FILE *fp, struct fsm_error *err);
int                 add_packet_to_window(struct sent_packet *window, struct packet *pt);
int                 receive_packet(int sockfd, struct sent_packet *window,
                                    struct packet *pt, FILE *fp, struct fsm_error *err);
int                 remove_packet_from_window(struct sent_packet *window, struct packet *pt);
uint32_t            create_second_handshake_seq_number(void);
uint32_t            create_ack_number(uint32_t previous_ack_number, uint32_t data_size);
uint32_t            create_sequence_number(uint32_t prev_seq_number, uint32_t data_size);
uint32_t            previous_seq_number(struct sent_packet *window);
uint32_t            previous_ack_number(struct sent_packet *window);
uint32_t            previous_data_size(struct sent_packet *window);
int                 check_ack_number(uint32_t expected_ack_number, uint32_t ack_number);
int                 previous_index(struct sent_packet *window);


#endif //CLIENT_PACKET_CONFIG_H
