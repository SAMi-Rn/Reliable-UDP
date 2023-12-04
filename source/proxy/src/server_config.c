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

int start_listening(int sockfd, int backlog, struct fsm_error *err)
{
    if(listen(sockfd, backlog) == -1)
    {
        SET_ERROR(err, strerror(errno));

        return -1;
    }

    return 0;
}

int socket_accept_connection(int sockfd, struct fsm_error *err)
{
    struct sockaddr             client_addr;
    socklen_t                   client_addr_len;

    client_addr_len             = sizeof(client_addr);
    int client_fd;

    errno = 0;
    client_fd = accept(sockfd, &client_addr, &client_addr_len);

    if(client_fd == -1)
    {
        if(errno != EINTR)
        {
            perror("Error in connecting to client.");
        }
        SET_ERROR(err, strerror(errno));

        return -1;
    }

    return client_fd;
}

int socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port, struct fsm_error *err)
{
    char      addr_str[INET6_ADDRSTRLEN];
    socklen_t addr_len;
    void     *vaddr;
    in_port_t net_port;

    net_port = htons(port);

    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        addr_len            = sizeof(*ipv4_addr);
        ipv4_addr->sin_port = net_port;
        vaddr               = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr_len             = sizeof(*ipv6_addr);
        ipv6_addr->sin6_port = net_port;
        vaddr                = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
    }
    else
    {
        fprintf(stderr, "Internal error: addr->ss_family must be AF_INET or AF_INET6, was: %d\n", addr->ss_family);
        return -1;
    }

    if(inet_ntop(addr->ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL)
    {
        perror("inet_ntop");
        return -1;
    }

    printf("Binding to: %s:%u\n", addr_str, port);

    if(bind(sockfd, (struct sockaddr *)addr, addr_len) == -1)
    {
        perror("Binding failed");
        fprintf(stderr, "Error code: %d\n", errno);
        return -1;
    }

    printf("Bound to socket: %s:%u\n", addr_str, port);

    return 0;
}

int convert_address(const char *address, struct sockaddr_storage *addr,
                    in_port_t port, struct fsm_error *err)
{
    memset(addr, 0, sizeof(*addr));
    char      addr_str[INET6_ADDRSTRLEN];
    socklen_t addr_len;
    void     *vaddr;
    in_port_t net_port;

    net_port = htons(port);

    if(inet_pton(AF_INET, address, &(((struct sockaddr_in *)addr)->sin_addr)) == 1)
    {
        // IPv4 server_addr
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        addr_len            = sizeof(*ipv4_addr);
        ipv4_addr->sin_port = net_port;
        vaddr               = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
        addr->ss_family = AF_INET;
        printf("IP: %u", ipv4_addr->sin_addr.s_addr);
    }
    else if(inet_pton(AF_INET6, address, &(((struct sockaddr_in6 *)addr)->sin6_addr)) == 1)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr_len             = sizeof(*ipv6_addr);
        ipv6_addr->sin6_port = net_port;
        vaddr                = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
        // IPv6 server_addr
        addr->ss_family = AF_INET6;
    }
    else
    {
        SET_ERROR(err, "Address family not supported");
        return -1;
    }

    return 0;
}

int socket_close(int sockfd, struct fsm_error *err)
{
    if (close(sockfd) == -1)
    {
        SET_ERROR(err, strerror(errno));
        return -1;
    }

    return 0;
}

int send_stats_gui(int sockfd, int stat)
{
    uint8_t converted_size;
    ssize_t result;

    converted_size  = htonl(stat);
    result          = write(sockfd, &stat, sizeof(stat));

    if (result <= 0)
    {
        return -1;
    }


    return 0;
}
