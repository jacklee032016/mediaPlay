/*
* $Id$
*/


#include "muxSvr.h"

#include "__services.h"

RTSPTransportField *find_transport(RTSPMessageHeader *h, enum RTSPLowerTransport protocol)
{
	RTSPTransportField *th;
	int i;

	for(i=0;i< h->nb_transports;i++)
	{
		th = &h->transports[i];

//		if (th->protocol == protocol)
		if (th->lower_transport == protocol)
			return th;
	}
	return NULL;
}


/* RTSP handling */
void rtspReplyHeader(SERVER_CONNECT *svrConn, enum RTSPStatusCode error_number)
{
	time_t ti;
	char *p;
	char buf2[32];

	muxSvrConnectPrintf(svrConn, "RTSP/1.0 %d %s\r\n", error_number, RTSP_STATUS_CODE2STRING(error_number) );
	muxSvrConnectPrintf(svrConn, "CSeq: %d\r\n", svrConn->seq);

	/* output GMT time */
	ti = time(NULL);
	p = ctime(&ti);
	strcpy(buf2, p);
	p = buf2 + strlen(p) - 1;
	
	if (*p == '\n')
		*p = '\0';
	
	muxSvrConnectPrintf(svrConn, "Date: %s GMT\r\n", buf2);
	
}


void muxSvrRemoveAutheninfoFromUrl(char* url)
{
	/* remove username and password in url */
	char* url_prefix = "rtsp://";
	char* url_address = NULL;
	int url_start_index = 0;
#if 0	/* invalidate assign array length, lizhijie, 2008.07.17 */
	int url_length  = strlen(url);
	char url_result[url_length];
	memset(url_result, '\0', url_length);
	strcpy(url_result, url);
#else
	char url_result[1024];
	memset(url_result, 0, 1024);//pstrcpy;
	av_strlcpy(url_result, url, 1024);
#endif

	url_address = strchr(url_result, '@');

	if(url_address)
	{
		url_start_index = strlen(url_result) - strlen(url_address) + 1;
		strcpy(url, url_prefix);

		url_address++;
		strcat(url, url_address);

		url_start_index = url_start_index;
	}

	return;
}


/* RTP handling */
DATA_CONNECT *muxSvrRtpNewConnection(SERVER_CONNECT *svrConn, int streamIndex, RTSPMessageHeader *rtspHeader )
{
	DATA_CONNECT *conn = NULL;
	RTSPTransportField *th = NULL;
	const char *proto_str;

	/* always prefer UDP */
	th = find_transport(rtspHeader, RTSP_LOWER_TRANSPORT_UDP);//RTSP_PROTOCOL_RTP_UDP);
	if (!th)
	{
		th = find_transport(rtspHeader, RTSP_LOWER_TRANSPORT_TCP);// RTSP_PROTOCOL_RTP_TCP);
	}

	/* check transport */
	if (!th || (th->lower_transport == RTSP_LOWER_TRANSPORT_UDP && th->client_port_min <= 0))
	{
		muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_TRANSPORT);
		return NULL;
	}
		
	conn = av_mallocz( sizeof(DATA_CONNECT));//new_connect_context(-1, &svrConn->from_addr, svrConn->server);
	if(conn == NULL)
	{
		MUX_SVR_ERROR( "New RTP Connection Failed");
		muxSvrRtspEventErrorInfo(svrConn, RTSP_STATUS_BANDWIDTH);
		return NULL;
	}

	conn->stream = svrConn->stream;
	conn->streamIndex = streamIndex;
	conn->myParent = svrConn;
	
	av_strlcpy(conn->session_id, rtspHeader->session_id, sizeof(conn->session_id));
//	conn->seq = rtspHeader->seq;	/* seq is just for command, not for stream, lzj */

	conn->is_packetized = 1;
	conn->rtpProtocol = th->lower_transport;

	/* protocol is shown in statistics */
	switch(conn->rtpProtocol)
	{
#if WITH_RTSP_UDP_MULTICAST
		case RTSP_LOWER_TRANSPORT_UDP_MULTICAST:
			proto_str = "MCAST";
			break;
#endif

		case RTSP_LOWER_TRANSPORT_UDP:
			proto_str = "UDP";
			break;
			
		case RTSP_LOWER_TRANSPORT_TCP:
			proto_str = "TCP";
			break;
			
		default:
			proto_str = "???";
			break;
	}
	
