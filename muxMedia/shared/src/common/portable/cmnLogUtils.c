/*
 * $Author: lizhijie $
 * $Id: cmn_utils.c,v 1.14 2007/03/25 08:32:47 lizhijie Exp $
*/
#include <stdio.h> 
#include <getopt.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/time.h>
#include <time.h>

#include <assert.h>

#include "libCmn.h"

static const char* priname[] =
{
      "EMRG",
      "ALRT",
      "CRIT",
      "ERR ",
      "WARN",
      "NOTC",
      "INFO",
      "DBUG",
      0
};

static log_stru_t logobj = 
{
	/* in order to prompt by logobj when it is used by standalone program such as test */
#ifndef   __CMN_RELEASE__
	lstyle		:	USE_CONSOLE,
	llevel		: 	CMN_LOG_DEBUG,
	lfacility		:	CMN_LOG_DEBUG,
#else
	lstyle		:	USE_SYSLOG,
	llevel		: 	CMN_LOG_WARNING,
	lfacility		:	CMN_LOG_UNKNOW,
#endif
	isDaemonized	:	0,
	fp			:	NULL
	
};

void cmn_daemon_init()
{
	pid_t pid;
	int fd;

	//printf("Daemonizing .....\n\n");
	if((pid = fork()) != 0)
		exit(0);
	
	setsid();	 //become session leader 
//	signal(SIGHUP,SIG_IGN);
	if((pid = fork()) != 0)
		exit(0); 
	
	chdir("/");
	umask(0);
	
	close(0);
	close(1);
	close(2);
	
	fd=open("/dev/null",O_RDWR);
	dup(0);
	dup(1);
	dup(2);

	close(fd);
}

int sys_log_init(log_facility_t facility)
{   
	log_facility_t temp = facility >> 3;
	if (temp >= 16 && temp <= 23)
		logobj.lfacility = facility;
	else
		return -1; // facility not exist

	openlog("MuxLab",  LOG_CONS, logobj.lfacility);

	return 0; //success
}

log_level_t get_current_level()
{
	return logobj.llevel;
}

int cmn_log_init(log_stru_t *lobj)
{
	int	isStdOut = 0;
	
	if (lobj->llevel < CMN_LOG_EMERG)
		;
	else if (lobj->llevel > CMN_LOG_DEBUG) 
		logobj.llevel= CMN_LOG_DEBUG;
	else 
		logobj.llevel = lobj->llevel;

	sprintf(logobj.name, "%s", lobj->name);
	logobj.lstyle = lobj->lstyle;
	logobj.isDaemonized = lobj->isDaemonized;

	logobj.offset = 0;
	if(lobj->maxSize < 0)
	{
		logobj.maxSize = UNIT_OF_KILO*256; 	/* default is 256K bytes */
	}
	else
	{
		logobj.maxSize = lobj->maxSize;
	}
	
	if (logobj.lstyle == USE_SYSLOG)
	{
#ifndef   __CMN_RELEASE__
		fprintf(stderr, "Use SysLog\n");
#endif
		return sys_log_init(lobj->lfacility);
	}
	else if(logobj.lstyle == USE_FILE)
	{
#ifndef   __CMN_RELEASE__
		if(! logobj.isDaemonized)
		{
			logobj.fp = stderr;
			isStdOut = 1;
		}
		else
		{
			logobj.fp = fopen( lobj->logFileName, "w+");
		}
#else
		logobj.fp = fopen( lobj->logFileName, "w+");
#endif
		
		if(!logobj.fp)
		{
			fprintf(stderr, "Log File '%s' initialized fail. %s\n\n",lobj->logFileName, strerror(errno) );
			logobj.fp = stderr;
			isStdOut = 1;
		}
#ifndef   __CMN_RELEASE__
		fprintf(stderr, "Use File '%s' as Log\n", lobj->logFileName);
#endif
	}
	else
	{
#ifndef   __CMN_RELEASE__
		fprintf(stderr, "Use stderr as Log\n");
#endif
		logobj.fp = stderr;
		isStdOut = 1;
	}
	
	if(!isStdOut && logobj.isDaemonized )
	{
		cmn_daemon_init();
	}

	{
		struct timeval tv;
		struct tm *ptm;

		char date[32] = {0};
		gettimeofday(&tv, NULL);
		ptm = localtime(&tv.tv_sec);
		strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", ptm);

#ifndef   __CMN_RELEASE__
		fprintf(stderr, "Startup at %s\n", date);
#endif
		//CMN_MSG_LOG(CMN_LOG_NOTICE, "%s startup at %s, Log Level is %s",logobj.name, date, priname[logobj.llevel] );
	}

	return 0;
}


