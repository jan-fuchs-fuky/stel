/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 * $URL$
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>
#include <fitsio.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>
#include <log4c.h>
#include <glib.h>

#include "modules.h"
#include "mod_ccd.h"
#include "mod_ccd_gandalf.h"
#include "header.h"
#include "thread.h"
#include "bxmlrpc.h"

#define GAN_RPC_SERVER_URL_MAX 1023
#define GAN_RPC_ERR_MSG_MAX    1023
#define GAN_RPC_ANSWER_MAX     1023
#define GAN_RPC_COMMAND_MAX    1023

#ifdef SELF_TEST_RPCGANDALF

log4c_category_t *p_logcat = NULL;

#endif

PESO_T peso;

static char ccd_readout_speeds[PESO_READOUT_SPEEDS_MAX + 1];
static char gan_ccd_gains[PESO_GAINS_MAX + 1];
static unsigned long gan_size;
static unsigned short *p_raw_data = NULL;
static unsigned short *p_raw_data_reverse = NULL;

static xmlrpc_env gan_rpc_env;
static xmlrpc_client *p_gan_rpc_client = NULL;
static xmlrpc_server_info *p_gan_rpc_server_info = NULL;
static char gan_rpc_err_msg[GAN_RPC_ERR_MSG_MAX + 1];

/* TODO: sdilet tuto funkci ve vsech modulech a i s daemonem exposed */
__attribute__((format(printf,1,2)))
static int save_sys_error(const char *p_fmt, ...)
{
    va_list ap;
    int len;

    va_start(ap, p_fmt);

    /* LOCK */
    //pthread_mutex_lock(peso.p_global_mutex);
    vsnprintf(peso.msg, CCD_MSG_MAX, p_fmt, ap);
    len = strlen(peso.msg);
    snprintf(peso.msg + len, CCD_MSG_MAX - len, " %i: %s", errno,
            strerror(errno));

    //pthread_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */

    va_end(ap);

    return 0;
}

static int gan_rpc_is_fault_occurred(xmlrpc_env *p_gan_rpc_env)
{
    if (p_gan_rpc_env->fault_occurred)
    {
        snprintf(gan_rpc_err_msg, GAN_RPC_ERR_MSG_MAX, "XML-RPC Fault: %s (%d)",
                p_gan_rpc_env->fault_string, p_gan_rpc_env->fault_code);
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_ERROR, gan_rpc_err_msg);
        return 1;
    }
    else
    {
        return 0;
    }
}

int gan_rpc_init(void)
{
    char server_url[GAN_RPC_SERVER_URL_MAX + 1];
    //char *p_spectrograph_host = "127.0.0.1";
    char *p_spectrograph_host = "192.168.193.198";
    char *p_spectrograph_port = "5000";

    bzero(gan_rpc_err_msg, GAN_RPC_ERR_MSG_MAX + 1);

    xmlrpc_env_init(&gan_rpc_env);
    xmlrpc_limit_set(XMLRPC_XML_SIZE_LIMIT_ID, 10 * 1024 * 1024);

    xmlrpc_client_create(&gan_rpc_env, 0, "peso", SVN_REV, NULL, 0, &p_gan_rpc_client);
    if (gan_rpc_is_fault_occurred(&gan_rpc_env))
    {
        return -1;
    }

    snprintf(server_url, GAN_RPC_SERVER_URL_MAX, "http://%s:%s/RPC2",
            p_spectrograph_host, p_spectrograph_port);
    p_gan_rpc_server_info = xmlrpc_server_info_new(&gan_rpc_env, server_url);
    if (gan_rpc_is_fault_occurred(&gan_rpc_env))
    {
        return -1;
    }

    return 0;
}

int gan_rpc_uninit(void)
{
    if (p_gan_rpc_server_info != NULL)
    {
        xmlrpc_server_info_free(p_gan_rpc_server_info);
        p_gan_rpc_server_info = NULL;
    }

    if (p_gan_rpc_client != NULL)
    {
        xmlrpc_client_destroy(p_gan_rpc_client);
        p_gan_rpc_client = NULL;
    }

    xmlrpc_env_clean(&gan_rpc_env);
    return 1;
}

