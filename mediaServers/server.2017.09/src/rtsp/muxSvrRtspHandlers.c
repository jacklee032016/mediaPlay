/*
* $Id$
*/

#include "muxSvr.h"

#include "__services.h"

#if WITH_RTSP_AUTHENTICATE
int countRtspAuthErr;
#endif

int muxSvrRtspEventErrorInfo(SERVER_CONNECT *svrConn, int statusCode)
{
	char nonce[33];
	memset(nonce,'\0', 33);

	MUX_SVR_LOG( "RTSP Reply Error to client, error code : %d", statusCode);

	rtspReplyHeader(svrConn, statusCode);

	switch(statusCode)
	{
#if WITH_RTSP_AUTHENTICATE
		case RTSP_STATUS_UNAUTHORIZED:
			msu_rtsp_auth_caculate_nonce(nonce);
			muxSvrConnectPrintf(svrConn,  "WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\"\r\n","SONiX Streaming Media",nonce);
			break;
#endif
		default:
			break;
	}

	muxSvrConnectPrintf(svrConn, "\r\n");

	muxSvrConnectFlushOut( svrConn);

#if WITH_RTSP_AUTHENTICATE
	/* according to RtspAuthErr times, decide CLOSING or CONTINUE */	
	if(countRtspAuthErr >= 4)
	{
		countRtspAuthErr = 0;
		return MUX_SVR_STATE_CLOSING;
	}
	else
	{
		return MUX_SVR_STATE_CONTINUE;
	}
	
#else

	return MUX_SVR_STATE_CLOSING;

#endif
}


int rtspEventError(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	return muxSvrRtspEventErrorInfo(svrConn, event->statusCode);
}


static MuxStream *_findRtspMediaStream(char *path, SERVER_CONNECT *svrConn)
{
	MuxStream *stream = muxSvrFoundStream(TRUE, path, svrConn->muxSvr);
	if(stream)
	{
		MUX_SVR_LOG( "found RTSP/RTP output stream of %s in SERVER stream list", stream->filename );
		return stream;
	}
	
	muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_SERVICE); /* XXX: right error ? */
	return NULL;
}

/* OPTIONS can be send at any state */
int rtspEventOptions(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
#if WITH_RTSP_AUTHENTICATE
	countRtspAuthErr = 0;
#endif

	muxSvrConnectPrintf(svrConn, "RTSP/1.0 %d %s\r\n", RTSP_STATUS_OK, "OK");
	muxSvrConnectPrintf(svrConn, "CSeq: %d\r\n", svrConn->seq);
	muxSvrConnectPrintf(svrConn, "Public: %s\r\n", "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE");
	muxSvrConnectPrintf(svrConn, "\r\n");

	muxSvrConnectFlushOut( svrConn);

	return MUX_SVR_STATE_CONTINUE;
}

/* in INIT state **/
int rtspEventDescribe(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	MuxStream *stream = NULL;
	uint8_t *content;
	int contentLength;

	rtsp_event_t	*rtspEvent = (rtsp_event_t *)event;

	/* find which url is asked */
	MUX_SVR_DEBUG( "RTSP Request is DESCRIBE");
#if WITH_RTSP_AUTHENTICATE
	if(svrConn->muxSvr->svrConfig.enableAuthen )
	{
		if(foundRtspAuthenInfo(svrConn->buffer) == 0)
		{
			countRtspAuthErr++;
		}
		if(!msu_rtsp_auth_client_is_activex(svrConn->buffer) && !msu_rtsp_authenticate(svrConn->buffer))
		{			
			countRtspAuthErr++;
			return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_UNAUTHORIZED);
		}
		countRtspAuthErr = 0;
	}
#endif

	stream = _findRtspMediaStream(rtspEvent->path, svrConn);
	if(stream == NULL  )
	{
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_SESSION);
	}

	svrConn->stream = stream;

