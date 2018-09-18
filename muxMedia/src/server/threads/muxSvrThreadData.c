/*
* $Id$
* HTTP and RTSP Server handlers defined in this file
*/

#include "muxSvr.h"


/* in bit/s */
#define SHORT_TERM_BANDWIDTH		8000000


/* send encoded buffer out  */   
int _serviceConnectSendOut(SERVER_CONNECT *svrConn )
{
	int len = 0, dt;
	int total = SVR_BUF_LENGTH(svrConn->dataBuffer);//svrConn->dataBuffer.bufferEnd - svrConn->dataBuffer.bufferPtr;
	int	left = total ;

//	while( (SVR_BUF_LENGTH(svrConn->dataBuffer)) > 0 )
	if( (SVR_BUF_LENGTH(svrConn->dataBuffer)) > 0 )
	{
		left = SVR_BUF_LENGTH(svrConn->dataBuffer);
#if MUX_SVR_DEBUG_DATA_FLOW
		MUX_SVR_DEBUG("'%s' : %d bytes wait to send out on stream %d", svrConn->name, left, svrConn->currentStreamIndex);
#endif
		if( IS_SVR_CONN_TYPE(svrConn, CONN_TYPE_RTMP) )
		{
			len = ffurl_write(svrConn->urlContext, svrConn->dataBuffer.bufferPtr, left);
		}
		else if( IS_SVR_CONN_TYPE(svrConn, CONN_TYPE_HTTP) )
		{
#if MUX_SVR_DEBUG_DATA_FLOW
//			VTRACE();
#endif
			len = muxSvrConnectDataWriteFd(svrConn, svrConn->dataBuffer.bufferPtr, left );
		}
		else// if (CONNECT_IS_RTP(dataConn) )
		{/* RTP data output */
		/* for UDP multicast, RTSP connection */
			DATA_CONNECT *dataConn = svrConn->dataConns[svrConn->currentStreamIndex];
				
			if(  left < 4)
			{/* fail safe - should never happen */
				MUX_SVR_ERROR("'%s' WARNS: Length of media data is too small : %d", svrConn->name, left);
fail1:
				svrConn->dataBuffer.bufferPtr = svrConn->dataBuffer.bufferEnd;
				return 0;
			}
			
			len = (svrConn->dataBuffer.bufferPtr[0] << 24) |(svrConn->dataBuffer.bufferPtr[1] << 16) |(svrConn->dataBuffer.bufferPtr[2] << 8) |(svrConn->dataBuffer.bufferPtr[3]);
			if (len > left )
			{
				MUX_SVR_ERROR("'%s' WARNS: Length of this sub-packet data %d is larger than left %d", svrConn->name, len, left);
				goto fail1;
			}

#if MUX_SVR_DEBUG_DATA_FLOW
			MUX_SVR_DEBUG("'%s' : Length of this sub-packet : %d on stream %d", svrConn->name, len, svrConn->currentStreamIndex );
#endif
			if(IS_SVR_CONN_TYPE(svrConn, CONN_TYPE_RTSP) && (dataConn->rtpProtocol == RTSP_LOWER_TRANSPORT_TCP ) )
			{/* RTP packets are sent inside the RTSP TCP connection */
				AVIOContext	*pb;

				int interleaved_index, size;
				uint8_t header[4];
				uint8_t *nBuffer;

				/* if already sending something, then wait. */
				if (svrConn->state !=  RTSP_STATE_PLAY)//CONN_STATE_WAIT_REQUEST)
				{
VTRACE();
					return 0;
				}

				if(avio_open_dyn_buf(&pb) < 0)
				{
VTRACE();
					return 0;
				}

				
//				interleaved_index = dataConn->packet_stream_index * 2;
				interleaved_index = svrConn->currentStreamIndex * 2;
				
				/* RTCP packets are sent at odd indexes */
				if( svrConn->dataBuffer.bufferPtr[1] == 200)
					interleaved_index++;
				
				/* write RTSP TCP header */
				//rfc-2326 10.12 Embeded(Interleaved) Binary Data.
				header[0] = '$';
				header[1] = interleaved_index;
				header[2] = len >> 8;
				header[3] = len;
				avio_write(pb, header, 4);
				
				/* write RTP packet data */
//				svrConn->dataBuffer.bufferPtr;
				avio_write(pb, svrConn->dataBuffer.bufferPtr +4, len );
				size = avio_close_dyn_buf(pb, &nBuffer);

#if MUX_SVR_DEBUG_DATA_FLOW
				MUX_SVR_DEBUG( "'%s' : %d bytes sub-packet length, sending %d bytes on stream %d", svrConn->name, len , size, svrConn->currentStreamIndex);
#endif				
				/* prepare asynchronous TCP sending */
				len = muxSvrConnectDataWriteFd(svrConn, nBuffer, size );
#if MUX_SVR_DEBUG_DATA_FLOW
				MUX_SVR_DEBUG( "'%s' : %d bytes sending, %d bytes writted on stream %d", svrConn->name, size, len , svrConn->currentStreamIndex);
#endif
				if(len > 0)
				{ /* ignore error of some bytes */
					len = size;
				}
			}
			else
			{/* RTP/UDP or UDP multicast: send RTP packet directly in UDP */
				/* short term bandwidth limitation */
				URLContext *urlCtx = svrConn->urlContext;

#if MUX_SVR_DEBUG_DATA_FLOW
				VTRACE();
#endif
				if(dataConn)
				{
					dt = av_gettime() - dataConn->packet_start_time_us;
					if (dt < 1)
						dt = 1;
					urlCtx = dataConn->urlContext;
				}

#if 0
				if ((dataConn->packet_byte_count + len) * (int64_t)1000000 >= 	(SHORT_TERM_BANDWIDTH / 8) * (int64_t)dt)
				{
					/* bandwidth overflow : wait at most one tick and retry */
	//				dataConn->state = RTP_STATE_WAIT_SHORT;
					return 0;
				}
				{
					struct timeval time;
					char buf[30];
					gettimeofday(&time,NULL);
					strftime(buf,30,"%d-%m-%Y,  %T.",localtime(&time));
					printf("send data at : %s\n",buf);
				}
#endif
				svrConn->dataBuffer.bufferPtr += 4;
				len = ffurl_write(urlCtx, svrConn->dataBuffer.bufferPtr, len );
			}

		}
		
#if MUX_SVR_DEBUG_DATA_FLOW
		MUX_SVR_DEBUG( "'%s' : %d bytes have been writted on stream %d", svrConn->name, len , svrConn->currentStreamIndex);
#endif
		if( len < 0)
		{
			perror("send media packet error");
			MUX_SVR_ERROR("'%s' : error on stream %d of  is %s", svrConn->name, svrConn->currentStreamIndex, av_err2str(len));
			HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_ERROR, SRV_HTTP_ERRCODE_CLOSED_BY_PEER);
			//break;
			return EXIT_SUCCESS; /* continue on next SvrConn for this packet */
		}
		
		svrConn->dataBuffer.bufferPtr += len;
//		svrConn->packet_byte_count += len;
		
		svrConn->dataCount += len;
	
	//	update_datarate(&dataConn->datarate, dataConn->dataCount, dataConn->server);

	}

