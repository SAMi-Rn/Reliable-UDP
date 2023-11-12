#include "protocol.h"

int protocol_connect(int sockfd, struct sockaddr_storage *addr, in_port_t port, struct sent_packet *window)
{
    char      addr_str[INET6_ADDRSTRLEN];
    in_port_t net_port;

    if(inet_ntop(addr->ss_family, addr->ss_family == AF_INET ? (void *)&(((struct sockaddr_in *)addr)->sin_addr) : (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr), addr_str, sizeof(addr_str)) == NULL)
    {
//        SET_ERROR(err, strerror(errno));
        return -1;
    }

    printf("Connecting to: %s:%u\n", addr_str, port);
    net_port = htons(port);

    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;
        ipv4_addr = (struct sockaddr_in *)addr;
        ipv4_addr->sin_port = net_port;
        send_syn_packet(sockfd, addr, window);
//        if(connect(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1)
//        {
////            SET_ERROR(err, strerror(errno));
//            return -1;
//        }
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;
        ipv6_addr = (struct sockaddr_in6 *)addr;
        ipv6_addr->sin6_port = net_port;
        send_syn_packet(sockfd, addr, window);
//        if(connect(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in6)) == -1)
//        {
////            SET_ERROR(err, strerror(errno));
//            return -1;
//        }
    }
    else
    {
//        SET_ERROR(err, "Address family not supported");
        return -1;
    }
    printf("Connected to: %s:%u\n", addr_str, port);

    return 0;
}

int receive_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt)
{
    int result;

    result = read_flags(pt->hd.flags);

    switch (result)
    {
        case ESTABLISH_HANDSHAKE:
        {
            send_syn_ack_packet(sockfd, addr, window, pt);
            break;
        }
        case SENDACK:
        {
            send_data_ack_packet(sockfd, addr, window, pt);
            break;
        }
        case RECVACK:
        {
            recv_ack_packet(sockfd, addr, window, pt);
            break;
        }
        case END_CONNECTION:
        {
            recv_termination_request(sockfd, addr, window, pt);
            break;
        }
        case RECVRST:
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
        return SENDACK;
    }

    if (flags == PSHACK)
    {
        return SENDACK;
    }

    if (flags == ACK)
    {
        return RECVACK;
    }

    if (flags == FINACK)
    {
        return END_CONNECTION;
    }

    if (flags == RSTACK)
    {
        return RECVRST;
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

    send_packet(sockfd, addr, window, &packet_to_send);

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
