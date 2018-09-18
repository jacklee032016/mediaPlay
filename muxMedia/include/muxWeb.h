/*
 *
 */

#ifndef __MUX_WEB_H__
#define __MUX_WEB_H__

#include <errno.h>
#include <stdlib.h>		/* malloc, free, etc. */
#include <stdio.h>			/* stdin, stdout, stderr */
#include <string.h>		/* strdup */

#include <sys/types.h>		/* socket, bind, accept */
#include <sys/stat.h>		/* open */
#include <time.h>			/* localtime, time */

#include <signal.h>		/* signal */
#include <setjmp.h>
#include <limits.h>             /* OPEN_MAX */

#include <stddef.h>             /* for offsetof */
#if WIN32
//#include <windows.h>
//#include <Winsock2.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>

#include <io.h>			/* _dup2  _chmod*/

#else
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>

#include <netinet/in.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>         /* socket, bind, accept, setsockopt, */
#include <sys/wait.h>
#include <sys/stat.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#endif
#include <fcntl.h>
#include <stdarg.h>

#include "dirent.h"

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#define	WEB_OPTIONS_DEBUG_FSM			0

/* debug data input/output */
#define	WEB_OPTIONS_DEBUG_DATA			0	

#define	WEB_OPTIONS_DEBUG_CGI			0	

#define	WEB_OPTIONS_DEBUG_ALIAS			0	

#define	WITH_JUMP		0

#ifdef _WIN32
#include <direct.h>
/*
* __declspec(dllexport) must be before return type of C function, 
and it must be after the class keyword in C++ class
*/
#ifdef DLL_LIBRARY_EXPORT
#define	DLL_DECLARE	__declspec(dllexport) //	__stdcall
#else
#define	DLL_DECLARE	__declspec(dllimport) //	__stdcall
#endif

#ifdef __cplusplus
	/* export the names in C++ as C */
	#define STD_CAPI	extern "C" 
#else
	#define STD_CAPI
#endif

#define	UID_T			unsigned int
#define	GID_T			unsigned int
#define	GETUID()		0
#define	GETPID			_getpid

#define	STDERR_FILENO 		_fileno( stderr )
#define	STDOUT_FILENO 		_fileno( stdout )
#define	STDIN_FILENO 		_fileno( stdin )

#define	JMP_BUFFER		jmp_buf

#define	DUP2			_dup2
#define	PIPE(fdSet)		_pipe(fdSet, 512, O_NOINHERIT)

#define	UNLINK			_unlink
#define	CHDIR			_chdir
#define 	CHMOD			_chmod
#define	UMASK			_umask		
#define	OPEN			_open
#define	CLOSE			_close
#define	LSEEK			_lseek
#define	READ			_read
#define	WRITE			_write

//#define	SSCANF			sscanf_s
#define	SSCANF			sscanf

#if 0
#define	GM_TIME(tm, time)		\
		{if(gmtime_s((tm), (time)) ) {ECPWS_ERR_INFO("gmtime_s"); /*exit(1);*/}}
#endif

#define	GM_TIME(tm, time)		{(tm) = gmtime((time));}

#define	F_OPEN(file, name, flags)		\
		{if(fopen_s((file), (name), (flags)) ) {ECPWS_ERR_INFO("fopen_s"); exit(1);}}

#define	FILENO			_fileno
#define	STRNCASECMP	_strnicmp
#define	STRCASECMP		_stricmp

#define	SPRINTF(dest, size, ...)		sprintf_s((dest), (size), ##__VA_ARGS__)

#define	SNPRINTF(dest, size, ...)		_snprintf_s(dest, size,  _TRUNCATE, ##__VA_ARGS__)
#define	STRCPY(dest, size, src)		strcpy_s((dest), (size), (src) )

#define	STRDUP			_strdup

#define	S_ISDIR(mode)	(mode & _S_IFDIR )
#define	S_IS_FILE(mode)	(mode & _S_IFREG)
#define	S_ISREG(mode)		S_IS_FILE(mode)
#define	CloseSocket(s)		closesocket(s)
#define	SEND(socket, buf, size)	send(socket, buf, size, 0)
#define	RECV(socket, buf, size)	recv(socket, buf, size, 0)

#else	/* else WIN32 */
#define	DLL_DECLARE

#define	UID_T			uid_t
#define	GID_T			gid_t
#define	GETUID()		getuid()
#define	GETPID			getpid

#define	JMP_BUFFER		sigjmp_buf /* ?? should be jmp_buf */

#define	DUP2			dup2
#define	PIPE(fdSet)		pipe(fdSet)

#define	UNLINK			unlink
#define	CHDIR			chdir
#define 	CHMOD			chmod
#define	UMASK			umask		
#define	OPEN			open
#define	CLOSE			close
#define	LSEEK			lseek
#define	READ			read
#define	WRITE			write

#define	SSCANF			sscanf

#define	GM_TIME(tm, time)		{(tm) = gmtime((time));}

#define	FILENO			fileno
#define	STRNCASECMP	strncasecmp
#define	STRCASECMP		strcasecmp

#define	SPRINTF(dest, size, ...)		sprintf((dest),##__VA_ARGS__)

#define	SNPRINTF(dest, size,...)		snprintf((dest), (size), ##__VA_ARGS__)
#define	STRCPY(dest, size, src)		strcpy((dest), (src) )

#define	STRDUP			strdup

 /* regular file | u+rx | g+rx */
#define	S_IS_FILE(mode)		(mode & (S_IFREG |(S_IRUSR | S_IXUSR) | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH)) )

