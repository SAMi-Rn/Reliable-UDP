#include <netinet/in.h>
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
        return 1;
    }

    is_window_available = 0;
    return 0;
}

int first_packet_ring_buffer(struct sent_packet *window)
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

int first_unacked_ring_buffer(struct sent_packet *window)
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


int send_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window, struct packet *pt)
{
    ssize_t result;

    // make new function that populates the send_packet struct in the window
    gettimeofday(&pt->hd.tv, NULL);
    result = sendto(sockfd, pt, sizeof(*pt), 0, (struct sockaddr *) addr, size_of_address(addr));
    first_packet_ring_buffer(window);
    if (result < 0)
    {
        printf("error: %d\n", errno);
        return -1;
    }

    return 0;
}

socklen_t size_of_address(struct sockaddr_storage *addr)
{
    return addr->ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
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

uint32_t previous_seq_number(struct sent_packet *window)
{
    if (first_empty_packet == 0)
    {
        return window[window_size - 1].pt.hd.seq_number;
    }

    return window[first_empty_packet - 1].pt.hd.seq_number;
}

uint32_t previous_data_size(struct sent_packet *window)
{
    if (first_empty_packet == 0)
    {
        return strlen(window[window_size - 1].pt.data);
    }

    return strlen(window[first_empty_packet - 1].pt.data);
}

uint32_t previous_ack_number(struct sent_packet *window)
{
    if (first_empty_packet == 0)
    {
        return window[window_size - 1].pt.hd.ack_number;
    }

    return window[first_empty_packet - 1].pt.hd.ack_number;
}