int64_t cmnGetTimeFromStartup(void)
{
	struct timeval 	tv;
	static int64_t 	_timeOffset;
	static int		_initTimeOffset = 0;

	gettimeofday(&tv,NULL);
	if(_initTimeOffset == 0)
	{
		_timeOffset = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
		_initTimeOffset = 1;
		return (int64_t) 0;
	}

	return ((int64_t)tv.tv_sec * 1000000 + tv.tv_usec)-_timeOffset;
}

char *colorEscapes[] =
{
	ERROR_TEXT_BEGIN,	/* CMN_LOG_EMERG */
	ERROR_TEXT_BEGIN,	/* CMN_LOG_ALERT */
	ERROR_TEXT_BEGIN,	/* CMN_LOG_CRIT */
	ERROR_TEXT_BEGIN,	/* CMN_LOG_ERR */
	WARN_TEXT_BEGIN,	/* CMN_LOG_WARNING */
	INFO_TEXT_BEGIN,		/* CMN_LOG_NOTICE */
	INFO_TEXT_BEGIN,		/* CMN_LOG_INFO */
	""					/* CMN_LOG_DEBUG */
};


#ifndef   __CMN_RELEASE__
void log_information(int pri, const char* file, int line, const char* frmt,...)
#else
void log_information(int pri, const char* frmt,...)
#endif
{
#ifndef   __CMN_RELEASE__
	struct timeval tv;
	struct tm *ptm;

	char date[32] = {0};
	char buf[1024*10] = {0};
#else	
	char buf[1024] = {0};
#endif	
	int pri_;
	int ret = 0;
	char *_colorEsc = (pri>= CMN_LOG_EMERG && pri<= CMN_LOG_DEBUG )?colorEscapes[pri]:"";

//	return;

	va_list ap;
	va_start(ap, frmt);
	vsnprintf(buf, sizeof(buf), frmt, ap);
	va_end(ap);

//	if(logobj.fp == NULL)
//		logobj.fp = stderr;
	
#ifndef   __CMN_RELEASE__
	gettimeofday(&tv, NULL);
	ptm = localtime(&tv.tv_sec);
	strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", ptm);
#endif	
	pri_ = (pri <= CMN_LOG_DEBUG) ? pri : CMN_LOG_DEBUG;
	
	if (logobj.lstyle == USE_SYSLOG)
	{
#ifndef   __CMN_RELEASE__
		syslog(pri_, "%s [%s] : %s:%d | %s\n", date, priname[pri_], file, line,buf);
#else
		syslog(pri_, "[%s] : %s\n", priname[pri_], buf);
#endif
	}
	else
	{
#ifndef   __CMN_RELEASE__
//		fprintf(stderr, "[%s] : %s:%d | %s \n", priname[pri_], file, line, buf);
		if(logobj.fp )
		{
			ret = fprintf(logobj.fp, "%s%s [%s,%s] : %s:%d | %s\n"ERROR_TEXT_END, _colorEsc, date, priname[pri_], cmnThreadGetName(), file, line,buf);
//			assert(0);
		}
		else
		{
		
#if 1	
			/* it makes VID failed in HiPlayer  */
			ret = fprintf(stderr, "%s%s [%s,%s] : %s:%d | %s\n"ERROR_TEXT_END, _colorEsc, date, priname[pri_], cmnThreadGetName(), file, line,buf);
#else
			ret = printf( "%s [%s] : %s:%d | %s\n", date, priname[pri_], file, line,buf);
#endif
		}
#else

		ret = fprintf(logobj.fp, "%s[%s][%s,%s]: %s\n"ERROR_TEXT_END, _colorEsc, sysTimestamp(), priname[pri_], cmnThreadGetName(), buf);
#endif
		if(logobj.fp != NULL)
			fflush(logobj.fp);	/* added this line, lizhijie, 2007.03.16 */
	}

	if(ret > 0 && logobj.lstyle != USE_SYSLOG)
	{
		logobj.offset += ret;
		if(logobj.offset > logobj.maxSize && logobj.fp)
		{
			fseek(logobj.fp, 0L, SEEK_SET);
			logobj.offset = 0;
		}
	}
	
}

