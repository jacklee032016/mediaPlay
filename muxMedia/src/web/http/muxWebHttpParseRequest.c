/*
 *
 */

#include "muxWeb.h"
/*
 *  This is called with the first conn->header_line received
 * by a request, called "logline" because it is logged to a file.
 * It is parsed to determine 'request type' and 'method', then passed to 'translate_uri' and 'header_host' for further parsing.  
 * Also sets up CGI environment if needed.
 * The first line of the received content, METHOD, VERSION, etc. The first step is parsing log line, then parsing the option lines
 */
static int _process_logline(WEB_CLIENT_CONN * conn)
{
	char *stop, *stop2;

	conn->logline = conn->client_stream;

#if WEB_OPTIONS_DEBUG_DATA			
	ECPWS_DEBUG_INFO("- Parsing Logline: : %s", conn->logline);
#endif
	if (strlen(conn->logline) < 5)
	{/* minimum length conn'd. */
		ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "Request too short: \"%s\"\n", conn->logline);
		return WEB_CLIENT_S_ERROR;
	}

	if (!memcmp(conn->logline, "GET ", 4))
		conn->method = WEB_M_GET;
	else if (!memcmp(conn->logline, "HEAD ", 5))
		/* head is just get w/no body */
		conn->method = WEB_M_HEAD;
	else if (!memcmp(conn->logline, "POST ", 5))
		conn->method = WEB_M_POST;
	else if (!memcmp(conn->logline, "DELETE ", 7))
		conn->method = WEB_M_DELETE;
	else
	{
		WEB_CONN_ERROR(conn, WEB_RES_NOT_IMP);
		ECPWS_LOG_REQ_NOERR(conn, "malformed request: \"%s\"\n", conn->logline);
		return 0;
	}

	conn->http_version = WEB_HTTP10;

	/* Guaranteed to find ' ' since we matched a method above */
	stop = conn->logline + 3;
	if (*stop != ' ')
		++stop;

	/* scan to start of non-whitespace */
	while (*(++stop) == ' ');

	stop2 = stop;

	/* scan to end of non-whitespace */
	while (*stop2 != '\0' && *stop2 != ' ')
		++stop2;

	if (stop2 - stop > MAX_HEADER_LENGTH)
	{
		ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "URI too long %d: \"%s\"\n", MAX_HEADER_LENGTH, conn->logline);
		return WEB_CLIENT_S_ERROR;
	}

	/* check for absolute URL */
	if (!memcmp(stop, SERVER_METHOD, strlen(SERVER_METHOD)) && !memcmp(stop + strlen(SERVER_METHOD), "://", 3))
	{
		char *host;

		/* we have an absolute URL */
		/* advance STOP until first '/' after host */
		stop += strlen(SERVER_METHOD) + 3;
		host = stop;
		/* if *host is '/' there is no host in the URI
		* if *host is ' ' there is corruption in the URI
		* if *host is '\0' there is nothing after http://
		*/
		if (*host == '/' || *host == ' ' || *host == '\0')
		{
			/* nothing *at all* after http:// */
			/* no host in absolute URI */
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn,"bogus absolute URI\n");
			/* we don't need to log conn->logline, because ecpwsLogErrorReq does */
			return WEB_CLIENT_S_ERROR;
		}

		/* OK.  The 'host' is at least 1 char long.
		* advance to '/', or end of host+url (' ' or ''\0')
		*/
		while(*stop != '\0' && *stop != '/' && *stop != ' ')
			++stop;

		if (*stop != '/')
		{ /* *stop is '\0' or ' ' */
			/* host is valid, but there is no URL. */
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "no URL in absolute URI: \"%s\"\n", conn->logline);
			return WEB_CLIENT_S_ERROR;
		}

		/* we have http://X/ where X is not ' ' or '/' (or '\0') */
		/* since stop2 stops on '\0' and ' ', it *must* be after stop */
		/* still, a safety check (belts and suspenders) */
		if (stop2 < stop)
		{/* Corruption in absolute URI */
			/* This prevents a DoS attack from format string attacks */
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "Error: corruption in absolute URI:\"%s\".  This should not happen.\n", conn->logline);
			return WEB_CLIENT_S_ERROR;
		}

		/* copy the URI */
		memcpy(conn->request_uri, stop, stop2 - stop);
		/* place a NIL in the file spot to terminate host */
		*stop = '\0';
		/* place host */
		/* according to RFC2616 --
		1. If Request-URI is an absoluteURI, the host is part of the
		Request-URI. Any Host header field value in the request MUST 	be ignored.

		Since we ignore any Host header if conn->host is already set, well, we rock!*/
		conn->header_host = host; /* this includes the port! (if any) */
	}
	else
	{/* copy the URI */
		memcpy(conn->request_uri, stop, stop2 - stop);
	}

	conn->request_uri[stop2 - stop] = '\0';

	/* METHOD URL\0 */
	if (*stop2 == '\0')
		conn->http_version = WEB_HTTP09;
	else if (*stop2 == ' ')
	{
		/* if found, we should get an HTTP/x.x */
		unsigned int p1, p2;

		/* scan to end of whitespace */
		++stop2;
		while (*stop2 == ' ' && *stop2 != '\0')
			++stop2;

		if (*stop2 == '\0')
		{
			conn->http_version = WEB_HTTP09;
		}
		else
		{
			/* scan in HTTP/major.minor */
			if (SSCANF(stop2, "HTTP/%u.%u", &p1, &p2) == 2)
			{
				/* HTTP/{0.9,1.0,1.1} */
				if (p1 == 0 && p2 == 9)
				{
					conn->http_version = WEB_HTTP09;
				}
				else if (p1 == 1 && p2 == 0)
				{
					conn->http_version = WEB_HTTP10;
				}
				else if (p1 == 1 && p2 == 1)
				{
					conn->http_version = WEB_HTTP11;
					conn->keepalive = KA_ACTIVE; /* enable keepalive */
					/* Disable send_r_continue because some clients
					* *still* don't work with it, Python 2.2 being one
					* see bug 227361 at the sourceforge web site.
					* fixed in revision 1.52 of httplib.py, dated
					* 2002-06-28 (perhaps Python 2.3 will
					* contain the fix.)
					*/
					/* send_r_continue(conn); */
				}
				else
				{
					goto BAD_VERSION;
				}
			}
			else
			{
				goto BAD_VERSION;
			}
		}
	}

	if (conn->method == WEB_M_HEAD && conn->http_version == WEB_HTTP09)
	{
		ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "method is HEAD but version is HTTP/0.9");
		return WEB_CLIENT_S_ERROR;
	}
	conn->cgi_env_index = COMMON_CGI_COUNT;

	return WEB_CLIENT_S_CONTINUE;

