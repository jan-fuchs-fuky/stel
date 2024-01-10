#ifndef _ARCCAM_API_H_
#define _ARCCAM_API_H_

#include "DllMain.h"

#ifdef linux
#include <sys/types.h>
#endif

// +------------------------------------------------------------------------------------+
// | Define C function status. This structure is filled with the proper data when
// | an exception is caught in a CController method.  This structure should always be
// | checked after a function call. dSuccess = 1 if the function call was successful;
// | 0 otherwise and szMessage will contain the message from the exception.
// +------------------------------------------------------------------------------------+
#define STATUS_MSG_SIZE		512

struct ArcCAPIStatus
{
    int dSuccess;
    char szMessage[STATUS_MSG_SIZE];
};

// +------------------------------------------------------------------------------------+
// | Device list. This structure is filled with the current device list containing both
// | PCI and PCIe devices. dDevCount contains the number of devices found in the system.
// | szDevList is an array of device strings that can be used to open a device.
// +------------------------------------------------------------------------------------+
#define DEV_NAME_SIZE		100
#define DEV_LIST_SIZE		25

struct ArcCAPIDevList
{
    int dDevCount;
    char szDevList[DEV_LIST_SIZE][DEV_NAME_SIZE];
};

// +------------------------------------------------------------------------------------+
// |  Data Types
// +------------------------------------------------------------------------------------+
#ifndef linux
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef unsigned char ubyte;
typedef unsigned int uint;
#endif

