/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 * $URL$
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
#include "mod_ccd_gandalf.h"

#include "gandalf/picam.h"

#ifdef SELF_TEST_GANDALF

log4c_category_t *p_logcat = NULL;

#endif

PESO_T peso;

static char ccd_readout_speeds[PESO_READOUT_SPEEDS_MAX + 1];
static char gan_ccd_gains[PESO_GAINS_MAX + 1];
static PicamHandle gan_camera;
static PicamCameraID gan_camera_id;
static PicamAvailableData gan_available_data;
static unsigned short *p_raw_data = NULL;
static unsigned short *p_raw_data_reverse = NULL;

static unsigned long gan_data_size;

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

static int ccd_commit_parameters(void)
{
    PicamError error;
    int committed;

    error = Picam_AreParametersCommitted(gan_camera, &committed);
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Picam_AreParametersCommitted(): => %i [committed = %i]",
        error, committed);

    const PicamParameter *p_failed_parameters;
    int failed_parameters_count;

    error = Picam_CommitParameters(
        gan_camera,
        &p_failed_parameters,
        &failed_parameters_count
    );
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Picam_CommitParameters(): => %i [failed_parameters_count = %i]",
        error, failed_parameters_count);

    Picam_DestroyParameters(p_failed_parameters);
    return 0;
}

int ccd_get_temp(double *p_temp)
{
    PicamError error;

    error = Picam_ReadParameterFloatingPointValue(
        gan_camera,
        PicamParameter_SensorTemperatureReading,
        p_temp
    );

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "PicamParameter_SensorTemperatureReading => %i [temp = %f]", error, *p_temp);

    return 0;
}

int ccd_set_temp(double temp)
{
    PicamError error;

    peso.require_temp = temp;

    error = Picam_SetParameterFloatingPointValue(
        gan_camera,
        PicamParameter_SensorTemperatureSetPoint,
        temp
    );

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "PicamParameter_SensorTemperatureSetPoint => %i [temp = %f]", error, temp);
    ccd_commit_parameters();

    return 0;
}

int ccd_init(void)
{
    int i;
    PicamError error;

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

    //gan_buffer_size = 2200 * 2200 * 2;

    gan_data_size = (peso.x2 - peso.x1 + 1) / peso.xb;
    gan_data_size *= (peso.y2 - peso.y1 + 1) / peso.yb;
    gan_data_size *= sizeof(unsigned short);

    //if (p_gan_raw_data != NULL)
    //{
    //    free(p_gan_raw_data);
    //    p_gan_raw_data = NULL;
    //}
    //if ((p_gan_raw_data = (unsigned short *) malloc(gan_data_size)) == NULL)
    //{
    //    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_ERROR, "Error: malloc() failed");
    //    return -1;
    //}

    //ArcDevice_FindDevices(&gan_status);
    //if (controller_setup() == -1)
    //ccd_get_temp(&peso.actual_temp);

    piint major;
    piint minor;
    piint distribution;
    piint released;

    Picam_GetVersion(&major, &minor, &distribution, &released);

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "picam version %i.%i.%i.%i",
        major, minor, distribution, released);

    pibln inited;
    gan_camera = NULL;

    Picam_IsLibraryInitialized(&inited);

    if (inited) {
        error = Picam_UninitializeLibrary();
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_WARN, "Picam_UninitializeLibrary() => %i", error);
    }

    error = Picam_InitializeLibrary();
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Picam_InitializeLibrary() => %i", error);
    
    for (i = 0; i < 6; ++i) {
        error = Picam_OpenFirstCamera(&gan_camera);
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Picam_OpenFirstCamera() => %i", error);

        if (error != 0) {
            if (i >= 5) {
                log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_ERROR, "Camera not opened");
                return -1;
            }

            sleep(10);
        }
        else {
            break;
        }
    }

    error = Picam_GetCameraID(gan_camera, &gan_camera_id);
    if (error == 0) {
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "model=%i, computer_interface=%i, sensor_name=%s, serial_number=%s",
            gan_camera_id.model, gan_camera_id.computer_interface,
            gan_camera_id.sensor_name, gan_camera_id.serial_number);
    }
    else {
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Picam_GetCameraID() failed => %i", error);
        return -1;
    }

    ccd_set_temp(peso.require_temp);
    ccd_get_temp(&peso.actual_temp);

    return 0;
}