#define	CloseSocket(s)	close(s)
#define	SEND(socket, buf, size)	write(socket, buf, size)
#define	RECV(socket, buf, size)	read(socket, buf, size)
#define	INVALID_SOCKET			-1
#define	SOCKET_ERROR			-1

/* Wild guess time, probably better done with configure */
#ifdef O_NONBLOCK
#define NOBLOCK		O_NONBLOCK      /* Linux */
#else                           /* O_NONBLOCK */
#ifdef O_NDELAY
#define NOBLOCK		O_NDELAY        /* Sun */
#else                           /* O_NDELAY */
#error "Can't find a way to #define NOBLOCK"
#endif                          /* O_NDELAY */
#endif                          /* O_NONBLOCK */

#ifndef MAP_FILE
#define MAP_OPTIONS	MAP_PRIVATE /* Sun */
#else
#define MAP_OPTIONS	MAP_FILE|MAP_PRIVATE /* Linux */
#endif

#if 1//def HAVE_TM_GMTOFF
#define TIMEZONE_OFFSET(foo)	foo->tm_gmtoff
#else
#define TIMEZONE_OFFSET(foo)	timezone
#endif

#if __GNUC__
#if __x86_64__ || __ppc64__
#define	ENV_64
#define	POINTRT_TO_INTEGER	long int
#else
#define	ENV_32
#define	POINTRT_TO_INTEGER	int
#endif
#endif



#endif


#ifdef INET6
    typedef struct sockaddr_in6 SOCKADDR_IN_T;
    #define AF_INET_V    AF_INET6
#else
    typedef struct sockaddr_in  SOCKADDR_IN_T;
    #define AF_INET_V    AF_INET
#endif

#ifdef INET6
/* #define S_FAMILY __s_family */
#define SOCKADDR sockaddr_storage
#define SERVER_PF PF_INET6
#define S_FAMILY sin6_family
#define BOA_NI_MAXHOST NI_MAXHOST
#else /* ifdef INET6 */
#define SOCKADDR sockaddr_in
#define SERVER_PF PF_INET
#define S_FAMILY sin_family
#endif /* ifdef INET6 */

/* options for features in building */
#define	WEB_DEBUG_ENABLE			1
#define	TUNE_SNDBUF


#define	SERVER_ROOT			"/etc/ecpws"

#define	DEFAULT_HOME_DIR		HOME_DIR



/***** Change this via the CGIPath configuration value in boa.conf *****/
#ifdef	WIN32
#define DEFAULT_PATH     HOME_DIR"\\WIN7_DEBUG:d:\\works"
#else
#define DEFAULT_PATH     "/bin:/usr/bin:/usr/local/bin"
#endif

/***** Change this via the DefaultVHost configuration directive in boa.conf *****/
#define DEFAULT_VHOST "default"

/***** Change this via the SinglePostLimit configuration value in boa.conf *****/
#define SINGLE_POST_LIMIT_DEFAULT               1024 * 1024 /* 1 MB */

/***** Various stuff that you may want to tweak, but probably shouldn't *****/

#define SOCKETBUF_SIZE                          32768
#define CLIENT_STREAM_SIZE                      8192
#define WEB_BUFFER_SIZE				4096*5
#define MAX_HEADER_LENGTH			1024
#define	HTTP_HEADER_LENGTH		256

#define MIME_HASHTABLE_SIZE			47
#define PASSWD_HASHTABLE_SIZE			47
#define ALIAS_HASHTABLE_SIZE			17

/*
  use the system account info to implement the www authentication
*/
#define WWW_AUTHEN_EXCEP_EXPLORE	-2//user  exploration
#define WWW_AUTHEN_EXCEP_HEADER		-1  //exceptional  authen header
#define WWW_AUTHEN_FAIL				0
#define WWW_AUTHEN_SUCCESS			1


#define REQUEST_TIMEOUT				60

#define MIME_TYPES_DEFAULT                      "/etc/mime.types"
#define CGI_MIME_TYPE                           "application/x-httpd-cgi"

/***** CHANGE ANYTHING BELOW THIS LINE AT YOUR OWN PERIL *****/
/***** You will probably introduce buffer overruns unless you know
       what you are doing *****/
#ifdef	WIN32
#define MAX_FILE_LENGTH				MAX_PATH
#else
#define MAX_FILE_LENGTH				NAME_MAX
#endif
#define MAX_PATH_LENGTH				PATH_MAX

#ifdef ACCEPT_ON
#define MAX_ACCEPT_LENGTH MAX_HEADER_LENGTH
#else
#define MAX_ACCEPT_LENGTH 0
#endif

#ifndef SERVER_VERSION
#define SERVER_VERSION 				"ECPWS/0.1.0rc10"
#endif
#define SERVER_NAME 				"ECPWS"


#define CGI_VERSION				"CGI/1.1"



#ifdef	WIN32
#define	DEBUG_OUT				DbgPrint
#else
#define	DEBUG_OUT				printf
#endif

#ifdef	__MINGW32__	
#else
#pragma pack(push, 1)		/* add into stack of compiler */
#pragma pack(pop)		/* now align in one byte */
#define	__attribute__(x)		
#endif

#ifdef	SHARED_BUILD
#ifdef	DLL_EXPORT
	#define DLLPORT __declspec (dllexport)
#else
	#define DLLPORT __declspec (dllimport)
#endif

#else
	#define DLLPORT
#endif

#define W32_CDECL __cdecl


