/*
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
 *
 *   Copyright (C) 2010-2012 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
 *
 *   This file is part of Observe (Observing System for Ondrejov).
 *
 *   Observe is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Observe is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Observe.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>
#include <xmlrpc-c/server_abyss.h>
#include <log4c.h>
#include <glib.h>
#include <time.h>

#include "spectrographd.h"
#include "../telescoped/thread.h"
#include "../telescoped/socket.h"
#include "../telescoped/str.h"

log4c_category_t *p_logcat = NULL;

static int spectrograph_exit_flag = 0;
static int sockfd_loop = -1;
static int sockfd_cmd = -1;
static pthread_mutex_t global_mutex;
static pthread_mutex_t data_mutex;
static pthread_t spectrograph_loop_pthread;
static pthread_t spectrograph_cmd_pthread;
static char info_data[INFO_SIZE_E][INFO_MAX+1];
static char info_cmds[INFO_SIZE_E][INFO_MAX+1];
static char *p_spectrographd_dir;
static SPECTROGRAPH_CFG_T spectrograph_cfg;
static SPECTROGRAPH_IP_T *p_spectrograph_ip_first = NULL;

static void spectrograph_help(char *name)
{
    printf("Usage: %s\n", name);
    printf("    -h, --help       Print this help and exit\n");
    printf("        --version    Display version information and exit\n");
}

static void spectrograph_version(char *name)
{
    printf("%s r%s %s\n", name, SVN_REV, MAKE_DATE_TIME);
    printf("Written by Jan Fuchs <fuky@sunstel.asu.cas.cz>\n\n");

    printf("Copyright (C) 2010-2011 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.\n");
    printf("This is free software; see the source for copying conditions.  There is NO\n");
    printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}

static void spectrograph_signal(int sig)
{
  void *thread_result;

    switch (sig) {
        case SIGTERM:
            spectrograph_exit_flag = 1;
            log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Received signal SIGTERM");

            pthread_join(spectrograph_loop_pthread, &thread_result);
            pthread_join(spectrograph_cmd_pthread, &thread_result);

            log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Exiting...");

            exit(EXIT_SUCCESS);

        default:
            break;
    }
}

static int spectrograph_exit(int status)
{
    if (spectrograph_cfg.p_key_file != NULL) {
        g_key_file_free(spectrograph_cfg.p_key_file);
        spectrograph_cfg.p_key_file = NULL;
    }

    exit(status);
}

static int spectrograph_loop_service()
{
    int i;
    char recvbuf[SOCK_RECVBUF_MAX];

    if (sock_client_create(spectrograph_cfg.ascol_ip, spectrograph_cfg.ascol_loop_port, &sockfd_loop) == -1) {
        return -1;
    }

    while (!spectrograph_exit_flag) {

#ifdef DBG
        struct timeval start, end;
        long mtime, seconds, useconds;
        gettimeofday(&start, NULL);
#endif

        for (i = 0; i < INFO_SIZE_E; ++i) {
            if (sock_send(sockfd_loop, info_cmds[i]) == -1) {
                return -1;
            }

            if (sock_recv(sockfd_loop, recvbuf) == -1) {
                return -1;
            }

            /* LOCK */
            pthr_mutex_lock(&data_mutex);
            strncpy(info_data[i], strim(recvbuf), INFO_MAX);
            pthr_mutex_unlock(&data_mutex);
            /* UNLOCK */
        }

#ifdef DBG
        gettimeofday(&end, NULL);
        seconds = end.tv_sec  - start.tv_sec;
        useconds = end.tv_usec - start.tv_usec;
        mtime = ((seconds) * 1000 + useconds/1000.0);
        log4c_category_log(p_logcat, LOG4C_PRIORITY_DEBUG,
            "spectrograph_loop_service() elapsed time: %ld milliseconds", mtime);
#endif

    }

    return 0;
}

static int spectrograph_cmd_service()
{
    int i;
    int socket_result = 0;
    char recvbuf[SOCK_RECVBUF_MAX];

    if (sock_client_create(spectrograph_cfg.ascol_ip, spectrograph_cfg.ascol_cmd_port, &sockfd_cmd) == -1) {
        return -1;
    }

    /* LOCK */
    pthr_mutex_lock(&global_mutex);
    if ((socket_result = sock_send(sockfd_cmd, "GLLG 123\n")) != -1) {
        socket_result = sock_recv(sockfd_cmd, recvbuf);
    }
    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    while ((!spectrograph_exit_flag) && (!socket_result)) {
        /* LOCK */
        pthr_mutex_lock(&global_mutex);
        if ((socket_result = sock_send(sockfd_cmd, "GLST\n")) != -1) {
            socket_result = sock_recv(sockfd_cmd, recvbuf);
        }
        pthr_mutex_unlock(&global_mutex);
        /* UNLOCK */

        if (socket_result == -1) {
            break;
        }

        for (i = 0; i < 30; ++i) {
            if (spectrograph_exit_flag) {
                break;
            }
            sleep(1);
        }
    }

    return socket_result;
}

