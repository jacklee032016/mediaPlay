/*
 * Create a CGI(NPH,CGI), directoryList and gunzip
 * Pipe is only used by CGI.CGI and DirectoryList
 */

#include "muxWeb.h"

/*

*/
#ifndef	WIN32
int _newProcess(WEB_CLIENT_CONN *conn, int pipeFd[2], BOOL_T usePipes)
{
	int child_pid;
	CmnMuxWebConfig *cfg = &conn->webSvr->cfg;
	
	child_pid = fork();
	switch (child_pid)
	{
		case -1:
			/* fork unsuccessful */
			WEB_CONN_ERROR(conn, WEB_RES_ERROR);
			ECPWS_LOG_REQ_ERR(conn, "fork");
			if (usePipes)
			{
				CLOSE(pipeFd[0]);
				CLOSE(pipeFd[1]);
			}
			/* FIXME: There is a problem here. send_r_error would work
			for CGI_T_NPH and CGI, but not for GUNZIP.  Fix that. */
			/* I'd like to send_r_error, but.... */
			return 0;
			break;
		
		case 0:
		/* child */

			if (conn->cgi_type == CGI_T_CGI || conn->cgi_type == CGI_T_NPH)
			{
				char *c;
				unsigned int l;
				char *newpath, *oldpath;

				c = strrchr(conn->pathname, '/');
				if (!c)
				{
					/* there will always be a '.' */
					ECPWS_FATAL_WITH_REQ(conn,  "unable to find '/' in conn->pathname: \"%s\"\n", conn->pathname);
					if (usePipes)
						CLOSE(pipeFd[1]);
				}

				*c = '\0';

				if (CHDIR(conn->pathname) != 0)
				{
					int saved_errno = errno;
					ECPWS_LOG_REQ_ERR(conn, "Could not chdir to \"%s\":", conn->pathname);
					errno = saved_errno;
					if (usePipes)
						CLOSE(pipeFd[1]);
					_exit(1);
				}

				oldpath = conn->pathname;
				conn->pathname = ++c;
				l = strlen(conn->pathname) + 3;
				/* prefix './' */
				newpath = malloc(sizeof (char) * l);
				if (!newpath)
				{
					/* there will always be a '.' */
					ECPWS_LOG_REQ_ERR(conn, "unable to malloc for newpath");
					if (usePipes)
						CLOSE(pipeFd[PIPE_FD_WRITE]);
					_exit(1);
				}
				
				newpath[0] = '.';
				newpath[1] = '/';
				memcpy(&newpath[2], conn->pathname, l - 2); /* includes the trailing '\0' */
				free(oldpath);
				conn->pathname = newpath;
			}
			
			if (usePipes)
			{
				/* close the 'read' end of the pipeFd[] */
				CLOSE(pipeFd[PIPE_FD_READ]);
				/* tie cgi's STDOUT to our write end of pipe */
				if (DUP2(pipeFd[PIPE_FD_WRITE], STDOUT_FILENO) == -1)
				{
					ECPWS_LOG_REQ_ERR(conn, "dup2 - pipeFd");
					_exit(1);
				}
				CLOSE(pipeFd[PIPE_FD_WRITE]);
			}
			else
			{
				/* tie stdout to socket */
				if (DUP2(conn->socket, STDOUT_FILENO) == -1)
				{
					ECPWS_LOG_REQ_ERR(conn, "dup2 - socket");
					_exit(1);
				}
				CloseSocket(conn->socket);
			}
			
			/* Switch socket flags back to blocking */
			if (set_block_fd(STDOUT_FILENO) == -1)
			{
				ECPWS_LOG_REQ_NOERR(conn, "cgi-fcntl");
				_exit(1);
			}
			
			/* tie post_data_fd to POST stdin */
			if (conn->method == WEB_M_POST)
			{ /* tie stdin to file */
				LSEEK(conn->post_data_fd, SEEK_SET, 0);
				DUP2(conn->post_data_fd, STDIN_FILENO);
				CLOSE(conn->post_data_fd);
			}
			
			umask(cfg->cgi_umask);       /* change umask *again* u=rwx,g=rxw,o= */

			/*
			* tie STDERR to _cgi_log_fd
			* _cgi_log_fd will automatically close, close-on-exec rocks!
			* if we don't tied STDERR (current ecpwsLogMsg) to _cgi_log_fd,
			*  then we ought to close it.
			*/
			if (cgiLogFile)
			{
				DUP2(FILENO(cgiLogFile), STDERR_FILENO);
				fclose(cgiLogFile);
			}

			if (conn->cgi_type)
			{
				char *aargv[CGI_ARGC_MAX + 1];
				create_argv(conn, aargv);
				execve(conn->pathname, aargv, conn->cgi_env);
			}
			else
			{
				if (conn->pathname[strlen(conn->pathname) - 1] == '/')
					execl(cfg->dirmaker, cfg->dirmaker, conn->pathname, conn->request_uri, (void *) NULL);
#ifdef GUNZIP
				else
					execl(GUNZIP, GUNZIP, "--stdout", "--decompress",	conn->pathname, (void *) NULL);
#endif
			}
			
			/* execve failed */
			ECPWS_FATAL_WITH_REQ(conn,  "Unable to execve/execl pathname: \"%s\"", conn->pathname);
			break;

		default:
			/* parent */
			/* if here, fork was successful */
			if (cfg->verbose_cgi_logs)
			{
				ECPWS_LOG_INFO( "Forked child \"%s\" pid %d\n", conn->pathname, child_pid);
			}

			if (conn->method == WEB_M_POST)
			{
				CLOSE(conn->post_data_fd); /* child closed it too */
				conn->post_data_fd = 0;
			}

			/* CGI_T_NPH, GUNZIP, etc... all go straight to the fd */
			if (!usePipes)
				return 0;

			CLOSE(pipeFd[PIPE_FD_WRITE]);
			conn->data_fd = pipeFd[PIPE_FD_READ];

			/* change state in FSM */
//			conn->status = WEB_CLIENT_S_CGI_READ;
			if (conn->cgi_type == CGI_T_CGI)
			{
				conn->cgi_status = CGI_PARSE; /* got to parse cgi header */
				/* for cgi_header... I get half the buffer! */
				conn->header_line = conn->header_end = (conn->buffer + WEB_BUFFER_SIZE / 2);
			}
			else
			{
				conn->cgi_status = CGI_BUFFER;
				/* I get all the buffer! */
				conn->header_line = conn->header_end = conn->buffer;
			}

#if WEB_OPTIONS_DEBUG_CGI
			ECPWS_DEBUG_INFO("CONN%d: buf:%p;header_end:%p(offset %d bytes), HeaderLine:%p(offset %d bytes) \n", conn->index, conn->buffer, 
				conn->header_end, conn->header_end-conn->buffer, conn->header_line, conn->header_line-conn->buffer);
#endif
			/* reset conn->filepos for logging (it's used in pipe.c) */
			/* still don't know why conn->filesize might be reset though */
			conn->filepos = 0;
			break;
	}

	/* return new state, otherwise ecpwsHttpHandlerReadHeader() will re-handle the process. Jack Lee, Oct.19,2017  */
//	return conn->status = WEB_CLIENT_S_CGI_READ;
	return WEB_CLIENT_S_CGI_READ;
	
}
#else