int ccd_uninit(void)
{
    if (gan_camera != NULL) {
        Picam_CloseCamera(gan_camera);
    }

    Picam_UninitializeLibrary();

    //free(p_gan_raw_data);
    //p_gan_raw_data = NULL;

    return 0;
}

int ccd_expose_init(void)
{
    PicamError error;

    peso_set_int(&peso.readout_time, 28);

    //shutter = (peso.shutter) ? OPEN_PRE_TRIGGER : OPEN_NEVER;

    gan_data_size = (peso.x2 - peso.x1 + 1) / peso.xb;
    gan_data_size *= (peso.y2 - peso.y1 + 1) / peso.yb;
    gan_data_size *= 2;

    if (p_raw_data != NULL)
    {
        free(p_raw_data);
        p_raw_data = NULL;
    }
    if ((p_raw_data = (unsigned short *) malloc(gan_data_size)) == NULL)
    {
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Error: malloc()");
        return -1;
    }

    if (p_raw_data_reverse != NULL)
    {
        free(p_raw_data_reverse);
        p_raw_data_reverse = NULL;
    }
    if ((p_raw_data_reverse = (unsigned short *) malloc(gan_data_size)) == NULL)
    {
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Error: malloc()");
        return -1;
    }

    int running = 1;
    error = Picam_IsAcquisitionRunning(gan_camera, &running);
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Picam_IsAcquisitionRunning(): => %i [running = %i]", error, running);

    double speed = peso.readout_speed / 1000.0; // MHz
    int settable;

    error = Picam_CanSetParameterFloatingPointValue(
        gan_camera,
        PicamParameter_AdcSpeed,
        speed,
        &settable
    );
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "PicamParameter_AdcSpeed => %i [settable = %i]", error, settable);

    error = Picam_SetParameterFloatingPointValue(
        gan_camera,
        PicamParameter_AdcSpeed,
        speed
    );
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "PicamParameter_AdcSpeed => %i", error);
    if (error != 0) {
        return -1;
    }

    int parameter_shutter;
    if (peso.shutter == 1) {
        parameter_shutter = 1;
    }
    else {
        parameter_shutter = 2;
    }

    error = Picam_SetParameterIntegerValue(
        gan_camera,
        PicamParameter_ShutterTimingMode,
        parameter_shutter
    );
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "PicamParameter_ShutterTimingMode => %i [shutter = %i]", error, parameter_shutter);

    int parameter_gain = peso.gain;

    error = Picam_SetParameterIntegerValue(
        gan_camera,
        PicamParameter_AdcAnalogGain,
        parameter_gain
    );
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "PicamParameter_AdcAnalogGain => %i [gain = %i]", error, parameter_gain);

    int64_t readout_count = 1;

    error = Picam_SetParameterLargeIntegerValue(
        gan_camera,
        PicamParameter_ReadoutCount,
        readout_count
    );
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "PicamParameter_ReadoutCount => %i", error);

    error = Picam_SetParameterIntegerValue(
        gan_camera,
        PicamParameter_TriggerResponse,
        PicamTriggerResponse_ExposeDuringTriggerPulse
    );
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "PicamParameter_TriggerResponse => %i", error);

    // Positive or negative edge of trigger pulse
    error = Picam_SetParameterIntegerValue(
        gan_camera,
        PicamParameter_TriggerDetermination,
        PicamTriggerDetermination_NegativePolarity
    );
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "PicamParameter_TriggerDetermination => %i", error);

    error = Picam_SetParameterIntegerValue(
        gan_camera,
        PicamParameter_OutputSignal,
        PicamOutputSignal_AlwaysHigh
    );
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "PicamParameter_OutputSignal => %i", error);

    double exptime = -1;

    error = Picam_SetParameterFloatingPointValue(
        gan_camera,
        PicamParameter_ExposureTime,
        exptime
    );
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "PicamParameter_ExposureTime => %i", error);

    ccd_commit_parameters();

    return 0;
}

