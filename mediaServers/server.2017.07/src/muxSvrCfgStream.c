
#include "muxSvr.h"

#define	LTRACE()	\
		printf("%s.%s().%d\n", __FILE__, __FUNCTION__, __LINE__)

int muxCfgSetFloatParam(float *dest, const char *value, float factor, float min, float max,
	MuxServerConfig *config, const char *error_msg, ...);
int muxCfgSetIntegerParam(int *dest, const char *value, int factor,
	int min, int max, MuxServerConfig *config, const char *error_msg, ...);

AVOutputFormat *muxCfgGuessFormat(const char *short_name, const char *filename, const char *mime_type);
int muxCfgSetCodec(AVCodecContext *ctx, const char *codec_name, MuxServerConfig *config);
FILE *muxCfgGetPresetFile(char *filename, size_t filename_size, const char *preset_name, int is_path, const char *codec_name);


/* add a codec and set the default parameters */
static void _addCodec(MuxStream *stream, AVCodecContext *av, MuxServerConfig *config)
{
	LayeredAVStream *st;
	AVDictionary **opts, *recommended = NULL;
	char *enc_config;

	if(stream->nb_streams >= MUX_ARRAY_ELEMS(stream->streams))
		return;

	opts = av->codec_type == AVMEDIA_TYPE_AUDIO ?&config->audio_opts : &config->video_opts;
	av_dict_copy(&recommended, *opts, 0);
	av_opt_set_dict2(av->priv_data, opts, AV_OPT_SEARCH_CHILDREN);
	av_opt_set_dict2(av, opts, AV_OPT_SEARCH_CHILDREN);

	if (av_dict_count(*opts))
		av_log(NULL, AV_LOG_WARNING, "Something is wrong, %d options are not set!\n", av_dict_count(*opts));

	if (!config->stream_use_defaults)
	{
		switch(av->codec_type)
		{
			case AVMEDIA_TYPE_AUDIO:
				if (av->bit_rate == 0)
					CONFIG_ERROR("audio bit rate is not set\n");
				if (av->sample_rate == 0)
					CONFIG_ERROR("audio sample rate is not set\n");
				break;
				
			case AVMEDIA_TYPE_VIDEO:
				if (av->width == 0 || av->height == 0)
					CONFIG_ERROR("video size is not set\n");
				break;
				
			default:
				av_assert0(0);
		}
		goto done;
	}

	/* stream_use_defaults = true */

	/* compute default parameters */
	switch(av->codec_type)
	{
		case AVMEDIA_TYPE_AUDIO:
			if (!av_dict_get(recommended, "b", NULL, 0))
			{
				av->bit_rate = 64000;
				av_dict_set_int(&recommended, "b", av->bit_rate, 0);
				CONFIG_WARNING("Setting default value for audio bit rate = %d. Use NoDefaults to disable it.\n", av->bit_rate);
			}
			
			if (!av_dict_get(recommended, "ar", NULL, 0))
			{
				av->sample_rate = 22050;
				av_dict_set_int(&recommended, "ar", av->sample_rate, 0);
				CONFIG_WARNING("Setting default value for audio sample rate = %d. Use NoDefaults to disable it.\n",av->sample_rate);
			}
			
			if (!av_dict_get(recommended, "ac", NULL, 0))
			{
				av->channels = 1;
				av_dict_set_int(&recommended, "ac", av->channels, 0);
				CONFIG_WARNING("Setting default value for audio channel count = %d. Use NoDefaults to disable it.\n",av->channels);
			}
			break;

		case AVMEDIA_TYPE_VIDEO:
			if (!av_dict_get(recommended, "b", NULL, 0))
			{
				av->bit_rate = 64000;
				av_dict_set_int(&recommended, "b", av->bit_rate, 0);
				CONFIG_WARNING("Setting default value for video bit rate = %d. Use NoDefaults to disable it.\n", av->bit_rate);
			}
			
			if (!av_dict_get(recommended, "time_base", NULL, 0))
			{
				av->time_base.den = 5;
				av->time_base.num = 1;
				av_dict_set(&recommended, "time_base", "1/5", 0);
				CONFIG_WARNING("Setting default value for video frame rate = %d. Use NoDefaults to disable it.\n", av->time_base.den);
			}
			
			if (!av_dict_get(recommended, "video_size", NULL, 0))
			{
				av->width = 160;
				av->height = 128;
				av_dict_set(&recommended, "video_size", "160x128", 0);
				CONFIG_WARNING("Setting default value for video size = %dx%d. Use NoDefaults to disable it.\n", av->width, av->height);
			}
			
			/* Bitrate tolerance is less for streaming */
			if (!av_dict_get(recommended, "bt", NULL, 0))
			{
				av->bit_rate_tolerance = FFMAX(av->bit_rate / 4, (int64_t)av->bit_rate*av->time_base.num/av->time_base.den);
				av_dict_set_int(&recommended, "bt", av->bit_rate_tolerance, 0);
				CONFIG_WARNING("Setting default value for video bit rate tolerance = %d. Use NoDefaults to disable it.\n", av->bit_rate_tolerance);
			}

			if (!av_dict_get(recommended, "rc_eq", NULL, 0))
			{
				av->rc_eq = av_strdup("tex^qComp");
				av_dict_set(&recommended, "rc_eq", "tex^qComp", 0);
				CONFIG_WARNING("Setting default value for video rate control equation = %s. Use NoDefaults to disable it.\n", av->rc_eq);
			}
			
			if (!av_dict_get(recommended, "maxrate", NULL, 0))
			{
				av->rc_max_rate = av->bit_rate * 2;
				av_dict_set_int(&recommended, "maxrate", av->rc_max_rate, 0);
				CONFIG_WARNING("Setting default value for video max rate = %d. Use NoDefaults to disable it.\n", av->rc_max_rate);
			}

			if (av->rc_max_rate && !av_dict_get(recommended, "bufsize", NULL, 0))
			{
				av->rc_buffer_size = av->rc_max_rate;
				av_dict_set_int(&recommended, "bufsize", av->rc_buffer_size, 0);
				CONFIG_WARNING("Setting default value for video buffer size = %d. Use NoDefaults to disable it.\n", av->rc_buffer_size);
			}
			break;

		default:
			abort();
	}

done:
	st = av_mallocz(sizeof(*st));
	if (!st)
		return;

	av_dict_get_string(recommended, &enc_config, '=', ',');
	av_dict_free(&recommended);
	st->recommended_encoder_configuration = enc_config;
	st->codec = av;
	st->codecpar = avcodec_parameters_alloc();
	avcodec_parameters_from_context(st->codecpar, av);
	stream->streams[stream->nb_streams++] = st;
}

