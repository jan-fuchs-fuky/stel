/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 */

#include <stdlib.h>                     
#include <string.h>                     
#include <assert.h>                     
#include <log4c.h>
#include <glib.h>
#include <sys/types.h>
#include <pwd.h>

#include "exposed.h"
#include "cfg.h"

EXPOSED_CFG_T exposed_cfg;

static CFG_T cfg[CFG_EVENT_MAX_E];

static void cfg_init(EXPOSED_CFG_T *p_exposed_cfg)
{
    cfg[CFG_EVENT_USER_E].p_group_name = "exposed";
    cfg[CFG_EVENT_USER_E].p_key = "user";
    cfg[CFG_EVENT_USER_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_USER_E].p_save = p_exposed_cfg->user;

    cfg[CFG_EVENT_PASSWORD_E].p_group_name = "exposed";
    cfg[CFG_EVENT_PASSWORD_E].p_key = "password";
    cfg[CFG_EVENT_PASSWORD_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_PASSWORD_E].p_save = p_exposed_cfg->password;

    cfg[CFG_EVENT_INSTRUMENT_E].p_group_name = "exposed";
    cfg[CFG_EVENT_INSTRUMENT_E].p_key = "instrument";
    cfg[CFG_EVENT_INSTRUMENT_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_INSTRUMENT_E].p_save = p_exposed_cfg->instrument;

    cfg[CFG_EVENT_INSTRUMENT_PREFIX_E].p_group_name = "exposed";
    cfg[CFG_EVENT_INSTRUMENT_PREFIX_E].p_key = "instrument_prefix";
    cfg[CFG_EVENT_INSTRUMENT_PREFIX_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_INSTRUMENT_PREFIX_E].p_save =
            &p_exposed_cfg->instrument_prefix;

    cfg[CFG_EVENT_IP_E].p_group_name = "exposed";
    cfg[CFG_EVENT_IP_E].p_key = "ip";
    cfg[CFG_EVENT_IP_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_IP_E].p_save = p_exposed_cfg->ip;

    cfg[CFG_EVENT_PORT_E].p_group_name = "exposed";
    cfg[CFG_EVENT_PORT_E].p_key = "port";
    cfg[CFG_EVENT_PORT_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_PORT_E].p_save = &p_exposed_cfg->port;

    cfg[CFG_EVENT_FILE_PID_E].p_group_name = "exposed";
    cfg[CFG_EVENT_FILE_PID_E].p_key = "file_pid";
    cfg[CFG_EVENT_FILE_PID_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_FILE_PID_E].p_save = &p_exposed_cfg->file_pid;

    cfg[CFG_EVENT_MOD_CCD_PATH_E].p_group_name = "modules";
    cfg[CFG_EVENT_MOD_CCD_PATH_E].p_key = "ccd";
    cfg[CFG_EVENT_MOD_CCD_PATH_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_MOD_CCD_PATH_E].p_save = p_exposed_cfg->mod_ccd;

    cfg[CFG_EVENT_OUTPUT_PATHS_E].p_group_name = "paths";
    cfg[CFG_EVENT_OUTPUT_PATHS_E].p_key = "output";
    cfg[CFG_EVENT_OUTPUT_PATHS_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_OUTPUT_PATHS_E].p_save = p_exposed_cfg->output_paths;

    cfg[CFG_EVENT_ARCHIVE_PATHS_E].p_group_name = "paths";
    cfg[CFG_EVENT_ARCHIVE_PATHS_E].p_key = "archive";
    cfg[CFG_EVENT_ARCHIVE_PATHS_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_ARCHIVE_PATHS_E].p_save = p_exposed_cfg->archive_paths;

    cfg[CFG_EVENT_ARCHIVE_E].p_group_name = "exposed";
    cfg[CFG_EVENT_ARCHIVE_E].p_key = "archive";
    cfg[CFG_EVENT_ARCHIVE_E].type = CFG_TYPE_BOOLEAN_E;
    cfg[CFG_EVENT_ARCHIVE_E].p_save = &p_exposed_cfg->archive;

    cfg[CFG_EVENT_ARCHIVE_SCRIPT_E].p_group_name = "exposed";
    cfg[CFG_EVENT_ARCHIVE_SCRIPT_E].p_key = "archive_script";
    cfg[CFG_EVENT_ARCHIVE_SCRIPT_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_ARCHIVE_SCRIPT_E].p_save = p_exposed_cfg->archive_script;

    cfg[CFG_EVENT_CMD_BEGIN_FLAT_E].p_group_name = "commands_begin";
    cfg[CFG_EVENT_CMD_BEGIN_FLAT_E].p_key = "flat";
    cfg[CFG_EVENT_CMD_BEGIN_FLAT_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_CMD_BEGIN_FLAT_E].p_save = p_exposed_cfg->cmd_begin.flat;

    cfg[CFG_EVENT_CMD_BEGIN_COMP_E].p_group_name = "commands_begin";
    cfg[CFG_EVENT_CMD_BEGIN_COMP_E].p_key = "comp";
    cfg[CFG_EVENT_CMD_BEGIN_COMP_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_CMD_BEGIN_COMP_E].p_save = p_exposed_cfg->cmd_begin.comp;

    cfg[CFG_EVENT_CMD_BEGIN_OBJECT_E].p_group_name = "commands_begin";
    cfg[CFG_EVENT_CMD_BEGIN_OBJECT_E].p_key = "target";
    cfg[CFG_EVENT_CMD_BEGIN_OBJECT_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_CMD_BEGIN_OBJECT_E].p_save = p_exposed_cfg->cmd_begin.target;

    cfg[CFG_EVENT_CMD_END_FLAT_E].p_group_name = "commands_end";
    cfg[CFG_EVENT_CMD_END_FLAT_E].p_key = "flat";
    cfg[CFG_EVENT_CMD_END_FLAT_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_CMD_END_FLAT_E].p_save = p_exposed_cfg->cmd_end.flat;

    cfg[CFG_EVENT_CMD_END_COMP_E].p_group_name = "commands_end";
    cfg[CFG_EVENT_CMD_END_COMP_E].p_key = "comp";
    cfg[CFG_EVENT_CMD_END_COMP_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_CMD_END_COMP_E].p_save = p_exposed_cfg->cmd_end.comp;

    cfg[CFG_EVENT_CMD_END_OBJECT_E].p_group_name = "commands_end";
    cfg[CFG_EVENT_CMD_END_OBJECT_E].p_key = "target";
    cfg[CFG_EVENT_CMD_END_OBJECT_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_CMD_END_OBJECT_E].p_save = p_exposed_cfg->cmd_end.target;

    cfg[CFG_EVENT_CCD_FRODO_TIM_FILE_E].p_group_name = "ccd_frodo";
    cfg[CFG_EVENT_CCD_FRODO_TIM_FILE_E].p_key = "tim_file";
    cfg[CFG_EVENT_CCD_FRODO_TIM_FILE_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_CCD_FRODO_TIM_FILE_E].p_save = p_exposed_cfg->ccd_frodo.tim_file;

    cfg[CFG_EVENT_CCD_FRODO_UTIL_FILE_E].p_group_name = "ccd_frodo";
    cfg[CFG_EVENT_CCD_FRODO_UTIL_FILE_E].p_key = "util_file";
    cfg[CFG_EVENT_CCD_FRODO_UTIL_FILE_E].type = CFG_TYPE_STR_E;
    cfg[CFG_EVENT_CCD_FRODO_UTIL_FILE_E].p_save = p_exposed_cfg->ccd_frodo.util_file;

    cfg[CFG_EVENT_CCD_BITS_PER_PIXEL_E].p_group_name = "ccd";
    cfg[CFG_EVENT_CCD_BITS_PER_PIXEL_E].p_key = "bits_per_pixel";
    cfg[CFG_EVENT_CCD_BITS_PER_PIXEL_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_CCD_BITS_PER_PIXEL_E].p_save =
            &p_exposed_cfg->ccd.bits_per_pixel;

    cfg[CFG_EVENT_CCD_FRODO_NUM_PCI_TESTS_E].p_group_name = "ccd_frodo";
    cfg[CFG_EVENT_CCD_FRODO_NUM_PCI_TESTS_E].p_key = "num_pci_tests";
    cfg[CFG_EVENT_CCD_FRODO_NUM_PCI_TESTS_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_CCD_FRODO_NUM_PCI_TESTS_E].p_save =
            &p_exposed_cfg->ccd_frodo.num_pci_tests;

    cfg[CFG_EVENT_CCD_FRODO_NUM_TIM_TESTS_E].p_group_name = "ccd_frodo";
    cfg[CFG_EVENT_CCD_FRODO_NUM_TIM_TESTS_E].p_key = "num_tim_tests";
    cfg[CFG_EVENT_CCD_FRODO_NUM_TIM_TESTS_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_CCD_FRODO_NUM_TIM_TESTS_E].p_save =
            &p_exposed_cfg->ccd_frodo.num_tim_tests;

    cfg[CFG_EVENT_CCD_FRODO_NUM_UTIL_TESTS_E].p_group_name = "ccd_frodo";
    cfg[CFG_EVENT_CCD_FRODO_NUM_UTIL_TESTS_E].p_key = "num_util_tests";
    cfg[CFG_EVENT_CCD_FRODO_NUM_UTIL_TESTS_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_CCD_FRODO_NUM_UTIL_TESTS_E].p_save =
            &p_exposed_cfg->ccd_frodo.num_util_tests;

    cfg[CFG_EVENT_CCD_TEMP_E].p_group_name = "ccd";
    cfg[CFG_EVENT_CCD_TEMP_E].p_key = "temp";
    cfg[CFG_EVENT_CCD_TEMP_E].type = CFG_TYPE_DOUBLE_E;
    cfg[CFG_EVENT_CCD_TEMP_E].p_save = &p_exposed_cfg->ccd.temp;

    cfg[CFG_EVENT_CCD_READOUT_TIME_E].p_group_name = "ccd";
    cfg[CFG_EVENT_CCD_READOUT_TIME_E].p_key = "readout_time";
    cfg[CFG_EVENT_CCD_READOUT_TIME_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_CCD_READOUT_TIME_E].p_save = &p_exposed_cfg->ccd.readout_time;

    cfg[CFG_EVENT_CCD_X1_E].p_group_name = "ccd";
    cfg[CFG_EVENT_CCD_X1_E].p_key = "x1";
    cfg[CFG_EVENT_CCD_X1_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_CCD_X1_E].p_save = &p_exposed_cfg->ccd.x1;

    cfg[CFG_EVENT_CCD_X2_E].p_group_name = "ccd";
    cfg[CFG_EVENT_CCD_X2_E].p_key = "x2";
    cfg[CFG_EVENT_CCD_X2_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_CCD_X2_E].p_save = &p_exposed_cfg->ccd.x2;

    cfg[CFG_EVENT_CCD_XB_E].p_group_name = "ccd";
    cfg[CFG_EVENT_CCD_XB_E].p_key = "xb";
    cfg[CFG_EVENT_CCD_XB_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_CCD_XB_E].p_save = &p_exposed_cfg->ccd.xb;

    cfg[CFG_EVENT_CCD_Y1_E].p_group_name = "ccd";
    cfg[CFG_EVENT_CCD_Y1_E].p_key = "y1";
    cfg[CFG_EVENT_CCD_Y1_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_CCD_Y1_E].p_save = &p_exposed_cfg->ccd.y1;

    cfg[CFG_EVENT_CCD_Y2_E].p_group_name = "ccd";
    cfg[CFG_EVENT_CCD_Y2_E].p_key = "y2";
    cfg[CFG_EVENT_CCD_Y2_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_CCD_Y2_E].p_save = &p_exposed_cfg->ccd.y2;

    cfg[CFG_EVENT_CCD_YB_E].p_group_name = "ccd";
    cfg[CFG_EVENT_CCD_YB_E].p_key = "yb";
    cfg[CFG_EVENT_CCD_YB_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_CCD_YB_E].p_save = &p_exposed_cfg->ccd.yb;

    cfg[CFG_EVENT_CCD_BILBO_PC_BOARD_BASE_E].p_group_name = "ccd_bilbo";
    cfg[CFG_EVENT_CCD_BILBO_PC_BOARD_BASE_E].p_key = "pc_board_base";
    cfg[CFG_EVENT_CCD_BILBO_PC_BOARD_BASE_E].type = CFG_TYPE_INT_E;
    cfg[CFG_EVENT_CCD_BILBO_PC_BOARD_BASE_E].p_save = &p_exposed_cfg->ccd_bilbo.pc_board_base;
}