int gan_rpc_execute_rint(char *p_fce, xmlrpc_value *p_param_array)
{
    int result = -1;
    xmlrpc_value *p_result;

    xmlrpc_client_call2(
        &gan_rpc_env,
        p_gan_rpc_client,
        p_gan_rpc_server_info,
        p_fce,
        p_param_array,
        &p_result
    );

    if (gan_rpc_is_fault_occurred(&gan_rpc_env))
    {
        return -1;
    }

    xmlrpc_read_int(&gan_rpc_env, p_result, &result);

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "RPC %s() => %i", p_fce, result);

    return result;
}

double gan_rpc_execute_rdouble(char *p_fce, xmlrpc_value *p_param_array)
{
    double result = -1;
    xmlrpc_value *p_result;

    xmlrpc_client_call2(
        &gan_rpc_env,
        p_gan_rpc_client,
        p_gan_rpc_server_info,
        p_fce,
        p_param_array,
        &p_result
    );

    if (gan_rpc_is_fault_occurred(&gan_rpc_env))
    {
        return -1;
    }

    xmlrpc_read_double(&gan_rpc_env, p_result, &result);

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "RPC %s() => %0.2f", p_fce, result);

    return result;
}

const char *gan_rpc_execute_rstr(char *p_fce)
{
    const char *p_str;
    xmlrpc_value *p_result;
    xmlrpc_value *p_param_array;

    p_param_array = xmlrpc_array_new(&gan_rpc_env);

    xmlrpc_client_call2(
        &gan_rpc_env,
        p_gan_rpc_client,
        p_gan_rpc_server_info,
        p_fce,
        p_param_array,
        &p_result
    );

    if (gan_rpc_is_fault_occurred(&gan_rpc_env))
    {
        return NULL;
    }

    xmlrpc_read_string(&gan_rpc_env, p_result, &p_str);
    xmlrpc_DECREF(p_result);

    return p_str;
}

int ccd_get_temp(double *p_temp)
{
    xmlrpc_value *p_param_array = xmlrpc_array_new(&gan_rpc_env);

    *p_temp = gan_rpc_execute_rdouble("ccd_get_temp", p_param_array);

    xmlrpc_DECREF(p_param_array);

    return 0;
}

int ccd_set_temp(double temp)
{
    int result;

    xmlrpc_value *p_param_array = xmlrpc_array_new(&gan_rpc_env);
    xmlrpc_value *p_param_temp = xmlrpc_double_new(&gan_rpc_env, temp);

    xmlrpc_array_append_item(&gan_rpc_env, p_param_array, p_param_temp);

    result = gan_rpc_execute_rint("ccd_set_temp", p_param_array);

    xmlrpc_DECREF(p_param_temp);
    xmlrpc_DECREF(p_param_array);

    return result;
}

int ccd_init(void)
{
    memset(ccd_readout_speeds, '\0', PESO_READOUT_SPEEDS_MAX + 1);
    memset(gan_ccd_gains, '\0', PESO_GAINS_MAX + 1);

    peso.state = CCD_STATE_UNKNOWN_E;
    peso.imgtype = CCD_IMGTYPE_UNKNOWN_E;

    peso.readout_speed = GAN_SPEED_50KHZ_E;
    peso.gain = GAN_GAIN_HIGH_E;
    peso.archive = 0;
    peso.require_temp = peso.p_exposed_cfg->ccd.temp;
    peso.x1 = peso.p_exposed_cfg->ccd.x1;
    peso.x2 = peso.p_exposed_cfg->ccd.x2;
    peso.xb = peso.p_exposed_cfg->ccd.xb;
    peso.y1 = peso.p_exposed_cfg->ccd.y1;
    peso.y2 = peso.p_exposed_cfg->ccd.y2;
    peso.yb = peso.p_exposed_cfg->ccd.yb;
    peso.pixel_count_max = peso.x2 * peso.y2;
    peso.bits_per_pixel = peso.p_exposed_cfg->ccd.bits_per_pixel;

    gan_rpc_init();
    ccd_set_temp(peso.require_temp);
    ccd_get_temp(&peso.actual_temp);

    return 0;
}

