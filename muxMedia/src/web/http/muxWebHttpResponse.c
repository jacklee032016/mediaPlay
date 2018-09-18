/*
 *
 */

#include "muxWeb.h"

#define CRLF "\r\n"

const char *getHeaderVerString(WEB_CLIENT_CONN * conn)
{
	switch(conn->http_version)
	{
		case WEB_HTTP09:
			return "HTTP/0.9";
			break;
		case WEB_HTTP10:
			return "HTTP/1.0";
			break;
		case WEB_HTTP11:
			return "HTTP/1.1";
			break;
		default:
			return "HTTP/1.0";
	}
	
	return "Unkonw Version";
}

char *getHeaderContentType(WEB_CLIENT_CONN * conn)
{
	static char	contentType[HTTP_HEADER_LENGTH];
	char * mime_type = get_mime_type(conn->request_uri);

	if (mime_type == NULL)
		return NULL;

	SPRINTF(contentType, sizeof(contentType), "Content-Type: %s", mime_type);
	if (conn->webSvr->cfg.default_charset != NULL && STRNCASECMP( mime_type, "text", 4)==0)
	{/* add default charset */
		SPRINTF(contentType+strlen(contentType), sizeof(contentType) -strlen(contentType), "; charset=%s", conn->webSvr->cfg.default_charset);
	}
	strcat(contentType, "\r\n");

	return contentType;
}


char *getHeaderLastModified(WEB_CLIENT_CONN * conn)
{
	static char lm[] = "Last-Modified: "
	"                             " "\r\n";
	rfc822_time_buf(lm + 15, conn->last_modified);
	return lm;
}


char *print_http_headers(WEB_CLIENT_CONN * conn)
{
	static char	header[HTTP_HEADER_LENGTH];
	char stuff[] = "Date: "
	"                             "
	"\r\nServer: " SERVER_VERSION "\r\n";

	rfc822_time_buf(stuff + 6, conn->webSvr->current_time);
	
	if (conn->kacount > 0 && conn->keepalive == KA_ACTIVE && conn->response_status < 500)
	{
		SPRINTF(header, sizeof(header),  "%sAccept-Ranges: bytes\r\nConnection: Keep-Alive\r\nKeep-Alive: timeout="
			"%s, max=%s\r\n",stuff, simple_itoa(conn->webSvr->cfg.ka_timeout), simple_itoa(conn->kacount) );
	}
	else
	{
		SPRINTF(header, sizeof(header), "%sAccept-Ranges: bytes\r\nConnection: close\r\n", stuff);
	}

	return header;
}

void print_content_range(WEB_CLIENT_CONN * conn)
{
	req_write(conn, "Content-Range: bytes %s-%s/%s\r\n", 
		simple_itoa(conn->ranges->start), simple_itoa(conn->ranges->stop), simple_itoa(conn->filesize));
}

void print_partial_content_continue(WEB_CLIENT_CONN * conn)
{
	static char msg[] = CRLF "--THIS_STRING_SEPARATES" CRLF;

	getHeaderContentType(conn);
	print_content_range(conn);
	req_write(conn, "Content-Length: ");
	req_write(conn, simple_itoa(conn->ranges->stop - conn->ranges->start + 1));
	req_write(conn, "\r\n");
	if (conn->numranges > 1)
	{
		req_write(conn, msg);
	}
}


int complete_response(WEB_CLIENT_CONN *conn)
{
	RANGE_T *r;

	/* we only want to push off the one range */
	r = conn->ranges;
	conn->ranges = conn->ranges->next;
	range_pool_push(r);

	/* successfully flushed */
	if (conn->response_status == WEB_RES_PARTIAL_CONTENT && conn->numranges > 1)
	{
		if (conn->ranges != NULL)
		{
			print_partial_content_continue(conn);
			req_write(conn, CRLF);
		}
		else
		{
			req_write(conn, CRLF "--THIS_STRING_SEPARATES--" CRLF CRLF);
			req_flush(conn);
			return WEB_CLIENT_S_READ_HEADER;
		}
	}
	else
	{
		return WEB_CLIENT_S_READ_HEADER;
	}
	
	return WEB_CLIENT_S_CONTINUE;
}


