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
    server_addr.sin_port = htons(60000);
    server_addr.sin_addr.s_addr = inet_addr("10.0.0.116");

    int sd = socket_create(AF_INET, SOCK_DGRAM, 0, &err);

//    bind(sd, (struct sockaddr*) &server_addr, sizeof(server_addr));

    struct packet pt;

    char temp[510] = "fjfksfjwe";
    strcpy(pt.data, temp);
    pt.hd.acknowledgment_number = 471264781;
    pt.hd.sequence_number = 28141084;
    pt.hd.window_size = 10;
    pt.hd.flags = 2;
    gettimeofday(&pt.hd.tv, NULL);

    printf("size packet: %lu\ndata: %lu\nheader: %lu", sizeof(pt), sizeof(pt.data), sizeof(pt.hd));
    printf("\nseq: %lu\nack: %lu", sizeof(pt.hd.sequence_number), sizeof(pt.hd.acknowledgment_number));
    printf("\ntv: %lu\nflags: %lu\nwindow: %lu", sizeof(pt.hd.tv), sizeof(pt.hd.flags),sizeof(pt.hd.window_size) );


    sendto(sd, &pt, sizeof(pt), 0, (struct sockaddr*) &server_addr, sizeof(server_addr));
//    while (1)
//    {
//        recvfrom(sd, &pt, sizeof(pt), 0, (struct sockaddr*)  &client_addr, (socklen_t *) sizeof(client_addr));
//    }


    return 0;
}
