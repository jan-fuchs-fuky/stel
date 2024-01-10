/*
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
 *
 *   Copyright (C) 2010-2020 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <time.h>
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

#include "telescoped.h"
#include "thread.h"
#include "socket.h"
#include "str.h"

log4c_category_t *p_logcat = NULL;

static int telescope_exit_flag = 0;
static int sockfd_loop = -1;
static int sockfd_cmd = -1;
static pthread_mutex_t global_mutex;
static pthread_mutex_t data_mutex;
static pthread_t telescope_loop_pthread;
static pthread_t telescope_cmd_pthread;
static char info_data[INFO_SIZE_E][INFO_MAX+1];
static char info_cmds[INFO_SIZE_E][INFO_MAX+1];
static char telescope_tsra[COMMAND_MAX+1];
static char telescope_object[COMMAND_MAX+1];
static char *p_telescoped_dir;
static TELESCOPE_CFG_T telescope_cfg;
static TELESCOPE_IP_T *p_telescope_ip_first = NULL;

static void telescope_help(char *name)
{
    printf("Usage: %s\n", name);
    printf("    -h, --help       Print this help and exit\n");
    printf("        --version    Display version information and exit\n");
}

static void telescope_version(char *name)
{
    printf("%s r%s %s\n", name, SVN_REV, MAKE_DATE_TIME);
    printf("Written by Jan Fuchs <fuky@sunstel.asu.cas.cz>\n\n");

    printf("Copyright (C) 2010-2012 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.\n");
    printf("This is free software; see the source for copying conditions.  There is NO\n");
    printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}

static void telescope_signal(int sig)
{
  void *thread_result;

    switch (sig) {
        case SIGTERM:
            telescope_exit_flag = 1;
            log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Received signal SIGTERM");

            pthread_join(telescope_loop_pthread, &thread_result);
            pthread_join(telescope_cmd_pthread, &thread_result);

            log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Exiting...");

            exit(EXIT_SUCCESS);

        default:
            break;
    }
}

static int telescope_exit(int status)
{
    if (telescope_cfg.p_key_file != NULL) {
        g_key_file_free(telescope_cfg.p_key_file);
        telescope_cfg.p_key_file = NULL;
    }

    exit(status);
}

static int telescope_loop_service()
{
    int i;
    char recvbuf[SOCK_RECVBUF_MAX];

    if (sock_client_create(telescope_cfg.ascol_ip, telescope_cfg.ascol_loop_port, &sockfd_loop) == -1) {
        return -1;
    }

    while (!telescope_exit_flag) {

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
            "telescope_loop_service() elapsed time: %ld milliseconds", mtime);
#endif

    }

    return 0;
}

static int telescope_cmd_service()
{
    int i;
    int socket_result = 0;
    char recvbuf[SOCK_RECVBUF_MAX];

    if (sock_client_create(telescope_cfg.ascol_ip, telescope_cfg.ascol_cmd_port, &sockfd_cmd) == -1) {
        return -1;
    }

    /* LOCK */
    pthr_mutex_lock(&global_mutex);
    if ((socket_result = sock_send(sockfd_cmd, "GLLG 123\n")) != -1) {
        socket_result = sock_recv(sockfd_cmd, recvbuf);
    }
    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    while ((!telescope_exit_flag) && (!socket_result)) {
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
            if (telescope_exit_flag) {
                break;
            }
            sleep(1);
        }
    }

    return socket_result;
}

static void run_loop(int *p_sockfd, int (*fce_loop)())
{
    while ((!telescope_exit_flag) && (fce_loop() == -1)) {
        if (*p_sockfd != -1) {
            close(*p_sockfd);
        }

        sleep(1);
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "Reconnect telescope");
    }

    if (*p_sockfd != -1) {
        close(*p_sockfd);
    }
}

static void *telescope_loop(void *p_arg)
{
    run_loop(&sockfd_loop, telescope_loop_service);
    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Exiting telescope_loop_pthread...");
    pthread_exit(NULL);
}

static void *telescope_cmd(void *p_arg)
{
    run_loop(&sockfd_cmd, telescope_cmd_service);
    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Exiting telescope_cmd_pthread...");
    pthread_exit(NULL);
}

