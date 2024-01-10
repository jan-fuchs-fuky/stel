/**
  * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
  * $Date$
  * $Rev$
  * $URL$
 */

#ifndef __THREAD_H
#define __THREAD_H

#include <pthread.h>
#include <semaphore.h>

int pthr_mutex_lock(pthread_mutex_t *p_mutex);
int pthr_mutex_unlock(pthread_mutex_t *p_mutex);
int pthr_sem_wait(sem_t *p_sem);
int pthr_sem_post(sem_t *p_sem);

#endif
