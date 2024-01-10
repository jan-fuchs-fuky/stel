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

#include "master.h"
#include "pvcam.h"
#include "ccd_sauron.h"
#include "socket.h"

static CCD_OPTION_T ccd_option;
static char *names = { "rspipci0" };
static char log_msg[LOG_MSG_MAX];
static unsigned long size;
static short cam;
static unsigned short *raw_data = NULL;
static int fits_create_success;
static int premature_readout = 0;
static int16 ccd_temp;

static void ccd_version(void);
static void ccd_log(int type, char *msg);
static void ccd_abort(void);
static void ccd_signal(int sig);
static void ccd_init(void);
static void ccd_exp_init(void);
static void ccd_exp_uninit(void);
static void ccd_uninit(void);
static void ccd_ttl_set(flt64 ttl);
static void str_strip(char *str);
static void add_comments2fit_header(fitsfile *fptr);
static int is_allow_fit_header_char(int c);
static long save_fit_header(fitsfile *fptr);
static void ccd_readout(void);
static void key_value2fit_hdr(char *p_key, char *p_value, char *p_comment, char *p_type);
static void ccd_loop(void);
static int16 ccd_get_temp(void);
static int ccd_save_option_xy(int *p_number);
static void run_long_option(const char *option);

static void ccd_log_fits(int type, int fits_status)
{
    char fits_error[FLEN_ERRMSG];

    fits_create_success = 0;

    fits_get_errstatus(fits_status, fits_error);
    ccd_log(type, fits_error);
}

static void ccd_log(int type, char *msg)
{
    int result;
    int buffer_len;
    char buffer[BUFFER_MAX+1];
    char rs_msg[ERROR_MSG_LEN+1];
    fd_set wfd;
    struct timeval timeout;

    memset(buffer, 0, BUFFER_MAX+1);

    switch (type) {
        case LOG_HIDDEN_INFO:
            syslog(LOG_INFO, msg);
            return;

        case LOG_PESO_INFO:
            snprintf(buffer, BUFFER_MAX, "%s", msg);
            break;

        case LOG_INFO:
            snprintf(buffer, BUFFER_MAX, "%s", msg);
            syslog(LOG_INFO, msg);
            break;

        case LOG_ERR:
            snprintf(buffer, BUFFER_MAX, "error: %s (%i - %s)", msg, errno, strerror(errno));
            syslog(LOG_ERR, "%s (%i - %s)", msg, errno, strerror(errno));
            break;

        case LOG_PVCAM_ERR:
            pl_error_message(pl_error_code(), rs_msg);
            snprintf(buffer, BUFFER_MAX, "error: %s(): %s", msg, rs_msg);
            syslog(LOG_ERR, "%s(): %s", msg, rs_msg);
            break;

        case LOG_WARNING:
            snprintf(buffer, BUFFER_MAX, "%s (%i - %s)", msg, errno, strerror(errno));
            syslog(LOG_WARNING, "%s (%i - %s)", msg, errno, strerror(errno));
            break;

        default:
            break;
    }

//    buffer_len = strlen(buffer);
//
//    FD_ZERO(&wfd);
//    FD_SET(sockfd_w, &wfd);
//
//    timeout.tv_sec = 0;
//    timeout.tv_usec = 100000; /* 100ms */
//
//    result = select(FD_SETSIZE, (fd_set *)0, &wfd, (fd_set *)0, &timeout);
//
//    switch (result) {
//        case 0:
//            syslog(LOG_WARNING, "Select sockfd_w timeout");
//            break;
//
//        case -1:
//            syslog(LOG_WARNING, "Select sockfd_w failure");
//            break;
//
//        default:
//            if (sendto(sockfd_w, buffer, buffer_len, 0, (struct sockaddr *)&serv_addr, serv_len) != buffer_len) {
//                perror(msg);
//                syslog(LOG_WARNING, "Socket sendto previous message failure");
//            }
//            break;
//    }

    switch (type) {
        case LOG_ERR:
        case LOG_PVCAM_ERR:
            if (ccd_option.cmd_after[0] != '\0') system(ccd_option.cmd_after);
            ccd_log(LOG_INFO, "exit");
            close(sockfd_w);
            if (remove(FILE_PID) == -1) ccd_log(LOG_WARNING, "Remove FILE_PID failure");
            exit(EXIT_FAILURE);
            break;

        default:
            break;
    }
}

