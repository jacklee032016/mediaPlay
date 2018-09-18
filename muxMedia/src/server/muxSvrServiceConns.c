/*
* $Id$
*/

#include "muxSvr.h"


int __startOutputFormat(SERVER_CONNECT *svrConn)
{
	AVDictionary	*headerOptions = NULL;
	AVFormatContext *ctx;
	AVStream *st;
	int len;
	int i, ret=0;
	
	/* now we can open the relevant output stream */
	if( IS_SVR_CONN_TYPE(svrConn, CONN_TYPE_HLS) )
	{
		avformat_alloc_output_context2(&ctx, NULL, NULL, svrConn->stream->filename);//"Example_Out.m3u8");
		av_dict_set_int(&headerOptions, "hls_time", 10, 0); /* scond for every segment */
		av_dict_set_int(&headerOptions, "hls_list_size", 6, 0);	/* max number of playlist entries */
		av_dict_set(&headerOptions, "hls_flags", "delete_segments", 0);
		av_dict_set(&headerOptions, "hls_segment_filename", "muxLabAv%03d.ts", 0);
//				av_dict_set(&headerOptions, "hls_flags", "single_file", 0);//AV_OPT_SEARCH_CHILDREN);
	}
	else
	{
		ctx = avformat_alloc_context();
	}
	
	if (!ctx)
	{
		MUX_SVR_ERROR("'%s' Error: %s in alloc avformat context", svrConn->name, av_err2str(ret));
		return -EXIT_FAILURE;
	}

	av_dict_copy(&(ctx->metadata), svrConn->stream->metadata, 0);
VTRACE();

#if 0	
	ctx->oformat = svrConn->stream->fmt;
#else
	if(svrConn->type == CONN_TYPE_HLS)
	{
		ctx->oformat = av_guess_format("hls", NULL, NULL);
	}
	else if(svrConn->type == CONN_TYPE_MULTICAST)
	{
		ctx->oformat = av_guess_format("mpegts", NULL, NULL);
	}
	else if(svrConn->type == CONN_TYPE_RTMP)
	{
		ctx->oformat = av_guess_format("flv", NULL, NULL);
	}
#endif

	av_assert0(ctx->oformat);

//	for(i=0;i< svrConn->stream->feed->nbInStreams; i++)
	for(i=0;i< svrConn->stream->feed->mediaCapture->nbStreams; i++)
	{
		st = avformat_new_stream(ctx, NULL);
		if (!st)
			return -EXIT_FAILURE;
			//goto fail;

		void *st_internal;

		st_internal = st->internal;

//		muxSvrMediaCodec(st, svrConn->stream->feed->lavStreams[i]);
//		cmnMediaInitAvStream(st, svrConn->stream->feed->feedStreams[i]);
		MUX_SVR_DEBUG("open No.%d stream %s", i, svrConn->stream->feed->mediaCapture->capturedMedias[i]->name );
		cmnMediaInitAvStream(st, svrConn->stream->feed->mediaCapture->capturedMedias[i]);
		av_assert0(st->priv_data == NULL);
		av_assert0(st->internal == st_internal);

#if 1
		if(IS_STREAM_FORMAT(ctx->oformat, "flv") && (st->codecpar->codec_id == AV_CODEC_ID_AAC ) )
		{
			ff_stream_add_bitstream_filter(st, "aac_adtstoasc", NULL);
			st->internal->bitstream_checked = 1;
			ctx->flags |= AVFMT_FLAG_AUTO_BSF;
		}
#endif
		
	}

VTRACE();
	/* normally, no packets should be output here, but the packet size may be checked */
	if(svrConn->maxPacketSize )
	{
		ret = ffio_open_dyn_packet_buf(&ctx->pb, svrConn->maxPacketSize);
	}
	else
	{
		ret = avio_open_dyn_buf(&ctx->pb);
	}
	if(ret < 0)
	{
		MUX_SVR_ERROR("open %s IOContext failed: %s", svrConn->name, av_err2str(ret));
		goto fail;
	}
	
VTRACE();
	if( IS_SVR_CONN_TYPE(svrConn, CONN_TYPE_HLS) )
	{
		ret = avformat_write_header(ctx, &headerOptions) ;
	}
	else
	{
		ret = avformat_write_header(ctx, NULL) ;
	}
	if( ret < 0)
	{
fail:
		MUX_SVR_ERROR("Write Header for %s failed: %s", svrConn->name, av_err2str(ret));
		av_free(ctx);
		return -EXIT_FAILURE;
	}
	
	len = avio_close_dyn_buf(ctx->pb, &svrConn->dataBuffer.dBuffer);
	/* for HLS, this len is 0 */
	
	av_dict_free(&ctx->metadata);

VTRACE();
	av_dict_free(&headerOptions);

	
	svrConn->outFmtCtx = ctx;

	/* write */
	if(svrConn->urlContext && len> 0)
	{
		i = ffurl_write(svrConn->urlContext, svrConn->dataBuffer.dBuffer, len);
		if(i != len)
		{
			MUX_SVR_ERROR("Only %d bytes written in %d bytes", i, len );
		}
	}
	
	SVR_BUF_RESET(svrConn->dataBuffer);
VTRACE();
/*
	Not free it here. It is freed in data process of DataThread when send out data. Jack Lee, Sep.07,2017
	av_freep(&svrConn->dataBuffer.dBuffer);
*/
	ctx->pb = NULL;
	
	return EXIT_SUCCESS;
}