static unsigned char *telescope_get_ip_addr(TSession * const p_abyss_session)
{
    struct abyss_unix_chaninfo *p_chan_info;
    struct sockaddr_in *p_sock_addr_in;

    SessionGetChannelInfo(p_abyss_session, (void*)&p_chan_info);

    p_sock_addr_in = (struct sockaddr_in *)&p_chan_info->peerAddr;
    return (unsigned char *)&p_sock_addr_in->sin_addr.s_addr;
}

static int telescope_allowed_ip(char *p_ip)
{
    TELESCOPE_IP_T *p_telescope_ip;

    p_telescope_ip = p_telescope_ip_first;
    while (p_telescope_ip != NULL) {
        if (!strcmp(p_telescope_ip->ip, p_ip)) {
            return 1;
        }
        p_telescope_ip = p_telescope_ip->p_next;
    }

    return 0;
}

static char *glut2ut(char *p_glut)
{
    static char buffer[256];

    struct tm tm;
    time_t t;
    char *in;
    char *out;
    char up;
    int i;

    setenv("TZ", "UTC", 1);
    memset(&tm, 0, sizeof(struct tm));
    memset(buffer, '\0', sizeof(buffer));

    strcpy(buffer, p_glut);

    // 0123456789
    // HHMMSS.SSSYYYYmmdd
    up = buffer[7];
    in = buffer + 10;
    out = buffer + 6;

    for (i = 0; i < 8; ++i) {
        *out++ = *in++;
    }
    *out = '\0';

    strptime(buffer, "%H%M%S%Y%m%d", &tm);
    t = mktime(&tm);
    if (up >= '5') {
        ++t;
    }

    memset(buffer, '\0', sizeof(buffer));
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);

    return buffer;
}

static xmlrpc_value *telescope_execute(xmlrpc_env * const p_env,
                                       xmlrpc_value * const p_param_array,
                                       void * const p_server_info,
                                       void * const p_chan_info)
{
    const char *p_result_str;

    int socket_result;
    char *p_command;
    char command[COMMAND_MAX+1];
    char recvbuf[SOCK_RECVBUF_MAX];
    char ip[CFG_TYPE_STR_MAX+1];
    unsigned char *p_ip_addr;
    xmlrpc_value *p_result = NULL;

    p_ip_addr = telescope_get_ip_addr(p_chan_info);
    snprintf(ip, CFG_TYPE_STR_MAX, "%u.%u.%u.%u",
        p_ip_addr[0], p_ip_addr[1], p_ip_addr[2], p_ip_addr[3]);

    if (!telescope_allowed_ip(ip)) {
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

    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
        "RPC telescope_execute is from IP address %s, command = '%s'",
        ip, p_command);

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    snprintf(command, COMMAND_MAX, "%s\n", p_command);
    socket_result = sock_send(sockfd_cmd, command);
    if ((socket_result = sock_recv(sockfd_cmd, recvbuf)) != -1) {
        p_result_str = strim(recvbuf);
        p_result = xmlrpc_build_value(p_env, "s", p_result_str);

        if ((strstr(p_command, "TSRA ") == p_command) && (p_result_str[0] == '1')) {
            bzero(&telescope_tsra, sizeof(telescope_tsra));
            bzero(&telescope_object, sizeof(telescope_object));
            strncpy(telescope_tsra, p_command+5, COMMAND_MAX);
            strncpy(telescope_object, "unknown", COMMAND_MAX);
            log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Save TSRA = '%s'", telescope_tsra);
        }
    }
    else {
        p_result = xmlrpc_build_value(p_env, "s", strerror(errno));
    }

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

cleanup:
    /* CLEANUP */

    return p_result;
}