static int cfg_get_allow_ips(GKeyFile *p_key_file)
{
    int i;
    char *p_char;
    gchar **p_keys;
    gsize length;
    GError *p_error = NULL;
    EXPOSED_IP_T *p_ip;
    EXPOSED_IP_T *p_ip_new;

    p_keys = g_key_file_get_keys(p_key_file, "allow_ips", &length, &p_error);

    if (p_error != NULL)
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "Get allow_ips failed: %s", p_error->message);
        return -1;
    }

    for (i = 0; i < length; ++i)
    {
        // TODO: append free
        if ((p_ip_new = malloc(sizeof(EXPOSED_IP_T))) == NULL)
        {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                    "malloc() failed");
            return -1;
        }

        if (exposed_cfg.p_allow_ip == NULL)
        {
            exposed_cfg.p_allow_ip = p_ip_new;
        }
        else
        {
            p_ip->p_next = p_ip_new;
        }

        p_ip = p_ip_new;
        p_ip->p_next = NULL;
        strncpy(p_ip->name, p_keys[i], CFG_TYPE_STR_MAX);
        p_char = g_key_file_get_string(p_key_file, "allow_ips", p_keys[i],
                &p_error);

        if (p_error != NULL)
        {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                    "Get allow_ips.%s failed: %s", p_keys[i], p_error->message);
            return -1;
        }

        if (p_char != NULL)
        {
            strncpy(p_ip->ip, p_char, CFG_TYPE_STR_MAX);
            log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
                    "allow_ips.%s = %s", p_keys[i], p_char);
        }
        else
        {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                    "allow_ips.%s = NULL", p_keys[i]);
            return -1;
        }
    }

    //p_ip = exposed_cfg.p_allow_ip;
    //while (p_ip != NULL) {
    //    p_ip = p_ip->p_next;
    //}

    return 0;
}

