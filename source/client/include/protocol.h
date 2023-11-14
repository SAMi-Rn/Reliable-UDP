#ifndef CLIENT_PROTOCOL_H
#define CLIENT_PROTOCOL_H

#include "packet_config.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

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
    SEND_HANDSHAKE_ACK,
    SEND_ACK,
    ESTABLISH_HANDSHAKE,
    RECV_ACK,
    END_CONNECTION,
    RECV_RST,
    UNKNOWN_FLAG
};

int                 read_flags(uint8_t flags);
int                 read_received_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt);
int                 protocol_connect(int sockfd, struct sockaddr_storage *addr, in_port_t port, struct sent_packet *window);
int                 send_syn_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window);
int                 send_syn_ack_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt);
int                 finish_handshake_ack(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt);
int                 send_handshake_ack_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt);
int                 send_data_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, char *data);
int                 send_data_ack_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt);
int                 recv_ack_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt);
int                 recv_termination_request(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt);
int                 initiate_termination(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window);
int                 create_flags(uint8_t flags);

#endif //CLIENT_PROTOCOL_H
