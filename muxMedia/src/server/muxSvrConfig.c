/*
 * This file is part of FFmpeg.
 */

#include "muxSvr.h"


static void _vCfgError(const char *filename, int line_num, int log_level, int *errors, const char *fmt, va_list vl)
{
	av_log(NULL, log_level, "%s:%d: ", filename, line_num);
	av_vlog(NULL, log_level, fmt, vl);
	if (errors)
		(*errors)++;
}

void muxCfgError(const char *filename, int line_num, int log_level, int *errors, const char *fmt, ...)
{
	va_list vl;
	va_start(vl, fmt);
	_vCfgError(filename, line_num, log_level, errors, fmt, vl);
	va_end(vl);
}

#if 0
int muxCfgSetCodec(AVCodecContext *ctx, const char *codec_name, MuxServerConfig *svrConfig)
{
	int ret;
	AVCodec *codec = avcodec_find_encoder_by_name(codec_name);
	if (!codec || codec->type != ctx->codec_type)
	{
		CONFIG_ERROR( "Invalid codec name: '%s'\n", codec_name);
		return 0;
	}
	
	if (ctx->codec_id == AV_CODEC_ID_NONE && !ctx->priv_data)
	{
		if ((ret = avcodec_get_context_defaults3(ctx, codec)) < 0)
			return ret;
		ctx->codec = codec;
	}
	
	if (ctx->codec_id != codec->id)
		CONFIG_ERROR("Inconsistent configuration: trying to set '%s' codec option, but '%s' codec is used previously\n", codec_name, avcodec_get_name(ctx->codec_id));
	return 0;
}

int muxCfgSetFloatParam(float *dest, const char *value, float factor, float min, float max,
	MuxServerConfig *svrConfig, const char *error_msg, ...)
{
	double tmp;
	char *tailp;
	if (!value || !value[0])
		goto error;
	errno = 0;
	tmp = strtod(value, &tailp);
	if (tmp < min || tmp > max)
		goto error;
	if (factor)
		tmp *= factor;
	if (tailp[0] || errno)
		goto error;
	if (dest)
		*dest = tmp;
	return 0;

error:
	if (svrConfig)
	{
		va_list vl;
		va_start(vl, error_msg);
		_vCfgError(svrConfig->filename, svrConfig->line_num, AV_LOG_ERROR, &svrConfig->errors, error_msg, vl);
		va_end(vl);
	}
	
	return AVERROR(EINVAL);
}

#endif

AVOutputFormat *muxCfgGuessFormat(const char *short_name, const char *filename, const char *mime_type)
{
	AVOutputFormat *fmt = av_guess_format(short_name, filename, mime_type);

	if (fmt)
	{
		AVOutputFormat *stream_fmt;
		char stream_format_name[64];

		snprintf(stream_format_name, sizeof(stream_format_name), "%s_stream", fmt->name);
		stream_fmt = av_guess_format(stream_format_name, NULL, NULL);

		if (stream_fmt)
			fmt = stream_fmt;
	}

	return fmt;
}

static int _setInteger(int *dest, const char *value, int factor,
	int min, int max, MuxServerConfig *svrConfig, const char *error_msg, ...)
{
	int tmp;
	char *tailp;
	if (!value || !value[0])
		goto error;
	errno = 0;
	tmp = strtol(value, &tailp, 0);
	if (tmp < min || tmp > max)
		goto error;
	if (factor)
	{
		if (tmp == INT_MIN || FFABS(tmp) > INT_MAX / FFABS(factor))
			goto error;
		tmp *= factor;
	}
	if (tailp[0] || errno)
		goto error;
	
	if (dest)
		*dest = tmp;
	return 0;

error:
	if (svrConfig)
	{
		va_list vl;
		va_start(vl, error_msg);
		_vCfgError(svrConfig->filename, svrConfig->line_num, AV_LOG_ERROR, &svrConfig->errors, error_msg, vl);
		va_end(vl);
	}
	
	return AVERROR(EINVAL);
}



