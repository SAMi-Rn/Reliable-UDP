#include "packet_config.h"

int create_window(struct sent_packet **window, uint8_t window_size)
{
    window = malloc(sizeof(sent_packet) * window_size);
    if (window == NULL)
    {
        return -1;
    }

    return 0;
}

int window_empty(struct sent_packet **window, uint8_t window_size)
{
    for (int i = 0; i < window_size; i++)
    {
        if (window[i] != 0)
        {
            can_send_packet = -1;
        }
    }

    can_send_packet = 0;

    return 0;
}





