/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 * $URL$
 */

#ifndef __MODULES_H
#define __MODULES_H

#include <time.h>
#include <pthread.h>
#include <log4c.h>

#include "cfg.h"

#define CCD_EXPTIME_MAX         (60*60*10) // 10 hours
#define CCD_MSG_MAX             1023
#define PESO_PATH_MAX           1023
#define PESO_READOUT_SPEED_MAX  15
#define PESO_READOUT_SPEEDS_MAX (10 * PESO_READOUT_SPEED_MAX)
#define PESO_GAIN_MAX           15
#define PESO_GAINS_MAX          (10 * PESO_GAIN_MAX)

typedef struct
{
    int (*init)();
    int (*uninit)();
    int (*expose_init)();
    int (*expose_start)();
    int (*expose)();
    int (*readout)();
    int (*save_raw_image)();
    int (*save_fits_file)();
    int (*expose_end)();
    int (*expose_uninit)();
    int (*get_temp)();
    int (*set_temp)();
    int (*set_readout_speed)();
    int (*set_gain)();

    void (*peso_set_int)();
    void (*peso_get_int)();
    void (*peso_set_double)();
    void (*peso_set_float)();
    void (*peso_get_float)();
    void (*peso_set_str)();
    void (*peso_set_time)();
    void (*peso_set_imgtype)();
    void (*peso_set_state)();

    const char *(*peso_get_version)();

    const char *(*get_readout_speed)();
    const char *(*get_readout_speeds)();
    const char *(*get_gain)();
    const char *(*get_gains)();
} MOD_CCD_T;

typedef enum
{
    CCD_IMGTYPE_UNKNOWN_E,
    CCD_IMGTYPE_FLAT_E,
    CCD_IMGTYPE_COMP_E,
    CCD_IMGTYPE_ZERO_E,
    CCD_IMGTYPE_DARK_E,
    CCD_IMGTYPE_TARGET_E,
} CCD_IMGTYPE_T;

typedef enum
{
    CCD_STATE_UNKNOWN_E,
    CCD_STATE_READY_E,
    CCD_STATE_PREPARE_EXPOSE_E,
    CCD_STATE_EXPOSE_E,
    CCD_STATE_FINISH_EXPOSE_E,
    CCD_STATE_READOUT_E,
} CCD_STATE_T;

typedef enum
{
    CCD_SPEED_SLOW_E, CCD_SPEED_FAST_E,
} CCD_SPEED_T;

typedef struct
{
    EXPOSED_CFG_T *p_exposed_cfg;
    int archive;
    int addtime; // TODO: odstranit
    int abort;
    int readout;
    int exptime;
    int exptime_update;
    int expcount;
    int expmeter;
    int expmeter_update;
    int expmeter_id;
    int expmeter_shutter_id;
    int spectemp_id;
    int camfocus_id;
    int expnum;
    int shutter;
    int elapsed_time;
    int readout_time;
    int x1;
    int x2;
    int xb;
    int y1;
    int y2;
    int yb;
    int bits_per_pixel;
    int pixel_count_max;
    int byte_swapping;
    int readout_speed;
    int gain;
    int *p_hdr_camfocus;
    int *p_hdr_spectemp;
    double actual_temp;
    double require_temp;
    CCD_IMGTYPE_T imgtype;
    CCD_STATE_T state;
    char msg[CCD_MSG_MAX + 1];
    char raw_image[PESO_PATH_MAX + 1];
    char fits_file[PESO_PATH_MAX + 1];
    char path[PESO_PATH_MAX + 1];
    char archive_path[PESO_PATH_MAX + 1];
    time_t start_exposure_time;
    time_t stop_exposure_time;
    pthread_mutex_t *p_global_mutex;
    log4c_category_t *p_logcat;
} PESO_T;

extern PESO_T peso;

int mod_ccd_init(char *p_mod_ccd_path, MOD_CCD_T *p_mod_ccd, PESO_T **p_peso);
int mod_ccd_uninit(void);

#endif