static void ccd_abort(void)
{
    if (!pl_exp_abort(cam, CCS_CLEAR_CLOSE_SHTR)) ccd_log(LOG_PVCAM_ERR, "pl_exp_abort");

    ccd_uninit();

    if (raw_data != NULL) {
        free(raw_data);
        raw_data = NULL;
    }

    if (ccd_option.cmd_after[0] != '\0') system(ccd_option.cmd_after);
}

static void ccd_init(void)
{
    if (!pl_pvcam_init())                                                    ccd_log(LOG_PVCAM_ERR, "pl_pvcam_init");
    if (!pl_cam_open(names, &cam, OPEN_EXCLUSIVE)) ccd_log(LOG_PVCAM_ERR, "pl_cam_open");
}

static void ccd_exp_init(void)
{
    rgn_type region;

    size    = (ccd_option.x2 - ccd_option.x1 + 1) / ccd_option.xb;
    size *= (ccd_option.y2 - ccd_option.y1 + 1) / ccd_option.yb;
    size *= 2;
        
    region.s1 = ccd_option.x1 - 1;
    region.p1 = ccd_option.y1 - 1;
    region.s2 = ccd_option.x2 - 1;
    region.p2 = ccd_option.y2 - 1;
    region.sbin = ccd_option.xb;
    region.pbin = ccd_option.yb;

    if (!pl_set_param(cam, PARAM_SPDTAB_INDEX, (void *)&ccd_option.speed))       ccd_log(LOG_PVCAM_ERR, "pl_set_param_speed");
    if (!pl_set_param(cam, PARAM_TEMP_SETPOINT, (void *)&ccd_option.temp))       ccd_log(LOG_PVCAM_ERR, "pl_set_param_temp");
    if (!pl_set_param(cam, PARAM_SHTR_OPEN_MODE, (void *)&ccd_option.shutter)) ccd_log(LOG_PVCAM_ERR, "pl_set_param_shutter");
    if (!pl_exp_init_seq())                                                                                                      ccd_log(LOG_PVCAM_ERR, "pl_exp_init_seq");
    if (!pl_exp_setup_seq(cam, 1, 1, &region, STROBED_MODE, 0, &size))               ccd_log(LOG_PVCAM_ERR, "pl_exp_setup_seq");
    if ((raw_data = (unsigned short *)malloc(size)) == NULL)                                     ccd_log(LOG_ERR, "Memory for buffer not found");
    if (!pl_exp_start_seq(cam, raw_data))                                                                            ccd_log(LOG_PVCAM_ERR, "pl_exp_start_seq");
}

static void ccd_exp_uninit(void)
{
    if (!pl_exp_finish_seq(cam, raw_data, 0)) ccd_log(LOG_PVCAM_ERR, "pl_exp_finish_seq");
    if (!pl_exp_uninit_seq())                               ccd_log(LOG_PVCAM_ERR, "pl_exp_uninit_seq");

    free(raw_data);
}

static void ccd_uninit(void)
{
    /* pl_cam_close() automaticky vola pl_pvcam_uninit() pro vsechny otevrene kamery */
    if (!pl_pvcam_uninit()) ccd_log(LOG_PVCAM_ERR, "pl_pvcam_uninit");
}

static void ccd_ttl_set(flt64 ttl)
{
    if (!pl_set_param(cam, PARAM_IO_STATE, (void *)&ttl)) {
        ccd_log(LOG_PVCAM_ERR, "pl_set_param_ttl");
    }
}

