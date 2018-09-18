/*
* $Id: cmn_mt.h,v 1.2 2007/03/07 16:18:08 lizhijie Exp $
*/

#ifndef __CMN_THREAD_INFO_H__
#define __CMN_THREAD_INFO_H__


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <internal.h>

/* @brief Thread, Mutex and Semaphore definitions
 * Those methods are only available if the library is compile in multi threaded mode.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#define	CMN_THREAD_FLAG_WAIT				1


#define	CMN_THREAD_FLAG_DYNAMIC			16	/* dynamic thread, detached thread */


/* thread wait on get event from queue when only event handler is defined for this thread */
#define	CMN_THREAD_WAIT_SET(th)	\
					CMN_SET_BIT(th->flags, CMN_THREAD_FLAG_WAIT)

#define	CMN_THREAD_WAIT_CHECK(th)	\
					CMN_CHECK_BIT(th->flags, CMN_THREAD_FLAG_WAIT)


#define	CMN_THREAD_DYNAMIC_SET(th)	\
					CMN_SET_BIT(th->flags, CMN_THREAD_FLAG_DYNAMIC)

#define	CMN_THREAD_DYNAMIC_CHECK(th)	\
					CMN_CHECK_BIT(th->flags, CMN_THREAD_FLAG_DYNAMIC)


typedef	struct _CmnThread
{
	char				name[CMN_NAME_LENGTH];

	pthread_t  		id;
	int				pId;
	int  				priority;

	cmn_fifo_t		*queue;

	int				flags;

	int				(*init)(struct _CmnThread *th, void *data);

	int				(*mainLoop)(struct _CmnThread *th);
	int				(*eventHandler)(struct _CmnThread *th, void *event);

	void				(*destory)(struct _CmnThread *th);
	
	void				*data;
}CmnThread;

//int cmnThreadMainloopWait(CmnThread *th);
int	cmnThreadAddEvent(CmnThread *th, void *event);
int	cmnThreadJoin(CmnThread *th );
int	cmnThreadDestory(CmnThread *th );
int 	cmnThreadId(void);
int	cmnThreadInit(CmnThread *th, void *data );

char	*cmnThreadGetName();
int	cmnThreadSetThreadName(pthread_t thId, char *threadName);
int	cmnThreadSetName(const char *format,...);

int	cmnThreadSetSignalHandler(int signum, void (*saHandler)(int) );
int cmnThreadMaskAllSignals(void);

CmnThread *cmnThreadCreateThread(int (*mainHandler)(CmnThread *), void *data, char *name);

int cmnThreadLibInit(void);
void cmnThreadLibDestroy(void);

cmn_list_t *cmnThreadLockList(void);
void cmnThreadReleaseList(void );


#ifdef __cplusplus
}
#endif

#endif

