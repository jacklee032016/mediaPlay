/*
 * response for the GET method
 */

#include "muxWeb.h"

#define	PROCESS_END	-1			/* error, handle ended */

/* I think that permanent redirections (301) are supposed
 * to be absolute URIs, but they can be troublesome.
 * change to 1 to allow much simpler redirections.
 */
/* #define ALLOW_LOCAL_REDIRECT */


/*
 * Description: Called from _get_cachedir_file if a directory html has to be generated on the fly
 * returns -1 for problem, else 0
 * This version is the fastest, ugliest, and most accurate yet.
 * It solves the "stale size or type" problem by not ever giving
 * the size or type.  This also speeds it up since no per-file stat() is required.
 */
static int _index_directory(WEB_CLIENT_CONN *conn, char *dest_filename)
{
	DIR *request_dir;
	FILE *fdstream;
	struct dirent *dirbuf;
	int bytes = 0;
	char *escname = NULL;

	if (CHDIR(conn->pathname) == -1)
	{
		if (errno == EACCES || errno == EPERM)
		{
			WEB_CONN_RES_ERR(conn, WEB_RES_FORBIDDEN);
		}
		else
		{
			ECPWS_LOG_REQ_ERR(conn, "chdir in '%s'", conn->pathname);
			WEB_CONN_RES_ERR(conn,WEB_RES_BAD_REQUEST);
		}
	}

	request_dir = opendir(".");
	if (request_dir == NULL)
	{
		ECPWS_LOG_REQ_ERR(conn, "opendir in directory \"%s\": ", conn->pathname);
		WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
	}

	fdstream = fopen(dest_filename, "w");
	if (fdstream == NULL)
	{
		closedir(request_dir);
		ECPWS_LOG_REQ_ERR(conn, "dircache fopen \"%s\"", dest_filename );
		WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
	}

	bytes += fprintf(fdstream, "<HTML><HEAD>\n<TITLE>Index of %s</TITLE>\n</HEAD>\n\n",conn->request_uri);
	bytes += fprintf(fdstream, "<BODY>\n\n<H2>Index of %s</H2>\n\n<PRE>\n", conn->request_uri);

	while ((dirbuf = readdir(request_dir)))
	{
		if (!strcmp(dirbuf->d_name, "."))
			continue;

		if (!strcmp(dirbuf->d_name, ".."))
		{
			bytes += fprintf(fdstream, " [DIR] <A HREF=\"../\">Parent Directory</A>\n");
			continue;
		}

		if ((escname = escape_string(dirbuf->d_name, NULL)) != NULL)
		{
			bytes += fprintf(fdstream, " <A HREF=\"%s/%s\">%s</A>\n", conn->request_uri, escname, dirbuf->d_name);
			free(escname);
			escname = NULL;
		}
	}
	
	closedir(request_dir);
	bytes += fprintf(fdstream, "</PRE>\n\n</BODY>\n</HTML>\n");

	fclose(fdstream);

	CHDIR(conn->webSvr->cfg.server_root);

	conn->filesize = bytes;      /* for logging transfer size */
	return OK; /* success */
}


