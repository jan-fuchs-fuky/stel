#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#define LINUX
typedef int HANDLE;

#include "../frodo/Driver.h"
#include "../frodo/DSPCommand.h"
#include "../frodo/Memory.h"
#include "../frodo/LoadDspFile.h"
#include "../frodo/Temperature.h"

#include "../frodo/ArcCameraAPI.h"

#define PCI_DEVICE_NAME         "/dev/astropci0\0"
#define TIME_FILE               "/opt/astro-cam/tim_slow.lod\0"
#define UTIL_FILE               "/opt/astro-cam/util.lod\0"

struct ArcCAPIDevList fro_dev_list;
struct ArcCAPIStatus fro_status;

static void check_err() {
    if (!fro_status.dSuccess)
    {
        printf("Error: %s\n", fro_status.szMessage);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    int fro_buffer_size = 2200 * 2200 * 2;
    int dReset = 1;
    int dTDLs = 1;
    int dPowerOn = 1;
    int cols = 2720;
    int rows = 512;
    int elapsed_time;
    int is_readout;
    int pixel_count;

    fro_status.dSuccess = 1;

    printf("ArcCam_GetDeviceList()\n");
    ArcCam_GetDeviceList(&fro_dev_list, &fro_status);
    check_err();

    if (fro_dev_list.dDevCount < 1)
    {
        printf("Error: AstroPCI devices not found\n");
        return -1;
    }

    printf("ArcCam_OpenByNameWithBuffer()\n");
    ArcCam_OpenByNameWithBuffer(fro_dev_list.szDevList[0], fro_buffer_size,
            &fro_status);
    check_err();

    printf("ArcCam_SetLogCmds()\n");
    ArcCam_SetLogCmds(1);

    while (ArcCam_GetLoggedCmdCount() > 0)
    {
        printf("OLD CMD => %s\n", ArcCam_GetNextLoggedCmd());
    }

    printf("ArcCam_SetupController()\n");
    ArcCam_SetupController(dReset, dTDLs, dPowerOn, rows, cols, TIME_FILE,
            UTIL_FILE, NULL, NULL, &fro_status);
    check_err();

    printf("ArcCam_Cmd1Arg()\n");
    ArcCam_Cmd1Arg(TIM_ID, SET, 5 * 1000, &fro_status);
    check_err();

    printf("ArcCam_SetOpenShutter()\n");
    ArcCam_SetOpenShutter(1, &fro_status);
    check_err();

    printf("ArcCam_Cmd1Arg()\n");
    ArcCam_Cmd1Arg(TIM_ID, SEX, -1, &fro_status);
    check_err();

    while (ArcCam_GetLoggedCmdCount() > 0)
    {
        printf("CMD => %s\n", ArcCam_GetNextLoggedCmd());
    }

    pixel_count = 0;
    while (pixel_count < (cols * rows)) {

        printf("ArcCam_IsReadout()\n");
        is_readout = ArcCam_IsReadout(&fro_status);
        check_err();

        printf("ArcCam_GetPixelCount()\n");
        pixel_count = ArcCam_GetPixelCount(&fro_status);
        check_err();
        printf("pixel_count = %i\n", pixel_count);

        if (!is_readout)
        {
            printf("ArcCam_Cmd1Arg()\n");
            elapsed_time = ArcCam_Cmd1Arg(TIM_ID, RET, -1, &fro_status);
            check_err();
            printf("elapsed_time = %i\n", elapsed_time);
        }

        sleep(1);
    }

    while (ArcCam_GetLoggedCmdCount() > 0)
    {
        printf("CMD => %s\n", ArcCam_GetNextLoggedCmd());
    }

    ArcCam_Close();
    return 0;
}