#define	EXEC(cmd, env, si, pi)	\
	CreateProcess( NULL, (cmd), NULL, NULL, FALSE_T, 0, (env), NULL, (si), (pi) )


int _newProcess(WEB_CLIENT_CONN *conn, int pipeFd[2], BOOL_T usePipes)
{
	int oldStdout = -1;
	char	cmdLine[MAX_PATH_LENGTH];
	STARTUPINFO si;
	
	ZeroMemory( &si, sizeof(si) );  
	si.cb = sizeof(si);  
	ZeroMemory( &conn->pi, sizeof(conn->pi) ); 
	
	memset(cmdLine, 0, sizeof(cmdLine));

	if (conn->cgi_type == CGI_T_CGI || conn->cgi_type == CGI_T_NPH)
	{

		ECPWS_DEBUG_INFO("finding '%c' in conn->pathname: \"%s\"\n", DIR_SPLITTER_CHAR,conn->pathname);
#if 0
		char *c;
		unsigned int l;
		char *newpath, *oldpath;
		c = strrchr(conn->pathname, DIR_SPLITTER_CHAR);
		if (!c)
		{/* there will always be a '.' */
			if (usePipes)
				CLOSE(pipeFd[1]);
			ECPWS_LOG_INFO(conn,  "unable to find '%c' in conn->pathname: \"%s\"\n", DIR_SPLITTER_CHAR, conn->pathname);
			conn->response_status = WEB_RES_ERROR;
			return WEB_CLIENT_S_READ_HEADER;
		}

		*c = '\0';

		if (CHDIR(conn->pathname) == -1)
		{
			if (usePipes)
				CLOSE(pipeFd[1]);
			ECPWS_FATAL_WITH_REQ(conn, "Could not chdir to \"%s\":", conn->pathname);
		}

		ECPWS_DEBUG_INFO("New working directory:'%s'\r\n", conn->pathname);

		oldpath = conn->pathname;
		conn->pathname = ++c;
		l = strlen(conn->pathname) + 3;
		/* prefix './' */
		newpath = malloc(sizeof (char) * l);
		if (!newpath)
		{/* there will always be a '.' */
			ECPWS_LOG_REQ_ERR(conn, "unable to malloc for newpath");
			if (usePipes)
				CLOSE(pipeFd[PIPE_FD_WRITE]);
			_exit(1);
		}
		
		newpath[0] = '.';
		newpath[1] = DIR_SPLITTER_CHAR;
		memcpy(&newpath[2], conn->pathname, l - 2); /* includes the trailing '\0' */
		free(oldpath);
		conn->pathname = newpath;
#endif

	}
	
	if (usePipes)
	{
		oldStdout = _dup(STDOUT_FILENO);
		/* tie cgi's STDOUT to our write end of pipe */
		if (DUP2(pipeFd[PIPE_FD_WRITE], STDOUT_FILENO) == -1)
		{
			ECPWS_LOG_REQ_ERR(conn, "dup2 - pipeFd");
			_exit(1);
		}
		CLOSE(pipeFd[PIPE_FD_WRITE]);
	}
	else
	{/* NPH and gunzip */
		/* tie stdout to socket */
		if (DUP2(conn->socket, STDOUT_FILENO) == -1)
		{
			ECPWS_LOG_REQ_ERR(conn, "dup2 - socket");
			_exit(1);
		}
		CloseSocket(conn->socket);
	}
	
	/* Switch socket flags back to blocking */
	if (set_block_fd(STDOUT_FILENO) == -1)
	{
		ECPWS_LOG_REQ_NOERR(conn, "cgi-fcntl");
		_exit(1);
	}
	
	/* tie post_data_fd to POST stdin */
	if (conn->method == WEB_M_POST)
	{ /* tie stdin to file */
		LSEEK(conn->post_data_fd, SEEK_SET, 0);
		DUP2(conn->post_data_fd, STDIN_FILENO);
		CLOSE(conn->post_data_fd);
	}

#ifndef	WIN32	
	umask(cfg->cgi_umask);       /* change umask *again* u=rwx,g=rxw,o= */
#endif

	/*
	* tie STDERR to _cgi_log_fd
	* _cgi_log_fd will automatically close, close-on-exec rocks!
	* if we don't tied STDERR (current ecpwsLogMsg) to _cgi_log_fd,
	*  then we ought to close it.
	*/
	if (cgiLogFile)
	{
		ECPWS_DEBUG_INFO("set log file for CGI program\n");
		if(DUP2(FILENO(cgiLogFile), STDERR_FILENO)==-1)
		{
			ECPWS_FATAL("Open log for CGI program\n");
		}
#if 0
		/* open for next CGI access */
		fclose(cgiLogFile);
		cgiLogFile = NULL;
#endif		
	}

	if (conn->cgi_type)
	{
		char *aargv[CGI_ARGC_MAX + 1];
		char	envs[WEB_BUFFER_SIZE];
		int index = 1; /* first arg is executable name */
		int position = 0;
		create_argv(conn, aargv);
		
//		execve(conn->pathname, aargv, conn->cgi_env);
		SPRINTF(cmdLine, sizeof(cmdLine), "%s ", conn->pathname);
		while(aargv[index] != NULL)
		{
			SPRINTF(cmdLine+ strlen(cmdLine), sizeof(cmdLine)-strlen(cmdLine), "%s ", aargv[index]);
			index++;
		}
		
		index = 0;
		memset(envs, 0, sizeof(envs));
		while(conn->cgi_env[index]!=NULL)
		{
			memcpy(envs+position, conn->cgi_env[index], strlen(conn->cgi_env[index]));
			position += strlen(conn->cgi_env[index])+1;
			index++;
		}
		
		 if( !EXEC(cmdLine, ((LPVOID)envs), &si, &conn->pi) )
		 {
		 	ECPWS_FATAL( "'%s' with ENV and argument running failure\r\n", cmdLine);
		 }

	}
	else
	{
		if (conn->pathname[strlen(conn->pathname) - 1] == '/')
		{
//			execl(cfg->dirmaker, cfg->dirmaker, conn->pathname, conn->request_uri, (void *) NULL);
			SPRINTF(cmdLine, sizeof(cmdLine), "%s %s %s", cfg->dirmaker,  conn->pathname, conn->request_uri);
		}
#ifdef GUNZIP
		else
		{
//			execl(GUNZIP, GUNZIP, "--stdout", "--decompress",	conn->pathname, (void *) NULL);
			SPRINTF(cmdLine, sizeof(cmdLine), "gunzip --stdout --decompress %s", conn->pathname);
		}
#endif
		if( !EXEC(cmdLine, NULL, &si, &conn->pi) )
		 {
		 	ECPWS_FATAL( "%s running failure\r\n", cmdLine);
		 }
			
	}
	
	ECPWS_DEBUG_INFO("CGI '%s' running OK\n", cmdLine);
	if (usePipes)
	{
		if (DUP2(STDOUT_FILENO, oldStdout) == -1)
		{
			ECPWS_LOG_REQ_ERR(conn, "dup2 - stdout");
			_exit(1);
		}
		CLOSE(oldStdout);
	}
		
	/* if here, fork was successful */
	if (cfg->verbose_cgi_logs)
	{
		ECPWS_LOG_INFO( "Forked child \"%s\" pid %d( tid %d)\n", cmdLine, conn->pi.dwProcessId, conn->pi.dwThreadId );
	}

	if (conn->method == WEB_M_POST)
	{
//		CLOSE(conn->post_data_fd); /* child closed it too */
		conn->post_data_fd = 0;
	}

	/* CGI_T_NPH, GUNZIP, etc... all go straight to the fd */
	if (!usePipes)
		return WEB_CLIENT_S_CGI_READ;

//	CLOSE(pipeFd[PIPE_FD_WRITE]);
	conn->data_fd = pipeFd[PIPE_FD_READ];

	if (conn->cgi_type == CGI_T_CGI)
	{
		conn->cgi_status = CGI_PARSE; /* got to parse cgi header */
		/* for cgi_header... I get half the buffer! */
		conn->header_line = conn->header_end = (conn->buffer + WEB_BUFFER_SIZE / 2);
	}
	else
	{
		conn->cgi_status = CGI_BUFFER;
		/* I get all the buffer! */
		conn->header_line = conn->header_end = conn->buffer;
	}

	ECPWS_DEBUG_INFO("buf:%p;header_end:%p(offset %d bytes), HeaderLine:%p(offset %d bytes) \n",conn->buffer, 
		conn->header_end, conn->header_end-conn->buffer, conn->header_line, conn->header_line-conn->buffer);
	/* reset conn->filepos for logging (it's used in pipe.c) */
	/* still don't know why conn->filesize might be reset though */
	conn->filepos = 0;

	return WEB_CLIENT_S_CGI_READ;
}
#endif


