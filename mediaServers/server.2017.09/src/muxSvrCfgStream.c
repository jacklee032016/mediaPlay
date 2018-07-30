

#include "muxSvr.h"

int muxCfgSetFloatParam(float *dest, const char *value, float factor, float min, float max,
	MuxServerConfig *svrConfig, const char *error_msg, ...);
int muxCfgSetIntegerParam(int *dest, const char *value, int factor,
	int min, int max, MuxServerConfig *svrConfig, const char *error_msg, ...);

AVOutputFormat *muxCfgGuessFormat(const char *short_name, const char *filename, const char *mime_type);
int muxCfgSetCodec(AVCodecContext *ctx, const char *codec_name, MuxServerConfig *svrConfig);
FILE *muxCfgGetPresetFile(char *filename, size_t filename_size, const char *preset_name, int is_path, const char *codec_name);


static int _saveAvOption(const char *opt, const char *arg, int type, MuxServerConfig *svrConfig)
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
			ctx = svrConfig->dummy_vctx;
			dict = &svrConfig->video_opts;
			guessed_codec_id = svrConfig->guessed_video_codec_id != AV_CODEC_ID_NONE ? svrConfig->guessed_video_codec_id : AV_CODEC_ID_H264;
			break;
		
		case AV_OPT_FLAG_AUDIO_PARAM:
			ctx = svrConfig->dummy_actx;
			dict = &svrConfig->audio_opts;
			guessed_codec_id = svrConfig->guessed_audio_codec_id != AV_CODEC_ID_NONE ?svrConfig->guessed_audio_codec_id : AV_CODEC_ID_AAC;
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
		if ((ret = muxCfgSetCodec(ctx, codec_name, svrConfig)) < 0)
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
			muxCfgError(svrConfig->filename, svrConfig->line_num, AV_LOG_ERROR, NULL, "If '%s' is a codec private option, then prefix it with codec name, for "
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

static int _saveAvOptionInteger(const char *opt, int64_t arg, int type, MuxServerConfig *svrConfig)
{
	char buf[22];
	snprintf(buf, sizeof(buf), "%"PRId64, arg);
	return _saveAvOption(opt, buf, type, svrConfig);
}



static int _optPreset(const char *arg, int type, MuxServerConfig *svrConfig)
{
	FILE *f=NULL;
	char filename[1000], tmp[1000], tmp2[1000], line[1000];
	int ret = 0;
	AVCodecContext *avctx;
	const AVCodec *codec;

	switch(type)
	{
		case AV_OPT_FLAG_AUDIO_PARAM:
			avctx = svrConfig->dummy_actx;
			break;
			
		case AV_OPT_FLAG_VIDEO_PARAM:
			avctx = svrConfig->dummy_vctx;
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
			if (muxCfgSetCodec(avctx, tmp2, svrConfig) < 0)
				break;
		}
		else if (!strcmp(tmp, "scodec"))
		{
			av_log(NULL, AV_LOG_ERROR, "Subtitles preset found.\n");
			ret = AVERROR(EINVAL);
			break;
		}
		else if (_saveAvOption(tmp, tmp2, type, svrConfig) < 0)
			break;
	}

	fclose(f);

	return ret;
}