static void ccd_readout(void)
{
    int fits_status;
    int image_index = 0;
    register unsigned short *from, *end;
    unsigned short *image;
    char filename_raw[FILENAME_MAX];
    char *p_basename;
    long nelements;
    long fpixel = 1;
    FILE *fw;
    fitsfile *fptr;

    /*****************/
    /* Save raw data */
    /*****************/

    ccd_log(LOG_INFO, "save raw data begin");

    if ((p_basename = strrchr(ccd_option.output, '/')) == NULL)
        p_basename = ccd_option.output;

    memset(filename_raw, 0, FILENAME_MAX);
    strncpy(filename_raw, ccd_option.output, p_basename - ccd_option.output);
    strcat(filename_raw, p_basename);
    filename_raw[strlen(filename_raw)-4] = '\0';
    strcat(filename_raw, ".raw");

    ccd_log(LOG_INFO, filename_raw);

    if ((fw = fopen(filename_raw, "w")) == NULL) ccd_log(LOG_ERR, filename_raw);

    /* Readout CCD */
    end = raw_data;
    from = (unsigned short *)((char *)end+size);
    while (from > end) {
        unsigned short morf;
        unsigned short levy;
        unsigned short pravy;
        
        levy = *from >> 8;
        levy += 0x80;
        pravy = *from << 8;
        morf = levy | pravy;
        
        fwrite(&morf, sizeof(morf), 1, fw);
        
        from--;
    }

    if (fclose(fw) == EOF) ccd_log(LOG_ERR, filename_raw);

    ccd_log(LOG_INFO, "save raw data end");

    /*************/
    /* Save fits */
    /*************/

    ccd_log(LOG_INFO, "save fits begin");

    fits_status = 0;
    fits_create_success = 1;

    if ((image = (unsigned short *)malloc(size)) == NULL) ccd_log(LOG_ERR, "Memory for image not found");
    if (fits_create_file(&fptr, ccd_option.output, &fits_status)) ccd_log_fits(LOG_ERR, fits_status);

    /* Readout CCD */
    from = (unsigned short *)((char *)end+size);
    while (from > end)
        image[image_index++] = *from--;

    nelements = save_fit_header(fptr);

    if (fits_write_img(fptr, TUSHORT, fpixel, nelements, image, &fits_status)) ccd_log_fits(LOG_ERR, fits_status);
    if (fits_write_chksum(fptr, &fits_status)) ccd_log_fits(LOG_ERR, fits_status);
    if (fits_close_file(fptr, &fits_status)) ccd_log_fits(LOG_ERR, fits_status);

    free(image);
    ccd_log(LOG_INFO, "save fits end");

    if (fits_create_success) {
        if (remove(filename_raw) == -1) ccd_log(LOG_WARNING, filename_raw);
    }
}

static void ccd_progress_bar(int full_time, int init, int add_time)
{
    static int finished_time;
    static int unfinished_time;
    static int percent_time;
    char buffer[BUFFER_MAX];

    if (init) {
        finished_time = 0;
        unfinished_time = full_time;
        percent_time = 0;
    }

    unfinished_time += add_time;

    if (!pl_get_param(cam, PARAM_TEMP, ATTR_CURRENT,    (void *)&ccd_temp)) ccd_log(LOG_PVCAM_ERR, "pl_get_param_temp");
    ccd_temp /= 100;

    snprintf(buffer, BUFFER_MAX, "%i;%i;%i;%i", percent_time, finished_time, unfinished_time, ccd_temp);
    ccd_log(LOG_PESO_INFO, buffer);

    ++finished_time;
    --unfinished_time;
    percent_time = finished_time / (full_time / 100.0);

    if (unfinished_time < 0) {
        finished_time = full_time;
        unfinished_time = 0;
        percent_time = 100;
    }
}

