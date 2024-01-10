/**
  * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
  * $Date$
  * $Rev$
  * $URL$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "telescoped.h"
#include "socket.h"

int sock_recv(int sockfd, char *p_recvbuf)
{
    fd_set rfd;
    struct timeval timeout;

    bzero(p_recvbuf, SOCK_RECVBUF_MAX);

    FD_ZERO(&rfd);
    FD_SET(sockfd, &rfd);

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (select(FD_SETSIZE, &rfd, (fd_set *)0, (fd_set *)0, &timeout) <= 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "select(rfd) failure: %i: %s", errno, strerror(errno));
        return -1;
    }

    if (recv(sockfd, p_recvbuf, SOCK_RECVBUF_MAX, 0) <= 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "recv() failure: %i: %s", errno, strerror(errno));
        return -1;
    }

    return 0;
}

int sock_send(int sockfd, char *p_msg)
{
    int msg_len = strlen(p_msg);
    int count;
    struct timeval timeout;
    fd_set wfd;

    FD_ZERO(&wfd);
    FD_SET(sockfd, &wfd);

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (select(FD_SETSIZE, (fd_set *)0, &wfd, (fd_set *)0, &timeout) <= 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "select(wfd) failure: %i: %s", errno, strerror(errno));
        return -1;
    }

    if ((count = send(sockfd, p_msg, msg_len, 0)) != msg_len) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "send() failure: %i: %s", errno, strerror(errno));
        return -1;
    }

    return 0;
}

int sock_client_create(char *p_ip, int port, int *p_sockfd)
{
    unsigned long flag_ioctl;
    fd_set wfd;
    struct timeval timeout;
    struct sockaddr_in server_address;

    bzero((char *)&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(p_ip);
    server_address.sin_port = htons(port);

    if ((*p_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "socket() failure: %i: %s", errno, strerror(errno));
        return -1;
    }

    flag_ioctl = 1;
    if (ioctl(*p_sockfd, FIONBIO, &flag_ioctl) == -1) { // SET NONBLOCK
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "ioctl(FIONBIO) failure: %i: %s", errno, strerror(errno));
        close(*p_sockfd);
        return -1;
    }

    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "socket %i connect to %s:%i", *p_sockfd, p_ip, port);
    if (connect(*p_sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        if (errno != EINPROGRESS) {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "connect() failure: %i: %s", errno, strerror(errno));
            close(*p_sockfd);
            return -1;
        }
    }

    FD_ZERO(&wfd);
    FD_SET(*p_sockfd, &wfd);

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if (select(FD_SETSIZE, (fd_set *)0, &wfd, (fd_set *)0, &timeout) <= 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "select() failure: %i: %s", errno, strerror(errno));
        close(*p_sockfd);
        return -1;
    }

    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "create socket %i success", *p_sockfd);
    return 0;
}