static xmlrpc_value *telescope_set_coordinates(xmlrpc_env * const p_env,
                                       xmlrpc_value * const p_param_array,
                                       void * const p_server_info,
                                       void * const p_chan_info)
{
    const char *p_result_str;

    int socket_result;
    char command[COMMAND_MAX+1];
    char recvbuf[SOCK_RECVBUF_MAX];
    char ip[CFG_TYPE_STR_MAX+1];
    unsigned char *p_ip_addr;
    xmlrpc_value *p_result = NULL;
    TELESCOPE_COORDINATES_T coords;

    p_ip_addr = telescope_get_ip_addr(p_chan_info);
    snprintf(ip, CFG_TYPE_STR_MAX, "%u.%u.%u.%u",
        p_ip_addr[0], p_ip_addr[1], p_ip_addr[2], p_ip_addr[3]);

    if (!telescope_allowed_ip(ip)) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN,
            "%s is not allowed IP", ip);
        XMLRPC_FAIL(p_env, XMLRPC_INTERNAL_ERROR, "permission denied");
    }

    xmlrpc_decompose_value(p_env, p_param_array, "({s:s,s:s,s:s,s:i,*})",
        "ra", &coords.p_ra,
        "dec", &coords.p_dec,
        "object", &coords.p_object,
        "position", &coords.position);

    if (p_env->fault_occurred) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN,
            "xmlrpc_decompose_value(): %i: %s", p_env->fault_code, p_env->fault_string);
        XMLRPC_FAIL(p_env, XMLRPC_PARSE_ERROR, "xmlrpc_decompose_value() failed");
    }

    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
        "RPC telescope_set_coordinates is from IP address %s, "
        "ra = '%s', dec = '%s', object = '%s', position = '%i'",
        ip, coords.p_ra, coords.p_dec, coords.p_object, coords.position);

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    snprintf(command, COMMAND_MAX, "TSRA %s %s %i\n", coords.p_ra, coords.p_dec, coords.position);
    socket_result = sock_send(sockfd_cmd, command);
    if ((socket_result = sock_recv(sockfd_cmd, recvbuf)) != -1) {
        p_result_str = strim(recvbuf);
        p_result = xmlrpc_build_value(p_env, "s", p_result_str);

        if (p_result_str[0] == '1') {
            bzero(&telescope_tsra, sizeof(telescope_tsra));
            bzero(&telescope_object, sizeof(telescope_object));
            strncpy(telescope_tsra, &command[5], strlen(command) - 6);
            strncpy(telescope_object, coords.p_object, COMMAND_MAX);
            log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Save TSRA = '%s'", telescope_tsra);
        }
    }
    else {
        p_result = xmlrpc_build_value(p_env, "s", strerror(errno));
    }

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

cleanup:
    /* CLEANUP */

    return p_result;
}

static xmlrpc_value *telescope_info(xmlrpc_env * const p_env,
                                    xmlrpc_value * const p_param_array,
                                    void * const p_server_info,
                                    void * const p_chan_info)
{
    xmlrpc_value *p_result = NULL;

    /* LOCK */
    pthr_mutex_lock(&data_mutex);

    p_result = xmlrpc_build_value(p_env, "{s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s}", 
        "ut", glut2ut(info_data[INFO_GLUT_E]),
        "glst", info_data[INFO_GLST_E],
        "trrd", info_data[INFO_TRRD_E],
        "trhd", info_data[INFO_TRHD_E],
        "trgv", info_data[INFO_TRGV_E],
        "trus", info_data[INFO_TRUS_E],
        "dopo", info_data[INFO_DOPO_E],
        "trcs", info_data[INFO_TRCS_E],
        "fopo", info_data[INFO_FOPO_E],
        "tsra", telescope_tsra,
        "object", telescope_object);

    pthr_mutex_unlock(&data_mutex);
    /* UNLOCK */

    return p_result;
}

static void telescope_info_init()
{
    bzero(info_data, sizeof(info_data));
    bzero(info_cmds, sizeof(info_cmds));

    strncpy(info_cmds[INFO_GLST_E], "GLST\n", INFO_MAX); /* Global State */
    strncpy(info_cmds[INFO_TRRD_E], "TRRD\n", INFO_MAX); /* Star Coordinates */
    strncpy(info_cmds[INFO_TRHD_E], "TRHD\n", INFO_MAX); /* Source Coordinates */
    strncpy(info_cmds[INFO_TRGV_E], "TRGV\n", INFO_MAX); /* Telescope Read Guiding Value */
    strncpy(info_cmds[INFO_TRUS_E], "TRUS\n", INFO_MAX); /* Telescope Read User Speed */
    strncpy(info_cmds[INFO_DOPO_E], "DOPO\n", INFO_MAX); /* DOme POsition */
    strncpy(info_cmds[INFO_TRCS_E], "TRCS\n", INFO_MAX); /* Telescope Read Correction Set */
    strncpy(info_cmds[INFO_FOPO_E], "FOPO\n", INFO_MAX); /* FOcus POsition */
    strncpy(info_cmds[INFO_GLUT_E], "GLUT\n", INFO_MAX); /* UT */
}

