
#include "play.h"

static int opt_frame_size(void *optctx, const char *opt, const char *arg)
{
	av_log(NULL, AV_LOG_WARNING, "Option -s is deprecated, use -video_size.\n");
	return opt_default(NULL, "video_size", arg);
}

static int opt_width(void *optctx, const char *opt, const char *arg)
{
	screen_width = parse_number_or_die(opt, arg, OPT_INT64, 1, INT_MAX);
	return 0;
}

static int opt_height(void *optctx, const char *opt, const char *arg)
{
	screen_height = parse_number_or_die(opt, arg, OPT_INT64, 1, INT_MAX);
	return 0;
}

static int opt_format(void *optctx, const char *opt, const char *arg)
{
	printf("input format %s\n", arg);
	file_iformat = av_find_input_format(arg);
	if (!file_iformat)
	{
		av_log(NULL, AV_LOG_FATAL, "Unknown input format: %s\n", arg);
		return AVERROR(EINVAL);
	}
	return 0;
}

static int opt_frame_pix_fmt(void *optctx, const char *opt, const char *arg)
{
	av_log(NULL, AV_LOG_WARNING, "Option -pix_fmt is deprecated, use -pixel_format.\n");
	return opt_default(NULL, "pixel_format", arg);
}

static int opt_sync(void *optctx, const char *opt, const char *arg)
{
	if (!strcmp(arg, "audio"))
		av_sync_type = AV_SYNC_AUDIO_MASTER;
	else if (!strcmp(arg, "video"))
		av_sync_type = AV_SYNC_VIDEO_MASTER;
	else if (!strcmp(arg, "ext"))
		av_sync_type = AV_SYNC_EXTERNAL_CLOCK;
	else
	{
		av_log(NULL, AV_LOG_ERROR, "Unknown value for %s: %s\n", opt, arg);
		exit(1);
	}
	return 0;
}

static int opt_seek(void *optctx, const char *opt, const char *arg)
{
	start_time = parse_time_or_die(opt, arg, 1);
	return 0;
}

static int opt_duration(void *optctx, const char *opt, const char *arg)
{
	duration = parse_time_or_die(opt, arg, 1);
	return 0;
}

static int opt_show_mode(void *optctx, const char *opt, const char *arg)
{
	show_mode = !strcmp(arg, "video") ? SHOW_MODE_VIDEO :
		!strcmp(arg, "waves") ? SHOW_MODE_WAVES :
		!strcmp(arg, "rdft" ) ? SHOW_MODE_RDFT  :
		parse_number_or_die(opt, arg, OPT_INT, 0, SHOW_MODE_NB-1);
	return 0;
}

void play_opt_input_file(void *optctx, const char *filename)
{
	if (input_filename)
	{
		av_log(NULL, AV_LOG_FATAL, "Argument '%s' provided as input filename, but '%s' was already specified.\n", filename, input_filename);
		exit(1);
	}
	
	if (!strcmp(filename, "-"))
		filename = "pipe:";
	
	input_filename = filename;
}

static int opt_codec(void *optctx, const char *opt, const char *arg)
{
	const char *spec = strchr(opt, ':');
	if (!spec)
	{
		av_log(NULL, AV_LOG_ERROR, "No media specifier was specified in '%s' in option '%s'\n", arg, opt);
		return AVERROR(EINVAL);
	}
	
	spec++;
	switch (spec[0])
	{
		case 'a' :    audio_codec_name = arg; break;
		case 's' : subtitle_codec_name = arg; break;
		case 'v' :    video_codec_name = arg; break;
		default:
			av_log(NULL, AV_LOG_ERROR, "Invalid media specifier '%s' in option '%s'\n", spec, opt);
			return AVERROR(EINVAL);
	}
	return 0;
}

static int dummy;

