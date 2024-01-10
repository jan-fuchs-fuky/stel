/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 */

#ifndef __MOD_CCD_H
#define __MOD_CCD_H

int mod_ccd_check_state(int state);

void peso_set_int(int *p_peso_int, int number);
void peso_get_int(int *p_peso_int, int *p_number);
void peso_set_double(double *p_peso_double, double number);
void peso_set_float(float *p_peso_float, float number);
void peso_get_float(float *p_peso_float, float *p_number);
void peso_set_str(char *p_peso_str, char *p_str, int str_len);
void peso_set_time(time_t *p_peso_time, time_t value);
void peso_set_imgtype(CCD_IMGTYPE_T imgtype);
void peso_set_state(CCD_STATE_T state);

const char *peso_get_version(void);

#endif