static int _serviceConnStart(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	MuxStream *stream = (MuxStream *)event->data;
	
	URLContext *h;
	int ret;
	char		protocol[1024];

	memset(protocol, 0, sizeof(protocol));

	if(svrConn->type == CONN_TYPE_RTMP)
	{
		if (  IS_STRING_NULL(stream->rtmpUrl) )
		{
			MUX_SVR_ERROR("Error: RTMP stream must be flv format");
			exit(1);
			return EXIT_SUCCESS;
		}

		if ( !IS_STREAM_TYPE(stream, MUX_STREAM_TYPE_RTMP) || !IS_STREAM_FORMAT(stream->fmt, "flv") )
		{
			MUX_SVR_ERROR("Error: RTMP stream must be flv format");
			exit(1);
			return EXIT_SUCCESS;
		}
		
		MUX_SVR_DEBUG("'%s' stream start RTMP push stream", stream->filename);
		snprintf(protocol, sizeof(protocol), "%s", stream->rtmpUrl);
	}
	else if(svrConn->type == CONN_TYPE_MULTICAST)
	{
		if (!IS_STREAM_TYPE(stream, MUX_STREAM_TYPE_UMC) || !IS_STREAM_FORMAT(stream->fmt, "mpegts") )
		{
			MUX_SVR_ERROR("Error: Multicast stream must be mpegts format");
			exit(1);
			return EXIT_SUCCESS;
		}

		MUX_SVR_DEBUG("'%s' stream start multicast service at udp://%s:%d", stream->filename, stream->multicastAddress.address, stream->multicastAddress.port);
		snprintf(protocol, sizeof(protocol), "udp://%s:%d", stream->multicastAddress.address, stream->multicastAddress.port);
	}
	else if(svrConn->type == CONN_TYPE_HLS)
	{
		if ( !IS_STREAM_FORMAT(stream->fmt, "hls") )
		{
			MUX_SVR_ERROR("Error: HLS stream must be hls format");
			exit(1);
			return EXIT_SUCCESS;
		}
	}
	else
	{
		MUX_SVR_ERROR("Error: stream '%s' is invalidate format ", stream->filename);
		exit(1);
	}

	
	if(!IS_STRING_NULL(protocol))
	{
		MUX_SVR_DEBUG( "Initialized Protocol URL: '%s'", protocol);
		ret = ffurl_open(&h, protocol, AVIO_FLAG_WRITE /*URL_WRONLY*/, NULL, NULL);
		if (ret < 0)
		{
			MUX_SVR_ERROR("RTMP URL initializing failed : %s", av_err2str(ret));
			exit(1);
			return EXIT_FAILURE;
		}
				
		svrConn->urlContext = h;
		svrConn->maxPacketSize = h->max_packet_size;
		MUX_SVR_DEBUG( "Protocol URL: '%s', max packet size :%d", protocol, svrConn->maxPacketSize);
	}

	svrConn->stream = stream;

	svrConn->muxSvr->nbConnections++;
	svrConn->muxSvr->currentBandwidth += stream->bandwidth;

//	av_strlcpy(svrConn->protocol, "UDP/", sizeof(svrConn->protocol));
//	av_strlcatf(svrConn->protocol, sizeof(svrConn->protocol), "MCAST");

	if(__startOutputFormat(svrConn) )
	{
		goto fail;
	}
VTRACE();

	svrConn->startTime = svrConn->muxSvr->currentTime;
	svrConn->firstPts = AV_NOPTS_VALUE;

	ENABLE_SVR_CONN(svrConn->stream->feed, svrConn);
VTRACE();	

	return HTTP_STATE_DATA;

fail:
	if (svrConn)
	{
		av_free(svrConn);
	}
	exit(1);
	return HTTP_STATE_ERROR;
}