#if WITH_ADDRESS_ACL
	if(stream->acl != NULL && stream->acl->action != MUX_SVR_ACL_DISABLE)
	{
		if(msSvrValidateAcl(stream, svrConn) == 0)
		{
			fprintf(stderr, "Remote IP Address is denied\n");
			return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_UNAUTHORIZED);
		}
		else 
			fprintf(stderr, "Remote IP Address is allowed\n");
	}
#endif
	

VTRACE();

	/* prepare the media description in sdp format */
	muxSvrGetHostIP(svrConn);

VTRACE();
	contentLength = muxSvrPrepareSdpDescription(stream, &content, stream->muxSvr->myAddr.sin_addr );
	if (content == NULL)
	{
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_INTERNAL);
	}

	rtspReplyHeader(svrConn, RTSP_STATUS_OK);

	muxSvrConnectPrintf(svrConn, "Content-Type: application/sdp\r\n");
	muxSvrConnectPrintf(svrConn, "Content-Length: %d\r\n", contentLength );
	muxSvrConnectPrintf(svrConn, "\r\n");
	muxSvrConnectPrintf(svrConn, "%s", content);

	muxSvrConnectFlushOut(svrConn);
	
VTRACE();
	av_free(content);
VTRACE();
	return MUX_SVR_STATE_CONTINUE;
}


int rtspEventPlay(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	DATA_CONNECT *rtpConn;
	rtsp_event_t	*rtspEvent = (rtsp_event_t *)event;

	MUX_SVR_LOG( "RTSP Request is Play, Session %s", rtspEvent->header.session_id);
	rtpConn = muxSvrRtpFindConnectWithUrl(rtspEvent->path, svrConn );
	if (rtpConn == NULL)
	{
		MUX_SVR_LOG( "No RTP Session is found");
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_SESSION);
	}

	if (svrConn->state != RTSP_STATE_READY ) //&& rtpConn->state != CONN_STATE_WAIT_FEED &&	rtpConn->state != RTP_STATE_READY)
	{
		MUX_SVR_LOG( "RTP Session state is error");
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_STATE);
	}

	/* now everything is OK, so we can send the connection parameters */
	rtspReplyHeader(svrConn, RTSP_STATUS_OK);

	/* session ID */
	muxSvrConnectPrintf(svrConn, "Session: %s\r\n", rtspEvent->header.session_id);
	muxSvrConnectPrintf(svrConn, "\r\n");

	muxSvrConnectFlushOut( svrConn);
	
		VTRACE();
	if(svrConn->state != RTSP_STATE_PLAY)
	{
		VTRACE();
		ENABLE_SVR_CONN(svrConn->stream->feed, svrConn);
		return RTSP_STATE_PLAY;
	}
	else
		return MUX_SVR_STATE_CONTINUE;
}


int rtspEventRecord(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	DATA_CONNECT *rtpConn;
	rtsp_event_t	*rtspEvent = (rtsp_event_t *)event;

	MUX_SVR_LOG( "RTSP Request is Play, Session %s",rtspEvent->header.session_id);
	rtpConn = muxSvrRtpFindConnectWithUrl(rtspEvent->path, svrConn );
	if (rtpConn == NULL)
	{
		MUX_SVR_LOG( "No RTP Session is found");
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_SESSION);
	}

	if (svrConn->state != RTSP_STATE_READY)// SEND_DATA && rtpConn->state != CONN_STATE_WAIT_FEED && rtpConn->state != RTP_STATE_READY)
	{
		MUX_SVR_LOG( "RTP Session state is error");
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_STATE);
	}

	/* now everything is OK, so we can send the connection parameters */
	rtspReplyHeader(svrConn, RTSP_STATUS_OK);

	/* session ID */
	muxSvrConnectPrintf(svrConn, "Session: %s\r\n", rtspEvent->header.session_id);
	muxSvrConnectPrintf(svrConn, "\r\n");

	muxSvrConnectFlushOut( svrConn);
	return RTSP_STATE_RECORD;
}

