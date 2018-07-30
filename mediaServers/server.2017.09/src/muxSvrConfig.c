/*
 * This file is part of FFmpeg.
 */

#include "muxSvr.h"

int muxCfgParseStream(MuxServerConfig *svrConfig, const char *cmd,  const char **p, MuxStream **pstream);


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

int muxCfgSetIntegerParam(int *dest, const char *value, int factor,
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


FILE *muxCfgGetPresetFile(char *filename, size_t filename_size, const char *preset_name, int is_path, const char *codec_name)
{
	FILE *f = NULL;
	int i;
	const char *base[3] = { getenv("FFMPEG_DATADIR"), getenv("HOME"), FFMPEG_DATADIR, };

	if (is_path)
	{
		av_strlcpy(filename, preset_name, filename_size);
		f = fopen(filename, "r");
	}
	else
	{

		for (i = 0; i < 3 && !f; i++)
		{
			if (!base[i])
				continue;
			snprintf(filename, filename_size, "%s%s/%s.ffpreset", base[i], i != 1 ? "" : "/.ffmpeg", preset_name);
			f = fopen(filename, "r");
			if (!f && codec_name)
			{
				snprintf(filename, filename_size, "%s%s/%s-%s.ffpreset",  base[i], i != 1 ? "" : "/.ffmpeg", codec_name, preset_name);
				f = fopen(filename, "r");
			}
		}
	}

	return f;
}

static int _parseGlobal(MuxServerConfig *svrConfig, const char *cmd, const char **p)
{
	int val;
	char arg[1024];
	
	if ( !av_strcasecmp(cmd, "HTTPPort"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, 1, 65535, svrConfig, "Invalid port: %s\n", arg);
		if (val < 1024)
			CONFIG_WARNING("Trying to use IETF assigned system port: '%d'\n", val);
		svrConfig->httpAddress.port = (val);
	}
	else if (!av_strcasecmp(cmd, "HTTPBindAddress") )
	{
#if 0	
		muxUtilGetArg(arg, sizeof(arg), p);
		if (muxUtilResolveHost(&svrConfig->http_addr.sin_addr, arg))
			CONFIG_ERROR("Invalid host/IP address: '%s'\n", arg);
#else
		muxUtilGetArg(svrConfig->httpAddress.address, sizeof(svrConfig->httpAddress.address), p);
#endif		
	}
	else if (!av_strcasecmp(cmd, "NoDaemon"))
	{
		CONFIG_WARNING("NoDaemon option has no effect. You should remove it.\n");
	}
	else if (!av_strcasecmp(cmd, "RTSPPort"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, 1, 65535, svrConfig, "Invalid port: %s\n", arg);
		svrConfig->rtspAddress.port = (val);
	}
	else if (!av_strcasecmp(cmd, "RTSPBindAddress"))
	{
#if 0
		muxUtilGetArg(arg, sizeof(arg), p);
		if (muxUtilResolveHost(&svrConfig->rtsp_addr.sin_addr, arg))
			CONFIG_ERROR("Invalid host/IP address: %s\n", arg);
#else
		muxUtilGetArg(svrConfig->rtspAddress.address, sizeof(svrConfig->httpAddress.address), p);
#endif		
	}
	else if (!av_strcasecmp(cmd, "MaxHTTPConnections"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, 1, 65535, svrConfig, "Invalid MaxHTTPConnections: %s\n", arg);
		svrConfig->maxHttpConnections = val;
		
		if (svrConfig->maxConnections > svrConfig->maxHttpConnections)
		{
			CONFIG_ERROR("Inconsistent configuration: MaxClients(%d) > MaxHTTPConnections(%d)\n", 
				svrConfig->maxConnections, svrConfig->maxHttpConnections);
		}
	}
	else if (!av_strcasecmp(cmd, "MaxClients"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, 1, 65535, svrConfig, "Invalid MaxClients: '%s'\n", arg);
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
		
		muxUtilGetArg(arg, sizeof(arg), p);
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
		muxUtilGetArg(svrConfig->serverLog.logFileName, 256, p);
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
		muxUtilGetArg(arg, sizeof(arg), p);
		svrConfig->serverLog.isDaemonized = (strcasecmp(arg , "Yes")==0)? TRUE: FALSE;
#if DEBUG_CONFIG
		if(svrConfig->serverLog.isDaemonized )
			fprintf(stderr, "running as daemon\n");
		else
			fprintf(stderr, "running as front-end\n");
#endif
	}
	else if (!av_strcasecmp(cmd, "MaxLogSize"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		svrConfig->serverLog.maxSize = 2048*UNIT_OF_KILO;//parseGetIntValue(p);
	}
	else if (!av_strcasecmp(cmd, "DebugLevel"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		val =atoi(arg);
		if(val < CMN_LOG_EMERG || val > CMN_LOG_DEBUG)
		{
			val = CMN_LOG_NOTICE;
		}
		svrConfig->serverLog.llevel = val;
	}
	else if (!av_strcasecmp(cmd, "FifoSize"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		val =atoi(arg);
		svrConfig->fifoSize = val;
	}
	else if (!av_strcasecmp(cmd, "FeedDelay"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		val =atoi(arg);
		svrConfig->feedDelay = val;
	}
	else if (!av_strcasecmp(cmd, "SendDelay"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		val =atoi(arg);
		svrConfig->sendDelay = val;
	}
	else if (!av_strcasecmp(cmd, "EnableAuth"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		svrConfig->enableAuthen = (strcasecmp(arg, "Yes")==0)? TRUE: FALSE;
	}
	else
		CONFIG_ERROR("Incorrect keyword: '%s'\n", cmd);
	
	return 0;
}


int muxCfgParseFeed(MuxServerConfig *svrConfig, const char *cmd,  const char **p, MuxFeed **pFeed)
{
	MuxFeed *feed;

	if (!av_strcasecmp(cmd, "<Feed"))
	{
		char *q;
		feed = av_mallocz(sizeof(MuxFeed));
		if (!feed)
			return AVERROR(ENOMEM);
		
		muxUtilGetArg(feed->filename, sizeof(feed->filename), p);
		q = strrchr(feed->filename, '>');
		if (q)
			*q = '\0';

		if( muxSvrFoundFeed(0, feed->filename, svrConfig->muxSvr) )	
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
		muxUtilGetArg(feed->feed_filename, sizeof(feed->feed_filename), p);
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

		muxUtilGetArg(cmd, sizeof(cmd), &p);

		if ( !av_strcasecmp(cmd, "<Feed") ||feed )
		{
			ret = muxCfgParseFeed(svrConfig, cmd, &p, &feed);
			if (ret < 0)
				break;
		}
		else if ( !av_strcasecmp(cmd, "<Stream") || stream )
		{
			ret = muxCfgParseStream(svrConfig, cmd, &p, &stream);
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

