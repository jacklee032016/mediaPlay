/*
 * SVR_WEB : Embedded Cross-Platform Web Server
*/

#include "muxWeb.h"

#include "libMedia.h"
#include "libMux.h"

#ifndef	WIN32
/*added lizhijie  */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
/* end of added */
#endif

#define PASSWD_FILE        "/etc/passwd"
#define GROUP_FILE         "/etc/group"

#define TINYLOGIN_PWD_LIB

#undef BUSYBOX_PWDGRP_LIB

#ifdef BUSYBOX_PWDGRP_LIB
	const char * const bb_path_passwd_file = PASSWD_FILE;
	const char * const bb_path_group_file = GROUP_FILE;
#else
	#ifdef TINYLOGIN_PWD_LIB
		const char * const passwd_file = PASSWD_FILE;
		const char * const group_file = GROUP_FILE;
	#endif	
#endif


static void _sanitize_request(WEB_CLIENT_CONN *conn, int isNewReq)
{
	static unsigned int bytes_to_zero = offsetof(WEB_CLIENT_CONN, socket);
	CmnMuxWebConfig *cfg = &conn->webSvr->cfg;

	if (isNewReq)
	{
		conn->kacount = cfg->ka_max;
		conn->time_last = conn->webSvr->current_time;
		conn->client_stream_pos = 0;
		conn->status = WEB_CLIENT_S_READ_HEADER;
	}
	else
	{
		unsigned int bytes_to_move = conn->client_stream_pos - conn->parse_pos;

		--(conn->kacount);

#if WEB_OPTIONS_DEBUG_DATA
		ECPWS_DEBUG_INFO("CONN%d, bytes_to_move: %d(%d-%d)\n", conn->index, bytes_to_move, conn->client_stream_pos, conn->parse_pos);
#endif
		if (bytes_to_move)
		{
			memmove(conn->client_stream, conn->client_stream + conn->parse_pos, bytes_to_move);
		}

		conn->client_stream_pos = bytes_to_move;
		conn->status = WEB_CLIENT_S_READ_HEADER;
	}

	/* we want to clear a middle part of the request:*/
#if WEB_OPTIONS_DEBUG_DATA
	ECPWS_DEBUG_INFO( "CONN%d, offset: %d\n", conn->index, bytes_to_zero);
#endif
	memset(conn, 0, bytes_to_zero);

	conn->header_line = conn->client_stream;
}

/* Waiting for next request in the same connection */
void freeOneRequest(SVR_WEB *webSvr, WEB_CLIENT_CONN *conn)
{/* freeOneRequest should *never* get called by anything but process_requests */
	int i;

	if (conn->buffer_end )//&& conn->status != WEB_CLIENT_S_ERROR)
	{
		//  WARN("request sent to freeOneRequest before WEB_CLIENT_S_READ_HEADER.");
		conn->status = WEB_CLIENT_S_READ_HEADER;

		/* THIS IS THE SAME CODE EXECUTED BY THE 'WEB_CLIENT_S_DONE' SECTION
		* of process_requests. It must be exactly the same!
		*/
		i = req_flush(conn);
		
		/* retval can be -2=error, -1=blocked, or bytes left */
		if (i == -2)
		{/* error */
			conn->status = WEB_CLIENT_S_ERROR;
		}
		else if (i > 0)
		{
			return;
		}
	}
	

	if (conn->logline)/* access log */
	{
		log_access(conn);
		conn->logline = NULL;
	}

	if (conn->mmapEntry)
	{
		release_mmap(conn->mmapEntry);
	}

	/* in WIN32 conn->data_mem is point to the conn->mmapEntry->mmap, which is freed in release_mmap() */
#ifndef	WIN32
	else if (conn->data_mem)
	{
		munmap(conn->data_mem, conn->filesize);
	}
#endif

#if 0
	/* debug data_fd: how is it assigned value */
	if (conn->data_fd >1)
		CLOSE(conn->data_fd);
#endif

	if (conn->post_data_fd > 1)
		CLOSE(conn->post_data_fd);

	if (conn->response_status >= 400)
		webSvr->statistics.errors++;

	for (i = COMMON_CGI_COUNT; i < conn->cgi_env_index; ++i)
	{
		if (conn->cgi_env[i])
		{
			free(conn->cgi_env[i]);
		}
		else
		{
			ECPWS_LOG_INFO("Warning: CGI Environment contains NULL value (index %d of %d).\n", i, conn->cgi_env_index);
		}
	}

	if (conn->pathname)
		free(conn->pathname);
	if (conn->path_info)
		free(conn->path_info);
	if (conn->path_translated)
		free(conn->path_translated);
	if (conn->script_name)
		free(conn->script_name);
	if (conn->host)
		free(conn->host);
	if (conn->ranges)
		ranges_reset(conn);

	conn->client_stream_pos = 0;
	conn->parse_pos = 0;
	if (conn->status != WEB_CLIENT_S_ERROR && (conn->keepalive == KA_ACTIVE) && (conn->response_status < 500) && conn->kacount > 0)
	{
		_sanitize_request(conn, 0);

		webSvr->statistics.requests++;
	}

	return;
}