#if MUX_SVR_DEBUG_DATA_FLOW
//	VTRACE();
#endif

	return EXIT_SUCCESS;
}

/* pkt are encoded into buffer */
static int _serviceConnectEncode(SERVER_CONNECT *svrConn, AVPacket *pkt)
{
	AVFormatContext *outFmt = NULL;
	AVStream *ost;
	AVRational sourceTimebase = AV_TIME_BASE_Q;
	int ret, len;

	
	/* store raw info of AVPacket */
	svrConn->currentStreamIndex = pkt->stream_index;
	
	if( IS_RTSP_CONNECT(svrConn) )
	{
		if( svrConn->dataConns[pkt->stream_index]==NULL)
		{
			MUX_SVR_ERROR("'%s' : No.%s Data Conn is null", svrConn->name );
			exit(1);
		}
		outFmt = svrConn->dataConns[pkt->stream_index]->outFmtCtx;
		ost = outFmt->streams[0];
	}
	else
	{
		outFmt = svrConn->outFmtCtx;
		ost = outFmt->streams[pkt->stream_index];
	}

	if(svrConn->stream->feed->type == MUX_FEED_TYPE_FILE)
	{
		AVStream *ist = svrConn->stream->feed->inputCtx->streams[pkt->stream_index];
		sourceTimebase = ist->time_base;
	}
	else
	{/* use same time_base, so don't change PTS/DTS between source(input) and dest(output) */
		sourceTimebase = ost->time_base;
	}

#if MUX_SVR_DEBUG_TIME_STAMP
	MUX_SVR_DEBUG("'%s' : raw (from media file) dts : %s, pts : %s ", svrConn->name, av_ts2str(pkt->dts), av_ts2str(pkt->pts) );
#endif
	pkt->dts = pkt->dts + svrConn->lastDTSes[pkt->stream_index];
	pkt->pts = pkt->pts + svrConn->lastDTSes[pkt->stream_index];
	
	/* before calculate new dts/pts/duration of this packet */
	/* update first pts if needed */
	if (svrConn->firstPts == AV_NOPTS_VALUE && pkt->dts != AV_NOPTS_VALUE)
	{
		svrConn->firstPts = av_rescale_q(pkt->dts, sourceTimebase, AV_TIME_BASE_Q);
		svrConn->startTime = svrConn->muxSvr->currentTime;
	}
#if MUX_SVR_TIMING_PACKET	
	/* compute send time and duration */
	if (pkt->dts != AV_NOPTS_VALUE)
	{
		svrConn->currentFramePts = av_rescale_q(pkt->dts, sourceTimebase, AV_TIME_BASE_Q);
		svrConn->currentFramePts -= svrConn->firstPts;
	}
	svrConn->currentFrameDuration = av_rescale_q(pkt->duration, sourceTimebase, AV_TIME_BASE_Q);
#endif


#if MUX_SVR_DEBUG_TIME_STAMP
	MUX_SVR_DEBUG("'%s' : before rescale dts : %s, pts : %s ", svrConn->name, av_ts2str(pkt->dts), av_ts2str(pkt->pts) );
#endif

	if (pkt->dts != AV_NOPTS_VALUE)
	{
		pkt->dts = av_rescale_q(pkt->dts, sourceTimebase, ost->time_base);
//		pkt->dts = av_rescale_q(pkt->dts+ost->cur_dts +  AV_NOPTS_VALUE+ ost->time_base.num * (int64_t)ost->time_base.den, ist->time_base, ost->time_base);
	}
	else
	{
//		MUX_SVR_DEBUG("dts is AV_NOPTS_VALUE");
	}
	
	if (pkt->pts != AV_NOPTS_VALUE)
	{
		pkt->pts = av_rescale_q(pkt->pts, sourceTimebase, ost->time_base);
//		pkt->pts = av_rescale_q(pkt->pts+ost->cur_dts + AV_NOPTS_VALUE+ ost->time_base.num * (int64_t)ost->time_base.den , ist->time_base, ost->time_base);
	}
	else
	{
//		MUX_SVR_DEBUG("pts is AV_NOPTS_VALUE");
	}

#if MUX_SVR_DEBUG_TIME_STAMP
	MUX_SVR_DEBUG("'%s' : after rescale dts : %s, pts : %s ", svrConn->name, av_ts2str(pkt->dts), av_ts2str(pkt->pts) );
#endif

	pkt->duration = av_rescale_q(pkt->duration, sourceTimebase, ost->time_base);

#if 0
	if( IS_SVR_CONN_TYPE(svrConn, CONN_TYPE_RTSP) ||
		IS_SVR_CONN_TYPE(svrConn, CONN_TYPE_MULTICAST) )//CONNECT_IS_RTP(dataConn))
#else
	if(svrConn->maxPacketSize)
#endif
	{/* for UDP multicast, RTSP(RTP/UDP, RTP/TCP)*/
		int max_packet_size = svrConn->maxPacketSize;

		if(IS_RTSP_CONNECT(svrConn) )
		{
			pkt->stream_index = 0;
		}
#if MUX_SVR_DEBUG_DATA_FLOW
		MUX_SVR_DEBUG("'%s' : max packet size :%d for stream %d", svrConn->name, max_packet_size, svrConn->currentStreamIndex);
#endif
		ret = ffio_open_dyn_packet_buf(&outFmt->pb, max_packet_size);
//		dataConn->packet_byte_count = 0;
//		dataConn->packet_start_time_us = av_gettime();

	}
	else
	{
//		VTRACE();
		ret = avio_open_dyn_buf(&outFmt->pb);
	}
	if (ret < 0)
	{
		MUX_SVR_ERROR("'%s' : Error in open_dyn_buf : %s\n", svrConn->name, av_err2str(ret));
	}
	
	outFmt->pb->seekable = 0;
	ret = av_write_frame( outFmt, pkt);
	if (ret < 0)
	{/* XXX: potential leak */
		MUX_SVR_ERROR("'%s' : Error writing frame to output : %s\n", svrConn->name, av_err2str(ret));
		av_freep(&svrConn->dataBuffer.dBuffer);
		return ret;
	}
	
	av_freep(&svrConn->dataBuffer.dBuffer);

	len = avio_close_dyn_buf(outFmt->pb, &svrConn->dataBuffer.dBuffer );
#if MUX_SVR_DEBUG_DATA_FLOW
	MUX_SVR_DEBUG("'%s' : avio_close_dyn_buf returned buffer is %d bytes (packet length :%d) on stream %d", svrConn->name, len, pkt->size, svrConn->currentStreamIndex );
	if(len < pkt->size &&  !IS_SVR_CONN_TYPE(svrConn, CONN_TYPE_HLS) && !IS_SVR_CONN_TYPE(svrConn, CONN_TYPE_MULTICAST) )
	{
		MUX_SVR_ERROR("'%s' : buffer is %d, smaller than packet length %d\n", svrConn->name, len, pkt->size );
	}

#endif
	outFmt->pb = NULL;
	svrConn->dataBuffer.bufferPtr = svrConn->dataBuffer.dBuffer;
	svrConn->dataBuffer.bufferEnd = svrConn->dataBuffer.dBuffer + len;

#if MUX_SVR_TIMING_PACKET	
	svrConn->currentFrameBytes = len;
#endif

	return len;
}

