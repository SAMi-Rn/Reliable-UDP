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
