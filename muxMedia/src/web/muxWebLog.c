/* 
 *
 */

#include "muxWeb.h"

FILE *cgiLogFile = NULL;
FILE *accessLogFile = NULL;
FILE *errorLogFile = NULL;


/*
 * Description: Returns the current time in common log format in a static char buffer.
 * commonlog time is exactly 25 characters long
 * because this is only used in logging, we add " [" before and "] " after making 29 characters
 * "[27/Feb/1998:20:20:04 +0000] "
 *
 * Contrast with rfc822 time:
 * "Sun, 06 Nov 1994 08:49:37 GMT"
 * Altered 10 Jan 2000 by Jon Nelson ala Drew Streib for non UTC logging
 */
/* Don't need or want the trailing nul for these character arrays */
const char month_tab[48] =
    "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec ";


char *get_commonlog_time(SVR_WEB  *webSvr)
{
	static char buf[30];
	struct tm *t;
	char *p;
	unsigned int a;
	int time_offset;

	if (webSvr->cfg.use_localtime)
	{
		t = localtime(&webSvr->current_time);
#ifdef	WIN32
		/*_timezone and _tzname[2] are global variables defined in time.h in VC */
		time_offset = _timezone;
//		ECPWS_LOG_INFO("Timezone-0:%s; Timezone-1:%s\n", _tzname[0], _tzname[1]);
#else
		time_offset = TIMEZONE_OFFSET(t);
#endif
	}
	else
	{
		GM_TIME( t, &webSvr->current_time);
		time_offset = 0;
	}

	p = buf + 29;
	*p-- = '\0';
	*p-- = ' ';
	*p-- = ']';
	a = abs(time_offset / 60);
	*p-- = '0' + a % 10;
	a /= 10;
	*p-- = '0' + a % 6;
	a /= 6;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = (time_offset >= 0) ? '+' : '-';
	*p-- = ' ';

	a = t->tm_sec;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = t->tm_min;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = t->tm_hour;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = 1900 + t->tm_year;
	
	while (a) {
		*p-- = '0' + a % 10;
		a /= 10;
	}
	
	/* p points to an unused spot */
	*p-- = '/';
	p -= 2;
	memcpy(p--, month_tab + 4 * (t->tm_mon), 3);
	*p-- = '/';
	a = t->tm_mday;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p = '[';
	
	return p;                   /* should be same as returning buf */
}

static FILE *_openFd(char *spec)
{
	FILE *file;
	
	file = fopen(spec, "w+");
	if( file == NULL)
	{
		ECPWS_FATAL("Open file \"%s\"", spec);
	}

	return file;
}

int _openGlobalFile(SVR_WEB  *webSvr)
{
	/* if error_log_name is set, dup2 stderr to it */
	/* otherwise, leave stderr alone */
	/* we don't want to tie stderr to /dev/null */
	if (webSvr->cfg.error_log_name) 
	{
		errorLogFile = _openFd( webSvr->cfg.error_log_name);
	}

	/**/
	if (webSvr->cfg.access_log_name)
	{
		accessLogFile = _openFd(webSvr->cfg.access_log_name);
	} 

	if ( webSvr->cfg.cgi_log_name) 
	{
		cgiLogFile = _openFd( webSvr->cfg.cgi_log_name);
	}

	return 0;
}

#ifdef	WIN32


/*
 * Description: Opens access log, error log, and if specified, cgi log
 * Ties stderr to error log, except during cgi execution, at which
 * time cgi log is the stderr for cgis.
 * Access log is line buffered, error log is not buffered.
 */
void webSvrOpenLogs(SVR_WEB *webSvr)
{
	_openGlobalFile(webSvr);
	
	if( setvbuf(errorLogFile, (char *) NULL, _IONBF, 0) !=0 )
	{/* _IONBF: no buffer */
		ECPWS_FATAL("Set Error buffer failed!");
	}

	if( setvbuf(accessLogFile, (char *) NULL, _IONBF, 0) !=0 )
	{/* _IONBF: no buffer */
		ECPWS_FATAL("Set Access buffer failed!");
	}
	
	if( setvbuf(cgiLogFile, (char *) NULL, _IONBF, 0) !=0 )
	{/* _IONBF: no buffer */
		ECPWS_FATAL("Set CGI buffer failed!");
	}

}
#else
static int _cgi_log_fd;

int open_gen_fd(char *spec)
{
	int fd;
#if 0	
	if (*spec == '|')
	{
		fd = open_pipe_fd(spec + 1);
	}
	else if (*spec == ':')
	{
		fd = open_net_fd(spec + 1);
	}
	else
#endif		
	{
		fd = open(spec, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP);
	}
	return fd;
}