static void key_value2fit_hdr(char *p_key, char *p_value, char *p_comment, char *p_type)
{
    int position;
    char *p_c;
    char buffer[BUFFER_MAX];
    char key[KEY_MAX];
    char comment[COMMENT_MAX];
    char type[TYPE_MAX];
    FILE *fw;
    FILE *fr;

    if ((fw = fopen(FILE_FIT_HEADER, "a")) != NULL) {
        if ((p_comment == NULL) || (p_type == NULL)) {
            if ((fr = fopen(FILE_FIT_HEADER_TEMPLATE, "r")) != NULL) {
                memset(buffer, 0, BUFFER_MAX);
                while (fgets(buffer, BUFFER_MAX, fr) != NULL) {
                    position = 0;
                    buffer[strlen(buffer)-1] = '\0';

                    memset(key, 0, KEY_MAX);
                    memset(comment, 0, COMMENT_MAX);
                    memset(type, 0, TYPE_MAX);

                    if ((p_c = strtok(buffer, ";")) != NULL) {
                        strncpy(key, p_c, KEY_MAX);
                        str_strip(key);

                        if (strcmp(p_key, key)) continue;

                        while ((p_c = strtok(NULL, ";")) != NULL) {
                            switch(++position) {
                                case 1:
                                    strncpy(comment, p_c, COMMENT_MAX);
                                    break;

                                case 2:
                                    strncpy(type, p_c, TYPE_MAX);
                                    break;

                                default:
                                    break;
                            }
                        }

                        if (!strcmp(p_key, key)) {
                            if ((p_comment == NULL) && (comment[0] != '\0')) {
                                str_strip(comment);
                                p_comment = comment;
                            }

                            if ((p_type == NULL) && (type[0] != '\0')) {
                                str_strip(type);
                                p_type = type;
                            }

                            break;
                        }
                    }
                }

                if (fclose(fr) == EOF)
                    ccd_log(LOG_WARNING, "Close FILE_FIT_HEADER_TEMPLATE failure");
            }
        }

        if (p_comment == NULL) {
            if (comment[0] == '\0')
                strcpy(comment, " ");
            p_comment = comment;
        }

        if (p_type == NULL) {
            if (type[0] == '\0')
                strcpy(type, " ");
            p_type = type;
        }

        if (fprintf(fw, "%-8s ; %-20s ; %-50s ; %-10s\n", p_key, p_value, p_comment, p_type) < 0)
            ccd_log(LOG_WARNING, "fprintf failure");

        if (fclose(fw) == EOF)
            ccd_log(LOG_WARNING, "Close FILE_FIT_HEADER failure");
    }
    else {
        ccd_log(LOG_WARNING, "Open FILE_FIT_HEADER failure");
    }
}

static void str_strip(char *str)
{
    int i;
    int str_len;
    char buffer[BUFFER_MAX];

    str_len = strlen(str);

    if (str_len > BUFFER_MAX)
        str_len = BUFFER_MAX;

    strncpy(buffer, str, BUFFER_MAX);

    for (i = (str_len-1); i >= 0; i--) {
        if (buffer[i] == ' ')
            buffer[i] = '\0';
        else
            break;
    }

    for (i = 0; i < str_len; i++) {
        if (buffer[i] != ' ')
            break;
    }

    strcpy(str, buffer+i);
}

static void add_comments2fit_header(fitsfile *fptr)
{
    int i;
    int c;
    int fits_status = 0;
    int index;
    char comment[COMMENT_MAX];
    char *file_comment = FILE_COMMENT1;
    FILE *fr;

    for (i = 0; i < 2; i++) {
        switch (i) {
            case 0:
                file_comment = FILE_COMMENT1;
                break;

            case 1:
                file_comment = FILE_COMMENT2;
                break;

            default:
                break;
        }

        if ((fr = fopen(file_comment, "r")) == NULL) {
            continue;
        }

        memset(comment, 0, COMMENT_MAX);
        strcpy(comment, "       ");
        index = 2;
        while ((c = fgetc(fr)) != EOF) {
            if (is_allow_fit_header_char(c)) {
                comment[index++] = c;
            }

            if ((index >= 70) || ((index > 0) && (c == '\n'))) {
                if (fits_write_comment(fptr, comment, &fits_status)) ccd_log_fits(LOG_WARNING, fits_status);
                memset(comment, 0, COMMENT_MAX);
                strcpy(comment, "       ");
                index = 2;
            }
        }

        if (index > 1) {
            if (fits_write_comment(fptr, comment, &fits_status)) ccd_log_fits(LOG_WARNING, fits_status);
        }

        fclose(fr);
    }
}

