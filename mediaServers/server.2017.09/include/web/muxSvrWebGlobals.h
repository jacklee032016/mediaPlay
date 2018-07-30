/*
 * Global Variables  and function declarations in this header
 */

#ifndef __MUX_SVR_WEB_GLOBALS_H__
#define __MUX_SVR_WEB_GLOBALS_H__

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
	char buffer[BUFFER_SIZE + 1]; /* generic I/O buffer */
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


typedef	struct	_WEB_CONFIG
{
	char		configFileName[1024];
	
	char		*error_log_name;
	char		*access_log_name;
	char		*cgi_log_name;
	int		verbose_cgi_logs;

	
	char		*server_name;
	char		*server_admin;
	char		*server_ip;
	unsigned int server_port;
	
	char		*default_vhost;
	int		maxConnections;
	int		backlog;		/* max link in listen() function */

	int		virtualhost;
	char		*vhost_root;

	char		*document_root;
	char		*user_dir;
	char		*directory_index;
	char		*default_type;	/* default MIME type */
	char		*default_charset;
	char		*dirmaker;		/* executable for directory list */
	char		*cachedir;

	char		*tempdir;

	unsigned int cgi_umask;

	char		*pid_file;
	char		*cgi_path;
	int		single_post_limit;

	char		*baseAuthen;

	char 	*server_root;
#ifdef WIN32	
	char		*server_uid;
	char		*server_gid;
#else
	UID_T	server_uid;
	GID_T	server_gid;
#endif	

	unsigned int ka_timeout;	/*KeepAlive timeout*/
	unsigned int ka_max;		/* KeepAlive */

	int use_localtime;
}WEB_CONFIG_T;

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
	WEB_CONFIG_T			cfg;

	int					serverSocket;
	fd_set				readFdSet;
	fd_set				writeFdSet;
	int					maxFd;

	unsigned				totalConnections;
	BOOL_T				hasPendingReqs;

	WEB_REQ_QUEUES			queues;
	REQ_STACK_HANDLER		popHandler;
	REQ_STACK_HANDLER		pushHandler;

	char					tempBuffer[BUFFER_SIZE];	/* temp buffer used in response handler */

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
	ECPWS_LOG_INFO("No. %d  is in FREE_QUEUE of total %d items\n", conn->index, webSvr->queues.freeCount);}



#define	QUEUE_DE_READY(webSvr, conn)	\
	{webSvr->popHandler(&webSvr->queues.readyQueue, conn);webSvr->queues.readyCount--;}

#define	QUEUE_DE_FREE(webSvr, conn)	\
	{webSvr->popHandler(&webSvr->queues.freeQueue, conn);webSvr->queues.freeCount--;}



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
void add_alias(const char *fakename, const char *realname, ALIAS_TYPE_T type);
int translate_uri(WEB_CLIENT_CONN * conn);
void cleanupAlias(void);

/* config */
void webSvrParseConfigFile(SVR_WEB *webSvr);

WEB_CLIENT_STATUS ecpwsHttpPrepareStaticResponse(WEB_CLIENT_CONN * conn);

/* hash */
char *get_mime_type(const char *filename);
char *get_home_dir(const char *name);
void clearupAllHash(void);
void hash_show_stats(void);
void add_mime_type(const char *extension, const char *type);

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
int boa_atoi(const char *s);
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

const char* stateName(WEB_CLIENT_STATUS _status);

void ecpwsOutputMsg(char *msg);
int www_base_authen(WEB_CLIENT_CONN *conn, char *buf);
int ecpwsHttpAuthen(WEB_CLIENT_CONN *conn);

int svrWebMainLoop(CMN_THREAD_INFO *th);

#endif

