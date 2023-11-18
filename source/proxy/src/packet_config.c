#include "packet_config.h"
int create_window(struct sent_packet **window, uint8_t cmd_line_window_size, uint8_t *first_empty_packet)
{
    window_size = cmd_line_window_size;
    *window = (sent_packet*)malloc(sizeof(sent_packet) * window_size);

    if (window == NULL)
    {
        return -1;
    }

    for (int i = 0; i < window_size; i++)
    {
        window[i] = (sent_packet*)calloc(0, sizeof(sent_packet));
    }

    for (int i = 0; i < window_size; i++)
    {
        printf("in create_window: %d: %d\n", i, window[i] -> is_packet_full);
    }

    *first_empty_packet      = 0;

    return 0;
}

int first_packet_ring_buffer(struct sent_packet *window, uint8_t *first_empty_packet)
{
    for (int i = 0; i < window_size; i++)
    {
        if (!window[i].is_packet_full)
        {
            *first_empty_packet = i;

            return 0;
        }
    }

    return -1;
}

int remove_packet_from_window(struct sent_packet *window, struct packet *pt)
{
    for (int i = 0; i < window_size; i++)
    {
        if (window[i].pt.hd.seq_number == pt -> hd.seq_number)
        {
            window[i].is_packet_full = 0;

            return 0;
        }
    }

    return -1;
}