static int is_allow_fit_header_char(int c)
{
    int i;
    int allow_chars_len;
    char allow_chars[] = " <>{}()[]+-_=~!@#$%^&*|/'\";:.?";

    /* a-z */
    if ((c >= 'a') && (c <= 'z')) {
        return 1;
    }

    /* A-Z */
    if ((c >= 'A') && (c <= 'Z')) {
        return 1;
    }

    /* 0-9 */
    if ((c >= '0') && (c <= '9')) {
        return 1;
    }

    allow_chars_len = strlen(allow_chars);
    for (i = 0; i < allow_chars_len; i++) {
        if (c == allow_chars[i]) {
            return 1;
        }
    }

    return 0;
}

static long save_fit_header(fitsfile *fptr)
{
    int fits_status = 0;
    int number_line = 0;
    int bscale = 1;
    int bzero = 32768;
    int value_int;
    float value_float;
    double value_double;
    char value_str[VALUE_MAX];
    char key[KEY_MAX];
    char value[VALUE_MAX];
    char comment[COMMENT_MAX];
    char type[TYPE_MAX];
    char line[KVCT_MAX];
    char *p_key;
    long naxis = 2;
    long naxes[2] = { 2048, 2048 };
    FILE *fr;

    if ((fr = fopen(FILE_FIT_HEADER, "r")) == NULL) ccd_log(LOG_ERR, FILE_FIT_HEADER);

    while (fgets(line, 1024, fr) != NULL) {
        memset(key, 0, KEY_MAX);
        memset(value, 0, VALUE_MAX);
        memset(comment, 0, COMMENT_MAX);
        memset(type, 0, TYPE_MAX);

        line[strlen(line)-1] = '\0';

        if ((p_key = (char *)strtok(line, ";")) != NULL) {
            strncpy(key, p_key, KEY_MAX);
            str_strip(key);
        }
        if ((p_key = (char *)strtok(NULL, ";")) != NULL) {
            strncpy(value, p_key, VALUE_MAX);
            str_strip(value);
        }
        if ((p_key = (char *)strtok(NULL, ";")) != NULL) {
            strncpy(comment, p_key, COMMENT_MAX);
            str_strip(comment);
        }
        if ((p_key = (char *)strtok(NULL, ";")) != NULL) {
            strncpy(type, p_key, TYPE_MAX);
            str_strip(type);
        }

        if (number_line == 0) {
            naxes[0] = atoi(key);       /* not key, but NAXIS1   */
            naxes[1] = atoi(value); /* not value, but NAXIS2 */

            if (fits_create_img(fptr, USHORT_IMG, naxis, naxes, &fits_status)) ccd_log_fits(LOG_ERR, fits_status);
            if (fits_update_key(fptr, TINT, "BZERO", &bzero, "", &fits_status)) ccd_log_fits(LOG_ERR, fits_status);
            if (fits_update_key(fptr, TINT, "BSCALE", &bscale, "REAL=TAPE*BSCALE+BZERO", &fits_status)) ccd_log_fits(LOG_ERR, fits_status);
        }
        else {
            if (!strcmp(type, "int")) {
                value_int = atoi(value);
                if (fits_update_key(fptr, TLONG, key, &value_int, comment, &fits_status)) ccd_log_fits(LOG_ERR, fits_status);
            }
            else if (!strcmp(type, "float")) {
                value_float = atof(value);
                if (fits_update_key(fptr, TFLOAT, key, &value_float, comment, &fits_status)) ccd_log_fits(LOG_ERR, fits_status);
            }
            else if (!strcmp(type, "double")) {
                value_double = atof(value);
                if (fits_update_key(fptr, TDOUBLE, key, &value_double, comment, &fits_status)) ccd_log_fits(LOG_ERR, fits_status);
            }
            else if (!strcmp(type, "str")) {
                strcpy(value_str, value);
                if (fits_update_key(fptr, TSTRING, key, &value_str, comment, &fits_status)) ccd_log_fits(LOG_ERR, fits_status);
            }
        }

        if (number_line == 0) {
            snprintf(log_msg, LOG_MSG_MAX, "    NAXIS1 = %18s\n", key);
            ccd_log(LOG_HIDDEN_INFO, log_msg);

            snprintf(log_msg, LOG_MSG_MAX, "    NAXIS2 = %18s\n", value);
            ccd_log(LOG_HIDDEN_INFO, log_msg);
        }
        else {
            snprintf(log_msg, LOG_MSG_MAX, "%8s = %18s\n", key, value);
            ccd_log(LOG_HIDDEN_INFO, log_msg);
        }

        number_line++;

#ifdef DBG
        printf("%8s | %8s | %18s | %44s\n", key, type, value, comment);
#endif
    }

    if (fclose(fr) == EOF) ccd_log(LOG_WARNING, FILE_FIT_HEADER);
    if (fits_write_date(fptr, &fits_status)) ccd_log_fits(LOG_ERR, fits_status);
    add_comments2fit_header(fptr);

    return naxes[0] * naxes[1]; /* nelements */
}

