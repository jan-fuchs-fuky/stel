/*                                       
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$                                          
 *   $URL$                                          
 */

#ifndef __TELESCOPE_H
#define __TELESCOPE_H

// longitude in radians
#define TLE_LONGITUDE_RAD (14.78361111111111 / 57.295779513082323)

#define TLE_SERVER_URL_MAX 1023
#define TLE_ERR_MSG_MAX    1023
#define TLE_ANSWER_MAX     1023
#define TLE_COMMAND_MAX    1023
#define TLE_NN_MAX         7
#define TLE_ST_MAX         127

/*
 *  airhumex - GLME 2
 *  airpress - GLME 1
 *  outtemp  - GLME 0
 *  dometemp - GLME 4
 *  telfocus - FOPO
 *  domeaz   - DOPO
 *  ra       - TRRD
 *  dec      - TRRD
 *  st       - compute
 */
typedef struct
{
    float fopo;
    int focus_state;
    char airhumex[TLE_ANSWER_MAX+1];
    char airpress[TLE_ANSWER_MAX+1];
    char outtemp[TLE_ANSWER_MAX+1];
    char dometemp[TLE_ANSWER_MAX+1];
    char domeaz[TLE_ANSWER_MAX+1];
    char ra[TLE_ANSWER_MAX+1];
    char dec[TLE_ANSWER_MAX+1];
    char st[TLE_ANSWER_MAX+1];
    char glst[TLE_ANSWER_MAX+1];
    char trcs[TLE_ANSWER_MAX+1];
    char trgv[TLE_ANSWER_MAX+1];
    char trhd[TLE_ANSWER_MAX+1];
    char trrd[TLE_ANSWER_MAX+1];
    char trus[TLE_ANSWER_MAX+1];
    char ut[TLE_ANSWER_MAX+1];
} TLE_INFO_T;

const char *tle_get_err_msg(void);
int tle_init(void);
int tle_uninit(void);
int tle_telescope_execute(char *p_command, char *p_answer);
int tle_telescope_info(TLE_INFO_T *p_tle_info);
void tle_compute_st(char *p_st, char *p_ut);

double sla_gmst_(double *ut1);
void sla_cr2tf_(int *ndp, float *rad, char *sign, int *hmsf, int _sign);
void sla_caldj_(int *year, int *month, int *day, double *jm, int *status);
void sla_ctf2r_(int *hour, int *min, float *sec, float *rad, int *status);

#endif
