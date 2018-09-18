/*
* $Id: port_sema.c,v 1.2 2007/03/07 16:18:08 lizhijie Exp $
*/

#define _GNU_SOURCE  

#include "libCmn.h"

//#include <cmnOsPort.h>
#include <sys/types.h>

#include <unistd.h>
#include <sys/syscall.h>


#ifdef SYS_gettid
	#define	GETTID() 	syscall(SYS_gettid)
#else
#error "SYS_gettid unavailable on this system"
#endif

static cmn_list_t 	_threadList;
static cmn_mutex_t		*_threadLock;

int	cmnThreadAddEvent(CmnThread *th, void *event)
{
	return cmn_fifo_add( th->queue, event);
}

static int	_cmnThreadEventHandler(CmnThread *th)
{
	void *event;

	if(CMN_THREAD_WAIT_CHECK(th) || th->mainLoop==NULL )
	{
		event = cmn_fifo_get( th->queue);
	}
	else
	{
		event = cmn_fifo_tryget( th->queue);
	}

#if 1
	/* only one event processed in one loop, so performance of send is optimzied */
	if(event )
	{
		return th->eventHandler(th, event);
	}
#else
	while(event)
	{/* all events must be handled in just one loop */
		th->eventHandler(th, event);
		cmn_free(event);

		event = cmn_fifo_tryget( &th->queue);
	};
#endif

	return 0;
}


static void _cmnThreadCleanup(void *_data)
{
	int	i;
	CmnThread *th = (CmnThread *)_data;
	cmn_list_t *threads = NULL;

#if CMN_THREAD_DEBUG
	MUX_INFO("Thread '%s' exit and is removing.....", th->name );
#endif
	threads = cmnThreadLockList();
	for(i=0; i < cmn_list_size(threads); i++)
	{
		CmnThread *temp = (CmnThread *)cmn_list_get(threads, i);
		if(temp == th)
		{
			cmn_list_remove(threads, i);

#if CMN_THREAD_DEBUG
			MUX_DEBUG("Remove thread '%s' from thread queue", th->name);
#endif			
			/* remove zombie thread from memory */
			cmnThreadJoin(th);
			
			cmnThreadDestory(th);
			
			cmn_free(th);

			cmnThreadReleaseList();
			return;
		}
	}

	cmnThreadReleaseList();
	MUX_ERROR("Thread '%s' not found in queue", th->name);

}

static void *_cmnThreadMain(void *param)
{
	CmnThread *th = (CmnThread *)param;

#if CMN_THREAD_DEBUG
	MUX_DEBUG( "Task '%s' (%d, getpthread_self: %lu tid:%lu) running.....", th->name, getpid(), pthread_self(), GETTID() );
#endif

	if(CMN_THREAD_DYNAMIC_CHECK(th) )
	{
		int            oldtype;
#if CMN_THREAD_DEBUG
		MUX_DEBUG("Thread '%s' set cancel_enable");
#endif
		if(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldtype)!=0)
		{
			MUX_WARN("Set thread '%s' as CANCEL_ENABLE failed : %s", strerror(errno));
		}
#if 0
		if(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype) != 0)
		{
			MUX_WARN("Set thread '%s' as CANCEL_ASYNC failed : %s", strerror(errno));
		}
#endif		
	}

	pthread_cleanup_push(_cmnThreadCleanup, param);

	th->pId = cmnThreadId();
	
	while(1)
	{
		if( th->mainLoop != NULL)
		{/* when mainLoop is null, only events are handled by this thread; 05.16,2017 */
			if(th->mainLoop(th)< 0)
			{
#if CMN_THREAD_DEBUG
				MUX_WARN("Task '%s' Quit from MainLoop", th->name);
#endif
				goto exiter;
//				_cmnThreadCleanup(th);
//				return NULL;
			}
		}

		if(th->eventHandler)
		{
			if(_cmnThreadEventHandler( th) <0 )
			{
#if CMN_THREAD_DEBUG
				MUX_WARN("Task '%s' Quit from EventHandler", th->name);
#endif
				goto exiter;
//				_cmnThreadCleanup(th);
//				return NULL;
			}
		}
	}

exiter:

#if CMN_THREAD_DEBUG
	MUX_INFO("'%s' Task running cleanup....", th->name );
#endif

	pthread_cleanup_pop(1);

	return NULL;
}

#if 0
int cmnThreadMainloopWait(CmnThread *th)
{
	int ret = EXIT_FAILURE;
	void *event;

	/* wait for next event */
	event = cmn_fifo_get( &th->queue);

	/* only one event processed in one loop, so performance of send is optimzied */
	while(event )
	{
		MUX_DEBUG( "main loop of '%s'", th->name );
		if(th->eventHandler)
		{
			ret = th->eventHandler(th, event);
		}
	
		event = cmn_fifo_get( &th->queue);
	}

	return ret;
}
#endif


int	cmnThreadJoin(CmnThread *th )
{
	int	*pid;
	
	if( pthread_join(th->id, (void **)&pid) != 0)
	{
//		MUX_ERROR("Join '%s' thread failed: %s", th->name, strerror(errno));
		return -EXIT_FAILURE;
	}
	
#if CMN_THREAD_DEBUG
	MUX_ERROR("Join '%s' thread OK : %d", th->name, *pid);
#endif
	return EXIT_SUCCESS;
}

int	cmnThreadDestory(CmnThread *th )
{
#if CMN_THREAD_DEBUG
	MUX_INFO("Thread '%s' is destroy.....", th->name);
#endif

	if( th->destory)
	{
		th->destory(th);
	}
	
//	TRACE();
	cmn_fifo_free( th->queue);
//	TRACE();

//	pthread_exit(&th->pId);

#if CMN_THREAD_DEBUG
	MUX_INFO("Thread '%s' is destroyed!", th->name);
#endif
	return 0;
}

