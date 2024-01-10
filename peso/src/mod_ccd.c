/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 * $URL$
 */

#include <string.h>

#include "modules.h"
#include "mod_ccd.h"
#include "thread.h"

int mod_ccd_check_state(int state)
{
    if (state != CCD_STATE_READY_E)
    {
        strncpy(peso.msg, "ccd not ready", CCD_MSG_MAX);
        return -1;
    }

    return 0;
}

void peso_set_int(int *p_peso_int, int number)
{
    /* LOCK */
    pthr_mutex_lock(peso.p_global_mutex);
    *p_peso_int = number;
    pthr_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */
}

void peso_get_int(int *p_peso_int, int *p_number)
{
    /* LOCK */
    pthr_mutex_lock(peso.p_global_mutex);
    *p_number = *p_peso_int;
    pthr_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */
}

void peso_set_double(double *p_peso_double, double number)
{
    /* LOCK */
    pthr_mutex_lock(peso.p_global_mutex);
    *p_peso_double = number;
    pthr_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */
}

void peso_set_float(float *p_peso_float, float number)
{
    /* LOCK */
    pthr_mutex_lock(peso.p_global_mutex);
    *p_peso_float = number;
    pthr_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */
}

void peso_get_float(float *p_peso_float, float *p_number)
{
    /* LOCK */
    pthr_mutex_lock(peso.p_global_mutex);
    *p_number = *p_peso_float;
    pthr_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */
}

void peso_set_str(char *p_peso_str, char *p_str, int str_len)
{
    /* LOCK */
    pthr_mutex_lock(peso.p_global_mutex);
    strncpy(p_peso_str, p_str, str_len);
    pthr_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */
}

void peso_set_time(time_t *p_peso_time, time_t value)
{
    /* LOCK */
    pthr_mutex_lock(peso.p_global_mutex);
    *p_peso_time = value;
    pthr_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */
}

void peso_set_imgtype(CCD_IMGTYPE_T imgtype)
{
    /* LOCK */
    pthr_mutex_lock(peso.p_global_mutex);
    peso.imgtype = imgtype;
    pthr_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */
}

void peso_set_state(CCD_STATE_T state)
{
    /* LOCK */
    pthr_mutex_lock(peso.p_global_mutex);
    peso.state = state;
    pthr_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */
}

const char *peso_get_version(void)
{
    return SVN_REV;
}
