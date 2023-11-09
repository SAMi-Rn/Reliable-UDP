#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "packet_config.h"

int main(int argc, char *argv[]) {
    int client_sockfd, server_sockfd;
    struct sockaddr_in client_addr, server_addr, proxy_client_addr, proxy_server_addr;
    socklen_t client_len = sizeof(client_addr), server_len = sizeof(server_addr);
    char buffer[544];
    uint32_t read, write;


    client_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_sockfd < 0 || server_sockfd < 0) perror("ERROR opening socket");

    memset(&proxy_client_addr, 0, sizeof(proxy_client_addr));
    proxy_client_addr.sin_family = AF_INET;
    proxy_client_addr.sin_addr.s_addr = inet_addr("10.0.0.116");
    proxy_client_addr.sin_port =  htons(50000);

    memset(&proxy_server_addr, 0, sizeof(proxy_server_addr));
    proxy_server_addr.sin_family = AF_INET;
    proxy_server_addr.sin_addr.s_addr = inet_addr("10.0.0.116");
    proxy_server_addr.sin_port = htons(60000);

    if (bind(client_sockfd, (struct sockaddr *)&proxy_client_addr, sizeof(proxy_client_addr)) < 0){
        perror("ERROR");
    }

    while (1) {
        // client to server
        read = recvfrom(client_sockfd, buffer, 544, 0, (struct sockaddr *)&client_addr, &client_len);
        if (read < 0) {
            perror("ERROR reading from client socket");
        }

        write = sendto(server_sockfd, buffer, (size_t)read, 0, (struct sockaddr *)&proxy_server_addr, sizeof(proxy_server_addr));
        if (write < 0){
            perror("ERROR writing to server socket");
        }

        // server to client
        read = recvfrom(server_sockfd, buffer, 512, 0, (struct sockaddr *)&server_addr, &server_len);
        if (read < 0){
            perror("ERROR reading from server socket");
        }

        write = sendto(client_sockfd, buffer, (size_t)read, 0, (struct sockaddr *)&client_addr, client_len);
        if (write < 0) {
            perror("ERROR writing to client socket");
        }
    }

    close(client_sockfd);
    close(server_sockfd);
    return 0;
}