/* return fd and update statbuf for the cached html of directory */
static int _get_cachedir_file(WEB_CLIENT_CONN * conn, struct stat *statbuf)
{
	char pathname_with_index[MAX_PATH_LENGTH];
	int data_fd;
	time_t real_dir_mtime;

	real_dir_mtime = statbuf->st_mtime;
	/* the sizeof() doesn't need a -1 because snprintf will
	* include the NUL when calculating if the size is enough
	*/
	SNPRINTF(pathname_with_index, sizeof(pathname_with_index), "%s%cdir.%d.%ld", conn->webSvr->cfg.cachedir, DIR_SPLITTER_CHAR, (int) statbuf->st_dev, statbuf->st_ino);
	data_fd = OPEN(pathname_with_index, O_RDONLY);
	STRCPY(conn->openedFile, sizeof(conn->openedFile), pathname_with_index);
	if (data_fd != -1)
	{/* index cache */
		fstat(data_fd, statbuf);
		if (statbuf->st_mtime > real_dir_mtime)
		{
			statbuf->st_mtime = real_dir_mtime; /* lie */
			STRCPY(conn->request_uri, sizeof(conn->request_uri), conn->webSvr->cfg.directory_index); /* for mimetype */
			return data_fd;
		}
		
		CLOSE(data_fd);
		UNLINK(pathname_with_index); /* cache is stale, delete it */
	}
	
	if (_index_directory(conn, pathname_with_index) == WEB_CLIENT_S_ERROR)
		return WEB_CLIENT_S_ERROR;

	data_fd = OPEN(pathname_with_index, O_RDONLY); /* Last chance */
	STRCPY(conn->openedFile, sizeof(conn->openedFile),pathname_with_index);
	if (data_fd != -1)
	{
		STRCPY(conn->request_uri,  sizeof(conn->request_uri), conn->webSvr->cfg.directory_index); /* for mimetype */
		fstat(data_fd, statbuf);
		statbuf->st_mtime = real_dir_mtime; /* lie */
		return data_fd;
	}

	ECPWS_LOG_REQ_ERR(conn, "re-opening dircache %s", pathname_with_index);
	WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
}


/*
 * Description: Called from below if the request is a directory.
 * statbuf must describe directory on input, since we may need its device, inode, and mtime.
 * statbuf is updated, since we may need to check mtimes of a cache.
 * returns:
 *  -1 error
 *  0  cgi (either gunzip or auto-generated)
 *  >0  file descriptor of file
 */
static int _get_dir(WEB_CLIENT_CONN *conn, struct stat *statbuf)
{
	char pathname_with_index[MAX_PATH_LENGTH];
	int data_fd;

	if (conn->webSvr->cfg.directory_index)
	{ /* look for index.html first?? */
		unsigned int l1, l2;

		l1 = strlen(conn->pathname);
		l2 = strlen(conn->webSvr->cfg.directory_index);
#ifdef GUNZIP
		if (l1 + l2 + 3 + 1 > sizeof(pathname_with_index))
#else
		if (l1 + l2 + 1 > sizeof(pathname_with_index))
#endif
		{
			errno = ENOMEM;
			ECPWS_LOG_REQ_NOERR(conn, "pathname_with_index not large enough for pathname + index");
			WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
		}
#if 0
		memcpy(pathname_with_index, conn->pathname, l1); /* doesn't copy NUL */
		memcpy(pathname_with_index + l1, cfg->directory_index, l2 + 1); /* does */
#else		
		SPRINTF(pathname_with_index, sizeof(pathname_with_index), "%s%c%s", conn->pathname,DIR_SPLITTER_CHAR, conn->webSvr->cfg.directory_index);
#endif
		ECPWS_DEBUG_INFO("doc is '%s'\r\n", pathname_with_index);

		data_fd = OPEN(pathname_with_index, O_RDONLY);
		if (data_fd != -1)
		{/* user's index file */
			/* We have to assume that directory_index will fit, because
			* if it doesn't, well, that's a huge configuation problem.
			* this is only the 'index.html' pathname for mime type
			*/
			memcpy(conn->request_uri, conn->webSvr->cfg.directory_index, l2 + 1); /* for mimetype */
			fstat(data_fd, statbuf);
			
			STRCPY(conn->openedFile, sizeof(conn->openedFile), pathname_with_index);
			return data_fd;
		}
		
		if (errno == EACCES)
		{
			WEB_CONN_RES_ERR(conn,WEB_RES_FORBIDDEN);
		}
		else if (errno != ENOENT)
		{
			/* if there is an error *other* than EACCES or ENOENT */
			WEB_CONN_RES_ERR(conn,WEB_RES_NOT_FOUND);
		}
		
#ifdef GUNZIP
		/* if we are here, trying index.html didn't work try index.html.gz */
		strcat(pathname_with_index, ".gz");
		data_fd = OPEN(pathname_with_index, O_RDONLY);
		if (data_fd != -1)
		{    /* user's index file */
			CLOSE(data_fd);

			conn->response_status = WEB_RES_REQUEST_OK;
			SQUASH_KA(conn);
			if (conn->pathname)
				free(conn->pathname);
			conn->pathname = STRDUP(pathname_with_index);			
			if (!conn->pathname)
			{
				ECPWS_LOG_REQ_ERR(conn, "strdup");
				WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
			}
			
			if (conn->http_version != WEB_HTTP09)
			{
				HTTP_HEADER_CONTENT_MODIFY(conn, " 200 OK-GUNZIP\r\n", get_mime_type(conn->webSvr->cfg.directory_index), getHeaderLastModified(conn) );
				req_flush(conn);
			}
			
			if (conn->method == WEB_M_HEAD)
			{
				WEB_CONN_RES_ERR(conn,WEB_RES_METHOD_NA);
			}
			return ecpwsCgiCreate(conn);
		}
#endif
	}

	/* only here if index.html, index.html.gz don't exist */
	if (conn->webSvr->cfg.dirmaker != NULL)
	{ /* don't look for index.html... maybe automake? */
		conn->response_status = WEB_RES_REQUEST_OK;
		SQUASH_KA(conn);

		/* the indexer should take care of all headers */
		if (conn->http_version != WEB_HTTP09)
		{
			HTTP_HEADER_CONTENT_MODIFY(conn, "200 OK", HTML, getHeaderLastModified(conn));
			req_flush(conn);
		}
		
		if (conn->method == WEB_M_HEAD)
		{
			WEB_CONN_RES_ERR(conn,WEB_RES_METHOD_NA);
		}

		return ecpwsCgiCreate(conn);
	}
	else if (conn->webSvr->cfg.cachedir)
	{
		return _get_cachedir_file(conn, statbuf);
	}
	else
	{/* neither index.html nor autogenerate are allowed */
		WEB_CONN_RES_ERR(conn,WEB_RES_FORBIDDEN);
	}
}