int rtspEventPause(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event )
{
	DATA_CONNECT *rtpConn;
	rtsp_event_t	*rtspEvent = (rtsp_event_t *)event;

	rtpConn = muxSvrRtpFindConnectWithUrl(rtspEvent->path, svrConn );
	if (rtpConn == NULL)
	{
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_SESSION);
	}

	if (svrConn->state != RTSP_STATE_PLAY && svrConn->state != RTSP_STATE_RECORD )
	{
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_STATE);
	}

	svrConn->firstPts = AV_NOPTS_VALUE;
	rtpConn->firstPts = AV_NOPTS_VALUE;

	/* now everything is OK, so we can send the connection parameters */
	rtspReplyHeader(svrConn, RTSP_STATUS_OK);
	/* session ID */
	muxSvrConnectPrintf(svrConn, "Session: %s\r\n", rtspEvent->header.session_id);
	muxSvrConnectPrintf(svrConn, "\r\n");

	muxSvrConnectFlushOut( svrConn);

	return RTSP_STATE_READY;
}

int rtspEventTeardown(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
#define	CLOSE_DIRECTLY		1

	DATA_CONNECT *rtpConn;
	MuxStream *stream;
	rtsp_event_t	*rtspEvent = (rtsp_event_t *)event;

	MUX_SVR_DEBUG( "RTSP Event is TREADOWN, path : %s", rtspEvent->path );
	DISABLE_SVR_CONN(svrConn->stream->feed, svrConn);

	stream = svrConn->stream;
	if(stream == NULL )
	{
VTRACE();
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_SESSION);
	}
	
VTRACE();
	rtpConn = muxSvrRtpFindConnectWithUrl(rtspEvent->path , svrConn );
	if (rtpConn == NULL)
	{
VTRACE();
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_SESSION);
	}

	/* abort the session if not use multicast */
#if CLOSE_DIRECTLY	
#else
	if(!stream->is_multicast)
		muxSvrRtpDataConnClose(rtpConn);
#endif		

#if 0
	if (ff_rtsp_callback)
	{
		ff_rtsp_callback(RTSP_ACTION_SERVER_TEARDOWN, rtspEvent->header.session_id,  NULL, 0, rtpConn->stream->rtsp_option);
	}
#endif

	/* now everything is OK, so we can send the connection parameters */
	rtspReplyHeader(svrConn, RTSP_STATUS_OK);
	/* session ID */
	muxSvrConnectPrintf(svrConn, "Session: %s\r\n", rtspEvent->header.session_id);
	muxSvrConnectPrintf(svrConn, "\r\n");

	muxSvrConnectFlushOut(svrConn);

	stream->conns_served--;

#if CLOSE_DIRECTLY	
	return MUX_SVR_STATE_CLOSING;
#else
	rtpConn = muxSvrRtpFindConnectWithUrl(svrConn->stream->filename, svrConn );
	if (rtpConn == NULL)
	{/* no other RTP connection for this RTSP connection */
		MUX_SVR_DEBUG( "No more RTP connection exist, closing whole connection" );
		return MUX_SVR_STATE_CLOSING;
	}
	
	MUX_SVR_DEBUG("More RTP Connect exist for this TREADOWN" );

	return MUX_SVR_STATE_CONTINUE;
#endif
}


