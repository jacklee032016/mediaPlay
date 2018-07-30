/*
* $Id$
*/

#include "muxSvr.h"

#include "__services.h"
#include <assert.h>

/* RTP handling */

/* only for HTTP connection of AVStream  */
//int httpEventLiveStreamDataHeader(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
int httpLiveSendDataHeader(SERVER_CONNECT *svrConn )
{
	AVDictionary	*headerOptions = NULL;
	AVFormatContext *ctx;
	int len;
	int i;
	int ret;
	
VTRACE();
	if(svrConn->dataBuffer.dBuffer)
	{
		av_freep(&svrConn->dataBuffer.dBuffer);
	}
	
VTRACE();
	ctx = avformat_alloc_context();
	if (!ctx)
	{
		assert(0);
		return -EXIT_FAILURE;
	}
	
	av_dict_copy(&ctx->metadata, svrConn->stream->metadata, 0);


	ctx->oformat = svrConn->stream->fmt;	/* AVI or RTP */
//	ctx->oformat = av_guess_format("mpegts", NULL, NULL);
	av_assert0(ctx->oformat);

	MUX_SVR_DEBUG("HTTP Connection output format '%s'", ctx->oformat->name );

	svrConn->outFmtCtx = ctx;
	for(i=0; i< svrConn->stream->feed->nbInStreams; i++)
	{
		AVStream *st;

		st = avformat_new_stream(ctx, NULL);
		if (!st)
		{
			assert(0);
		}
		
		void *st_internal;

		st_internal = st->internal;

VTRACE();
//		muxSvrMediaCodec(st, svrConn->stream->feed->lavStreams[i]);

		muxMediaInitAvStream(st, svrConn->stream->feed->feedStreams[i]);
		av_assert0(st->priv_data == NULL);
		av_assert0(st->internal == st_internal);

#if MUX_WITH_AVI_H264_ANNEXB
		if(IS_STREAM_FORMAT(ctx->oformat, "avi") && (st->codecpar->codec_id == AV_CODEC_ID_H264) )
		{
			ff_stream_add_bitstream_filter(st, "h264_mp4toannexb", NULL);
			st->internal->bitstream_checked = 1;
			ctx->flags |= AVFMT_FLAG_AUTO_BSF;
		}
#endif		

#if MUX_WITH_CODEC_FIELD
		st->codec->frame_number = 0; /* XXX: should be done in AVStream, not in codec */
		/* I'm pretty sure that this is not correct...
		* However, without it, we crash
		*/
		st->codec->coded_frame = &dummy_frame;
#endif

	}

VTRACE();
	svrConn->got_key_frame = 0;

	svrConn->startTime = svrConn->muxSvr->currentTime;
	svrConn->firstPts = AV_NOPTS_VALUE;

	/* prepare header and save header data in a stream */
	if (avio_open_dyn_buf(&svrConn->outFmtCtx->pb) < 0)
	{/* XXX: potential leak */
		return -EXIT_FAILURE;
	}
	
//	svrConn->outFmtCtx->pb->is_streamed = 1;
	ctx->oformat->flags |= AVFMT_ALLOW_FLUSH;

#if 0
	/* options in avformat_write_header only has effective on FormatContext or OutoutFormat, can't work with BSF */
	if(IS_STREAM_FORMAT(ctx->oformat, "avi") )
	{
		MUX_SVR_DEBUG("'%s' : add BSF for h264_mp4toannexb in AVI output format", svrConn->name );
		ret = av_dict_set(&headerOptions, "fflags", "-autobsf", 0);
		ret |= av_dict_set(&headerOptions, "bitstream_filters", "h264_mp4toannexb", 0);
		if (ret < 0)
		{
			MUX_SVR_LOG("Set dictionary failed: %s\n", av_err2str(ret));
		}
	}
#endif

//	av_set_parameters(&svrConn->outFmtCtx, NULL);
	ret = avformat_write_header(svrConn->outFmtCtx, &headerOptions);
	if (ret < 0)
	{
		MUX_SVR_LOG("Error write header in HTTP connections: %s\n", av_err2str(ret));
		return -EXIT_FAILURE;
	}
	
	len = avio_close_dyn_buf(svrConn->outFmtCtx->pb, &svrConn->dataBuffer.dBuffer);
	svrConn->dataBuffer.bufferPtr = svrConn->dataBuffer.dBuffer;
	svrConn->dataBuffer.bufferEnd = svrConn->dataBuffer.dBuffer + len;

	svrConn->last_packet_sent = 0;
		
VTRACE();
	muxSvrConnectDataWriteFd(svrConn, svrConn->dataBuffer.bufferPtr, SVR_BUF_LENGTH(svrConn->dataBuffer));

	ENABLE_SVR_CONN(svrConn->stream->feed, svrConn);
	
	SVR_BUF_RESET(svrConn->dataBuffer);

	return EXIT_SUCCESS;
}