//	pstrcpy(conn->protocol, sizeof(conn->protocol), "RTP/");
	av_strlcpy(conn->protocol, "RTP/", sizeof(conn->protocol));
	//pstrcat
	av_strlcatf(conn->protocol, sizeof(conn->protocol), proto_str);

	svrConn->dataConns[streamIndex] = conn;
	/* RTSP has 2 RTP connections, only update one time */
	if( streamIndex == 0)
	{
		svrConn->muxSvr->currentBandwidth += svrConn->stream->bandwidth;
		svrConn->stream->conns_served++;
	}

	ADD_ELEMENT(svrConn->stream->dataConns, conn);
	
	return conn;
}

/* add a new RTP stream in an RTP connection (used in RTSP SETUP
   command). If RTP/TCP protocol is used, TCP connection 'rtsp_c' is used. */
int muxSvrRtpNewOutHandler(DATA_CONNECT *rtpConn, int streamIndex, struct sockaddr_in *destAddr )
{
	AVFormatContext *ctx;
	char *ipaddr;
	URLContext *h;
	uint8_t *dummy_buf;
	char buf2[32];
	AVStream *st;

	if(streamIndex < 0)
	{
		MUX_SVR_ERROR( "Stream Index is invalidate value : %d", streamIndex);
		return -EXIT_FAILURE;
	}

	MUX_SVR_DEBUG( "Create No.%d RTP Connection", streamIndex );
	
	/* now we can open the relevant output stream */
	ctx = avformat_alloc_context();
	if (!ctx)
		return -1;

VTRACE();

	ctx->oformat = av_guess_format("rtp", NULL, NULL);

	av_assert0(ctx->oformat);
	
	st = avformat_new_stream(ctx, NULL);
	if (!st)
		goto fail;
	
	void *st_internal;

	st_internal = st->internal;

//	muxSvrMediaCodec(st, rtpConn->stream->feed->lavStreams[streamIndex]);
//	cmnMediaInitAvStream(st, rtpConn->stream->feed->feedStreams[streamIndex]);

	cmnMediaInitAvStream(st, rtpConn->stream->feed->mediaCapture->capturedMedias[streamIndex]);
	av_assert0(st->priv_data == NULL);
	av_assert0(st->internal == st_internal);

	/* build destination RTP address */
	ipaddr = inet_ntoa(destAddr->sin_addr);

	switch(rtpConn->rtpProtocol)
	{
		case RTSP_LOWER_TRANSPORT_UDP:
#if WITH_RTSP_UDP_MULTICAST
		case RTSP_LOWER_TRANSPORT_UDP_MULTICAST:
			/* RTP/UDP case */

			/* XXX: also pass as parameter to function ? */
			if ( rtpConn->stream->is_multicast)
			{
				int ttl;
				ttl = rtpConn->stream->multicast_ttl;
				if (!ttl)
					ttl = 16;

				snprintf(ctx->filename, sizeof(ctx->filename), "rtp://%s:%d?multicast=1&ttl=%d", ipaddr, ntohs(destAddr->sin_port), ttl);
			}
			else
#endif
			{
				snprintf(ctx->filename, sizeof(ctx->filename), "rtp://%s:%d", ipaddr, ntohs(destAddr->sin_port));
			}

VTRACE();

			if (ffurl_open(&h, ctx->filename, AVIO_FLAG_WRITE /*URL_WRONLY*/, NULL, NULL) < 0)
				goto fail;

			MUX_SVR_DEBUG( "ctx->filename='%s', stream index:%d, max packet size : %d", ctx->filename, streamIndex, h->max_packet_size);
			
			rtpConn->urlContext = h;
			rtpConn->myParent->maxPacketSize = h->max_packet_size;
			break;
		
		case RTSP_LOWER_TRANSPORT_TCP:
			MUX_SVR_DEBUG( "RTP/TCP stream '%s', index: %d", ctx->filename, streamIndex);
			/* RTP/TCP case */
			rtpConn->myParent->maxPacketSize = RTSP_TCP_MAX_PACKET_SIZE;
			break;
		
		default:
			MUX_SVR_ERROR( "Not support RTP low layer protocol: %d", ctx->filename, streamIndex);
			goto fail;
	}

//	MUX_SVR_DEBUG("%s:%d - - \"PLAY %s/streamid=%d %s\"\n", 	ipaddr, ntohs(dest_addr->sin_port), c->stream->filename, stream_index, c->protocol);
	MUX_SVR_DEBUG("%s: %s:%d - - [%s] \"SETUP %s/streamid=%d %d\"", rtpConn->protocol, ipaddr, ntohs(destAddr->sin_port), cmnGetCtime1(buf2, sizeof(buf2)),  rtpConn->stream->filename, streamIndex, rtpConn->rtpProtocol);
//	fprintf(stderr, "%s:%d - - [%s] \"SETUP %s/streamid=%d %d\"\n", ipaddr, ntohs(destAddr->sin_port), cmnGetCtime1(buf2, sizeof(buf2)),  rtpConn->stream->filename, streamIndex, rtpConn->rtpProtocol);

	/* normally, no packets should be output here, but the packet size may be checked */
	if (ffio_open_dyn_packet_buf(&ctx->pb, rtpConn->myParent->maxPacketSize) < 0)
	{
		/* XXX: close stream */
		goto fail;
	}
VTRACE();
	
//	av_set_parameters(ctx, NULL);
	if (avformat_write_header(ctx, NULL) < 0)
	{
fail:
VTRACE();
		if (h)
		{
			rtpConn->urlContext = NULL;
			ffurl_close(h);
		}
		
		av_free(ctx);
		return -EXIT_FAILURE;
	}
	
	avio_close_dyn_buf(ctx->pb, &dummy_buf);
	av_free(dummy_buf);

	rtpConn->outFmtCtx = ctx;
	
	return EXIT_SUCCESS;
}


