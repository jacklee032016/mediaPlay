/*
* $Id: internal.h,v 1.2 2007/03/07 16:18:07 lizhijie Exp $
*/


#ifndef __INTERNAL_H__
#define __INTERNAL_H__

/*
 * Structure for payload management. Each payload element represents one codec of a media line.
 * @var __payload_t
 */
typedef struct __payload __payload_t;

struct __payload
{
	char *payload;
	/*  char *port; this must be assigned by the application dynamicly */
	char *number_of_port;
	char *proto;
	char *c_nettype;
	char *c_addrtype;
	char *c_addr;
	char *c_addr_multicast_ttl;
	char *c_addr_multicast_int;
	/* rtpmap (rcvonly and other attributes are added dynamicly) */
	char *a_rtpmap;
};


/**
 * Allocate a payload element.
 * @param payload The payload.
 */
int __payload_init (__payload_t ** payload);
/**
 * Free a payload element.
 * @param payload The payload.
 */
void __payload_free (__payload_t * payload);

typedef pthread_mutex_t cmn_mutex_t;
//typedef pthread_t cmn_thread_t;


/* Semaphore and Mutex abstraction layer definition */

#if defined(HAVE_SEMAPHORE_H) //&& !defined(__APPLE_CC__)
#include <semaphore.h>

/**
 * Structure for referencing a semaphore element.
 * @var cmn_sem_t
 */
typedef sem_t cmn_sem_t;

#elif defined(HAVE_SYS_SEM_H)
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
typedef struct
{
	int					semid;
}cmn_sem_t;
#endif


typedef struct cmn_cond
{
	pthread_cond_t		cv;
}cmn_cond_t;

#include "libCmn.h"

#endif