/* WEB_RES_CONTINUE: 100 */
void send_r_continue(WEB_CLIENT_CONN * conn)
{
	/* shouldn't need this */
	if (conn->http_version != WEB_HTTP11)
		return;
	conn->response_status = WEB_RES_CONTINUE;

	req_write(conn,"%s %s\r\n\r\n", getHeaderVerString(conn), "100 Continue" );
	req_flush(conn); 
}

/* WEB_RES_REQUEST_OK: 200 */
void send_r_request_ok(WEB_CLIENT_CONN * conn)
{
	conn->response_status = WEB_RES_REQUEST_OK;
	if (conn->http_version == WEB_HTTP09)
		return;

	req_write(conn,  "%s 200 OK\r\n%s", getHeaderVerString(conn), print_http_headers(conn) );

	if (!conn->cgi_type)
	{
		req_write(conn, "Content-Length: %s\r\n%s%s\r\n", 
			simple_itoa(conn->filesize), getHeaderLastModified(conn), getHeaderContentType(conn) );
	}
	req_flush(conn); 
}

/* WEB_RES_NO_CONTENT: 204 */
void send_r_no_content(WEB_CLIENT_CONN * conn)
{
	conn->response_status = WEB_RES_NO_CONTENT;
	if (conn->http_version == WEB_HTTP09)
		return;

	req_write(conn,"%s 204 NO CONTENT \r\n%s", getHeaderVerString(conn), print_http_headers(conn) );

	/* FIXME: Why is this here? */
	if (!conn->cgi_type) {
		req_write(conn, CRLF);
	}
}

/* WEB_RES_PARTIAL_CONTENT: 206 */
void send_r_partial_content(WEB_CLIENT_CONN * conn)
{
	static char msg2[] = "Content-type: multipart/byteranges; "
		"boundary=THIS_STRING_SEPARATES" CRLF;

	conn->response_status = WEB_RES_PARTIAL_CONTENT;
#if 0
	if (conn->http_version != WEB_HTTP11) {
		ECPWS_LOG_INFO("can't do partial content if not HTTP/1.1!!");
		send_r_request_ok(conn);
		return;
	}
#endif

	HTTP_HEADER_HTML(conn, "206 Partial Content");

	req_write(conn, getHeaderLastModified(conn) );
	
	if (conn->numranges > 1)
	{
		req_write(conn, msg2);
	}
	print_partial_content_continue(conn);
	req_write(conn, CRLF);
	req_flush(conn); 
}


/* When some exception or fail happened, the active REQ enter into this state, then send error page, put REQ into free queue 
* and other actions
* response with web page, including HTTP header and HTML page
 */
