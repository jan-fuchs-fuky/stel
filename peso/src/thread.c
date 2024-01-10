/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 */

#include <string.h>
#include <errno.h>
#include <time.h>
#include <log4c.h>
#include <semaphore.h>

#include "exposed.h"
#include "thread.h"

int pthr_mutex_lock(pthread_mutex_t *p_mutex)
{
    int result;

    if ((result = pthread_mutex_lock(p_mutex)) != 0)
    {
        //log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "Warning: pthread_mutex_lock(): %i: %s", errno, strerror(errno));
    }

    return result;
}

int pthr_mutex_unlock(pthread_mutex_t *p_mutex)
{
    int result;

    if ((result = pthread_mutex_unlock(p_mutex)) != 0)
    {
        //log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "Warning: pthread_mutex_unlock(): %i: %s", errno, strerror(errno));
    }

    return result;
}

int pthr_sem_wait(sem_t *p_sem, int timeout)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        //log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "Warning: sem_wait(): %i: %s", errno, strerror(errno));
        return -1;
    }

    ts.tv_sec += timeout;

    if (sem_timedwait(p_sem, &ts) == -1)
    {
        if (errno != ETIMEDOUT)
        {
            //log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "Warning: sem_wait(): %i: %s", errno, strerror(errno));
        }

        return -1;
    }

    return 0;
}

int pthr_sem_post(sem_t *p_sem)
{
    int result;

    if ((result = sem_post(p_sem)) != 0)
    {
        //log4c_category_log(p_logcat, LOG4C_PRIORITY_WARN, "Warning: sem_post(): %i: %s", errno, strerror(errno));
    }

    return result;
}