#ifndef OPEN_MAX
#define OPEN_MAX 256
#endif

#ifdef FD_SETSIZE	/*FD_SETSIZE is defined by OS or CRT*/
#define	MAX_FD			FD_SETSIZE
#else
#define	MAX_FD			OPEN_MAX
#endif

#ifndef SO_MAXCONN
#define SO_MAXCONN 250
#endif

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif


/* add 1 for 'LC_ALL' environ variable for support i18n */
#ifdef USE_NCSA_CGI_ENV
#define COMMON_CGI_COUNT 		10//8+1		
#else
#define COMMON_CGI_COUNT 		8//6+1		
#endif

/* environment variables for every connection */
#define CGI_ENV_MAX			100
#define CGI_ARGC_MAX		128


#define BOA_NI_MAXHOST 20

#define SERVER_METHOD "http"

/*********** MMAP_LIST CONSTANTS ************************/
#define MMAP_LIST_SIZE 256
#define MMAP_LIST_MASK 255
#define MMAP_LIST_USE_MAX 128

#if 0
#define MAX_FILE_MMAP		100 * 1024 /* 100K */
#endif

#define ECPWS_READ_FDSET(webSvr)			(&webSvr->readFdSet)
#define ECPWS_WRITE_FDSET(webSvr)			(&webSvr->writeFdSet)
#define _ECPWS_SET_FDSET(webSvr, fd, where)	{ FD_SET(fd, where); if (fd > webSvr->maxFd)  webSvr->maxFd = fd; }
#define _ECPWS_CLEAR_FDSET(conn, fd, where)	{ FD_CLR(fd, where); }

#define	ECPWS_BLOCK_READ(webSvr, fd)		_ECPWS_SET_FDSET(webSvr, fd, ECPWS_READ_FDSET(webSvr) )
#define	ECPWS_BLOCK_WRITE(webSvr, fd)		_ECPWS_SET_FDSET(webSvr, fd, ECPWS_WRITE_FDSET(webSvr) )

#define	ECPWS_CLEAR_READ(webSvr, fd)		_ECPWS_CLEAR_FDSET(webSvr, fd, ECPWS_READ_FDSET(webSvr) )
#define	ECPWS_CLEAR_WRITE(webSvr, fd)		_ECPWS_CLEAR_FDSET(webSvr, fd, ECPWS_WRITE_FDSET(webSvr) )

#define	ECPWS_IS_READ(webSvr, fd)			FD_ISSET(fd, ECPWS_READ_FDSET(webSvr))
#define	ECPWS_IS_WRITE(webSvr, fd)			FD_ISSET(fd, ECPWS_WRITE_FDSET(webSvr))

/******** MACROS TO CHANGE BLOCK/NON-BLOCK **************/
/* If and when everyone has a modern gcc or other near-C99 compiler,
 * change these to static inline functions. Also note that since
 * we never fuss with O_APPEND append or O_ASYNC, we shouldn't have
 * to perform an extra system call to F_GETFL first.
 */
#if 1//def BOA_USE_GETFL
#define set_block_fd(fd)    real_set_block_fd(fd)
#define set_nonblock_fd(fd) real_set_nonblock_fd(fd)
#else
#define set_block_fd(fd)    fcntl(fd, F_SETFL, 0)
#define set_nonblock_fd(fd) fcntl(fd, F_SETFL, NOBLOCK)
#endif


#define	RESET_REQ_OUT_BUFFER(conn) 	(conn->buffer_end = 0)

/***************** USEFUL MACROS ************************/

#define SQUASH_KA(conn)	(conn->keepalive=KA_STOPPED)

#ifdef HAVE_FUNC
#define WARN(mesg) 	ECPWS_LOG_INFO(__FILE__" Line %d %s() : %s",  __LINE__, __func__, mesg)
#else
#define WARN(mesg)	printf(__FILE__" Line %d : %s", __LINE__, mesg)
#endif
#define DIE(mesg)	{ECPWS_ERR_INFO(mesg); exit(1);}


