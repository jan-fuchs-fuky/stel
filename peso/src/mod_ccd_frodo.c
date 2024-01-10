/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 */

/*
 *  http://www.astro-cam.com/MANUALS/General/ARC_API/
 *
 *  TODO: implementovat a otestovat nastavovani rychlosti
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <fitsio.h>
#include <stropts.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <log4c.h>

#include "modules.h"
#include "mod_ccd.h"
#include "thread.h"
#include "mod_ccd_frodo.h"

#define LINUX 
typedef int HANDLE;

//#include "frodo/Driver.h"
//#include "frodo/DSPCommand.h"
//#include "frodo/Memory.h"
//#include "frodo/LoadDspFile.h"
//#include "frodo/Temperature.h"
//#include "frodo/ArcCameraAPI.h"

#include "frodo/ArcDefs.h"
#include "frodo/ArcDeviceCAPI.h"

#define PCI_DEVICE_NAME         "/dev/astropci0\0"

#ifdef SELF_TEST_FRODO

log4c_category_t *p_logcat = NULL;

#endif

PESO_T peso;

static char ccd_readout_speeds[PESO_READOUT_SPEEDS_MAX + 1];
static char gan_ccd_gains[PESO_GAINS_MAX + 1];
static int fro_abort;
static int fro_readout;
static int fro_expose;
static int fro_buffer_size;
static float fro_readout_set;
static unsigned long fro_data_size;
static unsigned short *p_fro_raw_data = NULL;
static int fro_status;

__attribute__((format(printf,1,2)))
static int ccd_save_error(const char *p_fmt, ...)
{
    va_list ap;

    va_start(ap, p_fmt);

    /* LOCK */
    //pthread_mutex_lock(peso.p_global_mutex);
    vsnprintf(peso.msg, CCD_MSG_MAX, p_fmt, ap);

    //pthread_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */

    va_end(ap);

    return 0;
}

/* TODO: sdilet tuto funkci ve vsech modulech a i s daemonem exposed */
//__attribute__((format(printf,1,2)))
//static int save_sys_error(const char *p_fmt, ...)
//{
//    va_list ap;
//    int len;
//
//    va_start(ap, p_fmt);
//
//    /* LOCK */
//    //pthread_mutex_lock(peso.p_global_mutex);
//    vsnprintf(peso.msg, CCD_MSG_MAX, p_fmt, ap);
//    len = strlen(peso.msg);
//    snprintf(peso.msg + len, CCD_MSG_MAX - len, " %i: %s", errno,
//            strerror(errno));
//
//    //pthread_mutex_unlock(peso.p_global_mutex);
//    /* UNLOCK */
//
//    va_end(ap);
//
//    return 0;
//}

int controller_setup(void)
{
    //ArcCam_OpenByNameWithBuffer(fro_dev_list.szDevList[0], fro_buffer_size,
    //        &fro_status);
    ArcDevice_Open(0, &fro_status);

    if (fro_status != ARC_STATUS_OK)
    {
        ccd_save_error("Error: ArcCam_OpenByNameWithBuffer() failed: %s\n",
                ArcDevice_GetLastError());
        return -1;
    }

//    ArcCam_SetLogCmds(1);
//    while (ArcCam_GetLoggedCmdCount() > 0)
//    {
//        ArcCam_GetNextLoggedCmd();
//    }

    int dReset = 1;
    int dTDLs = 1;
    int dPowerOn = 1;

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO,
            "ArcDevice_SetupController(%i, %i, %i, %i, %i, %s, %s)\n", dReset,
            dTDLs, dPowerOn, peso.p_exposed_cfg->ccd.y2,
            peso.p_exposed_cfg->ccd.x2, peso.p_exposed_cfg->ccd_frodo.tim_file,
            peso.p_exposed_cfg->ccd_frodo.util_file);

    int abort = 0;
    ArcDevice_SetupController(dReset, dTDLs, dPowerOn, peso.p_exposed_cfg->ccd.y2,
            peso.p_exposed_cfg->ccd.x2, peso.p_exposed_cfg->ccd_frodo.tim_file,
            peso.p_exposed_cfg->ccd_frodo.util_file, "\0", &abort, &fro_status);

    if (fro_status != ARC_STATUS_OK)
    {
        ccd_save_error("Error: ArcDevice_SetupController() failed: %s\n",
                ArcDevice_GetLastError());
        return -1;
    }

    return 0;
}