BAD_VERSION:
	ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "bogus HTTP version: \"%s\"\n", stop2);
	return WEB_CLIENT_S_ERROR;
}

#ifdef ACCEPT_ON
/*
 * Adds a mime_type to a requests accept char buffer
 *   silently ignore any that don't fit - shouldn't happen because of relative buffer sizes
 */
void add_accept_header(WEB_CLIENT_CONN *conn, const char *mime_type)
{
	int l = strlen(conn->accept);
	int l2 = strlen(mime_type);

	if ((l + l2 + 2) >= MAX_HEADER_LENGTH)
		return;

	if (conn->accept[0] == '\0') {
		memcpy(conn->accept, mime_type, l2 + 1);
	}
	else
	{
		conn->accept[l] = ',';
		conn->accept[l + 1] = ' ';
		memcpy(conn->accept + l + 2, mime_type, l2 + 1);
		/* the +1 is for the '\0' */
	}
}
#endif

/* Parses the contents of conn->header_line and takes appropriate action. 
Option Header: after first line, all header is in the form of 'key:value'
*/
static int _process_option_line(WEB_CLIENT_CONN * conn)
{
	char c, *value, *line = conn->header_line;

	/* Start by aggressively hacking the in-place copy of the header line */
#if WEB_OPTIONS_DEBUG_DATA			
	ECPWS_DEBUG_INFO("- Parsing \"%s\"\n", line);
#endif

	value = strchr(line, ':');
	if (value == NULL)
	{
		WEB_CONN_ERROR(conn,WEB_RES_BAD_REQUEST);
		ECPWS_LOG_REQ_ERR(conn, "header \"%s\" does not contain ':'\n", line);
		return WEB_CLIENT_S_ERROR;
	}
	
	*value++ = '\0';            /* overwrite the : */
	to_upper(line);             /* header types are case-insensitive */

	/* the code below *does* catch '\0' due to the c = *value test */
	while ((c = *value) && (c == ' ' || c == '\t'))
		value++;

	/* if c == '\0' there was no 'value' for the key */
	if (c == '\0')
	{
		return WEB_CLIENT_S_ERROR;
	}

	switch (line[0])
	{
		case 'A':
			if (!memcmp(line, "ACCEPT", 7))
			{
#ifdef ACCEPT_ON
				add_accept_header(conn, value);
#endif
				return WEB_CLIENT_S_CONTINUE;
			}
			if (!memcmp(line, "AUTHORIZATION", 13))
			{
				if(www_base_authen(conn, value) == WWW_AUTHEN_SUCCESS)
				{
					conn->authen_status = REQUEST_AUTHENED;
					return WEB_CLIENT_S_CONTINUE;
				}
				conn->authen_status = REQUEST_AUTHEN_FAILED;
				WEB_CONN_RES_ERR(conn, WEB_RES_UNAUTHORIZED);
			}
			break;
		
		case 'C':
			if (!memcmp(line, "CONTENT_TYPE", 13) && !conn->content_type)
			{
				conn->content_type = value;
				return WEB_CLIENT_S_CONTINUE;
			}
			else if (!memcmp(line, "CONTENT_LENGTH", 15) && !conn->content_length)
			{
				conn->content_length = value;
				return WEB_CLIENT_S_CONTINUE;
			}
			else if (!memcmp(line, "CONNECTION", 11) && conn->webSvr->cfg.ka_max && conn->keepalive != KA_STOPPED)
			{
				conn->keepalive = (!STRNCASECMP(value, "Keep-Alive", 10) ? KA_ACTIVE : KA_STOPPED);
				return WEB_CLIENT_S_CONTINUE;
			}
			if (!memcmp(line, "COOKIE", 6) )
			{
				ECPWS_LOG_INFO("Cookie :%s\n", value);
				conn->cookies = value;
				return WEB_CLIENT_S_CONTINUE;
			}
			break;
		
		case 'H':
			if (!memcmp(line, "HOST", 5) && !conn->header_host)
			{
				conn->header_host = value; /* may be complete garbage! */
				return WEB_CLIENT_S_CONTINUE;
			}
			break;
		
		case 'I':
			if (!memcmp(line, "IF_MODIFIED_SINCE", 18) && !conn->if_modified_since)
			{
				ECPWS_LOG_INFO("IF_MODIFIED_SINCE :%s\n", value);
				conn->if_modified_since = value;
				return WEB_CLIENT_S_CONTINUE;
			}
			break;
			
		case 'R':
			/* Need agent and referer for logs */
			if (!memcmp(line, "REFERER", 8))
			{
				conn->header_referer = value;
				if (!add_cgi_env(conn, "REFERER", value, 1))
				{/* errors already logged */
					return WEB_CLIENT_S_ERROR;
				}
			}
			else if (!memcmp(line, "RANGE", 6))
			{
				if (conn->ranges && conn->ranges->stop == INT_MAX)
				{/* there was an error parsing, ignore */
					return WEB_CLIENT_S_CONTINUE;
				}
				else if (!range_parse(conn, value))
				{/* unable to parse range */
					conn->response_status = WEB_RES_INVALID_RANGE;
					return WEB_CLIENT_S_ERROR;
				}                   /* conn->ranges */
			}
			break;
		
		case 'U':
			if (!memcmp(line, "USER_AGENT", 11))
			{
				conn->header_user_agent = value;
				if (!add_cgi_env(conn, "USER_AGENT", value, 1))
				{/* errors already logged */
					return WEB_CLIENT_S_ERROR;
				}
				return WEB_CLIENT_S_CONTINUE;
			}
			break;
		
		default:
			/* no default */
			break;
	}/* switch */

	return add_cgi_env(conn, line, value, 1);
}

