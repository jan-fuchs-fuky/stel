/*
 *
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$
 *   $URL$
 *
 *   Copyright (C) 2008-2013 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
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
#include <stdarg.h>
#include <dlfcn.h>
#include <libgen.h>
#include <fitsio.h>
#include <log4c.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/server.h>
#include <xmlrpc-c/server_abyss.h>

#include "modules.h"
#include "exposed.h"
#include "socket.h"
#include "thread.h"
#include "header.h"
#include "fce.h"
#include "cfg.h"
#include "telescope.h"
#include "spectrograph.h"

log4c_category_t *p_logcat = NULL;

extern PESO_HEADER_T peso_header[];

static char exposed_log[CIRCULAR_BUFFER_SIZE][EXPOSED_LOG_MAX + 1];
static int exposed_log_index = 0;
static int exposed_exit = 0;
static sem_t expose_sem;
static sem_t service_sem;
static pthread_mutex_t global_mutex;
static MOD_CCD_T mod_ccd;
static PESO_T *p_peso;
static char exposed_ccd_name[EXPOSED_CCD_NAME_MAX + 1];
static char *p_cmd_begin;
static char *p_cmd_end;
static EXPOSED_ALLOCATE_T exposed_allocate;

static void daemon_version(void)
{
    printf("%s r%s %s\n", APP_NAME, SVN_REV, MAKE_DATE_TIME);
    printf("Written by Jan Fuchs <fuky@sunstel.asu.cas.cz>\n\n");
    printf("Copyright (C) 2008-2012 Free Software Foundation, Inc.\n");
    printf(
            "This is free software; see the source for copying conditions.  There is NO\n");
    printf(
            "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}

static void daemon_help(void)
{
    printf("%s -c XML_CONFIGURATION_FILE\n\n", APP_NAME);
    printf("-c, --config     set path to exposed configuration file\n");
    printf("-h, --help       display this help and exit\n");
    printf("-v, --version    output version information and exit\n\n");
    printf("Configuration file init script exposed is /etc/default/%s.\n",
            APP_NAME);
}

static void daemon_exit(int status)
{
    exposed_exit = 1;

    if (exposed_allocate.expose_sem)
    {
        sem_destroy(&expose_sem);
    }

    if (exposed_allocate.service_sem)
    {
        sem_destroy(&service_sem);
    }

    if (exposed_allocate.mod_ccd)
    {
        mod_ccd_uninit();
    }

    if (exposed_allocate.global_mutex)
    {
        pthread_mutex_destroy(&global_mutex);
    }

    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "exit");

    if (remove(exposed_cfg.file_pid) == -1)
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN,
                "Warning: File %s remove failed: %i: %s", exposed_cfg.file_pid,
                errno, strerror(errno));
    }

    exit(status);
}

__attribute__((format(printf,2,3)))
static void append_log(int priority, const char *p_fmt, ...)
{
    time_t t = time(NULL);
    struct tm *p_tm = gmtime(&t);
    char human_time[64];
    va_list ap;
    char msg[CCD_MSG_MAX + 1];

    va_start(ap, p_fmt);

    memset(msg, '\0', CCD_MSG_MAX + 1);
    vsnprintf(msg, CCD_MSG_MAX, p_fmt, ap);

    va_end(ap);

    if (p_tm != NULL)
    {
        if (strftime(human_time, sizeof(human_time), "%d.%m. %H:%M:%S ", p_tm)
                == 0)
        {
            strcpy(human_time, "NULL ");
            log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN,
                    "Warning: strftime(): %i: %s", errno, strerror(errno));
        }
    }
    else
    {
        strcpy(human_time, "NULL ");
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN,
                "Warning: gmtime(): %i: %s", errno, strerror(errno));
    }

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    memset(exposed_log[exposed_log_index], '\0', EXPOSED_LOG_MAX + 1);
    snprintf(exposed_log[exposed_log_index++], EXPOSED_LOG_MAX, "%s%s\n",
            human_time, msg);
    if (exposed_log_index >= CIRCULAR_BUFFER_SIZE)
    {
        exposed_log_index = 0;
    }

    log4c_category_log(p_logcat, priority, "%s", msg);

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */
}

__attribute__((format(printf,2,3)))
static int save_fits_error(int fits_status, const char *p_fmt, ...)
{
    va_list ap;
    char fits_error[FLEN_ERRMSG];
    int len;

    va_start(ap, p_fmt);

    memset(p_peso->msg, '\0', CCD_MSG_MAX + 1);
    vsnprintf(p_peso->msg, CCD_MSG_MAX, p_fmt, ap);
    len = strlen(p_peso->msg);
    fits_get_errstatus(fits_status, fits_error);
    snprintf(p_peso->msg + len, CCD_MSG_MAX - len, " %s", fits_error);

    va_end(ap);

    append_log(LOG4C_PRIORITY_ERROR, p_peso->msg);

    return 0;
}

__attribute__((format(printf,2,3)))
static int save_sys_error(int priority, const char *p_fmt, ...)
{
    va_list ap;
    int len;

    va_start(ap, p_fmt);

    memset(p_peso->msg, '\0', CCD_MSG_MAX + 1);
    vsnprintf(p_peso->msg, CCD_MSG_MAX, p_fmt, ap);
    len = strlen(p_peso->msg);
    snprintf(p_peso->msg + len, CCD_MSG_MAX - len, " %i: %s", errno,
            strerror(errno));

    va_end(ap);

    append_log(priority, p_peso->msg);

    return 0;
}

static void daemon_signal(int sig)
{
    switch (sig)
    {
    case SIGUSR1:
    case SIGHUP:
    case SIGINT:
        /* expose readout */
        log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
                "be received signal SIGUSR1 | SIGHUP | SIGINT");
        p_peso->readout = 1;
        break;

    case SIGTERM:
        /* expose abort */
        log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
                "be received signal SIGTERM");
        p_peso->abort = 1;
        exposed_exit = 1;
        sleep(3);
        exit(EXIT_SUCCESS);

    default:
        break;
    }
}

static int save_imagetype(char *p_imagetype)
{
    if (!strcasecmp(p_imagetype, "flat"))
    {
        p_peso->imgtype = CCD_IMGTYPE_FLAT_E;
        p_cmd_begin = exposed_cfg.cmd_begin.flat;
        p_cmd_end = exposed_cfg.cmd_end.flat;
        return 0;
    }
    else if (!strcasecmp(p_imagetype, "comp"))
    {
        p_peso->imgtype = CCD_IMGTYPE_COMP_E;
        p_cmd_begin = exposed_cfg.cmd_begin.comp;
        p_cmd_end = exposed_cfg.cmd_end.comp;
        return 0;
    }
    else if (!strcasecmp(p_imagetype, "zero"))
    {
        p_peso->imgtype = CCD_IMGTYPE_ZERO_E;
        p_cmd_begin = NULL;
        p_cmd_end = NULL;
        return 0;
    }
    else if (!strcasecmp(p_imagetype, "dark"))
    {
        p_peso->imgtype = CCD_IMGTYPE_DARK_E;
        p_cmd_begin = NULL;
        p_cmd_end = NULL;
        return 0;
    }
    else if (!strcasecmp(p_imagetype, "object"))
    {
        p_peso->imgtype = CCD_IMGTYPE_TARGET_E;
        p_cmd_begin = exposed_cfg.cmd_begin.target;
        p_cmd_end = exposed_cfg.cmd_end.target;
        return 0;
    }

    return -1;
}

