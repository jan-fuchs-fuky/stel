/*                                      
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$                                          
 *   $URL$                                          
 *                                                        
 *   Copyright (C) 2011-2013 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
 *                                                                                            
 *   This file is part of PESO.
 *                                                                                            
 *   PESO is free software: you can redistribute it and/or modify                          
 *   it under the terms of the GNU General Public License as published by                     
 *   the Free Software Foundation, either version 3 of the License, or                        
 *   (at your option) any later version.                                                      
 *                                                                                            
 *   Observe is distributed in the hope that it will be useful,                               
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of                           
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                            
 *   GNU General Public License for more details.                                             
 *                                                                                            
 *   You should have received a copy of the GNU General Public License                        
 *   along with Observe.  If not, see <http://www.gnu.org/licenses/>.                         
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#include "bxmlrpc.h"
#include "spectrograph.h"

static xmlrpc_env sgh_env;
static xmlrpc_client *p_sgh_client = NULL;
static xmlrpc_server_info *p_sgh_server_info = NULL;
static char sgh_err_msg[SGH_ERR_MSG_MAX + 1];

static int sgh_is_fault_occurred(xmlrpc_env *p_sgh_env)
{
    if (p_sgh_env->fault_occurred)
    {
        snprintf(sgh_err_msg, SGH_ERR_MSG_MAX, "XML-RPC Fault: %s (%d)",
                p_sgh_env->fault_string, p_sgh_env->fault_code);
        fprintf(stderr, "XML-RPC Fault: %s (%d)\n", p_sgh_env->fault_string,
                p_sgh_env->fault_code);
        return 1;
    }
    else
    {
        return 0;
    }
}

//static int sgh_struct_read(xmlrpc_value *p_result, const char *p_name, char *p_output)
//{
//    xmlrpc_value *p_value;
//    const char *p_str;
//
//    xmlrpc_struct_read_value(&sgh_env, p_result, p_name, &p_value);
//
//    if (sgh_is_fault_occurred(&sgh_env)) {
//        return -1;
//    }
//
//    xmlrpc_read_string(&sgh_env, p_value, &p_str);
//
//    if (sgh_is_fault_occurred(&sgh_env)) {
//        return -1;
//    }
//
//    strncpy(p_output, p_str, SGH_ANSWER_MAX);
//
//    xmlrpc_DECREF(p_value);
//    free((char*)p_str);
//
//    return 0;
//}

static char *sgh_get_item(xmlrpc_value *p_result, const char *p_name)
{
    xmlrpc_value *p_value;
    char *p_str;

    xmlrpc_struct_read_value(&sgh_env, p_result, p_name, &p_value);

    if (sgh_is_fault_occurred(&sgh_env))
    {
        return NULL;
    }

    xmlrpc_read_string(&sgh_env, p_value, (const char **) &p_str);

    if (sgh_is_fault_occurred(&sgh_env))
    {
        return NULL;
    }

    xmlrpc_DECREF(p_value);

    return p_str;
}

static void sgh_free_item(char *p_item)
{
    // TODO: osetrit p_item == NULL
    free((char*) p_item);
}

const char *sgh_get_err_msg(void)
{
    return sgh_err_msg;
}

// (-205.294 * <stupne.desetiny...> + 12667.1)  ....[pocet inkrementu]
int sgh_gratang2gratpos(double gratang)
{
    return (-205.294 * gratang + 12667.1);
}

// (-0.00487106 * <inkrementy> + 61.7024)  .... [uhel: stupne.desetiny...]
double sgh_gratpos2gratang(int gratpos)
{
    return (-0.00487106 * gratpos + 61.7024);
}

void sgh_gratpos2gratang_str(int gratpos, char *p_gratang, int gratang_len)
{
    double gratang;
    double degree;
    double minute;

    gratang = sgh_gratpos2gratang(gratpos);
    degree = floor(gratang);
    minute = round((gratang - degree) * 60);

    if (minute == 60) {
        ++degree;
        minute = 0;
    }

    snprintf(p_gratang, gratang_len, "%.0f:%.0f", degree, minute);
}

int sgh_init(void)
{
    char server_url[SGH_SERVER_URL_MAX + 1];
    char *p_spectrograph_host = getenv("SPECTROGRAPH_HOST");
    char *p_spectrograph_port = getenv("SPECTROGRAPH_PORT");

    bzero(sgh_err_msg, SGH_ERR_MSG_MAX + 1);

    if ((p_spectrograph_host == NULL) || (p_spectrograph_port == NULL))
    {
        snprintf(sgh_err_msg, SGH_ERR_MSG_MAX,
                "Undefined variable: SPECTROGRAPH_HOST=%s, SPECTROGRAPH_PORT=%s",
                p_spectrograph_host, p_spectrograph_port);
        return -1;
    }

    xmlrpc_env_init(&sgh_env);

    if (bxr_client_init() == -1)
    {
        return -1;
    }

    xmlrpc_client_create(&sgh_env, 0, "peso", SVN_REV, NULL, 0, &p_sgh_client);
    if (sgh_is_fault_occurred(&sgh_env))
    {
        fprintf(stderr, "\n%s\n", sgh_err_msg);
        return -1;
    }

    snprintf(server_url, SGH_SERVER_URL_MAX, "http://%s:%s/RPC2",
            p_spectrograph_host, p_spectrograph_port);
    p_sgh_server_info = xmlrpc_server_info_new(&sgh_env, server_url);
    if (sgh_is_fault_occurred(&sgh_env))
    {
        fprintf(stderr, "\n%s\n", sgh_err_msg);
        return -1;
    }

    return 0;
}

int sgh_uninit(void)
{
    if (p_sgh_server_info != NULL)
    {
        xmlrpc_server_info_free(p_sgh_server_info);
        p_sgh_server_info = NULL;
    }

    if (p_sgh_client != NULL)
    {
        xmlrpc_client_destroy(p_sgh_client);
        p_sgh_client = NULL;
    }

    xmlrpc_env_clean(&sgh_env);
    bxr_client_cleanup();

    return 1;
}

// TODO: uvolnovani pameti i pri predcasnem ukonceni
int sgh_spectrograph_execute(char *p_command, char *p_answer)
{
    xmlrpc_value *p_result;
    xmlrpc_value *p_param_array;
    xmlrpc_value *p_item;
    const char *p_str;

    p_param_array = xmlrpc_array_new(&sgh_env);
    p_item = xmlrpc_string_new(&sgh_env, p_command);
    if (sgh_is_fault_occurred(&sgh_env))
    {
        return -1;
    }

    xmlrpc_array_append_item(&sgh_env, p_param_array, p_item);
    if (sgh_is_fault_occurred(&sgh_env))
    {
        return -1;
    }

    xmlrpc_client_call2(&sgh_env, p_sgh_client, p_sgh_server_info,
            "spectrograph_execute", p_param_array, &p_result);
    if (sgh_is_fault_occurred(&sgh_env))
    {
        return -1;
    }

    xmlrpc_read_string(&sgh_env, p_result, &p_str);

    if (sgh_is_fault_occurred(&sgh_env))
    {
        return -1;
    }

    xmlrpc_DECREF(p_result);

    strncpy(p_answer, p_str, SGH_ANSWER_MAX);
    free((char*) p_str);

    xmlrpc_DECREF(p_item);
    xmlrpc_DECREF(p_param_array);

    return 0;
}

int sgh_spectrograph_info(SGH_INFO_T *p_sgh_info)
{
    int i;
    int star_calib;
    char *p_glst;
    char *p_item;
    char *p_save = NULL;
    xmlrpc_value *p_result;
    xmlrpc_value *p_param_array;

    memset(p_sgh_info, 0, sizeof(SGH_INFO_T));

    p_param_array = xmlrpc_array_new(&sgh_env);
    xmlrpc_client_call2(&sgh_env, p_sgh_client, p_sgh_server_info,
            "spectrograph_info", p_param_array, &p_result);
    if (sgh_is_fault_occurred(&sgh_env))
    {
        return -1;
    }

    p_sgh_info->flat = 0;
    p_sgh_info->comp = 0;

    if ((p_glst = sgh_get_item(p_result, "GLST")) == NULL)
    {
        return -1;
    }

    if ((p_item = strtok_r(p_glst, " ", &p_save)) != NULL)
    {
        p_sgh_info->dichroic_mirror = atoi(p_item);
    }

    i = 2;
    while ((p_item = strtok_r(NULL, " ", &p_save)) != NULL)
    {
        switch (i)
        {
        // spectral filter
        case 2:
            p_sgh_info->spectral_filter = atoi(p_item);
            break;

            // collimator
        case 3:
            p_sgh_info->collimator = atoi(p_item);
            break;

            // star/calibration
        case 6:
            star_calib = atoi(p_item);
            p_sgh_info->star_calib = star_calib;

            // 0 = unknown
            // 1 = star
            // 2 = calibration
            // 3 = moving
            // 4 = timeout
            if (star_calib == 2)
            {
                p_sgh_info->flat += 2;
                p_sgh_info->comp += 2;
            }
            break;

            // coude/oes
        case 7:
            p_sgh_info->coude_oes = atoi(p_item);
            break;

            // flat
        case 8:
            // 0 = off
            // 1 = on
            p_sgh_info->flat += atoi(p_item);
            break;

            // comp
        case 9:
            // 0 = off
            // 1 = on
            p_sgh_info->comp += atoi(p_item);
            break;

        // Correction plate 700
        case 16:
            p_sgh_info->correction_plate_700 = atoi(p_item);
            break;

        // Correction plate 400
        case 17:
            p_sgh_info->correction_plate_400 = atoi(p_item);
            break;

            // OES collimator
        case 21:
            p_sgh_info->oes_collimator = atoi(p_item);
            break;

        // OES Iodine cell
        case 26:
            p_sgh_info->oes_iodine_cell = atoi(p_item);
            break;

        default:
            break;
        }

        ++i;
    }

    sgh_free_item(p_glst);

    if ((p_item = sgh_get_item(p_result, "SPFE_14")) != NULL)
    {
        p_sgh_info->exp_freq = atoi(p_item);
        sgh_free_item(p_item);
    }

    if ((p_item = sgh_get_item(p_result, "SPCE_14")) != NULL)
    {
        p_sgh_info->exp_sum = atoi(p_item);
        sgh_free_item(p_item);
    }

    if ((p_item = sgh_get_item(p_result, "SPGP_13")) != NULL)
    {
        p_sgh_info->grating_position = atoi(p_item);
        sgh_free_item(p_item);
    }

    if ((p_item = sgh_get_item(p_result, "SPGP_5")) != NULL)
    {
        p_sgh_info->focus_1400 = atoi(p_item);
        sgh_free_item(p_item);
    }

    if ((p_item = sgh_get_item(p_result, "SPGP_4")) != NULL)
    {
        p_sgh_info->focus_700 = atoi(p_item);
        sgh_free_item(p_item);
    }

    if ((p_item = sgh_get_item(p_result, "SPGP_22")) != NULL)
    {
        p_sgh_info->focus_oes = atoi(p_item);
        sgh_free_item(p_item);
    }

    if ((p_item = sgh_get_item(p_result, "SPGS_19")) != NULL)
    {
        p_sgh_info->coude_temp = atoi(p_item);
        sgh_free_item(p_item);
    }

    if ((p_item = sgh_get_item(p_result, "SPGS_20")) != NULL)
    {
        p_sgh_info->oes_temp = atoi(p_item);
        sgh_free_item(p_item);
    }

    xmlrpc_DECREF(p_result);
    xmlrpc_DECREF(p_param_array);

    return 0;
}

float sgh_temp2human(int temp_raw) {
    float temp;

    temp = temp_raw / (27648/80.) - 30;

    return temp;
}
