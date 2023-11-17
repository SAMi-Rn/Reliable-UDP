#include "proxy_config.h"

int identify_sender(struct sockaddr_storage *dest_ip,
        struct sockaddr_storage *client, struct sockaddr_storage *server)
{
    if(dest_ip->ss_family == AF_INET)
    {
        struct sockaddr_in          *ipv4_dest_ip_addr;
        ipv4_dest_ip_addr           = (struct sockaddr_in *)dest_ip;

        if (client->ss_family == AF_INET)
        {
           struct sockaddr_in       *ipv4_client_addr;
           ipv4_client_addr         = (struct sockaddr_in *) client;

           if (ipv4_dest_ip_addr->sin_addr.s_addr == ipv4_client_addr->sin_addr.s_addr)
           {
              return CLIENT;
           }
        }
        else if (server->ss_family == AF_INET)
        {
            struct sockaddr_in      *ipv4_server_addr;
            ipv4_server_addr        = (struct sockaddr_in *) client;

            if (ipv4_dest_ip_addr->sin_addr.s_addr == ipv4_server_addr->sin_addr.s_addr)
            {
                return SERVER;
            }
        }
        else
        {
            return UNKNOWN;
        }
    }
    else if(dest_ip->ss_family == AF_INET6)
    {
        struct sockaddr_in6          *ipv6_dest_ip_addr;

        ipv6_dest_ip_addr            = (struct sockaddr_in6 *)dest_ip;

        if (client->ss_family == AF_INET)
        {
            struct sockaddr_in6      *ipv6_client_addr;
            ipv6_client_addr         = (struct sockaddr_in6 *) client;

            if (ipv6_dest_ip_addr->sin6_addr.s6_addr == ipv6_client_addr->sin6_addr.s6_addr)
            {
                return CLIENT;
            }
        }
        else if (server->ss_family == AF_INET)
        {
            struct sockaddr_in6      *ipv6_server_addr;
            ipv6_server_addr         = (struct sockaddr_in6 *) client;

            if (ipv6_dest_ip_addr->sin6_addr.s6_addr == ipv6_server_addr->sin6_addr.s6_addr)
            {
                return SERVER;
            }
        }
        else
        {
            return UNKNOWN;
        }
    }

    return UNKNOWN;
}

int random_number(void)
{
    srand(time(NULL));

    return rand() % 101;
}

int calculate_lossiness(uint8_t drop_rate, uint8_t delay_rate)
{
    if (drop_rate > 0)
    {
        if (calculate_drop(drop_rate))
        {
            return DROP;
        }
    }

    if (delay_rate > 0)
    {
        if (calculate_delay(delay_rate))
        {
            return DELAY;
        }
    }

    return SEND;
}

int calculate_drop(uint8_t percentage)
{
    int rand;
    rand = random_number();

    return rand > percentage ? FALSE : TRUE;
}

int calculate_delay(uint8_t percentage)
{
    int rand;
    rand = random_number();

    return rand > percentage ? FALSE : TRUE;
}

int send_packet(int sockfd, packet *pt, struct sockaddr_storage *addr)
{
    ssize_t result;

    result = sendto(sockfd, pt, sizeof(*pt), 0, (struct sockaddr *) addr,
                    size_of_address(addr));

    if (result < 0)
    {
        return -1;
    }

    return 0;
}

int receive_packet(int sockfd, struct packet *pt)
{
    struct sockaddr_storage     client_addr;
    socklen_t                   client_addr_len;
    struct packet               temp_pt;
    ssize_t                     result;

    client_addr_len = sizeof(client_addr);
    result = recvfrom(sockfd, &temp_pt, sizeof(temp_pt), 0, (struct sockaddr *) &client_addr, &client_addr_len);
    if (result == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }
    *pt = temp_pt;

    return 0;
}

void delay_packet(packet *pt, uint8_t delay_time)
{
    sleep(delay_time);
}

socklen_t size_of_address(struct sockaddr_storage *addr)
{
    return addr->ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
}
