#include "packet_config.h"

int create_window(uint8_t window_size, struct sent_packet **window)
{
    struct sent_packet *temp;

    temp = (sent_packet *) malloc(sizeof(sent_packet) * window_size);

    window = (struct sent_packet **) temp;

    printf("size: %lu", sizeof(window));
    if (window == NULL)
    {
        return -1;
    }

    return 0;
}