/*
 * Initializes a non-script GET or HEAD request.
 * Return values:
 *   0: finished or error, request will be freed
 *   1: successfully initialized, added to ready queue
 */
WEB_CLIENT_STATUS ecpwsHttpPrepareStaticResponse(WEB_CLIENT_CONN *conn)
{
	int data_fd, saved_errno, isDir;
	struct stat statbuf;
	volatile unsigned int bytes_free;

//		conn->pathname[strlen(conn->pathname)-1] = '\0';
	data_fd = OPEN(conn->pathname, O_RDONLY);
	STRCPY(conn->openedFile, sizeof(conn->openedFile), conn->pathname);
	saved_errno = errno;        /* might not get used */

#ifdef GUNZIP
	if (data_fd == -1 && errno == ENOENT)
	{
		/* cannot open */
		/* it's either a gunzipped file or a directory */
		char gzip_pathname[MAX_PATH_LENGTH];
		unsigned int len;

		len = strlen(conn->pathname);

		if (len + 4 > sizeof(gzip_pathname))
		{
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "Pathname + .gz too long! (%s)\n", conn->pathname);
			WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
		}

		memcpy(gzip_pathname, conn->pathname, len);
		memcpy(gzip_pathname + len, ".gz", 3);
		gzip_pathname[len + 3] = '\0';
		data_fd = OPEN(gzip_pathname, O_RDONLY);
		if (data_fd != -1)
		{
			CLOSE(data_fd);

			conn->response_status = WEB_RES_REQUEST_OK;
			if (conn->pathname)
				free(conn->pathname);
			
			conn->pathname = STRDUP(gzip_pathname);
			if (!conn->pathname)
			{
				ECPWS_LOG_REQ_ERR(conn, "strdup");
				WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
			}

			STRCPY(conn->openedFile, sizeof(conn->openedFile), conn->pathname);
			
			if (conn->http_version != WEB_HTTP09)
			{
				HTTP_HEADER_CONTENT_MODIFY(conn, "200 OK-GUNZIP", getHeaderContentType(conn), getHeaderLastModified(conn));
				req_flush(conn);
			}
			
			if (conn->method == WEB_M_HEAD)
			{
				WEB_CONN_RES_ERR(conn,WEB_RES_METHOD_NA);
			}

			return ecpwsCgiCreate(conn);
		}
	}