static int cfg_get_header(GKeyFile *p_key_file)
{
    int i;
    char *p_char;
    gchar **p_keys;
    gsize length;
    GError *p_error = NULL;
    EXPOSED_HEADER_T *p_hdr;
    EXPOSED_HEADER_T *p_hdr_new;

    p_keys = g_key_file_get_keys(p_key_file, "header", &length, &p_error);

    if (p_error != NULL)
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "Get header failed: %s", p_error->message);
        return -1;
    }

    for (i = 0; i < length; ++i)
    {
        // TODO: append free
        if ((p_hdr_new = malloc(sizeof(EXPOSED_HEADER_T))) == NULL)
        {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                    "malloc() failed");
            return -1;
        }

        if (exposed_cfg.p_header == NULL)
        {
            exposed_cfg.p_header = p_hdr_new;
        }
        else
        {
            p_hdr->p_next = p_hdr_new;
        }

        p_hdr = p_hdr_new;
        p_hdr->p_next = NULL;
        strncpy(p_hdr->key, p_keys[i], CFG_TYPE_STR_MAX);
        p_char = g_key_file_get_string(p_key_file, "header", p_keys[i],
                &p_error);

        if (p_error != NULL)
        {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                    "Get header.%s failed: %s", p_keys[i], p_error->message);
            return -1;
        }

        if (p_char != NULL)
        {
            strncpy(p_hdr->value, p_char, CFG_TYPE_STR_MAX);
            log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "header.%s = %s",
                    p_keys[i], p_char);
        }
        else
        {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                    "header.%s = NULL", p_keys[i]);
            return -1;
        }
    }

    return 0;
}