static void run_loop(int *p_sockfd, int (*fce_loop)())
{
    while ((!spectrograph_exit_flag) && (fce_loop() == -1)) {
        if (*p_sockfd != -1) {
            close(*p_sockfd);
        }

        sleep(1);
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "Reconnect spectrograph");
    }

    if (*p_sockfd != -1) {
        close(*p_sockfd);
    }
}

static void *spectrograph_loop(void *p_arg)
{
    run_loop(&sockfd_loop, spectrograph_loop_service);
    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Exiting spectrograph_loop_pthread...");
    pthread_exit(NULL);
}

static void *spectrograph_cmd(void *p_arg)
{
    run_loop(&sockfd_cmd, spectrograph_cmd_service);
    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Exiting spectrograph_cmd_pthread...");
    pthread_exit(NULL);
}

static unsigned char *spectrograph_get_ip_addr(TSession * const p_abyss_session)
{
    struct abyss_unix_chaninfo *p_chan_info;
    struct sockaddr_in *p_sock_addr_in;

    SessionGetChannelInfo(p_abyss_session, (void*)&p_chan_info);

    p_sock_addr_in = (struct sockaddr_in *)&p_chan_info->peerAddr;
    return (unsigned char *)&p_sock_addr_in->sin_addr.s_addr;
}

static int spectrograph_allowed_ip(char *p_ip)
{
    SPECTROGRAPH_IP_T *p_spectrograph_ip;

    p_spectrograph_ip = p_spectrograph_ip_first;
    while (p_spectrograph_ip != NULL) {
        if (!strcmp(p_spectrograph_ip->ip, p_ip)) {
            return 1;
        }
        p_spectrograph_ip = p_spectrograph_ip->p_next;
    }

    return 0;
}

static xmlrpc_value *spectrograph_execute(xmlrpc_env * const p_env,
                                       xmlrpc_value * const p_param_array,
                                       void * const p_server_info,
                                       void * const p_chan_info)
{
    int socket_result;
    char *p_command;
    char command[COMMAND_MAX+1];
    char recvbuf[SOCK_RECVBUF_MAX];
    char ip[CFG_TYPE_STR_MAX+1];
    unsigned char *p_ip_addr;
    xmlrpc_value *p_result = NULL;

    p_ip_addr = spectrograph_get_ip_addr(p_chan_info);
    snprintf(ip, CFG_TYPE_STR_MAX, "%u.%u.%u.%u",
        p_ip_addr[0], p_ip_addr[1], p_ip_addr[2], p_ip_addr[3]);

    if (!spectrograph_allowed_ip(ip)) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN,
            "%s is not allowed IP", ip);
        XMLRPC_FAIL(p_env, XMLRPC_INTERNAL_ERROR, "permission denied");
    }

    xmlrpc_decompose_value(p_env, p_param_array, "(s)", &p_command);
    if (p_env->fault_occurred) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN,
            "xmlrpc_decompose_value(): %i: %s", p_env->fault_code, p_env->fault_string);
        XMLRPC_FAIL(p_env, XMLRPC_PARSE_ERROR, "xmlrpc_decompose_value() failed");
    }

    if ((!spectrograph_cfg.execute_spch_10) && (!strncmp(p_command, "SPCH 10", 7))) {
        p_result = xmlrpc_build_value(p_env, "s", "0\0");

        log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
            "RPC spectrograph_execute is from IP address %s, IGNORING command = '%s'",
            ip, p_command);

        goto cleanup;
    }

    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
        "RPC spectrograph_execute is from IP address %s, command = '%s'",
        ip, p_command);

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    if (spectrograph_cfg.protection) {
        // 8 = flat, 9 = comp, 1 = on
        if ((!strcmp(p_command, "SPCH 8 1")) || (!strcmp(p_command, "SPCH 9 1"))) {
            if (spectrograph_cfg.coude_exposimeter_close) {
                // 10 = coude exposimeter shutter, 2 = close
                sock_send(sockfd_cmd, "SPCH 10 2\n");
                sock_recv(sockfd_cmd, recvbuf);
            }

            if (spectrograph_cfg.oes_exposimeter_close) {
                // 23 = oes exposimeter shutter, 2 = close
                sock_send(sockfd_cmd, "SPCH 23 2\n");
                sock_recv(sockfd_cmd, recvbuf);
            }

            // 15 = slith camera, 5 = close
            sock_send(sockfd_cmd, "SPCH 15 5\n");
            sock_recv(sockfd_cmd, recvbuf);
        }
    }

    snprintf(command, COMMAND_MAX, "%s\n", p_command);
    socket_result = sock_send(sockfd_cmd, command);
    if ((socket_result = sock_recv(sockfd_cmd, recvbuf)) != -1) {
        p_result = xmlrpc_build_value(p_env, "s", strim(recvbuf));
    }
    else {
        p_result = xmlrpc_build_value(p_env, "s", strerror(errno));
    }

    if (spectrograph_cfg.protection) {
        if ((!strcmp(p_command, "SPCH 8 0")) || (!strcmp(p_command, "SPCH 9 0"))) {
            // 15 = slith camera, 1 = open
            sock_send(sockfd_cmd, "SPCH 15 1\n");
            sock_recv(sockfd_cmd, recvbuf);
        }
    }

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