/*
 * Description: takes a request and performs some final checking before ecpwsCgiCreate or ecpwsHttpPrepareStaticResponse
 * Returns 0 for error or CGI_T_NPH, or 1 for success
 */
int process_header_end(WEB_CLIENT_CONN *conn)
{
	if (!conn->logline)
	{
		/* when accessed by telnet, no other content more than logline */
		if(_process_logline(conn)== WEB_CLIENT_S_ERROR)
			return WEB_CLIENT_S_ERROR;
	}

	if(ecpwsHttpAuthen(conn)==WEB_CLIENT_S_ERROR)
		return WEB_CLIENT_S_ERROR;

	/* Percent-decode request */
	if (unescape_uri(conn->request_uri, &(conn->query_string)) == 0)
	{
		ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "URI contains bogus characters\n");
		return WEB_CLIENT_S_ERROR;
	}
	
	ECPWS_LOG_INFO("conn->request:'%s'; query string:'%s'\n", conn->request_uri, conn->query_string);

	/* clean pathname */
	clean_pathname(conn->request_uri);

	if (conn->request_uri[0] != '/')
	{
		ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "URI does not begin with '/'(%s)\n", conn->request_uri);
		return WEB_CLIENT_S_ERROR;
	}

	if (conn->webSvr->cfg.vhost_root)
	{
		char *c;
		if (!conn->header_host)
		{
			conn->host = STRDUP(conn->webSvr->cfg.default_vhost);
		}
		else
		{
			conn->host = STRDUP(conn->header_host);
		}
		
		if (!conn->host)
		{
			WEB_CONN_ERROR(conn, WEB_RES_ERROR);
			ECPWS_LOG_REQ_NOERR(conn, "unable to strdup default_vhost/conn->header_host\n");
			return WEB_CLIENT_S_ERROR;
		}
		
		strlower(conn->host);
		/* check for port, and remove
		* we essentially ignore the port, because we cannot
		* as yet report a different port than the one we are
		* listening on
		*/
		c = strchr(conn->host, ':');
		if (c)
			*c = '\0';

		if (check_host(conn->host) < 1)
		{
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "host invalid!\n");
			return WEB_CLIENT_S_ERROR;
		}
	}

	if (translate_uri(conn) == WEB_CLIENT_S_ERROR)
	{ /* unescape, parse uri */
		/* errors already logged */
		SQUASH_KA(conn);
		return WEB_CLIENT_S_ERROR;               /* failure, close down */
	}

	if (conn->method == WEB_M_POST)
	{
		conn->post_data_fd = create_temporary_file(0, NULL, 0, conn->webSvr->cfg.tempdir);
		if (conn->post_data_fd == 0)
		{/* errors already logged */
			WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
		}

#ifndef	WIN32		
		if (fcntl(conn->post_data_fd, F_SETFD, 1) == -1)
		{
			ECPWS_LOG_REQ_NOERR(conn, "unable to set close-on-exec for conn->post_data_fd!\n" );
			CLOSE(conn->post_data_fd);
			conn->post_data_fd = 0;
			WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
		}
#endif		
		/* for body_{read,write}, set header_line to start of data,	and header_end to end of data */
//		conn->header_line = check;
//		conn->header_end = conn->client_stream + conn->client_stream_pos;

		/* have to write first, or read will be confused
		* because of the special case where the
		* filesize is less than we have already read.
		*/

		/*
		As quoted from RFC1945:
		A valid Content-Length is required on all HTTP/1.0 POST requests. An
		HTTP/1.0 server should respond with a 400 (bad request) message if it
		cannot determine the length of the request message's content.
		*/

		if (conn->content_length)
		{
			int content_length;

			content_length = web_atoi(conn->content_length);
			/* Is a content-length of 0 legal? */
			if (content_length < 0)
			{
				ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "Invalid Content-Length [%s] on POST!\n", conn->content_length);
				return WEB_CLIENT_S_ERROR;
			}
					
			if (conn->webSvr->cfg.single_post_limit && content_length > conn->webSvr->cfg.single_post_limit)
			{
				ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "Content-Length [%d] > SinglePostLimit [%d] on POST!\n", content_length, conn->webSvr->cfg.single_post_limit);
				return WEB_CLIENT_S_ERROR;
			}
					
			conn->filesize = content_length;
			conn->filepos = 0;