#if	WEB_DEBUG_ENABLE
#define	WEB_MSG_DEBUG( format ,...) \
			{char str[10240]; SPRINTF(str, sizeof(str), format, ##__VA_ARGS__); ecpwsOutputMsg(str); }
/*
When # appears before __VA_ARGS__, the entire expanded __VA_ARGS__ is enclosed in quotes:
#define showlist(...) puts(#__VA_ARGS__)
showlist();            // expands to puts("")
showlist(1, "x", int); // expands to puts("1, \"x\", int")

A ## operator between any two successive identifiers in the replacement-list runs parameter replacement on 
the two identifiers (which are not macro-expanded first) and then concatenates the result. This operation is 
called "concatenation" or "token pasting". Only tokens that form a valid token together may be pasted: identifiers 
that form a longer identifier, digits that form a number, or operators + and = that form a +=. A comment cannot 
be created by pasting / and * because comments are removed from text before macro substitution is considered. 
If the result of concatenation is not a valid token, the behavior is undefined.

Note: some compilers offer an extension that allows ## to appear after a comma and before __VA_ARGS__, in 
which case the ## does nothing when __VA_ARGS__ is non-empty, but removes the comma when __VA_ARGS__ 
is empty: this makes it possible to define macros such as fprintf (stderr, format, ##__VA_ARGS__)
*/

//#warning		"build with DEBUG info" format, ##__VA_ARGS__
	#define	TRACE_LINE()	WEB_MSG_DEBUG( __FILE__"|%s[%d] --", __FUNCTION__, __LINE__)
#else
#define	WEB_MSG_DEBUG( format ,...) 			{}
//#warning		"build without DEBUG info"
//#pragma 	warning( "build without DEBUG info" )

	#define	TRACE_LINE()	{}
#endif


#define	ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, ...)	\
		{TRACE_LINE();WEB_CONN_ERROR(conn,WEB_RES_BAD_REQUEST);	\
		ecpwsLog(conn, TRUE_T, FALSE_T, FALSE_T, ##__VA_ARGS__);}


#define	ECPWS_LOG_REQ_ERR(conn, ...)	\
		TRACE_LINE();ecpwsLog(conn, TRUE_T, TRUE_T, FALSE_T, ##__VA_ARGS__)

#define	ECPWS_LOG_REQ_NOERR(conn, ...)	\
		TRACE_LINE();ecpwsLog(conn, TRUE_T, FALSE_T, FALSE_T, ##__VA_ARGS__)


#define	ECPWS_FATAL(...)	\
		TRACE_LINE();ecpwsLog(NULL, FALSE_T, TRUE_T, TRUE_T, ##__VA_ARGS__)

#define	ECPWS_FATAL_WITH_REQ(conn, ...)	\
		TRACE_LINE();ecpwsLog(conn, TRUE_T, TRUE_T, TRUE_T, ##__VA_ARGS__)

#define	ECPWS_ERR_INFO(...)	\
		TRACE_LINE();ecpwsLog(NULL, FALSE_T, TRUE_T, FALSE_T, ##__VA_ARGS__)

#define	ECPWS_LOG_INFO(...)	\
		TRACE_LINE();ecpwsLog(NULL, FALSE_T, FALSE_T, FALSE_T, ##__VA_ARGS__)

#define	ECPWS_DEBUG_INFO(...)	\
		TRACE_LINE();ecpwsLog(NULL, FALSE_T, FALSE_T, FALSE_T, ##__VA_ARGS__)

#define	ECPWS_LOG_INFO_WITH_REQ(conn, ...)	\
		TRACE_LINE();ecpwsLog(conn, TRUE_T, FALSE_T, FALSE_T, ##__VA_ARGS__)

//

/* change the default charset as GB2312 
 */
//#define HTML "text/html; charset=GB2312" /*ISO-8859-1"*/
#define HTML "text/html; charset=ISO-8859-1"

/* HTTP header with HTML content type */
#define	HTTP_HEADER_HTML(conn, msg)	\
		req_write((conn), "%s %s\r\n%sContent-Type: "HTML"\r\n" , \
				getHeaderVerString((conn)), (msg), print_http_headers((conn)) )


#define	HTTP_HEADER_CONTENT(conn, msg, contentType)	\
		req_write((conn), "%s %s\r\n%sContent-Type: %s\r\n\r\n" , \
				getHeaderVerString((conn)), (msg), print_http_headers((conn)), (contentType) )

#define	HTTP_HEADER_CONTENT_MODIFY(conn, msg, contentType, modify)	\
		req_write((conn), "%s %s\r\n%sContent-Type: %s\r\n%s\r\n\r\n" , \
				getHeaderVerString((conn)), (msg), print_http_headers((conn)), (contentType), (modify) )

#define	HTTP_SEND_ERROR_PAGE(conn, headerMsg, bodyMsg)	\
	{ SQUASH_KA(conn); if (conn->http_version != WEB_HTTP09) \
		{ HTTP_HEADER_HTML(conn, headerMsg); if (strlen(bodyMsg)) { req_write(conn, "Content-Length: %d\r\n\r\n", strlen(bodyMsg));}} \
		if (conn->method != WEB_M_HEAD && strlen(bodyMsg)){req_write(conn, bodyMsg); } req_flush(conn); }


#if  WEB_DEBUG_ENABLE
#else
#endif

#define	DEBUG_INDEX_DIR	0

#if 1//def DONT_HAVE_SA_FAMILY_T
/* POSIX.1g specifies this type name for the `sa_family' member.  */
typedef unsigned short int sa_family_t;
#endif

typedef	enum _BOOL_T
{
	FALSE_T	= 0,
	TRUE_T	= 1
}BOOL_T;


#define	IS_FALSE(bool)		(bool==FALSE_T)
#define	IS_TRUE(bool)		(bool==TRUE_T)

typedef	enum WEB_HTTP_METHOD
{
	WEB_M_GET = 1,
	WEB_M_HEAD,
	WEB_M_PUT,
	WEB_M_POST,
	WEB_M_DELETE,
	WEB_M_LINK,
	WEB_M_UNLINK,
	WEB_M_MOVE,
	WEB_M_TRACE
}WEB_HTTP_METHOD;


typedef	enum WEB_HTTP_VERSION
{
	WEB_HTTP09=1,
	WEB_HTTP10,
	WEB_HTTP11 
}WEB_HTTP_VERSION;

/* REQUEST STATUS (conn->status) */
typedef	enum WEB_CLIENT_STATUS
{
	WEB_CLIENT_S_CONTINUE = 0,
	WEB_CLIENT_S_READ_HEADER,
	WEB_CLIENT_S_HTTP_WRITE,		/* write static html/content */
	WEB_CLIENT_S_BODY_READ,		/* POST data */
	WEB_CLIENT_S_BODY_WRITE,
	WEB_CLIENT_S_CGI_READ,
	WEB_CLIENT_S_CGI_WRITE,
	WEB_CLIENT_S_IO_SHUFFLE,	/* not MMAP output */
	WEB_CLIENT_S_DONE,
	WEB_CLIENT_S_ERROR
}WEB_CLIENT_STATUS;


typedef	enum _WEB_EVENT_T
{
	WEB_EVENT_NONE	= 0,
	WEB_EVENT_READY,
	WEB_EVENT_TIMEOUT,
	WEB_EVENT_ERROR,
}WEB_EVENT_T;

typedef enum WEB_RESPONSE_CODE
{
	WEB_RES_CONTINUE = 100,
	WEB_RES_REQUEST_OK = 200,
	WEB_RES_CREATED,
	WEB_RES_ACCEPTED,
	WEB_RES_PROVISIONAL,
	WEB_RES_NO_CONTENT,
	WEB_RES_R_205,
	WEB_RES_PARTIAL_CONTENT,
	WEB_RES_MULTIPLE = 300,
	WEB_RES_MOVED_PERM,
	WEB_RES_MOVED_TEMP,
	WEB_RES_303,
	WEB_RES_NOT_MODIFIED,
	WEB_RES_BAD_REQUEST = 400,
	WEB_RES_UNAUTHORIZED,
	WEB_RES_PAYMENT,
	WEB_RES_FORBIDDEN,
	WEB_RES_NOT_FOUND,
	WEB_RES_METHOD_NA, /* method not allowed */
	WEB_RES_NON_ACC,   /* non acceptable */
	WEB_RES_PROXY,     /* proxy auth required */
	WEB_RES_REQUEST_TO, /* request timeout */
	WEB_RES_CONFLICT,
	WEB_RES_GONE,
	WEB_RES_LENGTH_REQUIRED,
	WEB_RES_PRECONDITION_FAILED,
	WEB_RES_413,
	WEB_RES_REQUEST_URI_TOO_LONG,
	WEB_RES_415,
	WEB_RES_INVALID_RANGE,
	
	WEB_RES_ERROR = 500,
	WEB_RES_NOT_IMP = 501,
	WEB_RES_BAD_GATEWAY = 502,
	WEB_RES_SERVICE_UNAV = 503,
	WEB_RES_GATEWAY_TO = 504, /* gateway timeout */
	WEB_RES_BAD_VERSION =505
}WEB_RESPONSE_CODE;

typedef	enum ALIAS_TYPE_T
{
	ALIAS_T_ALIAS, 
	ALIAS_T_SCRIPT,
	ALIAS_T_REDIRECT
}ALIAS_TYPE_T;


/* KEEPALIVE CONSTANTS (conn->keepalive) */
typedef enum KA_STATUS
{
	KA_INACTIVE,
	KA_ACTIVE,
	KA_STOPPED
}KA_STATUS;

/* only for CGI_T_CGI type */
typedef enum CGI_STATUS
{
	CGI_PARSE,		/* used by CGI_CGI. Waiting data from pipe, then parse it and send first Header(not from CGI program) */
	CGI_BUFFER,	/* used by CGI_T_NPH and dirList, gunzip */
	CGI_DONE		/* data from can be send out now, send data which from CGI */
}CGI_STATUS;

/* CGI TYPE (conn->is_cgi) */
typedef enum CGI_TYPE
{
	CGI_T_NONE = 0,
	CGI_T_NPH = 1,
	CGI_T_CGI
}CGI_TYPE;


/* for pipe descriptors, the first one is for read, the second is for write*/
typedef	enum PIPE_FD
{
	PIPE_FD_READ = 0,
	PIPE_FD_WRITE	
}PIPE_FD_T;

typedef struct range
{
    unsigned long		start;
    unsigned long		stop;
    struct range		*next;
}RANGE_T;


typedef	struct MMAP_T
{
#ifdef	WIN32
	HANDLE		hFile;
	HANDLE		hFileMap;
#endif
	dev_t		dev;
	ino_t		ino;
	
	char			*mmap;
	off_t			len;
	int			use_count;
}MMAP_T;

typedef	enum REQUEST_AUTHEN_STATUS
{
	REQUEST_NOT_AUTHENED = 0,
	REQUEST_AUTHENED,
	REQUEST_AUTHEN_FAILED
}REQUEST_AUTHEN_STATUS;

typedef	struct web_request
{/* pending requests */
	WEB_EVENT_T					event;
	REQUEST_AUTHEN_STATUS	authen_status;

	WEB_CLIENT_STATUS			status;
	KA_STATUS				keepalive;   /* keepalive status */
	WEB_HTTP_VERSION			http_version;
	WEB_HTTP_METHOD			method;    /* WEB_M_GET, WEB_M_POST, etc. */
	WEB_RESPONSE_CODE			response_status; /* WEB_RES_NOT_FOUND, etc.. */

	CGI_TYPE				cgi_type;
	CGI_STATUS				cgi_status;

	/* should pollfd_id be zeroable or no ? */
#ifdef HAVE_POLL
	int pollfd_id;
#endif

	char				*pathname;             /* pathname of requested file */

	RANGE_T			*ranges;              /* our Ranges */
	int				numranges;

	int				data_fd;		/* fd of data */
	unsigned long		filesize;		/* filesize */
	unsigned long		filepos;		/* position in file */
	unsigned long		bytes_written;	/* total bytes written (sans header) */
	char				*data_mem;		/* mmapped/malloced char array */

	char *logline;              /* line to log file */

	char *header_line;          /* beginning of un or incompletely processed header line */
	char *header_end;           /* last known end of header, or end of processed data */
	int parse_pos;              /* how much have we parsed */

	int buffer_start;           /* where the buffer starts */
	int buffer_end;             /* where the buffer ends */

	char *if_modified_since;    /* If-Modified-Since */
	time_t last_modified;       /* Last-modified: */

	/* CGI vars */
	int cgi_env_index;          /* index into array */
#ifdef	WIN32
	PROCESS_INFORMATION	pi;
#endif
	/* Agent and referer for logfiles */
	char *header_host;
	char *header_user_agent;
	char *header_referer;
	char *header_ifrange;
	char *host;                 /* what we end up using for 'host', no matter the contents of header_host */

	char		*cookies;

	int post_data_fd;           /* fd for post data tmpfile */

	char *path_info;            /* env variable */
	char *path_translated;      /* env variable */
	char *script_name;          /* env variable */
	char *query_string;         /* env variable, part of URL, which is behind the '?' */
	char *content_type;         /* env variable */
	char *content_length;       /* env variable */

	MMAP_T		*mmapEntry;
	char			openedFile[MAX_HEADER_LENGTH];

	/* everything **above** this line is zeroed in sanitize_request */
	/* this may include 'fd' */
	/* in sanitize_request with the 'new' parameter set to 1,
	* kacount is set to ka_max and client_stream_pos is also zeroed.
	* Also, time_last is set to 'NOW'
	*/
	int socket;                     /* client's socket fd */
	time_t		time_last;           /* time of last succ. op. */
	char			localIpAddr[BOA_NI_MAXHOST]; /* for virtualhost */
	char			remoteIpAddr[BOA_NI_MAXHOST]; /* after inet_ntoa */
	unsigned int	remotePort;            /* could be used for ident */

	unsigned int kacount;                /* keepalive count */
	int client_stream_pos;      /* how much have we read... */

	/* everything below this line is kept regardless */
	char buffer[WEB_BUFFER_SIZE + 1]; /* generic I/O buffer */
	char request_uri[MAX_HEADER_LENGTH + 1]; /* uri, eg content after the GET command */
	char client_stream[CLIENT_STREAM_SIZE]; /* data from client - fit or be hosed */
	char *cgi_env[CGI_ENV_MAX + 1]; /* CGI environment */

#ifdef ACCEPT_ON
	char accept[MAX_ACCEPT_LENGTH]; /* Accept: fields */
#endif

	struct	_SVR_WEB		*webSvr;
	int					index;

	struct web_request *next;       /* next */
	struct web_request *prev;       /* previous */
}WEB_CLIENT_CONN;

typedef	struct	_fsm_event_t
{
	WEB_EVENT_T				event;
	
	WEB_CLIENT_STATUS		(*handler)(WEB_CLIENT_CONN *conn);
}fsm_event_t;



typedef	struct	_fsm_req_status_t
{
	WEB_CLIENT_STATUS		state;

	int					size;
	fsm_event_t			*events;
}fsm_req_status_t;


typedef	struct	_SERVICE_FSM
{
	int					size;
	fsm_req_status_t		*states;
}SERVICE_FSM;


typedef	struct statistics
{
	long		requests;
	long		errors;
}WEB_STATISTICS_T;


typedef	enum WEB_STATUS
{
	WEB_STATUS_INIT_CONFIG = 0 ,
	WEB_STATUS_INIT_LOG,
	WEB_STATUS_INIT_SOCKET,
	WEB_STATUS_INITED,
}WEB_STATUS_T;



typedef	struct	_WEB_REQ_QUEUES
{
	int				readyCount;
	int				freeCount;
	
	WEB_CLIENT_CONN		*readyQueue;  /* ready list head */
	WEB_CLIENT_CONN		*freeQueue;   /* free list head */
}WEB_REQ_QUEUES;

typedef	void (*REQ_STACK_HANDLER)(WEB_CLIENT_CONN **header,	WEB_CLIENT_CONN * conn);

typedef	struct	_SVR_WEB
{
	CmnMuxWebConfig		cfg;

	int					serverSocket;
	fd_set				readFdSet;
	fd_set				writeFdSet;
	int					maxFd;

	unsigned				totalConnections;
	BOOL_T				hasPendingReqs;

	WEB_REQ_QUEUES			queues;
	REQ_STACK_HANDLER		popHandler;
	REQ_STACK_HANDLER		pushHandler;

	char					tempBuffer[WEB_BUFFER_SIZE];	/* temp buffer used in response handler */

	WEB_STATUS_T			status;
	WEB_STATISTICS_T		statistics;

	time_t				start_time;
	time_t				current_time;

	void					*priv;
}SVR_WEB;

#define	QUEUE_EN_READY(webSvr, conn)	\
	{webSvr->pushHandler(&webSvr->queues.readyQueue, conn);webSvr->queues.readyCount++;\
	ECPWS_LOG_INFO("Total %d in READY_QUEUE\n", webSvr->queues.readyCount);}

#define	QUEUE_EN_FREE(webSvr, conn)	\
	{webSvr->pushHandler(&webSvr->queues.freeQueue, conn);webSvr->queues.freeCount++;\
	/*ECPWS_LOG_INFO("No. %d  is in FREE_QUEUE of total %d items\n", conn->index, webSvr->queues.freeCount);*/}



#define	QUEUE_DE_READY(webSvr, conn)	\
	{webSvr->popHandler(&webSvr->queues.readyQueue, conn);webSvr->queues.readyCount--;}

#define	QUEUE_DE_FREE(webSvr, conn)	\
	{webSvr->popHandler(&webSvr->queues.freeQueue, conn);webSvr->queues.freeCount--;}


#define	SVR_WEB_PARSE_CONFIG(webSvr)	\
		{	(webSvr)->cfg.addAlias = ecpwsAddAlias; \
			(webSvr)->cfg.addMimeType = ecpwsAddMimeType; \
		cmnMuxConfigParseWeb(MUX_WEB_CONFIG_FILE, &(webSvr)->cfg ); }


#define	QUEUE_FIRST_FREE(webSvr)	((webSvr)->queues.freeQueue)
#define	QUEUE_FIRST_READY(webSvr)	((webSvr)->queues.readyQueue)

#define	WEB_EMIT_EVENT(conn, ev)		(conn)->event = (ev)

#define	WEB_CONN_ERROR(conn, errorCode)	conn->response_status = errorCode

#define	WEB_CONN_RES_ERR(conn, code)	\
			{ (conn)->response_status = (code); return WEB_CLIENT_S_ERROR;}

typedef long sptime;

struct spwd
{
	char		*	sp_namp;				/* login name */
	char			*sp_pwdp;				/* encrypted password */
	sptime		sp_lstchg;			/* date of last change */
	sptime		sp_min;				/* minimum number of days between changes */
	sptime		sp_max;				/* maximum number of days between changes */
	sptime		sp_warn;				/* number of days of warning before password
								   expires */
	sptime		sp_inact;			/* number of days after password expires
								   until the account becomes unusable. */
	sptime		sp_expire;			/* days since 1/1/70 until account expires */
	unsigned long	sp_flag;		/* reserved for future use */
};

#if WIN32
struct passwd
{
	char		*pw_name;		/* Username.  */
	char		*pw_passwd;	/* Password.  */
	UID_T	pw_uid;			/* User ID.  */
	GID_T	pw_gid;			/* Group ID.  */
	char		*pw_gecos;		/* Real name.  */
	char		*pw_dir;		/* Home directory.  */
	char		*pw_shell;		/* Shell program.  */
};
#else
#include <sys/types.h>
#include <pwd.h>
#endif

/* Highest character number that can possibly be passed through un-escaped */
#define NEEDS_ESCAPE_BITS 128

#ifndef NEEDS_ESCAPE_SHIFT
#define NEEDS_ESCAPE_SHIFT 5    /* 1 << 5 is 32 bits */
#endif

#define NEEDS_ESCAPE_WORD_LENGTH		(1<<NEEDS_ESCAPE_SHIFT)

#define NEEDS_ESCAPE_INDEX(c)	((c)>>NEEDS_ESCAPE_SHIFT)

/* Assume variable shift is fast, otherwise this could be a table lookup */
#define NEEDS_ESCAPE_MASK(c)  (1<<((c)&(NEEDS_ESCAPE_WORD_LENGTH - 1)))

/* Newer compilers could use an inline function.
 * This macro works great, as long as you pass unsigned int or unsigned char.
 */
#define needs_escape(c)	((c)>=NEEDS_ESCAPE_BITS || _needs_escape[NEEDS_ESCAPE_INDEX(c)]&NEEDS_ESCAPE_MASK(c))

extern unsigned long     _needs_escape[(NEEDS_ESCAPE_BITS + NEEDS_ESCAPE_WORD_LENGTH -
                   1) / NEEDS_ESCAPE_WORD_LENGTH];


void build_needs_escape(void);

#ifdef	WIN32
extern char *optarg;            /* For getopt */
int mkstemp (char *tmpl);
#else
struct spwd *get_spnam(const char *name);
#endif


extern FILE	*cgiLogFile;


extern unsigned int system_bufsize;      /* Default size of SNDBUF given by system */


extern JMP_BUFFER	env;
extern int				handle_sigbus;


extern	SVR_WEB	 _webSvr;

/* alias */
void ecpwsAddAlias(const char *fakename, const char *realname, int _type);
int translate_uri(WEB_CLIENT_CONN * conn);
void cleanupAlias(void);


WEB_CLIENT_STATUS ecpwsHttpPrepareStaticResponse(WEB_CLIENT_CONN * conn);

/* hash */
char *get_mime_type(const char *filename);
char *get_home_dir(const char *name);
void clearupAllHash(void);
void hash_show_stats(void);
void ecpwsAddMimeType(const char *extension, const char *type);

/* log */
void webSvrOpenLogs(SVR_WEB *webSvr);

void log_access(WEB_CLIENT_CONN * conn);
void ecpwsLog(WEB_CLIENT_CONN *conn,BOOL_T withReqDebug, BOOL_T isSysErr, BOOL_T isQuit, const char *format,...);

/* queue */
void block_request(SVR_WEB *webSvr, WEB_CLIENT_CONN * conn);
void ready_request(SVR_WEB *webSvr, WEB_CLIENT_CONN * conn);


WEB_CLIENT_STATUS ecpwsHandlerTimeout(WEB_CLIENT_CONN *conn);
WEB_CLIENT_STATUS ecpwsHttpHandlerReadHeader(WEB_CLIENT_CONN * conn);
WEB_CLIENT_STATUS ecpwsHttpHandlerWriteHtml(WEB_CLIENT_CONN * conn);
WEB_CLIENT_STATUS ecpwsHttpHandlerReadPost(WEB_CLIENT_CONN * conn);
WEB_CLIENT_STATUS ecpwsHttpHandlerWritePost(WEB_CLIENT_CONN * conn);
WEB_CLIENT_STATUS ecpwsHttpHandlerError(WEB_CLIENT_CONN *conn);
WEB_CLIENT_STATUS ecpwsCgiHandlerReadPipe(WEB_CLIENT_CONN * conn);
WEB_CLIENT_STATUS ecpwsCgiHandlerSendout(WEB_CLIENT_CONN * conn);
WEB_CLIENT_STATUS io_shuffle(WEB_CLIENT_CONN * conn);
#ifdef HAVE_SENDFILE
#include <sys/sendfile.h>
WEB_CLIENT_STATUS io_shuffle_sendfile(WEB_CLIENT_CONN * conn);
#endif

/* request */
WEB_CLIENT_CONN *new_request(SVR_WEB *webSvr);
void process_requests(SVR_WEB *webSvr);
int process_header_end(WEB_CLIENT_CONN * conn);
int process_header_line(WEB_CLIENT_CONN * conn);
void add_accept_header(WEB_CLIENT_CONN * conn, const char *mime_type);
void freeAllRequests(SVR_WEB *webSvr);
WEB_CLIENT_STATUS ecpwsHttpParseRequestLine(WEB_CLIENT_CONN *conn);

/* response */
const char *getHeaderVerString(WEB_CLIENT_CONN * conn);
char *getHeaderContentType(WEB_CLIENT_CONN * conn);
char *getHeaderLastModified(WEB_CLIENT_CONN * conn);
char *print_http_headers(WEB_CLIENT_CONN * conn);
void print_content_range(WEB_CLIENT_CONN * conn);
void print_partial_content_continue(WEB_CLIENT_CONN * conn);
void print_partial_content_done(WEB_CLIENT_CONN * conn);
int complete_response(WEB_CLIENT_CONN *conn);

void send_r_continue(WEB_CLIENT_CONN * conn); /* 100 */
void send_r_request_ok(WEB_CLIENT_CONN * conn); /* 200 */
void send_r_no_content(WEB_CLIENT_CONN * conn); /* 204 */
void send_r_partial_content(WEB_CLIENT_CONN * conn); /* 206 */

void send_r_invalid_range(WEB_CLIENT_CONN * conn); /* 416 */

/* cgi */
void createCgiCommonEnvs(SVR_WEB *webSvr);
void clearCgiCommonEnvs(void);
int add_cgi_env(WEB_CLIENT_CONN * conn, const char *key, const char *value, BOOL_T http_prefix);
int complete_env(WEB_CLIENT_CONN * conn);
void create_argv(WEB_CLIENT_CONN * conn, char **aargv);
WEB_CLIENT_STATUS ecpwsCgiCreate(WEB_CLIENT_CONN * conn);

/* signals */
void init_signals(void);

/* util.c */
void clean_pathname(char *pathname);
char *get_commonlog_time(SVR_WEB  *webSvr);
void rfc822_time_buf(char *buf, time_t s);
char *simple_itoa(unsigned int i);
int web_atoi(const char *s);
int month2int(const char *month);
int modified_since(time_t * mtime, const char *if_modified_since);
int unescape_uri(char *uri, char **query_string);
int create_temporary_file(short want_unlink, char *storage, unsigned int size, char *tempdir);
char *normalize_path(char *path);
int real_set_block_fd(int fd);
int real_set_nonblock_fd(int fd);
char *to_upper(char *str);
void strlower(char *s);
int check_host(const char *r);

/* buffer */
int req_write(WEB_CLIENT_CONN * conn, const char *format, ...);
char *req_write_escape_http(WEB_CLIENT_CONN * conn, const char *msg);
char *req_write_escape_html(WEB_CLIENT_CONN * conn, const char *msg);
int req_flush(WEB_CLIENT_CONN * conn);
char *escape_uri(const char *uri);
char *escape_string(const char *inp, char *buf);

/* timestamp */
void timestamp(void);

/* mmap_cache */
MMAP_T *find_mmap(WEB_CLIENT_CONN *conn, struct stat *s);
void release_mmap(MMAP_T *e);


int scandir(const char *dir, struct dirent ***namelist,
        int (*select) (const struct dirent *),
        int (*compar) (const struct dirent **, const struct dirent **));

/* ip */
int bind_server(int sock, char *ip, unsigned int port);
char *ascii_sockaddr(struct SOCKADDR *s, char *dest, unsigned int len);
int net_port(struct SOCKADDR *s);

int webSvrServiceSocketInit(SVR_WEB *);
void webSvrNewConnection(SVR_WEB *);
int svcWebConnRead(WEB_CLIENT_CONN *conn, char *buffer, int size);
int svcWebConnWrite(WEB_CLIENT_CONN *conn, char *buffer, int size);


void freeOneRequest(SVR_WEB *webSvr, WEB_CLIENT_CONN *conn);


/* range.c */
void ranges_reset(WEB_CLIENT_CONN * conn);
RANGE_T *range_pool_pop(void);
void range_pool_empty(void);
void range_pool_push(RANGE_T * r);
int ranges_fixup(WEB_CLIENT_CONN * conn);
int range_parse(WEB_CLIENT_CONN * conn, const char *str);

const char* ecpwsStateName(WEB_CLIENT_STATUS _status);

void ecpwsOutputMsg(char *msg);
int www_base_authen(WEB_CLIENT_CONN *conn, char *buf);
int ecpwsHttpAuthen(WEB_CLIENT_CONN *conn);

int svrWebMainLoop(CmnThread *th);


extern	PluginJSonHandler muxWebJsonHandlers[];


#endif