static long save_fits_header(fitsfile *p_fits)
{
    int i;
    int fits_status = 0;
    int bscale = 1;
    int bzero = 32768;
    int value_int;
    float value_float;
    double value_double;
    char key[PHDR_KEY_MAX + 1];
    char *p_value;
    char *p_comment;
    long naxis = 2;
    // TODO: nacitat z konfiguracniho souboru
    //long naxes[2] = { 2048, 2048 };
    //long naxes[2] = { 2720, 512 };
    long naxes[2] = { exposed_cfg.ccd.x2, exposed_cfg.ccd.y2 };

    if (fits_create_img(p_fits, USHORT_IMG, naxis, naxes, &fits_status))
    {
        save_fits_error(fits_status,
                "Error: fits_create_img(USHORT_IMG, %li, [%li, %li]):", naxis,
                naxes[0], naxes[1]);
        return -1;
    }
    if (fits_update_key(p_fits, TINT, "BZERO", &bzero, "", &fits_status))
    {
        save_fits_error(fits_status, "Error: fits_update_key(bzero = %i):",
                bzero);
        return -1;
    }
    if (fits_update_key(p_fits, TINT, "BSCALE", &bscale,
            "REAL=TAPE*BSCALE+BZERO", &fits_status))
    {
        save_fits_error(fits_status, "Error: fits_update_key(bscale = %i):",
                bscale);
        return -1;
    }

    for (i = 0; i < PHDR_INDEX_MAX_E; ++i)
    {
        if (peso_header[i].value[0] == '\0')
        {
            continue;
        }

        memset(key, '\0', PHDR_KEY_MAX + 1);
        strncpy(key, peso_header[i].key, PHDR_KEY_MAX);
        p_value = peso_header[i].value;
        p_comment = peso_header[i].comment;

        switch (peso_header[i].type)
        {
        case PHDR_TYPE_INT_E:
            value_int = atoi(p_value);
            if (fits_update_key(p_fits, TINT, key, &value_int, p_comment,
                    &fits_status))
            {
                save_fits_error(fits_status,
                        "Error: fits_update_key(TLONG, %s, %i, %s):", key,
                        value_int, p_comment);
                return -1;
            }
            break;

        case PHDR_TYPE_FLOAT_E:
            value_float = atof(p_value);
            if (fits_update_key(p_fits, TFLOAT, key, &value_float, p_comment,
                    &fits_status))
            {
                save_fits_error(fits_status,
                        "Error: fits_update_key(TFLOAT, %s, %0.2f, %s):", key,
                        value_float, p_comment);
                return -1;
            }
            break;

        case PHDR_TYPE_DOUBLE_E:
            value_double = atof(p_value);
            if (fits_update_key(p_fits, TDOUBLE, key, &value_double, p_comment,
                    &fits_status))
            {
                save_fits_error(fits_status,
                        "Error: fits_update_key(TDOUBLE, %s, %0.2f, %s):", key,
                        value_double, p_comment);
                return -1;
            }
            break;

        case PHDR_TYPE_STR_E:
            if (fits_update_key(p_fits, TSTRING, key, p_value, p_comment,
                    &fits_status))
            {
                save_fits_error(fits_status,
                        "Error: fits_update_key(TSTRING, %s, %s, %s):", key,
                        p_value, p_comment);
                return -1;
            }
            break;

        default:
            break;
        }
    }

    if (fits_write_date(p_fits, &fits_status))
    {
        save_fits_error(fits_status, "Error: fits_write_date():");
        return -1;
    }

    /* TODO: pridavat komentare */
    //if (fits_write_comment(p_fits, comment, &fits_status)) ccd_log_fits(LOG4C_PRIORITY_WARN, fits_status);
    return 0;
}

static char *exposed_state2str(int state)
{
    switch (state)
    {
    case CCD_STATE_READY_E:
        return "ready";

    case CCD_STATE_PREPARE_EXPOSE_E:
        return "prepare expose";

    case CCD_STATE_EXPOSE_E:
        return "expose";

    case CCD_STATE_FINISH_EXPOSE_E:
        return "finish expose";

    case CCD_STATE_READOUT_E:
        return "readout";

    default:
        return "unknown";
    }
}

static xmlrpc_value *cmd_set_key(xmlrpc_env *p_env, char *p_key, char *p_value,
        char *p_comment)
{
    int i;
    char result[RESULT_MAX + 1];

    // TODO: check allow chars - fce_isallow_fitshdr_c()
    for (i = 0; i < PHDR_INDEX_MAX_E; ++i)
    {
        if (!strcmp(peso_header[i].key, p_key))
        {
            if (p_value[0] == '\0')
            {
                strncpy(result, "-ERR unknown value", RESULT_MAX);
                goto finish;
            }

            if ((peso_header[i].index == PHDR_IMAGETYP_E)
                    && (save_imagetype(p_value) == -1))
            {
                snprintf(result, RESULT_MAX,
                        "-ERR %s is not supported imagetype", p_value);
                goto finish;
            }

            memset(peso_header[i].value, 0, PHDR_VALUE_MAX + 1);
            strncpy(peso_header[i].value, p_value, PHDR_VALUE_MAX);

            if (p_comment[0] != '\0')
            {
                memset(peso_header[i].comment, 0, PHDR_COMMENT_MAX + 1);
                strncpy(peso_header[i].comment, p_comment, PHDR_COMMENT_MAX);
            }

            snprintf(result, RESULT_MAX, "+OK %s = %s / %s", p_key, p_value,
                    p_comment);
            goto finish;
        }
    }

    snprintf(result, RESULT_MAX, "-ERR %s is unknown key", p_key);

    finish: return xmlrpc_build_value(p_env, "s", result);
}

static xmlrpc_value *cmd_get_key(xmlrpc_env *p_env, char *p_key)
{
    int i;
    char result[RESULT_MAX + 1];

    for (i = 0; i < PHDR_INDEX_MAX_E; ++i)
    {
        if (!strcmp(peso_header[i].key, p_key))
        {
            snprintf(result, RESULT_MAX, "+OK %s = %s / %s", peso_header[i].key,
                    peso_header[i].value, peso_header[i].comment);
            goto finish;
        }
    }

    snprintf(result, RESULT_MAX, "-ERR %s is unknown key", p_key);

    finish: return xmlrpc_build_value(p_env, "s", result);
}

// TODO: vracet strukturu
static xmlrpc_value *get_all_key(xmlrpc_env *p_env)
{
    int i;
    int len;
    char result[RESULT_MAX + 1];

    for (i = 0; i < PHDR_INDEX_MAX_E; ++i)
    {
        len = strlen(result);
        snprintf(result + len, RESULT_MAX - len, "%s = %s / %s\n",
                peso_header[i].key, peso_header[i].value,
                peso_header[i].comment);
    }

    strncat(result, "+OK", RESULT_MAX - strlen(result));

    return xmlrpc_build_value(p_env, "s", result);
}

static int ccd_state(char *p_result, int result_len)
{
    switch (p_peso->state)
    {
    case CCD_STATE_READY_E:
        snprintf(p_result, result_len, "+OK %i ccd is ready '%s'",
                p_peso->state, peso_header[PHDR_FILENAME_E].value);
        break;

    case CCD_STATE_PREPARE_EXPOSE_E:
        snprintf(p_result, result_len, "+OK %i preparing expose '%s'",
                p_peso->state, peso_header[PHDR_FILENAME_E].value);
        break;

    case CCD_STATE_EXPOSE_E:
        snprintf(p_result, result_len, "+OK %i exposing %i %i '%s'",
                p_peso->state, p_peso->elapsed_time, p_peso->exptime,
                peso_header[PHDR_FILENAME_E].value);
        break;

    case CCD_STATE_FINISH_EXPOSE_E:
        snprintf(p_result, result_len, "+OK %i finishing expose '%s'",
                p_peso->state, peso_header[PHDR_FILENAME_E].value);
        break;

    case CCD_STATE_READOUT_E:
        snprintf(p_result, result_len, "+OK %i reading out CCD %i %i '%s'",
                p_peso->state, p_peso->elapsed_time, p_peso->readout_time,
                peso_header[PHDR_FILENAME_E].value);
        break;

    default:
        strncpy(p_result, "-ERR", result_len);
        break;
    }

    return 0;
}

static int get_message(char *p_arg, char *p_result, int result_len)
{
    int i;
    int len = 0;
    //int flag_new = 0;
    int start = 0;

    if (p_arg != NULL)
    {
        ++p_arg;
        if (!strcmp(p_arg, "NEW"))
        {
            //flag_new = 1;
        }
        else
        {
            start = exposed_log_index - atoi(p_arg);
        }
    }

    if (start < 0)
    {
        start = 0;
    }

    for (i = start; i < exposed_log_index; ++i)
    {
        /* TODO: dokoncit flag_new = 1 */
        //if ((!flag_new) ||) {
        strncat(p_result, exposed_log[i], result_len - len);
        len += strlen(exposed_log[i]);
        //}
    }

    strncat(p_result, "+OK", result_len - len);
    return 0;
}

static xmlrpc_value *cmd_get(xmlrpc_env *p_env, char *p_variable)
{
    char result[RESULT_MAX + 1];

    if (!strcmp(p_variable, "PATH"))
    {
        snprintf(result, RESULT_MAX, "+OK PATH = %s", p_peso->path);
    }
    else if (!strcmp(p_variable, "ARCHIVEPATH"))
    {
        snprintf(result, RESULT_MAX, "+OK ARCHIVEPATH = %s",
                p_peso->archive_path);
    }
    else if (!strcmp(p_variable, "PATHS"))
    {
        snprintf(result, RESULT_MAX, "+OK PATHS = %s",
                exposed_cfg.output_paths);
    }
    else if (!strcmp(p_variable, "ARCHIVEPATHS"))
    {
        snprintf(result, RESULT_MAX, "+OK ARCHIVEPATHS = %s",
                exposed_cfg.archive_paths);
    }
    else if (!strcmp(p_variable, "ARCHIVE"))
    {
        snprintf(result, RESULT_MAX, "+OK ARCHIVE = %i", p_peso->archive);
    }
    else if (!strcmp(p_variable, "CCDTEMP"))
    {
        snprintf(result, RESULT_MAX, "+OK CCDTEMP = %0.1f",
                p_peso->actual_temp);
    }
    else if (!strcmp(p_variable, "CCDSTATE"))
    {
        ccd_state(result, RESULT_MAX);
    }
    else if (strstr(p_variable, "MESSAGE") == p_variable)
    {
        get_message(strchr(p_variable, ' '), result, RESULT_MAX);
    }
    else if (!strcmp(p_variable, "INSTRUMENT"))
    {
        snprintf(result, RESULT_MAX, "+OK INSTRUMENT = %s",
                exposed_cfg.instrument);
    }
    else if (!strcmp(p_variable, "READOUT_SPEED"))
    {
        snprintf(result, RESULT_MAX, "+OK READOUT_SPEED = %s",
                mod_ccd.get_readout_speed());
    }
    else if (!strcmp(p_variable, "READOUT_SPEEDS"))
    {
        snprintf(result, RESULT_MAX, "+OK READOUT_SPEEDS = %s",
                mod_ccd.get_readout_speeds());
    }
    else if (!strcmp(p_variable, "GAIN"))
    {
        snprintf(result, RESULT_MAX, "+OK GAIN = %s",
                mod_ccd.get_gain());
    }
    else if (!strcmp(p_variable, "GAINS"))
    {
        snprintf(result, RESULT_MAX, "+OK GAINS = %s",
                mod_ccd.get_gains());
    }
    else
    {
        snprintf(result, RESULT_MAX, "-ERR %s is unknown variable", p_variable);
    }

    return xmlrpc_build_value(p_env, "s", result);
}

