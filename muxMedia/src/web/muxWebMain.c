/*
 *
 */

#include "muxWeb.h"

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>           /* wait */
#endif

#ifdef	WIN32
#define	ADD_SIGNAL_HANDLE(sa, _signal, handler)	\
		{if( signal(_signal, handler) == SIG_ERR ){printf("Can't set signal %d:%s\n", _signal, strerror( errno) ); exit(1);}}
#else
#define	ADD_SIGNAL_HANDLE(sa, signal, handler)	\
		{sa.sa_handler = handler; sigaction(signal, &sa, NULL); }
#endif

JMP_BUFFER		env;
int handle_sigbus;

int sigterm_flag = 0;           /* lame duck mode */

#ifndef	WIN32
int sighup_flag = 0;            /* 1 => signal has happened, needs attention */
int sigchld_flag = 0;           /* 1 => signal has happened, needs attention */
int sigalrm_flag = 0;           /* 1 => signal has happened, needs attention */

static void _sigbus(int dummy)
{

	if (handle_sigbus)
	{
#if	WITH_JUMP
		longjmp(env, dummy);
#endif
	}
	
	time(&_webSvr.current_time);
	ECPWS_LOG_INFO( "caught SIGBUS, dumping core in %s\n", _webSvr.cfg.tempdir);
	CHDIR(_webSvr.cfg.tempdir);
	abort();
}

static void _sighup(int dummy)
{
	sighup_flag = 1;
}

static void _sigchld(int dummy)
{
	sigchld_flag = 1;
}

static void _sigalrm(int dummy)
{
	sigalrm_flag = 1;
}
#endif

static void _sigsegv(int dummy)
{
//	time(&current_time);
	ECPWS_LOG_INFO( "caught SIGSEGV(Illegal Segment Fault), dumping core in %s\n", _webSvr.cfg.tempdir);
//	CHDIR(cfg->tempdir);
	abort();
}


static void _sigint(int dummy)
{
//	time(&current_time);
	ECPWS_LOG_INFO("caught SIGINT: ignore\n");
	return;
//	CHDIR(cfg->tempdir);
	exit(1);
}


#if 0
static void _sigterm(int dummy)
{
	if (!sigterm_flag)
		sigterm_flag = 1;
	ECPWS_LOG_INFO("caught SIGTERM, skip\n");
}
#endif

/* Sets up signal handlers for all our friends. */
void init_signals(void)
{
#ifndef	WIN32
	struct sigaction sa;

	sa.sa_flags = 0;

	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, SIGSEGV);
	sigaddset(&sa.sa_mask, SIGBUS);
	sigaddset(&sa.sa_mask, SIGTERM);
	sigaddset(&sa.sa_mask, SIGHUP);
	sigaddset(&sa.sa_mask, SIGINT);
	sigaddset(&sa.sa_mask, SIGPIPE);
	sigaddset(&sa.sa_mask, SIGCHLD);
	sigaddset(&sa.sa_mask, SIGALRM);
	sigaddset(&sa.sa_mask, SIGUSR1);
	sigaddset(&sa.sa_mask, SIGUSR2);

	ADD_SIGNAL_HANDLE(sa, SIGBUS, _sigbus);
	ADD_SIGNAL_HANDLE(sa, SIGHUP, _sighup);
	ADD_SIGNAL_HANDLE(sa, SIGCHLD, _sigchld);
	ADD_SIGNAL_HANDLE(sa, SIGALRM, _sigalrm);
	ADD_SIGNAL_HANDLE(sa, SIGPIPE, SIG_IGN);
	ADD_SIGNAL_HANDLE(sa, SIGUSR1, SIG_IGN);
	ADD_SIGNAL_HANDLE(sa, SIGUSR2, SIG_IGN);
#endif

	ADD_SIGNAL_HANDLE(sa, SIGSEGV, _sigsegv);
	ADD_SIGNAL_HANDLE(sa, SIGINT, _sigint);

	/* added this line to terminate boa by 'kill' command, lizhijie,2006.07.25 */
#if 0
	ADD_SIGNAL_HANDLE(sa, SIGTERM, _sigterm);
#endif

}

#ifndef	WIN32
/*followings are user handler(which is called in user context) for these signal */
void sigchld_run(SVR_WEB  *webSvr)
{
	int child_status;
	pid_t pid;

	sigchld_flag = 0;

	while ((pid = waitpid(-1, &child_status, WNOHANG)) > 0)
		if (webSvr->cfg.verbose_cgi_logs)
		{
			time(&webSvr->current_time);
			ECPWS_LOG_INFO("reaping child %d: status %d\n", (int) pid,child_status);
		}
	return;
}