int	cmnThreadInit(CmnThread *th, void *data )
{
	pthread_attr_t attr;
	struct sched_param schedulingValue;
	cmn_list_t *threads = NULL;

#if CMN_THREAD_DEBUG
	MUX_DEBUG("Task '%s' initting.....", th->name );
#endif

	/* FIFO must be created before init: sometimes init will create event which needs FIFO; 05.26.2017 */	
	cmn_fifo_init(DEFAULT_THREAD_QUEUE_LENGTH, &th->queue);
	
	if( th->init)
	{
		if(th->init(th, data) != EXIT_SUCCESS)
		{
			MUX_ERROR("'%s' Task init failed", th->name );
			return EXIT_FAILURE;
		}
	}
	
	schedulingValue.sched_priority = th->priority; 

	pthread_attr_init(&attr);
	if(CMN_THREAD_DYNAMIC_CHECK(th) )
	{
#if CMN_THREAD_DEBUG
		MUX_DEBUG("Thread '%s' is detachable");
#endif
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	}
	
	pthread_attr_setschedparam(&attr, &schedulingValue);

	
//	res = pthread_create(&thread->id, &attr, thread->handler, thread);
	if( pthread_create(&th->id, &attr, _cmnThreadMain, th ) != 0)
	{
		MUX_ERROR("Error while creating a new Task '%s': %s", th->name, strerror(errno) );
		pthread_attr_destroy( &attr);
		return EXIT_FAILURE;
	}

	pthread_attr_destroy( &attr);
	cmnThreadSetThreadName(th->id, th->name);

	threads = cmnThreadLockList();
	cmn_list_append(threads, th);
	cmnThreadReleaseList();

#if CMN_THREAD_DEBUG
	MUX_DEBUG("Task '%s' init finished", th->name );
#endif
	return EXIT_SUCCESS;
}

int	cmnThreadId(void)
{
//	return(getpid());
	return(GETTID());
	//return((int)pthread_self());
}

char	*cmnThreadGetName()
{
	static char threadName[16] = "Unknown";
	pthread_t             self = pthread_self();

	if(pthread_getname_np(self, threadName, sizeof(threadName)) != 0)
	{
		MUX_WARN("Fail in get thread name: %s", strerror(errno));
		return "Unknown";
	}

	return threadName;
}

int	cmnThreadSetSignalHandler(int signum, void (*saHandler)(int) )
{
	struct sigaction        actions;

	memset(&actions, 0, sizeof(actions));
	sigemptyset(&actions.sa_mask);

	actions.sa_flags = 0;
	actions.sa_handler = saHandler;

	if(sigaction(signum, &actions,NULL) != 0)
	{
		MUX_ERROR("Thread '%s' failed in set signal handler", cmnThreadGetName() );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


int cmnThreadMaskAllSignals(void)
{
	sigset_t              mask;

	if(sigfillset(&mask) != 0) /* Mask all allowed signals */
	{
		MUX_ERROR("Thread '%s' failed in sigfillset for all signals", cmnThreadGetName());
		return EXIT_FAILURE;
	}
	
	if(pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0)
	{
		MUX_ERROR("Thread '%s' failed in mask all signals", cmnThreadGetName());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


int	cmnThreadSetThreadName(pthread_t thId, char *threadName)
{
	if( pthread_setname_np(thId, threadName) != 0)
	{
		MUX_WARN("Error in set thread name '%s'", threadName );
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

int	cmnThreadSetName(const char *format,...)
{
	pthread_t	self = pthread_self();
	char		threadName[16];
	va_list marker;

	va_start( marker, format );

	memset(threadName, 0, sizeof(threadName));

	vsnprintf(threadName, sizeof(threadName), format, marker);
	va_end( marker);

	return cmnThreadSetThreadName(self, threadName);
}


CmnThread *cmnThreadCreateThread( int (*mainHandler)(CmnThread *), void *data, char *name)
{
	CmnThread *th = NULL;

	if(!mainHandler)
	{
		MUX_WARN("No main looper in thread '%s'", name);
		return NULL;
	}
	
	th = cmn_malloc(sizeof(CmnThread) );
	
	snprintf(th->name, sizeof(th->name), "%s", name);
	CMN_THREAD_DYNAMIC_SET(th);
	th->mainLoop = mainHandler;
	th->data = data;

#if CMN_THREAD_DEBUG
	MUX_INFO("Dynamic Thread '%s' is creating...", th->name );
#endif
	if(cmnThreadInit(th, data) == EXIT_FAILURE)
	{
		cmnThreadDestory( th);
		cmn_free(th);
		return NULL;
	}

	return th;
}


int cmnThreadLibInit(void)
{
	cmn_list_init(&_threadList);
	_threadLock = cmn_mutex_init();

	return EXIT_SUCCESS;
}


void cmnThreadLibDestroy(void)
{
	/* free thread one by one */
	cmn_list_ofchar_free(&_threadList, FALSE);

	cmn_mutex_destroy(_threadLock);
}

cmn_list_t *cmnThreadLockList(void)
{
	cmn_mutex_lock(_threadLock);
	return &_threadList;
}

void cmnThreadReleaseList(void )
{
	cmn_mutex_unlock(_threadLock);
}

void cmnThreadMask(char *threadName)
{
	sigset_t set;
	int res;

	/* Block SIGQUIT and SIGUSR1; other threads created by main()
	will inherit a copy of the signal mask. */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	res = pthread_sigmask(SIG_BLOCK, &set, NULL);
	if (res != 0)
	{
		MUX_ERROR("Signal Block for thread '%s' failed : %s", threadName, strerror(errno) );
		exit(1);
	}

}