int ccd_uninit(void)
{
    gan_rpc_uninit();
    return 0;
}

int ccd_expose_init(void)
{
    int result;

    peso_set_int(&peso.readout_time, 28);

    //shutter = (peso.shutter) ? OPEN_PRE_TRIGGER : OPEN_NEVER;

    gan_size = (peso.x2 - peso.x1 + 1) / peso.xb;
    gan_size *= (peso.y2 - peso.y1 + 1) / peso.yb;
    gan_size *= 2;

    if (p_raw_data != NULL)
    {
        free(p_raw_data);
        p_raw_data = NULL;
    }
    if ((p_raw_data = (unsigned short *) malloc(gan_size)) == NULL)
    {
        save_sys_error("Error: malloc():");
        return -1;
    }

    if (p_raw_data_reverse != NULL)
    {
        free(p_raw_data_reverse);
        p_raw_data_reverse = NULL;
    }
    if ((p_raw_data_reverse = (unsigned short *) malloc(gan_size)) == NULL)
    {
        save_sys_error("Error: malloc():");
        return -1;
    }

    xmlrpc_value *p_param_array = xmlrpc_array_new(&gan_rpc_env);
    xmlrpc_value *p_param_exptime = xmlrpc_int_new(&gan_rpc_env, peso.exptime);
    xmlrpc_value *p_param_readout_speed = xmlrpc_int_new(&gan_rpc_env, peso.readout_speed);
    xmlrpc_value *p_param_shutter = xmlrpc_int_new(&gan_rpc_env, peso.shutter);
    xmlrpc_value *p_param_gain = xmlrpc_int_new(&gan_rpc_env, peso.gain);

    xmlrpc_array_append_item(&gan_rpc_env, p_param_array, p_param_exptime);
    xmlrpc_array_append_item(&gan_rpc_env, p_param_array, p_param_readout_speed);
    xmlrpc_array_append_item(&gan_rpc_env, p_param_array, p_param_shutter);
    xmlrpc_array_append_item(&gan_rpc_env, p_param_array, p_param_gain);

    result = gan_rpc_execute_rint("ccd_expose_init", p_param_array);

    xmlrpc_DECREF(p_param_gain);
    xmlrpc_DECREF(p_param_shutter);
    xmlrpc_DECREF(p_param_readout_speed);
    xmlrpc_DECREF(p_param_exptime);
    xmlrpc_DECREF(p_param_array);

    return result;
}

static int ccd_set_ttl_out(int value)
{
    int result;

    xmlrpc_value *p_param_array = xmlrpc_array_new(&gan_rpc_env);
    xmlrpc_value *p_param_ttl_out = xmlrpc_int_new(&gan_rpc_env, value);

    xmlrpc_array_append_item(&gan_rpc_env, p_param_array, p_param_ttl_out);

    result = gan_rpc_execute_rint("ccd_set_ttl_out", p_param_array);

    xmlrpc_DECREF(p_param_ttl_out);
    xmlrpc_DECREF(p_param_array);

    return result;
}

int ccd_expose_start(void)
{
    int result;

    xmlrpc_value *p_param_array = xmlrpc_array_new(&gan_rpc_env);

    result = gan_rpc_execute_rint("ccd_expose_start", p_param_array);

    xmlrpc_DECREF(p_param_array);

    result = ccd_set_ttl_out(255);

    return result;
}

int ccd_expose(void)
{
    time_t actual_time;

    if (peso.exptime_update != 0)
    {
        /* LOCK */
        pthread_mutex_lock(peso.p_global_mutex);

        if (peso.exptime_update == -1)
        {
            peso.exptime = CCD_EXPTIME_MAX;
        }
        else if (peso.exptime == 0)
        {
            peso.readout = 1;
        }
        else
        {
            peso.exptime = peso.exptime_update;
        }

        peso.exptime_update = 0;

        pthread_mutex_unlock(peso.p_global_mutex);
        /* UNLOCK */
    }

    if (peso.abort != 0)
    {
        peso_set_int(&peso.abort, 2);

        xmlrpc_value *p_param_array = xmlrpc_array_new(&gan_rpc_env);

        gan_rpc_execute_rint("ccd_expose_stop", p_param_array);

        xmlrpc_DECREF(p_param_array);

        return 0;
    }
    else if (peso.readout)
    {
        ccd_set_ttl_out(0);
        return 0;
    }

    (void) time(&actual_time);

    peso_set_int(&peso.elapsed_time, actual_time - peso.start_exposure_time);

    if (peso.elapsed_time >= peso.exptime)
    {
        ccd_set_ttl_out(0);
        return 0;
    }

    /* exposing */
    return 1;
}