static int _serviceConnStop(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	int len, ret;
	
	/* prepare header */
	if ( (ret = avio_open_dyn_buf(&svrConn->outFmtCtx->pb) )< 0)
	{
		MUX_SVR_ERROR("open dyn buffer failed for service connect '%s': %s", svrConn->name, av_err2str(ret));
		/* XXX: potential leak */
//		return -EXIT_FAILURE;
	}

	svrConn->outFmtCtx->pb->seekable = 0;
	av_write_trailer(svrConn->outFmtCtx);
	len = avio_close_dyn_buf(svrConn->outFmtCtx->pb, &svrConn->dataBuffer.dBuffer);
	svrConn->dataBuffer.bufferPtr = svrConn->dataBuffer.dBuffer;
	svrConn->dataBuffer.bufferEnd = svrConn->dataBuffer.dBuffer + len;

	svrConn->last_packet_sent = 1;

	DISABLE_SVR_CONN(svrConn->stream->feed, svrConn);

	if(svrConn->urlContext)
	{
		ret = ffurl_write(svrConn->urlContext, svrConn->dataBuffer.bufferPtr, len);
		if(ret != len)
		{
			MUX_SVR_ERROR("Only write %d bytes of %d bytes when write trailer", ret, len);
		}
		
		ffurl_close(svrConn->urlContext);
		svrConn->urlContext = NULL;

		SVR_BUF_RESET(svrConn->dataBuffer);
	}

	avformat_free_context(svrConn->outFmtCtx);
	svrConn->outFmtCtx = NULL;
	svrConn->stream = NULL;

	if(event->event == MUX_SVR_EVENT_ERROR)
		return HTTP_STATE_ERROR;
	
	return HTTP_STATE_INIT;
}

static svr_transition_t	_hlsStateInit[] =
{
	{
		MUX_SVR_EVENT_START,
		_serviceConnStart,
	},
};


static svr_transition_t	_hlsStateData[] =
{
	{
		MUX_SVR_EVENT_DATA,
		muxSvrConnSendPacket,
	},
	{
		MUX_SVR_EVENT_ERROR,
		_serviceConnStop,
	},
	{
		MUX_SVR_EVENT_CLOSE,
		_serviceConnStop,
	},
};



static svr_statemachine_t	_hlsFsmStates[] =
{
	{
		HTTP_STATE_INIT,
		sizeof(_hlsStateInit)/sizeof(svr_transition_t),
		_hlsStateInit,
		NULL,
	},
	{
		HTTP_STATE_DATA,
		sizeof(_hlsStateData)/sizeof(svr_transition_t),
		_hlsStateData,
		NULL,
	},
};

static SVR_SERVICE_FSM		_hlsFsm =
{
	sizeof( _hlsFsmStates)/sizeof(svr_statemachine_t),
	_hlsFsmStates,
};


