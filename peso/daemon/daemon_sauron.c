#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <syslog.h>
#include <getopt.h>
#include <time.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <fitsio.h>
#include <pthread.h>
#include <semaphore.h>

#include "daemon_sauron.h"
#include "socket.h"

static int server_sockfd;
static int client_sockfd;
static sem_t expose_sem;
static sem_t service_sem;

static void daemon_version(void)
{
    printf("%s %s%s\n", APP_NAME, VERSION, MAKE_DATE_TIME);
    printf("Written by Jan Fuchs <fuky@sunstel.asu.cas.cz>\n\n");
    printf("Copyright (C) 2008 Free Software Foundation, Inc.\n");
    printf("This is free software; see the source for copying conditions.  There is NO\n");
    printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}

static void daemon_signal(int sig)
{
    switch (sig) {
        case SIGUSR1:
        case SIGHUP:
        case SIGINT:
            /* expose readout */
            break;

        case SIGTERM:
            /* expose abort */
            exit(EXIT_SUCCESS);

        default:
            break;
    }
}

static int parse_commands(char *p_commands)
{
    char *p_str;
    char *p_save;
    char *p_cmd_arg;
    char *p_arg;
    char cmd[CMD_MAX+1];
    
    for (p_str = p_commands; ; p_str = NULL) {
        p_cmd_arg = strtok_r(p_str, "\n", &p_save);
        if (p_cmd_arg == NULL) {
            break;
        }
        syslog(LOG_INFO, p_cmd_arg);
    
        if ((p_arg = strchr(p_cmd_arg, ' ')) != NULL) {
            memset(&cmd, 0, CMD_MAX+1);
            strncpy(&cmd, p_cmd_arg, p_arg-p_cmd_arg);

            ++p_arg;
            syslog(LOG_INFO, cmd);
            syslog(LOG_INFO, p_arg);
        }
    }

    return 0;
}

static void *service_loop(void *arg)
{
    char recvbuf[SOCKET_RECVBUF_MAX];
    int sockfd;
    int result;
    fd_set rfd;
    fd_set wfd;
    struct timeval timeout;

    sockfd = client_sockfd;

    if (sem_post(&service_sem) == -1) {
        syslog(LOG_ERR, "sem_post");
    }

    while (1) {
        FD_ZERO(&rfd);
        FD_ZERO(&wfd);
        FD_SET(sockfd, &rfd);
        FD_SET(sockfd, &wfd);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        if ((result = select(FD_SETSIZE, &rfd, &wfd, (fd_set *)0, &timeout)) == 0) {
            continue;
        }
        else if (result == -1) {
            syslog(LOG_WARNING, "Select failure");
            continue;
        }

        if (FD_ISSET(sockfd, &rfd)) {
            memset(recvbuf, 0, SOCKET_RECVBUF_MAX);
            if ((result = recv(sockfd, recvbuf, SOCKET_RECVBUF_MAX, 0)) > 0) {
                parse_commands(recvbuf);
            }
            else if (result == -1) {
                syslog(LOG_WARNING, "Recv failure");
                continue;
            }
        }

        if (FD_ISSET(sockfd, &wfd)) {
            if ((result = send(sockfd, "Hello World\n", 12, 0)) == -1) {
                syslog(LOG_WARNING, "Send failure");
            }
        }

        sleep(1);
    }
}

static void *accept_loop(void *arg)
{
    static struct sockaddr_in remote_address;
    socklen_t remote_len;
    pthread_t service_pthread;
    int result;
    fd_set rfd;
    struct timeval timeout;

    while (1) {
        FD_ZERO(&rfd);
        FD_SET(server_sockfd, &rfd);

        timeout.tv_sec = 3600;
        timeout.tv_usec = 0;

        result = select(FD_SETSIZE, &rfd, (fd_set *)0, (fd_set *)0, &timeout);

        if (result == 0) {
            continue;
        }
        else if (result == -1) {
            syslog(LOG_WARNING, "Select failure");
            continue;
        }

        if ((client_sockfd = accept(server_sockfd, (struct sockaddr *)&remote_address, &remote_len)) != -1) {
            syslog(LOG_INFO, "accept");

            if (pthread_create(&service_pthread, NULL, service_loop, NULL) != 0) {
                syslog(LOG_ERR, "pthread_create");
            }

            if (sem_wait(&service_sem) == -1) {
                syslog(LOG_ERR, "sem_wait");
            }
        }
    }
}

static int expose_loop()
{
    syslog(LOG_INFO, "waiting on expose");
    if (sem_wait(&expose_sem) == -1) {
        syslog(LOG_ERR, "sem_wait");
    }

    syslog(LOG_INFO, "expose begin");
    syslog(LOG_INFO, "expose end");

    return 0;
}

int main(int argc, char *argv[])
{
    FILE *fw;
    pid_t pid;
    pid_t sid;
    pthread_t accept_pthread;
    
    if ((argc == 2) && ((strcmp(argv[1], "-v") == 0) || (strcmp(argv[1], "--version") == 0))) {
        daemon_version();
        exit(EXIT_SUCCESS);
    }

    if (argc > 1) {
        printf("Unknown argument\n");
        exit(EXIT_FAILURE);
    }

    /* DBG */
    system("kill -9 $(cat /tmp/ccd_sauron.pid)");

    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        /* exit the parent process */
        exit(EXIT_SUCCESS);
    }

    umask(0);
            
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }
    
    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    openlog("ccd_sauron", LOG_PID|LOG_CONS, LOG_LOCAL0);
    syslog(LOG_INFO, "starting");
    
    if ((fw = fopen(FILE_PID, "w")) == NULL) {
        syslog(LOG_WARNING, "Write ccd_sauron PID failure");
    }
    else {
        fprintf(fw, "%i\n", sid);

        if (fclose(fw) == EOF) {
            syslog(LOG_WARNING, "Close FILE_PID failure");
        }
    }

    (void)signal(SIGTERM, daemon_signal); /* abort                                */
    (void)signal(SIGINT, daemon_signal);  /* readout (ctrl+c)                     */
    (void)signal(SIGHUP, daemon_signal);  /* readout (nechtene zavreni terminalu) */
    (void)signal(SIGUSR1, daemon_signal); /* readout                              */

    if (socket_server_create("127.0.0.1", 5000, &server_sockfd) == -1) {
        syslog(LOG_ERR, "socket_server_create");
    }       

    if (sem_init(&expose_sem, 0, 0) == -1) {
        syslog(LOG_ERR, "sem_init");
    }

    if (sem_init(&service_sem, 0, 0) == -1) {
        syslog(LOG_ERR, "sem_init");
    }

    if (pthread_create(&accept_pthread, NULL, accept_loop, NULL) != 0) {
        syslog(LOG_ERR, "pthread_create");
    }

    while (1) {
        expose_loop();
    }

    sem_destroy(&expose_sem);
    sem_destroy(&service_sem);
    syslog(LOG_INFO, "exit");

    if (remove(FILE_PID) == -1) syslog(LOG_WARNING, "Remove FILE_PID failure");

    exit(EXIT_SUCCESS);
}
