/**
  * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
  * $Date$
  * $Rev$
  * $URL$
 */

#include <string.h>
#include <errno.h>
#include <log4c.h>

#include "telescoped.h"
#include "thread.h"

int pthr_mutex_lock(pthread_mutex_t *p_mutex)
{
    int result;

    if ((result = pthread_mutex_lock(p_mutex)) != 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "pthread_mutex_lock() failure: %i: %s", errno, strerror(errno));
    }

    return result;
}

int pthr_mutex_unlock(pthread_mutex_t *p_mutex)
{
    int result;

    if ((result = pthread_mutex_unlock(p_mutex)) != 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "pthread_mutex_unlock() failure: %i: %s", errno, strerror(errno));
    }

    return result;
}

int pthr_sem_wait(sem_t *p_sem)
{
    int result;

    if ((result = sem_wait(p_sem)) != 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "sem_wait() failure: %i: %s", errno, strerror(errno));
    }

    return result;
}

int pthr_sem_post(sem_t *p_sem)
{
    int result;

    if ((result = sem_post(p_sem)) != 0) {
        log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "sem_post() failure: %i: %s", errno, strerror(errno));
    }

    return result;
}