/* in INIT state and go to READY state */
int rtspEventSetup(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event )
{
	MuxStream *stream = NULL;
	int streamIndex = -1;
	int	rtp_port, rtcp_port;
	RTSPTransportField *transports = NULL;
	char buf[1024];
	rtsp_event_t	*rtspEvent = (rtsp_event_t *)event;
	int i;

	DATA_CONNECT *rtpConn;
	struct sockaddr_in dest_addr;
	RTSPActionServerSetup setup;
	int setupStream = FALSE;

	MUX_SVR	 *muxSvr = svrConn->muxSvr;
	memset(buf, 0 ,sizeof(buf));

VTRACE();
	//stream = svrConn->stream;/* assigned when processing DESCRIBE msg */
	if(stream == NULL)
	{
		for(i=0 ;i <cmn_list_size(&muxSvr->svrConfig.streams); i++)
		{
			MuxStream *st = (MuxStream *)cmn_list_get(&muxSvr->svrConfig.streams, i);

			if ( !st->fmt ||strcmp(st->fmt->name, "rtp"))
			{
				continue;
			}
			
			/* accept aggregate filenames only if single stream */
			if (!strcmp((char *)rtspEvent->path, st->filename))
			{
				if (st->feed->nbInStreams != 1)
				{
					return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_AGGREGATE);
				}
				streamIndex = 0;

				stream = st;
				svrConn->stream = stream;
			}

			for(streamIndex = 0; streamIndex < st->feed->nbInStreams; streamIndex++)
			{
				snprintf(buf, sizeof(buf), "%s/streamid=%d", st->filename, streamIndex);
				if (!strcmp((char *)rtspEvent->path, buf))
				{
					MUX_SVR_DEBUG( "Stream with name of '%s' is found in No.%d stream in %s", rtspEvent->path, streamIndex, st->filename );
					stream = st;
					svrConn->stream = stream;
					break;
				}
			}

			if(stream)
				break;

		}
			
	}

	if(stream==NULL)
	{
		MUX_SVR_LOG( "Stream with name of '%s' is not found", rtspEvent->path );
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_SESSION);
	}
	
	if(streamIndex == -1)
	{/* no stream found */
		MUX_SVR_LOG( "Stream index is not validate" );
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_SERVICE);
	}
	
	/* generate session id if needed */
	if (rtspEvent->header.session_id[0] == '\0')
	{
		/*  if multicast, then allocate mcast_session_id for stream session */
		
		unsigned random0 = av_lfg_get(&muxSvr->random_state);
		unsigned random1 = av_lfg_get(&muxSvr->random_state);
		snprintf(rtspEvent->header.session_id, sizeof(rtspEvent->header.session_id), "%08x%08x", random0, random1);
		MUX_SVR_DEBUG( "Stream seesion Id is allocated as '%s'", rtspEvent->header.session_id );
	}


	/* find rtp session, and create it if none found */
	rtpConn = muxSvrRtpFindConnectWithUrl(rtspEvent->path, svrConn );
	if (!rtpConn)
	{

		MUX_SVR_DEBUG( "Create new RTP Session" );
		rtpConn = muxSvrRtpNewConnection(svrConn, streamIndex, &rtspEvent->header );
		if (!rtpConn)
		{
			MUX_SVR_LOG( "New RTP Connection Failed");
			return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_BANDWIDTH);
		}

		/* open input stream */
		if (muxSvrRtpDataConnOpen(rtpConn, NULL) < 0)
		{
			return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_INTERNAL);
		}

		setupStream = TRUE;
	}
	else
	{
		MUX_SVR_DEBUG( "found exist RTP Session" );
		if (!rtpConn->outFmtCtx)
		{/* if stream has already set up, so no 'SETUP' cmd is needed */
			MUX_SVR_LOG( "RTP AvFormat of Stream with name of '%s' index is '%d' has exist", rtspEvent->path, streamIndex );
			return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_STATE);
		}
	}

	/* test if stream is OK (test needed because several SETUP needs to be done for a given file) */
	if (rtpConn->stream != stream)
	{
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_SERVICE);
	}


	/* setup default options */
	setup.transport_option[0] = '\0';

	transports = find_transport(&rtspEvent->header, rtpConn->rtpProtocol );

	if (!transports || (transports->lower_transport == RTSP_LOWER_TRANSPORT_UDP && transports->client_port_min <= 0))
	{
		return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_TRANSPORT);
	}
	MUX_SVR_DEBUG( "No. %d Client Port(min) %d", streamIndex,  transports->client_port_min );

	memcpy(&dest_addr, &svrConn->peerAddress, sizeof(struct sockaddr_in));
	dest_addr.sin_port = htons(transports->client_port_min);