WEB_CLIENT_STATUS ecpwsHttpHandlerError(WEB_CLIENT_CONN *conn)
{
	char *headerMsg=NULL, varHeader[HTTP_HEADER_LENGTH];
	char bodyMsg[HTTP_HEADER_LENGTH];
#define	NEED_BREAK 0
#if NEED_BREAK
	BOOL_T	needBroken = TRUE_T;
#endif

	memset(bodyMsg, 0 , sizeof(bodyMsg));

	switch (conn->response_status)
	{
		case WEB_RES_MOVED_PERM:
			SPRINTF(varHeader, sizeof(varHeader), "301 Moved Permanently\r\nLocation: %s\r\n", 
				req_write_escape_http(conn, conn->request_uri));
			headerMsg =varHeader;
			SPRINTF(bodyMsg+strlen(bodyMsg), sizeof(bodyMsg)-strlen(bodyMsg), 
				"<HTML><HEAD><TITLE>301 Moved Permanently</TITLE></HEAD>\n"
				"<BODY>\n<H1>301 Moved</H1>The document has moved\n"
		 		"<A HREF=\"%s\">here</A>.\n</BODY></HTML>\n" , req_write_escape_html(conn, conn->request_uri));

			break;
		case WEB_RES_MOVED_TEMP:
			SPRINTF(varHeader, sizeof(varHeader), "302 Moved Temporarily\r\nLocation: %s\r\n", 
				req_write_escape_http(conn, conn->request_uri));
			headerMsg =varHeader;
			SPRINTF(bodyMsg+strlen(bodyMsg), sizeof(bodyMsg)-strlen(bodyMsg), 
				"<HTML><HEAD><TITLE>302 Moved Temporarily</TITLE></HEAD>\n"
				"<BODY>\n<H1>302 Moved</H1>The document has moved\n"
				"<A HREF=\"%s\">here</A>.\n</BODY></HTML>\n" , req_write_escape_html(conn, conn->request_uri));

			break;
		case WEB_RES_NOT_MODIFIED:
			headerMsg = "304 Not Modified";
#if NEED_BREAK
			needBroken = FALSE_T;
#endif
			break;
		case WEB_RES_BAD_REQUEST:
			headerMsg = "400 Bad Request";
			SPRINTF(bodyMsg, sizeof(bodyMsg), "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n"
				  "<BODY><H1>400 Bad Request</H1>\nYour client has issued "
				  "a malformed or illegal request.\n</BODY></HTML>\n");
			break;
		case WEB_RES_UNAUTHORIZED:
			SPRINTF(varHeader, sizeof(varHeader), "401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"%s\"", 
				conn->webSvr->cfg.baseAuthen );
			headerMsg =varHeader;
			SPRINTF(bodyMsg, sizeof(bodyMsg), 	"<HTML><HEAD><TITLE>401 Unauthorized</TITLE></HEAD>\n"
			"<BODY><H1>401 Unauthorized</H1>\nAccess to URL'%s' is not authorized: %s\n<br>Your client does not "
			"have permission to get URL from this server.\n</BODY></HTML>\n", req_write_escape_html(conn, conn->request_uri),
			(conn->authen_status==REQUEST_NOT_AUTHENED)?"No auth data":"Auth data is not validate");
#if NEED_BREAK
			needBroken = FALSE_T;
#endif
			break;
		case WEB_RES_FORBIDDEN:
			headerMsg = "403 Forbidden";
			SPRINTF(bodyMsg, sizeof(bodyMsg), 
				"<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n"
				"<BODY><H1>403 Forbidden</H1>\nYour client does not have permission to get URL %s"
				" from this server.\n</BODY></HTML>\n", req_write_escape_html(conn, conn->request_uri) );
			break;
			
		case WEB_RES_NOT_FOUND:
			headerMsg ="404 Not Found";
			SPRINTF(bodyMsg, sizeof(bodyMsg), 
				"<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n"
				"<BODY><H1>404 Not Found</H1>\nThe requested URL %s" " was not found on this server.\n</BODY></HTML>\n", 
				req_write_escape_html(conn, conn->request_uri) );
			break;
			
		case WEB_RES_LENGTH_REQUIRED:
			headerMsg ="411 Length Required";
 //       HTTP_HEADER_CONTENT_MODIFY(conn, "411 Length Required", getHeaderContentType(conn), getHeaderLastModified(conn) );
        		SPRINTF(bodyMsg, sizeof(bodyMsg), "<HTML><HEAD><TITLE>411 Length Required</TITLE></HEAD>\n"
	                  "<BODY><H1>411 Length Required</H1>\nThe requested URL requires that a valid Content-Length header be "
	                  "%d sent with it.\n</BODY></HTML>\n",(POINTRT_TO_INTEGER) req_write_escape_html(conn, conn->request_uri) );
			break;
		case WEB_RES_PRECONDITION_FAILED:
			headerMsg ="412 Precondition Failed";
			SPRINTF(bodyMsg, sizeof(bodyMsg),"<HTML><HEAD><TITLE>412 Precondition Failed</TITLE></HEAD>\n"
				"<BODY><H1>412 Precondition Failed</H1>\n</BODY></HTML>\n");
			break;
		case WEB_RES_413:
			break;
		case WEB_RES_REQUEST_URI_TOO_LONG:
			headerMsg = "414 Request URI Too Long";
			SPRINTF(bodyMsg, sizeof(bodyMsg),"<HTML><HEAD><TITLE>414 Request URI Too Long</TITLE></HEAD>\n"
				"<BODY><H1>414 Request URI Too Long</H1>\nYour client has issued "
				"a malformed or illegal request.\n</BODY></HTML>\n");
			break;
		case WEB_RES_INVALID_RANGE:
			headerMsg = "416 Invalid Range";
			SPRINTF(bodyMsg, sizeof(bodyMsg),"<HTML><HEAD><TITLE>416 Invalid Range</TITLE></HEAD>\n"
				"<BODY><H1>416 Invalid Range</H1>\n</BODY></HTML>\n");
			break;
		case WEB_RES_ERROR:
			headerMsg =  "500 Server Error";
			SPRINTF(bodyMsg, sizeof(bodyMsg),"<HTML><HEAD><TITLE>500 Server Error</TITLE></HEAD>\n"
					"<BODY><H1>500 Server Error</H1>\nThe server encountered "
					"an internal error and could not complete your request.\n"
					"</BODY></HTML>\n");
			break;
		case WEB_RES_NOT_IMP:
			headerMsg = "501 Not Implemented";
			SPRINTF(bodyMsg, sizeof(bodyMsg),"<HTML><HEAD><TITLE>501 Not Implemented</TITLE></HEAD>\n"
				"<BODY><H1>501 Not Implemented</H1>\nPOST to non-script "
				"is not supported in SVR_WEB.\n</BODY></HTML>\n");
			break;
		case WEB_RES_BAD_GATEWAY:
			headerMsg = "502 Bad Gateway";
			SPRINTF(bodyMsg, sizeof(bodyMsg),"<HTML><HEAD><TITLE>502 Bad Gateway</TITLE></HEAD>\n"
				"<BODY><H1>502 Bad Gateway</H1>\nThe CGI was not CGI/1.1 compliant.\n" "</BODY></HTML>\n");
			break;
		case WEB_RES_SERVICE_UNAV:
			headerMsg = "503 Service Unavailable";
			SPRINTF(bodyMsg, sizeof(bodyMsg),
			        "<HTML><HEAD><TITLE>503 Service Unavailable</TITLE></HEAD>\n"
			        "<BODY><H1>503 Service Unavailable</H1>\n"
			        "There are too many connections in use right now.\r\n"
			        "Please try again later.\r\n</BODY></HTML>\n");
			break;
			
		case WEB_RES_BAD_VERSION:
			headerMsg = "505 HTTP Version Not Supported";
			SPRINTF(bodyMsg, sizeof(bodyMsg),"<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n"
				"<BODY><H1>505 HTTP Version Not Supported</H1>\nHTTP versions "
				"other than 0.9 and 1.0 "
				"are not supported in ECPWS.\n<p><p>Version encountered: %s<p><p></BODY></HTML>\n", 
				getHeaderVerString(conn));
			break;
			
		case WEB_RES_MULTIPLE:
		case WEB_RES_303:

		case WEB_RES_PAYMENT:
		case WEB_RES_METHOD_NA:
		case WEB_RES_NON_ACC:
		case WEB_RES_PROXY:
		case WEB_RES_REQUEST_TO:
		case WEB_RES_CONFLICT:
		case WEB_RES_GONE:
		case WEB_RES_415:

		case WEB_RES_GATEWAY_TO:
		default:
			SPRINTF(headerMsg, sizeof(headerMsg), "%d Unimplmented Error", conn->response_status);
			SPRINTF(bodyMsg, sizeof(bodyMsg),"<HTML><HEAD><TITLE>%d HTTP Error Not Supported</TITLE></HEAD>\n"
				"<BODY><H1>%d HTTP Error Not Implemented</H1>\nHTTP versions </BODY></HTML>\n", 
				conn->response_status, conn->response_status);
			break;
	}
	
	HTTP_SEND_ERROR_PAGE(conn, headerMsg, bodyMsg);

#if NEED_BREAK
	if(IS_FALSE(needBroken))
		return WEB_CLIENT_S_READ_HEADER;
#endif

	if (conn->method == WEB_M_POST)
	{
		char buf[32768];
		svcWebConnRead(conn,buf, sizeof(buf));
	}

	return ecpwsHandlerTimeout( conn);
}

