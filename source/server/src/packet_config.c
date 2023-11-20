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
        window[i] = (sent_packet*)calloc(0, sizeof(sent_packet));
//        window[i] -> is_packet_full = 1;
    }

    for (int i = 0; i < 5; i++)
    {
        printf("in create_window: %d: %d\n", i, window[i] -> is_packet_full);
    }

    first_empty_packet      = 0;
    first_unacked_packet    = 0;
    is_window_available     = TRUE;

    return 0;
}

int send_packet(int sockfd, struct sockaddr_storage *addr, struct packet *pt, struct fsm_error *err)
{
    ssize_t result;

    result = sendto(sockfd, pt, sizeof(*pt), 0, (struct sockaddr *) addr,
                    size_of_address(addr));

    if (result == -1)
    {
        SET_ERROR(err, strerror(errno));
        return -1;
    }

    printf("\nfirst empty: %d\nfirst unacked: %d\n", first_empty_packet, first_unacked_packet);
    printf("\n\nSENDING:\n");
    printf("bytes: %zd\n", result);
    printf("seq number: %u\n", pt->hd.seq_number);
    printf("ack number: %u\n", pt->hd.ack_number);
    printf("window number: %u\n", pt->hd.window_size);
    printf("flags: %u\n", pt->hd.flags);
    printf("time: %ld\n", pt->hd.tv.tv_sec);
    printf("data: %s\n\n", pt->data);

    return 0;
}

int receive_packet(int sockfd, struct packet *temp_packet, struct fsm_error *err)
{
    struct sockaddr_storage     client_addr;
    socklen_t                   client_addr_len;
    struct packet               pt;
    ssize_t                     result;

    client_addr_len = sizeof(client_addr);
    result = recvfrom(sockfd, &pt, sizeof(pt), 0, (struct sockaddr *) &client_addr, &client_addr_len);

    if (result == -1)
    {
        SET_ERROR(err, strerror(errno));
        return -1;
    }

    printf("\n\nRECEIVED:\n");
    printf("bytes: %zd\n", result);
    printf("seq number: %u\n", pt.hd.seq_number);
    printf("ack number: %u\n", pt.hd.ack_number);
    printf("window number: %u\n", pt.hd.window_size);
    printf("flags: %u\n", pt.hd.flags);
    printf("time: %ld\n", pt.hd.tv.tv_sec);
    printf("data: %s\n\n\n\n", pt.data);

    *temp_packet = pt;

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

int check_seq_number(uint32_t seq_number, uint32_t expected_seq_number)
{
    return seq_number <= expected_seq_number ? TRUE : FALSE;
}

uint32_t update_expected_seq_number(uint32_t seq_number, uint32_t data_size)
{
    printf("expected: %u\n", seq_number + data_size);
    return seq_number + data_size;
}