static xmlrpc_value *cmd_set(xmlrpc_env *p_env, char *p_variable, char *p_value)
{
    char result[RESULT_MAX + 1];
    double temp;

    /* TODO: check path len */
    if (!strcmp(p_variable, "PATH"))
    {
        if (fce_isdir_rwxu(p_value))
        {
            memset(p_peso->path, 0, PESO_PATH_MAX + 1);
            strncpy(p_peso->path, p_value, PESO_PATH_MAX);
            strncpy(result, "+OK", RESULT_MAX);
        }
        else
        {
            snprintf(result, RESULT_MAX, "-ERR %s is forbidden path", p_value);
        }
    }
    else if (!strcmp(p_variable, "ARCHIVEPATH"))
    {
        if (fce_isdir_rwxu(p_value))
        {
            memset(p_peso->archive_path, 0, PESO_PATH_MAX + 1);
            strncpy(p_peso->archive_path, p_value, PESO_PATH_MAX);
            strncpy(result, "+OK", RESULT_MAX);
        }
        else
        {
            snprintf(result, RESULT_MAX, "-ERR %s is forbidden path", p_value);
        }
    }
    else if (!strcmp(p_variable, "ARCHIVE"))
    {
        if (*p_value == '0')
        {
            p_peso->archive = 0;
        }
        else if (*p_value == '1')
        {
            p_peso->archive = 1;
        }
        else
        {
            snprintf(result, RESULT_MAX, "-ERR %s is unknown value", p_value);
            goto finish;
        }

        strncpy(result, "+OK", RESULT_MAX);
    }
    // TODO: implementovat
    else if (!strcmp(p_variable, "CCDTEMP"))
    {
        temp = atof(p_value);

        if ((temp > -90.0) || (temp < -150.0))
        {
            snprintf(result, RESULT_MAX,
                    "-ERR temperature %0.2f is out of range (-150.0, -90.0)",
                    temp);
            goto finish;
        }
        else if (mod_ccd.set_temp(temp) == -1)
        {
            snprintf(result, RESULT_MAX, "-ERR %s", p_peso->msg);
            goto finish;
        }

        snprintf(result, RESULT_MAX, "+OK");
    }
    else if (!strcmp(p_variable, "READOUT_SPEED"))
    {
        if (mod_ccd.set_readout_speed(p_value) == -1)
        {
            snprintf(result, RESULT_MAX, "-ERR %s is unknown value", p_value);
            goto finish;
        }

        snprintf(result, RESULT_MAX, "+OK");
    }
    else if (!strcmp(p_variable, "GAIN"))
    {
        if (mod_ccd.set_gain(p_value) == -1)
        {
            snprintf(result, RESULT_MAX, "-ERR %s is unknown value", p_value);
            goto finish;
        }

        snprintf(result, RESULT_MAX, "+OK");
    }
    else
    {
        snprintf(result, RESULT_MAX, "-ERR %s is unknown variable", p_variable);
        goto finish;
    }

    finish: return xmlrpc_build_value(p_env, "s", result);
}

static xmlrpc_value *cmd_expose(xmlrpc_env *p_env, int exptime, int expcount, int expmeter)
{
    char result[RESULT_MAX + 1];

    switch (p_peso->imgtype)
    {
    case CCD_IMGTYPE_FLAT_E:
    case CCD_IMGTYPE_COMP_E:
    case CCD_IMGTYPE_TARGET_E:
        p_peso->shutter = 1;
        break;

    case CCD_IMGTYPE_DARK_E:
    case CCD_IMGTYPE_ZERO_E:
        p_peso->shutter = 0;
        break;

    default:
        snprintf(result, RESULT_MAX, "-ERR must execute SETKEY IMAGETYP");
        xmlrpc_env_set_fault_formatted(p_env, XMLRPC_INTERNAL_ERROR, "%s",
                result);
        goto finish;
    }

    if (exptime == -1) {
        p_peso->exptime = CCD_EXPTIME_MAX;
    }
    else {
        p_peso->exptime = exptime;
    }

    p_peso->expcount = expcount;
    p_peso->expmeter = expmeter;
    p_peso->expnum = 0;

    if (p_peso->expcount <= 0)
    {
        p_peso->expcount = 1;
    }

    snprintf(result, RESULT_MAX, "+OK EXPOSE %i %i %i", p_peso->exptime,
            p_peso->expcount, p_peso->expmeter);

    pthr_sem_post(&expose_sem);

    finish: return xmlrpc_build_value(p_env, "s", result);
}

/* TODO: odstranit */
static xmlrpc_value *cmd_addtime(xmlrpc_env *p_env, int addtime)
{
    char result[RESULT_MAX + 1];

    p_peso->addtime = addtime;
    strncpy(result, "+OK", RESULT_MAX);

    return xmlrpc_build_value(p_env, "s", result);
}

static xmlrpc_value *cmd_exptime_update(xmlrpc_env *p_env, int exptime)
{
    char result[RESULT_MAX + 1];

    p_peso->exptime_update = exptime;
    strncpy(result, "+OK", RESULT_MAX);

    return xmlrpc_build_value(p_env, "s", result);
}

static xmlrpc_value *cmd_expmeter_update(xmlrpc_env *p_env, int expmeter)
{
    char result[RESULT_MAX + 1];

    p_peso->expmeter_update = expmeter;
    strncpy(result, "+OK", RESULT_MAX);

    return xmlrpc_build_value(p_env, "s", result);
}

/*
 * Examples:
 *
 *     "12:30:36" => "12.51"
 *     "-12:30:36" => "-12.51"
 *     "-12:30:36\n" => "-12.51"
 *
 */
static int expose_dms2number(char *p_input, char *p_output, int output_len)
{
    char *p_number;
    char *p_save = NULL;
    int sign;
    float degree;
    float minute;
    float second;

    if ((p_number = strtok_r(p_input, ":", &p_save)) == NULL) {
        return -1;
    }
    degree = atof(p_number);

    if ((p_number = strtok_r(NULL, ":", &p_save)) == NULL) {
        return -1;
    }
    minute = atof(p_number);

    if ((p_number = strtok_r(NULL, "\n", &p_save)) == NULL) {
        return -1;
    }
    second = atof(p_number);

    sign = (degree >= 0) ? 1 : -1;
    memset(p_output, 0, output_len);
    snprintf(p_output, output_len, "%f", (((sign * degree) + (minute / 60.0) + (second / 3600.0)) * sign));

    return 0;
}