int muxSvrConnSendPacket(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *_event)
{
	int len;
	AVPacket		*pkt;

	int	left = SVR_BUF_LENGTH(svrConn->dataBuffer);
	if( left > 0)
	{
#if 0
		len = muxSvrConnectDataWriteFd(svrConn, svrConn->dataBuffer.bufferPtr, left);
		if(len < 0)
		{
			MUX_SVR_ERROR("Write media data for '%s' error: %s: left: %d", svrConn->name, strerror(errno), left);
			HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_ERROR, SRV_HTTP_ERRCODE_CLOSED_BY_PEER);
		}
		else
		{
			svrConn->dataBuffer.bufferPtr += len;
		}
#else		
#if MUX_SVR_DEBUG_DATA_FLOW
		MUX_SVR_DEBUG("'%s' : Send out left %d bytes data on stream %d", svrConn->name, left, svrConn->currentStreamIndex );
#endif
		len = _serviceConnectSendOut(svrConn);
		if(len < 0)
		{
			MUX_SVR_ERROR("'%s' : Error in send out left data buffer error on ", svrConn->name);
			HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_CLOSE, 0);
		}
#endif
		return MUX_SVR_STATE_CONTINUE;
	}

	pkt = (AVPacket *)cmn_fifo_tryget(svrConn->fifo);
	if(!pkt)
	{
		return MUX_SVR_STATE_CONTINUE;
	}
	
//VTRACE();
#if MUX_SVR_DEBUG_DATA_FLOW
	MUX_SVR_DEBUG("'%s' : send out a new packet: %d on stream %d", svrConn->name, pkt->size, pkt->stream_index);
#endif
	len = _serviceConnectEncode(svrConn, pkt);
	av_packet_free(&pkt);
	if(len < 0)
	{
//		MUX_SVR_ERROR("'%s' : Encode AVPacket error", svrConn->name);
//		HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_CLOSE, 0);
		return MUX_SVR_STATE_CONTINUE;
	}
	
	len = _serviceConnectSendOut(svrConn);
	if(len < 0)
	{
		MUX_SVR_ERROR("'%s' : send out data buffer error", svrConn->name);
		HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_CLOSE, 0);
	}

	return MUX_SVR_STATE_CONTINUE;
}