int muxCfgParseStream(MuxServerConfig *svrConfig, const char *cmd,  const char **p, MuxStream **pstream)
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
		muxUtilGetArg(stream->filename, sizeof(stream->filename), p);
		q = strrchr(stream->filename, '>');
		if (q)
			*q = '\0';

		if( muxSvrFoundStream(FALSE, stream->filename, svrConfig->muxSvr) )	
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
		muxUtilGetArg(arg, sizeof(arg), p);

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
	else if (!av_strcasecmp(cmd, "InputFormat"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		stream->ifmt = av_find_input_format(arg);
		if (!stream->ifmt)
			CONFIG_ERROR("Unknown input format: '%s'\n", arg);
	}
	else if (!av_strcasecmp(cmd, "RtmpURL"))
	{
		muxUtilGetArg(stream->rtmpUrl, sizeof(stream->rtmpUrl), p);
		stream->type = MUX_STREAM_TYPE_RTMP;
	}
	else if (!av_strcasecmp(cmd, "FaviconURL"))
	{
		if (stream->type == MUX_STREAM_TYPE_STATUS)
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
		muxCfgSetCodec(svrConfig->dummy_actx, arg, svrConfig);
	}
	else if (!av_strcasecmp(cmd, "VideoCodec"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetCodec(svrConfig->dummy_vctx, arg, svrConfig);
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
		muxCfgSetFloatParam(&f, arg, 1000, -FLT_MAX, FLT_MAX, svrConfig, "Invalid %s: '%s'\n", cmd, arg);
		if (_saveAvOptionInteger("b", (int64_t)lrintf(f), AV_OPT_FLAG_AUDIO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "AudioChannels"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("ac", arg, AV_OPT_FLAG_AUDIO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "AudioSampleRate"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("ar", arg, AV_OPT_FLAG_AUDIO_PARAM, svrConfig) < 0)
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
			if (muxCfgSetIntegerParam(&minrate, arg,  1000, 0, INT_MAX, svrConfig, "Invalid %s: '%s'", cmd, arg) >= 0 &&
				muxCfgSetIntegerParam(&maxrate, dash, 1000, 0, INT_MAX, svrConfig, "Invalid %s: '%s'", cmd, arg) >= 0)
			{
				if (_saveAvOptionInteger("minrate", minrate, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0 ||
					_saveAvOptionInteger("maxrate", maxrate, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
					goto nomem;
			}
		}
		else
			CONFIG_ERROR("Incorrect format for VideoBitRateRange. It should be <min>-<max>: '%s'.\n", arg);
	}
	else if (!av_strcasecmp(cmd, "Debug"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("debug", arg, AV_OPT_FLAG_AUDIO_PARAM, svrConfig) < 0 ||
			_saveAvOption("debug", arg, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "Strict"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("strict", arg, AV_OPT_FLAG_AUDIO_PARAM, svrConfig) < 0 ||
			_saveAvOption("strict", arg, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoBufferSize"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 8*1024, 0, INT_MAX, svrConfig, "Invalid %s: '%s'", cmd, arg);
		if (_saveAvOptionInteger("bufsize", val, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoBitRateTolerance"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 1000, INT_MIN, INT_MAX, svrConfig, "Invalid %s: '%s'", cmd, arg);
		if (_saveAvOptionInteger("bt", val, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoBitRate"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 1000, INT_MIN, INT_MAX, svrConfig,	"Invalid %s: '%s'", cmd, arg);
		if (_saveAvOptionInteger("b", val, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
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
			if (_saveAvOption("video_size", arg, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
				goto nomem;
		}
	}
	else if (!av_strcasecmp(cmd, "VideoFrameRate"))
	{
		muxUtilGetArg(&arg[2], sizeof(arg) - 2, p);
		arg[0] = '1'; arg[1] = '/';
		if (_saveAvOption("time_base", arg, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "PixelFormat"))
	{
		enum AVPixelFormat pix_fmt;
		
		muxUtilGetArg(arg, sizeof(arg), p);
		pix_fmt = av_get_pix_fmt(arg);
		if (pix_fmt == AV_PIX_FMT_NONE)
			CONFIG_ERROR("Unknown pixel format: '%s'\n", arg);
		else if (_saveAvOption("pixel_format", arg, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoGopSize"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("g", arg, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoIntraOnly"))
	{
		if (_saveAvOption("g", "1", AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoHighQuality"))
	{
		if (_saveAvOption("mbd", "+bits", AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "Video4MotionVector"))
	{
		if (_saveAvOption("mbd", "+bits",  AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0 || //FIXME remove
			_saveAvOption("flags", "+mv4", AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "AVOptionVideo") ||!av_strcasecmp(cmd, "AVOptionAudio"))
	{
		int ret;
		muxUtilGetArg(arg, sizeof(arg), p);
		muxUtilGetArg(arg2, sizeof(arg2), p);
		if (!av_strcasecmp(cmd, "AVOptionVideo"))
			ret = _saveAvOption(arg, arg2, AV_OPT_FLAG_VIDEO_PARAM, svrConfig);
		else
			ret = _saveAvOption(arg, arg2, AV_OPT_FLAG_AUDIO_PARAM, svrConfig);
		
		if (ret < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "AVPresetVideo") ||!av_strcasecmp(cmd, "AVPresetAudio"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (!av_strcasecmp(cmd, "AVPresetVideo"))
			_optPreset(arg, AV_OPT_FLAG_VIDEO_PARAM, svrConfig);
		else
			_optPreset(arg, AV_OPT_FLAG_AUDIO_PARAM, svrConfig);
	}
	else if (!av_strcasecmp(cmd, "VideoTag"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (strlen(arg) == 4 && 	_saveAvOptionInteger("codec_tag", MKTAG(arg[0], arg[1], arg[2], arg[3]), AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "BitExact"))
	{
		svrConfig->bitexact = 1;
		if (_saveAvOption("flags", "+bitexact", AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "DctFastint"))
	{
		if (_saveAvOption("dct", "fastint", AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "IdctSimple"))
	{
		if (_saveAvOption("idct", "simple", AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "Qscale"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, INT_MIN, INT_MAX, svrConfig, "Invalid Qscale: '%s'\n", arg);
		
		if (_saveAvOption("flags", "+qscale", AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0 ||
			_saveAvOptionInteger("global_quality", FF_QP2LAMBDA * val, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoQDiff")) {
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("qdiff", arg, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoQMax"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("qmax", arg, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "VideoQMin"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("qmin", arg, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "LumiMask"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("lumi_mask", arg, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
	}
	else if (!av_strcasecmp(cmd, "DarkMask"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		if (_saveAvOption("dark_mask", arg, AV_OPT_FLAG_VIDEO_PARAM, svrConfig) < 0)
			goto nomem;
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
#if 0	
		muxUtilGetArg(arg, sizeof(arg), p);
		if (muxUtilResolveHost(&stream->multicast_ip, arg))
			CONFIG_ERROR("Invalid host/IP address: '%s'\n", arg);
#endif
		muxUtilGetArg(stream->multicastAddress.address, sizeof(stream->multicastAddress.address), p);

		stream->is_multicast = 1;
		stream->type = MUX_STREAM_TYPE_UMC;
	}
	else if (!av_strcasecmp(cmd, "MulticastPort"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, 1, 65535, svrConfig, "Invalid MulticastPort: '%s'\n", arg);
		stream->multicastAddress.port = val;
	}
	else if (!av_strcasecmp(cmd, "MulticastTTL"))
	{
		muxUtilGetArg(arg, sizeof(arg), p);
		muxCfgSetIntegerParam(&val, arg, 0, INT_MIN, INT_MAX, svrConfig,"Invalid MulticastTTL: '%s'\n", arg);
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
		muxUtilGetArg(arg, sizeof(arg), p);
		MuxFeed *feed = muxSvrFoundFeed(0, arg, svrConfig->muxSvr);
		if(feed == NULL)
			CONFIG_ERROR("Feed '%s' in stream '%s' is not existed", arg, stream->filename );
		
		ADD_ELEMENT( feed->myStreams, stream);
		stream->feed = feed;
	}
	else if (!av_strcasecmp(cmd, "File") ||!av_strcasecmp(cmd, "ReadOnlyFile"))
	{
		muxUtilGetArg(stream->feed_filename, sizeof(stream->feed_filename), p);
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