int cfg_load(char *p_cfg_file, EXPOSED_CFG_T *p_exposed_cfg, char *p_ccd_name)
{
    int i;
    int *p_int;
    int result = 0;
    double *p_double;
    char *p_char;
    GKeyFile *p_key_file;
    GKeyFileFlags flags;
    GError *p_error = NULL;
    struct passwd *p_pw;

    memset(p_exposed_cfg, 0, sizeof(EXPOSED_CFG_T));
    cfg_init(p_exposed_cfg);

    p_key_file = g_key_file_new();
    flags = G_KEY_FILE_NONE;

    if (!g_key_file_load_from_file(p_key_file, p_cfg_file, flags, &p_error))
    {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                "Error: Unable to load file %s", p_cfg_file);
        return -1;
    }

    for (i = 0; i < CFG_EVENT_MAX_E; ++i)
    {
        p_error = NULL;

        if ((!strncmp(cfg[i].p_group_name, "ccd_", 4))
                && (strcmp(cfg[i].p_group_name+4, p_ccd_name)))
        {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "%s.%s skipping",
                    cfg[i].p_group_name, cfg[i].p_key);
            continue;
        }

        if (cfg[i].type == CFG_TYPE_STR_E)
        {
            p_char = g_key_file_get_string(p_key_file, cfg[i].p_group_name,
                    cfg[i].p_key, &p_error);

            if (p_error == NULL)
            {
                if (i == CFG_EVENT_PASSWORD_E)
                {
                    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
                            "%s.%s = ******", cfg[i].p_group_name,
                            cfg[i].p_key);
                }
                else
                {
                    log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO,
                            "%s.%s = %s", cfg[i].p_group_name, cfg[i].p_key,
                            p_char);
                }

                if (p_char != NULL)
                {
                    strncpy(cfg[i].p_save, (char *) p_char, CFG_TYPE_STR_MAX);
                }
                else
                {
                    log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                            "Error: %s.%s = NULL", cfg[i].p_group_name,
                            cfg[i].p_key);
                    return -1;
                }
            }
        }
        else if (cfg[i].type == CFG_TYPE_INT_E)
        {
            p_int = cfg[i].p_save;
            *p_int = g_key_file_get_integer(p_key_file, cfg[i].p_group_name,
                    cfg[i].p_key, &p_error);

            if (p_error == NULL)
            {
                log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "%s.%s = %i",
                        cfg[i].p_group_name, cfg[i].p_key, *p_int);
            }
        }
        else if (cfg[i].type == CFG_TYPE_DOUBLE_E)
        {
            p_double = cfg[i].p_save;
            *p_double = g_key_file_get_double(p_key_file, cfg[i].p_group_name,
                    cfg[i].p_key, &p_error);

            if (p_error == NULL)
            {
                log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "%s.%s = %f",
                        cfg[i].p_group_name, cfg[i].p_key, *p_double);
            }
        }
        else if (cfg[i].type == CFG_TYPE_BOOLEAN_E)
        {
            p_int = cfg[i].p_save;
            *p_int =
                    g_key_file_get_boolean(p_key_file, cfg[i].p_group_name,
                            cfg[i].p_key, &p_error) ? 1 : 0;

            if (p_error == NULL)
            {
                log4c_category_log(p_logcat, LOG4C_PRIORITY_INFO, "%s.%s = %i",
                        cfg[i].p_group_name, cfg[i].p_key, *p_int);
            }
        }

        if (p_error != NULL)
        {
            log4c_category_log(p_logcat, LOG4C_PRIORITY_ERROR,
                    "Error: Get %s.%s failed: %s", cfg[i].p_group_name,
                    cfg[i].p_key, p_error->message);
            return -1;
        }
    }

    result = cfg_get_allow_ips(p_key_file);
    cfg_get_header(p_key_file);
    g_key_file_free(p_key_file);

    if ((p_pw = getpwnam(exposed_cfg.user)) == NULL)
    {
        exposed_cfg.uid = 0;
        exposed_cfg.gid = 0;
    }
    else
    {
        exposed_cfg.uid = p_pw->pw_uid;
        exposed_cfg.gid = p_pw->pw_gid;
    }

    return result;
}
