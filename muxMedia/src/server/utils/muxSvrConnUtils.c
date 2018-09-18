/*
* $Id$
*/

#include "muxSvr.h"

/* implement ACL */
#if WITH_ADDRESS_ACL
int msSvrValidateAcl(MuxStream *stream, SERVER_CONNECT *conn)
{
	enum IPAddressAction last_action = MUX_SVR_ACL_IP_ALLOW;
	IPAddressACL *acl;
	
	unsigned long src_addr = conn->peerAddress.sin_addr.s_addr;
	fprintf(stderr, "from_addr:%s\n", inet_ntoa(conn->peerAddress.sin_addr));

	/* debug acl info */
#if 0
	if(stream->acl->action == 1)
		fprintf(stderr, "ACLGlob allow. The deny_IP_list is:\n");
	if(stream->acl->action == 2)
		fprintf(stderr, "ACLGlob deny. The allow_IP_list is:\n");
	for (acl = stream->acl; acl != NULL; acl = acl->next)
	{
		if(ntohl(acl->first.s_addr) == ntohl(acl->last.s_addr))
			fprintf(stderr, "%s\n", inet_ntoa(acl->first));
		else
			fprintf(stderr, "%s -- %s\n", inet_ntoa(acl->first), inet_ntoa(acl->last));
	}
#endif
	/* debug end */
	
	for (acl = stream->acl; acl != NULL; acl = acl->next)
	{		
		if ( (src_addr >= ntohl(acl->first.s_addr) ) && (src_addr <= ntohl(acl->last.s_addr) ) )
		{			
			return (stream->acl->action == MUX_SVR_ACL_IP_ALLOW) ? 0 : 1;
		}
		last_action = stream->acl->action;
	}

	/* Nothing matched, so return the global action */
	return (last_action == MUX_SVR_ACL_IP_ALLOW) ? 1 : 0;
}
#endif


/* read data in newly created HTTP/RTSP connection repeatly */
int muxSvrConnectReceive(SERVER_CONNECT *svrConn)
{
	int len;
	uint8_t *ptr;

	len = read(svrConn->fd, svrConn->readBuffer.bufferPtr, SVR_BUF_LENGTH(svrConn->readBuffer) );
	if (len < 0)
	{
		if (errno != EAGAIN && errno != EINTR)
			return -1;
	}
	else if (len == 0)
	{/* EOF : connection is closed */
		return 0;//-1;
	}

	MUX_SVR_DEBUG("'%s' received %d bytes: '%s'", svrConn->name, len, svrConn->readBuffer.bufferPtr);
	svrConn->readBuffer.bufferPtr += len;
	ptr = svrConn->readBuffer.bufferPtr;
	
	if ((ptr >= svrConn->readBuffer.dBuffer+ 2 && !memcmp(ptr-2, "\n\n", 2)) ||
		(ptr >= svrConn->readBuffer.dBuffer + 4 && !memcmp(ptr-4, "\r\n\r\n", 4)))
	{
		return len;
	}
	else if (ptr >= svrConn->readBuffer.bufferEnd)
	{
		/* request too long: cannot do anything */
		return len;
	}

	/* continue to read more */
	return 0;
}


int muxSvrConnectPrintf(SERVER_CONNECT *svrConn, const char *fmt, ...)
{
	va_list ap;
	char buf[4096];
	int ret;

	va_start(ap, fmt);
	ret = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	avio_write( svrConn->dynaBuffer, (unsigned char *)buf, strlen(buf));

	return ret;
}

/* needFlush is false only when send HEADER and TRAILER for container; other HTTP/RTSP contents are send with needFlush=TRUE */
int muxSvrConnectFlushOut(SERVER_CONNECT *svrConn)
{
	int total, len = 0, ret;

	/* flush all data in ByteIOBuffer into conn->pb_buffer and return length of buffer */
	total = avio_close_dyn_buf(svrConn->dynaBuffer, &svrConn->cmdBuffer.dBuffer);
	if (total < 0)
	{/* XXX: cannot do more : close this connection */
		return total;
	}
	svrConn->isInitedDynaBuf = FALSE;

	svrConn->cmdBuffer.bufferPtr = svrConn->cmdBuffer.dBuffer;
	svrConn->cmdBuffer.bufferEnd = svrConn->cmdBuffer.dBuffer + total ;
	
retry:
	ret = write(svrConn->fd, svrConn->cmdBuffer.bufferPtr, SVR_BUF_LENGTH(svrConn->cmdBuffer) );
	if( ret < 0)
	{
		if (errno != EAGAIN && errno != EINTR)
		{/* error : close connection */
			return ret;
		}
		else
		{
			goto retry; //ret = 0;
		}
	}


	MUX_SVR_DEBUG( "%s send out %d of %d bytes", svrConn->name, ret, total );

	{
		char msg[8192];
		memset(msg, 0, sizeof(msg));
		memcpy(msg, svrConn->cmdBuffer.bufferPtr, (total<sizeof(msg))?total:sizeof(msg) );
		MUX_SVR_DEBUG( "send out msg is '%s'", msg );
	}

	svrConn->cmdBuffer.bufferPtr += ret;
	len += ret;

	if(len < total )
	{
		goto retry;
	}

	svrConn->dataCount += total;
	
	av_free(svrConn->cmdBuffer.dBuffer);

	if (avio_open_dyn_buf( &svrConn->dynaBuffer ) < 0)
	{/* XXX: cannot do more */
		return -EXIT_FAILURE;
	}
	svrConn->isInitedDynaBuf = TRUE;
	return total;
}