int httpEventLiveStream(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	const char *mime_type = NULL;
	int ret;

	/* prepare http header */
	muxSvrConnectPrintf(svrConn, "HTTP/1.0 200 OK\r\n");
	mime_type = svrConn->stream->fmt->mime_type;
	if (!mime_type)
		mime_type = "application/x-octet_stream";
	
	muxSvrConnectPrintf(svrConn, "Pragma: no-cache\r\n");

	/* for asf, we need extra headers */
	if (!strcmp(svrConn->stream->fmt->name,"asf_stream"))
	{/* Need to allocate a client id */
		svrConn->wmp_client_id = random() & 0x7fffffff;

		muxSvrConnectPrintf(svrConn, "Server: Cougar 4.1.0.3923\r\nCache-Control: no-cache\r\nPragma: client-id=%d\r\nPragma: features=\"broadcast\"\r\n", svrConn->wmp_client_id);
	}
	
	muxSvrConnectPrintf(svrConn, "Content-Type: %s\r\n", mime_type);
	muxSvrConnectPrintf(svrConn, "\r\n");

	MUX_SVR_LOG( "Connection enter into state of 'HTTPSTATE_SEND_HEADER'");

	muxSvrConnectFlushOut(svrConn);

	SVR_BUF_RESET(svrConn->dataBuffer);

	ret = httpLiveSendDataHeader(svrConn);
	if(ret == EXIT_SUCCESS)
		return HTTP_STATE_DATA;
	return MUX_SVR_STATE_CLOSING;
}


/* only for HTTP/TCP protocol */
//int httpEventLiveStreamDataTailer(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
void httpLiveSendDataTailer(SERVER_CONNECT *svrConn )
{
	int len;
	AVFormatContext *ctx;

	DISABLE_SVR_CONN(svrConn->stream->feed, svrConn);
	
	av_freep(&svrConn->dataBuffer.dBuffer);
	if (svrConn->last_packet_sent ||CONNECT_IS_RTP(svrConn) )
		return;
	
	ctx = svrConn->outFmtCtx;
	/* prepare header */
	if (avio_open_dyn_buf(&ctx->pb) < 0)
	{/* XXX: potential leak */
		return;
	}
	
	av_write_trailer(ctx);
	len = avio_close_dyn_buf(ctx->pb, &svrConn->dataBuffer.dBuffer);
	svrConn->dataBuffer.bufferPtr = svrConn->dataBuffer.dBuffer;
	svrConn->dataBuffer.bufferEnd = svrConn->dataBuffer.dBuffer + len;

	svrConn->last_packet_sent = TRUE;

	muxSvrConnectDataWriteFd(svrConn, svrConn->dataBuffer.bufferPtr, SVR_BUF_LENGTH(svrConn->dataBuffer)) ;
	
	SVR_BUF_RESET(svrConn->dataBuffer);

	return;
}


static void _httpReplyError(SERVER_CONNECT *svrConn, char *errMsg, int errNo )
{
	/* http errNo */
	switch(errNo)
	{
		case 400:
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 400 Error Request\r\n");
			muxSvrConnectPrintf(svrConn, "Content-type: text/html\r\n\r\n");
			muxSvrConnectPrintf(svrConn, "<HTML>\n<HEAD><TITLE>400 Error Request</TITLE></HEAD>\n");
			break;
		case 401:
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 401  Failed Authentication\r\n");
			muxSvrConnectPrintf(svrConn,  "WWW-Authenticate: Basic realm=\"%s\"\r\n", "SONiX Streaming Media");
			muxSvrConnectPrintf(svrConn, "Content-type: text/html\r\n\r\n");
			muxSvrConnectPrintf(svrConn, "<HTML>\n<HEAD><TITLE>401 Failed Authentication</TITLE></HEAD>\n");
			break;
		case 403:
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 403  Access Deny\r\n");
			muxSvrConnectPrintf(svrConn, "Content-type: text/html\r\n\r\n");
			muxSvrConnectPrintf(svrConn, "<HTML>\n<HEAD><TITLE>403 Access Deny</TITLE></HEAD>\n");
			break;
		case 404:
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 404 Not Found\r\n");
			muxSvrConnectPrintf(svrConn, "Content-type: text/html\r\n\r\n");
			muxSvrConnectPrintf(svrConn, "<HTML>\n<HEAD><TITLE>404 Not Found</TITLE></HEAD>\n");
			break;
		case 450:
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 450 Server Busy Now\r\n");
			muxSvrConnectPrintf(svrConn, "Content-type: text/html\r\n\r\n");
			muxSvrConnectPrintf(svrConn, "<HTML>\n<HEAD><TITLE>450 Server Busy Now</TITLE></HEAD>\n");
			break;
		case 505:
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 505 Unsupport Http Version\r\n");
			muxSvrConnectPrintf(svrConn, "Content-type: text/html\r\n\r\n");
			muxSvrConnectPrintf(svrConn, "<HTML>\n<HEAD><TITLE>505 Unsupport Http Version</TITLE></HEAD>\n");
			break;
		default:
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 400 Error Request\r\n");
			muxSvrConnectPrintf(svrConn, "Content-type: text/html\r\n\r\n");
			muxSvrConnectPrintf(svrConn, "<HTML>\n<HEAD><TITLE>400 Error Request</TITLE></HEAD>\n");
			break;		
	}
	muxSvrConnectPrintf(svrConn, "<BODY>%s</BODY>\n</HTML>\n", errMsg);

	MUX_SVR_LOG( "HTTP Request error: %s", errMsg);
}

