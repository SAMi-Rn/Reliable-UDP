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
int                 send_packet(int sockfd, struct sockaddr_storage *addr, struct packet *pt, struct fsm_error *err);
int                 receive_packet(int sockfd, struct packet *temp_packet, struct fsm_error *err);
uint32_t            create_second_handshake_seq_number(void);
uint32_t            create_ack_number(uint32_t previous_ack_number, uint32_t data_size);
uint32_t            create_sequence_number(uint32_t prev_seq_number, uint32_t data_size);
int                 add_connection(struct sockaddr_storage *addr);
int                 valid_connection(struct sockaddr_storage *addr);
int                 check_seq_number(uint32_t seq_number, uint32_t expected_seq_number);
uint32_t            update_expected_seq_number(uint32_t seq_number, uint32_t data_size);
int check_if_equal(uint32_t seq_number, uint32_t expected_seq_number);
int check_if_less(uint32_t seq_number, uint32_t expected_seq_number);


#endif //CLIENT_PACKET_CONFIG_H