const OptionDef options[] = {
#include "cmdutils_common_opts.h"
    { "x", HAS_ARG, { .func_arg = opt_width }, "force displayed width", "width" },
    { "y", HAS_ARG, { .func_arg = opt_height }, "force displayed height", "height" },
    { "s", HAS_ARG | OPT_VIDEO, { .func_arg = opt_frame_size }, "set frame size (WxH or abbreviation)", "size" },
    { "fs", OPT_BOOL, { &is_full_screen }, "force full screen" },
    { "an", OPT_BOOL, { &audio_disable }, "disable audio" },
    { "vn", OPT_BOOL, { &video_disable }, "disable video" },
    { "sn", OPT_BOOL, { &subtitle_disable }, "disable subtitling" },
    { "ast", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_AUDIO] }, "select desired audio stream", "stream_specifier" },
    { "vst", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_VIDEO] }, "select desired video stream", "stream_specifier" },
    { "sst", OPT_STRING | HAS_ARG | OPT_EXPERT, { &wanted_stream_spec[AVMEDIA_TYPE_SUBTITLE] }, "select desired subtitle stream", "stream_specifier" },
    { "ss", HAS_ARG, { .func_arg = opt_seek }, "seek to a given position in seconds", "pos" },
    { "t", HAS_ARG, { .func_arg = opt_duration }, "play  \"duration\" seconds of audio/video", "duration" },
    { "bytes", OPT_INT | HAS_ARG, { &seek_by_bytes }, "seek by bytes 0=off 1=on -1=auto", "val" },
    { "nodisp", OPT_BOOL, { &display_disable }, "disable graphical display" },
    { "noborder", OPT_BOOL, { &borderless }, "borderless window" },
    { "volume", OPT_INT | HAS_ARG, { &startup_volume}, "set startup volume 0=min 100=max", "volume" },
    { "f", HAS_ARG, { .func_arg = opt_format }, "force format", "fmt" },
    { "pix_fmt", HAS_ARG | OPT_EXPERT | OPT_VIDEO, { .func_arg = opt_frame_pix_fmt }, "set pixel format", "format" },
    { "stats", OPT_BOOL | OPT_EXPERT, { &show_status }, "show status", "" },
    { "fast", OPT_BOOL | OPT_EXPERT, { &fast }, "non spec compliant optimizations", "" },
    { "genpts", OPT_BOOL | OPT_EXPERT, { &genpts }, "generate pts", "" },
    { "drp", OPT_INT | HAS_ARG | OPT_EXPERT, { &decoder_reorder_pts }, "let decoder reorder pts 0=off 1=on -1=auto", ""},
    { "lowres", OPT_INT | HAS_ARG | OPT_EXPERT, { &lowres }, "", "" },
    { "sync", HAS_ARG | OPT_EXPERT, { .func_arg = opt_sync }, "set audio-video sync. type (type=audio/video/ext)", "type" },
    { "autoexit", OPT_BOOL | OPT_EXPERT, { &autoexit }, "exit at the end", "" },
    { "exitonkeydown", OPT_BOOL | OPT_EXPERT, { &exit_on_keydown }, "exit on key down", "" },
    { "exitonmousedown", OPT_BOOL | OPT_EXPERT, { &exit_on_mousedown }, "exit on mouse down", "" },
    { "loop", OPT_INT | HAS_ARG | OPT_EXPERT, { &loop }, "set number of times the playback shall be looped", "loop count" },
    { "framedrop", OPT_BOOL | OPT_EXPERT, { &framedrop }, "drop frames when cpu is too slow", "" },
    { "infbuf", OPT_BOOL | OPT_EXPERT, { &infinite_buffer }, "don't limit the input buffer size (useful with realtime streams)", "" },
    { "window_title", OPT_STRING | HAS_ARG, { &window_title }, "set window title", "window title" },
#if CONFIG_AVFILTER
    { "vf", OPT_EXPERT | HAS_ARG, { .func_arg = opt_add_vfilter }, "set video filters", "filter_graph" },
    { "af", OPT_STRING | HAS_ARG, { &afilters }, "set audio filters", "filter_graph" },
