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

enum destinations
{
    CLIENT,
    SERVER,
    UNKNOWN
};

enum return_states
{
    DROP,
    DELAY,
    SEND
};

//uint8_t client_delay_rate;
//uint8_t server_delay_rate;
//uint8_t client_drop_rate;
//uint8_t server_drop_rate;

int         identify_sender(struct sockaddr_storage *dest_ip,
            struct sockaddr_storage *client, struct sockaddr_storage *server);
int         random_number(void);
int         calculate_lossiness(uint8_t drop_rate, uint8_t delay_rate);
int         calculate_drop(uint8_t percentage);
int         calculate_delay(uint8_t percentage);
int         send_packet(int sockfd, packet *pt, struct sockaddr_storage *addr);
int         receive_packet(int sockfd, struct packet *pt);
void        delay_packet(uint8_t delay_time);
void       read_keyboard(uint8_t *client_drop, uint8_t *client_delay, uint8_t *server_drop, uint8_t *server_delay);
int         read_menu(int upperbound);
socklen_t   size_of_address(struct sockaddr_storage *addr);

#endif //PROXY_PROXY_CONFIG_H
