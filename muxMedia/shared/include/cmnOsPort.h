/*
* $Id: cmn_port.h,v 1.7 2007/04/07 12:01:19 lizhijie Exp $
*/

#ifndef __CMN_OS_PORT_H__
#define __CMN_OS_PORT_H__

#include <stdio.h>
#include <signal.h>

#define	HAVE_CMN_CONFIG_H	1

#if defined (HAVE_CMN_CONFIG_H)
	#include <cmnConfig.h>
	#if defined (HAVE_STRING_H)
		#include <string.h>
	#else
		#include <strings.h>
	#endif /* HAVE_STRING_H */
#else
	#include <string.h>
#endif /* !HAVE_CONFIG_H */

#if defined (HAVE_SYS_TYPES_H)
	#include <sys/types.h>
#endif

#if STDC_HEADERS
	#include <stdlib.h>
#endif /* !STDC_HEADERS */

#if defined(HAVE_STDARG_H) || defined(WIN32)
	#include <stdarg.h>
	#define VA_START(a, f)  va_start(a, f)
#else
	#if defined(HAVE_VARARGS_H)
		#include <varargs.h>
		#define VA_START(a, f) va_start(a)
	#else
		#include <stdarg.h>
		#define VA_START(a, f)  va_start(a, f)
		#endif
#endif

#ifdef HAVE_TIME_H
	#include <time.h>
#endif

#if 1//defined (HAVE_SYS_TIME_H)
	#include <sys/time.h>
#endif
#include <sys/time.h>

#ifdef __PSOS__
#define VA_START(a, f)  va_start(a, f)
#endif

#if __STDC__
#  ifndef NOPROTOS
#    define PARAMS(args)   args
#  endif
#endif

#ifndef PARAMS
#  define PARAMS(args)     ()
#endif

#include <internal.h>
#include <cmnMutex.h>
#include <cmnList.h>
#include <cmnFifo.h>
#include <cmnTimer.h>
#include <cmnThread.h>

#ifdef __cplusplus
extern "C"
{
#endif

void *cmn_malloc(size_t size);
void *cmn_realloc(void *, size_t size);
void cmn_free(void *);


void add_gettimeofday (struct timeval *atv, int ms);
void min_timercmp (struct timeval *tv1, struct timeval *tv2);

/**************************/
/* TIMER support          */
/**************************/

#define SP   " \0"

void cmn_usleep (int useconds);

#ifdef __cplusplus
}
#endif

#endif