void muxSvrRtpDataConnClose(DATA_CONNECT *dataConn)
{
	AVFormatContext *ctx;
	URLContext *h;

	MuxStream *stream = dataConn->stream;
	if(stream )
	{
		REMOVE_ELEMENT(stream->dataConns, dataConn, DATA_CONNECT);
	}
	
	MUX_SVR_DEBUG("'%s' : data conn is free OutFmt", dataConn->myParent->name);

	/* free RTP output streams if any */
	ctx = dataConn->outFmtCtx;
	if (ctx)
	{
VTRACE();
//		av_write_trailer(ctx);

		if (avio_open_dyn_buf(&ctx->pb) >= 0)
		{
VTRACE();
			av_write_trailer(ctx);
			avio_close_dyn_buf(ctx->pb, &dataConn->myParent->dataBuffer.dBuffer);
VTRACE();

			av_freep(&dataConn->myParent->dataBuffer.dBuffer);
VTRACE();
		}
		
		/* free CTX and its all AvStreams */
		avformat_free_context(ctx);
	}
	
VTRACE();
	h = dataConn->urlContext;
	if (h)
	{
		MUX_SVR_DEBUG("'%s' : data conn is free UrlContext", dataConn->myParent->name);
		ffurl_close(h);
	}

	/* minized in svrConn */
#if 0
	if(stream) 
		stream->muxSvr->currentBandwidth -= stream->bandwidth;
#endif
	
VTRACE();
	av_freep(&dataConn);

}

int muxSvrRtpDataConnOpen(DATA_CONNECT *dataConn, const char *info)
{
	char buf[128];
#if 0	
	char input_filename[1024];
	AVFormatContext *avCtx;
	int i;
	int buf_size;
#endif	
	
	int64_t stream_pos;

	/* find file name */
	if (dataConn->stream && info != NULL )
	{/* there are feed in the stream of this connection, then get file of feed file and stream position */
//		buf_size = IOBUFFER_INIT_SIZE;
		/* compute position (absolute time) */
		if (av_find_info_tag(buf, sizeof(buf), "date", info))
		{
#if 0		
			stream_pos = parse_date(buf, 1);
#else
			int ret;
			if ((ret = av_parse_time(&stream_pos, buf, 1)) < 0)
			{
				MUX_SVR_ERROR("Invalid date specification '%s' for stream\n", buf);
				return ret;
			}
#endif
		}
		else if (av_find_info_tag(buf, sizeof(buf), "buffer", info))
		{
			int prebuffer = strtol(buf, 0, 10);
			stream_pos = av_gettime() - prebuffer * (int64_t)1000000;
		}
		else
		{
			stream_pos = av_gettime() - dataConn->stream->prebuffer * (int64_t)1000;
		}
#if 0		
		else
		{
			stream_pos = 0;
		}
#endif		
	}
	

#if 0
	{
		time_t when = stream_pos / 1000000;
		MUX_SVR_DEBUG( "Stream pos = %lld, time=%s", stream_pos, ctime(&when));
	}
#endif

#if 0
	if (input_filename[0] == '\0')
		return -1;
	/* open stream */
//	if (av_open_input_file(&avCtx, input_filename, NULL, buf_size, NULL) < 0)
	if(avformat_open_input(&avCtx, input_filename, NULL, NULL) < 0)
	{
		MUX_SVR_ERROR( "%s not found", input_filename);
		return -1;
	}

	/* open each parser */
	for(i=0;i<avCtx->nb_streams;i++)
	{
		_openDecodeParser(avCtx, i);
	}

	/* choose stream as clock source (we favorize video stream if 	present) for packet sending */
	dataConn->pts_stream_index = 0;
	for(i=0; i< dataConn->stream->nbStreams;i++)
	{
		if ( dataConn->pts_stream_index == 0 && dataConn->stream->streams[i]->codec.codec_type == AVMEDIA_TYPE_VIDEO)
		{
			conn->pts_stream_index = i;
		}
	}
#endif

	/* set the start time (needed for maxtime and RTP packet timing) */
	dataConn->startTime = dataConn->myParent->muxSvr->currentTime;
	dataConn->firstPts = AV_NOPTS_VALUE;
	return 0;
}

