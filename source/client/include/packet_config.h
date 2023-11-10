#ifndef CLIENT_PACKET_CONFIG_H
#define CLIENT_PACKET_CONFIG_H

#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <printf.h>
#include <string.h>
#include <sys/socket.h>

uint8_t             first_empty_packet;
uint8_t             first_unacked_packet;
uint8_t             is_window_available;
uint8_t             window_size;

enum flags
{
    SYN = 1,
    ACK = 2,
    PSH = 4,
    FIN = 8,
    URG = 16,
    RST = 32,
    SYNACK = SYN + ACK,
    PSHACK = PSH + ACK,
    FINACK = FIN + ACK,
    RSTACK = RST + ACK

};

enum next_state_for_packet
{
    SENDACK,
    ESTABLISH_HANDSHAKE,
    RECVACK,
    END_CONNECTION,
    RECVRST,
    UNKNOWN_FLAG
};

typedef struct header
{
    uint32_t        seq_number;
    uint32_t        ack_number;
    uint8_t         flags;
    uint8_t         window_size;
    struct timeval  tv;
    struct sockaddr_storage addr;
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
    uint8_t         is_packet_empty;
} sent_packet;

int                 create_window(struct sent_packet **window, uint8_t window_size);
int                 window_empty(struct sent_packet *window);
int                 first_packet_ring_buffer(struct sent_packet *window, uint8_t window_size);
int                 first_unacked_ring_buffer(struct sent_packet *window, uint8_t window_size);
int                 receive_packet(int sockfd, struct sockaddr_storage addr, struct sent_packet *window, struct packet *pt);
int                 read_flags(uint8_t flags);
int                 send_syn_packet(int sockfd, struct sockaddr_storage addr, struct sent_packet *window);
int                 send_syn_ack_packet(int sockfd, struct sockaddr_storage addr, struct sent_packet *window, struct packet *pt);
int                 send_ack_packet(int sockfd, struct sockaddr_storage addr, struct sent_packet *window, struct packet *pt);
int                 recv_ack_packet(int sockfd, struct sockaddr_storage addr, struct sent_packet *window, struct packet *pt);
int                 recv_termination_request(int sockfd, struct sockaddr_storage addr, struct sent_packet *window, struct packet *pt);
int                 send_packet(int sockfd, struct sockaddr_storage addr, struct sent_packet *window, struct packet *pt);
int                 create_flags(uint8_t flags);
uint32_t            create_second_handshake_seq_number(void);
uint32_t            create_ack_number(uint32_t recv_seq_number, uint32_t data_size);
uint32_t            create_sequence_number(uint32_t prev_seq_number, uint32_t data_size);

#endif //CLIENT_PACKET_CONFIG_H