/*
 * Name: open_logs
 *
 * Description: Opens access log, error log, and if specified, cgi log
 * Ties stderr to error log, except during cgi execution, at which
 * time cgi log is the stderr for cgis.
 *
 * Access log is line buffered, error log is not buffered.
 *
 */

void webSvrOpenLogs(SVR_WEB *webSvr)
{
	CmnMuxWebConfig	*cfg = &webSvr->cfg;
	int access_log;

	/* if error_log_name is set, dup2 stderr to it */
	/* otherwise, leave stderr alone */
	/* we don't want to tie stderr to /dev/null */
	if (cfg->error_log_name && (webSvr->cfg.debug==0) ) 
	{
		int error_log;

		/* open the log file */
		if ( (error_log = open_gen_fd(cfg->error_log_name)) == -1) 
		{
			DIE("unable to open error log");
		}

#ifdef	PLUGIN_SUPPORT
#else
		/* redirect stderr to error_log */
		if (dup2(error_log, STDERR_FILENO) == -1) 
		{
			DIE("unable to dup2 the error log");
		}

#endif		
		close(error_log);
	}

	if (cfg->access_log_name)
	{
		access_log = open_gen_fd(cfg->access_log_name);
	} 
	else 
	{
		access_log = open("/dev/null", 0);
	}
	
	/*	
	access_log = open("/dev/null", 0);
	*/
	if (access_log < 0) 
	{
		DIE("unable to open access log");
	}

#if 0
	if (dup2(access_log, STDOUT_FILENO) == -1) 
	{
		DIE("can't dup2 /dev/null to STDOUT_FILENO");
	}
#endif
	close(access_log);

	if (cfg->cgi_log_name) 
	{
		_cgi_log_fd = open_gen_fd(cfg->cgi_log_name);
		if (_cgi_log_fd == -1)
		{
			WARN("open cgi_log");
			//free(cfg->cgi_log_name);
			//cfg->cgi_log_name = NULL;
			_cgi_log_fd = 0;
		}
		else
		{
			if (fcntl(_cgi_log_fd, F_SETFD, 1) == -1)
			{
				WARN("unable to set close-on-exec flag for cgi_log");
				//free(cfg->cgi_log_name);
				//cfg->cgi_log_name = NULL;
				close(_cgi_log_fd);
				_cgi_log_fd = 0;
			}
		}
	}
	
	setvbuf(stderr, (char *) NULL, _IONBF, 0);
	setvbuf(stdout, (char *) NULL, _IOLBF, 0);

	_openGlobalFile(webSvr);

}
#endif

/*
 * Writes log data to access_log when this request is freed.
 */

/* NOTES on the commonlog format:
 * Taken from notes on the NetBuddy program
 *  http://www.computer-dynamics.com/commonlog.html

 remotehost

 remotehost rfc931 authuser [date] "request" status bytes

 remotehost - IP of the client
 rfc931 - remote name of the user (always '-')
 authuser - username entered for authentication - almost always '-'
 [date] - the date in [08/Nov/1997:01:05:03 -0600] (with brackets) format
 "request" - literal request from the client (boa may clean this up,
   replacing control-characters with '_' perhaps - NOTE: not done)
 status - http status code
 bytes - number of bytes transferred

 boa appends:
   referer
   user-agent

 and may prepend (depending on configuration):
 virtualhost - the name or IP (depending on whether name-based
   virtualhosting is enabled) of the host the client accessed
*/
void log_access(WEB_CLIENT_CONN * conn)
{
	CmnMuxWebConfig	*cfg = &conn->webSvr->cfg;
	
	if (!cfg->access_log_name)
		return;
#if 0
	ECPWS_LOG_INFO("RELEASED REQUEST : %s - - %s\"%s\" %d %ld \"%s\" \"%s\"\n",
		conn->remoteIpAddr,get_commonlog_time(conn->webSvr), conn->logline,
		conn->response_status, conn->bytes_written, (conn->header_referer ? conn->header_referer : "-"), 
		(conn->header_user_agent ? conn->header_user_agent : "-"));
#else
	fprintf(accessLogFile, "RELEASED REQUEST : %s - - %s\"%s\" %d %ld \"%s\" \"%s\"\n",
		conn->remoteIpAddr,get_commonlog_time(conn->webSvr), conn->logline,
		conn->response_status, conn->bytes_written, (conn->header_referer ? conn->header_referer : "-"), 
		(conn->header_user_agent ? conn->header_user_agent : "-"));
	fflush(accessLogFile);
#endif
	
#if 0
	if (cfg->virtualhost) {
		ECPWS_LOG_INFO("%s ", conn->localIpAddr);
	}
	else if (cfg->vhost_root)
	{
		ECPWS_LOG_INFO("%s ", (conn->host ? conn->host : "(null)"));
	}
#endif
}