static int _saveAvOption(const char *opt, const char *arg, int type, MuxServerConfig *config)
{
	static int hinted = 0;
	int ret = 0;
	AVDictionaryEntry *e;
	const AVOption *o = NULL;
	const char *option = NULL;
	const char *codec_name = NULL;
	char buff[1024];
	AVCodecContext *ctx;
	AVDictionary **dict;
	enum AVCodecID guessed_codec_id;

	switch (type)
	{
		case AV_OPT_FLAG_VIDEO_PARAM:
			ctx = config->dummy_vctx;
			dict = &config->video_opts;
			guessed_codec_id = config->guessed_video_codec_id != AV_CODEC_ID_NONE ? config->guessed_video_codec_id : AV_CODEC_ID_H264;
			break;
		
		case AV_OPT_FLAG_AUDIO_PARAM:
			ctx = config->dummy_actx;
			dict = &config->audio_opts;
			guessed_codec_id = config->guessed_audio_codec_id != AV_CODEC_ID_NONE ?config->guessed_audio_codec_id : AV_CODEC_ID_AAC;
			break;
		
		default:
			av_assert0(0);
	}

	if (strchr(opt, ':'))
	{
		//explicit private option
		snprintf(buff, sizeof(buff), "%s", opt);
		codec_name = buff;
		
		if(!(option = strchr(buff, ':')))
		{
			CONFIG_ERROR( "Syntax error. Unmatched ':'\n");
			return -1;
		}
		
		buff[option - buff] = '\0';
		option++;
		if ((ret = muxCfgSetCodec(ctx, codec_name, config)) < 0)
			return ret;
		if (!ctx->codec || !ctx->priv_data)
			return -1;
	}
	else
	{
		option = opt;
	}

	o = av_opt_find(ctx, option, NULL, type | AV_OPT_FLAG_ENCODING_PARAM, AV_OPT_SEARCH_CHILDREN);
	if (!o && (!strcmp(option, "time_base")  || !strcmp(option, "pixel_format") || !strcmp(option, "video_size") || !strcmp(option, "codec_tag")))
		o = av_opt_find(ctx, option, NULL, 0, 0);

	if (!o)
	{
		CONFIG_ERROR( "Option not found: '%s'\n", opt);
		
		if (!hinted && ctx->codec_id == AV_CODEC_ID_NONE)
		{
			hinted = 1;
			muxCfgError(config->filename, config->line_num, AV_LOG_ERROR, NULL, "If '%s' is a codec private option, then prefix it with codec name, for "
				"example '%s:%s %s' or define codec earlier.\n", opt, avcodec_get_name(guessed_codec_id) ,opt, arg);
		}
	}
	else if ((ret = av_opt_set(ctx, option, arg, AV_OPT_SEARCH_CHILDREN)) < 0)
	{
		CONFIG_ERROR( "Invalid value for option %s (%s): %s\n", opt, arg, av_err2str(ret));
	}
	else if ((e = av_dict_get(*dict, option, NULL, 0)))
	{
		if ((o->type == AV_OPT_TYPE_FLAGS) && arg && (arg[0] == '+' || arg[0] == '-'))
			return av_dict_set(dict, option, arg, AV_DICT_APPEND);

		CONFIG_ERROR("Redeclaring value of option '%s'.Previous value was: '%s'.\n", opt, e->value);
	}
	else if (av_dict_set(dict, option, arg, 0) < 0)
	{
		return AVERROR(ENOMEM);
	}
	
	return 0;
}

