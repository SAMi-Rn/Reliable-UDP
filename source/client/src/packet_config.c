#include <netinet/in.h>
#include "packet_config.h"

int create_window(struct sent_packet **window, uint8_t cmd_line_window_size, struct fsm_error *err)
{
    window_size     = cmd_line_window_size;
    *window         = (struct sent_packet *) malloc(sizeof(struct sent_packet) * window_size + 1);

    if (window == NULL)
    {
        SET_ERROR(err, strerror(errno));
        return -1;
    }

    for (int i = 0; i < window_size; i++)
    {
//        window[i] = malloc(sizeof(*window[i]));
        (*window)[i].is_packet_full = 0;
    }

    first_empty_packet      = 0;
    first_unacked_packet    = 0;
    is_window_available     = TRUE;

    return 0;
}

int window_empty(struct sent_packet *window)
{
    if (!window[first_empty_packet].is_packet_full)
    {
        is_window_available = TRUE;
        return 1;
    }

    is_window_available = FALSE;
    return 0;
}

int first_packet_ring_buffer(struct sent_packet *window)
{
    if (!window[first_empty_packet].is_packet_full)
    {
        return 1;
    }

    if (first_empty_packet + 1 >= window_size)
    {
        if (!window[0].is_packet_full)
        {
            first_empty_packet = 0;
            return 1;
        }

        first_empty_packet = first_unacked_packet;
        return 0;
    }

    if (!window[first_empty_packet + 1].is_packet_full)
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
    if (window[first_unacked_packet].is_packet_full)
    {
        return 1;
    }

    if (first_unacked_packet + 1 >= window_size)
    {
        if (window[0].is_packet_full)
        {
            first_unacked_packet = 0;
            return 1;
        }

        first_unacked_packet = first_empty_packet;
        return 0;
    }

    if (window[first_unacked_packet + 1].is_packet_full)
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


int send_packet(int sockfd, struct sockaddr_storage *addr, struct sent_packet *window,
                struct packet *pt, struct fsm_error *err)
{
    ssize_t result;

    result = sendto(sockfd, pt, sizeof(*pt), 0, (struct sockaddr *) addr,
                    size_of_address(addr));

//    printf("first empty: %d\nfirst unacked: %d\n", first_empty_packet, first_unacked_packet);
    printf("\n\nSENDING:\n");
    printf("seq number: %u\n", pt->hd.seq_number);
    printf("flags: %u\n", pt->hd.flags);
    printf("data: %s\n\n\n\n", pt->data);

    if (result < 0)
    {
        SET_ERROR(err, strerror(errno));
        return -1;
    }

    return 0;
}

int add_packet_to_window(struct sent_packet *window, struct packet *pt)
{
    gettimeofday(&pt->hd.tv, NULL);
    window[first_empty_packet].pt                           = *pt;
    if (pt->hd.flags == ACK)
    {
        window[first_empty_packet].is_packet_full           = FALSE;
        first_empty_packet++;
        first_unacked_ring_buffer(window);
    }
    else
    {
        window[first_empty_packet].is_packet_full           = TRUE;
    }

    if (pt->hd.flags == SYN)
    {
        window[first_empty_packet].expected_ack_number      = pt -> hd.seq_number + 1;
    }
    else if (pt->hd.flags == SYNACK)
    {
        window[first_empty_packet].expected_ack_number      = pt -> hd.seq_number + 1;
        window[first_empty_packet].pt.hd.seq_number         = pt -> hd.seq_number + 1;
    }
    else
    {
        window[first_empty_packet].expected_ack_number      = pt->hd.seq_number +
                                                                strlen(pt->data);
    }
    first_packet_ring_buffer(window);
    window_empty(window);

    return 0;
}

int receive_packet(int sockfd, struct sent_packet *window, struct packet *pt, struct fsm_error *err)
{
    struct sockaddr_storage     client_addr;
    socklen_t                   client_addr_len;
    struct packet               temp_pt;
    ssize_t                     result;

    client_addr_len     = sizeof(client_addr);
    result              = recvfrom(sockfd, &temp_pt, sizeof(temp_pt), 0, (struct sockaddr *) &client_addr, &client_addr_len);

    if (result == -1)
    {
        SET_ERROR(err, strerror(errno));
        return -1;
    }

    *pt = temp_pt;

    printf("\n\nRECEIVED:\n");
    printf("ack number: %u\n", pt->hd.ack_number);
    printf("flags: %u\n", pt->hd.flags);

    window_empty(window);

    return 0;
}

int remove_packet_from_window(struct sent_packet *window, struct packet *pt)
{
    if (window[first_unacked_packet].expected_ack_number == pt->hd.ack_number)
    {
        window[first_unacked_packet].is_packet_full = FALSE;
        first_unacked_ring_buffer(window);

        return 1;
    }

    return 0;
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

int add_connection(struct sockaddr_storage *addr)
{
    struct sockaddr_storage *temp;
    temp = (struct sockaddr_storage *) realloc(list_of_connections,
                                                              (sizeof(list_of_connections)/ sizeof(struct sockaddr_storage) + 1) *
                                                                      sizeof(struct sockaddr_storage));

    if (temp == NULL)
    {
        return -1;
    }

    temp[(sizeof(temp)/ sizeof(struct sockaddr_storage))] = *addr;
    list_of_connections = temp;

    return 0;
}

int valid_connection(struct sockaddr_storage *addr)
{
    for (int i = 0; i < sizeof(list_of_connections)/ sizeof(struct sockaddr_storage); i++)
    {
//        if (list_of_connections[i] == server_addr_struct)
//        {
//           return 1;
//        }
    }

    return 0;
}

int check_ack_number(uint32_t expected_ack_number, uint32_t ack_number)
{
    return expected_ack_number == ack_number ? TRUE : FALSE;
}

int previous_index(struct sent_packet *window)
{
    if (first_empty_packet == 0)
    {
        return window_size - 1;
    }

    return first_empty_packet - 1;
}
