/*
 * This file is part of FFmpeg.
 */

#include "muxSvr.h"

int muxCfgParseStream(MuxServerConfig *config, const char *cmd,  const char **p, MuxStream **pstream);


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


int muxCfgSetCodec(AVCodecContext *ctx, const char *codec_name, MuxServerConfig *config)
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
	int min, int max, MuxServerConfig *config, const char *error_msg, ...)
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
	if (config)
	{
		va_list vl;
		va_start(vl, error_msg);
		_vCfgError(config->filename, config->line_num, AV_LOG_ERROR, &config->errors, error_msg, vl);
		va_end(vl);
	}
	
	return AVERROR(EINVAL);
}


int muxCfgSetFloatParam(float *dest, const char *value, float factor, float min, float max,
	MuxServerConfig *config, const char *error_msg, ...)
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
	if (config)
	{
		va_list vl;
		va_start(vl, error_msg);
		_vCfgError(config->filename, config->line_num, AV_LOG_ERROR, &config->errors, error_msg, vl);
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

static int _parseGlobal(MuxServerConfig *config, const char *cmd, const char **p)
{
	int val;
	char arg[1024];
	
	if ( !av_strcasecmp(cmd, "HTTPPort"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, 1, 65535, config, "Invalid port: %s\n", arg);
		if (val < 1024)
			CONFIG_WARNING("Trying to use IETF assigned system port: '%d'\n", val);
		config->http_addr.sin_port = htons(val);
	}
	else if (!av_strcasecmp(cmd, "HTTPBindAddress") )
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (muxUtilResolveHost(&config->http_addr.sin_addr, arg))
			CONFIG_ERROR("Invalid host/IP address: '%s'\n", arg);
	}
	else if (!av_strcasecmp(cmd, "NoDaemon"))
	{
		CONFIG_WARNING("NoDaemon option has no effect. You should remove it.\n");
	}
	else if (!av_strcasecmp(cmd, "RTSPPort"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, 1, 65535, config, "Invalid port: %s\n", arg);
		config->rtsp_addr.sin_port = htons(val);
	}
	else if (!av_strcasecmp(cmd, "RTSPBindAddress"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (muxUtilResolveHost(&config->rtsp_addr.sin_addr, arg))
			CONFIG_ERROR("Invalid host/IP address: %s\n", arg);
	}
	else if (!av_strcasecmp(cmd, "MaxHTTPConnections"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, 1, 65535, config, "Invalid MaxHTTPConnections: %s\n", arg);
		config->nb_max_http_connections = val;
		
		if (config->nb_max_connections > config->nb_max_http_connections)
		{
			CONFIG_ERROR("Inconsistent configuration: MaxClients(%d) > MaxHTTPConnections(%d)\n", 
				config->nb_max_connections, config->nb_max_http_connections);
		}
	}
	else if (!av_strcasecmp(cmd, "MaxClients"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, 1, 65535, config, "Invalid MaxClients: '%s'\n", arg);
		config->nb_max_connections = val;
		
		if (config->nb_max_connections > config->nb_max_http_connections)
		{
			CONFIG_ERROR("Inconsistent configuration: MaxClients(%d) > MaxHTTPConnections(%d)\n", 
				config->nb_max_connections, config->nb_max_http_connections);
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
			config->max_bandwidth = llval;
	}
	else if (!av_strcasecmp(cmd, "CustomLog"))
	{
		if (!config->debug)
		{
			muxUtilGetArg(config->logfilename, sizeof(config->logfilename), p);
		}
	}
	else if (!av_strcasecmp(cmd, "LoadModule"))
	{
		CONFIG_ERROR("Loadable modules are no longer supported\n");
	}
	else if (!av_strcasecmp(cmd, "NoDefaults"))
	{
		config->use_defaults = 0;
	}
	else if (!av_strcasecmp(cmd, "UseDefaults"))
	{
		config->use_defaults = 1;
	}
	else
		CONFIG_ERROR("Incorrect keyword: '%s'\n", cmd);
	
	return 0;
}

static int _parseFeed(MuxServerConfig *config, const char *cmd, const char **p, MuxStream **pfeed)
{
	MuxStream *feed;
	char arg[1024];
	
	av_assert0(pfeed);
	feed = *pfeed;
	
	if (!av_strcasecmp(cmd, "<Feed"))
	{
		char *q;
		MuxStream *s;
		feed = av_mallocz(sizeof(MuxStream));
		if (!feed)
			return AVERROR(ENOMEM);
		
		muxUtilGetArg(feed->filename, sizeof(feed->filename), p);
		q = strrchr(feed->filename, '>');
		if (*q)
			*q = '\0';

		for (s = config->first_feed; s; s = s->next)
		{
			if (!strcmp(feed->filename, s->filename))
				CONFIG_ERROR("Feed '%s' already registered\n", s->filename);
		}

		feed->fmt = av_guess_format("ffm", NULL, NULL);
		/* default feed file */
		snprintf(feed->feed_filename, sizeof(feed->feed_filename), "/tmp/%s.ffm", feed->filename);
		feed->feed_max_size = 5 * 1024 * 1024;
		feed->is_feed = 1;
		feed->feed = feed; /* self feeding :-) */
		*pfeed = feed;
		return 0;
	}

	av_assert0(feed);
	if (!av_strcasecmp(cmd, "Launch"))
	{
		int i;

		feed->child_argv = av_mallocz_array(MAX_CHILD_ARGS, sizeof(char *));
		if (!feed->child_argv)
			return AVERROR(ENOMEM);
		for (i = 0; i < MAX_CHILD_ARGS - 2; i++)
		{
			muxUtilGetArg(arg, sizeof(arg), p);
			if (!arg[0])
				break;

			feed->child_argv[i] = av_strdup(arg);
			if (!feed->child_argv[i])
				return AVERROR(ENOMEM);
		}

		feed->child_argv[i] = av_asprintf("http://%s:%d/%s", (config->http_addr.sin_addr.s_addr == INADDR_ANY) ? "127.0.0.1" : inet_ntoa(config->http_addr.sin_addr),
			ntohs(config->http_addr.sin_port), feed->filename);
		
		if (!feed->child_argv[i])
			return AVERROR(ENOMEM);
	}
	else if (!av_strcasecmp(cmd, "ACL"))
	{
		muxSvrParseAclRow(NULL, feed, NULL, *p, config->filename, config->line_num);
	}
	else if (!av_strcasecmp(cmd, "File") ||!av_strcasecmp(cmd, "ReadOnlyFile"))
	{
		muxUtilGetArg(feed->feed_filename, sizeof(feed->feed_filename), p);
		feed->readonly = !av_strcasecmp(cmd, "ReadOnlyFile");
	}
	else if (!av_strcasecmp(cmd, "Truncate"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		/* assume Truncate is true in case no argument is specified */
		if (!arg[0])
		{
			feed->truncate = 1;
		}
		else
		{
			CONFIG_WARNING("Truncate N syntax in configuration file is deprecated. Use Truncate alone with no arguments.\n");
			feed->truncate = strtod(arg, NULL);
		}
	}
	else if (!av_strcasecmp(cmd, "FileMaxSize"))
	{
		char *p1;
		double fsize;

		muxUtilGetArg(arg, sizeof(arg), p);
		p1 = arg;
		fsize = strtod(p1, &p1);
		switch(av_toupper(*p1))
		{
			case 'K':
				fsize *= 1024;
				break;
				
			case 'M':
				fsize *= 1024 * 1024;
				break;
				
			case 'G':
				fsize *= 1024 * 1024 * 1024;
				break;
				
			default:
				CONFIG_ERROR("Invalid file size: '%s'\n", arg);
				break;
		}

		feed->feed_max_size = (int64_t)fsize;
		if (feed->feed_max_size < MUX_PACKET_SIZE*4)
		{
			CONFIG_ERROR("Feed max file size is too small. Must be at least %d.\n",MUX_PACKET_SIZE*4);
		}
	}
	else if (!av_strcasecmp(cmd, "</Feed>"))
	{
		*pfeed = NULL;
	}
	else
	{
		CONFIG_ERROR("Invalid entry '%s' inside <Feed></Feed>\n", cmd);
	}
	return 0;
}

/* only url for redirect stream */
static int _parseRedirect(MuxServerConfig *config, const char *cmd, const char **p, MuxStream **predirect)
{
	MuxStream *redirect;
	av_assert0(predirect);
	redirect = *predirect;

	if (!av_strcasecmp(cmd, "<Redirect"))
	{
		char *q;
		redirect = av_mallocz(sizeof(MuxStream));
		if (!redirect)
			return AVERROR(ENOMEM);

		muxUtilGetArg(redirect->filename, sizeof(redirect->filename), p);
		q = strrchr(redirect->filename, '>');
		if (*q)
			*q = '\0';
		
		redirect->stream_type = STREAM_TYPE_REDIRECT;
		*predirect = redirect;
		return 0;
	}
	
	av_assert0(redirect);
	
	if (!av_strcasecmp(cmd, "URL"))
	{
		muxUtilGetArg(redirect->feed_filename, sizeof(redirect->feed_filename), p);
	}
	else if (!av_strcasecmp(cmd, "</Redirect>"))
	{
		if (!redirect->feed_filename[0])
			CONFIG_ERROR("No URL found for <Redirect>\n");
		*predirect = NULL;
	}
	else
	{
		CONFIG_ERROR("Invalid entry '%s' inside <Redirect></Redirect>\n", cmd);
	}
	
	return 0;
}

int muxSvrParseConfig(const char *filename, MuxServerConfig *config)
{
	FILE *f;
	char line[1024];
	char cmd[64];
	const char *p;
	MuxStream **last_stream, *stream = NULL, *redirect = NULL;
	MuxStream **last_feed, *feed = NULL;
	int ret = 0;

	av_assert0(config);

	f = fopen(filename, "r");
	if (!f)
	{
		ret = AVERROR(errno);
		av_log(NULL, AV_LOG_ERROR, "Could not open the configuration file '%s'\n", filename);
		return ret;
	}

	config->first_stream = NULL;
	config->first_feed = NULL;
	config->errors = config->warnings = 0;

	last_stream = &config->first_stream;
	last_feed = &config->first_feed;

	config->line_num = 0;
	while (fgets(line, sizeof(line), f) != NULL)
	{
		config->line_num++;
		p = line;
		while (av_isspace(*p))
			p++;
		
		if (*p == '\0' || *p == '#')
			continue;

		muxUtilGetArg(cmd, sizeof(cmd), &p);

		if (feed || !av_strcasecmp(cmd, "<Feed"))
		{
			int opening = !av_strcasecmp(cmd, "<Feed");
			if (opening && (stream || feed || redirect))
			{
				CONFIG_ERROR("Already in a tag\n");
			}
			else
			{
				ret = _parseFeed(config, cmd, &p, &feed);
				if (ret < 0)
					break;
				
				if (opening)
				{/* add in stream & feed list */
					*last_stream = feed;
					*last_feed = feed;
					last_stream = &feed->next;
					last_feed = &feed->next_feed;
				}
			}
		}
		else if (stream || !av_strcasecmp(cmd, "<Stream"))
		{
			int opening = !av_strcasecmp(cmd, "<Stream");
			if (opening && (stream || feed || redirect))
			{
				CONFIG_ERROR("Already in a tag\n");
			}
			else
			{
				ret = muxCfgParseStream(config, cmd, &p, &stream);
				if (ret < 0)
					break;
				
				if (opening)
				{/* add in stream list */
					*last_stream = stream;
					last_stream = &stream->next;
				}
			}
		}
		else if (redirect || !av_strcasecmp(cmd, "<Redirect"))
		{
			int opening = !av_strcasecmp(cmd, "<Redirect");
			if (opening && (stream || feed || redirect))
				CONFIG_ERROR("Already in a tag\n");
			else
			{
				ret = _parseRedirect(config, cmd, &p, &redirect);
				if (ret < 0)
					break;
				
				if (opening)
				{/* add in stream list */
					*last_stream = redirect;
					last_stream = &redirect->next;
				}
			}
		}
		else
		{
			_parseGlobal(config, cmd, &p);
		}
	}
	
	if (stream || feed || redirect)
		CONFIG_ERROR("Missing closing </%s> tag\n", stream ? "Stream" : (feed ? "Feed" : "Redirect"));

	fclose(f);
	if (ret < 0)
		return ret;
	
	if (config->errors)
		return AVERROR(EINVAL);
	else
		return 0;
}