int ccd_readout(void)
{
    int result;
    time_t actual_time;

    (void) time(&actual_time);
    peso_set_int(&peso.elapsed_time, actual_time - peso.stop_exposure_time);

    xmlrpc_value *p_param_array = xmlrpc_array_new(&gan_rpc_env);

    result = gan_rpc_execute_rint("ccd_readout", p_param_array);

    xmlrpc_DECREF(p_param_array);

    return result;
}

int ccd_save_raw_image()
{
    const char *p_str;
    //register unsigned short *from;
    FILE *fw;

    p_str = gan_rpc_execute_rstr("ccd_save_raw_image");

    guchar *p_out;
    gsize out_len;

    p_out = g_base64_decode(p_str, &out_len);
    free((void *)p_str);

    /* peso.raw_image not lock */
    if ((fw = fopen(peso.raw_image, "w")) == NULL)
    {
        return -1;
    }

    fwrite(p_out, sizeof(*p_out), out_len, fw);
    // TODO: kontrolovat meze
    memcpy(p_raw_data, p_out, sizeof(*p_out) * out_len);
    g_free(p_out);

    ///* swap bytes */
    //from = (unsigned short *) ((char *) p_raw_data + gan_size);
    //// TODO: zapomnelo se na prvni byte
    //while (from > p_raw_data)
    //{
    //    unsigned short morf;
    //    unsigned short left;
    //    unsigned short right;

    //    left = *from >> 8;
    //    left += 0x80;
    //    right = *from << 8;
    //    morf = left | right;

    //    fwrite(&morf, sizeof(morf), 1, fw);
    //    from--;
    //}

    if (fclose(fw) == EOF)
    {
        return -1;
    }

    return 0;
}

int ccd_save_fits_file(fitsfile *p_fits, int *p_fits_status)
{
    //register unsigned short *from;
    //int index = 0;
    long fpixel = 1;
    long nelements = peso.x2 * peso.y2;

    /* save reverse raw data */
    //from = (unsigned short *) ((char *) p_raw_data + gan_size);
    //while (from > p_raw_data)
    //{
    //    p_raw_data_reverse[index++] = *from--;
    //}

    if (fits_write_img(p_fits, TUSHORT, fpixel, nelements, p_raw_data,
            p_fits_status))
    {
        return -1;
    }

    return 0;
}

int ccd_expose_end(void)
{
    return 0;
}

int ccd_expose_uninit(void)
{
    int result;

    free(p_raw_data_reverse);
    p_raw_data_reverse = NULL;

    free(p_raw_data);
    p_raw_data = NULL;

    xmlrpc_value *p_param_array = xmlrpc_array_new(&gan_rpc_env);

    result = gan_rpc_execute_rint("ccd_expose_uninit", p_param_array);

    xmlrpc_DECREF(p_param_array);

    return result;
}

// Nezamykat, vola se z klienta
int ccd_set_readout_speed(char *p_speed)
{
    int i;

    if (mod_ccd_check_state(peso.state) == -1)
    {
        return -1;
    }

    for (i = 0; i < GAN_SPEED_MAX_E; ++i)
    {
        if (!strcmp(p_speed, gan_speed_str2enum[i].str))
        {
            peso.readout_speed = gan_speed_str2enum[i].speed_e;
            return 0;
        }
    }

    return -1;
}

// TODO: osetrit nenalezeni odpovidajici hodnoty
const char *ccd_get_readout_speed(void)
{
    int i;

    for (i = 0; i < GAN_SPEED_MAX_E; ++i)
    {
        if (gan_speed_str2enum[i].speed_e == peso.readout_speed)
        {
            return gan_speed_str2enum[i].str;
        }
    }

    return gan_speed_str2enum[0].str;
}

