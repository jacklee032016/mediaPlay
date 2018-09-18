/*
* $Id: port_sema.c,v 1.2 2007/03/07 16:18:08 lizhijie Exp $
*/

#include <cmnOsPort.h>

#if defined(HAVE_PTHREAD) || defined(HAVE_PTH_PTHREAD_H)

cmn_mutex_t *cmn_mutex_init(void)
{
	cmn_mutex_t *mut = (cmn_mutex_t *) cmn_malloc (sizeof (cmn_mutex_t));

	if (mut == NULL)
		return NULL;
	pthread_mutex_init (mut, NULL);
	return (cmn_mutex_t *) mut;
}

void cmn_mutex_destroy(cmn_mutex_t *_mut)
{
	cmn_mutex_t *mut = (cmn_mutex_t *) _mut;
	if (mut == NULL)
		return;
	pthread_mutex_destroy (mut);
	cmn_free (mut);
}

int cmn_mutex_lock (cmn_mutex_t *_mut)
{
	cmn_mutex_t *mut = (cmn_mutex_t *) _mut;
	if (mut == NULL)
		return -1;
	return pthread_mutex_lock (mut);
}

int cmn_mutex_unlock(cmn_mutex_t *_mut)
{
	cmn_mutex_t *mut = (cmn_mutex_t *) _mut;
	if (mut == NULL)
		return -1;
	return pthread_mutex_unlock (mut);
}
#endif

#if defined (HAVE_SEMAPHORE_H) && !defined(__APPLE_CC__)

/* Counting Semaphore is initialized to value */
cmn_sem_t *cmn_sem_init (unsigned int value)
{
	cmn_sem_t *sem = (cmn_sem_t *) cmn_malloc (sizeof (cmn_sem_t));

	if (sem_init (sem, 0, value) == 0)
		return (cmn_sem_t *) sem;

	cmn_free (sem);
	return NULL;
}

int cmn_sem_destroy(cmn_sem_t *_sem)
{
	cmn_sem_t *sem = (cmn_sem_t *) _sem;
	if (sem == NULL)
		return 0;
	
	sem_destroy (sem);
	cmn_free (sem);
	return 0;
}

int cmn_sem_post (cmn_sem_t *_sem)
{
	cmn_sem_t *sem = (cmn_sem_t *) _sem;
	if (sem == NULL)
		return -1;
	return sem_post (sem);
}

int cmn_sem_wait (cmn_sem_t *_sem)
{
	cmn_sem_t *sem = (cmn_sem_t *) _sem;
	if (sem == NULL)
		return -1;
	return sem_wait (sem);
}

int cmn_sem_trywait (cmn_sem_t *_sem)
{
	cmn_sem_t *sem = (cmn_sem_t *) _sem;
	if (sem == NULL)
		return -1;
	return sem_trywait (sem);
}

#elif defined (HAVE_SYS_SEM_H)
/* support for semctl, semop, semget */
#define SEM_PERM 0600

cmn_sem_t *cmn_sem_init (unsigned int value)
{
	union semun val;
	int i;
	cmn_sem_t *sem = (cmn_sem_t *) cmn_malloc (sizeof (cmn_sem_t));

	sem->semid = semget (IPC_PRIVATE, 1, IPC_CREAT | SEM_PERM);
	if (sem->semid == -1)
	{
		MUX_ERROR( "semget error : %s", strerror(errno) );
		cmn_free (sem);
		return NULL;
	}

	val.val = (int) value;
	i = semctl (sem->semid, 0, SETVAL, val);
	if (i != 0)
	{
		MUX_ERROR( "semctl error : %s", strerror(errno) );
		cmn_free (sem);
		return NULL;
	}

	return(cmn_sem_t *) sem;
}

int cmn_sem_destroy (cmn_sem_t *_sem)
{
	union semun val;
	cmn_sem_t *sem = (cmn_sem_t *) _sem;
	if (sem == NULL)
		return 0;
	
	val.val = 0;
	semctl (sem->semid, 0, IPC_RMID, val);
	cmn_free (sem);
	return 0;
}

int cmn_sem_post (cmn_sem_t *_sem)
{
	struct sembuf sb;
	cmn_sem_t *sem = (cmn_sem_t *) _sem;

	if (sem == NULL)
		return -1;

	sb.sem_num = 0;
	sb.sem_op = 1;
	sb.sem_flg = 0;
	return semop (sem->semid, &sb, 1);
}

int cmn_sem_wait (cmn_sem_t *_sem)
{
	struct sembuf sb;
	cmn_sem_t *sem = (cmn_sem_t *) _sem;

	if (sem == NULL)
		return -1;
	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = 0;
	return semop (sem->semid, &sb, 1);
}

int cmn_sem_trywait(cmn_sem_t *_sem)
{
	struct sembuf sb;
	cmn_sem_t *sem = (cmn_sem_t *) _sem;

	if (sem == NULL)
		return -1;
	sb.sem_num = 0;
	sb.sem_op = -1;
	sb.sem_flg = IPC_NOWAIT;
	return semop (sem->semid, &sb, 1);
}

#endif

cmn_cond_t *cmn_cond_init ()
{
	cmn_cond_t *cond = (cmn_cond_t *) cmn_malloc (sizeof (cmn_cond_t));
	if (cond && (pthread_cond_init (&cond->cv, NULL) == 0))
	{
		return (cmn_cond_t *) (cond);
	}
	cmn_free (cond);

	return NULL;
}

int cmn_cond_destroy (cmn_cond_t *_cond)
{
	int ret;

	ret = pthread_cond_destroy (&_cond->cv);
	cmn_free (_cond);
	return ret;
}

int cmn_cond_signal (cmn_cond_t *_cond)
{
	return pthread_cond_signal (&_cond->cv);
}


int cmn_cond_wait (cmn_cond_t *_cond, cmn_mutex_t *_mut)
{
	return pthread_cond_wait (&_cond->cv, (pthread_mutex_t *) _mut);
}

int cmn_cond_timedwait (cmn_cond_t *_cond, cmn_mutex_t *_mut, const struct timespec *abstime)
{
	return pthread_cond_timedwait (&_cond->cv, (pthread_mutex_t *) _mut, (const struct timespec *) abstime);
}

