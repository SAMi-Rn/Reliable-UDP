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
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(50000);
    server_addr.sin_addr.s_addr = inet_addr("10.0.0.116");

    int sd = socket_create(AF_INET, SOCK_DGRAM, 0, &err);


    struct packet pt;

    char temp[510] = "ack";
    strcpy(pt.data, temp);
    pt.hd.acknowledgment_number = 471264781;
    pt.hd.sequence_number = 28141084;
    pt.hd.window_size = 10;
    pt.hd.flags = 2;
    gettimeofday(&pt.hd.tv, NULL);

    struct sent_packet pp;
    struct sent_packet *ddp;
    create_window(&ddp, 3);
    ddp[0].pt.hd.acknowledgment_number = 2312313;
    ddp[1].pt.hd.acknowledgment_number = 34141;
    ddp[2].pt.hd.acknowledgment_number = 442342;
//    window_empty(&ddp, 3);
//    printf("is_empty: %d", can_send_packet);

    printf("\nsize of sent_packet: %lu\n", sizeof(pp));
    printf("size of window: %lu\n", sizeof(ddp[2]));
    printf("1 packet ack: %u", ddp[0].pt.hd.acknowledgment_number);
    printf("2 packet ack: %u", ddp[1].pt.hd.acknowledgment_number);
    printf("3 packet ack: %u\n", ddp[2].pt.hd.acknowledgment_number);


    sendto(sd, &pt, sizeof(pt), 0, (struct sockaddr*) &server_addr, sizeof(server_addr));
//    while (1)
//    {
//        recvfrom(sd, &pt, sizeof(pt), 0, (struct sockaddr*)  &client_addr, (socklen_t *) sizeof(client_addr));
//    }

    free(ddp);
    return 0;
}