void sighup_run(SVR_WEB  *webSvr)
{
	sighup_flag = 0;
	time(&webSvr->current_time);
	ECPWS_LOG_INFO("caught SIGHUP, restarting\n");


    /* why ? */
    /*
       FD_ZERO(&block_read_fdset);
       FD_ZERO(&block_write_fdset);
     */
    /* clearCgiCommonEnvs(); NEVER DO THIS */
	clearupAllHash();

	cleanupAlias();
	freeAllRequests(webSvr);

	ECPWS_LOG_INFO("re-reading configuration files\n");

	SVR_WEB_PARSE_CONFIG(webSvr);

	ECPWS_LOG_INFO("successful restart\n");
}

void sigalrm_run(SVR_WEB  *webSvr)
{
	time(&webSvr->current_time);
//	ECPWS_LOG_INFO( "%ld requests, %ld errors\n", status.requests, status.errors);
	hash_show_stats();
	sigalrm_flag = 0;
}
#endif

/* program terminate handlers of 2 stages */
void sigterm_stage1_run(SVR_WEB  *webSvr)
{/* lame duck mode */
	time(&webSvr->current_time);
	ECPWS_LOG_INFO("caught SIGTERM, starting shutdown\n");
	sigterm_flag = 2;
}

void sigterm_stage2_run(SVR_WEB  *webSvr)
{ /* lame duck mode */
	ECPWS_LOG_INFO("exiting SVR_WEB normally (uptime %d seconds)\n", (int) (webSvr->current_time - webSvr->start_time));
	CHDIR(webSvr->cfg.tempdir);
	clearCgiCommonEnvs();
	clearupAllHash();

	cleanupAlias();
	freeAllRequests(webSvr);
	range_pool_empty();
	free(webSvr->cfg.server_root);
	free(webSvr->cfg.server_name);
	webSvr->cfg.server_root = NULL;
	exit(0);
}



/* Prepare the ready queue of requests and set FD_SET respectively */
/*
 * Here, we need to do some things:
 *  - keepalive timeouts simply close
 *    (this is special:: a keepalive timeout is a timeout where
       keepalive is active but nothing has been read yet)
 *  - regular timeouts close + error
 *  - stuff in buffer and fd ready?  write it out
 *  - fd ready for other actions?  do them
 */
static void _prepareReadyReqs(SVR_WEB *webSvr)
{
	WEB_CLIENT_CONN *current, *next;

	time(&webSvr->current_time);
	for (current = QUEUE_FIRST_READY(webSvr); current; current = next)
	{
		next = current->next;

		{
			switch (current->status)
			{
				/* read request or read post data */	
				case WEB_CLIENT_S_READ_HEADER:
					ECPWS_CLEAR_WRITE(webSvr, current->socket);
					ECPWS_BLOCK_READ(webSvr, current->socket );
#if WEB_OPTIONS_DEBUG_FSM
					ECPWS_LOG_INFO("CONN %d is in READ Block\n", current->index);
#endif
					break;

				/* CGI read is waiting for CGI output, so it is not blocked.
				so send the socket into writeFd to wake the select */	
				case WEB_CLIENT_S_CGI_READ:
				/* Body write is not block to write data to CGI,
				so send the socket into writeFd to wake the select */	
				case WEB_CLIENT_S_BODY_READ:
				case WEB_CLIENT_S_BODY_WRITE:
				case WEB_CLIENT_S_HTTP_WRITE:
				case WEB_CLIENT_S_CGI_WRITE:
					ECPWS_CLEAR_READ(webSvr, current->socket);
					ECPWS_BLOCK_WRITE(webSvr, current->socket );
#if WEB_OPTIONS_DEBUG_FSM
					ECPWS_LOG_INFO("CONN %d is in WRITE Block\n", current->index);
#endif
					break;
					
				case WEB_CLIENT_S_ERROR:
					ECPWS_CLEAR_READ(webSvr, current->socket);
					ECPWS_BLOCK_WRITE(webSvr, current->socket );
					ECPWS_LOG_INFO("CONN %d is in WRITE Block in ERROR state\n", current->index);
					break;
					
				default:
					ECPWS_LOG_INFO("Error:Not validate status %s\n", ecpwsStateName(current->status));
					break;
			}
		}
		
		current = next;
	}
}

