/*                                      
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$                                          
 *   $URL$                                          
 *                                                        
 *   Copyright (C) 2010-2012 Astronomical Institute, Academy Sciences of the Czech Republic, v.v.i.
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
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

#define _XOPEN_SOURCE
#include <time.h>

#include "include/slalib.h"
#include "bxmlrpc.h"
#include "telescope.h"

static xmlrpc_env tle_env;
static xmlrpc_client *p_tle_client = NULL;
static xmlrpc_server_info *p_tle_server_info = NULL;
static char tle_err_msg[TLE_ERR_MSG_MAX + 1];

static int tle_is_fault_occurred(xmlrpc_env *p_tle_env)
{
    if (p_tle_env->fault_occurred)
    {
        snprintf(tle_err_msg, TLE_ERR_MSG_MAX, "XML-RPC Fault: %s (%d)",
                p_tle_env->fault_string, p_tle_env->fault_code);
        return 1;
    }
    else
    {
        return 0;
    }
}

static int tle_struct_read(xmlrpc_value *p_result, const char *p_name,
        char *p_output)
{
    xmlrpc_value *p_value;
    const char *p_str;

    xmlrpc_struct_read_value(&tle_env, p_result, p_name, &p_value);

    // TODO
    tle_is_fault_occurred(&tle_env);

    xmlrpc_read_string(&tle_env, p_value, &p_str);

    // TODO
    tle_is_fault_occurred(&tle_env);

    strncpy(p_output, p_str, TLE_ANSWER_MAX);

    xmlrpc_DECREF(p_value);
    free((char*) p_str);

    return 0;
}

static int tle_trrd2radec(TLE_INFO_T *p_tle_info)
{
    char trrd[TLE_ANSWER_MAX+1];
    char dd[TLE_NN_MAX+1];
    char hh[TLE_NN_MAX+1];
    char mm[TLE_NN_MAX+1];
    char *p_dec;
    char *p_pos;
    int trrd_len;
    int sig;

    strncpy(trrd, p_tle_info->trrd, TLE_ANSWER_MAX);
    trrd_len = strlen(trrd);
    p_dec = strchr(trrd, ' ') + 1;
    p_pos = strrchr(trrd, ' ') + 1;

    if ((p_dec != NULL) && (p_pos != NULL) && (p_dec != p_pos)) {
        trrd[trrd_len-strlen(p_dec)-1] = '\0';
        trrd[trrd_len-strlen(p_pos)-1] = '\0';

        if (strlen(trrd) >= 8) {
            memset(hh, 0, TLE_NN_MAX);
            memset(mm, 0, TLE_NN_MAX);

            strncpy(hh, trrd, 2);
            strncpy(mm, trrd+2, 2);

            snprintf(p_tle_info->ra, TLE_ANSWER_MAX, "%s:%s:%s", hh, mm, trrd+4);
        }

        if (strlen(p_dec) >= 8) {
            memset(dd, 0, TLE_NN_MAX);
            memset(mm, 0, TLE_NN_MAX);

            sig = ((p_dec[0] == '-') || (p_dec[0] == '+')) ? 1 : 0;
            strncpy(dd, p_dec, sig+2);
            strncpy(mm, p_dec+sig+2, 2);

            snprintf(p_tle_info->dec, TLE_ANSWER_MAX, "%s:%s:%s", dd, mm, p_dec+sig+4);
        }
    }

    return 0;
}

/*
 *  Compute Local Apparent Sidereal Time
 *  http://tycho.usno.navy.mil/sidereal.html
 */
void tle_compute_st(char *p_st, char *p_ut)
{
    time_t ut_time;
    char sign;
    int ndp = 0;
    int status;
    int st[4];
    float sec;
    float rad;
    float gmst_f;
    double gmst;
    double jd;
    struct tm *p_gm_tm;
    struct tm tm;

    setenv("TZ", "UTC", 1);

    if (p_ut == NULL)
    {
        ut_time = time(NULL);
        p_gm_tm = gmtime(&ut_time);
    }
    else
    {
        /* '2011-10-20 09:23:00' */
        strptime(p_ut, "%Y-%m-%d %H:%M:%S", &tm);
        p_gm_tm = &tm;
    }

    /* The number of years since 1900. */
    p_gm_tm->tm_year += 1900;
    /* The number of months since January, in the range 0 to 11. */
    p_gm_tm->tm_mon += 1;

    // void slaCaldj ( int iy, int im, int id, double *djm, int *j );
    slaCaldj(p_gm_tm->tm_year, p_gm_tm->tm_mon, p_gm_tm->tm_mday, &jd,
            &status);

    // double slaGmst ( double ut1 );
    gmst = slaGmst(jd);
    sec = p_gm_tm->tm_sec;

    // void slaCtf2r ( int ihour, int imin, float sec, float *rad, int *j );
    sla_ctf2r_(&p_gm_tm->tm_hour, &p_gm_tm->tm_min, &sec, &rad, &status);

    gmst_f = gmst + (rad * 1.0027379093) + TLE_LONGITUDE_RAD;
    // void slaCr2tf ( int ndp, float angle, char *sign, int ihmsf[4] );
    sla_cr2tf_(&ndp, &gmst_f, &sign, &st[0], 1);

    //double v = gmst_f / 6.283185307179586476925287;
    //sla_dd2tf_(&ndp, &v, &sign, &st[0]);

    if (st[0] >= 24)
    {
        st[0] -= 24;
    }

    snprintf(p_st, TLE_ST_MAX, "%02i:%02i:%02i", st[0], st[1], st[2]);
}

