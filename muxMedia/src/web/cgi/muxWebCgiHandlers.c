/*
 *  cgi_header.c - cgi header parsing and control
 */

#include "muxWeb.h"

/* 
* returns 0 -=> error or HEAD, close down.
* returns 1 -=> done processing
* leaves conn->cgi_status as WRITE
*/

/*
 The server MUST also resolve any conflicts between header fields returned by
 the script and header fields that it would otherwise send itself.
 ...
 At least one CGI-Field MUST be supplied, but no CGI field name may be used
 more than once in a response. If a body is supplied, then a
 "Content-type" header field MUST be supplied by the script,
 otherwise the script MUST send a "Location" or "Status" header
 field. If a Location CGI-Field is returned, then the script
 MUST NOT supply any HTTP-Fields.
 */

/* TODO:
 We still need to cycle through the data before the end of the headers,
 line-by-line, and check for any problems with the CGI
 outputting overriding http responses, etc...
 */

/* send out normal HTTP header directly here, or status/location response by error handler */
static int _process_cgi_header(WEB_CLIENT_CONN *conn)
{
	char *buf;
	char *c;

	if (conn->cgi_status != CGI_DONE)
		conn->cgi_status = CGI_BUFFER;

	buf = conn->header_line;

	c = strstr(buf, "\n\r\n");/* lookup the splitter between header and html body content */
	if (c == NULL)
	{
		c = strstr(buf, "\n\n");
		if (c == NULL)
		{
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "cgi_header: '%s' unable to find LFLF\n", buf);
			cmnHexDump((unsigned char *)buf, 16);
			return WEB_CLIENT_S_ERROR;
		}
	}
	
	if (conn->http_version == WEB_HTTP09)
	{
		if (*(c + 1) == '\r')
			conn->header_line = c + 2;
		else
			conn->header_line = c + 1;

		return WEB_CLIENT_S_CONTINUE;
	}
	
	if (!STRNCASECMP(buf, "Status: ", 8))
	{
#if WEB_OPTIONS_DEBUG_CGI
		ECPWS_DEBUG_INFO("- found Status \"%s\"\n", buf);
#endif
		conn->header_line--;
		memcpy(conn->header_line, "HTTP/1.0 ", 9);
	}
	else if (!STRNCASECMP(buf, "Location: ", 10))
	{ /* location header : redirect to other location */
#if WEB_OPTIONS_DEBUG_CGI
		ECPWS_DEBUG_INFO("- found Location header \"%s\"\n", buf + 10);
#endif
		if (buf[10] == '/')
		{/* virtual path */
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "server does not support internal redirection: %s\"\n", buf + 10);

			/*
			* We (I, Jon) have declined to support absolute-path parsing
			* because I see it as a major security hole.
			* Location: /etc/passwd or Location: /etc/shadow is not funny.
			*
			* Also, the below code is borked.
			* request_uri could contain /cgi-bin/bob/extra_path
			*/

			/*
			STRCPY(conn->request_uri, sizeof(conn->request_uri), buf + 10);
			return internal_redirect(conn);
			*/
		}
		else
		{/* URL */
			char *c2;
			c2 = strchr(buf + 10, '\n');
			/* c2 cannot ever equal NULL here because we already have found one */

			--c2;
			while (*c2 == '\r')
				--c2;
			++c2;
			/* c2 now points to a '\r' or the '\n' */
			*c2++ = '\0';       /* end header */

			/* first next header, or is at conn->header_end */
			while ((*c2 == '\n' || *c2 == '\r') && c2 < conn->header_end)
				++c2;
			
			if (c2 == conn->header_end)
//				send_r_moved_temp(conn, buf + 10, "");
				conn->response_status = WEB_RES_MOVED_TEMP;
			else
				conn->response_status = WEB_RES_MOVED_TEMP;
//				send_r_moved_temp(conn, buf + 10, c2);

			SPRINTF(conn->request_uri, sizeof(conn->request_uri), buf + 10);

		}

		return WEB_CLIENT_S_ERROR;
	}
	else
	{ /* not location and not status */
		char *dest;
		unsigned int howmuch;

		send_r_request_ok(conn); /* does not terminate */
		
		/* got to do special things because
		a) we have a single buffer divided into 2 pieces
		b) we need to merge those pieces
		Easiest way is to memmove the cgi data backward until
		it touches the buffered data, then reset the cgi data pointers
		*/
		dest = conn->buffer + conn->buffer_end;
		if (conn->method == WEB_M_HEAD)
		{
			if (*(c + 1) == '\r')
				conn->header_end = c + 2;
			else
				conn->header_end = c + 1;
			conn->cgi_status = CGI_DONE;
		}
		
		howmuch = conn->header_end - conn->header_line;

		if (dest + howmuch > conn->buffer + WEB_BUFFER_SIZE)
		{
			/* big problem */
			conn->buffer_start = conn->buffer_end = 0;
			/* reset buffer pointers because we already called send_r_request_ok... */
			WEB_CONN_ERROR(conn, WEB_RES_ERROR);
			ECPWS_LOG_REQ_NOERR(conn, "Too much data to move! Aborting! %s %d\n",__FILE__, __LINE__);
			return WEB_CLIENT_S_ERROR;
		}

#if WEB_OPTIONS_DEBUG_CGI
		ECPWS_DEBUG_INFO("CONN%d: copied %d bytes into offset of %d\n", conn->index, howmuch, dest - conn->buffer);
#endif
		memmove(dest, conn->header_line, howmuch);
		conn->buffer_end += howmuch;
		conn->header_line = conn->buffer + conn->buffer_end;
		conn->header_end = conn->header_line;
		req_flush(conn);
		
		if (conn->method == WEB_M_HEAD)
			return WEB_CLIENT_S_ERROR;

		return WEB_CLIENT_S_CGI_WRITE;
	}
	
	return WEB_CLIENT_S_CGI_WRITE;
}