cleanup:
    /* CLEANUP */

    return p_result;
}

static xmlrpc_value *spectrograph_info(xmlrpc_env * const p_env,
                                    xmlrpc_value * const p_param_array,
                                    void * const p_server_info,
                                    void * const p_chan_info)
{
    xmlrpc_value *p_result = NULL;

    /* LOCK */
    pthr_mutex_lock(&data_mutex);

    p_result = xmlrpc_build_value(p_env, "{s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s}",
        "GLST", info_data[INFO_GLST_E],
        "SPGP_4", info_data[INFO_SPGP_4_E],
        "SPGP_5", info_data[INFO_SPGP_5_E],
        "SPGP_13", info_data[INFO_SPGP_13_E],
        "SPCE_14", info_data[INFO_SPCE_14_E],
        "SPFE_14", info_data[INFO_SPFE_14_E],
        "SPCE_24", info_data[INFO_SPCE_24_E],
        "SPFE_24", info_data[INFO_SPFE_24_E],
        "SPGP_22", info_data[INFO_SPGP_22_E],
        "SPGS_19", info_data[INFO_SPGS_19_E],
        "SPGS_20", info_data[INFO_SPGS_20_E]);

    pthr_mutex_unlock(&data_mutex);
    /* UNLOCK */

    return p_result;
}

static void spectrograph_info_init()
{
    bzero(info_data, sizeof(info_data));
    bzero(info_cmds, sizeof(info_cmds));

    strncpy(info_cmds[INFO_GLST_E], "GLST\n", INFO_MAX);
    strncpy(info_cmds[INFO_SPGP_4_E], "SPGP 4\n", INFO_MAX);
    strncpy(info_cmds[INFO_SPGP_5_E], "SPGP 5\n", INFO_MAX);
    strncpy(info_cmds[INFO_SPGP_13_E], "SPGP 13\n", INFO_MAX);
    strncpy(info_cmds[INFO_SPCE_14_E], "SPCE 14\n", INFO_MAX);
    strncpy(info_cmds[INFO_SPFE_14_E], "SPFE 14\n", INFO_MAX);
    strncpy(info_cmds[INFO_SPCE_24_E], "SPCE 24\n", INFO_MAX);
    strncpy(info_cmds[INFO_SPFE_24_E], "SPFE 24\n", INFO_MAX);
    strncpy(info_cmds[INFO_SPGP_22_E], "SPGP 22\n", INFO_MAX);
    strncpy(info_cmds[INFO_SPGS_19_E], "SPGS 19\n", INFO_MAX);
    strncpy(info_cmds[INFO_SPGS_20_E], "SPGS 20\n", INFO_MAX);
}

static void spectrograph_cfg_get_string(char *p_dest, char *p_group_name, char *p_key)
{
    char *p_char;
    GError *p_error = NULL;

    p_char = g_key_file_get_string(spectrograph_cfg.p_key_file, p_group_name, p_key, &p_error);

    if (p_error != NULL) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
            "Get %s.%s failed: %s", p_group_name, p_key, p_error->message);
        spectrograph_exit(EXIT_FAILURE);
    }

    if (p_char != NULL) {
        strncpy(p_dest, p_char, CFG_TYPE_STR_MAX);
        log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
            "%s.%s = %s", p_group_name, p_key, p_char);
    }
    else {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
            "%s.%s = NULL", p_group_name, p_key);
        spectrograph_exit(EXIT_FAILURE);
    }
}