int safe_open (const char *pathname,int flags)
{
   	int fd;

   	fd = open (pathname,flags);
   	if (fd < 0)
	{
		MUX_ERROR("While trying to open %s",pathname);
		if (flags & O_RDWR)
		{
			MUX_ERROR(" for read/write access");
		}
		else if (flags & O_RDONLY)
		{
			MUX_ERROR(" for read access");
		}
		else if (flags & O_WRONLY)
		{
			MUX_ERROR(" for write access");
		}
		
		MUX_ERROR(": %m\n");
		exit (EXIT_FAILURE);
	}
	return (fd);
}

void safe_rewind (int fd, int offset, const char *filename)
{
   	if (lseek(fd, offset,SEEK_SET) < 0)
	{
		MUX_ERROR( "While seeking to start of %s: %m",filename);
		exit (EXIT_FAILURE);
	}
}

void safe_read (int fd,const char *filename,void *buf,size_t count, int verbose)
{
   	ssize_t result;

   	result = read (fd,buf,count);
   	if (count != result)
	{
		if (verbose)
		{
			MUX_DEBUG("\n");
		}
		
		if (result < 0)
		{
			MUX_ERROR("While reading data from %s: %m",filename);
			exit (EXIT_FAILURE);
		}
		
		MUX_ERROR( "Short read count returned while reading from %s",filename);
		exit (EXIT_FAILURE);
	}
}


char *cmn_read_file(const char *filename)
{
	FILE *file = NULL;
	long length = 0;
	char *content = NULL;
	size_t read_chars = 0;

	/* open in read binary mode */
	file = fopen(filename, "rb");
	if (file == NULL)
	{
		MUX_ERROR("fopen file '%s' failed : %s",filename, strerror(errno));
		goto cleanup;
	}

	/* get the length */
	if (fseek(file, 0, SEEK_END) != 0)
	{
		MUX_ERROR("fseek file '%s' failed : %s",filename, strerror(errno));
		goto cleanup;
	}
	
	length = ftell(file);
	if (length < 0)
	{
		MUX_ERROR("ftell file '%s' failed : %s",filename, strerror(errno));
		goto cleanup;
	}
	if (fseek(file, 0, SEEK_SET) != 0)
	{
		MUX_ERROR("fseek file '%s' failed : %s",filename, strerror(errno));
		goto cleanup;
	}

	/* allocate content buffer */
	content = (char*)malloc((size_t)length + sizeof(""));
	if (content == NULL)
	{
		goto cleanup;
	}

	/* read the file into memory */
	read_chars = fread(content, sizeof(char), (size_t)length, file);
	if ((long)read_chars != length)
	{
		MUX_ERROR("fread file '%s' failed : %s",filename, strerror(errno));
		free(content);
		content = NULL;
		goto cleanup;
	}
	content[read_chars] = '\0';

cleanup:
	if (file != NULL)
	{
		fclose(file);
	}

	return content;
}


