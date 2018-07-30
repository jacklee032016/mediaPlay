/*
 * Constants definations
 */


#ifndef	__MUX_SVR_WEB_DEFINES_H__
#define	__MUX_SVR_WEB_DEFINES_H__

/* options for features in building */
#define	WEB_DEBUG_ENABLE			1
#define	TUNE_SNDBUF


#ifdef	WIN32
#define	DIR_SPLITTER_CHAR		'\\'
#define	HOME_DIR		"d:\\works\\webservers\\webSvr\\"
#define	DEFAULT_CONFIG_FILE 	HOME_DIR"config\\etc\\webSvr\\webSvr.win.conf" /* locate me in the server root */
#else
#define	DIR_SPLITTER_CHAR		'/'
#define	HOME_DIR				"/works/webserver/webSvr/"
#if ARCH_X86
#define	DEFAULT_CONFIG_FILE 	ROOT_DIR"/releases/etc/webService.conf" /* locate me in the server root */
#else
#define	DEFAULT_CONFIG_FILE 	"/etc/webService.conf" /* locate me in the server root */
#endif
#endif

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
#define BUFFER_SIZE                             4096
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

#ifndef	OK
#define	OK									0
#endif

#ifndef	FAIL
#define	FAIL								-1
#endif


#ifndef	false
#define	false								0
#endif

#ifndef	true
#define	true									(!false)
#endif

#define	EXIT_FAILURE						1
#define	EXIT_SUCCESS						0

#define	SET_BIT(value, bit)			((value) << (bit))
#define	GET_BIT(value, bit)			(((value)>>(bit))&0x01)

#define	SET_FLAGS(flags, bitPosition)	\
		flags |= SET_BIT(0x01, (bitPosition) ) 

#define	CLEAR_FLAGS(flags, bitPosition)	\
		flags &= ~(SET_BIT(0x01, (bitPosition) ) )	

#define	CHECK_FLAGS(flags, bitPosition)	\
		( (flags&SET_BIT(0x01,(bitPosition) ) )!=0 )

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
#define COMMON_CGI_COUNT 8+1		
#else
#define COMMON_CGI_COUNT 6+1		
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
			{char str[10240]; SPRINTF(str, sizeof(str), format, ##__VA_ARGS__); ECPWS_LOG_INFO(str); }
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
	#define	trace()			ecpwsLog(NULL, FALSE_T, FALSE_T, FALSE_T, __FILE__"|%s[%d]\r\n", __FUNCTION__, __LINE__)
	#define	TRACE_LINE()	ecpwsLog(NULL, FALSE_T, FALSE_T, FALSE_T, __FILE__"|%s[%d] --", __FUNCTION__, __LINE__)
#else
#define	WEB_MSG_DEBUG( format ,...) 			{}
//#warning		"build without DEBUG info"
//#pragma 	warning( "build without DEBUG info" )

	#define trace()			{}
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

#define	ECPWS_LOG_INFO_WITH_REQ(conn, ...)	\
		TRACE_LINE();ecpwsLog(conn, TRUE_T, FALSE_T, FALSE_T, ##__VA_ARGS__)


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

#endif