int ccd_get_temp(double *p_temp)
{
    if (mod_ccd_check_state(peso.state) == -1)
    {
        return -1;
    }

    *p_temp = ArcDevice_GetArrayTemperature(&fro_status);

    if (fro_status != ARC_STATUS_OK)
    {
        ccd_save_error("Error: ArcCam_GetArrayTemperature() failed: %s\n",
                ArcDevice_GetLastError());
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
    peso.x1 = peso.p_exposed_cfg->ccd.x1;
    peso.x2 = peso.p_exposed_cfg->ccd.x2;
    peso.xb = peso.p_exposed_cfg->ccd.xb;
    peso.y1 = peso.p_exposed_cfg->ccd.y1;
    peso.y2 = peso.p_exposed_cfg->ccd.y2;
    peso.yb = peso.p_exposed_cfg->ccd.yb;
    peso.pixel_count_max = peso.x2 * peso.y2;
    peso.bits_per_pixel = peso.p_exposed_cfg->ccd.bits_per_pixel;

    fro_buffer_size = 2200 * 2200 * 2;

    fro_data_size = (peso.x2 - peso.x1 + 1) / peso.xb;
    fro_data_size *= (peso.y2 - peso.y1 + 1) / peso.yb;
    fro_data_size *= sizeof(unsigned short);

    if (p_fro_raw_data != NULL)
    {
        free(p_fro_raw_data);
        p_fro_raw_data = NULL;
    }
    if ((p_fro_raw_data = (unsigned short *) malloc(fro_data_size)) == NULL)
    {
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_ERROR, "Error: malloc() failed");
        return -1;
    }

    //ArcCam_GetDeviceList(&fro_dev_list, &fro_status);
    ArcDevice_FindDevices(&fro_status);

    //if (fro_status != ARC_STATUS_OK)
    //{
    //    ccd_save_error("Error: ArcDevice_FindDevices() failed: %s\n",
    //            ArcDevice_GetLastError());
    //    return -1;
    //}

    if (ArcDevice_DeviceCount() < 1)
    {
        ccd_save_error("Error: AstroPCI devices not found\n");
        return -1;
    }

    if (controller_setup() == -1)
    {
        return -1;
    }

//    while (ArcCam_GetLoggedCmdCount() > 0)
//    {
//        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "ASTROPCI => %s\n",
//                ArcCam_GetNextLoggedCmd());
//    }

    // TODO: otestovat
    //ArcDevice_SetArrayTemperature(peso.require_temp, &fro_status);

    //if (fro_status != ARC_STATUS_OK)
    //{
    //    ccd_save_error("Error: ArcCam_SetArrayTemperature(%f) failed: %s\n",
    //            peso.require_temp, ArcDevice_GetLastError());
    //    return -1;
    //}

    ccd_get_temp(&peso.actual_temp);

    return 0;
}

int ccd_uninit(void)
{
    ArcDevice_Close();

    free(p_fro_raw_data);
    p_fro_raw_data = NULL;

    return 0;
}

int ccd_expose_init(void)
{
    peso_set_int(&peso.readout_time, 30);

    return 0;
}

//void expose_callback(float elapsed_time)
//{
//    peso_set_int(&peso.elapsed_time, (int) elapsed_time);
//}
//
//void read_callback(int pixel_count)
//{
//    time_t actual_time;
//
//    if (!fro_readout)
//    {
//        // TODO: reportovat pripadnou chybu
//        /* Close shutter */
//        ArcCam_Cmd1Arg(TIM_ID, CSH, -1, &fro_status);
//        fro_readout = 1;
//    }
//
//    (void) time(&actual_time);
//    peso_set_int(&peso.elapsed_time, actual_time - peso.stop_exposure_time);
//}