/*
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: EOF or error, close it down
 *   1: successful read, recycle in ready queue
 */
WEB_CLIENT_STATUS ecpwsCgiHandlerReadPipe(WEB_CLIENT_CONN *conn)
{
	int bytes_read; /* signed */
	unsigned int bytes_to_read; /* unsigned */

	bytes_to_read = WEB_BUFFER_SIZE - (conn->header_end - conn->buffer - 1);
#if WEB_OPTIONS_DEBUG_CGI
	ECPWS_DEBUG_INFO("CONN%d: buf:%p;header_end:%p(offset %d bytes), HeaderLine:%p(offset %d bytes), bytes_to_read:%d\n", conn->index, conn->buffer, 
			conn->header_end, conn->header_end-conn->buffer, conn->header_line, conn->header_line-conn->buffer, bytes_to_read );
#endif

	if(bytes_to_read< 0)
	{
		CMN_ABORT("Read length is invalidate : %d", bytes_to_read );
	}

	if (bytes_to_read == 0)
	{   /* buffer full */
		if (conn->cgi_status == CGI_PARSE)
		{ /* got+parsed header */
			conn->cgi_status = CGI_BUFFER;
			*conn->header_end = '\0'; /* points to end of read data */
			/* Could the above statement overwrite data???
			No, because conn->header_end points to where new data
			should begin, not where old data is.
			*/
			return _process_cgi_header(conn); /* cgi_status will change */
		}
		return WEB_CLIENT_S_CGI_WRITE;
	}

	bytes_read = READ(conn->data_fd, conn->header_end, bytes_to_read);
//	bytes_read = READ(conn->data_fd, buf, 12400);
	if (bytes_read > 0)
	{
		*(conn->header_end + bytes_read) = '\0';
#if WEB_OPTIONS_DEBUG_CGI
		ECPWS_DEBUG_INFO("CONN %d:Pipe - read %d bytes: \"%s\"\n",conn->index, bytes_read, conn->header_end);
#endif
	}
	else if(bytes_read ==0 )
	{
//		ECPWS_DEBUG_INFO("CONN %d:Pipe -read %d bytes\n",conn->index, bytes_read);
			return WEB_CLIENT_S_CONTINUE;
	}
#if WEB_OPTIONS_DEBUG_CGI
	ECPWS_DEBUG_INFO( "client status:%s, cgi_status:%d\n", ecpwsStateName(conn->status), conn->cgi_status);
#endif
	else //if (bytes_read == -1)
	{
		if (errno == EINTR)
		{
			usleep(10); 
			return WEB_CLIENT_S_CONTINUE;
		}
		else if (errno == EWOULDBLOCK || errno == EAGAIN)
		{
			usleep(10); 
			return WEB_CLIENT_S_CONTINUE;          /* request blocked at the pipe level, but keep going */
		}
		else
		{
			ECPWS_LOG_REQ_ERR(conn, "pipe read on error on fd %d: %s", conn->data_fd, strerror(errno));
			return WEB_CLIENT_S_ERROR;
		}
	}
	
	*(conn->header_end + bytes_read) = '\0';

	if (bytes_read == 0)
	{/* eof, write rest of buffer */
		if (conn->cgi_status == CGI_PARSE)
		{ /* hasn't processed header yet */
			conn->cgi_status = CGI_DONE;
			*conn->header_end = '\0'; /* points to end of read data */
			return _process_cgi_header(conn); /* cgi_status will change */
		}
		conn->cgi_status = CGI_DONE;
		return WEB_CLIENT_S_CGI_WRITE;
	}

#if WEB_OPTIONS_DEBUG_CGI
	ECPWS_DEBUG_INFO("buf:%p;header_end:%p(offset %d bytes), HeaderLine:%p(offset %d bytes) \n",conn->buffer, 
			conn->header_end, conn->header_end-conn->buffer, conn->header_line, conn->header_line-conn->buffer);
#endif
	conn->header_end += bytes_read;

	if (conn->cgi_status != CGI_PARSE)
	{
		return WEB_CLIENT_S_CGI_WRITE; /* why not try and flush the buffer now? */
	}
	else
	{
		char *c, *buf;

		buf = conn->header_line;

#if WEB_OPTIONS_DEBUG_CGI
		ECPWS_DEBUG_INFO("buf:%p;header_end:%p(offset %d bytes), HeaderLine:%p(offset %d bytes) (%s)\n",conn->buffer, 
			conn->header_end, conn->header_end-conn->buffer, conn->header_line, conn->header_line-conn->buffer, conn->header_line );
#endif

		c = strstr(buf, "\n\r\n");
		if (c == NULL)
		{
			c = strstr(buf, "\n\n");
			if (c == NULL)
			{
				return WEB_CLIENT_S_CGI_WRITE;
			}
		}
		conn->cgi_status = CGI_DONE;
		*conn->header_end = '\0'; /* points to end of read data */
		return _process_cgi_header(conn); /* cgi_status will change */
	}

	return WEB_CLIENT_S_CGI_WRITE;
}


