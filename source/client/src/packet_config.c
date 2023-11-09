#include "packet_config.h"

int create_window(struct sent_packet **window, uint8_t window_size)
{
    window = malloc(sizeof(sent_packet) * window_size);
    if (window == NULL)
    {
        return -1;
    }

    for (int i = 0; i < window_size; i++)
    {
        window[i]->has_been_acked = 1;
    }
    first_empty_packet      = 0;
    first_unacked_packet    = 0;
    is_window_available     = 0;
    return 0;
}

int window_empty(struct sent_packet *window)
{
    if (window[first_empty_packet].has_been_acked == 0)
    {
        first_empty_packet = 0;
        return 0;
    }

    first_empty_packet = 1;
    return -1;
}

int first_packet_ring_buffer(struct sent_packet *window, uint8_t window_size)
{
    if (window[first_empty_packet].has_been_acked)
    {
        return 0;
    }

    if (first_empty_packet + 1 >= window_size)
    {
        if (window[0].has_been_acked)
        {
            first_empty_packet = 0;
            return 0;
        }

        first_empty_packet = first_unacked_packet;
        return -1;
    }

    if (window[first_empty_packet + 1].has_been_acked)
    {
        first_empty_packet++;
        return 0;
    }
    else
    {
        first_empty_packet = first_unacked_packet;
        return -1;
    }
}

int first_unacked_ring_buffer(struct sent_packet *window, uint8_t window_size)
{

}