static void fit_start_time(void)
{
    char value[VALUE_MAX];
    char comment[COMMENT_MAX];
    struct tm *p_tm;
    time_t actual_time;

    (void)time(&actual_time);
    p_tm = gmtime(&actual_time);

    memset(value, 0, VALUE_MAX);
    snprintf(value, VALUE_MAX, "%i", hmv2v(p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec));
    memset(comment, 0, COMMENT_MAX);
    snprintf(comment, COMMENT_MAX, "%02d:%02d:%02d, %ld", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec, actual_time);
    key_value2fit_hdr("TM_START", value, comment, NULL);

    memset(value, 0, VALUE_MAX);
    snprintf(value, VALUE_MAX, "%02d:%02d:%02d", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
    key_value2fit_hdr("UT", value, NULL, NULL);

    memset(value, 0, VALUE_MAX);
    snprintf(value, VALUE_MAX, "%f", equinox(gregorian2julian(p_tm->tm_year+1900, p_tm->tm_mon+1, p_tm->tm_mday)));
    key_value2fit_hdr("EPOCH", value, NULL, NULL);
    key_value2fit_hdr("EQUINOX", value, NULL, NULL);

    memset(value, 0, VALUE_MAX);
    snprintf(value, VALUE_MAX, "%04d-%02d-%02d", p_tm->tm_year+1900, p_tm->tm_mon+1, p_tm->tm_mday);
    /* fits_write_date(fitsfile *fptr, int *status)  */
    /* key_value2fit_hdr("DATE", value, NULL, NULL); */
    key_value2fit_hdr("DATE-OBS", value, NULL, NULL);
}

static void fit_stop_time(void)
{
    struct tm *p_tm;
    time_t actual_time;
    char value[VALUE_MAX];
    char comment[COMMENT_MAX];

    (void)time(&actual_time);
    p_tm = gmtime(&actual_time);

    memset(value, 0, VALUE_MAX);
    snprintf(value, VALUE_MAX, "%i", hmv2v(p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec));
    memset(comment, 0, COMMENT_MAX);
    snprintf(comment, COMMENT_MAX, "%02d:%02d:%02d, %ld", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec, actual_time);
    key_value2fit_hdr("TM_STOP", value, comment, NULL);

    memset(value, 0, VALUE_MAX);
    snprintf(value, VALUE_MAX, "%i", ccd_option.time);
    key_value2fit_hdr("EXPTIME", value, NULL, NULL);
    key_value2fit_hdr("DARKTIME", value, NULL, NULL);

    memset(value, 0, VALUE_MAX);
    snprintf(value, VALUE_MAX, "%i", ccd_temp);
    key_value2fit_hdr("CCDTEMP", value, NULL, NULL);
}

static void ccd_loop(void)
{
    int i;
    int add_time = 0;
    char line[16];
    short status = 0;
    unsigned long bytes;
    FILE *fr;
 
    ccd_init();
    ccd_exp_init();

    ccd_ttl_set(1);
    fit_start_time();

    ccd_log(LOG_INFO, "expose begin");
    ccd_progress_bar(ccd_option.time, 1, 0);

    for (i = 0; i < ccd_option.time; i++) {
        if (premature_readout) break;

        ccd_progress_bar(ccd_option.time, 0, 0);

        if ((fr = fopen(FILE_CCD_ADD_TIME, "r")) != NULL) {
            if (fgets(line, 16, fr) != NULL)
                add_time = atoi(line);

            fclose(fr);

            if (remove(FILE_CCD_ADD_TIME) == -1) ccd_log(LOG_WARNING, FILE_CCD_ADD_TIME);

            ccd_option.time += add_time;
            ccd_progress_bar(ccd_option.time, 0, add_time);
            snprintf(log_msg, LOG_MSG_MAX, "add_time = %is", add_time);
            ccd_log(LOG_INFO, log_msg);
        }

        sleep(1);
    }

    ccd_option.time = i;
    ccd_ttl_set(0);
    fit_stop_time();

    if (ccd_option.cmd_after[0] != '\0') {
        system(ccd_option.cmd_after);
        ccd_log(LOG_INFO, ccd_option.cmd_after);
    }

    ccd_log(LOG_INFO, "expose end");
    ccd_log(LOG_INFO, "readout ccd begin");
    ccd_progress_bar(ccd_option.readout_time, 1, 0);

    while (1) {
        if (!pl_exp_check_status(cam, &status, &bytes)) ccd_log(LOG_PVCAM_ERR, "pl_exp_check_status");

        if (status == READOUT_COMPLETE)
            break;

        ccd_progress_bar(ccd_option.readout_time, 0, 0);
        sleep(1);
    }

    ccd_log(LOG_INFO, "readout ccd end");
    ccd_readout();

    ccd_exp_uninit();
    ccd_uninit();
}

static int16 ccd_get_temp(void)
{
    int16 temp;
    char buffer[BUFFER_MAX];

    ccd_init();
    if (!pl_get_param(cam, PARAM_TEMP, ATTR_CURRENT,    (void *)&temp)) ccd_log(LOG_PVCAM_ERR, "pl_get_param_temp");
    temp /= 100;
    ccd_uninit();

    memset(buffer, 0, BUFFER_MAX);
    snprintf(buffer, BUFFER_MAX, "ccd temp = %i", temp);
    ccd_log(LOG_INFO, buffer);

    return temp;
}

static int ccd_save_option_xy(int *p_number)
{
    *p_number = atoi(optarg);

    if ((*p_number < 0) || (*p_number > 2048))
        return 0;

    return 1;
}

static int cmd_before(void)
{
    char buffer[BUFFER_MAX];
    FILE *pr;
    fd_set rfd;
    struct timeval timeout;

    memset(buffer, 0, BUFFER_MAX);

    if (ccd_option.cmd_before[0] != '\0') {
        if ((pr = popen(ccd_option.cmd_before, "r")) != NULL) {
            FD_ZERO(&rfd);
            FD_SET(fileno(pr), &rfd);

            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            if (select(FD_SETSIZE, &rfd, (fd_set *)0, (fd_set *)0, &timeout) <= 0) {
                ccd_log(LOG_ERR, "cmd timeout");
                return -1;
            }

            if (fgets(buffer, BUFFER_MAX, pr) != NULL) {
                ccd_log(LOG_INFO, ccd_option.cmd_before);
                if (strcmp(buffer, "EXIT\n")) {
                    ccd_log(LOG_ERR, "cmd failure");
                    return -1;
                }
            }
            else {
                pclose(pr);
                ccd_log(LOG_ERR, "fgets() failure");
                return -1;
            }
            pclose(pr);
        }
        else {
            ccd_log(LOG_ERR, "popen() failure");
            return -1;
        }
    }

    return 0;
}
