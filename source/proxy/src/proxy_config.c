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

void delay_packet(uint8_t delay_time)
{
    sleep(delay_time);
}

socklen_t size_of_address(struct sockaddr_storage *addr)
{
    return addr->ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
}

int read_keyboard(uint8_t *client_drop, uint8_t *client_delay, uint8_t *server_drop, uint8_t *server_delay) {
//    char *line = NULL;
//    size_t len = 0;
//    ssize_t read;
    int first_menu;
    int second_menu;
    char menu[100];
    snprintf(menu, sizeof (menu), "\nDynamic Proxy Lossiness Value:\n"
                                  "1. Client Losiness:\n"
                                  "2. Server Losiness:\n"
                                  "3. Exit:\n"
                                  "Enter your Answer: ");

    do
    {
        printf("%s\n", menu);
        first_menu = read_menu(3);
        switch (first_menu)
        {
            case 1:
                printf("%d\n", first_menu);
                do
                {
                    printf("Client Drop and Delay rate\n");
                    printf("1. Drop Rate: \n");
                    printf("2. Delay Rate: \n");
                    printf("3. Back \n");
                    second_menu = read_menu(3);
                    switch (second_menu)
                    {
                        case 1:
                            printf("Enter Client's Drop Rate :");
                            int drop =  read_menu(100);
                            if(drop == -1)
                            {
                                printf("Client's Drop Rate value should be between 0-100!\n");
                                break;
                            }
                            *client_drop = drop;
                            second_menu = 3;
                            break;
                        case 2:
                            printf("Enter Client's Delay Rate :");
                            int delay =read_menu(100);
                            if(delay == -1)
                            {
                                printf("Client's Delay Rate value should be between 0-100!\n");
                                break;
                            }
                            *client_delay = delay;
                            second_menu = 3;
                            break;
                        case 3:
                            second_menu = 3;
                            break;
                        case -1:
                            printf("It is not valid, try again\n");
                        default:
                            break;
                    }
                }
                while(second_menu != 3);
            case 2:
                do
                {
                    printf("Server Drop and Delay rate\n");
                    printf("1. Drop Rate: \n");
                    printf("2. Delay Rate: \n");
                    printf("3. Back \n");
                    second_menu = read_menu(3);
                    switch (second_menu)
                    {
                        case 1:
                            printf("Enter Server's Drop Rate :");
                            int drop =  read_menu(100);
                            if(drop == -1)
                            {
                                printf("Server's Drop Rate value should be between 0-100!\n");
                                break;
                            }
                            *server_drop = drop;
                            second_menu = 3;
                            break;

                        case 2:
                            printf("Enter Server's Delay Rate :");
                            int delay =read_menu(100);
                            if(delay == -1)
                            {
                                printf("Server's Delay Rate value should be between 0-100!\n");
                                break;
                            }
                            *server_delay = delay;
                            second_menu = 3;
                            break;

                        case 3:
                            second_menu = 3;
                            break;
                        case -1:
                            printf("It is not valid, try again\n");
                        default:
                            break;
                    }
                }
                while(second_menu != 3);
            case 3:
                first_menu = 3;
                return 0;
            case -1:
                printf("It is not valid, try again\n");
            default:
                break;
        }
    }
    while (first_menu != 3);

    /*
     * User entered C (Client)
     *      Enter B to go back, D for drop, L for delay
     *      1. Drop
     *      2. Delay
     *      3. Back
     *      entered : 2
     *          Enter your drop rate: 20 (change the client drop rate to 20!)
     *
     *          return
     */

//    printf("read: %s\nsize: %zd", line, strlen(line));
//    *buffer = (char *) malloc(strlen(line));
//    strcpy(*buffer, line);


    return 0;
}

int read_menu(int upperbound)
{
    char buf[128];
    errno = 0;
    fgets(buf, 128, stdin);

    char            *endptr;
    int temp = (int)strtol(buf, &endptr, 10);
    if (errno != 0)
    {
//        SET_ERROR(err, strerror(errno));

        return -1;
    }

    if(*endptr != '\n')
    {
        printf("end pointer: %c\n", *endptr);
        printf("%d ",temp);

        return -1;
    }
    if((int) temp > upperbound)
    {
        printf("larger than upperbound!\n");
        return -1;
    }
    printf("Temp : %d\n\n", (int) temp);
    return (int) temp ;
}