static void spectrograph_cfg_get_integer(int *p_dest, char *p_group_name, char *p_key)
{
    GError *p_error = NULL;

    *p_dest = g_key_file_get_integer(spectrograph_cfg.p_key_file, p_group_name, p_key, &p_error);

    if (p_error != NULL) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
            "Get %s.%s failed: %s", p_group_name, p_key, p_error->message);
        spectrograph_exit(EXIT_FAILURE);
    }

    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
        "%s.%s = %i", p_group_name, p_key, *p_dest);
}

static void spectrograph_cfg_get_allow_ips()
{
    int i;
    gchar **p_keys;
    gsize length;
    GError *p_error = NULL;
    SPECTROGRAPH_IP_T *p_spectrograph_ip;
    SPECTROGRAPH_IP_T *p_spectrograph_ip_new;

    p_keys = g_key_file_get_keys(spectrograph_cfg.p_key_file, "allow_ips", &length, &p_error);

    if (p_error != NULL) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
            "Get allow_ips failed: %s", p_error->message);
        spectrograph_exit(EXIT_FAILURE);
    }

    for (i = 0; i < length; ++i) {
        // TODO: append free
        if ((p_spectrograph_ip_new = malloc(sizeof(SPECTROGRAPH_IP_T))) == NULL) {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR, "malloc() failed");
            spectrograph_exit(EXIT_FAILURE);

            exit(EXIT_FAILURE);
        }

        if (p_spectrograph_ip_first == NULL) {
            p_spectrograph_ip_first = p_spectrograph_ip_new;
        }
        else {
            p_spectrograph_ip->p_next = p_spectrograph_ip_new;
        }

        p_spectrograph_ip = p_spectrograph_ip_new;
        p_spectrograph_ip->p_next = NULL;

        strncpy(p_spectrograph_ip->name, p_keys[i], CFG_TYPE_STR_MAX);
        spectrograph_cfg_get_string(p_spectrograph_ip->ip, "allow_ips", p_keys[i]);
    }

    p_spectrograph_ip = p_spectrograph_ip_first;
    while (p_spectrograph_ip != NULL) {
        //log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
        //    "allowed name = %s, ip = %s", p_spectrograph_ip->name, p_spectrograph_ip->ip);
        p_spectrograph_ip = p_spectrograph_ip->p_next;
    }
}

static void spectrograph_load_cfg()
{
    GKeyFileFlags flags;
    GError *p_error = NULL;

    spectrograph_cfg.p_key_file = g_key_file_new();
    flags = G_KEY_FILE_NONE;

    if (!g_key_file_load_from_file(spectrograph_cfg.p_key_file, spectrograph_cfg.cfg_file, flags, &p_error)) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
            "Unable to load file %s", spectrograph_cfg.cfg_file);
        spectrograph_exit(EXIT_FAILURE);
    }

    spectrograph_cfg_get_string(spectrograph_cfg.file_pid, "spectrographd", "file_pid");
    spectrograph_cfg_get_string(spectrograph_cfg.xmlrpc_log, "spectrographd", "xmlrpc_log");
    spectrograph_cfg_get_string(spectrograph_cfg.ascol_ip, "spectrographd", "ascol_ip");
    spectrograph_cfg_get_integer(&spectrograph_cfg.port, "spectrographd", "port");
    spectrograph_cfg_get_integer(&spectrograph_cfg.execute_spch_10, "spectrographd", "execute_spch_10");
    spectrograph_cfg_get_integer(&spectrograph_cfg.protection, "spectrographd", "protection");
    spectrograph_cfg_get_integer(&spectrograph_cfg.oes_exposimeter_close, "spectrographd", "oes_exposimeter_close");
    spectrograph_cfg_get_integer(&spectrograph_cfg.coude_exposimeter_close, "spectrographd", "coude_exposimeter_close");
    spectrograph_cfg_get_integer(&spectrograph_cfg.ascol_loop_port, "spectrographd", "ascol_loop_port");
    spectrograph_cfg_get_integer(&spectrograph_cfg.ascol_cmd_port, "spectrographd", "ascol_cmd_port");
    spectrograph_cfg_get_allow_ips();

    g_key_file_free(spectrograph_cfg.p_key_file);
    spectrograph_cfg.p_key_file = NULL;
}