static int _saveAvOptionInteger(const char *opt, int64_t arg, int type, MuxServerConfig *config)
{
	char buf[22];
	snprintf(buf, sizeof(buf), "%"PRId64, arg);
	return _saveAvOption(opt, buf, type, config);
}



static int _optPreset(const char *arg, int type, MuxServerConfig *config)
{
	FILE *f=NULL;
	char filename[1000], tmp[1000], tmp2[1000], line[1000];
	int ret = 0;
	AVCodecContext *avctx;
	const AVCodec *codec;

	switch(type)
	{
		case AV_OPT_FLAG_AUDIO_PARAM:
			avctx = config->dummy_actx;
			break;
			
		case AV_OPT_FLAG_VIDEO_PARAM:
			avctx = config->dummy_vctx;
			break;
			
		default:
			av_assert0(0);
	}
	codec = avcodec_find_encoder(avctx->codec_id);

	if (!(f = muxCfgGetPresetFile(filename, sizeof(filename), arg, 0, codec ? codec->name : NULL)))
	{
		av_log(NULL, AV_LOG_ERROR, "File for preset '%s' not found\n", arg);
		return AVERROR(EINVAL);
	}

	while(!feof(f))
	{
		int e= fscanf(f, "%999[^\n]\n", line) - 1;
		if(line[0] == '#' && !e)
			continue;
		
		e|= sscanf(line, "%999[^=]=%999[^\n]\n", tmp, tmp2) - 2;
		if(e)
		{
			av_log(NULL, AV_LOG_ERROR, "%s: Invalid syntax: '%s'\n", filename, line);
			ret = AVERROR(EINVAL);
			break;
		}

		if ((!strcmp(tmp, "acodec") && (avctx->codec_type == AVMEDIA_TYPE_AUDIO))||
			(!strcmp(tmp, "vcodec") && (avctx->codec_type == AVMEDIA_TYPE_VIDEO)) )
		{
			if (muxCfgSetCodec(avctx, tmp2, config) < 0)
				break;
		}
		else if (!strcmp(tmp, "scodec"))
		{
			av_log(NULL, AV_LOG_ERROR, "Subtitles preset found.\n");
			ret = AVERROR(EINVAL);
			break;
		}
		else if (_saveAvOption(tmp, tmp2, type, config) < 0)
			break;
	}

	fclose(f);

	return ret;
}

