/*
 *
 */

#include "muxWeb.h"

#if 0
/* "\r\r\n" eg. 0x0D0x0D0x0A is splitter between fields in HTTP headers */
static unsigned char *_findHeaderSplitter(unsigned char *header, int size)
{
	unsigned char *check = header;
	int count = 0;

//	ECPWS_DEBUG_INFO("lookup in '%s', size of %d bytes\n", header, size)
#if 1
	while( check[0] != '\r' && count< size)
	{
		check++;
		count++;
	}
#else	
	check = strchr(header, '\r' );
#endif	
//	ECPWS_DEBUG_INFO("count %d 0x%x\n", count, check[0] );
	if(check[0]=='\r' )
	{
//		ECPWS_DEBUG_INFO("find First slpit 0x%x 0x%x\n", check[1], check[2]);
		if( (check[0]=='\r' && check[1]=='\n' ) )
			return check;
	}

	return NULL;
}
#endif

WEB_CLIENT_STATUS ecpwsHttpHandlerReadHeader(WEB_CLIENT_CONN * conn)
{
	int bytes, count;
	char *check, *buffer;
//	CmnMuxWebConfig *cfg = conn->webSvr->cfg;
	WEB_CLIENT_STATUS newStatus= WEB_CLIENT_S_HTTP_WRITE;
	/* only reached if request is split across more than one packet */
	unsigned int buf_bytes_left;

	buffer = conn->client_stream;

	buf_bytes_left = CLIENT_STREAM_SIZE - conn->client_stream_pos;
	if (buf_bytes_left < 1 || buf_bytes_left > CLIENT_STREAM_SIZE)
	{
		ECPWS_LOG_INFO_WITH_REQ(conn, "No space left in client stream buffer, closing\n");
		WEB_CONN_RES_ERR(conn, WEB_RES_ERROR);
	}

	
	bytes = svcWebConnRead(conn, buffer + conn->client_stream_pos, buf_bytes_left);
	if (bytes <= 0 )
	{/* peer close this socket */
		return ecpwsHandlerTimeout( conn);
	}

	/* bytes is positive */
	conn->client_stream_pos += bytes;

	{
		conn->client_stream[conn->client_stream_pos] = '\0';
#if WEB_OPTIONS_DEBUG_DATA			
		ECPWS_LOG_INFO("- CONN.%d read %d bytes:\"", conn->index, bytes);
		ecpwsOutputMsg( conn->client_stream + conn->client_stream_pos - bytes);
		ecpwsOutputMsg( "\"\n");
#endif
	}
	check = conn->client_stream + conn->parse_pos;
	buffer = conn->client_stream;
	count = bytes = conn->client_stream_pos - conn->parse_pos;
	conn->header_line = conn->client_stream + conn->parse_pos;

	if (check < (buffer + bytes))
	{
		buffer[bytes] = '\0';
//		ECPWS_DEBUG_INFO( "- Parsing headers (\"%s\")\n", check);
	}

	while (conn->header_line < (buffer + bytes))
	{

		if( (check=strstr(conn->header_line, "\r\n")))
		{
			*check = '\0';
			conn->header_end = check;
			check = check+2;

			conn->parse_pos = check - buffer;
			
			if (conn->header_end - conn->header_line >= MAX_HEADER_LENGTH)
			{
				ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, 
					"CONN%d: Header too long at %d bytes: \"%s\"\n", conn->index, conn->header_end - conn->header_line, conn->header_line);
				return WEB_CLIENT_S_ERROR;
			}


#if WEB_OPTIONS_DEBUG_DATA			
			ECPWS_DEBUG_INFO("parsing header '%s'\n", conn->header_line);
#endif
			newStatus = ecpwsHttpParseRequestLine(conn);

			/* next SPLITTER string */
			check=strstr(check, "\r\n");
			if( (check !=NULL && check == conn->header_end +2) ||(check==NULL) )
			{
#if WEB_OPTIONS_DEBUG_DATA			
				ECPWS_DEBUG_INFO("find End of HTTP REQ\n");
#endif				

				newStatus =  process_header_end(conn);
#if 1
				/* change here or change the return value of _newProcess() to debug the problem of CGI not works. Jack Lee, Oct.19,2017  */
				if(newStatus != WEB_CLIENT_S_CGI_READ) 
//				if(conn->status != WEB_CLIENT_S_CGI_READ)
				{
					if(check == NULL)
					{/* for telnet client */
						count = count - (conn->header_end - conn->header_line +2);
						conn->header_line = conn->header_end+2;
					}
					else
					{
//						count = count - (conn->header_end - conn->header_line +4);
						conn->header_line = conn->header_end;//+4;
						conn->header_end = conn->client_stream+conn->client_stream_pos;
					}
				}
#endif				
				return newStatus;
			}
			else
			{
				count = count - (conn->header_end - conn->header_line +2);
				conn->header_line = conn->header_end+2;
			}
		}
		else
		{
			conn->header_line = conn->header_line +count;
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "Not legal HTTP request(%s) in stream.\n", check);
			return WEB_CLIENT_S_ERROR;
		}


	}                           /* done processing available buffer */

	/* move data in the buffer */
	conn->parse_pos = conn->header_line-conn->client_stream;
	if(conn->parse_pos == conn->client_stream_pos)
	{
		conn->parse_pos = conn->client_stream_pos = 0;
	}
	else if(conn->parse_pos < conn->client_stream_pos )
	{
		memmove(conn->client_stream, conn->client_stream+conn->parse_pos, (conn->client_stream_pos-conn->parse_pos));
		conn->client_stream_pos = conn->client_stream_pos-conn->parse_pos;
		conn->parse_pos = 0;
	}
	else
	{
		ECPWS_ERR_INFO("parse position %d is bigger than stream position %d\r\n", conn->parse_pos, conn->client_stream_pos);
	}

	return newStatus;
}

