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

int read_keyboard(char **buffer) {
    char line[DATA_SIZE];
    memset(line, 0, sizeof(line));

    printf("\nEnter string below [ctrl + d] to quit\n");
    if (fgets(line, sizeof(line), stdin) == NULL)
    {
        return -1;
    }

    *buffer = (char *) malloc(strlen(line) + 1);
    strcpy(*buffer, line);

    return 0;
}


int socket_connect(int sockfd, struct sockaddr_storage *server_addr_struct, in_port_t port, struct fsm_error *err)
{
    char      addr_str[INET6_ADDRSTRLEN];
    in_port_t net_port;

    if(inet_ntop(server_addr_struct->ss_family, server_addr_struct->ss_family == AF_INET ? (void *)&(((struct sockaddr_in *)server_addr_struct)->sin_addr) : (void *)&(((struct sockaddr_in6 *)server_addr_struct)->sin6_addr), addr_str, sizeof(addr_str)) == NULL)
    {
        SET_ERROR(err, strerror(errno));
        return -1;
    }

    printf("Connecting to: %s:%u\n", addr_str, port);
    net_port = htons(port);

    if(server_addr_struct->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;
        ipv4_addr = (struct sockaddr_in *)server_addr_struct;
        ipv4_addr->sin_port = net_port;
        if(connect(sockfd, (struct sockaddr *)server_addr_struct, sizeof(struct sockaddr_in)) == -1)
        {
            SET_ERROR(err, strerror(errno));
            return -1;
        }
    }
    else if(server_addr_struct->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;
        ipv6_addr = (struct sockaddr_in6 *)server_addr_struct;
        ipv6_addr->sin6_port = net_port;
        if(connect(sockfd, (struct sockaddr *)server_addr_struct, sizeof(struct sockaddr_in6)) == -1)
        {
            SET_ERROR(err, strerror(errno));
            return -1;
        }
    }
    else
    {
        SET_ERROR(err, "Address family not supported");
        return -1;
    }
    printf("Connected to: %s:%u\n", addr_str, port);

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
        struct sockaddr_in *ipv4_addr;

        ipv4_addr           = (struct sockaddr_in *)addr;
        addr_len            = sizeof(*ipv4_addr);
        ipv4_addr->sin_port = net_port;
        vaddr               = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
        addr->ss_family = AF_INET;
    }
    else if(inet_pton(AF_INET6, address, &(((struct sockaddr_in6 *)addr)->sin6_addr)) == 1)
    {
        struct sockaddr_in6 *ipv6_addr;

        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr_len             = sizeof(*ipv6_addr);
        ipv6_addr->sin6_port = net_port;
        vaddr                = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
        addr->ss_family = AF_INET6;
    }
    else
    {
        SET_ERROR(err, "Address family not supported");
        return -1;
    }

    return 0;

}

int socket_bind(int sockfd, struct sockaddr_storage *addr, struct fsm_error *err)
{
    char *ip_address;
    char *port;

    ip_address  = safe_malloc(sizeof(char) * NI_MAXHOST, err);
    port        = safe_malloc(sizeof(char) * NI_MAXSERV, err);

    if (get_sockaddr_info(addr, &ip_address, &port, err) != 0)
    {
        return -1;
    }

    printf("binding to: %s:%s\n", ip_address, port);

    if(bind(sockfd, (struct sockaddr *)addr, size_of_address(addr)) == -1)
    {
        SET_ERROR(err, strerror(errno));
        return -1;
    }

    printf("Bound to socket: %s:%s\n", ip_address, port);

    free(ip_address);
    free(port);

    return 0;

}

socklen_t size_of_address(struct sockaddr_storage *addr)
{
    return addr->ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
}

int get_sockaddr_info(struct sockaddr_storage *addr, char **ip_address, char **port, struct fsm_error *err)
{
    char temp_ip[NI_MAXHOST];
    char temp_port[NI_MAXSERV];
    socklen_t ip_size;
    int result;

    ip_size     = sizeof(*addr);
    result      = getnameinfo((struct sockaddr *)addr,
                              ip_size, temp_ip, sizeof(temp_ip), temp_port, sizeof(temp_port),
                              NI_NUMERICHOST | NI_NUMERICSERV);
    if (result != 0)
    {
        SET_ERROR(err, strerror(errno));
        return -1;
    }

    strcpy(*ip_address, temp_ip);
    strcpy(*port, temp_port);

    return 0;
}

void *safe_malloc(uint32_t size, struct fsm_error *err)
{
    void *ptr;

    ptr = malloc(size);

    if (!ptr && size > 0)
    {
        perror("Malloc failed\n");
        exit(EXIT_FAILURE);
    }

    return ptr;
}
