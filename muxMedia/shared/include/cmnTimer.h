/*
* $Id$
*/

#ifndef	__CMN_TIMER_H__
#define	__CMN_TIMER_H__


typedef	enum
{
	CMN_TIMER_THREADED,
	CMN_TIMER_ITIMER,	
}cmn_timer_mode_t;

#define CMN_TIMESLICE						10

/* This is the maximum resolution of the timer on all platforms */
#define CMN_TIMER_RESOLUTION				10	/* Experimentally determined */

#define ROUND_RESOLUTION(X)	\
	(((X+CMN_TIMER_RESOLUTION-1)/CMN_TIMER_RESOLUTION)*CMN_TIMER_RESOLUTION)

/* used by other applications */
#define	CMN_TIMER_1_SECOND		1000


#define	CMN_TIMER_NAME_LENGTH		256


/* Function prototype for the timer callback function */
typedef int (*CMN_ALARM_TIMER_CALLBACK)(int interval);

/* New timer API, supports multiple timers
* The callback function is passed the current timer interval and returns the next timer interval.  If the 
* returned value is the same as the one passed in, the periodic alarm continues, otherwise a new alarm 
* is scheduled.  If the callback returns 0, the periodic alarm is cancelled.
 */
typedef int (*CMN_THREAD_TIMER_CALLBACK)(int interval, void *param);


/* Get the number of milliseconds since the library initialization.
 * Note that this value wraps if the program runs for more than ~49 days.
 */ 
extern int cmn_get_ticks(void);

/* Wait a specified number of milliseconds before returning */
extern void cmn_delay(int	ms);

/* Set a callback to run after the specified number of milliseconds has elapsed. The callback function 
* is passed the current timer interval and returns the next timer interval.  If the returned value is the 
* same as the one passed in, the periodic alarm continues, otherwise a new alarm is scheduled.  
* If the callback returns 0, the periodic alarm is cancelled.
*
* To cancel a currently running timer, call cmn_set_timer(0, NULL);
*
* The timer callback function may run in a different thread than your
* main code, and so shouldn't call any functions from within itself.
*
* The maximum resolution of this timer is 10 ms, which means that if you request a 16 ms timer, your 
* callback will run approximately 20 ms later on an unloaded system.  If you wanted to set a flag signaling
* a frame update at 30 frames per second (every 33 ms), you might set a timer for 30 ms:
*   cmn_set_timer((33/10)*10, flag_update);
*
* Under UNIX, you should not use raise or use SIGALRM and this function in the same program, as it is 
* implemented using setitimer().  You also should not use this function in multi-threaded applications as 
* signals to multi-threaded apps have undefined behavior in some implementations.
 */
extern int cmn_set_timer(int interval /* in unit of ms, round to 10 ms */, CMN_ALARM_TIMER_CALLBACK callback);


/* Add a new timer to the pool of timers already running. Returns a timer ID, or NULL when an error occurs. */
extern void *cmn_add_timer(int interval /* in unit of ms, round to 10 ms */, CMN_THREAD_TIMER_CALLBACK callback, void *param, char *name);


/* Remove one of the multiple timers knowing its ID.  Returns a boolean value indicating success. */
extern  int cmn_remove_timer(void *_tid);

extern	int cmn_timer_init(void);

extern	void cmn_timer_quit(void);

extern	void cmn_cancel_timers(cmn_timer_mode_t mode);

#endif

