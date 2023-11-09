#include "packet_config.h"
int create_window(struct sent_packet **window, uint8_t window_size)
{
    *window = malloc(sizeof(struct sent_packet) * window_size);
    if (*window == NULL)
    {
        return -1; // Allocation failed
    }

    // Initialize the allocated memory
    memset(*window, 0, sizeof(struct sent_packet) * window_size);

    return 0; // Success
}


int window_empty(struct sent_packet *window, uint8_t window_size)
{


        if (window[0].has_been_acked == 0)
        {

            return 0;
        }

    return 1;
}





