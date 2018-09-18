/*
* $Id: cmn_utils.h,v 1.8 2007/03/25 08:32:47 lizhijie Exp $
*/

#ifndef  __CMN_LOG_UTILS_H__
#define  __CMN_LOG_UTILS_H__

#include <stdio.h>

#define CMN_LOG_0      (16<<3) /* facility not used */
#define CMN_LOG_1      (17<<3) /* facility not used */
#define CMN_LOG_2      (18<<3) /* facility not used*/
#define CMN_LOG_3      (19<<3) /* facility not used */
#define CMN_LOG_4      (20<<3) /* facility for cdr */
#define CMN_LOG_5      (21<<3) /* facility for sipserver */
#define CMN_LOG_6      (22<<3) /* facility for cgi */
#define CMN_LOG_7      (23<<3) /* facility for pbx */

#define CMN_LOG_UNKNOW   0   /* facility for unknow */

typedef unsigned int log_facility_t;

typedef enum
{
	USE_SYSLOG = 0,		/* log messages to syslog */
	USE_CONSOLE ,		/* log messages to console */
	USE_FILE			/* log messages into a normal disk file */

}log_style_t;

//#ifdef LOG_EMERG
#if 1
	typedef enum
	{
		CMN_LOG_EMERG = 0,		/* system is unusable */
		CMN_LOG_ALERT,			/* action must be taken immediately */
		CMN_LOG_CRIT ,			/* critical conditions */
		CMN_LOG_ERR , 			/* error conditions */
		CMN_LOG_WARNING ,		/* warning conditions */
		CMN_LOG_NOTICE ,		/* normal but significant condition */
		CMN_LOG_INFO,			/* informational */
		CMN_LOG_DEBUG			/* 7 : debug-level messages */
	}log_level_t;
#else
	typedef enum
	{
		LOG_EMERG = 0,       /* system is unusable */
		LOG_ALERT,       	     /* action must be taken immediately */
		LOG_CRIT ,              /* critical conditions */
		LOG_ERR ,       /* error conditions */
		LOG_WARNING , /* warning conditions */
		LOG_NOTICE ,     /* normal but significant condition */
		LOG_INFO ,   /* informational */
		LOG_DEBUG      /* debug-level messages */
		
	}log_level_t;

#endif

#define	CMN_PROGRAM_NAME_LENGTH			64
struct log_object
{
	char			name[CMN_PROGRAM_NAME_LENGTH];
	log_style_t 	lstyle;
	log_level_t 	llevel;
	log_facility_t 	lfacility;
	int			isDaemonized;		/* 1 : daemonized; 0 : front-end */

	/* following 2 fields are added by lizhijie, 2008.08.07 */
	int			maxSize;			/* in byte */
	int			offset;				/* current offset of write */
	
	char			logFileName[256];

	FILE			*fp;
};

typedef struct log_object log_stru_t;

#ifndef   __CMN_RELEASE__
void log_information(int pri, const char* file, int line, const char* frmt,...);
#else
void log_information(int pri, const char* frmt,...);
#endif

log_level_t get_current_level();

char *cmn_read_file(const char *filename);

void cmnThreadMask(char *threadName);


#endif