/*
 * Obtains a request struct off the free list, or if the free list is empty, allocates memory
 * Return value: pointer to initialized request
 */
WEB_CLIENT_CONN *new_request(SVR_WEB *webSvr)
{
	WEB_CLIENT_CONN *conn;

	if( !QUEUE_FIRST_FREE(webSvr))
	{
		return NULL;
	}

	conn = QUEUE_FIRST_FREE(webSvr);/* first on free list */
	QUEUE_DE_FREE(webSvr, conn); /* dequeue the head */
	_sanitize_request(conn, 1);

	return conn;
}


/* Removes a WEB_CLIENT_CONN from its current queue */
static void _dequeue(WEB_CLIENT_CONN **head, WEB_CLIENT_CONN *conn)
{
	if (*head == conn)
		*head = conn->next;

	if (conn->prev)
		conn->prev->next = conn->next;
	if (conn->next)
		conn->next->prev = conn->prev;

	conn->next = NULL;
	conn->prev = NULL;
}

/* Adds a request to the head of a queue */
static void _enqueue(WEB_CLIENT_CONN **head, WEB_CLIENT_CONN *conn)
{
	if (*head)
		(*head)->prev = conn;    /* previous head's prev is us */

	conn->next = *head;          /* our next is previous head */
	conn->prev = NULL;           /* first in list */

	*head = conn;                /* now we are head */
}

void freeAllRequests(SVR_WEB *webSvr)
{
	WEB_CLIENT_CONN *ptr, *next;

	ptr = webSvr->queues.freeQueue;
	while (ptr != NULL)
	{
		next = ptr->next;
		free(ptr);
		ptr = next;
	}
	webSvr->queues.freeQueue = NULL;
}

static int	_initQueue(SVR_WEB *webSvr)
{
	WEB_CLIENT_CONN *conn;
	int i;

	webSvr->popHandler = _dequeue;
	webSvr->pushHandler = _enqueue;

	for(i=0; i < webSvr->cfg.maxConnections; i++)
	{
		conn = (WEB_CLIENT_CONN *) malloc(sizeof (WEB_CLIENT_CONN));
		if (!conn)
		{
			ECPWS_LOG_INFO("malloc for new request");
			return -1;
		}
		memset(conn, 0, sizeof(WEB_CLIENT_CONN));
		conn->index = i;
		conn->webSvr = webSvr;

		QUEUE_EN_FREE(webSvr, conn);
	}

	return 0;
}



static void drop_privs(SVR_WEB *webSvr)
{
#ifndef	WIN32
	/* give away our privs if we can */
	if (getuid() == 0)
	{
		struct passwd *passwdbuf;
		passwdbuf = getpwuid(webSvr->cfg.server_uid);
		if (passwdbuf == NULL)
		{
			ECPWS_FATAL("getpwuid");
		}
		
		if (initgroups(passwdbuf->pw_name, passwdbuf->pw_gid) == -1)
		{
			ECPWS_FATAL("initgroups");
		}
		
		if (setgid(webSvr->cfg.server_gid) == -1)
		{
			ECPWS_FATAL("setgid");
		}
		if (setuid(webSvr->cfg.server_uid) == -1)
		{
			ECPWS_FATAL("setuid");
		}
		/* test for failed-but-return-was-successful setuid
		* http://www.securityportal.com/list-archive/bugtraq/2000/Jun/0101.html
		*/
		if (webSvr->cfg.server_uid != 0 && setuid(0) != -1)
		{
			ECPWS_FATAL("icky Linux kernel bug!");
		}
	}
	else
	{
		if (webSvr->cfg.server_gid || webSvr->cfg.server_uid)
		{
			ECPWS_LOG_INFO("Warning: Not running as root: no attempt to change"
				" to uid %d gid %d\n", webSvr->cfg.server_uid, webSvr->cfg.server_gid);
		}
		webSvr->cfg.server_gid = getgid();
		webSvr->cfg.server_uid = getuid();
	}
#endif
}

/* Makes sure the server root is valid. */
static void _change2ServerRoot(SVR_WEB *webSvr)
{

	if (!webSvr->cfg.server_root) 
	{
#ifdef SERVER_ROOT
		webSvr->cfg.server_root = STRDUP(SERVER_ROOT);
		if (!webSvr->cfg.server_root) 
		{
			ECPWS_FATAL("strdup (SERVER_ROOT)");
		}
#else
		ECPWS_FATAL("webSvr: don't know where server root is.  Please #define SERVER_ROOT in muxWeb.h\n"
	              "and recompile, or use the -c command line option to specify it.\n");
#endif
	}

	if (CHDIR(webSvr->cfg.server_root) == -1) 
	{
		ECPWS_FATAL( "Could not chdir to \"%s\": aborting\n",  webSvr->cfg.server_root);
	}

}