static void fit_save_tle_hdr(void)
{
    TLE_INFO_T tle_info;
    char value[EXPOSED_STR_MAX+1];

    if (tle_telescope_info(&tle_info) == -1)
    {
        // TODO: zalogovat
    }
    else
    {
        // TLE-TRCS - Correction Set
        snprintf(peso_header[PHDR_TLE_TRCS_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.trcs);

        // TLE-TRGV - Guiding Value
        snprintf(peso_header[PHDR_TLE_TRGV_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.trgv);

        // TLE-TRHD - Hour and Declination Axis
        snprintf(peso_header[PHDR_TLE_TRHD_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.trhd);

        // TLE-TRRD - Right ascension and Declination
        snprintf(peso_header[PHDR_TLE_TRRD_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.trrd);

        // TLE-TRUS - User Speed
        snprintf(peso_header[PHDR_TLE_TRUS_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.trus);

        // AIRHUMEX
        snprintf(peso_header[PHDR_AIRHUMEX_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.airhumex);

        // AIRPRESS
        snprintf(peso_header[PHDR_AIRPRESS_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.airpress);

        // DOMEAZ
        snprintf(peso_header[PHDR_DOMEAZ_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.domeaz);

        // DOMETEMP
        snprintf(peso_header[PHDR_DOMETEMP_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.dometemp);

        // OUTTEMP
        snprintf(peso_header[PHDR_OUTTEMP_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.outtemp);

        // TELFOCUS
        snprintf(peso_header[PHDR_TELFOCUS_E].value, PHDR_VALUE_MAX, "%.2f",
                tle_info.fopo);

        // DEC
        snprintf(peso_header[PHDR_DEC_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.dec);

        expose_dms2number(tle_info.dec, value, EXPOSED_STR_MAX);
        snprintf(peso_header[PHDR_DEC_E].comment, PHDR_COMMENT_MAX, "%s",
                value);

        // RA
        snprintf(peso_header[PHDR_RA_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.ra);

        // ST
        snprintf(peso_header[PHDR_ST_E].value, PHDR_VALUE_MAX, "%s",
                tle_info.st);

        expose_dms2number(tle_info.ra, value, EXPOSED_STR_MAX);
        snprintf(peso_header[PHDR_RA_E].comment, PHDR_COMMENT_MAX, "%s",
                value);

        // TM-DIFF
        time_t ut_time = time(NULL);
        time_t tle_ut_time = 0;
        struct tm tle_tm;

        setenv("TZ", "UTC", 1);
        if (strptime(tle_info.ut, "%Y-%m-%d %H:%M:%S", &tle_tm) != NULL) {
            tle_ut_time = mktime(&tle_tm);
        }

        snprintf(peso_header[PHDR_TM_DIFF_E].value, PHDR_VALUE_MAX, "%jd",
                (intmax_t)(tle_ut_time - ut_time));

        snprintf(peso_header[PHDR_TM_DIFF_E].comment, PHDR_COMMENT_MAX, "T%jd - P%jd",
                (intmax_t)tle_ut_time, (intmax_t)ut_time);
    }
}

static void sgh_collimator2human(SGH_INFO_T *p_sgh_info, char *p_value,
        int value_len)
{
    int collimator = p_sgh_info->collimator;

    if (!strcmp(exposed_cfg.instrument, "OES")) {
        collimator = p_sgh_info->oes_collimator;
    }

    switch (collimator)
    {
    case 1:
        strncpy(p_value, "open", value_len);
        break;

    case 2:
        strncpy(p_value, "close", value_len);
        break;

    case 3:
        strncpy(p_value, "left", value_len);
        break;

    case 4:
        strncpy(p_value, "right", value_len);
        break;

    default:
        strncpy(p_value, "unknown", value_len);
        break;
    }
}

static void sgh_cplate2human(int value, char *p_value, int value_len)
{
    switch (value)
    {
    case 1:
        strncpy(p_value, "in", value_len);
        break;

    case 2:
        strncpy(p_value, "out", value_len);
        break;

    default:
        strncpy(p_value, "unknown", value_len);
        break;
    }
}

static void sgh_mco2human(SGH_INFO_T *p_sgh_info, char *p_value, int value_len)
{
    switch (p_sgh_info->coude_oes)
    {
    case 1:
        strncpy(p_value, "coude", value_len);
        break;

    case 2:
        strncpy(p_value, "oes", value_len);
        break;

    default:
        strncpy(p_value, "unknown", value_len);
        break;
    }
}

static void sgh_msc2human(SGH_INFO_T *p_sgh_info, char *p_value, int value_len)
{
    switch (p_sgh_info->star_calib)
    {
    case 1:
        strncpy(p_value, "star", value_len);
        break;

    case 2:
        strncpy(p_value, "calibration", value_len);
        break;

    default:
        strncpy(p_value, "unknown", value_len);
        break;
    }
}

static void fit_save_sgh_hdr(void)
{
    SGH_INFO_T sgh_info;
    char value[EXPOSED_STR_MAX+1];
    int *p_spectemp = NULL;
    int *p_camfocus = NULL;
    float spectemp_human;

    if (sgh_spectrograph_info(&sgh_info) == -1)
    {
        // TODO: zalogovat
    }
    else
    {
        if (!strcmp(exposed_cfg.instrument, "OES")) {
            // SGH-OIC - OES Iodine cell
            snprintf(peso_header[PHDR_SGH_OIC_E].value, PHDR_VALUE_MAX, "%i",
                    sgh_info.oes_iodine_cell);
        }
        else {
            // SGH-CPA - Correction plate 700
            sgh_cplate2human(sgh_info.correction_plate_700, value, EXPOSED_STR_MAX);
            snprintf(peso_header[PHDR_SGH_CPA_E].value, PHDR_VALUE_MAX, "%s",
                    value);

            // SGH-CPB - Correction plate 400
            sgh_cplate2human(sgh_info.correction_plate_400, value, EXPOSED_STR_MAX);
            snprintf(peso_header[PHDR_SGH_CPB_E].value, PHDR_VALUE_MAX, "%s",
                    value);

            // GRATANG
            snprintf(peso_header[PHDR_GRATANG_E].value, PHDR_VALUE_MAX, "%4.2f",
                    sgh_gratpos2gratang(sgh_info.grating_position));

            sgh_gratpos2gratang_str(sgh_info.grating_position, value,
                    EXPOSED_STR_MAX);
            snprintf(peso_header[PHDR_GRATANG_E].comment, PHDR_COMMENT_MAX, "%s",
                    value);

            // GRATPOS
            snprintf(peso_header[PHDR_GRATPOS_E].value, PHDR_VALUE_MAX, "%i",
                    sgh_info.grating_position);

            // SPECFILT
            snprintf(peso_header[PHDR_SPECFILT_E].value, PHDR_VALUE_MAX, "%i",
                    sgh_info.spectral_filter);

            // DICHMIR
            snprintf(peso_header[PHDR_DICHMIR_E].value, PHDR_VALUE_MAX, "%i",
                    sgh_info.dichroic_mirror);
        }

        // COLIMAT - Collimator mask status
        sgh_collimator2human(&sgh_info, value, EXPOSED_STR_MAX);
        snprintf(peso_header[PHDR_COLIMAT_E].value, PHDR_VALUE_MAX, "%s",
                value);

        // SGH-MCO - Mirror Coude Oes
        sgh_mco2human(&sgh_info, value, EXPOSED_STR_MAX);
        snprintf(peso_header[PHDR_SGH_MCO_E].value, PHDR_VALUE_MAX, "%s",
                value);

        // SGH-MSC - Mirror Star Calibration
        sgh_msc2human(&sgh_info, value, EXPOSED_STR_MAX);
        snprintf(peso_header[PHDR_SGH_MSC_E].value, PHDR_VALUE_MAX, "%s",
                value);

        if (!strcmp(exposed_cfg.instrument, "CCD700")) {
            p_spectemp = &sgh_info.coude_temp;
            p_camfocus = &sgh_info.focus_700;
        }
        else if (!strcmp(exposed_cfg.instrument, "CCD400")) {
            p_spectemp = &sgh_info.coude_temp;
            p_camfocus = &sgh_info.focus_1400;
        }
        else if (!strcmp(exposed_cfg.instrument, "OES")) {
            p_spectemp = &sgh_info.oes_temp;
            p_camfocus = &sgh_info.focus_oes;
        }

        // SPECTEMP
        if (p_spectemp != NULL)
        {
            spectemp_human = sgh_temp2human(*p_spectemp);
            snprintf(peso_header[PHDR_SPECTEMP_E].value, PHDR_VALUE_MAX, "%.1f",
                    spectemp_human);
            snprintf(peso_header[PHDR_SPECTEMP_E].comment, PHDR_COMMENT_MAX, "%i",
                    *p_spectemp);
        }

        // CAMFOCUS
        if (p_camfocus != NULL)
        {
            snprintf(peso_header[PHDR_CAMFOCUS_E].value, PHDR_VALUE_MAX, "%i",
                    *p_camfocus);
        }
    }
}

__attribute__((format(printf,2,3)))
static int expose_sgh_exe(char *p_answer, const char *p_fmt, ...)
{
    va_list ap;
    int result;
    char command[SGH_COMMAND_MAX+1];

    va_start(ap, p_fmt);

    vsnprintf(command, SGH_COMMAND_MAX, p_fmt, ap);

    if ((result = sgh_spectrograph_execute(command, p_answer)) == -1) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "Warning: %s",
                sgh_get_err_msg());
    }

    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "SGH_EXE %s => %s", command, p_answer);

    va_end(ap);

    return result;
}

static void exposure_meter_start(void)
{
    char answer[SGH_ANSWER_MAX+1];

    /* exposure meter stop and reset */
    expose_sgh_exe(answer, "SSPE %i", p_peso->expmeter_id);

    /* exposure meter shutter open */
    expose_sgh_exe(answer, "SPCH %i 1", p_peso->expmeter_shutter_id);

    /* exposure meter counter start */
    expose_sgh_exe(answer, "SSTE %i", p_peso->expmeter_id);
}

static void exposure_meter_end(void)
{
    char answer[SGH_ANSWER_MAX+1];
    float expval;

    /* exposure meter shutter close */
    expose_sgh_exe(answer, "SPCH %i 2", p_peso->expmeter_shutter_id);

    /* exposure meter count of pulses */
    if (expose_sgh_exe(answer, "SPCE %i", p_peso->expmeter_id) != -1) {
        /* EXPVAL in Mcounts */
        expval = atof(answer) / 1000000.0;
        snprintf(peso_header[PHDR_EXPVAL_E].value, PHDR_VALUE_MAX, "%f", expval);
    }

    /* exposure meter stop and reset */
    expose_sgh_exe(answer, "SSPE %i", p_peso->expmeter_id);
}

static void fit_start_time(void)
{
    struct tm *p_tm;
    time_t actual_time;

    (void) time(&actual_time);
    p_peso->start_exposure_time = actual_time;
    p_tm = gmtime(&actual_time);

    /* TM_START */
    snprintf(peso_header[PHDR_TM_START_E].value, PHDR_VALUE_MAX, "%i",
            hms2s(p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec));
    snprintf(peso_header[PHDR_TM_START_E].comment, PHDR_COMMENT_MAX,
            "%02d:%02d:%02d, %ld", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec,
            actual_time);

    /* UT */
    snprintf(peso_header[PHDR_UT_E].value, PHDR_VALUE_MAX, "%02d:%02d:%02d",
            p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);

    /* EPOCH */
    //snprintf(peso_header[PHDR_EPOCH_E].value, PHDR_VALUE_MAX, "%f",
    //        equinox(gregorian2julian(p_tm->tm_year+1900, p_tm->tm_mon+1, p_tm->tm_mday)));
    snprintf(peso_header[PHDR_EPOCH_E].value, PHDR_VALUE_MAX, "2000.0");

    /* EQUINOX */
    strncpy(peso_header[PHDR_EQUINOX_E].value, peso_header[PHDR_EPOCH_E].value,
            PHDR_VALUE_MAX);

    /* DATE-OBS */
    snprintf(peso_header[PHDR_DATE_OBS_E].value, PHDR_VALUE_MAX,
            "%04d-%02d-%02d", p_tm->tm_year + 1900, p_tm->tm_mon + 1,
            p_tm->tm_mday);

    /* READSPD */
    strncpy(peso_header[PHDR_READSPD_E].value, mod_ccd.get_readout_speed(),
            PHDR_VALUE_MAX);

    /* GAINM */
    strncpy(peso_header[PHDR_GAINM_E].value, mod_ccd.get_gain(),
            PHDR_VALUE_MAX);

    // TODO
    /* GAIN */
    // peso_header[PHDR_GAIN_E].value = INTEGER;

    /* SYSVER */
    snprintf(peso_header[PHDR_SYSVER_E].value, PHDR_VALUE_MAX, "PESO %s.%s",
            SVN_REV, mod_ccd.peso_get_version());

    fit_save_tle_hdr();
    fit_save_sgh_hdr();
    exposure_meter_start();
}

static void fit_end_time(void)
{
    struct tm *p_tm;
    time_t actual_time;

    (void) time(&actual_time);
    p_peso->stop_exposure_time = actual_time;
    p_tm = gmtime(&actual_time);

    exposure_meter_end();

    /* TM_END */
    snprintf(peso_header[PHDR_TM_END_E].value, PHDR_VALUE_MAX, "%i",
            hms2s(p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec));
    snprintf(peso_header[PHDR_TM_END_E].comment, PHDR_COMMENT_MAX,
            "%02d:%02d:%02d, %ld", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec,
            actual_time);

    /* EXPTIME */
    snprintf(peso_header[PHDR_EXPTIME_E].value, PHDR_VALUE_MAX, "%li",
            p_peso->stop_exposure_time - p_peso->start_exposure_time);

    /* DARKTIME */
    strncpy(peso_header[PHDR_DARKTIME_E].value,
            peso_header[PHDR_EXPTIME_E].value, PHDR_VALUE_MAX);

    /* CCDTEMP */
    snprintf(peso_header[PHDR_CCDTEMP_E].value, PHDR_VALUE_MAX, "%0.1f",
            p_peso->actual_temp);
}

static int save_image(void)
{
    int i;
    int fits_status = 0;
    fitsfile *p_fits;

    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "BEGIN header");

    for (i = 0; i < PHDR_INDEX_MAX_E; ++i)
    {
        if (peso_header[i].value[0] == '\0')
        {
            continue;
        }

        log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "    %s = %s / %s",
                peso_header[i].key, peso_header[i].value,
                peso_header[i].comment);
    }

    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "END header");

    if (mod_ccd.save_raw_image() == -1)
    {
        save_sys_error(LOG4C_PRIORITY_WARN, "Warning: save_raw_image():");
    }
    else
    {
        append_log(LOG4C_PRIORITY_INFO, "save raw image %s success",
                p_peso->raw_image);
    }

    if (fits_create_file(&p_fits, p_peso->fits_file, &fits_status))
    {
        save_fits_error(fits_status, "Error: fits_create_file(%s):",
                p_peso->fits_file);
        return -1;
    }

    if (save_fits_header(p_fits) == -1)
    {
        fits_close_file(p_fits, &fits_status);
        return -1;
    }

    if (mod_ccd.save_fits_file(p_fits, &fits_status) == -1)
    {
        save_fits_error(fits_status, "Error: save_fits_file():");
        fits_close_file(p_fits, &fits_status);
        return -1;
    }

    if (fits_write_chksum(p_fits, &fits_status))
    {
        save_fits_error(fits_status, "Error: fits_write_chksum():");
        fits_close_file(p_fits, &fits_status);
        return -1;
    }

    if (fits_close_file(p_fits, &fits_status))
    {
        save_fits_error(fits_status, "Error: fits_close_file():");
        return -1;
    }

    chown(p_peso->fits_file, exposed_cfg.uid, exposed_cfg.gid);
    append_log(LOG4C_PRIORITY_INFO, "save fits file %s success",
            p_peso->fits_file);

    if (remove(p_peso->raw_image) == -1)
    {
        save_sys_error(LOG4C_PRIORITY_WARN,
                "Warning: remove raw image %s failed:", p_peso->raw_image);
    }
    else
    {
        append_log(LOG4C_PRIORITY_INFO, "remove raw image %s",
                p_peso->raw_image);
    }

    if (p_peso->archive)
    {
        char archive_cmd[EXPOSED_STR_MAX + 1];

        snprintf(archive_cmd, EXPOSED_STR_MAX, "%s %s",
                exposed_cfg.archive_script, p_peso->fits_file);
        append_log(LOG4C_PRIORITY_INFO, "execute archive script %s",
                archive_cmd);
        system(archive_cmd);
    }

    return 0;
}

static int is_exposure_meter_exit()
{
    int expmeter;
    int expmeter_update;
    char answer[SGH_ANSWER_MAX+1];
    int expval;

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    expmeter_update = p_peso->expmeter_update;

    if (expmeter_update != 0) {
        p_peso->expmeter = expmeter_update;
        p_peso->expmeter_update = 0;
    }

    expmeter = p_peso->expmeter;

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    /* exposure meter off */
    if (expmeter == -1) {
        return 0; // false
    }

    /* exposure meter count of pulses */
    if (expose_sgh_exe(answer, "SPCE %i", p_peso->expmeter_id) != -1) {
        expval = atoi(answer);

        if (expval >= expmeter) {
            append_log(LOG4C_PRIORITY_INFO,
                    "actual expval = %i, required expval = %i", expval,
                    expmeter);
            return 1; // true
        }
    }

    return 0; // false
}

static void expose()
{
    int i;
    int result = 0;
    float second;
    char prefix[PREFIX_MAX + 1];

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    p_peso->addtime = 0;
    p_peso->expmeter_update = 0;
    p_peso->exptime_update = 0;
    p_peso->abort = 0;
    p_peso->readout = 0;
    p_peso->elapsed_time = 0;

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    if (fce_make_fits_prefix(exposed_cfg.instrument_prefix[0], prefix,
            PREFIX_MAX))
    {
        p_peso->state = CCD_STATE_READY_E;
        // TODO
        return;
    }

    /* preparing expose */
    if (p_cmd_begin != NULL)
    {
        system(p_cmd_begin);
    }
    // TODO
    //if (system(p_cmd_begin) == -1) {
    //    // implementovat
    //}

    for (i = 0; i < p_peso->expcount; ++i)
    {
        /* LOCK */
        pthr_mutex_lock(&global_mutex);

        if ((result = fce_make_filename(p_peso->path, prefix, p_peso->fits_file,
                PESO_PATH_MAX)) == -1)
        {
            save_sys_error(LOG4C_PRIORITY_ERROR,
                    "Error: fce_make_filename(%s, %s, %s, %i):", p_peso->path,
                    prefix, p_peso->fits_file, PESO_PATH_MAX);
            break;
        }

        p_peso->expnum = i + 1;

        pthr_mutex_unlock(&global_mutex);
        /* UNLOCK */

        strcpy(p_peso->raw_image, p_peso->fits_file);
        strcat(p_peso->raw_image, "raw");
        strcat(p_peso->fits_file, "fit");

        /* SETKEY FILENAME */
        memset(peso_header[PHDR_FILENAME_E].value, 0, PHDR_VALUE_MAX + 1);
        strncpy(peso_header[PHDR_FILENAME_E].value, basename(p_peso->fits_file),
                PHDR_VALUE_MAX);

        if ((result = mod_ccd.expose_init()) == -1)
        {
            break;
        }

        if ((result = mod_ccd.expose_start()) == -1)
        {
            append_log(LOG4C_PRIORITY_ERROR,
                    "Error: mod_ccd.expose_start(): %s", p_peso->msg);
            break;
        }

        fit_start_time();

        /* exposing */
        append_log(LOG4C_PRIORITY_INFO, "expose begin");
        mod_ccd.peso_set_state(CCD_STATE_EXPOSE_E);
        second = 15;
        while (mod_ccd.expose())
        {
            if (second >= 15)
            {
                second = 0;
                if (mod_ccd.get_temp(&p_peso->actual_temp) == -1)
                {
                    append_log(LOG4C_PRIORITY_WARN,
                            "Error: mod_ccd.get_temp(): %s", p_peso->msg);
                }
            }

            if (is_exposure_meter_exit()) {
                /* LOCK */
                pthr_mutex_lock(&global_mutex);
                p_peso->readout = 1;
                pthr_mutex_unlock(&global_mutex);
                /* UNLOCK */
            }

            second += 0.1;
            usleep(100000);
        }
        append_log(LOG4C_PRIORITY_INFO, "expose end");

        if (p_peso->abort <= 1)
        {
            fit_end_time();

            if ((result = mod_ccd.expose_end()) == -1)
            {
                append_log(LOG4C_PRIORITY_WARN, "Warning: expose_end(): %s",
                        p_peso->msg);
            }

            /* reading out */
            append_log(LOG4C_PRIORITY_INFO, "readout begin");
            mod_ccd.peso_set_int(&p_peso->elapsed_time, 0);
            mod_ccd.peso_set_state(CCD_STATE_READOUT_E);
            while (mod_ccd.readout())
            {
                usleep(100000);
            }
            append_log(LOG4C_PRIORITY_INFO, "readout end");

            if (save_image() == -1)
            {
                /* TODO: report to client */
            }
        }

        if ((result = mod_ccd.expose_uninit()) == -1)
        {
            append_log(LOG4C_PRIORITY_ERROR, "Error: expose_uninit(): %s",
                    p_peso->msg);
            break;
        }

        if ((p_peso->abort == 2) || (p_peso->readout))
        {
            break;
        }
    }

    /* finishing expose */
    mod_ccd.peso_set_state(CCD_STATE_FINISH_EXPOSE_E);
    if (p_cmd_end != NULL)
    {
        system(p_cmd_end);
    }

    /* CCD is ready */
    mod_ccd.peso_set_state(CCD_STATE_READY_E);
}

static void *expose_loop(void *arg)
{
    while (!exposed_exit)
    {
        if (mod_ccd.get_temp(&p_peso->actual_temp) == -1)
        {
            append_log(LOG4C_PRIORITY_WARN, "Error: mod_ccd.get_temp(): %s",
                    p_peso->msg);
        }

        if (pthr_sem_wait(&expose_sem, 15) != -1)
        {
            expose();
        }
    }

    pthread_exit(0);
    return NULL;
}

static void exposed_get_ip_addr(TSession * const p_abyss_session, char *p_ip,
        int ip_max)
{
    unsigned char *p_ip_addr;
    struct abyss_unix_chaninfo *p_chan_info;
    struct sockaddr_in *p_sock_addr_in;

    SessionGetChannelInfo(p_abyss_session, (void*) &p_chan_info);

    p_sock_addr_in = (struct sockaddr_in *) &p_chan_info->peerAddr;
    p_ip_addr = (unsigned char *) &p_sock_addr_in->sin_addr.s_addr;

    snprintf(p_ip, ip_max, "%u.%u.%u.%u", p_ip_addr[0], p_ip_addr[1],
            p_ip_addr[2], p_ip_addr[3]);
}

static int exposed_allowed_ip(char *p_ip)
{
    EXPOSED_IP_T *p_exposed_ip;

    p_exposed_ip = exposed_cfg.p_allow_ip;
    while (p_exposed_ip != NULL)
    {
        if (!strcmp(p_exposed_ip->ip, p_ip))
        {
            return 1;
        }
        p_exposed_ip = p_exposed_ip->p_next;
    }

    return 0;
}

static xmlrpc_env *expose_xmlrpc_init(xmlrpc_env *p_env, void *p_chan_info,
        char *p_ip)
{
    exposed_get_ip_addr(p_chan_info, p_ip, CFG_TYPE_STR_MAX);

    if (!exposed_allowed_ip(p_ip))
    {
        xmlrpc_env_set_fault_formatted(p_env, XMLRPC_INTERNAL_ERROR,
                "Connection from %s denied", p_ip);
        return p_env;
    }

    return p_env;
}

__attribute__((format(printf,2,3)))
static void expose_xmlrpc_err2log(xmlrpc_env *p_env, const char *p_fmt, ...)
{
    va_list ap;
    char msg[CCD_MSG_MAX + 1];

    va_start(ap, p_fmt);

    memset(msg, '\0', CCD_MSG_MAX + 1);
    vsnprintf(msg, CCD_MSG_MAX, p_fmt, ap);

    va_end(ap);

    if (p_env->fault_occurred)
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN,
                "%s - xmlrpc failed (%i): %s", msg, p_env->fault_code,
                p_env->fault_string);
    }
    else
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, msg);
    }
}

static xmlrpc_value *expose_set(xmlrpc_env * const p_env,
        xmlrpc_value * const p_param_array, void * const p_server_info,
        void * const p_chan_info)
{
    char ip[CFG_TYPE_STR_MAX + 1];
    char *p_variable = NULL;
    char *p_value = NULL;
    xmlrpc_value *p_xmlrpc_result = NULL;

    XMLRPC_FAIL_IF_FAULT(expose_xmlrpc_init(p_env, p_chan_info, ip));

    xmlrpc_decompose_value(p_env, p_param_array, "(ss)", &p_variable, &p_value);
    XMLRPC_FAIL_IF_FAULT(p_env);

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    p_xmlrpc_result = cmd_set(p_env, p_variable, p_value);

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    cleanup: expose_xmlrpc_err2log(p_env, "%s:expose_set(%s, %s)", ip,
            p_variable, p_value);

    return p_xmlrpc_result;
}

static xmlrpc_value *expose_get(xmlrpc_env * const p_env,
        xmlrpc_value * const p_param_array, void * const p_server_info,
        void * const p_chan_info)
{
    char ip[CFG_TYPE_STR_MAX + 1];
    char *p_variable = NULL;
    xmlrpc_value *p_xmlrpc_result = NULL;

    XMLRPC_FAIL_IF_FAULT(expose_xmlrpc_init(p_env, p_chan_info, ip));

    xmlrpc_decompose_value(p_env, p_param_array, "(s)", &p_variable);
    XMLRPC_FAIL_IF_FAULT(p_env);

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    p_xmlrpc_result = cmd_get(p_env, p_variable);

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    cleanup: expose_xmlrpc_err2log(p_env, "%s:expose_get(%s)", ip, p_variable);

    return p_xmlrpc_result;
}

static xmlrpc_value *expose_set_key(xmlrpc_env * const p_env,
        xmlrpc_value * const p_param_array, void * const p_server_info,
        void * const p_chan_info)
{
    char ip[CFG_TYPE_STR_MAX + 1];
    char *p_key = NULL;
    char *p_value = NULL;
    char *p_comment = NULL;
    xmlrpc_value *p_xmlrpc_result = NULL;

    XMLRPC_FAIL_IF_FAULT(expose_xmlrpc_init(p_env, p_chan_info, ip));

    xmlrpc_decompose_value(p_env, p_param_array, "(sss)", &p_key, &p_value,
            &p_comment);
    XMLRPC_FAIL_IF_FAULT(p_env);

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    p_xmlrpc_result = cmd_set_key(p_env, p_key, p_value, p_comment);

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    cleanup: expose_xmlrpc_err2log(p_env, "%s:expose_set_key(%s, %s, %s)", ip,
            p_key, p_value, p_comment);

    return p_xmlrpc_result;
}

static xmlrpc_value *expose_get_key(xmlrpc_env * const p_env,
        xmlrpc_value * const p_param_array, void * const p_server_info,
        void * const p_chan_info)
{
    char ip[CFG_TYPE_STR_MAX + 1];
    char *p_key = NULL;
    xmlrpc_value *p_xmlrpc_result = NULL;

    XMLRPC_FAIL_IF_FAULT(expose_xmlrpc_init(p_env, p_chan_info, ip));

    xmlrpc_decompose_value(p_env, p_param_array, "(s)", &p_key);
    XMLRPC_FAIL_IF_FAULT(p_env);

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    p_xmlrpc_result = cmd_get_key(p_env, p_key);

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    cleanup: expose_xmlrpc_err2log(p_env, "%s:expose_get_key(%s)", ip, p_key);

    return p_xmlrpc_result;
}

static xmlrpc_value *expose_get_all_keys(xmlrpc_env * const p_env,
        xmlrpc_value * const p_param_array, void * const p_server_info,
        void * const p_chan_info)
{
    char ip[CFG_TYPE_STR_MAX + 1];
    xmlrpc_value *p_xmlrpc_result = NULL;

    XMLRPC_FAIL_IF_FAULT(expose_xmlrpc_init(p_env, p_chan_info, ip));

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    p_xmlrpc_result = get_all_key(p_env);

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    cleanup: expose_xmlrpc_err2log(p_env, "%s:expose_get_all_keys()", ip);

    return p_xmlrpc_result;
}

static xmlrpc_value *expose_start(xmlrpc_env * const p_env,
        xmlrpc_value * const p_param_array, void * const p_server_info,
        void * const p_chan_info)
{
    char ip[CFG_TYPE_STR_MAX + 1];
    int exptime = -1;
    int expcount = -1;
    int expmeter = -1;
    xmlrpc_value *p_xmlrpc_result = NULL;

    XMLRPC_FAIL_IF_FAULT(expose_xmlrpc_init(p_env, p_chan_info, ip));

    xmlrpc_decompose_value(p_env, p_param_array, "(iii)", &exptime, &expcount, &expmeter);
    XMLRPC_FAIL_IF_FAULT(p_env);

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    if (p_peso->state == CCD_STATE_READY_E)
    {
        p_xmlrpc_result = cmd_expose(p_env, exptime, expcount, expmeter);

        if (!p_env->fault_occurred)
        {
            p_peso->state = CCD_STATE_PREPARE_EXPOSE_E;
        }
    }
    else
    {
        p_xmlrpc_result = xmlrpc_build_value(p_env, "s",
                "-ERR expose already running");
    }

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    cleanup: expose_xmlrpc_err2log(p_env, "%s:expose_start(%i, %i, %i)", ip,
            exptime, expcount, expmeter);

    return p_xmlrpc_result;
}

static xmlrpc_value *expose_add_time(xmlrpc_env * const p_env,
        xmlrpc_value * const p_param_array, void * const p_server_info,
        void * const p_chan_info)
{
    char ip[CFG_TYPE_STR_MAX + 1];
    int addtime = -1;
    xmlrpc_value *p_xmlrpc_result = NULL;

    XMLRPC_FAIL_IF_FAULT(expose_xmlrpc_init(p_env, p_chan_info, ip));

    xmlrpc_decompose_value(p_env, p_param_array, "(i)", &addtime);
    XMLRPC_FAIL_IF_FAULT(p_env);

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    p_xmlrpc_result = cmd_addtime(p_env, addtime);

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    cleanup: expose_xmlrpc_err2log(p_env, "%s:expose_add_time(%i)", ip,
            addtime);

    return p_xmlrpc_result;
}

static xmlrpc_value *expose_time_update(xmlrpc_env * const p_env,
        xmlrpc_value * const p_param_array, void * const p_server_info,
        void * const p_chan_info)
{
    char ip[CFG_TYPE_STR_MAX + 1];
    int exptime = -1;
    xmlrpc_value *p_xmlrpc_result = NULL;

    XMLRPC_FAIL_IF_FAULT(expose_xmlrpc_init(p_env, p_chan_info, ip));

    xmlrpc_decompose_value(p_env, p_param_array, "(i)", &exptime);
    XMLRPC_FAIL_IF_FAULT(p_env);

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    p_xmlrpc_result = cmd_exptime_update(p_env, exptime);

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    cleanup: expose_xmlrpc_err2log(p_env, "%s:expose_time_update(%i)", ip,
            exptime);

    return p_xmlrpc_result;
}

static xmlrpc_value *expose_meter_update(xmlrpc_env * const p_env,
        xmlrpc_value * const p_param_array, void * const p_server_info,
        void * const p_chan_info)
{
    char ip[CFG_TYPE_STR_MAX + 1];
    int expmeter = -1;
    xmlrpc_value *p_xmlrpc_result = NULL;

    XMLRPC_FAIL_IF_FAULT(expose_xmlrpc_init(p_env, p_chan_info, ip));

    xmlrpc_decompose_value(p_env, p_param_array, "(i)", &expmeter);
    XMLRPC_FAIL_IF_FAULT(p_env);

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    p_xmlrpc_result = cmd_expmeter_update(p_env, expmeter);

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    cleanup: expose_xmlrpc_err2log(p_env, "%s:expose_meter_update(%i)", ip,
            expmeter);

    return p_xmlrpc_result;
}

static xmlrpc_value *expose_readout(xmlrpc_env * const p_env,
        xmlrpc_value * const p_param_array, void * const p_server_info,
        void * const p_chan_info)
{
    char ip[CFG_TYPE_STR_MAX + 1];
    xmlrpc_value *p_xmlrpc_result = NULL;

    XMLRPC_FAIL_IF_FAULT(expose_xmlrpc_init(p_env, p_chan_info, ip));

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    p_peso->readout = 1;
    p_xmlrpc_result = xmlrpc_build_value(p_env, "s", "+OK");

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    cleanup: expose_xmlrpc_err2log(p_env, "%s:expose_readout()", ip);

    return p_xmlrpc_result;
}

static xmlrpc_value *expose_abort(xmlrpc_env * const p_env,
        xmlrpc_value * const p_param_array, void * const p_server_info,
        void * const p_chan_info)
{
    char ip[CFG_TYPE_STR_MAX + 1];
    xmlrpc_value *p_xmlrpc_result = NULL;

    XMLRPC_FAIL_IF_FAULT(expose_xmlrpc_init(p_env, p_chan_info, ip));

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    if (p_peso->abort == 0)
    {
        p_peso->abort = 1;
    }

    p_xmlrpc_result = xmlrpc_build_value(p_env, "s", "+OK");

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    cleanup: expose_xmlrpc_err2log(p_env, "%s:expose_abort()", ip);

    return p_xmlrpc_result;
}

static xmlrpc_value *expose_info(xmlrpc_env * const p_env,
        xmlrpc_value * const p_param_array, void * const p_server_info,
        void * const p_chan_info)
{
    int full_time;
    xmlrpc_value *p_xmlrpc_result = NULL;

    /* LOCK */
    pthr_mutex_lock(&global_mutex);

    switch (p_peso->state)
    {
    case CCD_STATE_EXPOSE_E:
        full_time = p_peso->exptime;
        break;

    case CCD_STATE_READOUT_E:
        full_time = p_peso->readout_time;
        break;

    default:
        full_time = 0;
        break;
    }

    p_xmlrpc_result = xmlrpc_build_value(p_env,
            "{s:s,s:s,s:i,s:i,s:i,s:s,s:s,s:s,s:s,s:d,s:i,s:i,s:s}", "filename",
            peso_header[PHDR_FILENAME_E].value, "state",
            exposed_state2str(p_peso->state), "elapsed_time",
            p_peso->elapsed_time, "full_time", full_time, "archive",
            p_peso->archive, "path", p_peso->path, "archive_path",
            p_peso->archive_path, "paths", exposed_cfg.output_paths,
            "archive_paths", exposed_cfg.archive_paths, "ccd_temp",
            p_peso->actual_temp, "expose_count", p_peso->expcount,
            "expose_number", p_peso->expnum, "instrument",
            exposed_cfg.instrument);

    pthr_mutex_unlock(&global_mutex);
    /* UNLOCK */

    return p_xmlrpc_result;
}

static void init_fits_header(void)
{
    int i;
    EXPOSED_HEADER_T *p_hdr;

    p_hdr = exposed_cfg.p_header;
    while (p_hdr != NULL)
    {
        for (i = 0; i < PHDR_INDEX_MAX_E; ++i)
        {
            if (!strcmp(peso_header[i].key, p_hdr->key))
            {
                memset(peso_header[i].value, 0, PHDR_VALUE_MAX + 1);
                strncpy(peso_header[i].value, p_hdr->value, PHDR_VALUE_MAX);
                break;
            }
        }

        p_hdr = p_hdr->p_next;
    }
}

// TODO:
//
//    - nacitat z konfiguracniho souboru cely prikaz pro ziskani hodnoty
//    - odstranit nepouzivane promene
//
static int instrument2exposure_meter_id()
{
    if (!strcmp(exposed_cfg.instrument, "CCD700")) {
        p_peso->expmeter_id = 14;
        p_peso->expmeter_shutter_id = 10;
        p_peso->camfocus_id = 4;
        p_peso->spectemp_id = 19;
        //p_peso->p_hdr_camfocus =
    }
    else if (!strcmp(exposed_cfg.instrument, "CCD400")) {
        p_peso->expmeter_id = 14;
        p_peso->expmeter_shutter_id = 10;
        p_peso->camfocus_id = 5;
        p_peso->spectemp_id = 19;
    }
    else if (!strcmp(exposed_cfg.instrument, "OES")) {
        p_peso->expmeter_id = 24;
        p_peso->expmeter_shutter_id = 23;
        p_peso->camfocus_id = 22;
        p_peso->spectemp_id = 20;
    }
    else {
        return -1;
    }

    return 0;
}

//static void run_long_option(const char *option)
//{
//    if (!strcmp(option, "option_name")) {
//        strncpy(string, optarg, STRING_MAX);
//    }
//}

int main(int argc, char *argv[])
{
    int daemonize = 0;
    char exposed_ini[EXPOSED_XML_MAX + 1];
    FILE *fw;
    pid_t pid;
    pid_t sid;
    pthread_t accept_pthread;
    char *p_dlerror_msg;
    xmlrpc_server_abyss_parms serverparm;
    xmlrpc_registry *registryP;
    xmlrpc_env env;

    struct xmlrpc_method_info3 const expose_set_MI =
    { .methodName = "expose_set", .methodFunction = &expose_set, };

    struct xmlrpc_method_info3 const expose_get_MI =
    { .methodName = "expose_get", .methodFunction = &expose_get, };

    struct xmlrpc_method_info3 const expose_set_key_MI =
    { .methodName = "expose_set_key", .methodFunction = &expose_set_key, };

    struct xmlrpc_method_info3 const expose_get_key_MI =
    { .methodName = "expose_get_key", .methodFunction = &expose_get_key, };

    // TODO: proverit
    struct xmlrpc_method_info3 const expose_get_all_keys_MI =
    { .methodName = "expose_get_all_keys", .methodFunction =
            &expose_get_all_keys, };

    struct xmlrpc_method_info3 const expose_start_MI =
    { .methodName = "expose_start", .methodFunction = &expose_start, };

    struct xmlrpc_method_info3 const expose_add_time_MI =
    { .methodName = "expose_add_time", .methodFunction = &expose_add_time, };

    struct xmlrpc_method_info3 const expose_abort_MI =
    { .methodName = "expose_abort", .methodFunction = &expose_abort, };

    struct xmlrpc_method_info3 const expose_readout_MI =
    { .methodName = "expose_readout", .methodFunction = &expose_readout, };

    struct xmlrpc_method_info3 const expose_info_MI =
    { .methodName = "expose_info", .methodFunction = &expose_info, };

    struct xmlrpc_method_info3 const expose_time_update_MI =
    { .methodName = "expose_time_update", .methodFunction = &expose_time_update, };

    struct xmlrpc_method_info3 const expose_meter_update_MI =
    { .methodName = "expose_meter_update", .methodFunction = &expose_meter_update, };

    memset(exposed_ini, '\0', EXPOSED_XML_MAX + 1);

    while (1)
    {
        int o;
        int option_index = 0;
        static struct option long_options[] =
        {
        { "help", 0, 0, 'h' },
        { "version", 0, 0, 'v' },
        { "conf", 1, 0, 'c' },
        { "daemonize", 0, 0, 'd' },
        { 0, 0, 0, 0 } };

        o = getopt_long(argc, argv, "hvdc:", long_options, &option_index);
        if (o == -1)
        {
            break;
        }

        switch (o)
        {
        case 0:
            //run_long_option(long_options[option_index].name);
            break;

        case 'v':
            daemon_version();
            exit(EXIT_SUCCESS);
            break;

        case 'h':
            daemon_help();
            exit(EXIT_SUCCESS);
            break;

        case 'd':
            daemonize = 1;
            break;

        case 'c':
            strncpy(exposed_ini, optarg, EXPOSED_XML_MAX);
            break;

            /* '?' */
        default:
            exit(EXIT_FAILURE);
        }
    }

    /* TODO: vyuzivat konstantu pro nazev souboru, posilat SIGTERM a teprve po chvili SIGKILL */
    //system("kill -9 $(cat /opt/peso/run/exposed.pid)");
    if (daemonize)
    {
        pid = fork();
        if (pid < 0)
        {
            exit(EXIT_FAILURE);
        }
        else if (pid > 0)
        {
            /* exit the parent process */
            exit(EXIT_SUCCESS);
        }

        umask(0);

        sid = setsid();
        if (sid < 0)
        {
            exit(EXIT_FAILURE);
        }

        if ((chdir("/")) < 0)
        {
            exit(EXIT_FAILURE);
        }

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    if (log4c_init())
    {
        perror("log4c_init() failed");
        exit(EXIT_FAILURE);
    }

    // TODO: logovat PID
    p_logcat = log4c_category_get("exposed");
    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "Starting exposed r%s %s",
            SVN_REV, MAKE_DATE_TIME);

    if (exposed_ini[0] == '\0')
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "Error: Must usage option -c INI_CONFIGURATION_FILE");
        daemon_exit(EXIT_FAILURE);
    }

    {
        char *p_path;
        char *p_begin;
        char *p_end;

        if ((p_path = strdup(exposed_ini)) == NULL) {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                    "Error: strdup() => ENOMEM");
            daemon_exit(EXIT_FAILURE);
        }

        p_begin = strchr(basename(p_path), '-');
        p_end = strchr(p_path, '.');

        if ((p_begin == NULL) || (p_end == NULL)) {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                    "Error: Bad format filename %s", exposed_ini);
            daemon_exit(EXIT_FAILURE);
        }

        *p_end = '\0';
        strncpy(exposed_ccd_name, p_begin+1, EXPOSED_CCD_NAME_MAX);
        free(p_path);
        p_path = NULL;

        log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
                "exposed_ccd_name = %s", exposed_ccd_name);
    }

    if (cfg_load(exposed_ini, &exposed_cfg, exposed_ccd_name))
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "Error: Load configuration from %s failed", exposed_ini);
        daemon_exit(EXIT_FAILURE);
    }

    init_fits_header();

    if ((fw = fopen(exposed_cfg.file_pid, "w")) == NULL)
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN,
                "Warning: Write exposed PID failed: %i: %s", errno,
                strerror(errno));
    }
    else
    {
        fprintf(fw, "%i\n", sid);

        if (fclose(fw) == EOF)
        {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN,
                    "Warning: File %s close failed: %i: %s",
                    exposed_cfg.file_pid, errno, strerror(errno));
        }
    }

    memset(&exposed_allocate, 0, sizeof(EXPOSED_ALLOCATE_T));

    if (mod_ccd_init(exposed_cfg.mod_ccd, &mod_ccd, &p_peso) == -1)
    {
        p_dlerror_msg = dlerror();

        if (p_dlerror_msg != NULL)
        {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                    "Error: mod_ccd_init(): %s", p_dlerror_msg);
        }
        else
        {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                    "Error: mod_ccd_init() failed");
        }

        daemon_exit(EXIT_FAILURE);
    }
    exposed_allocate.mod_ccd = 1;

    if (pthread_mutex_init(&global_mutex, NULL) != 0)
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "Error: pthread_mutex_init(): %i: %s", errno, strerror(errno));
        daemon_exit(EXIT_FAILURE);
    }
    exposed_allocate.global_mutex = 1;
    p_peso->p_global_mutex = &global_mutex;
    p_peso->p_exposed_cfg = &exposed_cfg;
    p_peso->expcount = 1;
    p_peso->expnum = 1;
    p_peso->p_logcat = p_logcat;

    if (instrument2exposure_meter_id() == -1) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "Error: instrument2exposure_meter_id(): Unknown instrument '%s'",
                exposed_cfg.instrument);
        daemon_exit(EXIT_FAILURE);
    }

    // bxr_client_init()
    if (tle_init() == -1)
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR, "Error: %s",
                tle_get_err_msg());
    }

    // bxr_client_init()
    if (sgh_init() == -1)
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR, "Error: %s",
                sgh_get_err_msg());
    }

    // Warning: must be call after bxr_client_init()
    if (mod_ccd.init() == -1)
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "Error: mod_ccd.init(): %s", p_peso->msg);
        daemon_exit(EXIT_FAILURE);
    }

    p_peso->state = CCD_STATE_READY_E;

    if (sem_init(&expose_sem, 0, 0) == -1)
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "Error: sem_init(): %i: %s", errno, strerror(errno));
        daemon_exit(EXIT_FAILURE);
    }
    exposed_allocate.expose_sem = 1;

    if (sem_init(&service_sem, 0, 0) == -1)
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "Error: sem_init(): %i: %s", errno, strerror(errno));
        daemon_exit(EXIT_FAILURE);
    }
    exposed_allocate.service_sem = 1;

    memset(exposed_log, 0, sizeof(exposed_log));

    (void) signal(SIGTERM, daemon_signal); /* abort            */
    (void) signal(SIGINT, daemon_signal); /* readout (ctrl+c) */
    (void) signal(SIGHUP, daemon_signal); /* readout          */
    (void) signal(SIGUSR1, daemon_signal); /* readout          */
    (void) signal(SIGPIPE, SIG_IGN); /* send()           */

    if (pthread_create(&accept_pthread, NULL, expose_loop, NULL) != 0)
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "Error: pthread_create(): %i: %s", errno, strerror(errno));
        daemon_exit(EXIT_FAILURE);
    }

    xmlrpc_env_init(&env);

    registryP = xmlrpc_registry_new(&env);

    xmlrpc_registry_add_method3(&env, registryP, &expose_set_MI);
    xmlrpc_registry_add_method3(&env, registryP, &expose_get_MI);
    xmlrpc_registry_add_method3(&env, registryP, &expose_set_key_MI);
    xmlrpc_registry_add_method3(&env, registryP, &expose_get_key_MI);
    xmlrpc_registry_add_method3(&env, registryP, &expose_get_all_keys_MI);
    xmlrpc_registry_add_method3(&env, registryP, &expose_start_MI);
    xmlrpc_registry_add_method3(&env, registryP, &expose_add_time_MI);
    xmlrpc_registry_add_method3(&env, registryP, &expose_abort_MI);
    xmlrpc_registry_add_method3(&env, registryP, &expose_readout_MI);
    xmlrpc_registry_add_method3(&env, registryP, &expose_info_MI);
    xmlrpc_registry_add_method3(&env, registryP, &expose_time_update_MI);
    xmlrpc_registry_add_method3(&env, registryP, &expose_meter_update_MI);

    serverparm.config_file_name = NULL;
    serverparm.registryP = registryP;
    serverparm.port_number = exposed_cfg.port;
    serverparm.log_file_name = NULL;

    xmlrpc_server_abyss(&env, &serverparm, XMLRPC_APSIZE(log_file_name));

    tle_uninit();
    sgh_uninit();

    log4c_fini();
    daemon_exit(EXIT_SUCCESS);
    return 0;
}
