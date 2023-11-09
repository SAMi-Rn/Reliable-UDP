#include "packet_config.h"

int create_ack_number(uint32_t seq_number, uint32_t data_size, uint32_t *ack_number)
{
    *ack_number = seq_number + data_size;

    return 0;
}

int create_ack_packet(const struct packet *client_packet, struct packet *ack_packet)
{
    ack_packet->hd.sequence_number = client_packet->hd.acknowledgment_number;
    ack_packet->hd.acknowledgment_number = client_packet->hd.sequence_number + strlen(client_packet->data);


}



