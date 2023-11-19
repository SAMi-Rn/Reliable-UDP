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

uint8_t                     first_empty_packet;
uint8_t                     first_unacked_packet;
uint8_t                     is_window_available;
uint8_t                     window_size;
struct sockaddr_storage     *list_of_connections;

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
    uint32_t        expected_ack_number;
    uint8_t         is_packet_full;
} sent_packet;

int                 create_window(struct sent_packet **window, uint8_t window_size);
//int                 window_empty(struct sent_packet *window);
//int                 first_packet_ring_buffer(struct sent_packet *window);
//int                 first_unacked_ring_buffer(struct sent_packet *window);
int                 send_packet(int sockfd, struct sockaddr_storage *addr, struct packet *pt);
//int                 add_packet_to_window(struct sent_packet *window, struct packet *pt);
int                 receive_packet(int sockfd, struct packet *temp_packet);
//int                 remove_packet_from_window(struct sent_packet *window, struct packet *pt);
uint32_t            create_second_handshake_seq_number(void);
uint32_t            create_ack_number(uint32_t previous_ack_number, uint32_t data_size);
uint32_t            create_sequence_number(uint32_t prev_seq_number, uint32_t data_size);
//uint32_t            previous_seq_number(struct sent_packet *window);
//uint32_t            previous_ack_number(struct sent_packet *window);
//uint32_t            previous_data_size(struct sent_packet *window);
socklen_t           size_of_address(struct sockaddr_storage *addr);
int                 add_connection(struct sockaddr_storage *addr);
int                 valid_connection(struct sockaddr_storage *addr);
int                 check_seq_number(uint32_t seq_number, uint32_t expected_seq_number);
uint32_t            update_expected_seq_number(uint32_t seq_number, uint32_t data_size);


#endif //CLIENT_PACKET_CONFIG_H
