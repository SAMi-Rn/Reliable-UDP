#include "packet_config.h"

int create_ack_number(uint32_t seq_number, uint32_t data_size, uint32_t *ack_number)
{
    *ack_number = seq_number + data_size;

    return 0;
}

int recreate_packet(const char *buffer, struct packet *pt)
{

}