int httpEventError(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	//char nonce[33];
	char	msg[1024];
	//memset(nonce,'\0', 33);

	if(event->statusCode == SRV_HTTP_ERRCODE_NO_ERROR || event->statusCode == SRV_HTTP_ERRCODE_CLOSED_BY_PEER)
	{
		return MUX_SVR_STATE_CLOSING;
	}


	switch(event->statusCode )
	{
		case SRV_HTTP_ERRCODE_UNAUTHORIZED:
			_httpReplyError(svrConn, "401 Unauthorized", 401);
			break;

		case SRV_HTTP_ERRCODE_IP_ACL_DENY:
			_httpReplyError(svrConn, "Remote IP Address is denied", 403);
			break;

		case SRV_HTTP_ERRCODE_UNSUPPORT_HTTP_METHOD:
			snprintf(msg, sizeof(msg), "Not support HTTP Request method : %s", svrConn->method);
			_httpReplyError(svrConn, msg, 400);
			break;
			
		case SRV_HTTP_ERRCODE_UNSUPPORT_PROTOCOL:
			snprintf(msg, sizeof(msg), "Not support HTTP Protocol or version : %s<p>\r\n", svrConn->protocol );
			_httpReplyError(svrConn, msg, 505);
			break;

		case SRV_HTTP_ERRCODE_INVALIDATE_STREAM:
			snprintf(msg, sizeof(msg), "Input stream corresponding to '%s' not found", svrConn->filename );
			_httpReplyError(svrConn,  msg, 404);
			break;

		case SRV_HTTP_ERRCODE_FILE_NOT_FOUND:
			snprintf(msg, sizeof(msg), "File '%s' not found", svrConn->filename );
			_httpReplyError(svrConn, msg, 404);
			break;
			
		case SRV_HTTP_ERRCODE_REDIRECT_FAILED:
			_httpReplyError(svrConn, "ASX/RAM file not handled", 404);
			break;
			
		case SRV_HTTP_ERRCODE_RESOURCE_UNAVAILABE:
			snprintf(msg, sizeof(msg), "File '%s' : resource is not available now", svrConn->url  );
			_httpReplyError(svrConn, msg, 450);
			break;
			
		default:
			snprintf(msg, sizeof(msg), "File '%s' : unknown error", svrConn->filename  );
			_httpReplyError(svrConn, msg, 400);
			break;
	}

	muxSvrConnectFlushOut( svrConn);
	
	return MUX_SVR_STATE_CLOSING;
}

int httpEventInfo(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	switch(event->statusCode )
	{
		case SRV_HTTP_INFOCODE_REDIRECT:
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 301 Moved\r\nLocation: %s\r\n", svrConn->stream->filename);
			muxSvrConnectPrintf(svrConn, "Content-type: text/html\r\n\r\n<html><head><title>Moved</title></head><body>\r\n");
			muxSvrConnectPrintf(svrConn, "You should be <a href=\"%s\">redirected</a>.\r\n</body></html>\r\n", svrConn->stream->filename);
			break;
			
		case SRV_HTTP_INFOCODE_BANDWIDTH_LIMITED:
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 200 Server too busy\r\nContent-type: text/html\r\n\r\n");
			muxSvrConnectPrintf(svrConn, "<html><head><title>Too busy</title></head><body>\r\n");
			muxSvrConnectPrintf(svrConn, "The server is too busy to serve your request at this time.<p>\r\n");
			muxSvrConnectPrintf(svrConn, "The bandwidth being served (including your stream) is %" PRId64 " kbit/sec, and this exceeds the limit of %" PRId64 " kbit/sec\r\n",
				svrConn->muxSvr->currentBandwidth, svrConn->muxSvr->svrConfig.maxBandwidth);
			muxSvrConnectPrintf(svrConn, "</body></html>\r\n");

		default:
			break;
	}

	muxSvrConnectFlushOut( svrConn);
	
	return MUX_SVR_STATE_CLOSING;
}