static int _parseGlobal(MuxServerConfig *svrConfig, const char *cmd, const char **p)
{
	int val;
	char arg[1024];
	
	if ( !av_strcasecmp(cmd, "HTTPPort"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		_setInteger(&val, arg, 0, 1, 65535, svrConfig, "Invalid port: %s\n", arg);
		if (val < 1024)
			CONFIG_WARNING("Trying to use IETF assigned system port: '%d'\n", val);
		svrConfig->httpAddress.port = (val);
	}
	else if (!av_strcasecmp(cmd, "HTTPBindAddress") )
	{
#if 0	
		cmnParseGetArg(arg, sizeof(arg), p);
		if (muxUtilResolveHost(&svrConfig->http_addr.sin_addr, arg))
			CONFIG_ERROR("Invalid host/IP address: '%s'\n", arg);
#else
		cmnParseGetArg(svrConfig->httpAddress.address, sizeof(svrConfig->httpAddress.address), p);
#endif		
	}
	else if (!av_strcasecmp(cmd, "RTSPPort"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		_setInteger(&val, arg, 0, 1, 65535, svrConfig, "Invalid port: %s\n", arg);
		svrConfig->rtspAddress.port = (val);
	}
	else if (!av_strcasecmp(cmd, "RTSPBindAddress"))
	{
#if 0
		cmnParseGetArg(arg, sizeof(arg), p);
		if (muxUtilResolveHost(&svrConfig->rtsp_addr.sin_addr, arg))
			CONFIG_ERROR("Invalid host/IP address: %s\n", arg);
#else
		cmnParseGetArg(svrConfig->rtspAddress.address, sizeof(svrConfig->httpAddress.address), p);
#endif		
	}
	else if (!av_strcasecmp(cmd, "MaxHTTPConnections"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		_setInteger(&val, arg, 0, 1, 65535, svrConfig, "Invalid MaxHTTPConnections: %s\n", arg);
		svrConfig->maxHttpConnections = val;
		
		if (svrConfig->maxConnections > svrConfig->maxHttpConnections)
		{
			CONFIG_ERROR("Inconsistent configuration: MaxClients(%d) > MaxHTTPConnections(%d)\n", 
				svrConfig->maxConnections, svrConfig->maxHttpConnections);
		}
	}
	else if (!av_strcasecmp(cmd, "MaxClients"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		_setInteger(&val, arg, 0, 1, 65535, svrConfig, "Invalid MaxClients: '%s'\n", arg);
		svrConfig->maxConnections = val;
		
		if (svrConfig->maxConnections > svrConfig->maxHttpConnections)
		{
			CONFIG_ERROR("Inconsistent configuration: MaxClients(%d) > MaxHTTPConnections(%d)\n", 
				svrConfig->maxConnections, svrConfig->maxHttpConnections);
		}
	}
	else if (!av_strcasecmp(cmd, "MaxBandwidth"))
	{
		int64_t llval;
		char *tailp;
		
		cmnParseGetArg(arg, sizeof(arg), p);
		errno = 0;
		llval = strtoll(arg, &tailp, 10);
		if (llval < 10 || llval > 10000000 || tailp[0] || errno)
			CONFIG_ERROR("Invalid MaxBandwidth: '%s'\n", arg);
		else
			svrConfig->maxBandwidth = llval;
		printf("maxBandwidth: %" PRId64 "\n", svrConfig->maxBandwidth);
	}
	else if (!av_strcasecmp(cmd, "CustomLog"))
	{
		cmnParseGetArg(svrConfig->serverLog.logFileName, 256, p);
	}
	else if (!av_strcasecmp(cmd, "NoDefaults"))
	{
		svrConfig->use_defaults = 0;
	}
	else if (!av_strcasecmp(cmd, "UseDefaults"))
	{
		svrConfig->use_defaults = 1;
	}
	else if (!av_strcasecmp(cmd, "Daemon"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		svrConfig->serverLog.isDaemonized = (strcasecmp(arg , "Yes")==0)? TRUE: FALSE;
#if DEBUG_CONFIG_FILE
		if(svrConfig->serverLog.isDaemonized )
			fprintf(stderr, "running as daemon\n");
		else
			fprintf(stderr, "running as front-end\n");
#endif
	}
	else if (!av_strcasecmp(cmd, "MaxLogSize"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		svrConfig->serverLog.maxSize = 2048*UNIT_OF_KILO;
	}
	else if (!av_strcasecmp(cmd, "DebugLevel"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		val =atoi(arg);
		if(val < CMN_LOG_EMERG || val > CMN_LOG_DEBUG)
		{
			val = CMN_LOG_NOTICE;
		}
		svrConfig->serverLog.llevel = val;
	}
	else if (!av_strcasecmp(cmd, "FifoSize"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		val =atoi(arg);
		svrConfig->fifoSize = val;
	}
	else if (!av_strcasecmp(cmd, "FeedDelay"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		val =atoi(arg);
		svrConfig->feedDelay = val;
	}
	else if (!av_strcasecmp(cmd, "SendDelay"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		val =atoi(arg);
		svrConfig->sendDelay = val;
	}
	else if (!av_strcasecmp(cmd, "EnableAuth"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		svrConfig->enableAuthen = (strcasecmp(arg, "Yes")==0)? TRUE: FALSE;
	}
	else
		CONFIG_ERROR("Incorrect keyword: '%s'\n", cmd);
	
	return 0;
}


static int _parseFeed(MuxServerConfig *svrConfig, const char *cmd,  const char **p, MuxFeed **pFeed)
{
	MuxFeed *feed;
	char arg[1024];

	if (!av_strcasecmp(cmd, "<Feed"))
	{
		char *q;
		feed = av_mallocz(sizeof(MuxFeed));
		if (!feed)
			return AVERROR(ENOMEM);
		
		cmnParseGetArg(feed->filename, sizeof(feed->filename), p);
		q = strrchr(feed->filename, '>');
		if (q)
			*q = '\0';

		if( muxSvrFoundFeed(0, feed->filename, svrConfig ) )	
		{
			CONFIG_ERROR("Stream '%s' already registered\n", feed->filename);
		}

		feed->muxSvr = svrConfig->muxSvr;

		cmn_list_append(&svrConfig->feeds, feed);
		*pFeed = feed;
		return 0;
	}

	feed = *pFeed;
	av_assert0(feed);
	
	if (!av_strcasecmp(cmd, "File")  )
	{
		cmnParseGetArg(feed->feed_filename, sizeof(feed->feed_filename), p);
	}
	else if (!av_strcasecmp(cmd, "Type")  )
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		if (!strcasecmp(arg, "Capture"))
		{
			feed->type = MUX_FEED_TYPE_CAPTURE;
		}
	}
	else if (!av_strcasecmp(cmd, "CaptureName")  )
	{
		cmnParseGetArg(feed->captureName, sizeof(feed->captureName), p);
	}
	else if (!av_strcasecmp(cmd, "</Feed>"))
	{
		*pFeed = NULL;
	}
	else
	{
		CONFIG_ERROR("Invalid entry '%s' inside <Feed></Feed>\n", cmd);
	}
	
	return 0;
	
}

static int _parseStream(MuxServerConfig *svrConfig, const char *cmd,  const char **p, MuxStream **pstream)
{
	char arg[1024], arg2[1024];
	MuxStream *stream;
	int val;

	if (!av_strcasecmp(cmd, "<Stream"))
	{
		char *q;
		stream = av_mallocz(sizeof(MuxStream));
		if (!stream)
			return AVERROR(ENOMEM);
		
		svrConfig->dummy_actx = avcodec_alloc_context3(NULL);
		svrConfig->dummy_vctx = avcodec_alloc_context3(NULL);
		
		if (!svrConfig->dummy_vctx || !svrConfig->dummy_actx)
		{
			av_free(stream);
			avcodec_free_context(&svrConfig->dummy_vctx);
			avcodec_free_context(&svrConfig->dummy_actx);
			return AVERROR(ENOMEM);
		}
		
		svrConfig->dummy_actx->codec_type = AVMEDIA_TYPE_AUDIO;
		svrConfig->dummy_vctx->codec_type = AVMEDIA_TYPE_VIDEO;
		cmnParseGetArg(stream->filename, sizeof(stream->filename), p);
		q = strrchr(stream->filename, '>');
		if (q)
			*q = '\0';

		if( muxSvrFoundStream(FALSE, stream->filename, svrConfig) )	
		{
			CONFIG_ERROR("Stream '%s' already registered\n", stream->filename);
		}
		
		stream->fmt = muxCfgGuessFormat(NULL, stream->filename, NULL);
		if (stream->fmt)
		{
			svrConfig->guessed_audio_codec_id = stream->fmt->audio_codec;
			svrConfig->guessed_video_codec_id = stream->fmt->video_codec;
		}
		else
		{
			svrConfig->guessed_audio_codec_id = AV_CODEC_ID_NONE;
			svrConfig->guessed_video_codec_id = AV_CODEC_ID_NONE;
		}
		
		svrConfig->stream_use_defaults = svrConfig->use_defaults;
		stream->muxSvr = svrConfig->muxSvr;
		
		cmn_list_append(&svrConfig->streams, stream);
		*pstream = stream;
		return 0;
	}

	stream = *pstream;
	av_assert0(stream);
	
	if (!av_strcasecmp(cmd, "Format"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);

		if (!strcmp(arg, "status"))
		{
			stream->type = MUX_STREAM_TYPE_STATUS;
			stream->fmt = NULL;
		}
		else
		{
			stream->type = MUX_STREAM_TYPE_LIVE;
			/* JPEG cannot be used here, so use single frame MJPEG */
			if (!strcmp(arg, "jpeg"))
			{
				strcpy(arg, "singlejpeg");
				stream->single_frame=1;
			}
			
			stream->fmt = muxCfgGuessFormat(arg, NULL, NULL);
			if (!stream->fmt)
				CONFIG_ERROR("Unknown Format: '%s'\n", arg);
		}
		
		if (stream->fmt)
		{
			/* specific case: if transport stream output to RTP, we use a raw transport stream reader */
			if (IS_STREAM_FORMAT(stream->fmt, "rtp") )
			{
				av_dict_set(&stream->in_opts, "mpeg2ts_compute_pcr", "1", 0);
			}

			if( IS_STREAM_FORMAT( stream->fmt, "hls")  )
			{
				stream->type = MUX_STREAM_TYPE_HLS;
			}

			svrConfig->guessed_audio_codec_id = stream->fmt->audio_codec;
			svrConfig->guessed_video_codec_id = stream->fmt->video_codec;
			
		}
	}
	else if (!av_strcasecmp(cmd, "RtmpURL"))
	{
		cmnParseGetArg(stream->rtmpUrl, sizeof(stream->rtmpUrl), p);
		stream->type = MUX_STREAM_TYPE_RTMP;
	}
	else if (!av_strcasecmp(cmd, "FaviconURL"))
	{
		if (stream->type == MUX_STREAM_TYPE_STATUS)
			cmnParseGetArg(stream->feed_filename, sizeof(stream->feed_filename), p);
		else
			CONFIG_ERROR("FaviconURL only permitted for status streams\n");
	}
	else if (!av_strcasecmp(cmd, "Author") || !av_strcasecmp(cmd, "Comment")   ||
		!av_strcasecmp(cmd, "Copyright") ||!av_strcasecmp(cmd, "Title"))
	{
		char key[32];
		int i;
		cmnParseGetArg(arg, sizeof(arg), p);
		for (i = 0; i < strlen(cmd); i++)
			key[i] = av_tolower(cmd[i]);

		key[i] = 0;
		CONFIG_WARNING("Deprecated '%s' option in configuration file. Use 'Metadata %s VALUE' instead.\n", cmd, key);
		if (av_dict_set(&stream->metadata, key, arg, 0) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "Metadata"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		cmnParseGetArg(arg2, sizeof(arg2), p);
		if (av_dict_set(&stream->metadata, arg, arg2, 0) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "StartSendOnKey"))
	{
		stream->send_on_key = 1;
	}
	else if (!av_strcasecmp(cmd, "MaxTime"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		stream->max_time = atof(arg) * 1000;
	}
	else if (!av_strcasecmp(cmd, "NoVideo"))
	{
		svrConfig->noVideo = TRUE;
	}
	else if (!av_strcasecmp(cmd, "NoAudio"))
	{
		svrConfig->noAudio = TRUE;
	}
	else if (!av_strcasecmp(cmd, "NoSubtitle"))
	{
		svrConfig->noSubtitle = TRUE;
	}
	else if (!av_strcasecmp(cmd, "ACL"))
	{
		muxSvrParseAclRow(stream, NULL, NULL, *p, svrConfig->filename, svrConfig->line_num);
	}
	else if (!av_strcasecmp(cmd, "DynamicACL"))
	{
		cmnParseGetArg(stream->dynamic_acl, sizeof(stream->dynamic_acl), p);
	}
	else if (!av_strcasecmp(cmd, "RTSPOption"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		av_freep(&stream->rtsp_option);
		stream->rtsp_option = av_strdup(arg);
	}
	else if (!av_strcasecmp(cmd, "MulticastAddress"))
	{
#if 0	
		cmnParseGetArg(arg, sizeof(arg), p);
		if (muxUtilResolveHost(&stream->multicast_ip, arg))
			CONFIG_ERROR("Invalid host/IP address: '%s'\n", arg);
#endif
		cmnParseGetArg(stream->multicastAddress.address, sizeof(stream->multicastAddress.address), p);

		stream->is_multicast = 1;
		stream->type = MUX_STREAM_TYPE_UMC;
	}
	else if (!av_strcasecmp(cmd, "MulticastPort"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		_setInteger(&val, arg, 0, 1, 65535, svrConfig, "Invalid MulticastPort: '%s'\n", arg);
		stream->multicastAddress.port = val;
	}
	else if (!av_strcasecmp(cmd, "MulticastTTL"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		_setInteger(&val, arg, 0, INT_MIN, INT_MAX, svrConfig,"Invalid MulticastTTL: '%s'\n", arg);
		stream->multicast_ttl = val;
	}
	else if (!av_strcasecmp(cmd, "</Stream>"))
	{
		if( (stream->type==MUX_STREAM_TYPE_LIVE) && (stream->feed == NULL) )
			CONFIG_ERROR("Feed of stream '%s' is not existed", stream->filename );
		svrConfig->stream_use_defaults &= 1;
		
		av_dict_free(&svrConfig->video_opts);
		av_dict_free(&svrConfig->audio_opts);
		avcodec_free_context(&svrConfig->dummy_vctx);
		avcodec_free_context(&svrConfig->dummy_actx);
		svrConfig->noVideo = 0;
		svrConfig->noAudio = 0;
		svrConfig->noSubtitle = 0;

		*pstream = NULL;
	}
	else if (!av_strcasecmp(cmd, "Feed")  )
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		MuxFeed *feed = muxSvrFoundFeed(0, arg, svrConfig);
		if(feed == NULL)
			CONFIG_ERROR("Feed '%s' in stream '%s' is not existed", arg, stream->filename );
		
		ADD_ELEMENT( feed->myStreams, stream);
		stream->feed = feed;
	}
	else if (!av_strcasecmp(cmd, "UseDefaults"))
	{
		if (svrConfig->stream_use_defaults > 1)
			CONFIG_WARNING("Multiple UseDefaults/NoDefaults entries.\n");
		svrConfig->stream_use_defaults = 3;
	}
	else if (!av_strcasecmp(cmd, "NoDefaults"))
	{
		if (svrConfig->stream_use_defaults > 1)
			CONFIG_WARNING("Multiple UseDefaults/NoDefaults entries.\n");
		svrConfig->stream_use_defaults = 2;
	}
	else
	{
		CONFIG_ERROR("Invalid entry '%s' inside <Stream></Stream>\n", cmd);
	}
	
	return 0;
	
nomem:
	av_log(NULL, AV_LOG_ERROR, "Out of memory. Aborting.\n");
	av_dict_free(&svrConfig->video_opts);
	av_dict_free(&svrConfig->audio_opts);
	avcodec_free_context(&svrConfig->dummy_vctx);
	avcodec_free_context(&svrConfig->dummy_actx);
	return AVERROR(ENOMEM);
}


int muxSvrConfigParse(const char *filename, MuxServerConfig *svrConfig)
{
	FILE *f;
	char line[1024];
	char cmd[64];
	const char *p;
	int ret = 0;
	MuxStream *stream = NULL;
	MuxFeed *feed = NULL;

	av_assert0(svrConfig);

	f = fopen(filename, "r");
	if (!f)
	{
		ret = AVERROR(errno);
		av_log(NULL, AV_LOG_ERROR, "Could not open the configuration file '%s'\n", filename);
		return ret;
	}

	svrConfig->errors = svrConfig->warnings = 0;
	cmn_list_init(&svrConfig->streams);
	cmn_list_init(&svrConfig->feeds);

	svrConfig->line_num = 0;
	while (fgets(line, sizeof(line), f) != NULL)
	{
		svrConfig->line_num++;
		p = line;
		while (av_isspace(*p))
			p++;
		
		if (*p == '\0' || *p == '#')
			continue;

		cmnParseGetArg(cmd, sizeof(cmd), &p);

		if ( !av_strcasecmp(cmd, "<Feed") ||feed )
		{
			ret = _parseFeed(svrConfig, cmd, &p, &feed);
			if (ret < 0)
				break;
		}
		else if ( !av_strcasecmp(cmd, "<Stream") || stream )
		{
			ret = _parseStream(svrConfig, cmd, &p, &stream);
			if (ret < 0)
				break;
		}
		else
		{
			_parseGlobal(svrConfig, cmd, &p);
		}
	}
	

	fclose(f);
	if (ret < 0)
		return ret;

	if (svrConfig->errors)
		return AVERROR(EINVAL);
	else
		return 0;
}

void muxSvrConfigFree(MuxServerConfig *svrConfig)
{
	MuxFeed *feed;
	MuxStream *stream;
	int i;
	
	for(i=0; i< cmn_list_size(&svrConfig->feeds); i++)
	{
		feed = (MuxFeed *) cmn_list_get (&svrConfig->feeds, i);
		
		cmn_mutex_destroy(feed->connsMutex);
	}
	cmn_list_ofchar_free(&svrConfig->feeds, FALSE);

	for(i=0; i< cmn_list_size(&svrConfig->streams); i++)
	{
		stream = (MuxStream *) cmn_list_get (&svrConfig->streams, i);

		stream = stream;
	}
	cmn_list_ofchar_free(&svrConfig->streams, FALSE);

}

