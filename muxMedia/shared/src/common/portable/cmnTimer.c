/*
* $Id$
*/

#include <cmnOsPort.h>

typedef	enum _timer_state
{
	CMN_TIMER_STATE_UNINIT = 0,
	CMN_TIMER_STATE_INITED,
	CMN_TIMER_STATE_RUNNING,
}cmn_timer_state_t;

typedef struct _cmn_timer_id
{
	int							interval;
	CMN_THREAD_TIMER_CALLBACK	cb;
	void 						*param;
	int							lastAlarm;
	struct _cmn_timer_id			*next;

	char							name[CMN_TIMER_NAME_LENGTH];
}cmn_timer_id_t;

/*
typedef	struct _timer_stat
{
	int							started;
	int							running;
	int							threaded;

	int							listChanged;
}cmn_timer_state_t;
*/
	
typedef	struct cmn_timer
{
	cmn_timer_state_t			state;
	
	/* used in threaded timers */
	int							numOfTimers;
	cmn_timer_id_t				*timers;
	
	cmn_mutex_t				*mutex;
#if 0
	pthread_t  					tId;
#else
	CmnThread					*tId;
#endif

	int							timersChanged;
	
	/* used in alarm timers */
	int							alarmInterval;
	CMN_ALARM_TIMER_CALLBACK 	alarmCallback;
	struct timeval				startTimeStamp;		/* The first ticks value of the application */

}cmn_timer_t;

static cmn_timer_t		_timers =
{
	state	:	CMN_TIMER_STATE_UNINIT,
};


int cmn_get_ticks (void)
{
	struct timeval now;
	int		ticks;

	gettimeofday(&now, NULL);
	ticks=(now.tv_sec - _timers.startTimeStamp.tv_sec)*1000+(now.tv_usec - _timers.startTimeStamp.tv_usec)/1000;

	return(ticks);
}

static void __handle_alarm_timer(int sig)
{
	int ms;

	if ( _timers.alarmCallback )
	{
		ms = (*_timers.alarmCallback)(_timers.alarmInterval );
		if ( ms != _timers.alarmInterval)
		{
			cmn_set_timer(ms, _timers.alarmCallback );
		}
	}
}

static int _sys_timer_init_alarm(void)
{
	struct sigaction action;

	/* Set the alarm handler (Linux specific) */
	memset(&action, 0, sizeof(action));
	action.sa_handler = __handle_alarm_timer;
	action.sa_flags = SA_RESTART;
	sigemptyset(&action.sa_mask);
	sigaction(SIGALRM, &action, NULL);
	
	return(0);
}

static int _sys_start_timer_alarm(void)
{
	struct itimerval timer;

	timer.it_value.tv_sec = (_timers.alarmInterval/1000);
	timer.it_value.tv_usec = (_timers.alarmInterval%1000)*1000;
	timer.it_interval.tv_sec = (_timers.alarmInterval/1000);
	timer.it_interval.tv_usec = (_timers.alarmInterval%1000)*1000;
	
	setitimer(ITIMER_REAL, &timer, NULL);
	return(0);
}

static void _sys_stop_timer_alarm(void)
{
	struct itimerval timer;

	memset(&timer, 0, (sizeof timer));
	setitimer(ITIMER_REAL, &timer, NULL);
}

static void __threaded_timer_check(void)
{
	int now, ms;
	cmn_timer_id_t	*t, *prev, *next;
	int removed;

	now = cmn_get_ticks( );

	cmn_mutex_lock(_timers.mutex);
	
	for ( prev = NULL, t = _timers.timers; t; t = next )
	{
		removed = 0;
		ms = t->interval - CMN_TIMESLICE;
		next = t->next;
		
		if ( (t->lastAlarm < now) && ((now - t->lastAlarm) > ms) )
		{
			if ( (now - t->lastAlarm) < t->interval )
			{
				t->lastAlarm += t->interval;
			}
			else
			{
				t->lastAlarm = now;
			}

			_timers.timersChanged = FALSE;

#if 1//CMN_TIMER_DEBUG
			MUX_DEBUG( "Executing timer %s(%p) (thread = %s)", t->name, t, cmnThreadGetName() );
#endif

			cmn_mutex_unlock(_timers.mutex );

			ms = t->cb(t->interval, t->param);
			
			cmn_mutex_lock(_timers.mutex);
			
			if ( _timers.timersChanged )
			{/* Abort, list of timers has been modified by other threads */
				break;
			}

			if ( ms != t->interval )
			{
				if ( ms )
				{
					t->interval = ROUND_RESOLUTION(ms);
				}
				else
				{ /* Remove the timer from the linked list */

#if CMN_TIMER_DEBUG
					MUX_DEBUG("Removing timer %s(%p) after it has been executed", t->name, t);
#endif

					if ( prev )
					{
						prev->next = next;
					}
					else
					{
						_timers.timers = next;
					}

					cmn_free(t);
					-- _timers.numOfTimers;
					removed = 1;
				}
			}
		}
		
		/* Don't update prev if the timer has disappeared */
		if ( ! removed )
		{
			prev = t;
		}
	}
	
	cmn_mutex_unlock(_timers.mutex);
}

#if 0
static void *__run_thread_timers(void *unused)
#else
static int __run_thread_timers(CmnThread *unused)
#endif
{
	while( _timers.state != CMN_TIMER_STATE_UNINIT )
	{
		//MUX_DEBUG( "wakeup ...");
		if ( _timers.state == CMN_TIMER_STATE_RUNNING)
		{
		//	MUX_DEBUG( "wakeup to check timers...");
			__threaded_timer_check();
		}
		
		cmn_delay(1);
	}
#if 0
	return NULL;
#else
	return EXIT_SUCCESS;
#endif
}