static void spectrograph_create_pid()
{
    FILE *fw;

    if ((fw = fopen(spectrograph_cfg.file_pid, "w")) == NULL) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
            "spectrograph_create_pid() => fopen(): %i: %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    else {
        if (fprintf(fw, "%i\n", getpid()) <= 0) {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "spectrograph_create_pid() => fprintf(): %i: %s", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (fclose(fw) == EOF) {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "spectrograph_create_pid() => fclose(): %i: %s", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

static void spectrograph_daemonize(void)
{
    int stdin_null, stdout_null, stderr_null; 
    pid_t pid;
    pid_t sid;

    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        /* exit the parent process */
        exit(EXIT_SUCCESS);
    }
            
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    umask(0);

    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    else if (pid > 0) {
        /* exit the parent process */
        exit(EXIT_SUCCESS);
    }
    
    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }
    
    stdin_null = open("/dev/null", O_RDONLY);
    stdout_null = open("/dev/null", O_APPEND | O_CREAT);
    stderr_null = open("/dev/null", O_APPEND | O_CREAT);

    dup2(stdin_null, STDIN_FILENO);
    dup2(stdout_null, STDOUT_FILENO);
    dup2(stderr_null, STDERR_FILENO);
}

int main(int argc, char *argv[])
{
    xmlrpc_server_abyss_parms serverparm;
    xmlrpc_registry *registryP;
    xmlrpc_env env;

    struct xmlrpc_method_info3 const spectrograph_execute_MI = {
        .methodName     = "spectrograph_execute",
        .methodFunction = &spectrograph_execute,
    };

    struct xmlrpc_method_info3 const spectrograph_info_MI = {
        .methodName     = "spectrograph_info",
        .methodFunction = &spectrograph_info,
    };

    if ((p_spectrographd_dir = getenv("SPECTROGRAPHD_DIR")) == NULL) {
        fprintf(stderr, "The SPECTROGRAPHD_DIR environment variable is not set.");
        exit(EXIT_FAILURE);
    }

    bzero(&spectrograph_cfg, sizeof(SPECTROGRAPH_CFG_T));
    spectrograph_cfg.p_key_file = NULL;
    snprintf(spectrograph_cfg.cfg_file, CFG_TYPE_STR_MAX, "%s/etc/spectrographd.cfg", p_spectrographd_dir);

    switch (argc) {
        case 2:
            if (!strcmp(argv[1], "--version")) {
                spectrograph_version(argv[0]);
            }
            else {
                spectrograph_help(argv[0]);
            }
            exit(EXIT_SUCCESS);
            break;

        default: break;
    }

    spectrograph_daemonize();
    spectrograph_info_init();

    if (log4c_init()) {
        perror("log4c_init() failed");
        exit(EXIT_FAILURE);
    }

    p_logcat = log4c_category_get("spectrographd");
    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Starting spectrographd r%s %s",
                       SVN_REV, MAKE_DATE_TIME);

    spectrograph_load_cfg();
    spectrograph_create_pid();

    (void)signal(SIGTERM, spectrograph_signal);
    (void)signal(SIGPIPE, SIG_IGN);       /* send() */

    if (pthread_mutex_init(&global_mutex, NULL) != 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR, "pthread_mutex_init(): %i: %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&data_mutex, NULL) != 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR, "pthread_mutex_init(): %i: %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&spectrograph_loop_pthread, NULL, spectrograph_loop, NULL) != 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR, "pthread_create(): %i: %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&spectrograph_cmd_pthread, NULL, spectrograph_cmd, NULL) != 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR, "pthread_create(): %i: %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
 
    xmlrpc_env_init(&env);

    registryP = xmlrpc_registry_new(&env);

    xmlrpc_registry_add_method3(&env, registryP, &spectrograph_execute_MI);
    xmlrpc_registry_add_method3(&env, registryP, &spectrograph_info_MI);

    serverparm.config_file_name = NULL;
    serverparm.registryP = registryP;
    serverparm.port_number = spectrograph_cfg.port;

    if (*spectrograph_cfg.xmlrpc_log != '\0') {
        serverparm.log_file_name = spectrograph_cfg.xmlrpc_log;
        log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "xmlrpc_log is enabled");
    }
    else {
        serverparm.log_file_name = NULL;
        log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "xmlrpc_log is disabled");
    }

    xmlrpc_server_abyss(&env, &serverparm, XMLRPC_APSIZE(log_file_name));

    pthread_mutex_destroy(&global_mutex);
    pthread_mutex_destroy(&data_mutex);
    log4c_fini();

    exit(EXIT_SUCCESS);
}
