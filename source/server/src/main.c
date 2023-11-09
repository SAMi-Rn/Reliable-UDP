#include <stdio.h>
#include "fsm.h"
#include "packet_config.h"

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


    struct packet pt;

    printf("%s", pt);

    printf("size packet: %lu\ndata: %lu\nheader: %lu", sizeof(pt), sizeof(pt.data), sizeof(pt.hd));
    printf("\nseq: %lu\nack: %lu", sizeof(pt.hd.sequence_number), sizeof(pt.hd.acknowledgment_number));
    printf("\ntv: %lu\nflags: %lu\nwindow: %lu", sizeof(pt.hd.tv), sizeof(pt.hd.flags),sizeof(pt.hd.window_size) );
    return 0;
}