//static void *fro_expose_loop(void *arg)
//{
//    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO,
//            "fro_expose_loop() starting");
//
//    ArcCam_Expose(peso.exptime, peso.p_exposed_cfg->ccd.y2,
//            peso.p_exposed_cfg->ccd.x2, &fro_abort, expose_callback,
//            read_callback, peso.shutter, &fro_status);
//
//    // TODO: zalogovat chybu
//    //if (!fro_status.dSuccess) {
//    //    ccd_save_error("Error: ArcCam_Expose() failed: %s\n", fro_status.szMessage);
//    //    return -1;
//    //}
//
//    fro_expose = 0;
//    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO,
//            "fro_expose_loop() exiting");
//
//    pthread_exit(0);
//    return NULL;
//}

int ccd_expose_start(void)
{
    pthread_t expose_pthread;
    fro_abort = 0;
    fro_readout = 0;
    fro_expose = 1;
    fro_readout_set = 0.0;

//    if (pthread_create(&expose_pthread, NULL, fro_expose_loop, NULL) != 0)
//    {
//        ccd_save_error("Error: pthread_create(fro_expose_loop): %i: %s", errno,
//                strerror(errno));
//        return -1;
//    }

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "ccd_expose_start SET");
    ArcDevice_Command_I(TIM_ID, SET, peso.exptime * 1000, &fro_status);
    if (fro_status != ARC_STATUS_OK)
    {
        ccd_save_error("Error: ArcDevice_Command_I(TIM_ID, SET, %d) failed: %s\n",
                peso.exptime, ArcDevice_GetLastError());
        return -1;
    }

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "ccd_expose_start SHUTTER");
    ArcDevice_SetOpenShutter(peso.shutter, &fro_status);
    if (fro_status != ARC_STATUS_OK)
    {
        ccd_save_error("Error: ArcDevice_SetOpenShutter(%d) failed: %s\n",
                peso.shutter, ArcDevice_GetLastError());
        return -1;
    }

    //log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "ArcCam_SetOpenShutter(%d)", peso.shutter);

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "ccd_expose_start SEX");
    ArcDevice_Command_I(TIM_ID, SEX, -1, &fro_status);
    if (fro_status != ARC_STATUS_OK)
    {
        ccd_save_error("Error: ArcDevice_Command_I(TIM_ID, SEX, -1) failed: %s\n",
                ArcDevice_GetLastError());
        return -1;
    }

    return 0;
}

