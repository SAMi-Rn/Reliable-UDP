#include "packet_config.h"

int create_window(struct sent_packet **window, uint8_t cmd_line_window_size)
{
    window_size = cmd_line_window_size;

    *window = (sent_packet*)malloc(sizeof(sent_packet) * window_size);
    if (window == NULL)
    {
        return -1;
    }
    for (int i = 0; i < window_size; i++)
    {
        window[i] = (sent_packet*)malloc(sizeof(sent_packet));
    }

    for (int i = 0; i < window_size; i++)
    {
        window[i]->is_packet_empty = 0;
    }
    first_empty_packet      = 0;
    first_unacked_packet    = 0;
    is_window_available     = 1;
    return 0;
}

int window_empty(struct sent_packet *window)
{
    if (window[first_empty_packet].is_packet_empty)
    {
        is_window_available = 1;
        return 0;
    }

    is_window_available = 0;
    return -1;
}

int first_packet_ring_buffer(struct sent_packet *window, uint8_t window_size)
{
    if (window[first_empty_packet].is_packet_empty)
    {
        return 1;
    }

    if (first_empty_packet + 1 >= window_size)
    {
        if (window[0].is_packet_empty)
        {
            first_empty_packet = 0;
            return 1;
        }

        first_empty_packet = first_unacked_packet;
        return 0;
    }

    if (window[first_empty_packet + 1].is_packet_empty)
    {
        first_empty_packet++;
        return 1;
    }
    else
    {
        first_empty_packet = first_unacked_packet;
        return 0;
    }
}

int first_unacked_ring_buffer(struct sent_packet *window, uint8_t window_size)
{
    if (!window[first_unacked_packet].is_packet_empty)
    {
        return 1;
    }

    if (first_unacked_packet + 1 >= window_size)
    {
        if (!window[0].is_packet_empty)
        {
            first_unacked_packet = 0;
            return 1;
        }

        first_unacked_packet = first_empty_packet;
        return 0;
    }

    if (!window[first_unacked_packet + 1].is_packet_empty)
    {
        first_unacked_packet++;
        return 1;
    }
    else
    {
        first_unacked_packet = first_empty_packet;
        return 0;
    }
}

int send_syn_packet(int sockfd, struct sockaddr_storage addr, struct sent_packet *window)
{
    struct packet packet_to_send;

    packet_to_send.hd.seq_number            = create_sequence_number(0, 0);
    packet_to_send.hd.ack_number            = create_ack_number(0, 0);
    packet_to_send.hd.flags                 = SYN;
    packet_to_send.hd.window_size           = window_size;

    send_packet(sockfd, addr, window, &packet_to_send);
    return 0;
}

int receive_packet(int sockfd, struct sockaddr_storage addr, struct sent_packet *window, struct packet *pt)
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
            send_ack_packet(sockfd, addr, window, pt);
            break;
        }
        case RECVACK:
        {
            send_ack_packet(sockfd, addr, window, pt);
            break;
        }
        case END_CONNECTION:
        {
            recv_termination_request(sockfd, addr, window, pt);
            break;
        }
        case RECVRST:
        {
            return -1;
        }
        case UNKNOWN_FLAG:
        {
            return -1;
        }
        default:
        {
            return -1;
        }
    }

    return 0;
}

int send_packet(int sockfd, struct sockaddr_storage addr, struct sent_packet *window, struct packet *pt)
{

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

int send_syn_ack_packet(int sockfd, struct sockaddr_storage addr, struct sent_packet *window, struct packet *pt)
{
    struct packet packet_to_send;

    packet_to_send.hd.flags             = create_flags(pt->hd.flags);
    packet_to_send.hd.window_size       = window_size;
    packet_to_send.hd.seq_number        = create_second_handshake_seq_number();
    packet_to_send.hd.ack_number        = create_ack_number(pt->hd.seq_number, strlen(pt->data));

    send_packet(sockfd, addr, window, &packet_to_send);

    return 0;
}

int send_ack_packet(int sockfd, struct sockaddr_storage addr, struct sent_packet *window, struct packet *pt)
{
    return 0;
}

int recv_ack_packet(int sockfd, struct sockaddr_storage addr, struct sent_packet *window, struct packet *pt)
{
    // check if incoming ack is same as expected ack
    return 0;
}

int recv_termination_request(int sockfd, struct sockaddr_storage addr, struct sent_packet *window, struct packet *pt)
{
    // send ack and then check if all packets have been acked
    // then send fin ack
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

uint32_t create_second_handshake_seq_number(void)
{
     return 100;
}


uint32_t create_sequence_number(uint32_t prev_seq_number, uint32_t data_size)
{
    return prev_seq_number + data_size;
}

uint32_t create_ack_number(uint32_t recv_seq_number, uint32_t data_size)
{
    return recv_seq_number + data_size;
}


