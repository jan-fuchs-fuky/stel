#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/tcp.h>

#include "socket.h"

int socket_server_create(char *p_ip, int port, int *p_sockfd)
{
    int flag_sockopt;
    unsigned long flag_ioctl;
    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr((char *)p_ip);
    server_address.sin_port = htons(port);

    if ((*p_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        return -1;
    }

    flag_sockopt = SOCKET_MAXSEG_SIZE;
    if (setsockopt(*p_sockfd, IPPROTO_TCP, TCP_MAXSEG, (char *)&flag_sockopt, sizeof(flag_sockopt)) < 0) {
        return -1;
    }

    flag_ioctl = 1;
    if (ioctl(*p_sockfd, FIONBIO, &flag_ioctl) == -1) {
        return -1;
    }

    flag_sockopt = 1;
    if (setsockopt(*p_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag_sockopt, sizeof(flag_sockopt)) < 0) {
        return -1;
    }

    if (bind(*p_sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        return -1;
    }

    if (listen(*p_sockfd, SOCKET_LISTEN_MAX) == -1) {
        return -1;
    }

    return 0;
}

int socket_server_destroy(int sockfd)
{
    return close(sockfd);
}