VTRACE();
#if 0
	/* add transport option if needed */
	if (ff_rtsp_callback)
	{
		setup.ipaddr = ntohl(dest_addr.sin_addr.s_addr);
		if (ff_rtsp_callback(RTSP_ACTION_SERVER_SETUP, rtspEvent->header.session_id, (char *)&setup, sizeof(setup), stream->rtsp_option) < 0)
		{
			return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_TRANSPORT);
		}

		dest_addr.sin_addr.s_addr = htonl(setup.ipaddr);
	}
#endif

	/*
	  * if not multicast, setup stream
	  * if multicast, no need to setup stream
	  */
	if(!stream->is_multicast)
	{
		if(setupStream == TRUE)
		{/* setup stream */
VTRACE();
			if (muxSvrRtpNewOutHandler(rtpConn, streamIndex, &dest_addr ) < 0)
			{
				return muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_TRANSPORT);
			}
		}
	}

VTRACE();
	/* now everything is OK, so we can send the connection parameters */
	rtspReplyHeader(svrConn, RTSP_STATUS_OK);
	/* session ID */
	muxSvrConnectPrintf(svrConn, "Session: %s\r\n", rtspEvent->header.session_id);

	switch(rtpConn->rtpProtocol )
	{
		case RTSP_LOWER_TRANSPORT_UDP:
			rtp_port = ff_rtp_get_local_rtp_port(rtpConn->urlContext);
			rtcp_port = ff_rtp_get_local_rtcp_port(rtpConn->urlContext);
			
VTRACE();
			muxSvrConnectPrintf(svrConn, "Transport: RTP/AVP/UDP;unicast;"
				"client_port=%d-%d;server_port=%d-%d", transports->client_port_min, transports->client_port_max, rtp_port, rtcp_port);
			break;
		
		case RTSP_LOWER_TRANSPORT_TCP:
			VTRACE();
			muxSvrConnectPrintf(svrConn, "Transport: RTP/AVP/TCP;interleaved=%d-%d", streamIndex * 2, streamIndex * 2 + 1);
			break;
			
#if WITH_RTSP_UDP_MULTICAST
		case RTSP_LOWER_TRANSPORT_UDP_MULTICAST:
VTRACE();
			port = stream->multicast_port + 2*streamIndex; 
			muxSvrConnectPrintf(svrConn, "Transport: RTP/AVP;multicast;"
				"destination=%s;port=%d-%d;ttl=%d", inet_ntoa(stream->multicast_ip), port, port+1,  stream->multicast_ttl);
			//fprintf(stderr, "Transport: RTP/AVP;multicast;"
			//	"destination=%s;port=%d-%d;ttl=%d\n", inet_ntoa(stream->multicast_ip), port, port+1, stream->multicast_ttl);
			break;
#endif

		default:
			break;
	}
	
VTRACE();
	if (setup.transport_option[0] != '\0')
	{
		muxSvrConnectPrintf(svrConn, ";%s", setup.transport_option);
	}
VTRACE();
	muxSvrConnectPrintf(svrConn, "\r\n\r\n");

VTRACE();
	muxSvrConnectFlushOut(svrConn);

VTRACE();
	if(svrConn->state == RTSP_STATE_INIT)
		return RTSP_STATE_READY;
	else/* in state of READY or PLAY */
		return MUX_SVR_STATE_CONTINUE;
}

/* connection is broken, not by RTSP protocol */
int rtspEventClosing(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	/* different handler in different state, ???? */

	return httpEventClosing(svrConn, event);
}