#endif


#ifdef ACCESS_CONTROL
	if (!access_allow(conn->pathname))
	{
		WEB_CONN_RES_ERR(conn,WEB_RES_FORBIDDEN);
	}
#endif

	if(data_fd == -1)
	{
//		conn->pathname[strlen(conn->pathname)-1] = '\0';
#ifdef	WIN32	
		isDir = _stat(conn->pathname, &statbuf);
#else
		isDir = stat(conn->pathname, &statbuf);
#endif
		ECPWS_DEBUG_INFO("%s is not a regular file, is %s a directoty\n", conn->pathname, (isDir == -1)?"not":"");
		if(isDir == -1)
		{
			ECPWS_ERR_INFO("%s is not a directory:", conn->pathname);
		}
	}
	else
		fstat(data_fd, &statbuf);

	if (data_fd == -1 && isDir == -1 )
	{
		errno = saved_errno;
		ECPWS_LOG_REQ_ERR(conn, "document open");

		if (saved_errno == ENOENT)
			conn->response_status = WEB_RES_NOT_FOUND;
		else if (saved_errno == EACCES)
			conn->response_status = WEB_RES_FORBIDDEN;
		else
			conn->response_status = WEB_RES_BAD_REQUEST;
		return WEB_CLIENT_S_ERROR;
	}
	
	if (S_ISDIR(statbuf.st_mode))
	{ /* directory */
		if(data_fd != -1)
		{
			CLOSE(data_fd);/* close dir */
		}

//		ECPWS_DEBUG_INFO("conn->pathname: '%s'\n", conn->pathname );

#ifndef	WIN32	
		if (conn->pathname[strlen(conn->pathname) - 1] != DIR_SPLITTER_CHAR)
//		if (conn->pathname[strlen(conn->pathname) ] != DIR_SPLITTER_CHAR)
		{
			char buffer[3 * MAX_PATH_LENGTH + 128];
			unsigned int len;

#ifdef ALLOW_LOCAL_REDIRECT
			len = strlen(conn->request_uri);
			if (len + 2 > sizeof(buffer))
			{
				WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
			}
			
			memcpy(buffer, conn->request_uri, len);
			buffer[len] = DIR_SPLITTER_CHAR;
			buffer[len+1] = '\0';
#else
			char *host = conn->webSvr->cfg.server_name;
			unsigned int l2;
			char *port = NULL;
			const char *prefix = "http://";
			static unsigned int l3 = 0;
			static unsigned int l4 = 0;

			if (l4 == 0)
			{
				l4 = strlen(prefix);
			}
			
			len = strlen(conn->request_uri);
			if (!port && conn->webSvr->cfg.server_port != 80)
			{
				port = STRDUP(simple_itoa(conn->webSvr->cfg.server_port));
				if (port == NULL)
				{
					errno = ENOMEM;
					ECPWS_LOG_REQ_ERR(conn, "Unable to perform simple_itoa conversion on server port!");
					WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
				}
				l3 = strlen(port);
			}
			/* l3 and l4 are done */

			if (conn->host)
			{
				/* only shows up in vhost mode */
				/* what about the port? (in vhost_mode?) */
				/* we don't currently report ports that differ
				* from out "bound" (listening) port, so we don't care
				*/
				host = conn->host;
			}
			l2 = strlen(host);

			if (conn->webSvr->cfg.server_port != 80)
			{
				if (l4 + l2 + 1 + l3 + len + 1 > sizeof(buffer))
				{
					errno = ENOMEM;
					ECPWS_LOG_REQ_ERR(conn, "buffer not large enough for directory redirect");
					WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
				}
				
				memcpy(buffer, prefix, l4);
				memcpy(buffer + l4, host, l2);
				buffer[l4 + l2] = ':';
				memcpy(buffer + l4 + l2 + 1, port, l3);
				memcpy(buffer + l4 + l2 + 1 + l3, conn->request_uri, len);
				buffer[l4 + l2 + 1 + l3 + len] = DIR_SPLITTER_CHAR;
				buffer[l4 + l2 + 1 + l3 + len + 1] = '\0';
			}
			else
			{
				if (l4 + l2 + len + 1 > sizeof(buffer))
				{
					errno = ENOMEM;
					ECPWS_LOG_REQ_ERR(conn,"buffer not large enough for directory redirect");
					WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
				}
				
				memcpy(buffer, prefix, l4);
				memcpy(buffer + l4, host, l2);
				memcpy(buffer + l4 + l2, conn->request_uri, len);
				buffer[l4 + l2 + len] = DIR_SPLITTER_CHAR;
				buffer[l4 + l2 + len + 1] = '\0';
			}
#endif /* ALLOW LOCAL ALIAS_T_REDIRECT */

			WEB_CONN_RES_ERR(conn,WEB_RES_MOVED_PERM);
		}
#endif	/* WIN32 */

		data_fd = _get_dir(conn, &statbuf); /* updates statbuf */
		if (data_fd < 0)
		{
			WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
		}
		else if (data_fd == 0 || data_fd == 1)
			return data_fd;
		/* else, data_fd contains the fd of the file... */
	}

	if (!S_ISREG(statbuf.st_mode))
	{ /* regular file */
		ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "Resulting file is not a regular file.\n");
		if(data_fd != -1)
			CLOSE(data_fd);
		WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
	}

	if (conn->if_modified_since && !modified_since(&(statbuf.st_mtime), conn->if_modified_since))
	{
		CLOSE(data_fd);
		WEB_CONN_RES_ERR(conn,WEB_RES_NOT_MODIFIED);
	}

	conn->filesize = statbuf.st_size;
	conn->last_modified = statbuf.st_mtime;

	/* ignore if-range without range */
	if (conn->header_ifrange && !conn->ranges)
		conn->header_ifrange = NULL;

	/* we don't support it yet */
	conn->header_ifrange = NULL;

	/* parse ranges now */
	/* we have to wait until conn->filesize exists to fix them up */
	/* fixup handles handles communicating with the client */
	/* ranges_fixup logs as appropriate, and sends
	* send_r_invalid_range on error.
	*/

	if (conn->filesize == 0)
	{
		if (conn->http_version < WEB_HTTP11)
		{
			send_r_request_ok(conn);
			CLOSE(data_fd);
			return WEB_CLIENT_S_READ_HEADER;/* have finish the request, so wait for next request */
		}
		send_r_no_content(conn);
		CLOSE(data_fd);
		return WEB_CLIENT_S_READ_HEADER;/* have finish the request, so wait for next request */
	}

	if (conn->ranges && !ranges_fixup(conn))
	{
		CLOSE(data_fd);
		return WEB_CLIENT_S_READ_HEADER;/* have finish the request, so wait for next request */
	}

	/* if no range has been set, use default range */