/*
 * Writes data previously read from a pipe
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: EOF or error, close it down
 *   1: successful write, recycle in ready queue
 */
WEB_CLIENT_STATUS ecpwsCgiHandlerSendout(WEB_CLIENT_CONN * conn)
{
	int bytes_written;
	unsigned int bytes_to_write;

//	conn->header_line = conn->buffer+WEB_BUFFER_SIZE/2;
	bytes_to_write = conn->header_end - conn->header_line;
	if (bytes_to_write == 0)
	{
		if (conn->cgi_status == CGI_DONE)
		{
#if 0		
			freeOneRequest(conn->webSvr, conn);
			return WEB_CLIENT_S_READ_HEADER;
#else
			/* Because content length is not known before transfer msg body, so socket must be close to notify client the end of transfer */
			return ecpwsHandlerTimeout( conn);
#endif
		}

		conn->header_end = conn->header_line = conn->buffer;
		return WEB_CLIENT_S_CGI_READ;
	}

#if WEB_OPTIONS_DEBUG_CGI
	ECPWS_DEBUG_INFO("CGI:%d bytes='%s'\n",bytes_to_write, conn->header_line);
#endif
	bytes_written = svcWebConnWrite(conn, conn->header_line, bytes_to_write);
	if (bytes_written == -1)
	{
		ECPWS_LOG_REQ_ERR(conn, "pipe write"); /* maybe superfluous */
		WEB_CONN_RES_ERR(conn, WEB_RES_ERROR);
	}

	conn->header_line += bytes_written;
	conn->filepos += bytes_written;
	conn->bytes_written += bytes_written;

	/* if there won't be anything to write next time, switch state */
	if ((unsigned) bytes_written == bytes_to_write)
	{
		conn->header_end = conn->header_line = conn->buffer;
		return WEB_CLIENT_S_CGI_READ;
	}

	return WEB_CLIENT_S_CGI_WRITE;
}

