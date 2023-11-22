#include <pthread.h>
#include "protocol.h"

int protocol_connect(int sockfd, struct sockaddr_storage *addr, in_port_t port, struct sent_packet *window)
{
    struct packet temp_packet;
    send_syn_packet(sockfd, addr, window);
    receive_packet(sockfd, window, &temp_packet);
    remove_packet_from_window(window, &temp_packet);
    read_received_packet(sockfd, addr, window, &temp_packet);
    
    return 0;
}

int read_received_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt)
{
    int result;

    result = read_flags(pt->hd.flags);

    remove_packet_from_window(window, pt);
    switch (result)
    {
        case ESTABLISH_HANDSHAKE:
        {
            send_syn_ack_packet(sockfd, addr, window, pt);
            break;
        }
        case SEND_HANDSHAKE_ACK:
        {
            send_handshake_ack_packet(sockfd, addr, window, pt);
            break;
        }
        case SEND_ACK:
        {
            send_data_ack_packet(sockfd, addr, window, pt);
            break;
        }
        case RECV_ACK:
        {
            recv_ack_packet(sockfd, addr, window, pt);
            break;
        }
        case END_CONNECTION:
        {
            recv_termination_request(sockfd, addr, window, pt);
            break;
        }
        case RECV_RST:
        case UNKNOWN_FLAG:
        default:
        {
            return -1;
        }
    }

    return 0;
}

int read_flags(uint8_t flags)
{
    if (flags == SYN)
    {
        return ESTABLISH_HANDSHAKE;
    }

    if (flags == SYNACK)
    {
        return SEND_HANDSHAKE_ACK;
    }

    if (flags == PSHACK)
    {
        return SEND_ACK;
    }

    if (flags == ACK)
    {
        return RECV_ACK;
    }

    if (flags == FINACK)
    {
        return END_CONNECTION;
    }

    if (flags == RSTACK)
    {
        return RECV_RST;
    }

    return UNKNOWN_FLAG;
}

int send_syn_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window)
{
    struct packet packet_to_send;

    packet_to_send.hd.seq_number            = create_sequence_number(0, 0);
    packet_to_send.hd.ack_number            = create_ack_number(0, 0);
    packet_to_send.hd.flags                 = SYN;
    packet_to_send.hd.window_size           = window_size;
    memset(packet_to_send.data, 0, sizeof(packet_to_send.data));

    send_packet(sockfd, addr, window, &packet_to_send);
    add_packet_to_window(window, &packet_to_send);

    return 0;
}

int send_syn_ack_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt)
{
    struct packet packet_to_send;

    packet_to_send.hd.seq_number        = create_second_handshake_seq_number();
    packet_to_send.hd.ack_number        = create_ack_number(pt->hd.seq_number, 1);
    packet_to_send.hd.flags             = create_flags(pt->hd.flags);
    packet_to_send.hd.window_size       = window_size;

    send_packet(sockfd, addr, window, &packet_to_send);
    add_packet_to_window(window, &packet_to_send);

    return 0;
}

int finish_handshake_ack(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt)
{
    struct packet packet_to_send;

    packet_to_send.hd.seq_number        = create_sequence_number(previous_seq_number(window), 1);
    packet_to_send.hd.ack_number        = create_ack_number(pt->hd.seq_number, 1);
    packet_to_send.hd.flags             = create_flags(pt->hd.flags);
    packet_to_send.hd.window_size       = window_size;

    send_packet(sockfd, addr, window, &packet_to_send);
    add_packet_to_window(window, &packet_to_send);

    return 0;
}

int send_handshake_ack_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt)
{
    struct packet packet_to_send;

    packet_to_send.hd.seq_number        = create_sequence_number(pt->hd.ack_number, 0);
    packet_to_send.hd.ack_number        = create_ack_number(pt->hd.seq_number, 1);
    packet_to_send.hd.flags             = create_flags(pt->hd.flags);
    packet_to_send.hd.window_size       = window_size;
    memset(packet_to_send.data, 0, sizeof(packet_to_send.data));

    send_packet(sockfd, addr, window, &packet_to_send);
    add_packet_to_window(window, &packet_to_send);

    return 0;
}

int send_data_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, char *data)
{
    struct packet packet_to_send;

    packet_to_send.hd.seq_number        = create_sequence_number(previous_seq_number(window), previous_data_size(window));
    packet_to_send.hd.ack_number        = create_ack_number(previous_ack_number(window), 0);
    packet_to_send.hd.flags             = PSHACK;
    packet_to_send.hd.window_size       = window_size;
    strcpy(packet_to_send.data, data);

    send_packet(sockfd, addr, window, &packet_to_send);
    add_packet_to_window(window, &packet_to_send);

    return 0;
}

int send_data_ack_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt)
{
    struct packet packet_to_send;

    packet_to_send.hd.seq_number        = create_sequence_number(previous_seq_number(window), previous_data_size(window));
    packet_to_send.hd.ack_number        = create_ack_number(pt->hd.seq_number, strlen(pt->data));
    packet_to_send.hd.flags             = create_flags(pt->hd.flags);
    packet_to_send.hd.window_size       = window_size;

    send_packet(sockfd, addr, window, &packet_to_send);
    add_packet_to_window(window, &packet_to_send);

    return 0;
}

int recv_ack_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt)
{
    // check if incoming ack is same as expected ack
    return 0;
}

int recv_termination_request(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt)
{
    // send ack and then check if all packets have been acked
    // then send fin ack
    return 0;
}

int initiate_termination(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window)
{
    struct packet packet_to_send;

    packet_to_send.hd.seq_number            = create_sequence_number(previous_seq_number(window), previous_data_size(window));
    packet_to_send.hd.ack_number            = create_ack_number(previous_ack_number(window), previous_data_size(window));
    packet_to_send.hd.flags                 = FINACK;
    packet_to_send.hd.window_size           = window_size;

    send_packet(sockfd, addr, window, &packet_to_send);
    return 0;
}


int create_flags(uint8_t flags)
{
    if (flags == SYN)
    {
        return SYNACK;
    }

    if (flags == SYNACK)
    {
        return ACK;
    }

    if (flags == PSHACK)
    {
        return ACK;
    }

    if (flags == FINACK)
    {
        return ACK;
    }

    return UNKNOWN_FLAG;
}

int create_data_packet(struct packet *pt, struct sent_packet *window, char *data)
{
    struct packet packet_to_send;

    packet_to_send.hd.seq_number        = create_sequence_number(previous_seq_number(window), previous_data_size(window));
    packet_to_send.hd.ack_number        = create_ack_number(previous_ack_number(window), 0);
    packet_to_send.hd.flags             = PSHACK;
    packet_to_send.hd.window_size       = window_size;
    strcpy(packet_to_send.data, data);

    printf("seq number: %u\n", packet_to_send.hd.seq_number);
    printf("ack number: %u\n", packet_to_send.hd.ack_number);
    printf("window number: %u\n", packet_to_send.hd.window_size);
    printf("flags: %u\n", packet_to_send.hd.flags);
    printf("time: %ld\n", packet_to_send.hd.tv.tv_sec);
    printf("data: %s\n\n\n\n", packet_to_send.data);

    *pt = packet_to_send;
    add_packet_to_window(window, &packet_to_send);

    return 0;
}