SVR_WEB	 _webSvr;

static int _initWebService(CmnThread *th, void *data)
{
	th->data = data;
	return EXIT_SUCCESS;
}

static void	_destoryWebService(struct _CmnThread *th)
{
 #ifdef WIN32
	WSAUnhookBlockingHook();
	if (WSACleanup() == SOCKET_ERROR)
	{
		ECPWS_FATAL("Failure to cleanup Winsock");
	}
#endif

}


static int	_muxSvrEventHandler(struct _CmnThread *th, void *_event)
{
	SVR_WEB  *webSvr = (SVR_WEB *)th->data;
	int ret;
	MUX_DEBUG("WEB handling JSON/API event");

	{/* jSonEvent, API event from controller. jsonEvent is freed after reply it automatically */
		CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)_event;
		MUX_DEBUG("WEB handling JSON/API event");
		ret = cmnMuxJSonPluginHandle(webSvr, muxWebJsonHandlers, jsonEvent);

		ret = ret;
	}
#if 0	
	if (SYSTEM_IS_EXITING())
	{
		MUX_PLAY_WARN( "'%s' Thread recv EXIT signal\n", th->name );
		return -EXIT_FAILURE;
	}
#endif
	return EXIT_SUCCESS;
}

int svrWebGetPort(void)
{
	SVR_WEB  *webSvr = &_webSvr;
	return webSvr->cfg.server_port;
}



CmnThread  threadWeb =
{
	name		:	"WebService",
		
	init			:	_initWebService,
	mainLoop		:	svrWebMainLoop,
	eventHandler	:	_muxSvrEventHandler,
	destory		:	_destoryWebService,
	
	data			:	NULL,
};

static int _muxWebReportEvent(struct _MuxPlugin *plugin, CMN_PLAY_JSON_EVENT *jEvent)
{
	MUX_DEBUG("WEB receiving JSON/API event");
	return cmnThreadAddEvent(&threadWeb, jEvent);
}


#ifdef	PLUGIN_SUPPORT
int init(MuxPlugIn *plugin)
#else
int main(int argc, char **argv)
 #endif
{
#define	SET_RUN_STATUS(_status)	(webSvr->status=(_status) )

	SVR_WEB  *webSvr = &_webSvr;
	memset(webSvr, 0, sizeof(SVR_WEB));

#ifdef	PLUGIN_SUPPORT
	plugin->priv = webSvr;
	plugin->type = MUX_PLUGIN_TYPE_WEB;
	plugin->signalExit = NULL;
	plugin->reportEvent = _muxWebReportEvent;
	plugin->destory = NULL;
	snprintf(plugin->name, sizeof(plugin->name), "%s", CMN_MODULE_WEB_NAME );
	snprintf(plugin->version, sizeof(plugin->version), "%s", CMN_VERSION_INFO(CMN_MODULE_WEB_NAME) );
	webSvr->priv = plugin->muxMain;
#else
#endif

	time(&webSvr->current_time);

#ifdef WIN32
	/* winsock must be inited for gethostname */
	WSADATA data;
	if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
	{
		ECPWS_FATAL("Failure to start winsock");
	}
#endif

	SVR_WEB_PARSE_CONFIG(webSvr);

	
	SET_RUN_STATUS(WEB_STATUS_INIT_LOG);
	ECPWS_LOG_INFO("init Logging....!\n");
	webSvrOpenLogs(webSvr);

	SET_RUN_STATUS(WEB_STATUS_INIT_SOCKET);
	
	_change2ServerRoot(webSvr);
	/* add float point support */
	//ECPWS_LOG_INFO("%2f init other parameters....\n", 1.1);

	createCgiCommonEnvs(webSvr);
	build_needs_escape();

//	init_signals();
	_initQueue(webSvr);
	
	ECPWS_LOG_INFO("init network parameters....\n");
	webSvrServiceSocketInit(webSvr);

	webSvr->start_time = webSvr->current_time;
	drop_privs(webSvr);

	SET_RUN_STATUS(WEB_STATUS_INITED);

	ECPWS_LOG_INFO(SERVER_NAME" : server version %s\n", SERVER_VERSION);
	ECPWS_LOG_INFO(SERVER_NAME" : server built " __DATE__ " at " __TIME__ ".\n");
	ECPWS_LOG_INFO(SERVER_NAME" : starting server pid=%d, port %d\n", (int) GETPID(), webSvr->cfg.server_port);


#ifdef	PLUGIN_SUPPORT
	plugin->muxMain->initThread(plugin->muxMain, &threadWeb, webSvr);
#else
	cmnThreadInit( &threadWeb, webSvr);

	cmnThreadJoin(&threadWeb);
#endif

	return 0;
}