#if 0
	ECPWS_DEBUG_INFO( "if-range: %s\time_cval: %d\tmtime: %d\n",
	conn->header_ifrange, conn->time_cval, statbuf->st_mtime);
#endif

	/*
	If the entity tag given in the If-Range header matches the current
	entity tag for the entity, then the server should provide the
	specified sub-range of the entity using a 206 (Partial content)
	response.

	If the entity tag does not match, then the server should
	return the entire entity using a 200 (OK) response.
	*/
	/* IF we have range data *and* no if-range or if-range matches... */

#ifdef MAX_FILE_MMAP
	if (conn->filesize > MAX_FILE_MMAP)
	{
		conn->data_fd = data_fd;
		conn->status = WEB_CLIENT_S_IO_SHUFFLE;
	}
	else
#endif
	{
		/* NOTE: I (Jon Nelson) tried performing a read(2)
		* into the output buffer provided the file data would
		* fit, before mmapping, and if successful, writing that
		* and stopping there -- all to avoid the cost
		* of a mmap.  Oddly, it was *slower* in benchmarks.
		*/
		conn->data_fd = data_fd;
		conn->mmapEntry = find_mmap(conn, &statbuf);
		if (conn->mmapEntry == NULL)
		{
			conn->data_fd = data_fd;
			conn->status = WEB_CLIENT_S_IO_SHUFFLE;
		}
		else
		{
			conn->data_mem = conn->mmapEntry->mmap;
			CLOSE(data_fd);             /* close data file */
		}
	}

	if (!conn->ranges)
	{
		conn->ranges = range_pool_pop();
		conn->ranges->start = 0;
		conn->ranges->stop = -1;
		if (!ranges_fixup(conn))
		{
			return 0;
		}
		send_r_request_ok(conn);
	}
	else
	{
		/* FIXME: support if-range header here, by the following logic:
		* if !conn->header_ifrange || st_mtime > header_ifrange,
		*   send_r_partial_content
		* else
		*   reset-ranges, etc...
		*/
		if (!conn->header_ifrange)
		{
			send_r_partial_content(conn);
		}
		else
		{
			/* either no if-range or the if-range does not match */
			ranges_reset(conn);
			conn->ranges = range_pool_pop();
			conn->ranges->start = 0;
			conn->ranges->stop = -1;
			if (!ranges_fixup(conn))
			{
				return 0;
			}
			send_r_request_ok(conn);
		}
		conn->filepos = conn->ranges->start;
	}

	if (conn->method == WEB_M_HEAD)
	{
		return complete_response(conn);
	}

	bytes_free = 0;
	if (conn->data_mem)
	{
		/* things can really go tilt if conn->buffer_end > WEB_BUFFER_SIZE, but basically that can't happen*/

		/* We lose statbuf here, so make sure response has been sent */
		bytes_free = WEB_BUFFER_SIZE - conn->buffer_end;
		/* 256 bytes for the **trailing** headers */

		/* bytes is now how much the buffer can hold after the headers */
	}

	if (0)//conn->data_mem && bytes_free > 256)
	{
		unsigned int want;
		RANGE_T *r;

		r = conn->ranges;

		want = (r->stop - r->start) + 1;

		if (bytes_free > want)
			bytes_free = want;
		else
		{
			/* bytes_free <= want */
			;
		}

#if WITH_JUMP
		if (setjmp(env) == 0)
#else
		if(1)
#endif			
		{
//			handle_sigbus = 1;
			memcpy(conn->buffer + conn->buffer_end, conn->data_mem + r->start, bytes_free);
//			handle_sigbus = 0;
			/* OK, SIGBUS **after** this point is very bad! */
		}
		else
		{
			/* sigbus! */
			RESET_REQ_OUT_BUFFER(conn);
			ECPWS_LOG_REQ_NOERR(conn, "%sGot SIGBUS in memcpy!\n", get_commonlog_time(conn->webSvr));
			WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
		}
		
		conn->buffer_end += bytes_free;
		conn->filepos += bytes_free;
		conn->bytes_written += bytes_free;
		r->start += bytes_free;
		if (bytes_free == want)
		{/* this will fit due to the 256 extra bytes_free */
			return complete_response(conn);
		}
	}

	/* We lose statbuf here, so make sure response has been sent */
	return 1;
}