/* This is only called if the event thread is not running */
static int _sys_timer_init(void)
{
#if 0
	if( pthread_create(&_timers.tId, NULL, __run_thread_timers, NULL ) != 0)
#else
	_timers.tId = cmnThreadCreateThread(__run_thread_timers, NULL, "CmnTimer");
	if(!_timers.tId )
#endif
	{
		return (-EXIT_FAILURE);
	}

	return EXIT_SUCCESS;
}


static void _sys_timer_quit(void)
{
	_timers.state = CMN_TIMER_STATE_UNINIT;

//	if ( _timers.tId )
	{
#if 0	
		pthread_join(_timers.tId, NULL);
#else		
		cmnThreadJoin(_timers.tId);
#endif
		cmn_mutex_destroy(_timers.mutex);
		_timers.mutex = NULL;
	}
}

/* in unit of ms, round to 10 ms */
void *cmn_add_timer(int interval, CMN_THREAD_TIMER_CALLBACK callback, void *param, char *name)
{
	cmn_timer_id_t  *t;
	
	cmn_mutex_lock(_timers.mutex);
	t = (cmn_timer_id_t *)cmn_malloc(sizeof( cmn_timer_id_t));
	if ( t )
	{
		t->interval = ROUND_RESOLUTION(interval);
		t->cb = callback;
		t->param = param;
		t->lastAlarm = cmn_get_ticks();
		snprintf(t->name, CMN_TIMER_NAME_LENGTH, "%s", name);
		
		t->next = _timers.timers;
		_timers.timers = t;

		++ _timers.numOfTimers;
		
		_timers.timersChanged = TRUE;
		_timers.state = CMN_TIMER_STATE_RUNNING;
	}
	
#if CMN_TIMER_DEBUG
	MUX_WARN("Added Timer(%s: %d second) = %08x num_timers = %d", t->name, interval, (POINTRT_TO_INTEGER)t, _timers.numOfTimers );
#endif

	cmn_mutex_unlock(_timers.mutex);
	return t;
}

int cmn_remove_timer(void *_tid)
{
	cmn_timer_id_t *t, *prev = NULL;
	int removed = FALSE;
	cmn_timer_id_t *id = (cmn_timer_id_t *)_tid;
	if(id == NULL)
		return -EXIT_FAILURE;

	cmn_mutex_lock(_timers.mutex);
	/* Look for id in the linked list of timers */
	for (t = _timers.timers; t; prev=t, t = t->next )
	{
		if ( t == id )
		{
			if(prev)
			{
				prev->next = t->next;
			}
			else
			{
				_timers.timers = t->next;
			}
			
#if CMN_TIMER_DEBUG
			MUX_DEBUG("Timer (%s: %08x) = %d num_timers = %d thread = %s is removed", t->name,
				(POINTRT_TO_INTEGER)id, removed, _timers.numOfTimers, cmnThreadGetName() );
#endif
			cmn_free(t);
			
			-- _timers.numOfTimers;
			
			removed = TRUE;
			_timers.timersChanged = TRUE;
			break;
		}
	}
	

	cmn_mutex_unlock(_timers.mutex);
	return removed;
}

static void _remove_all_timers(cmn_timer_id_t *t)
{
	cmn_timer_id_t *freeme;
	/* Changed to non-recursive implementation.
	   The recursive implementation is elegant, but subject to stack overflow if there are lots and lots of timers. */
	while ( t )
	{
		freeme = t;
		t = t->next;
		cmn_free(freeme);
	}
}

void cmn_cancel_timers(cmn_timer_mode_t mode)
{
	if(mode == CMN_TIMER_ITIMER)
	{
		return _sys_stop_timer_alarm();
	}
	
	if( _timers.state == CMN_TIMER_STATE_RUNNING )
	{/* Stop any currently running timer */
		cmn_mutex_lock(_timers.mutex);
		_remove_all_timers(_timers.timers );
		_timers.timers = NULL;
		cmn_mutex_unlock(_timers.mutex);
		
		_timers.state = CMN_TIMER_STATE_INITED;
	}
}

/* function interface for alarm syscall implemented timer. eg. set_timer, not add_timer 
* so remove current alarm timer and added this alarm timer 
*/
int cmn_set_timer(int ms, CMN_ALARM_TIMER_CALLBACK callback)
{
	int retval = 0;

#if CMN_TIMER_DEBUG
	MUX_DEBUG("cmn_set_timer(%d)", ms);
#endif

	cmn_cancel_timers(CMN_TIMER_ITIMER);
	
	if ( ms > 0 )
	{
		_timers.alarmInterval = ms;
		_timers.alarmCallback = callback;
		
		retval = _sys_start_timer_alarm( );
	}
	
	return retval;
}


int cmn_timer_init(void)
{
	int ret1 = 0, ret2 = 0;

	if( _timers.state != CMN_TIMER_STATE_UNINIT)
	{
		MUX_WARN("Timer already initialized");
		return EXIT_FAILURE;
	}

	memset(&_timers, 0 , sizeof(cmn_timer_t) );
	
	gettimeofday(&_timers.startTimeStamp, NULL);
	_timers.state = CMN_TIMER_STATE_INITED;

	ret1 = _sys_timer_init();
	ret2 = _sys_timer_init_alarm();
	if(ret1 <0 || ret2 < 0)
		return EXIT_FAILURE;
	
	_timers.mutex = cmn_mutex_init();

	return EXIT_SUCCESS;
}

void cmn_timer_quit(void)
{
	cmn_cancel_timers(CMN_TIMER_ITIMER);
	cmn_cancel_timers(CMN_TIMER_THREADED);

	_sys_timer_quit();
}

