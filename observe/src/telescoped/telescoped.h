/**
  * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
  * $Date$
  * $Rev$
  * $URL$
 */

#ifndef __TELESCOPE_H
#define __TELESCOPE_H

#include <glib.h>
#include <log4c.h>

#define MAKE_DATE_TIME "(" __DATE__ " " __TIME__ ")"

#define COMMAND_MAX 1023
#define INFO_MAX    123

#define CFG_TYPE_STR_MAX 511

extern log4c_category_t *p_logcat;

typedef enum {
    INFO_GLST_E, 
    INFO_TRRD_E,
    INFO_TRHD_E,
    INFO_TRGV_E,
    INFO_TRUS_E,
    INFO_DOPO_E,
    INFO_TRCS_E,
    INFO_FOPO_E,
    INFO_GLUT_E, // vraci UT ve formatu HHMMSS.SSSYYYYmmdd
    INFO_SIZE_E,
} TELESCOPE_INFO_E;

typedef struct {
    const char *p_ra;
    const char *p_dec;
    const char *p_object;
    int position;
} TELESCOPE_COORDINATES_T;

typedef struct {
    GKeyFile *p_key_file;
    char cfg_file[CFG_TYPE_STR_MAX+1];
    char file_pid[CFG_TYPE_STR_MAX+1];
    char xmlrpc_log[CFG_TYPE_STR_MAX+1];
    char ascol_ip[CFG_TYPE_STR_MAX+1];
    int port;
    int ascol_loop_port;
    int ascol_cmd_port;
} TELESCOPE_CFG_T;

typedef struct telescope_ip {
    struct telescope_ip *p_next;
    char name[CFG_TYPE_STR_MAX+1];
    char ip[CFG_TYPE_STR_MAX+1];
} TELESCOPE_IP_T;

#endif
