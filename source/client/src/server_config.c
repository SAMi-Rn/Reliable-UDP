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

int read_keyboard(char **buffer, uint32_t *file_index) {
//    char *line = NULL;
char line[511];
    size_t len = 0;
    ssize_t read;

    read = 0;
    memset(line, 0, sizeof(line));
    printf("\nEnter string below [ctrl + d] to quit\n");
    while (fgets(line, sizeof(line), stdin) == NULL)
    {
//        printf("data: %s", line);
//        if (strlen(line) > 0)
//        {
//            break;
//        }
    }

    printf("in read keyboard\ndata: %s\n", line);
    printf("length: %lu\n", strlen(line));
//    while(fgets(line, sizeof(line) , stdin) != NULL)
//    {
//        printf("%s\n", line);
//    }

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
        // IPv4 server_addr
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

int socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port, struct fsm_error *err)
{
//    char      addr_str[INET6_ADDRSTRLEN];
//    socklen_t addr_len;
//    void     *vaddr;
//    in_port_t net_port;
//
//    net_port = htons(60000);
//
//    if(addr->ss_family == AF_INET)
//    {
//        struct sockaddr_in *ipv4_addr;
//
//        ipv4_addr           = (struct sockaddr_in *)addr;
//        addr_len            = sizeof(*ipv4_addr);
//        ipv4_addr->sin_port = net_port;
//        vaddr               = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
//    }
//    else if(addr->ss_family == AF_INET6)
//    {
//        struct sockaddr_in6 *ipv6_addr;
//
//        ipv6_addr            = (struct sockaddr_in6 *)addr;
//        addr_len             = sizeof(*ipv6_addr);
//        ipv6_addr->sin6_port = net_port;
//        vaddr                = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
//    }
//    else
//    {
//        fprintf(stderr, "Internal error: addr->ss_family must be AF_INET or AF_INET6, was: %d\n", addr->ss_family);
//        return -1;
//    }
//
//    if(inet_ntop(addr->ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL)
//    {
//        perror("inet_ntop");
//        return -1;
//    }

//    printf("Binding to: %s:%u\n", add, port);

    if(bind(sockfd, (struct sockaddr *)addr, size_of_address(addr)) == -1)
    {
        perror("Binding failed");
        fprintf(stderr, "Error code: %d\n", errno);
        return -1;
    }

//    printf("Bound to socket: %s:%u\n", addr_str, port);

    return 0;
}

socklen_t size_of_address(struct sockaddr_storage *addr)
{
    return addr->ss_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
}
