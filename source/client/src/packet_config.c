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
        window[i]->is_packet_empty = 1;
    }
    first_empty_packet      = 0;
    first_unacked_packet    = 0;
    is_window_available     = 0;
    return 0;
}

int window_empty(struct sent_packet *window)
{
    if (window[first_empty_packet].is_packet_empty == 0)
    {
        is_window_available = 0;
        return 0;
    }

    is_window_available = 1;
    return -1;
}

int first_packet_ring_buffer(struct sent_packet *window, uint8_t window_size)
{
    if (window[first_empty_packet].is_packet_empty)
    {
        return 1;
    }

    if (first_empty_packet + 1 >= window_size)
    {
        if (window[0].is_packet_empty)
        {
            first_empty_packet = 0;
            return 1;
        }

        first_empty_packet = first_unacked_packet;
        return 0;
    }

    if (window[first_empty_packet + 1].is_packet_empty)
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

int first_unacked_ring_buffer(struct sent_packet *window, uint8_t window_size)
{
    if(!window[first_unacked_packet].is_packet_empty){
        return 1;
    }
    if(first_unacked_packet + 1 >= window_size){
        if
    }

}