/* export this function in order to avoid the problem of 'R6200, Float point support is not loaded'*/
void ecpwsOutputMsg(char *msg)
{
	if( _webSvr.status < WEB_STATUS_INIT_SOCKET || _webSvr.cfg.debug )
		fprintf(stderr, msg );
	else
	{
		fprintf(errorLogFile, msg, strlen(msg));
		fflush(errorLogFile);
	}
}

/*
 * Description: Logs the current time and transaction identification to the stderr (the error log):
 * Example output:
 [08/Nov/1997:01:05:03 -0600] request from 192.228.331.232 "GET /~joeblow/dir/ HTTP/1.0" ("/usr/user1/joeblow/public_html/dir/"): write: Broken pipe

 Apache uses:
 [Wed Oct 11 14:32:52 2000] [error] [client 127.0.0.1] client denied by server configuration: /export/home/live/ap/htdocs/test
 */
static void _ecpwsLogErrorReq(WEB_CLIENT_CONN * conn)
{
	static char output[1024];
	CmnMuxWebConfig	*cfg = &_webSvr.cfg;

	if (cfg->virtualhost)
	{
		SPRINTF(output,sizeof(output), "%s ", conn->localIpAddr);
	}
	else if (cfg->vhost_root)
	{
		SPRINTF(output, sizeof(output), "%s ", (conn->host ? conn->host : "host(null)"));
	}
	
	if (cfg->vhost_root)
	{
		SPRINTF(output+strlen(output), sizeof(output)-strlen(output), "%s - - %srequest [%s] \"%s\" (\"%s\"): ",
			conn->remoteIpAddr, get_commonlog_time(conn->webSvr),  (conn->header_host ? conn->header_host : "header_host(null)"),
			(conn->logline ? conn->logline : "logline(null)"),
			(conn->pathname ? conn->pathname : "pathname(null)"));
	}
	else
	{
		SPRINTF(output+strlen(output), sizeof(output)-strlen(output), "%s - - %srequest \"%s\" (\"%s\"): ",
		conn->remoteIpAddr,  get_commonlog_time(conn->webSvr), (conn->logline ? conn->logline : "logline(null)"),
		(conn->pathname ? conn->pathname : "pathname(null)"));
	}

	ecpwsOutputMsg(output);
}


static char *_ecpwsGetSystemErrorMsg(void )
{
	static char output[1024];
#ifdef	WIN32	
	char	tmp[512];
	int val;

	val= GetLastError();
	memset(tmp, 0, sizeof(tmp));
	memset(output, 0, sizeof(output));
	
//	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, val, LANG_USER_DEFAULT, tmp, sizeof(tmp) - 1, NULL);
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
				  FORMAT_MESSAGE_MAX_WIDTH_MASK,
				  NULL, val, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				  (LPTSTR) tmp, sizeof(tmp) - 1, NULL);
	SNPRINTF(output, sizeof(output), ("failed with error %d: %s\n"), val, tmp);
#else
	SNPRINTF(output, sizeof(output),  strerror( errno) );
#endif

	return output;
}

void ecpwsLog(WEB_CLIENT_CONN *conn,BOOL_T withReqDebug, BOOL_T isSysErr, BOOL_T isQuit, const char *format,...)
{
	static char debugStr[SOCKETBUF_SIZE];
#if 1
	va_list marker;
	va_start( marker, format );

	/* Initialize variable arguments. */
	memset(debugStr, 0, sizeof(debugStr));

	/* vsprintf : param of va_list; sprintf : param of varied params such as 'format ...' */
	vsprintf(debugStr, format, marker);
	va_end( marker);
#else
	SNPRINTF(debugStr, sizeof(debugStr), format, __VA_ARGS__ );
#endif

	if(IS_TRUE(withReqDebug) )
	{
		_ecpwsLogErrorReq(conn);
	}
	
	if(IS_TRUE(isSysErr) )
	{
		SPRINTF(debugStr+strlen(debugStr), sizeof(debugStr)-strlen(debugStr), ":%s\n", _ecpwsGetSystemErrorMsg() );
	}

	ecpwsOutputMsg(get_commonlog_time(&_webSvr));

	ecpwsOutputMsg(debugStr);

	if(IS_TRUE(isQuit) )
	{
		exit(1);
	}

}