static void _checkStates(SVR_WEB *webSvr)
{
	WEB_CLIENT_CONN *current, *next;

	time(&webSvr->current_time); /* for "new" requests if we've been in select too long */

	for (current = QUEUE_FIRST_READY(webSvr); current; current = next)
	{
		time_t time_since = webSvr->current_time - current->time_last;
		next = current->next;
		if(ECPWS_IS_WRITE(webSvr, current->socket) || ECPWS_IS_READ(webSvr, current->socket) )
//			||(current->status == WEB_CLIENT_S_ERROR) )
		{
			WEB_EMIT_EVENT(current, WEB_EVENT_READY);
		}
		else if (current->kacount < webSvr->cfg.ka_max && /* we *are* in a keepalive */
			(time_since >= webSvr->cfg.ka_timeout) && /* ka timeout */
			!current->logline)  /* haven't read anything yet */
		{
			ECPWS_LOG_INFO( "connection keepalive timed out\n" );
			WEB_EMIT_EVENT(current, WEB_EVENT_TIMEOUT) /* connection keepalive timed out */;
		}
		else if (time_since > REQUEST_TIMEOUT)
		{
			ECPWS_LOG_INFO_WITH_REQ(current, "connection timed out\n" );
			WEB_EMIT_EVENT(current, WEB_EVENT_TIMEOUT);
		}
		
		current = next;
	}
}


int svrWebMainLoop(CmnThread *th)
{
	SVR_WEB  *webSvr = (SVR_WEB *)th->data;
	
	
	FD_ZERO(ECPWS_READ_FDSET(webSvr));
	FD_ZERO(ECPWS_WRITE_FDSET(webSvr));
	webSvr->maxFd = -1;

#ifndef	WIN32
	/* handle signals here */
	if (sighup_flag)
		sighup_run(webSvr);
	if (sigchld_flag)
		sigchld_run(webSvr);
	if (sigalrm_flag)
		sigalrm_run(webSvr);
#endif
	if (sigterm_flag)
	{
		/* sigterm_flag:
		* 1. caught, unprocessed.
		* 2. caught, stage 1 processed
		*/
		if (sigterm_flag == 1)
		{
			sigterm_stage1_run(webSvr);
			ECPWS_CLEAR_READ(webSvr, webSvr->serverSocket );
			CloseSocket(webSvr->serverSocket);
			/* make sure the server isn't in the block list */
			webSvr->serverSocket = -1;
		}
		
		if (sigterm_flag == 2 && !QUEUE_FIRST_READY(webSvr) )
		{
			sigterm_stage2_run(webSvr); /* terminal */
			return -EXIT_FAILURE;
		}
	}
	else
	{
		if (webSvr->totalConnections > webSvr->cfg.maxConnections)
		{/* FIXME: for poll we don't subtract 20. why? */
			ECPWS_CLEAR_READ(webSvr, webSvr->serverSocket );
		}
		else
		{
			ECPWS_BLOCK_READ(webSvr, webSvr->serverSocket ); /* server always set */
		}
	}


	/* move selected conn's from reqBlockQueue to reqReadyQueue */
	_prepareReadyReqs(webSvr);

	/* max_fd is > 0 when something is blocked */
	if (webSvr->maxFd)
	{
		int	res;
		struct timeval req_timeout; /* timeval for select */

		req_timeout.tv_sec = (QUEUE_FIRST_READY(webSvr) ? 0 : (webSvr->cfg.ka_timeout ? webSvr->cfg.ka_timeout : REQUEST_TIMEOUT));
//		req_timeout.tv_sec = REQUEST_TIMEOUT;
		req_timeout.tv_sec = REQUEST_TIMEOUT/30;
		req_timeout.tv_usec = 0l; /* reset timeout */

#if 0
		res = select(webSvr->maxFd + 1, ECPWS_READ_FDSET(webSvr), ECPWS_WRITE_FDSET(webSvr), NULL, 
			((QUEUE_FIRST_READY(webSvr))? &req_timeout : NULL));
#else
		/* make eventHandler can be scheduled at anytime */
		res = select(webSvr->maxFd + 1, ECPWS_READ_FDSET(webSvr), ECPWS_WRITE_FDSET(webSvr), NULL,  &req_timeout);
#endif
		if (res == SOCKET_ERROR )
		{
			/* what is the appropriate thing to do here on EBADF */
			if (errno == EINTR)
			{
			//	break;       /* while(1) */
			}
			else if (errno != EBADF)
			{
				ECPWS_FATAL("select");
			}
		}
		else if(res == 0)/* timeout */
		{
		}
		
		if (!sigterm_flag && ECPWS_IS_READ(webSvr, webSvr->serverSocket) )
		{
			webSvrNewConnection(webSvr );
		}
	}

	_checkStates( webSvr);
	process_requests(webSvr );


	return EXIT_SUCCESS;

}