#if 0			
			if ( (conn->header_end - conn->header_line) > conn->filesize)
			{
				conn->header_end = conn->header_line + conn->filesize;
			}
			if(conn->header_end == conn->header_line+conn->filesize)
			{
				ECPWS_LOG_INFO("CONN%d: POST data of %d bytes has been RX:\"%s\"\nHeader_end\"%s\"\n", 
					conn->index, conn->filesize, conn->header_line, conn->header_end );
				return WEB_CLIENT_S_BODY_WRITE;
			}
			else
			{
				ECPWS_LOG_INFO("CONN%d: POST data of %d bytes has not been RX:%d\n", conn->index, conn->filesize, conn->header_end-conn->header_line);
			}
#endif
				return WEB_CLIENT_S_BODY_READ;
		}
		else
		{
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "Unknown Content-Length POST!\n");
			return WEB_CLIENT_S_ERROR;
		}
	
	}
	
	if (conn->cgi_type)
	{
		return ecpwsCgiCreate(conn);
	}

	return WEB_CLIENT_S_HTTP_WRITE;
}

WEB_CLIENT_STATUS ecpwsHttpParseRequestLine(WEB_CLIENT_CONN *conn)
{
	WEB_CLIENT_STATUS newState = WEB_CLIENT_S_HTTP_WRITE;
	
	/* terminate string that begins at conn->header_line */
	if (conn->logline)
	{
		return _process_option_line(conn);
	}
	else
	{
		if (_process_logline(conn) == WEB_CLIENT_S_ERROR)
		{
			return WEB_CLIENT_S_ERROR;
		}
		
		if (conn->http_version == WEB_HTTP09)
		{
			newState = process_header_end(conn);
		}
	}

	return newState;
}

