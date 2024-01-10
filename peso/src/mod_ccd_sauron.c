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
#include <log4c.h>

#include "sauron/master.h"
#include "sauron/pvcam.h"
#include "modules.h"
#include "mod_ccd.h"
#include "mod_ccd_sauron.h"
#include "header.h"
#include "thread.h"

PESO_T peso;

static char *names =
{ "rspipci0" };
static char ccd_readout_speeds[PESO_READOUT_SPEEDS_MAX + 1];
static char gan_ccd_gains[PESO_GAINS_MAX + 1];
static short cam;
static unsigned long size;
static unsigned short *p_raw_data = NULL;
static unsigned short *p_raw_data_reverse = NULL;

__attribute__((format(printf,1,2)))
static int ccd_save_pl_error(const char *p_fmt, ...)
{
    va_list ap;
    int len;
    char p_err_msg[ERROR_MSG_LEN + 1];

    va_start(ap, p_fmt);

    /* LOCK */
    //pthread_mutex_lock(peso.p_global_mutex);
    vsnprintf(peso.msg, CCD_MSG_MAX, p_fmt, ap);
    len = strlen(peso.msg);
    memset(p_err_msg, '\0', ERROR_MSG_LEN + 1);
    pl_error_message(pl_error_code(), p_err_msg);
    snprintf(peso.msg + len, CCD_MSG_MAX - len, " %s", p_err_msg);

    //pthread_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */

    va_end(ap);

    return 0;
}

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

// TODO: otestovat
int ccd_get_temp(double *p_temp)
{
    int16 ccd_temp;

    if (mod_ccd_check_state(peso.state) == -1)
    {
        return -1;
    }

    if (!pl_get_param(cam, PARAM_TEMP, ATTR_CURRENT, (void *) &ccd_temp))
    {
        // TODO: zalogovat
        return -1;
    }
    else
    {
        *p_temp = ccd_temp / 100;
    }

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "ccd_get_temp() => %0.0f", *p_temp);

    //strncpy(peso.msg, "ccd_get_temp() failure", CCD_MSG_MAX);
    //return -1;

    return 0;
}

// TODO: vyresit zamykani
int ccd_set_temp(double temp)
{
    int temp_int = temp * 100;

    //if (mod_ccd_check_state(peso.state) == -1)
    //{
    //    return -1;
    //}

    if (!pl_set_param(cam, PARAM_TEMP_SETPOINT, (void *)&temp_int))
    {
        // TODO: zalogovat i popis chyby
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_ERROR, "ccd_set_temp(%0.0f) => failure", temp);
        return -1;
    }

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "ccd_set_temp(%0.0f) => success", temp);

    //strncpy(peso.msg, "ccd_set_temp() failure", CCD_MSG_MAX);
    //return -1;

    return 0;
}

static int ccd_ttl_set(flt64 ttl)
{
    if (!pl_set_param(cam, PARAM_IO_STATE, (void *) &ttl))
    {
        ccd_save_pl_error("Error: pl_set_param(PARAM_IO_STATE, %f):", ttl);
        return -1;
    }

    return 0;
}

int ccd_init(void)
{
    memset(ccd_readout_speeds, '\0', PESO_READOUT_SPEEDS_MAX + 1);
    memset(gan_ccd_gains, '\0', PESO_GAINS_MAX + 1);

    peso.state = CCD_STATE_UNKNOWN_E;
    peso.imgtype = CCD_IMGTYPE_UNKNOWN_E;
    peso.archive = 0;
    peso.require_temp = peso.p_exposed_cfg->ccd.temp;
    peso.x1 = 1;
    peso.x2 = 2048;
    peso.xb = 1;
    peso.y1 = 1;
    peso.y2 = 2048;
    peso.yb = 1;

    if (!pl_pvcam_init())
    {
        ccd_save_pl_error("Error: pl_pvcam_init():");
        return -1;
    }

    if (!pl_cam_open(names, &cam, OPEN_EXCLUSIVE))
    {
        ccd_save_pl_error("Error: pl_cam_open(%s, OPEN_EXCLUSIVE):", names);
        return -1;
    }

    ccd_get_temp(&peso.actual_temp);
    ccd_set_temp(peso.require_temp);

    return 0;
}

