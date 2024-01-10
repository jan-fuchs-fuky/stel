#ifndef __SOCKET_H
#define __SOCKET_H

#define SOCKET_MAXSEG_SIZE 512
#define SOCKET_LISTEN_MAX  100
#define SOCKET_RECVBUF_MAX (1024*10)

int socket_server_create(char *p_ip, int port, int *p_sockfd);
int socket_server_destroy(int sockfd);

#endif