int tle_telescope_info(TLE_INFO_T *p_tle_info)
{
    int i;
    xmlrpc_value *p_result;
    xmlrpc_value *p_param_array;
    char answer[TLE_ANSWER_MAX + 1];
    char *p_save;
    char *p_token;
    char *p_str;

    bzero(p_tle_info, sizeof(TLE_INFO_T));

    p_param_array = xmlrpc_array_new(&tle_env);
    xmlrpc_client_call2(&tle_env, p_tle_client, p_tle_server_info,
            "telescope_info", p_param_array, &p_result);
    if (tle_is_fault_occurred(&tle_env))
    {
        return -1;
    }

    tle_struct_read(p_result, "fopo", answer);
    p_tle_info->fopo = atof(answer);

    tle_struct_read(p_result, "glst", answer);
    p_str = answer;
    p_tle_info->focus_state = -1;
    for (i = 0; i < 5; ++i)
    {
        p_token = strtok_r(p_str, " ", &p_save);
        if (p_token == NULL)
        {
            break;
        }

        if (i == 4)
        {
            p_tle_info->focus_state = atoi(p_token);
        }

        p_str = NULL;
    }

    tle_struct_read(p_result, "dopo", p_tle_info->domeaz);
    tle_struct_read(p_result, "trcs", p_tle_info->trcs);
    tle_struct_read(p_result, "trgv", p_tle_info->trgv);
    tle_struct_read(p_result, "trhd", p_tle_info->trhd);
    tle_struct_read(p_result, "trus", p_tle_info->trus);
    tle_struct_read(p_result, "ut", p_tle_info->ut);

    tle_compute_st(p_tle_info->st, p_tle_info->ut);

    tle_struct_read(p_result, "trrd", p_tle_info->trrd);
    tle_trrd2radec(p_tle_info);

    tle_telescope_execute("GLME 2", p_tle_info->airhumex);
    tle_telescope_execute("GLME 1", p_tle_info->airpress);
    tle_telescope_execute("GLME 0", p_tle_info->outtemp);
    tle_telescope_execute("GLME 4", p_tle_info->dometemp);

    xmlrpc_DECREF(p_result);
    xmlrpc_DECREF(p_param_array);

    return 0;
}

const char *tle_get_err_msg(void)
{
    return tle_err_msg;
}

int tle_init(void)
{
    char server_url[TLE_SERVER_URL_MAX + 1];
    char *p_telescope_host = getenv("TELESCOPE_HOST");
    char *p_telescope_port = getenv("TELESCOPE_PORT");

    bzero(tle_err_msg, TLE_ERR_MSG_MAX + 1);

    if ((p_telescope_host == NULL) || (p_telescope_port == NULL))
    {
        snprintf(tle_err_msg, TLE_ERR_MSG_MAX,
                "Undefined variable: TELESCOPE_HOST=%s, TELESCOPE_PORT=%s",
                p_telescope_host, p_telescope_port);
        return -1;
    }

    xmlrpc_env_init(&tle_env);

    if (bxr_client_init() == -1)
    {
        return -1;
    }

    xmlrpc_client_create(&tle_env, 0, "peso", SVN_REV, NULL, 0, &p_tle_client);
    if (tle_is_fault_occurred(&tle_env))
    {
        fprintf(stderr, "\n%s\n", tle_err_msg);
        return -1;
    }

    snprintf(server_url, TLE_SERVER_URL_MAX, "http://%s:%s/RPC2",
            p_telescope_host, p_telescope_port);
    p_tle_server_info = xmlrpc_server_info_new(&tle_env, server_url);
    if (tle_is_fault_occurred(&tle_env))
    {
        fprintf(stderr, "\n%s\n", tle_err_msg);
        return -1;
    }

    return 0;
}

int tle_uninit(void)
{
    if (p_tle_server_info != NULL)
    {
        xmlrpc_server_info_free(p_tle_server_info);
        p_tle_server_info = NULL;
    }

    if (p_tle_client != NULL)
    {
        xmlrpc_client_destroy(p_tle_client);
        p_tle_client = NULL;
    }

    xmlrpc_env_clean(&tle_env);
    bxr_client_cleanup();

    return 1;
}

// TODO: uvolnovani pameti i pri predcasnem ukonceni
int tle_telescope_execute(char *p_command, char *p_answer)
{
    xmlrpc_value *p_result;
    xmlrpc_value *p_param_array;
    xmlrpc_value *p_item;
    const char *p_str;

    p_param_array = xmlrpc_array_new(&tle_env);
    p_item = xmlrpc_string_new(&tle_env, p_command);
    if (tle_is_fault_occurred(&tle_env))
    {
        return -1;
    }

    xmlrpc_array_append_item(&tle_env, p_param_array, p_item);
    if (tle_is_fault_occurred(&tle_env))
    {
        return -1;
    }

    xmlrpc_client_call2(&tle_env, p_tle_client, p_tle_server_info,
            "telescope_execute", p_param_array, &p_result);
    if (tle_is_fault_occurred(&tle_env))
    {
        return -1;
    }

    xmlrpc_read_string(&tle_env, p_result, &p_str);

    if (tle_is_fault_occurred(&tle_env))
    {
        return -1;
    }

    xmlrpc_DECREF(p_result);

    strncpy(p_answer, p_str, TLE_ANSWER_MAX);
    free((char*) p_str);

    xmlrpc_DECREF(p_item);
    xmlrpc_DECREF(p_param_array);

    return 0;
}

// TODO: odstranit
int tle_load_info(TLE_INFO_T *p_tle_info)
{
    return 0;
}

#ifdef SELF_TELESCOPE

int main(int argc, char *argv[])
{
    char ut[1024];
    char st[TLE_ST_MAX+1];

    strcpy(ut, "2011-10-20 09:23:00");

    tle_compute_st(st, ut);
}

#endif
