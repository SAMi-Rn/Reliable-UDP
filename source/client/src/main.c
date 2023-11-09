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
    printf("Size of packets: %zu \n", sizeof(packet));
    printf("Size of header: %zu \n", sizeof(((packet *)0)->hd));
    printf("Size of date: %zu \n", sizeof(((packet *)0)->data));
    printf("Size of header: %zu \n", sizeof(header));
    printf("Size of sequence_number: %zu \n", sizeof(((header*)0)->sequence_number));
    printf("Size of acknowledgment_number: %zu \n", sizeof(((header*)0)->acknowledgment_number));
    printf("Size of flags: %zu \n", sizeof(((header*)0)->flags));
    printf("Size of window_size: %zu \n", sizeof(((header*)0)->window_size));
    printf("Size of struct timeval: %zu \n", sizeof(((header*)0)->tv));

    return 0;
}
