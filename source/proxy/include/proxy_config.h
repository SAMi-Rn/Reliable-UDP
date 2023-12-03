#ifndef PROXY_PROXY_CONFIG_H
#define PROXY_PROXY_CONFIG_H

#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "packet_config.h"
#include "inttypes.h"

enum bools
{
    FALSE = 0,
    TRUE = 1
};

enum return_states
{
    DROP,
    DELAY,
    CORRUPT,
    SEND
};


int         random_number(size_t upperbound);
int         calculate_lossiness(uint8_t drop_rate, uint8_t delay_rate, uint8_t corruption_rate);
int         calculate_drop(uint8_t percentage);
int         calculate_delay(uint8_t percentage);
int         calculate_corruption(uint8_t percentage);
int         send_packet(int sockfd, packet *pt, struct sockaddr_storage *addr);
int         receive_packet(int sockfd, struct packet *pt);
void        delay_packet(uint8_t delay_time);
void        read_keyboard(uint8_t *client_drop, uint8_t *client_delay, uint8_t *server_drop, uint8_t *server_delay, uint8_t *corruption_rate);
int         read_menu(int upperbound);
socklen_t   size_of_address(struct sockaddr_storage *addr);
int         corrupt_data(char **data, size_t length);

#endif //PROXY_PROXY_CONFIG_H