static void telescope_cfg_get_string(char *p_dest, char *p_group_name, char *p_key)
{
    char *p_char;
    GError *p_error = NULL;

    p_char = g_key_file_get_string(telescope_cfg.p_key_file, p_group_name, p_key, &p_error);

    if (p_error != NULL) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
            "Get %s.%s failed: %s", p_group_name, p_key, p_error->message);
        telescope_exit(EXIT_FAILURE);
    }

    if (p_char != NULL) {
        strncpy(p_dest, p_char, CFG_TYPE_STR_MAX);
        log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
            "%s.%s = %s", p_group_name, p_key, p_char);
    }
    else {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
            "%s.%s = NULL", p_group_name, p_key);
        telescope_exit(EXIT_FAILURE);
    }
}

static void telescope_cfg_get_integer(int *p_dest, char *p_group_name, char *p_key)
{
    GError *p_error = NULL;

    *p_dest = g_key_file_get_integer(telescope_cfg.p_key_file, p_group_name, p_key, &p_error);

    if (p_error != NULL) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
            "Get %s.%s failed: %s", p_group_name, p_key, p_error->message);
        telescope_exit(EXIT_FAILURE);
    }

    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
        "%s.%s = %i", p_group_name, p_key, *p_dest);
}

static void telescope_cfg_get_allow_ips()
{
    int i;
    gchar **p_keys;
    gsize length;
    GError *p_error = NULL;
    TELESCOPE_IP_T *p_telescope_ip;
    TELESCOPE_IP_T *p_telescope_ip_new;

    p_keys = g_key_file_get_keys(telescope_cfg.p_key_file, "allow_ips", &length, &p_error);

    if (p_error != NULL) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
            "Get allow_ips failed: %s", p_error->message);
        telescope_exit(EXIT_FAILURE);
    }

    for (i = 0; i < length; ++i) {
        // TODO: append free
        if ((p_telescope_ip_new = malloc(sizeof(TELESCOPE_IP_T))) == NULL) {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR, "malloc() failed");
            telescope_exit(EXIT_FAILURE);

            exit(EXIT_FAILURE);
        }

        if (p_telescope_ip_first == NULL) {
            p_telescope_ip_first = p_telescope_ip_new;
        }
        else {
            p_telescope_ip->p_next = p_telescope_ip_new;
        }

        p_telescope_ip = p_telescope_ip_new;
        p_telescope_ip->p_next = NULL;

        strncpy(p_telescope_ip->name, p_keys[i], CFG_TYPE_STR_MAX);
        telescope_cfg_get_string(p_telescope_ip->ip, "allow_ips", p_keys[i]);
    }

    p_telescope_ip = p_telescope_ip_first;
    while (p_telescope_ip != NULL) {
        //log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
        //    "allowed name = %s, ip = %s", p_telescope_ip->name, p_telescope_ip->ip);
        p_telescope_ip = p_telescope_ip->p_next;
    }
}

static void telescope_load_cfg()
{
    GKeyFileFlags flags;
    GError *p_error = NULL;

    telescope_cfg.p_key_file = g_key_file_new();
    flags = G_KEY_FILE_NONE;

    if (!g_key_file_load_from_file(telescope_cfg.p_key_file, telescope_cfg.cfg_file, flags, &p_error)) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
            "Unable to load file %s", telescope_cfg.cfg_file);
        telescope_exit(EXIT_FAILURE);
    }

    telescope_cfg_get_string(telescope_cfg.file_pid, "telescoped", "file_pid");
    telescope_cfg_get_string(telescope_cfg.xmlrpc_log, "telescoped", "xmlrpc_log");
    telescope_cfg_get_string(telescope_cfg.ascol_ip, "telescoped", "ascol_ip");
    telescope_cfg_get_integer(&telescope_cfg.port, "telescoped", "port");
    telescope_cfg_get_integer(&telescope_cfg.ascol_loop_port, "telescoped", "ascol_loop_port");
    telescope_cfg_get_integer(&telescope_cfg.ascol_cmd_port, "telescoped", "ascol_cmd_port");
    telescope_cfg_get_allow_ips();

    g_key_file_free(telescope_cfg.p_key_file);
    telescope_cfg.p_key_file = NULL;
}