int ccd_expose(void)
{
    int elapsed_time;
    int is_readout;

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "ccd_expose IS_READOUT");
    is_readout = ArcDevice_IsReadout(&fro_status);
    if (fro_status != ARC_STATUS_OK)
    {
        ccd_save_error("Error: ArcDevice_IsReadout() failed: %s\n",
                ArcDevice_GetLastError());
        // TODO
        //return -1;
    }

    //ArcCam_GetPixelCount(&fro_status);

    //log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "is_readout = %d\n", is_readout);

    if (is_readout)
    {
        fro_readout = 1;
        /* expose = false */
        return 0;
    }

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "ccd_expose ELAPSED_TIME");
    elapsed_time = ArcDevice_Command_I(TIM_ID, RET, -1, &fro_status);
    if (fro_status != ARC_STATUS_OK) 
    {
        ccd_save_error("Error: ArcDevice_Command_I(TIM_ID, RET, -1) failed: %s\n",
                ArcDevice_GetLastError());
        // TODO
        //return -1;
    }

    peso_set_int(&peso.elapsed_time, elapsed_time / 1000.);

    /* ignore peso.exptime_update */
    if ((peso.exptime_update != 0)
            && (((peso.exptime_update - peso.elapsed_time) <= 10)
                    || ((peso.exptime - peso.elapsed_time) <= 10)))
    {
        /* LOCK */
        pthread_mutex_lock(peso.p_global_mutex);
        peso.exptime_update = 0;
        pthread_mutex_unlock(peso.p_global_mutex);
        /* UNLOCK */

        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "ignore peso.exptime_update");
    }

    if (peso.abort != 0)
    {
        fro_abort = 1;
        peso_set_int(&peso.abort, 2);
        /* expose = false */
        return 0;
    }
    else if (peso.exptime_update != 0)
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

        if (!peso.readout)
        {
            /* Set Exposure Time in milliseconds */
            ArcDevice_Command_I(TIM_ID, SET, peso.exptime * 1000.0, &fro_status);
            log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "TIM_ID SET %i", peso.exptime);

            if (fro_status != ARC_STATUS_OK)
            {
                ccd_save_error(
                        "Error: ArcDevice_Command_I(TIM_ID, SET, 0) failed: %s\n",
                        ArcDevice_GetLastError());
            }
        }
    }

    if ((peso.readout) && (fro_readout_set < (elapsed_time - 1000.0)))
    {
        fro_readout_set = elapsed_time + 1000.0;

        /* Set Exposure Time in milliseconds */
        ArcDevice_Command_I(TIM_ID, SET, fro_readout_set, &fro_status);
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO,
                "readout = 1, TIM_ID SET %f", fro_readout_set);

        if (fro_status != ARC_STATUS_OK)
        {
            ccd_save_error("Error: ArcDevice_Command_I(TIM_ID, SET, 0) failed: %s\n",
                    ArcDevice_GetLastError());
            // TODO
            //return -1;
        }

//        fro_readout = 1;
//        /* expose = false */
//        return 0;
    }

    /* expose = true */
    return 1;
}

int ccd_readout(void)
{
    int pixel_count;
    time_t actual_time;

    //ArcCam_IsReadout(&fro_status);

//    while (ArcCam_GetLoggedCmdCount() > 0)
//    {
//        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "ASTROPCI => %s\n",
//                ArcCam_GetNextLoggedCmd());
//    }

    (void) time(&actual_time);
    peso_set_int(&peso.elapsed_time, actual_time - peso.stop_exposure_time);

    pixel_count = ArcDevice_GetPixelCount(&fro_status);
    if (fro_status != ARC_STATUS_OK)
    {
        ccd_save_error("Error: ArcDevice_GetPixelCount() failed: %s\n",
                ArcDevice_GetLastError());
        // TODO
        //return -1;
    }

    //log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "pixel_count = %i\n", pixel_count);

    if (pixel_count < (peso.p_exposed_cfg->ccd.y2 * peso.p_exposed_cfg->ccd.x2))
    {
        /* readout = true */
        return 1;
    }

    //ArcCam_Deinterlace(ArcCam_CommonBufferVA(&fro_status),
    //        peso.p_exposed_cfg->ccd.y2, peso.p_exposed_cfg->ccd.x2,
    //        DEINTERLACE_NONE, &fro_status);

    //swap_memory(fro_mem_fd, peso.pixel_count_max * (peso.bits_per_pixel / 8));

    /* readout = false */
    return 0;
}

int ccd_save_raw_image(void)
{
    FILE *fw;

    /* peso.raw_image not lock */
    if ((fw = fopen(peso.raw_image, "w")) == NULL)
    {
        return -1;
    }

    fwrite(ArcDevice_CommonBufferVA(&fro_status), fro_data_size, 1, fw);

    if (fclose(fw) == EOF)
    {
        return -1;
    }

    return 0;
}

