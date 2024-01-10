/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <fitsio.h>

#include "modules.h"
#include "mod_ccd.h"
#include "thread.h"

PESO_T peso;

int ccd_init(void)
{
    peso.state = CCD_STATE_UNKNOWN_E;
    peso.imgtype = CCD_IMGTYPE_UNKNOWN_E;

    peso.archive = 0;
    peso.readout_time = 5;
    peso.actual_temp = -100.5;
    strncpy(peso.path, "/tmp", PESO_PATH_MAX);
    strncpy(peso.archive_path, "pleione:/tmp", PESO_PATH_MAX);
    strncpy(peso.msg, "ccd_init()", CCD_MSG_MAX);

    peso.state = CCD_STATE_READY_E;
    return 0;
}

int ccd_uninit(void)
{
    strncpy(peso.msg, "ccd_uninit()", CCD_MSG_MAX);

    return 0;
}

int ccd_expose_init()
{
    return 0;
}

int ccd_expose_start(void)
{
    return 0;
}

int ccd_expose(void)
{
    time_t actual_time;

    if (peso.abort != 0)
    {
        peso.abort = 2;
        return 0;
    }
    else if (peso.readout)
    {
        return 0;
    }

    (void) time(&actual_time);

    peso.elapsed_time = actual_time - peso.start_exposure_time;

    if (peso.elapsed_time >= peso.exptime)
    {
        return 0;
    }

    return 1;
}

int ccd_readout(void)
{
    time_t actual_time;

    (void) time(&actual_time);

    peso.elapsed_time = actual_time - peso.stop_exposure_time;

    if (peso.elapsed_time >= peso.readout_time)
    {
        return 0;
    }

    return 1;
}

int ccd_save_raw_image(void)
{
    FILE *fw;

    if ((fw = fopen(peso.raw_image, "w")) == NULL)
    {
        return -1;
    }

    fwrite("x", sizeof(char), 1, fw);

    if (fclose(fw) == EOF)
    {
        return -1;
    }

    return 0;
}

int ccd_save_fits_file(fitsfile *p_fits, int *p_fits_status)
{

    return 0;
}

int ccd_expose_end(void)
{
    return 0;
}

int ccd_expose_uninit(void)
{
    peso.state = CCD_STATE_READY_E;
    return 0;
}

int ccd_get_temp(double *p_temp)
{
    if (mod_ccd_check_state(peso.state) == -1)
    {
        return -1;
    }

    *p_temp = peso.actual_temp;

    //strncpy(peso.msg, "ccd_get_temp() failure", CCD_MSG_MAX);
    //return -1;

    return 0;
}

int ccd_set_temp(double temp)
{
    if (mod_ccd_check_state(peso.state) == -1)
    {
        return -1;
    }

    //strncpy(peso.msg, "ccd_set_temp() failure", CCD_MSG_MAX);
    //return -1;

    return 0;
}

int ccd_set_readout_speed(char *p_speed)
{
    /* TODO: zamykani */
    if (mod_ccd_check_state(peso.state) == -1)
    {
        return -1;
    }

    return 0;
}

const char *ccd_get_readout_speed(void)
{
    return "dummy";
}

const char *ccd_get_readout_speeds(void)
{
    return "dummy";
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
