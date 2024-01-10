/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 */

#ifndef __CFG_H
#define __CFG_H

#include <pwd.h>

#define CFG_TYPE_STR_MAX         511
#define IP_MAX                   CFG_TYPE_STR_MAX 
#define CMD_MAX                  CFG_TYPE_STR_MAX 
#define INSTRUMENT_MAX           CFG_TYPE_STR_MAX 
#define MOD_CCD_MAX              CFG_TYPE_STR_MAX 
#define OUTPUT_PATHS_MAX         CFG_TYPE_STR_MAX 
#define ARCHIVE_PATHS_MAX        CFG_TYPE_STR_MAX 
#define INSTRUMENT_PREFIX_MAX    CFG_TYPE_STR_MAX 
#define USER_MAX                 CFG_TYPE_STR_MAX 
#define PASSWORD_MAX             CFG_TYPE_STR_MAX 
#define FILE_PID_MAX             CFG_TYPE_STR_MAX 
#define ARCHIVE_SCRIPT_MAX       CFG_TYPE_STR_MAX 
#define TIM_FILE_MAX             CFG_TYPE_STR_MAX
#define UTIL_FILE_MAX            CFG_TYPE_STR_MAX

typedef enum
{
    CFG_EVENT_USER_E,
    CFG_EVENT_PASSWORD_E,
    CFG_EVENT_INSTRUMENT_E,
    CFG_EVENT_INSTRUMENT_PREFIX_E,
    CFG_EVENT_IP_E,
    CFG_EVENT_PORT_E,
    CFG_EVENT_FILE_PID_E,
    CFG_EVENT_MOD_CCD_PATH_E,
    CFG_EVENT_OUTPUT_PATHS_E,
    CFG_EVENT_ARCHIVE_PATHS_E,
    CFG_EVENT_ARCHIVE_E,
    CFG_EVENT_ARCHIVE_SCRIPT_E,
    CFG_EVENT_CMD_BEGIN_FLAT_E,
    CFG_EVENT_CMD_BEGIN_COMP_E,
    CFG_EVENT_CMD_BEGIN_OBJECT_E,
    CFG_EVENT_CMD_END_FLAT_E,
    CFG_EVENT_CMD_END_COMP_E,
    CFG_EVENT_CMD_END_OBJECT_E,
    CFG_EVENT_CCD_TEMP_E,
    CFG_EVENT_CCD_READOUT_TIME_E,
    CFG_EVENT_CCD_X1_E,
    CFG_EVENT_CCD_X2_E,
    CFG_EVENT_CCD_XB_E,
    CFG_EVENT_CCD_Y1_E,
    CFG_EVENT_CCD_Y2_E,
    CFG_EVENT_CCD_YB_E,
    CFG_EVENT_CCD_BITS_PER_PIXEL_E,
    CFG_EVENT_CCD_BILBO_PC_BOARD_BASE_E,
    CFG_EVENT_CCD_FRODO_TIM_FILE_E,
    CFG_EVENT_CCD_FRODO_UTIL_FILE_E,
    CFG_EVENT_CCD_FRODO_NUM_PCI_TESTS_E,
    CFG_EVENT_CCD_FRODO_NUM_TIM_TESTS_E,
    CFG_EVENT_CCD_FRODO_NUM_UTIL_TESTS_E,
    CFG_EVENT_MAX_E,
} CFG_EVENT_T;

typedef struct
{
    char flat[CMD_MAX + 1];
    char comp[CMD_MAX + 1];
    char zero[CMD_MAX + 1];
    char dark[CMD_MAX + 1];
    char target[CMD_MAX + 1];
} CMD_T;

typedef enum
{
    CFG_TYPE_STR_E, CFG_TYPE_INT_E, CFG_TYPE_DOUBLE_E, CFG_TYPE_BOOLEAN_E,
} CFG_TYPE_T;

typedef struct
{
    double temp;
    int readout_time;
    int x1;
    int x2;
    int xb;
    int y1;
    int y2;
    int yb;
    int bits_per_pixel;
} CCD_T;

typedef struct
{
    int pc_board_base;
} CCD_BILBO_T;

typedef struct
{
    char tim_file[TIM_FILE_MAX + 1];
    char util_file[UTIL_FILE_MAX + 1];
    int num_pci_tests;
    int num_tim_tests;
    int num_util_tests;
} CCD_FRODO_T;

typedef struct
{
} CCD_SAURON_T;

typedef struct exposed_allow_ip
{
    struct exposed_allow_ip *p_next;
    char name[CFG_TYPE_STR_MAX + 1];
    char ip[CFG_TYPE_STR_MAX + 1];
} EXPOSED_IP_T;

typedef struct exposed_header
{
    struct exposed_header *p_next;
    char key[CFG_TYPE_STR_MAX + 1];
    char value[CFG_TYPE_STR_MAX + 1];
} EXPOSED_HEADER_T;

typedef struct
{
    char user[USER_MAX + 1];
    char password[PASSWORD_MAX + 1];
    char instrument[INSTRUMENT_MAX + 1];
    char ip[IP_MAX + 1];
    char mod_ccd[MOD_CCD_MAX + 1];
    char output_paths[OUTPUT_PATHS_MAX + 1];
    char archive_paths[ARCHIVE_PATHS_MAX + 1];
    char archive_script[ARCHIVE_SCRIPT_MAX + 1];
    char instrument_prefix[INSTRUMENT_PREFIX_MAX + 1];
    char file_pid[FILE_PID_MAX + 1];
    int port;
    int archive;
    CMD_T cmd_begin;
    CMD_T cmd_end;
    CCD_T ccd;
    CCD_BILBO_T ccd_bilbo;
    CCD_FRODO_T ccd_frodo;
    CCD_SAURON_T ccd_sauron;
    EXPOSED_IP_T *p_allow_ip;
    EXPOSED_HEADER_T *p_header;
    uid_t uid; /* automatic load */
    gid_t gid; /* automatic load */
} EXPOSED_CFG_T;

typedef struct
{
    const char *p_group_name;
    const char *p_key;
    CFG_TYPE_T type;
    void *p_save;
} CFG_T;

extern EXPOSED_CFG_T exposed_cfg;

int cfg_load(char *p_cfg_file, EXPOSED_CFG_T *p_exposed_cfg, char *p_ccd_name);

#endif