/* DataBuffer output, used for HTTP media data, RTP/TCP media data and Header/Tailer of RTMP/UMC/HLS 
* this tranport can be retransmited in FSM+ select() 
*/
int	muxSvrConnectDataWriteFd(SERVER_CONNECT *svrConn, uint8_t *buffer, int length )
{
	int len;

	len = write(svrConn->fd,  buffer, length );
	if (len < 0)
	{
		MUX_SVR_ERROR( "Write media data for '%s' error: %s", svrConn->name, strerror(errno));
#if 0
		if (errno != EAGAIN && errno != EINTR)
#else			
		if (ff_neterrno() != AVERROR(EAGAIN) && ff_neterrno() != AVERROR(EINTR))
#endif			
		{/* error : close connection */
			return -1;
		}
		else
		{/*EAGAIN, 11,Try again */
			VTRACE();
//			cmn_usleep(2000*10);
			return 0;
		}
	}
	else
	{
//		svrConn->bufferPtr += len;
	}

	return len;
}



int muxSvrServiceReportEvent(SERVER_CONNECT *svrConn, int event, int  errorCode, void *data )
{
	MUX_SVR_EVENT	*newEvent;
//	rtsp_event_t	*serviceEvent;

	newEvent = av_mallocz( sizeof(MUX_SVR_EVENT) );
	newEvent->event = event;
	SET_FSM_EVENT(newEvent->event);
	
	newEvent->statusCode = errorCode;
	newEvent->myConn = svrConn;
	newEvent->data = data;
	
	SVR_REPORT_FSM_EVENT(newEvent);
	return EXIT_SUCCESS;
}

MuxStream  *muxSvrFoundStream(int isRtsp, char *feedName, MuxServerConfig *svrConfig)
{
	MuxStream *stream;
	int i;	

	for(i=0; i< cmn_list_size(&svrConfig->streams); i++ ) 
	{
		stream = (MuxStream *)cmn_list_get(&svrConfig->streams, i);
		if(stream == NULL)
		{
			CMN_ABORT( "No %d Stream is NULL" , i);
			exit(1);
		}

//		MUX_SVR_DEBUG( "No.%d stream is '%s' in stream list", i, stream->filename );
		if(isRtsp == TRUE)
		{
			if ( IS_STREAM_FORMAT(stream->fmt,  "rtp" ) && strstr(feedName, stream->filename) )
//			if ( stream->fmt == &ff_rtp_muxer && strstr(feedName, stream->name) )
			{
				MUX_SVR_DEBUG( "found RTSP/RTP output stream of %s in SERVER stream list", stream->filename );
	//			path[strlen(stream->filename)] = '\0';
				return stream;
			}
		}
		else
		{
			if (!strcmp(stream->filename, feedName) )
			{
				return stream;
			}
		}
	}

	return NULL;
}

MuxFeed  *muxSvrFoundFeed(int type, char *feedName, MuxServerConfig *svrConfig)
{
	MuxFeed *feed;
	int i;	

	for(i=0; i< cmn_list_size(&svrConfig->feeds); i++ ) 
	{
		feed = (MuxFeed *)cmn_list_get(&svrConfig->feeds, i);
		if(feed == NULL)
		{
			MUX_SVR_ERROR( "No %d Feed is NULL" , i);
			exit(1);
		}

		{
			if (!strcmp(feed->filename, feedName) )
			{
				return feed;
			}
		}
	}

	return NULL;
}


int	muxSvrServerConnectionIterate( CONNECTION_HANDLER *connHandler, MUX_SVR *muxSvr)
{
	SERVER_CONNECT *svrConn;
	int 	i;

	if(connHandler == NULL)
		return -EXIT_FAILURE;

	SERVER_CONNS_LOCK(muxSvr);
	for(i=0; i< cmn_list_size(&muxSvr->serverConns); i++ ) 
	{
		svrConn = (SERVER_CONNECT *)cmn_list_get(&muxSvr->serverConns, i);
		if(svrConn == NULL)
		{
			MUX_SVR_ERROR( "No %d Service Connection is NULL" , i);
			exit(1);
		}

		(*connHandler)( svrConn);
	}
	SERVER_CONNS_UNLOCK(muxSvr);

	return 0;
}


