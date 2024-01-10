/**
  * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
  * $Date$
  * $Rev$
  * $URL$
 */

#ifndef __SPECTROGRAPH_H
#define __SPECTROGRAPH_H

#include <glib.h>
#include <log4c.h>

#define MAKE_DATE_TIME "(" __DATE__ " " __TIME__ ")"

#define COMMAND_MAX 1023
#define INFO_MAX    123

#define CFG_TYPE_STR_MAX 511

extern log4c_category_t *p_logcat;

/*
 *  GLST - GLobal STate:
 *
 *    1. dichroicka zrcatka - SPGS 1
 *    2. spektralni filtr - SPGS 2
 *    3. maska kolimatoru - SPGS 3
 *    4. ostreni 700
 *    5. ostreni 1400/400
 *    6. preklapeni hvezda/kalibrace - SPGS 6
 *    7. preklapeni Coude/Oes - SPGS 7
 *    8. flat field - SPGS 8
 *    9. srovnavaci spektrum - SPGS 9
 *    10. zaverka expozimetru - SPGS 10
 *    11. zaverka kamery 700 - SPGS 11
 *    12. zaverka kamery 1400/400 - SPGS 12
 *    13. mrizka
 *    14. expozimeter
 *    15. sterbinova kamera - SPGS 15
 *    16. korekcni desky ostreni 700 - SPGS 16
 *    17. korekcni desky ostreni 1400/400 - SPGS 17
 *    18. rezerva
 *    19. rezerva
 *    20. rezerva
 *    21. maska kolimatoru Oes - SPGS 21
 *    22. ostreni Oes
 *    23. zaverka expozimetru Oes - SPGS 23
 *    24. expozimeter Oes
 *    25. rezerva
 *    26. jodova banka - SPGS 26
 *
 *  GLGI - GLobal Get Input
 */
typedef enum {
    INFO_GLST_E, // GLobal STate
    INFO_SPGP_4_E, // absolutni pozice ostreni 700
    INFO_SPGP_5_E, // absolutni pozice ostreni 1400/400
    INFO_SPGP_13_E, // absolutni pozice mrizky
    INFO_SPCE_14_E, // pocet pulzu nacitanych expozimetrem
    INFO_SPFE_14_E, // frekvence pulsu nacitanych expozimetrem
    INFO_SPCE_24_E, // pocet pulzu nacitanych expozimetrem Oes
    INFO_SPFE_24_E, // frekvence pulzu nacitanych expozimetrem Oes
    INFO_SPGP_22_E, // absolutni pozice ostreni Oes
    INFO_SPGS_19_E, // teplota v Coude
    INFO_SPGS_20_E, // teplota v OESu
    INFO_SIZE_E
} SPECTROGRAPH_INFO_E;

typedef struct {
    GKeyFile *p_key_file;
    char cfg_file[CFG_TYPE_STR_MAX+1];
    char file_pid[CFG_TYPE_STR_MAX+1];
    char xmlrpc_log[CFG_TYPE_STR_MAX+1];
    char ascol_ip[CFG_TYPE_STR_MAX+1];
    int port;
    int execute_spch_10;
    int protection;
    int oes_exposimeter_close;
    int coude_exposimeter_close;
    int ascol_loop_port;
    int ascol_cmd_port;
} SPECTROGRAPH_CFG_T;

typedef struct spectrograph_ip {
    struct spectrograph_ip *p_next;
    char name[CFG_TYPE_STR_MAX+1];
    char ip[CFG_TYPE_STR_MAX+1];
} SPECTROGRAPH_IP_T;

#endif