const char *ccd_get_readout_speeds(void)
{
    int i;
    int gan_size = 0;

    if (ccd_readout_speeds[0] == '\0')
    {
        for (i = 0; i < GAN_SPEED_MAX_E; ++i)
        {
            strncat(ccd_readout_speeds, gan_speed_str2enum[i].str,
                    PESO_READOUT_SPEEDS_MAX - gan_size - 1);
            strcat(ccd_readout_speeds, ";");
            gan_size += strlen(gan_speed_str2enum[i].str);
        }
    }

    return ccd_readout_speeds;
}

// Nezamykat, vola se z klienta
int ccd_set_gain(char *p_gain)
{
    int i;

    if (mod_ccd_check_state(peso.state) == -1)
    {
        return -1;
    }

    for (i = 0; i < GAN_GAIN_MAX_E; ++i)
    {
        if (!strcmp(p_gain, gan_gain_str2enum[i].str))
        {
            peso.gain = gan_gain_str2enum[i].gain_e;
            return 0;
        }
    }

    return -1;
}

// TODO: osetrit nenalezeni odpovidajici hodnoty
const char *ccd_get_gain(void)
{
    int i;

    for (i = 0; i < GAN_GAIN_MAX_E; ++i)
    {
        if (gan_gain_str2enum[i].gain_e == peso.gain)
        {
            return gan_gain_str2enum[i].str;
        }
    }

    return gan_gain_str2enum[0].str;
}

const char *ccd_get_gains(void)
{
    int i;
    int gan_size = 0;

    if (gan_ccd_gains[0] == '\0')
    {
        for (i = 0; i < GAN_GAIN_MAX_E; ++i)
        {
            strncat(gan_ccd_gains, gan_gain_str2enum[i].str,
                    PESO_GAINS_MAX - gan_size - 1);
            strcat(gan_ccd_gains, ";");
            gan_size += strlen(gan_gain_str2enum[i].str + 1);
        }
    }

    return gan_ccd_gains;
}

#ifdef SELF_TEST_RPCGANDALF

int main(int argc, char *argv[])
{
    pthread_mutex_t global_mutex;

    if (pthread_mutex_init(&global_mutex, NULL) != 0)
    {
        printf("pthread_mutex_init() failed\n");
        exit(EXIT_FAILURE);
    }

    if (cfg_load("/opt/exposed/etc/exposed-gandalf.cfg", &exposed_cfg, "gandalf"))
    {
        printf("cfg_load() failed\n");
        exit(EXIT_FAILURE);
    }

    if (log4c_init())
    {
        perror("log4c_init() failed");
        exit(EXIT_FAILURE);
    }

    p_logcat = log4c_category_get("exposed");
    peso.p_global_mutex = &global_mutex;
    peso.p_exposed_cfg = &exposed_cfg;
    peso.exptime = 5;
    peso.expcount = 1;
    peso.expnum = 1;
    peso.p_logcat = p_logcat;
    peso.state = CCD_STATE_READY_E;
    strcpy(peso.raw_image, "/tmp/gandalf.fit");

    xmlrpc_env bxr_env;

    xmlrpc_env_init(&bxr_env);
    xmlrpc_client_init2(&bxr_env, 0, "peso", SVN_REV, NULL, 0);
    if (bxr_env.fault_occurred)
    {
        fprintf(stderr, "\nXML-RPC Fault: %s (%d)\n", bxr_env.fault_string,
                bxr_env.fault_code);
        xmlrpc_env_clean(&bxr_env);
        return -1;
    }

    ccd_init();

    printf("%s\n", ccd_get_readout_speeds());
    return -1;

    ccd_expose_init();
    ccd_expose_start();

    printf("exposing");
    while (ccd_expose()) {
        printf(".");
        fflush(stdout);
        sleep(1);
    }

    ccd_expose_end();

    printf("reading-out");
    while (ccd_readout()) {
        printf(".");
        fflush(stdout);
        sleep(1);
    }

    ccd_save_raw_image();
    ccd_expose_uninit();
    ccd_uninit();
    log4c_fini();

    xmlrpc_env_clean(&bxr_env);
    xmlrpc_client_cleanup();
}

#endif