/* find an rtp connection by using the session ID. Check consistency with filename */
DATA_CONNECT *muxSvrRtpFindConnectWithUrl(char *path, SERVER_CONNECT *svrConn)
{
	DATA_CONNECT *rtpConn;
	char buf[1024];

	for(rtpConn = svrConn->stream->dataConns; rtpConn != NULL; rtpConn = rtpConn->next) 
	{
		//if (!strcmp(rtpConn->session_id, rtspEvent->header.session_id) )//&& header->seq==c->seq )
			//break;

		if(!svrConn->stream->is_multicast)
		{// not use multicast
			if(rtpConn->myParent == svrConn)
			{
				if(!strcmp(path, rtpConn->stream->filename))
					return rtpConn;
			
				snprintf(buf, sizeof(buf), "%s/streamid=%d", rtpConn->stream->filename, rtpConn->streamIndex);
				if(!strncmp(path, buf, sizeof(buf)))
				{// XXX: Should we reply with RTSP_STATUS_ONLY_AGGREGATE if nbStreams>1?
					return rtpConn;
				}
			}
		}
		else
		{// use multicast
			if(!strcmp(path, rtpConn->stream->filename))
				return rtpConn;
			
			snprintf(buf, sizeof(buf), "%s/streamid=%d", rtpConn->stream->filename, rtpConn->streamIndex);
			if(!strncmp(path, buf, sizeof(buf)))
			{
				return rtpConn;
			}			
		}
	}
	
	return NULL;
}




int muxSvrPrepareSdpDescription(MuxStream *stream, uint8_t **sdpDescription, struct in_addr myIp)
{
	AVFormatContext *avc;
	AVOutputFormat *rtp_format = av_guess_format("rtp", NULL, NULL);
	AVDictionaryEntry *entry = av_dict_get(stream->metadata, "title", NULL, 0);
	int i;

	*sdpDescription = NULL;

VTRACE();
	avc =  avformat_alloc_context();
	if (!avc || !rtp_format)
		return -1;

VTRACE();
	avc->oformat = rtp_format;
	av_dict_set(&avc->metadata, "title", entry ? entry->value : "No Title", 0);
#if WITH_RTSP_UDP_MULTICAST
	if (stream->is_multicast)
	{
		snprintf(avc->filename, 1024, "rtp://%s:%d?multicast=1?ttl=%d", inet_ntoa(stream->multicast_ip), stream->multicast_port, stream->multicast_ttl);
	}
	else
#endif
	{
		snprintf(avc->filename, 1024, "rtp://0.0.0.0");	
	}

VTRACE();
//	for(i = 0; i < stream->feed->nbInStreams; i++)
	for(i = 0; i < stream->feed->mediaCapture->nbStreams; i++)
	{
		AVStream *st = avformat_new_stream(avc, NULL);
		if (!st)
			goto sdp_done;

//		muxSvrMediaCodec(st, stream->feed->lavStreams[i]);
//		cmnMediaInitAvStream(st, stream->feed->feedStreams[i]);
		cmnMediaInitAvStream(st, stream->feed->mediaCapture->capturedMedias[i]);
VTRACE();
	}
#define PBUFFER_SIZE 2048

	*sdpDescription = av_mallocz(PBUFFER_SIZE);
	if (!*sdpDescription)
		goto sdp_done;
	av_sdp_create(&avc, 1, (char *)*sdpDescription, PBUFFER_SIZE);

sdp_done:
VTRACE();
	av_freep(&avc->streams);
	av_dict_free(&avc->metadata);
	av_free(avc);

	return *sdpDescription ? strlen((char *)*sdpDescription) : AVERROR(ENOMEM);
}