int ccd_uninit(void)
{
    /* pl_cam_close() automaticky vola pl_pvcam_uninit() pro vsechny otevrene kamery */
    if (!pl_pvcam_uninit())
    {
        ccd_save_pl_error("Error: pl_pvcam_uninit():");
        return -1;
    }

    return 0;
}

int ccd_expose_init(void)
{
    int16 speed;
    //int16 temp;
    int16 shutter;
    rgn_type region;

    /* LOCK */
    //pthread_mutex_lock(peso.p_global_mutex);
    if (peso.readout_speed == 0)
    {
        peso_set_int(&peso.readout_time, 43);
    }
    else
    {
        peso_set_int(&peso.readout_time, 5);
    }

    speed = peso.readout_speed;
    //temp = peso.require_temp * 100;
    shutter = (peso.shutter) ? OPEN_PRE_TRIGGER : OPEN_NEVER;

    size = (peso.x2 - peso.x1 + 1) / peso.xb;
    size *= (peso.y2 - peso.y1 + 1) / peso.yb;
    size *= 2;

    region.s1 = peso.x1 - 1;
    region.p1 = peso.y1 - 1;
    region.s2 = peso.x2 - 1;
    region.p2 = peso.y2 - 1;
    region.sbin = peso.xb;
    region.pbin = peso.yb;

    //pthread_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */

    if (p_raw_data != NULL)
    {
        free(p_raw_data);
        p_raw_data = NULL;
    }
    if ((p_raw_data = (unsigned short *) malloc(size)) == NULL)
    {
        save_sys_error("Error: malloc():");
        return -1;
    }

    if (p_raw_data_reverse != NULL)
    {
        free(p_raw_data_reverse);
        p_raw_data_reverse = NULL;
    }
    if ((p_raw_data_reverse = (unsigned short *) malloc(size)) == NULL)
    {
        save_sys_error("Error: malloc():");
        return -1;
    }

    if (!pl_set_param(cam, PARAM_SPDTAB_INDEX, (void *) &speed))
    {
        ccd_save_pl_error("Error: pl_set_param(PARAM_SPDTAB_INDEX, %i):",
                speed);
        return -1;
    }
    //if (!pl_set_param(cam, PARAM_TEMP_SETPOINT, (void *)&temp)) {
    //    ccd_save_pl_error("Error: pl_set_param(PARAM_TEMP_SETPOINT, %i):", temp);
    //    return -1;
    //}
    if (!pl_set_param(cam, PARAM_SHTR_OPEN_MODE, (void *) &shutter))
    {
        ccd_save_pl_error("Error: pl_set_param(PARAM_SHTR_OPEN_MODE, %i):",
                shutter);
        return -1;
    }
    if (!pl_exp_init_seq())
    {
        ccd_save_pl_error("Error: pl_exp_init_seq():");
        return -1;
    }
    if (!pl_exp_setup_seq(cam, 1, 1, &region, STROBED_MODE, 0, &size))
    {
        ccd_save_pl_error("Error: pl_exp_setup_seq(STROBED_MODE, size = %li):",
                size);
        return -1;
    }
    if (!pl_exp_start_seq(cam, p_raw_data))
    {
        ccd_save_pl_error("Error: pl_exp_start_seq():");
        return -1;
    }

    return 0;
}

int ccd_expose_start(void)
{
    return ccd_ttl_set(1);
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
        return 0;
    }
    else if (peso.readout)
    {
        return 0;
    }

    (void) time(&actual_time);

    peso_set_int(&peso.elapsed_time, actual_time - peso.start_exposure_time);

    if (peso.elapsed_time >= peso.exptime)
    {
        return 0;
    }

    /* exposing */
    return 1;
}

