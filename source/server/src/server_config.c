#include "server_config.h"

int socket_create(int domain, int type, int protocol, struct fsm_error *err)
{
    int sockfd;

    sockfd = socket(domain, type, protocol);

    if(sockfd == -1)
    {
        SET_ERROR(err, strerror(errno));
        return -1;
    }

    return sockfd;
}

//int socket_connect(int sockfd, struct sockaddr_storage *addr, in_port_t port, struct fsm_error *err)
//{
//    char      addr_str[INET6_ADDRSTRLEN];
//    in_port_t net_port;
//
//    if(inet_ntop(addr->ss_family, addr->ss_family == AF_INET ? (void *)&(((struct sockaddr_in *)addr)->sin_addr) : (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr), addr_str, sizeof(addr_str)) == NULL)
//    {
//        SET_ERROR(err, strerror(errno));
//        return -1;
//    }
//
//    printf("Connecting to: %s:%u\n", addr_str, port);
//    net_port = htons(port);
//
//    if(addr->ss_family == AF_INET)
//    {
//        struct sockaddr_in *ipv4_addr;
//        ipv4_addr = (struct sockaddr_in *)addr;
//        ipv4_addr->sin_port = net_port;
//        if(connect(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1)
//        {
//            SET_ERROR(err, strerror(errno));
//            return -1;
//        }
//    }
//    else if(addr->ss_family == AF_INET6)
//    {
//        struct sockaddr_in6 *ipv6_addr;
//        ipv6_addr = (struct sockaddr_in6 *)addr;
//        ipv6_addr->sin6_port = net_port;
//        if(connect(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in6)) == -1)
//        {
//            SET_ERROR(err, strerror(errno));
//            return -1;
//        }
//    }
//    else
//    {
//        SET_ERROR(err, "Address family not supported");
//        return -1;
//    }
//    printf("Connected to: %s:%u\n", addr_str, port);
//
//    return 0;
//}


int socket_close(int sockfd, struct fsm_error *err)
{
    if (close(sockfd) == -1)
    {
        SET_ERROR(err, strerror(errno));
        return -1;
    }

    return 0;
}

int convert_address(const char *address, struct sockaddr_storage *addr, struct fsm_error *err)
{
    memset(addr, 0, sizeof(*addr));

    if(inet_pton(AF_INET, address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        // IPv4 address
        addr->ss_family = AF_INET;
    }
    else if(inet_pton(AF_INET6, address, &(((struct sockaddr_in6 *)addr)->sin6_addr)) == 1)
    {
        // IPv6 address
        addr->ss_family = AF_INET6;
    }
    else
    {
        SET_ERROR(err, "Address family not supported");
        return -1;
    }

    return 0;
}