#ifdef __cplusplus
extern "C"
{ // Using a C++ compiler
#endif

// +------------------------------------------------------------------------------------+
// |  Deinterlace Alogrithms
// +------------------------------------------------------------------------------------+
ARCCAM_API extern const int DEINTERLACE_NONE;
ARCCAM_API extern const int DEINTERLACE_PARALLEL;
ARCCAM_API extern const int DEINTERLACE_SERIAL;
ARCCAM_API extern const int DEINTERLACE_CCD_QUAD;
ARCCAM_API extern const int DEINTERLACE_IR_QUAD;
ARCCAM_API extern const int DEINTERLACE_CDS_IR_QUAD;
ARCCAM_API extern const int DEINTERLACE_HAWAII_RG;
ARCCAM_API extern const int DEINTERLACE_STA1600;

// +============================================================================+
// | CArcDevice functions                                                       |
// +============================================================================+

//  Device access
// +-------------------------------------------------+
ARCCAM_API void ArcCam_GetDeviceList(struct ArcCAPIDevList* pDevList,
        struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_IsOpen(struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_Open(int dDevNum, struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_OpenWithBuffer(int dDevNum, int dBytes,
        struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_OpenByName(const char* pszDeviceName,
        struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_OpenByNameWithBuffer(const char* pszDeviceName,
        int dBytes, struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_Close();
ARCCAM_API void ArcCam_Reset(struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_MapCommonBuffer(int dBytes,
        struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_UnMapCommonBuffer(struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_ReMapCommonBuffer(int dBytes,
        struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_GetCommonBufferProperties(struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_FillCommonBuffer(unsigned short u16Value,
        struct ArcCAPIStatus* pStatus);
ARCCAM_API void* ArcCam_CommonBufferVA(struct ArcCAPIStatus* pStatus);
ARCCAM_API ulong ArcCam_CommonBufferPA(struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_CommonBufferSize(struct ArcCAPIStatus* pStatus);

ARCCAM_API int ArcCam_GetId(struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_GetStatus(struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_ClearStatus(struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_Set2xFOTransmitter(int dOnOff,
        struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_LoadDeviceFile(const char* pszFile,
        struct ArcCAPIStatus* pStatus);

//  Setup & General commands
// +-------------------------------------------------+
ARCCAM_API int ArcCam_Command(int dBoardId, int dCommand, int dArg1, int dArg2,
        int dArg3, int dArg4, struct ArcCAPIStatus* pStatus);

ARCCAM_API int ArcCam_Cmd1Arg(int dBoardId, int dCommand, int dArg1,
        struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_Cmd2Arg(int dBoardId, int dCommand, int dArg1, int dArg2,
        struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_Cmd3Arg(int dBoardId, int dCommand, int dArg1, int dArg2,
        int dArg3, struct ArcCAPIStatus* pStatus);

ARCCAM_API int ArcCam_GetControllerId(struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_ResetController(struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_IsControllerConnected(struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_SetupController(int dReset, int dTdl, int dPower,
        int dRows, int dCols, const char* pszTimFile, const char* pszUtilFile,
        const char* pszPciFile, int* pAbort, struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_LoadControllerFile(const char* pszFilename,
        int dValidate, int* pAbort, struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_SetImageSize(int dRows, int dCols,
        struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_GetImageRows(struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_GetImageCols(struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_GetCCParams(struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_IsCCParamSupported(int dParameter,
        struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_IsCCD(struct ArcCAPIStatus* pStatus);

ARCCAM_API int ArcCam_IsBinningSet(struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_SetBinning(int dRows, int dCols, int dRowFactor,
        int dColFactor, int* pBinRows, int* pBinCols,
        struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_UnSetBinning(int dRows, int dCols,
        struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_SetSubArray(int* pOldRows, int* pOldCols, int dRow,
        int dCol, int dSubRows, int dSubCols, int dBiasOffset, int dBiasWidth,
        struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_UnSetSubArray(int dRows, int dCols,
        struct ArcCAPIStatus* pStatus);

ARCCAM_API int ArcCam_IsSyntheticImageMode(struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_SetSyntheticImageMode(int dMode,
        struct ArcCAPIStatus* pStatus);

//  Expose commands
// +-------------------------------------------------+
ARCCAM_API void ArcCam_SetOpenShutter(int dShouldOpen,
        struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_Expose(float fExpTime, int dRows, int dCols, int* pAbort,
        void (*pExposeCall)(float), void (*pReadCall)(int), int dOpenShutter,
        struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_StopExposure(struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_Continuous(int dRows, int dCols, int dNumOfFrames,
        float fExpTime, int* pAbort,
        void (*pFrameCall)(int, int, int, int, void *), int dOpenShutter,
        struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_StopContinuous(struct ArcCAPIStatus* pStatus);

ARCCAM_API int ArcCam_IsReadout(struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_GetPixelCount(struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_GetCRPixelCount(struct ArcCAPIStatus* pStatus);
ARCCAM_API int ArcCam_GetFrameCount(struct ArcCAPIStatus* pStatus);

//  Error & Degug message access
// +-------------------------------------------------+
ARCCAM_API int ArcCam_ContainsError(int dWord);
ARCCAM_API int ArcCam_ContainsMinMaxError(int dWord, int dWordMin, int dWordMax);

ARCCAM_API const char* ArcCam_GetNextLoggedCmd();
ARCCAM_API int ArcCam_GetLoggedCmdCount();
ARCCAM_API void ArcCam_SetLogCmds(int dOnOff);

//  Temperature control
// +-------------------------------------------------+
ARCCAM_API double ArcCam_GetArrayTemperature(struct ArcCAPIStatus* pStatus);
ARCCAM_API double ArcCam_GetArrayTemperatureDN();
ARCCAM_API void ArcCam_SetArrayTemperature(double gTempVal,
        struct ArcCAPIStatus* pStatus);
ARCCAM_API void ArcCam_LoadTemperatureCtrlData(const char* pszFilename);
ARCCAM_API void ArcCam_SaveTemperatureCtrlData(const char* pszFilename);

// +============================================================================+
// | CDeinterlace functions                                                     |
// +============================================================================+
ARCCAM_API void ArcCam_Deinterlace(void *pData, int dRows, int dCols,
        int dAlgorithm, struct ArcCAPIStatus* pStatus);

ARCCAM_API void ArcCam_DeinterlaceWithArg(void *pData, int dRows, int dCols,
        int dAlgorithm, int dArg, struct ArcCAPIStatus* pStatus);

// +============================================================================+
// | CImage functions                                                           |
// +============================================================================+
ARCCAM_API void ArcCam_SubtractImageHalves(void* pMem, int dRows, int dCols);

ARCCAM_API void ArcCam_VerifyImageAsSynthetic(void* pMem, int dRows, int dCols,
        struct ArcCAPIStatus* pStatus);

// +============================================================================+
// | CFitsFile functions                                                        |
// +============================================================================+
ARCCAM_API void ArcCam_WriteToFitsFile(const char* pszFilename, void* pData,
        int dRows, int dCols, struct ArcCAPIStatus* pStatus);

#ifdef __cplusplus
}
#endif

#endif		// _ARCCAM_API_H_