/*
 * Description: Called for GET/POST requests that refer to ScriptAlias
 * directories or application/x-httpd-cgi files.  Ties stdout to socket,
 * stdin to data if POST, and execs CGI.
 * stderr remains tied to our log file; is this good?
 *
 * Returns:
 * 0 - error or CGI_T_NPH, either way the socket is closed
 * 1 - success
 */

WEB_CLIENT_STATUS ecpwsCgiCreate(WEB_CLIENT_CONN *conn)
{
	BOOL_T	usePipes = FALSE_T;
	int		pipeFds[2];
	
	SQUASH_KA(conn);

	if (conn->cgi_type)
	{
		int ret = complete_env(conn);
		if ( ret == 0 ||ret == WEB_CLIENT_S_ERROR )
		{
			WEB_CONN_ERROR(conn, WEB_RES_BAD_GATEWAY);
			return WEB_CLIENT_S_ERROR;
		}
	}
#ifdef FASCIST_LOGGING
	{
		int i;
		for (i = 0; i < conn->cgi_env_index; ++i)
			ECPWS_DEBUG_INFO(" - environment variable for cgi: \"%s\"\n", conn->cgi_env[i]);
	}
#endif

	/* we want to use pipeFd whenever it's a CGI or directory (using directory list program ) */
	/* otherwise (CGI_T_NPH, gunzip) we want no pipeFd */
	if (conn->cgi_type == CGI_T_CGI ||(!conn->cgi_type &&(conn->pathname[strlen(conn->pathname) - 1] == '/')))
	{
		usePipes = TRUE_T;
		if (PIPE(pipeFds) != 0 )// ==-1)
		{
			ECPWS_LOG_REQ_ERR(conn, "pipe");
			ECPWS_FATAL( "pipe: %s", strerror(errno) );
			return WEB_CLIENT_S_ERROR;
		}

#if WEB_OPTIONS_DEBUG_CGI
		ECPWS_DEBUG_INFO(" - pipeFds: %d:%d\n", pipeFds[0], pipeFds[1] );
#endif

		/* set the read end of the socket to non-blocking */
		if (set_nonblock_fd(pipeFds[0]) != 0) //= -1)
		{
			ECPWS_LOG_REQ_NOERR(conn, "cgi-fcntl");
			CLOSE(pipeFds[0]);
			CLOSE(pipeFds[1]);
			ECPWS_FATAL( "cgi-fcntl: %s", strerror(errno) );
			return WEB_CLIENT_S_ERROR;
		}
	}

	return _newProcess(conn, pipeFds, usePipes);
}


