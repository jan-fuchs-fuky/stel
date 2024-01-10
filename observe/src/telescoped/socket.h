/**
  * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
  * $Date$
  * $Rev$
  * $URL$
 */

#ifndef __SOCKET_H
#define __SOCKET_H

#define SOCK_RECVBUF_MAX 1024

int sock_recv(int sockfd, char *p_recvbuf);
int sock_send(int sockfd, char *p_msg);
int sock_client_create(char *p_ip, int port, int *p_sockfd);

#endif
