/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 * $URL$
 */

#ifndef __MOD_CCD_BILBO_H
#define __MOD_CCD_BILBO_H

// Nastaveni konstant pro kameru
const int BiasLevelLow[] = {1144, 863, 0, 0, 0, 0, 0, 0};
const int BiasLevelHigh[] = {2725, 1735, 0, 0, 0, 0, 0, 0};

#define BIL_SB_MAX 256
#define BIL_BUFZ 2110
#define BIL_MEMZ (20*1048576)
#define BIL_HEADER 1024

// define PC board interface
#define BIL_RWD (bil_pc_board_base)        /* port for read/write to RAM       */
#define BIL_PCPTH (bil_pc_board_base + 2)  /* read/write addr. bit 16 -> 31    */
#define BIL_PCPTL (bil_pc_board_base + 4)  /* read/write addr. bit 0 -> 15     */
#define BIL_SERPT (bil_pc_board_base + 6)  /* serial input addr bit 16 -> 31   */
#define BIL_TXDATA (bil_pc_board_base + 8) /* send data                        */
#define BIL_TXCOM (bil_pc_board_base + 10) /* transmit command (read/write)        */
                                           /* on read bit0=1 transmitter not empty */
                                           /*         bit0=0 transmitter empty     */
#define BIL_INCSET (bil_pc_board_base + 12)/* autoincrement/no increment       */
                                           /* read/write on rwd port           */
                                           /* bit0=1  read increment enable    */
                                           /* bit0=0  read increment disable   */
                                           /* bit1=1  write increment enable   */
                                           /* bit1=0  write increment disable  */
#define BIL_TXE (bil_pc_board_base + 14)   /* transmitter/receiver control     */
                                           /* bit0=1  enable transmitter       */
                                           /* bit0=0  disable transmitter      */
                                           /* bit1=1  enable receiver          */
                                           /* bit1=0  disable receiver         */

// BIL_BROCAM_INFO_T ..._MAX
#define BIL__CAM_ID_MAX 4
#define BIL__CCD_TYPE_MAX 16
#define BIL_CAM_ID_MAX 5
#define BIL_CCD_TYPE_MAX 17
#define BIL_DUMMY0_MAX 24
#define BIL_DUMMY1_MAX 894

typedef struct {
    char _cam_id[BIL__CAM_ID_MAX];  /* 0 */
    char frame_type;
    char cam_mode;
    char _ccd_type[BIL__CCD_TYPE_MAX];
    unsigned short int totx;    /* 22 */
    unsigned short int toty;
    unsigned short int beginx;
    unsigned short int beginy;
    unsigned short int imx;
    unsigned short int imy;
    unsigned short int binx;
    unsigned short int biny;
    short int ccd_temp;         /* 38 */
    short int amb_temp;
    short int ref_temp;
    unsigned short int int_period;
    unsigned short int readout_time;
    unsigned short int frame_left;
    unsigned short int frame_total;
    unsigned short int cds_setup;    /* 52 */
    unsigned short int mpp_mode;
    unsigned short int auto_clear;
    unsigned short int auto_readout;
    unsigned short int auto_shutter;
    unsigned short int motor_number;
    unsigned short int shutter_delay;
    unsigned short int over_scan;
    int exptime;                        /* 68 */
    int expleft;                        /* 72 */
    int acc_amb_temp;                   /* 76 */
    int acc_ccd_temp;                   /* 80 */
    char filter;                        /* 84 */
    char readdir;                       /* 85 */
    unsigned short int filt_pos;        /* 86 */
    unsigned int totsh;                 /* 88 */
    unsigned int totsh_old;             /* 92 */
    unsigned int lastrd;                /* 96 */
    unsigned int lastrd_old;            /*100 */
    char dummy0[BIL_DUMMY0_MAX];        /* fill up to 128 bytes */
    unsigned short int pressure;        /* 128 */
    char dummy1[BIL_DUMMY1_MAX];        /* fill up to 1024 bytes */
    char cam_id[BIL_CAM_ID_MAX];        /* '$_cam_id\0' */
    char ccd_type[BIL_CCD_TYPE_MAX];    /* '$_ccd_type\0' */
} BIL_BROCAM_INFO_T;

// TODO: default nahradit skutecnou hodnotou vycitani
typedef enum
{
    BILBO_SPEED_DEFAULT_E, BILBO_SPEED_MAX_E,
} BILBO_SPEED_E;

struct
{
    const char *str;
    BILBO_SPEED_E speed_e;
} bilbo_speed_str2enum[] =
{
    { "default", BILBO_SPEED_DEFAULT_E },
};

#endif