int ccd_save_fits_file(fitsfile *p_fits, int *p_fits_status)
{
    unsigned short *p_from;
    unsigned short *p_morf;
    unsigned short *p_data = ArcDevice_CommonBufferVA(&fro_status);
    long fpixel = 1;
    // TODO: pripravit a otestovat i pro x1, xb, y1, yb
    long nelements = peso.x2 * peso.y2;

    /*
     *  Ukazatele p_fro_raw_data a p_data je treba pred prictenim
     *  fro_data_size pretypovat na (char *), protoze fro_data_size
     *  uvadi velikost jiz vynasobenou sizeof(unsigned short)
     */
    //p_morf = (unsigned short *) ((char *) p_fro_raw_data + fro_data_size);
    //p_from = (unsigned short *) ((char *) p_data + fro_data_size);

    /* swap bytes */
    //while (p_from > p_data)
    //{
    //    unsigned short left;
    //    unsigned short right;

    //    left = *p_from >> 8;
    //    right = *p_from << 8;
    //    *p_morf = left | right;

    //    p_from--;
    //    p_morf--;
    //}

    //if (fits_write_img(p_fits, TUSHORT, fpixel, nelements,
    //        p_fro_raw_data, p_fits_status))
    //{
    //    return -1;
    //}

    if (fits_write_img(p_fits, TUSHORT, fpixel, nelements,
            ArcDevice_CommonBufferVA(&fro_status), p_fits_status))
    {
        return -1;
    }

    return 0;
}

int ccd_expose_end(void)
{
    if (!fro_readout)
    {
        ArcDevice_StopExposure(&fro_status);

        if (fro_status != ARC_STATUS_OK)
        {
            ccd_save_error("Error: ArcDevice_StopExposure() failed: %s\n",
                    ArcDevice_GetLastError());
            return -1;
        }
    }

    return 0;
}

int ccd_expose_uninit(void)
{
    if (fro_abort) {
        ArcDevice_StopExposure(&fro_status);

        if (fro_status != ARC_STATUS_OK)
        {
            ccd_save_error("Error: ArcDevice_StopExposure() failed: %s\n",
                    ArcDevice_GetLastError());
            return -1;
        }
    }

    return 0;
}

int ccd_set_temp(double temp)
{
    if (mod_ccd_check_state(peso.state) == -1)
    {
        return -1;
    }

    peso.require_temp = temp;

    ArcDevice_SetArrayTemperature(temp, &fro_status);

    if (fro_status != ARC_STATUS_OK)
    {
        ccd_save_error("Error: ArcDevice_SetArrayTemperature(%f) failed: %s\n",
                temp, ArcDevice_GetLastError());
        return -1;
    }

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

    for (i = 0; i < FRO_SPEED_MAX_E; ++i)
    {
        if (!strcmp(p_speed, fro_speed_str2enum[i].str))
        {
            peso.readout_speed = fro_speed_str2enum[i].speed_e;
            return 0;
        }
    }

    return -1;
}

const char *ccd_get_readout_speed(void)
{
    return fro_speed_str2enum[peso.readout_speed].str;
}

const char *ccd_get_readout_speeds(void)
{
    int i;
    int size = 0;

    if (ccd_readout_speeds[0] == '\0')
    {
        for (i = 0; i < FRO_SPEED_MAX_E; ++i)
        {
            strncat(ccd_readout_speeds, fro_speed_str2enum[i].str,
                    PESO_READOUT_SPEEDS_MAX - size - 1);
            strcat(ccd_readout_speeds, ";");
            size += strlen(fro_speed_str2enum[i].str);
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

#ifdef SELF_TEST_FRODO

int main(int argc, char *argv[])
{
    pthread_mutex_t global_mutex;

    if (pthread_mutex_init(&global_mutex, NULL) != 0)
    {
        printf("pthread_mutex_init() failed\n");
        exit(EXIT_FAILURE);
    }

    if (cfg_load("/opt/exposed/etc/exposed-frodo.cfg", &exposed_cfg, "frodo"))
    {
        printf("cfg_load() failed\n");
        exit(EXIT_FAILURE);
    }

    p_logcat = log4c_category_get("exposed");
    peso.p_global_mutex = &global_mutex;
    peso.p_exposed_cfg = &exposed_cfg;
    peso.expcount = 1;
    peso.expnum = 1;
    peso.p_logcat = p_logcat;
    peso.state = CCD_STATE_READY_E;
    strcpy(peso.raw_image, "/tmp/frodo.fit");

    ccd_init();

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
}

#endif