int ccd_readout(void)
{
    time_t actual_time;
    short status = 0;
    unsigned long bytes;

    if (!pl_exp_check_status(cam, &status, &bytes))
    {
        ccd_save_pl_error("pl_exp_check_status() failure:");
        return -1;
    }

    (void) time(&actual_time);
    peso_set_int(&peso.elapsed_time, actual_time - peso.stop_exposure_time);

    if (status == READOUT_COMPLETE)
    {
        return 0;
    }

    return 1;
}

int ccd_save_raw_image()
{
    register unsigned short *from;
    FILE *fw;

    /* peso.raw_image not lock */
    if ((fw = fopen(peso.raw_image, "w")) == NULL)
    {
        return -1;
    }

    /* swap bytes */
    from = (unsigned short *) ((char *) p_raw_data + size);
    // TODO: zapomnelo se na prvni byte
    while (from > p_raw_data)
    {
        unsigned short morf;
        unsigned short left;
        unsigned short right;

        left = *from >> 8;
        left += 0x80;
        right = *from << 8;
        morf = left | right;

        fwrite(&morf, sizeof(morf), 1, fw);
        from--;
    }

    if (fclose(fw) == EOF)
    {
        return -1;
    }

    return 0;
}

int ccd_save_fits_file(fitsfile *p_fits, int *p_fits_status)
{
    register unsigned short *from;
    int index = 0;
    long fpixel = 1;
    long nelements = 2048 * 2048;

    /* save reverse raw data */
    from = (unsigned short *) ((char *) p_raw_data + size);
    while (from > p_raw_data)
    {
        p_raw_data_reverse[index++] = *from--;
    }

    if (fits_write_img(p_fits, TUSHORT, fpixel, nelements, p_raw_data_reverse,
            p_fits_status))
    {
        return -1;
    }

    return 0;
}

int ccd_expose_end(void)
{
    return ccd_ttl_set(0);
}

int ccd_expose_uninit(void)
{
    free(p_raw_data_reverse);
    p_raw_data_reverse = NULL;

    if (!pl_exp_finish_seq(cam, p_raw_data, 0))
    {
        ccd_save_pl_error("pl_exp_finish_seq() failure:");
        return -1;
    }
    if (!pl_exp_uninit_seq())
    {
        ccd_save_pl_error("pl_exp_uninit_seq() failure:");
        return -1;
    }

    free(p_raw_data);
    p_raw_data = NULL;

    return 0;
}

// Nezamykat, vola se z klienta
int ccd_set_readout_speed(char *p_speed)
{
    int i;

    if (mod_ccd_check_state(peso.state) == -1)
    {
        return -1;
    }

    for (i = 0; i < SAURON_SPEED_MAX_E; ++i)
    {
        if (!strcmp(p_speed, sauron_speed_str2enum[i].str))
        {
            peso.readout_speed = sauron_speed_str2enum[i].speed_e;
            return 0;
        }
    }

    return -1;
}

const char *ccd_get_readout_speed(void)
{
    return sauron_speed_str2enum[peso.readout_speed].str;
}

const char *ccd_get_readout_speeds(void)
{
    int i;
    int size = 0;

    if (ccd_readout_speeds[0] == '\0')
    {
        for (i = 0; i < SAURON_SPEED_MAX_E; ++i)
        {
            strncat(ccd_readout_speeds, sauron_speed_str2enum[i].str,
                    PESO_READOUT_SPEEDS_MAX - size - 1);
            strcat(ccd_readout_speeds, ";");
            size += strlen(sauron_speed_str2enum[i].str);
        }
    }

    return ccd_readout_speeds;
}

// Nezamykat, vola se z klienta
int ccd_set_gain(char *p_gain)
{
    return 0;
}

const char *ccd_get_gain(void)
{
    return "default";
}

const char *ccd_get_gains(void)
{
    return "default";
}
