/*
* $Id: cmn_mt.h,v 1.2 2007/03/07 16:18:08 lizhijie Exp $
*/

#ifndef __CMN_MUTEX_H__
#define __CMN_MUTEX_H__


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <internal.h>

/**
 * @brief Thread, Mutex and Semaphore definitions
 * Those methods are only available if the library is compile in multi threaded mode.
 */

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Allocate and Initialise a semaphore.
 * @param value The initial value for the semaphore.
 */
cmn_sem_t *cmn_sem_init (unsigned int value);

int cmn_sem_destroy (cmn_sem_t *sem);

/**
 * Post operation on a semaphore.
 * @param sem The semaphore to destroy.
 */
int cmn_sem_post (cmn_sem_t *sem);

/**
 * Wait operation on a semaphore.
 * NOTE: this call will block if the semaphore is at 0.
 * @param sem The semaphore to destroy.
 */
int cmn_sem_wait (cmn_sem_t *sem);

/**
 * Wait operation on a semaphore.
 * NOTE: if the semaphore is at 0, this call won't block.
 * @param sem The semaphore to destroy.
 */
int cmn_sem_trywait (cmn_sem_t *sem);


cmn_mutex_t *cmn_mutex_init (void);

/* Destroy the mutex. @param mut The mutex to destroy. */
void cmn_mutex_destroy (cmn_mutex_t *mut);

/* Lock the mutex. @param mut The mutex to lock. */
int cmn_mutex_lock (cmn_mutex_t *mut);

/* Unlock the mutex. @param mut The mutex to unlock. */
int cmn_mutex_unlock(cmn_mutex_t *mut);


/* condv implementation */
cmn_cond_t  *cmn_cond_init (void);

/* Destroy a condition variable @param cond The condition variable to destroy. */
int cmn_cond_destroy (cmn_cond_t *cond);

/* Signal the condition variable. @param cond The condition variable to signal. */
int cmn_cond_signal (cmn_cond_t *cond);

/* Wait on the condition variable. @param cond The condition variable to wait on. @param mut The external mutex */
int cmn_cond_wait (cmn_cond_t *cond, cmn_mutex_t *mut);

/**
 * Timed wait on the condition variable.
 * @param cond The condition variable to wait on.
 * @param mut The external mutex 
 * @param abstime time to wait until
 */
int cmn_cond_timedwait(cmn_cond_t *cond, cmn_mutex_t *mut, const struct timespec *abstime);

#ifdef __cplusplus
}
#endif

#endif