#endif
    { "rdftspeed", OPT_INT | HAS_ARG| OPT_AUDIO | OPT_EXPERT, { &rdftspeed }, "rdft speed", "msecs" },
    { "showmode", HAS_ARG, { .func_arg = opt_show_mode}, "select show mode (0 = video, 1 = waves, 2 = RDFT)", "mode" },
    { "default", HAS_ARG | OPT_AUDIO | OPT_VIDEO | OPT_EXPERT, { .func_arg = opt_default }, "generic catch all option", "" },
    { "i", OPT_BOOL, { &dummy}, "read specified file", "input_file"},
    { "codec", HAS_ARG, { .func_arg = opt_codec}, "force decoder", "decoder_name" },
    { "acodec", HAS_ARG | OPT_STRING | OPT_EXPERT, {    &audio_codec_name }, "force audio decoder",    "decoder_name" },
    { "scodec", HAS_ARG | OPT_STRING | OPT_EXPERT, { &subtitle_codec_name }, "force subtitle decoder", "decoder_name" },
    { "vcodec", HAS_ARG | OPT_STRING | OPT_EXPERT, {    &video_codec_name }, "force video decoder",    "decoder_name" },
    { "autorotate", OPT_BOOL, { &autorotate }, "automatically rotate video", "" },
    { NULL, },
};

void show_help_default(const char *opt, const char *arg)
{
	av_log_set_callback(log_callback_help);
	show_help_options(options, "Main options:", 0, OPT_EXPERT, 0);
	show_help_options(options, "Advanced options:", OPT_EXPERT, 0, 0);
	printf("\n");
	show_help_children(avcodec_get_class(), AV_OPT_FLAG_DECODING_PARAM);
	show_help_children(avformat_get_class(), AV_OPT_FLAG_DECODING_PARAM);
#if !CONFIG_AVFILTER
	show_help_children(sws_get_class(), AV_OPT_FLAG_ENCODING_PARAM);
#else
	show_help_children(avfilter_get_class(), AV_OPT_FLAG_FILTERING_PARAM);
#endif
	printf("\nWhile playing:\n"
		"q, ESC              quit\n"
		"f                   toggle full screen\n"
		"p, SPC              pause\n"
		"m                   toggle mute\n"
		"9, 0                decrease and increase volume respectively\n"
		"/, *                decrease and increase volume respectively\n"
		"a                   cycle audio channel in the current program\n"
		"v                   cycle video channel\n"
		"t                   cycle subtitle channel in the current program\n"
		"c                   cycle program\n"
		"w                   cycle video filters or show modes\n"
		"s                   activate frame-step mode\n"
		"left/right          seek backward/forward 10 seconds\n"
		"down/up             seek backward/forward 1 minute\n"
		"page down/page up   seek backward/forward 10 minutes\n"
		"right mouse click   seek to percentage in file corresponding to fraction of width\n"
		"left double-click   toggle full screen\n"
		);
}

void play_parse_options(void *optctx, int argc, char **argv, const OptionDef *options,  void (*parse_arg_function)(void *, const char*))
{
	const char *opt;
	int optindex, handleoptions = 1, ret;

	/* perform system-dependent conversions for arguments list */
	//prepare_app_arguments(&argc, &argv);

	/* parse options */
	optindex = 1;
	while (optindex < argc)
	{
		opt = argv[optindex++];

		if (handleoptions && opt[0] == '-' && opt[1] != '\0')
		{
			if (opt[1] == '-' && opt[2] == '\0')
			{
				handleoptions = 0;
				continue;
			}
			opt++;

			if ((ret = parse_option(optctx, opt, argv[optindex], options)) < 0)
				exit_program(1);
			optindex += ret;
		}
		else
		{
			if (parse_arg_function)
				parse_arg_function(optctx, opt);
		}
	}
}