int muxCfgParseStream(MuxServerConfig *config, const char *cmd,  const char **p, MuxStream **pstream)
{
	char arg[1024], arg2[1024];
	MuxStream *stream;
	int val;

	av_assert0(pstream);
	stream = *pstream;

	if (!av_strcasecmp(cmd, "<Stream"))
	{
		char *q;
		MuxStream *s;
		stream = av_mallocz(sizeof(MuxStream));
		if (!stream)
			return AVERROR(ENOMEM);
		
		config->dummy_actx = avcodec_alloc_context3(NULL);
		config->dummy_vctx = avcodec_alloc_context3(NULL);
		
		if (!config->dummy_vctx || !config->dummy_actx)
		{
			av_free(stream);
			avcodec_free_context(&config->dummy_vctx);
			avcodec_free_context(&config->dummy_actx);
			return AVERROR(ENOMEM);
		}
		
		config->dummy_actx->codec_type = AVMEDIA_TYPE_AUDIO;
		config->dummy_vctx->codec_type = AVMEDIA_TYPE_VIDEO;
		muxUtilGetArg(stream->filename, sizeof(stream->filename), p);
		q = strrchr(stream->filename, '>');
		if (q)
			*q = '\0';

		for (s = config->first_stream; s; s = s->next)
		{
			if (!strcmp(stream->filename, s->filename))
				CONFIG_ERROR("Stream '%s' already registered\n", s->filename);
		}

		stream->fmt = muxCfgGuessFormat(NULL, stream->filename, NULL);
		if (stream->fmt)
		{
			config->guessed_audio_codec_id = stream->fmt->audio_codec;
			config->guessed_video_codec_id = stream->fmt->video_codec;
		}
		else
		{
			config->guessed_audio_codec_id = AV_CODEC_ID_NONE;
			config->guessed_video_codec_id = AV_CODEC_ID_NONE;
		}
		
		config->stream_use_defaults = config->use_defaults;
		*pstream = stream;
		return 0;
	}
	av_assert0(stream);
	
	if (!av_strcasecmp(cmd, "Feed"))
	{
		MuxStream *sfeed;
		muxUtilGetArg(arg, sizeof(arg), p);
		sfeed = config->first_feed;
		
		while (sfeed)
		{
			if (!strcmp(sfeed->filename, arg))
				break;
			sfeed = sfeed->next_feed;
		}
		
		if (!sfeed)
			CONFIG_ERROR("Feed with name '%s' for stream '%s' is not defined\n", arg, stream->filename);
		else
			stream->feed = sfeed;
	}
	else if (!av_strcasecmp(cmd, "Format"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);

		if (!strcmp(arg, "status"))
		{
			stream->stream_type = STREAM_TYPE_STATUS;
			stream->fmt = NULL;
		}
		else
		{
			stream->stream_type = STREAM_TYPE_LIVE;
			/* JPEG cannot be used here, so use single frame MJPEG */
			if (!strcmp(arg, "jpeg"))
			{
				strcpy(arg, "singlejpeg");
				stream->single_frame=1;
			}
			
			stream->fmt = muxCfgGuessFormat(arg, NULL, NULL);
//			stream->fmt = muxCfgGuessFormat(arg, stream->filename, NULL);
			if (!stream->fmt)
				CONFIG_ERROR("Unknown Format: '%s'\n", arg);

			printf("OutFormatContext name:'%s'\n", stream->fmt->name );
			/* AvFormatContext.oformat can't be set options here, the options can only be set when avformat_write_header() is called. Jack Lee, August 30,2017 */
			if (0)//!strcmp(stream->fmt->name, "hls"))
			{
				printf("%s.%s().%d\n", __FILE__, __FUNCTION__, __LINE__);
				LTRACE();
//				av_opt_set(&stream->fmt->priv_class, "hls_flags", "single_file", 0);//AV_OPT_SEARCH_CHILDREN);
				av_opt_set_int(&stream->fmt->priv_class, "hls_time", 10, 0);//AV_OPT_SEARCH_CHILDREN);
				av_opt_set_int(&stream->fmt->priv_class, "hls_list_size", 6, 0);//AV_OPT_SEARCH_CHILDREN);
				av_opt_set(&stream->fmt->priv_class, "hls_flags", "delete_segments", 0);//AV_OPT_SEARCH_CHILDREN);
				LTRACE();
			}
		}


		
		if (stream->fmt)
		{
			config->guessed_audio_codec_id = stream->fmt->audio_codec;
			config->guessed_video_codec_id = stream->fmt->video_codec;
		}
	}
	else if (!av_strcasecmp(cmd, "InputFormat"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		stream->ifmt = av_find_input_format(arg);
		if (!stream->ifmt)
			CONFIG_ERROR("Unknown input format: '%s'\n", arg);
	}
	else if (!av_strcasecmp(cmd, "FaviconURL"))
	{
		if (stream->stream_type == STREAM_TYPE_STATUS)
			muxUtilGetArg(stream->feed_filename, sizeof(stream->feed_filename), p);
		else
			CONFIG_ERROR("FaviconURL only permitted for status streams\n");
	}
	else if (!av_strcasecmp(cmd, "Author") || !av_strcasecmp(cmd, "Comment")   ||
		!av_strcasecmp(cmd, "Copyright") ||!av_strcasecmp(cmd, "Title"))
	{
		char key[32];
		int i;
		muxUtilGetArg(arg, sizeof(arg), p);
		for (i = 0; i < strlen(cmd); i++)
			key[i] = av_tolower(cmd[i]);

		key[i] = 0;
		CONFIG_WARNING("Deprecated '%s' option in configuration file. Use 'Metadata %s VALUE' instead.\n", cmd, key);
		if (av_dict_set(&stream->metadata, key, arg, 0) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "Metadata"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxUtilGetArg(arg2, sizeof(arg2), p);
		if (av_dict_set(&stream->metadata, arg, arg2, 0) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "Preroll"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		stream->prebuffer = atof(arg) * 1000;
	}
	else if (!av_strcasecmp(cmd, "StartSendOnKey"))
	{
		stream->send_on_key = 1;
	}
	else if (!av_strcasecmp(cmd, "AudioCodec"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetCodec(config->dummy_actx, arg, config);
	}
	else if (!av_strcasecmp(cmd, "VideoCodec"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetCodec(config->dummy_vctx, arg, config);
	}
	else if (!av_strcasecmp(cmd, "MaxTime"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		stream->max_time = atof(arg) * 1000;
	}
	else if (!av_strcasecmp(cmd, "AudioBitRate"))
	{
		float f;
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetFloatParam(&f, arg, 1000, -FLT_MAX, FLT_MAX, config, "Invalid %s: '%s'\n", cmd, arg);
		if (_saveAvOptionInteger("b", (int64_t)lrintf(f), AV_OPT_FLAG_AUDIO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "AudioChannels"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("ac", arg, AV_OPT_FLAG_AUDIO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "AudioSampleRate"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("ar", arg, AV_OPT_FLAG_AUDIO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoBitRateRange"))
	{
		int minrate, maxrate;
		char *dash;
		muxUtilGetArg(arg, sizeof(arg), p);
		dash = strchr(arg, '-');
		if (dash)
		{
			*dash = '\0';
			dash++;
			if (muxCfgSetIntegerParam(&minrate, arg,  1000, 0, INT_MAX, config, "Invalid %s: '%s'", cmd, arg) >= 0 &&
				muxCfgSetIntegerParam(&maxrate, dash, 1000, 0, INT_MAX, config, "Invalid %s: '%s'", cmd, arg) >= 0)
			{
				if (_saveAvOptionInteger("minrate", minrate, AV_OPT_FLAG_VIDEO_PARAM, config) < 0 ||
					_saveAvOptionInteger("maxrate", maxrate, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
					goto nomem;
			}
		}
		else
			CONFIG_ERROR("Incorrect format for VideoBitRateRange. It should be <min>-<max>: '%s'.\n", arg);
	}
	else if (!av_strcasecmp(cmd, "Debug"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("debug", arg, AV_OPT_FLAG_AUDIO_PARAM, config) < 0 ||
			_saveAvOption("debug", arg, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "Strict"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("strict", arg, AV_OPT_FLAG_AUDIO_PARAM, config) < 0 ||
			_saveAvOption("strict", arg, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoBufferSize"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 8*1024, 0, INT_MAX, config, "Invalid %s: '%s'", cmd, arg);
		if (_saveAvOptionInteger("bufsize", val, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoBitRateTolerance"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 1000, INT_MIN, INT_MAX, config, "Invalid %s: '%s'", cmd, arg);
		if (_saveAvOptionInteger("bt", val, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoBitRate"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 1000, INT_MIN, INT_MAX, config,	"Invalid %s: '%s'", cmd, arg);
		if (_saveAvOptionInteger("b", val, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoSize"))
	{
		int ret, w, h;
		muxUtilGetArg(arg, sizeof(arg), p);
		ret = av_parse_video_size(&w, &h, arg);
		if (ret < 0)
			CONFIG_ERROR("Invalid video size '%s'\n", arg);
		else
		{
			if (w % 2 || h % 2)
				CONFIG_WARNING("Image size is not a multiple of 2\n");
			if (_saveAvOption("video_size", arg, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
				goto nomem;
		}
	}
	else if (!av_strcasecmp(cmd, "VideoFrameRate"))
	{
		muxUtilGetArg(&arg[2], sizeof(arg) - 2, p);
		arg[0] = '1'; arg[1] = '/';
		if (_saveAvOption("time_base", arg, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "PixelFormat"))
	{
		enum AVPixelFormat pix_fmt;
		
		muxUtilGetArg(arg, sizeof(arg), p);
		pix_fmt = av_get_pix_fmt(arg);
		if (pix_fmt == AV_PIX_FMT_NONE)
			CONFIG_ERROR("Unknown pixel format: '%s'\n", arg);
		else if (_saveAvOption("pixel_format", arg, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoGopSize"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("g", arg, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoIntraOnly"))
	{
		if (_saveAvOption("g", "1", AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoHighQuality"))
	{
		if (_saveAvOption("mbd", "+bits", AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "Video4MotionVector"))
	{
		if (_saveAvOption("mbd", "+bits",  AV_OPT_FLAG_VIDEO_PARAM, config) < 0 || //FIXME remove
			_saveAvOption("flags", "+mv4", AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "AVOptionVideo") ||!av_strcasecmp(cmd, "AVOptionAudio"))
	{
		int ret;
		muxUtilGetArg(arg, sizeof(arg), p);
		muxUtilGetArg(arg2, sizeof(arg2), p);
		if (!av_strcasecmp(cmd, "AVOptionVideo"))
			ret = _saveAvOption(arg, arg2, AV_OPT_FLAG_VIDEO_PARAM, config);
		else
			ret = _saveAvOption(arg, arg2, AV_OPT_FLAG_AUDIO_PARAM, config);
		
		if (ret < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "AVPresetVideo") ||!av_strcasecmp(cmd, "AVPresetAudio"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (!av_strcasecmp(cmd, "AVPresetVideo"))
			_optPreset(arg, AV_OPT_FLAG_VIDEO_PARAM, config);
		else
			_optPreset(arg, AV_OPT_FLAG_AUDIO_PARAM, config);
	}
	else if (!av_strcasecmp(cmd, "VideoTag"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (strlen(arg) == 4 && 	_saveAvOptionInteger("codec_tag", MKTAG(arg[0], arg[1], arg[2], arg[3]), AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "BitExact"))
	{
		config->bitexact = 1;
		if (_saveAvOption("flags", "+bitexact", AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "DctFastint"))
	{
		if (_saveAvOption("dct", "fastint", AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "IdctSimple"))
	{
		if (_saveAvOption("idct", "simple", AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "Qscale"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, INT_MIN, INT_MAX, config, "Invalid Qscale: '%s'\n", arg);
		
		if (_saveAvOption("flags", "+qscale", AV_OPT_FLAG_VIDEO_PARAM, config) < 0 ||
			_saveAvOptionInteger("global_quality", FF_QP2LAMBDA * val, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoQDiff")) {
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("qdiff", arg, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoQMax"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("qmax", arg, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoQMin"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("qmin", arg, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "LumiMask"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("lumi_mask", arg, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "DarkMask"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("dark_mask", arg, AV_OPT_FLAG_VIDEO_PARAM, config) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "NoVideo"))
	{
		config->no_video = 1;
	}
	else if (!av_strcasecmp(cmd, "NoAudio"))
	{
		config->no_audio = 1;
	}
	else if (!av_strcasecmp(cmd, "ACL"))
	{
		muxSvrParseAclRow(stream, NULL, NULL, *p, config->filename, config->line_num);
	}
	else if (!av_strcasecmp(cmd, "DynamicACL"))
	{
		muxUtilGetArg(stream->dynamic_acl, sizeof(stream->dynamic_acl), p);
	}
	else if (!av_strcasecmp(cmd, "RTSPOption"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		av_freep(&stream->rtsp_option);
		stream->rtsp_option = av_strdup(arg);
	}
	else if (!av_strcasecmp(cmd, "MulticastAddress"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (muxUtilResolveHost(&stream->multicast_ip, arg))
			CONFIG_ERROR("Invalid host/IP address: '%s'\n", arg);
		stream->is_multicast = 1;
		stream->loop = 1; /* default is looping */
	}
	else if (!av_strcasecmp(cmd, "MulticastPort"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, 1, 65535, config, "Invalid MulticastPort: '%s'\n", arg);
		stream->multicast_port = val;
	}
	else if (!av_strcasecmp(cmd, "MulticastTTL"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, INT_MIN, INT_MAX, config,"Invalid MulticastTTL: '%s'\n", arg);
		stream->multicast_ttl = val;
	}
	else if (!av_strcasecmp(cmd, "NoLoop"))
	{
		stream->loop = 0;
	}
	else if (!av_strcasecmp(cmd, "</Stream>"))
	{
		config->stream_use_defaults &= 1;
		if (stream->feed && stream->fmt && strcmp(stream->fmt->name, "ffm"))
		{
			if (config->dummy_actx->codec_id == AV_CODEC_ID_NONE)
				config->dummy_actx->codec_id = config->guessed_audio_codec_id;
			if (!config->no_audio && config->dummy_actx->codec_id != AV_CODEC_ID_NONE)
			{
				AVCodecContext *audio_enc = avcodec_alloc_context3(avcodec_find_encoder(config->dummy_actx->codec_id));
				_addCodec(stream, audio_enc, config);
			}

			if (config->dummy_vctx->codec_id == AV_CODEC_ID_NONE)
				config->dummy_vctx->codec_id = config->guessed_video_codec_id;

			if (!config->no_video && config->dummy_vctx->codec_id != AV_CODEC_ID_NONE)
			{
				AVCodecContext *video_enc = avcodec_alloc_context3(avcodec_find_encoder(config->dummy_vctx->codec_id));
				_addCodec(stream, video_enc, config);
			}
		}
		
		av_dict_free(&config->video_opts);
		av_dict_free(&config->audio_opts);
		avcodec_free_context(&config->dummy_vctx);
		avcodec_free_context(&config->dummy_actx);
		config->no_video = 0;
		config->no_audio = 0;
		*pstream = NULL;
	}
	else if (!av_strcasecmp(cmd, "File") ||!av_strcasecmp(cmd, "ReadOnlyFile"))
	{
		muxUtilGetArg(stream->feed_filename, sizeof(stream->feed_filename), p);
	}
	else if (!av_strcasecmp(cmd, "UseDefaults"))
	{
		if (config->stream_use_defaults > 1)
			CONFIG_WARNING("Multiple UseDefaults/NoDefaults entries.\n");
		config->stream_use_defaults = 3;
	}
	else if (!av_strcasecmp(cmd, "NoDefaults"))
	{
		if (config->stream_use_defaults > 1)
			CONFIG_WARNING("Multiple UseDefaults/NoDefaults entries.\n");
		config->stream_use_defaults = 2;
	}
	else
	{
		CONFIG_ERROR("Invalid entry '%s' inside <Stream></Stream>\n", cmd);
	}
	
	return 0;
	
nomem:
	av_log(NULL, AV_LOG_ERROR, "Out of memory. Aborting.\n");
	av_dict_free(&config->video_opts);
	av_dict_free(&config->audio_opts);
	avcodec_free_context(&config->dummy_vctx);
	avcodec_free_context(&config->dummy_actx);
	return AVERROR(ENOMEM);
}