/*  */
static int _svrInitGenericConn(int index, void *ele, void *priv)
{
	MUX_SVR *muxSvr = (MUX_SVR *)priv;
	MuxStream *stream = (MuxStream *)ele;
	SERVER_CONNECT *svrConn = NULL;

	if(stream == NULL)
	{
		MUX_SVR_ERROR( "Stream of '%s' is found, please check your configuration", RECORD_STREAM_NAME);
		exit(1);
	}

	if( !IS_STREAM_TYPE(stream, MUX_STREAM_TYPE_HLS) &&
		!IS_STREAM_TYPE(stream, MUX_STREAM_TYPE_UMC) &&
		!IS_STREAM_TYPE(stream, MUX_STREAM_TYPE_RTMP) )
	{
		return EXIT_SUCCESS;
	}

	svrConn = av_mallocz( sizeof(SERVER_CONNECT));
	if(svrConn == NULL)
	{
		return -EXIT_FAILURE;
	}

	svrConn->fd = -1;
	svrConn->muxSvr = muxSvr;
	svrConn->myListen = NULL;
	svrConn->stream = stream;

//	svrConn->fifo = av_mallocz(sizeof(cmn_fifo_t) );
	svrConn->lastPkt = NULL;
	cmn_fifo_init(muxSvr->svrConfig.fifoSize, &svrConn->fifo);
	
	svrConn->timeout = muxSvr->currentTime +HTTP_REQUEST_TIMEOUT;
	svrConn->startTime = svrConn->muxSvr->currentTime;
	svrConn->firstPts = AV_NOPTS_VALUE;

	svrConn->handle = NULL;
	if( IS_STREAM_TYPE(stream, MUX_STREAM_TYPE_HLS) )
	{
		snprintf(svrConn->name, sizeof(svrConn->name), "%s", "HLSEncoder");
		svrConn->fsm = &_hlsFsm;
		svrConn->type = CONN_TYPE_HLS;
	}
	else if(IS_STREAM_TYPE(stream, MUX_STREAM_TYPE_RTMP) )
	{
		snprintf(svrConn->name, sizeof(svrConn->name), "%s", "RTMPClient");
		svrConn->fsm = &_hlsFsm;
		svrConn->type = CONN_TYPE_RTMP;
	}
	else if(IS_STREAM_TYPE(stream, MUX_STREAM_TYPE_UMC) )
	{
		snprintf(svrConn->name, sizeof(svrConn->name), "%s", "UDPMulticast");
		svrConn->fsm = &_hlsFsm;
		svrConn->type = CONN_TYPE_MULTICAST;
	}
	else
	{
		MUX_SVR_ERROR("Invalidate Service Connection");
		av_free(svrConn);
		return EXIT_SUCCESS;
	}
	
	svrConn->state = HTTP_STATE_INIT;

	/* For RTMP/UMC/HLS, this IOContext never be used */
	svrConn->isInitedDynaBuf = FALSE;

	muxSvrServiceReportEvent(svrConn, MUX_SVR_EVENT_START, 0, stream);

	MUX_ADD_SVR_CONNECTION(muxSvr, svrConn);

	return EXIT_SUCCESS;
}



/* start all multicast streams */
int muxSvrServiceConnectsInit(MUX_SVR *muxSvr)
{
	int ret = 0;
	
	ret = cmn_list_iterate( &muxSvr->svrConfig.streams, FALSE, _svrInitGenericConn, muxSvr);

	return ret;
}

void muxSvrServiceConnectsDestroy(MUX_SVR *muxSvr)
{
	int i;
	SERVER_CONNECT *svrConn;

	for(i=0; i< cmn_list_size(&muxSvr->serverConns); i++)
	{
		svrConn = (SERVER_CONNECT *) cmn_list_get (&muxSvr->serverConns, i);

		svrConn = svrConn;
//		cmn_mutex_destroy(feed->connsMutex);
	}
	
}