static int ccd_set_ttl_out(int value)
{
    // TODO
    return 0;
}

int ccd_expose_start(void)
{
    PicamError error;

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "ccd_expose_start()");

    error = Picam_StartAcquisition(gan_camera);
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Picam_StartAcquisition(): => %i", error);

    system("/opt/bin/gandalf_set_ttl_out.py 255");
    return 0;
}

int ccd_expose(void)
{
    PicamError error;
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
        // TODO: Pokud se pouzije nasledujici kod, tak pri spusteni dalsi
        // expozice nastane deadlock v Picam_StartAcquisition()
        //peso_set_int(&peso.abort, 2);

        //error = Picam_StopAcquisition(gan_camera);
        //log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Picam_StopAcquisition(): => %i", error);

        //system("/opt/bin/gandalf_set_ttl_out.py 0");

        //int readout_time_out = 1500; // miliseconds
        //PicamAcquisitionStatus status;

        //error = Picam_WaitForAcquisitionUpdate(
        //    gan_camera,
        //    readout_time_out,
        //    &gan_available_data,
        //    &status
        //);

        //log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Picam_WaitForAcquisitionUpdate(): => %i [readout_count = %i, running = %i]",
        //    error, gan_available_data.readout_count, status.running);

        system("/opt/bin/gandalf_set_ttl_out.py 0");
        return 0;
    }
    else if (peso.readout)
    {
        system("/opt/bin/gandalf_set_ttl_out.py 0");
        return 0;
    }

    (void) time(&actual_time);

    peso_set_int(&peso.elapsed_time, actual_time - peso.start_exposure_time);

    if (peso.elapsed_time >= peso.exptime)
    {
        system("/opt/bin/gandalf_set_ttl_out.py 0");
        return 0;
    }

    /* exposing */
    return 1;
}

int ccd_readout(void)
{
    PicamError error;
    time_t actual_time;

    (void) time(&actual_time);
    peso_set_int(&peso.elapsed_time, actual_time - peso.stop_exposure_time);

    int readout_time_out = 100; // miliseconds
    PicamAcquisitionStatus status;

    error = Picam_WaitForAcquisitionUpdate(
        gan_camera,
        readout_time_out,
        &gan_available_data,
        &status
    );

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Picam_WaitForAcquisitionUpdate(): => %i [readout_count = %i, running = %i]",
        error, gan_available_data.readout_count, status.running);

    int running = 1;
    error = Picam_IsAcquisitionRunning(gan_camera, &running);
    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "Picam_IsAcquisitionRunning(): => %i [running = %i]", error, running);

    if (((error == 0) || (error == 32)) && (gan_available_data.readout_count != 1)) {
        return 1;
    }
    else if (gan_available_data.readout_count == 1) {
        /* readout = false */
        return 0;
    }

    return -1;
}

int ccd_save_raw_image(void)
{
    FILE *fw;

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_INFO, "save_raw_image()");

    // TODO
    gan_data_size = 2048*512*2;

    /* peso.raw_image not lock */
    if ((fw = fopen(peso.raw_image, "w")) == NULL)
    {
        return -1;
    }

    // TODO: kontrolovat meze
    memcpy(p_raw_data, gan_available_data.initial_readout, sizeof(int16_t) * (2048 * 512));
    fwrite(p_raw_data, sizeof(int16_t), (2048 * 512), fw);

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
    free(p_raw_data_reverse);
    p_raw_data_reverse = NULL;

    free(p_raw_data);
    p_raw_data = NULL;

    ccd_readout();
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
    int size = 0;

    if (ccd_readout_speeds[0] == '\0')
    {
        for (i = 0; i < GAN_SPEED_MAX_E; ++i)
        {
            strncat(ccd_readout_speeds, gan_speed_str2enum[i].str,
                    PESO_READOUT_SPEEDS_MAX - size - 1);
            strcat(ccd_readout_speeds, ";");
            size += strlen(gan_speed_str2enum[i].str);
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

#ifdef SELF_TEST_GANDALF

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
    log4c_fini();
}

#endif