int httpEventRedirectParsed(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	MuxStream  *stream;
	uint8_t	*content;

VTRACE();
	muxSvrGetHostIP(svrConn);

VTRACE();
	/* skip the char of '/' in path */
//	stream = muxSvrFoundStream(TRUE, svrConn->method+1, svrConn->muxSvr);
	stream = muxSvrFoundStream(TRUE, svrConn->filename, svrConn->muxSvr);
	
	switch(event->statusCode )
	{
		case REDIR_ASX:
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 200 ASX Follows\r\nContent-type: video/x-ms-asf\r\n\r\n");
			muxSvrConnectPrintf(svrConn, "<ASX Version=\"3\">\r\n<!-- Autogenerated by mediaserver -->\r\n");
			muxSvrConnectPrintf(svrConn, "<ENTRY><REF HREF=\"http://%s/%s%s\"/></ENTRY>\r\n</ASX>\r\n", inet_ntoa(svrConn->muxSvr->myAddr.sin_addr), svrConn->method, svrConn->info);
			break;
			
		case REDIR_RAM:
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 200 RAM Follows\r\nContent-type: audio/x-pn-realaudio\r\n\r\n");
			muxSvrConnectPrintf(svrConn, "# Autogenerated by mediaserver\r\nhttp://%s/%s%s\r\n", inet_ntoa(svrConn->muxSvr->myAddr.sin_addr), svrConn->method, svrConn->info);
			break;
		
		case REDIR_ASF:
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 200 ASF Redirect follows\r\nContent-type: video/x-ms-asf\r\n\r\n");
			muxSvrConnectPrintf(svrConn, "[Reference]\r\nRef1=http://%s/%s%s\r\n", inet_ntoa(svrConn->muxSvr->myAddr.sin_addr), svrConn->method, svrConn->info);
			break;
			
		case REDIR_RTSP:
		{/* unicast */
			if(stream == NULL)
			{
				HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_ERROR, SRV_HTTP_ERRCODE_FILE_NOT_FOUND);
				return MUX_SVR_STATE_CONTINUE;
			}
			
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 200 RTSP Redirect follows\r\n");
			/* XXX: incorrect mime type ? */
			muxSvrConnectPrintf(svrConn, "Content-type: application/x-rtsp\r\n\r\n");
			muxSvrConnectPrintf(svrConn, "rtsp://%s:%d/%s\r\n", inet_ntoa(svrConn->muxSvr->myAddr.sin_addr), 
				svrConn->muxSvr->svrConfig.rtspAddress.port, stream->filename);
		}
		break;
		
		case REDIR_SDP:
		{/* multicast */
			int len;

			if(stream == NULL)
			{
				HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_ERROR, SRV_HTTP_ERRCODE_FILE_NOT_FOUND);
				return MUX_SVR_STATE_CONTINUE;
			}
			muxSvrConnectPrintf(svrConn, "HTTP/1.0 200 OK\r\nContent-type: application/sdp\r\n\r\n");

			/* XXX: should use a dynamic buffer */
			len = muxSvrPrepareSdpDescription(stream, &content, svrConn->muxSvr->myAddr.sin_addr );
			if (content == NULL || len< 0)
			{
				HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_ERROR, SRV_HTTP_ERRCODE_RESOURCE_UNAVAILABE);
				return MUX_SVR_STATE_CONTINUE;
			}
VTRACE();
			muxSvrConnectPrintf(svrConn, "%s", content);
			av_free(content);
			
VTRACE();
		}
		break;
		
		default:
			av_assert0(0);
			break;
	}

	muxSvrConnectFlushOut(svrConn);

	return MUX_SVR_STATE_CLOSING;
}


/* HTTP service connection is broken, so closing this connection */
int httpEventClosing(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
#if 0	
	int i;
	DATA_CONNECT 	*dataConn;
//	SERVER_INFO		*server = svrConn->server;
//	CONNECTION 		*conn;

	/* link has been broken, so no data need to send out */
	for(i=0; i< svrConn->stream->feed->outputFile->nbStreams;i++)
	{
		dataConn = svrConn->dataConns[i];
		REMOVE_ELEMENT( svrConn->stream->dataConns, dataConn, DATA_CONNECT);
		svrConn->stream->conns_served--;
		svrConn->muxSvr->current_bandwidth -= svrConn->stream->bandwidth;
		av_free(dataConn);
		
	}
#endif

	/* service connection can not be remove and freed, because statemachine will used this connection */
//	muxSvrCloseConnect(server);

	return MUX_SVR_STATE_CLOSING;
}

