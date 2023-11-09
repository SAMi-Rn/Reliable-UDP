#include <stdio.h>
#include "fsm.h"
#include "packet_config.h"
#include "server_config.h"

enum application_states
{
    STATE_PARSE_ARGUMENTS = FSM_USER_START
};

typedef struct arguments
{
    struct packet_sent *window;
} arguments;



int main(int argc, char **argv)
{
    struct fsm_error err;
    struct arguments args;
    struct fsm_context context = {
            .argc = argc,
            .argv = argv,
            .args = &args
    };

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    client_addr_len = sizeof(client_addr);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(60000);
    server_addr.sin_addr.s_addr = inet_addr("10.0.0.116");

    int sd = socket_create(AF_INET, SOCK_DGRAM, 0, &err);

    int bind_result;
    bind_result = bind(sd, (struct sockaddr*) &server_addr, sizeof(server_addr));

    if (bind_result < 0)
    {
        printf("bind failed");
        exit(EXIT_FAILURE);
    }
    struct packet pt;
    struct sent_packet pp;

    printf("size packet: %lu\ndata: %lu\nheader: %lu", sizeof(pt), sizeof(pt.data), sizeof(pt.hd));
    printf("\nseq: %lu\nack: %lu", sizeof(pt.hd.sequence_number), sizeof(pt.hd.acknowledgment_number));
    printf("\ntv: %lu\nflags: %lu\nwindow: %lu", sizeof(pt.hd.tv), sizeof(pt.hd.flags),sizeof(pt.hd.window_size) );
    printf("size of window: %lu\n", sizeof(pp));

    while (1)
    {
        uint32_t result;
        result = 0;
        result = recvfrom(sd, &pt, sizeof(pt), 0, (struct sockaddr*)  &client_addr, &client_addr_len);
        if (result > 0)
        {
            printf("bytes: %u\n", result);
            printf("seq number: %u\n", pt.hd.sequence_number);
            printf("ack number: %u\n", pt.hd.acknowledgment_number);
            printf("window number: %u\n", pt.hd.window_size);
            printf("flags: %u\n", pt.hd.flags);
            printf("time: %ld\n", pt.hd.tv.tv_sec);
            printf("time: %s\n", pt.data);
        }
    }
    return 0;
}