static void telescope_create_pid()
{
    FILE *fw;

    if ((fw = fopen(telescope_cfg.file_pid, "w")) == NULL) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
            "telescope_create_pid() => fopen(): %i: %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    else {
        if (fprintf(fw, "%i\n", getpid()) <= 0) {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "telescope_create_pid() => fprintf(): %i: %s", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }

        if (fclose(fw) == EOF) {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "telescope_create_pid() => fclose(): %i: %s", errno, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

static void telescope_daemonize(void)
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

    struct xmlrpc_method_info3 const telescope_execute_MI = {
        .methodName     = "telescope_execute",
        .methodFunction = &telescope_execute,
    };

    struct xmlrpc_method_info3 const telescope_set_coordinates_MI = {
        .methodName     = "telescope_set_coordinates",
        .methodFunction = &telescope_set_coordinates,
    };

    struct xmlrpc_method_info3 const telescope_info_MI = {
        .methodName     = "telescope_info",
        .methodFunction = &telescope_info,
    };

    if ((p_telescoped_dir = getenv("TELESCOPED_DIR")) == NULL) {
        fprintf(stderr, "The TELESCOPED_DIR environment variable is not set.");
        exit(EXIT_FAILURE);
    }

    bzero(&telescope_cfg, sizeof(TELESCOPE_CFG_T));
    bzero(&telescope_tsra, sizeof(telescope_tsra));
    bzero(&telescope_object, sizeof(telescope_object));

    strncpy(telescope_object, "unknown", COMMAND_MAX);

    telescope_cfg.p_key_file = NULL;
    snprintf(telescope_cfg.cfg_file, CFG_TYPE_STR_MAX, "%s/etc/telescoped.cfg", p_telescoped_dir);

    switch (argc) {
        case 2:
            if (!strcmp(argv[1], "--version")) {
                telescope_version(argv[0]);
            }
            else {
                telescope_help(argv[0]);
            }
            exit(EXIT_SUCCESS);
            break;

        default: break;
    }

    telescope_daemonize();
    telescope_info_init();

    if (log4c_init()) {
        perror("log4c_init() failed");
        exit(EXIT_FAILURE);
    }

    p_logcat = log4c_category_get("telescoped");
    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Starting telescoped r%s %s",
                       SVN_REV, MAKE_DATE_TIME);

    telescope_load_cfg();
    telescope_create_pid();

    (void)signal(SIGTERM, telescope_signal);
    (void)signal(SIGPIPE, SIG_IGN);       /* send() */

    if (pthread_mutex_init(&global_mutex, NULL) != 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR, "pthread_mutex_init(): %i: %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&data_mutex, NULL) != 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR, "pthread_mutex_init(): %i: %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&telescope_loop_pthread, NULL, telescope_loop, NULL) != 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR, "pthread_create(): %i: %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&telescope_cmd_pthread, NULL, telescope_cmd, NULL) != 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR, "pthread_create(): %i: %s", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
 
    xmlrpc_env_init(&env);

    registryP = xmlrpc_registry_new(&env);

    xmlrpc_registry_add_method3(&env, registryP, &telescope_execute_MI);
    xmlrpc_registry_add_method3(&env, registryP, &telescope_set_coordinates_MI);
    xmlrpc_registry_add_method3(&env, registryP, &telescope_info_MI);

    serverparm.config_file_name = NULL;
    serverparm.registryP = registryP;
    serverparm.port_number = telescope_cfg.port;

    if (*telescope_cfg.xmlrpc_log != '\0') {
        serverparm.log_file_name = telescope_cfg.xmlrpc_log;
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
