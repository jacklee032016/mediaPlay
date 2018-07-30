/*
 */

#include "play.h"

const char program_name[] = "ffplay";
const int program_birth_year = 2003;

/* options specified by the user */
AVInputFormat *file_iformat;

const char *input_filename;

const char *window_title;
int default_width  = 640;
int default_height = 480;

int screen_width  = 0;
int screen_height = 0;

int audio_disable;
int video_disable;
int subtitle_disable;


const char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
int seek_by_bytes = -1;
int display_disable;
int borderless;
int startup_volume = 100;
int show_status = 1;

#if 1
int av_sync_type = AV_SYNC_AUDIO_MASTER;
#else
int av_sync_type = AV_SYNC_VIDEO_MASTER;
#endif

int64_t start_time = AV_NOPTS_VALUE;
int64_t duration = AV_NOPTS_VALUE;

int fast = 0;
int genpts = 0;
int lowres = 0;

int decoder_reorder_pts = -1;

int autoexit;
int exit_on_keydown;
int exit_on_mousedown;
int loop = 1;
int framedrop = -1;
int infinite_buffer = -1;

SHOW_MODE_T show_mode = SHOW_MODE_NONE;

const char *audio_codec_name;
const char *subtitle_codec_name;
const char *video_codec_name;


double rdftspeed = 0.02;
int64_t cursor_last_shown;
int cursor_hidden = 0;
#if CONFIG_AVFILTER
const char **vfilters_list = NULL;
int nb_vfilters = 0;
char *afilters = NULL;
#endif
int autorotate = 1;

/* current context */
int is_full_screen;
int64_t audio_callback_time;	/* start time of current callback */

AVPacket flush_pkt;


unsigned sws_flags = SWS_BICUBIC;


static void sigterm_handler(int sig)
{
	exit(123);
}


/* pause or resume the video */
void stream_toggle_pause(VideoState *is)
{
	if (is->paused)
	{
		is->frame_timer += av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
		if (is->read_pause_return != AVERROR(ENOSYS))
		{
			is->vidclk.paused = 0;
		}

		playClockSet(&is->vidclk, playClockGet(&is->vidclk), is->vidclk.serial);
	}
	
	playClockSet(&is->extclk, playClockGet(&is->extclk), is->extclk.serial);
	is->paused = is->audclk.paused = is->vidclk.paused = is->extclk.paused = !is->paused;
}

static int lockmgr(void **mtx, enum AVLockOp op)
{
	switch(op)
	{
		case AV_LOCK_CREATE:
			*mtx = SDL_CreateMutex();
			if(!*mtx)
			{
				av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
				return 1;
			}
			return 0;
		case AV_LOCK_OBTAIN:
			return !!SDL_LockMutex(*mtx);
			
		case AV_LOCK_RELEASE:
			return !!SDL_UnlockMutex(*mtx);
		
		case AV_LOCK_DESTROY:
			SDL_DestroyMutex(*mtx);
			return 0;
   }
   return 1;
}



void playPlayerDestory(VideoState *is)
{
	/* XXX: use a special url_shutdown call to abort parse cleanly */
	is->abort_request = 1;
	SDL_WaitThread(is->read_tid, NULL);

	/* close each stream */
	if (is->audio_stream >= 0)
		playStreamComponentClose(is, is->audio_stream);
	if (is->video_stream >= 0)
		playStreamComponentClose(is, is->video_stream);
	if (is->subtitle_stream >= 0)
		playStreamComponentClose(is, is->subtitle_stream);

	avformat_close_input(&is->ic);

	playPacketQueueDestroy(&is->videoq);
	playPacketQueueDestroy(&is->audioq);
	playPacketQueueDestroy(&is->subtitleq);

	/* free all pictures */
	playFrameQueueDestory(&is->videoFrameQueue);
	playFrameQueueDestory(&is->audioFrameQueue);
	playFrameQueueDestory(&is->subtitleFrameQueue);
	
	SDL_DestroyCond(is->continue_read_thread);
	sws_freeContext(is->img_convert_ctx);
	sws_freeContext(is->sub_convert_ctx);
	
	av_free(is->filename);
	
	if (is->vis_texture)
		SDL_DestroyTexture(is->vis_texture);
	if (is->vid_texture)
		SDL_DestroyTexture(is->vid_texture);
	if (is->sub_texture)
		SDL_DestroyTexture(is->sub_texture);
	
	av_free(is);
}


static VideoState *_playPlayerInit(const char *filename, AVInputFormat *iformat)
{
	VideoState *is;

	is = av_mallocz(sizeof(VideoState));
	if (!is)
		return NULL;

	is->filename = av_strdup(filename);
	if (!is->filename)
		goto fail;
	
	is->iformat = iformat;
	is->ytop    = 0;
	is->xleft   = 0;

	/* start video display */
	if (playFrameQueueInit(&is->videoFrameQueue, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
		goto fail;
	if (playFrameQueueInit(&is->subtitleFrameQueue, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0)
		goto fail;
	if (playFrameQueueInit(&is->audioFrameQueue, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0)
		goto fail;

	if (playPacketQueueInit(&is->videoq) < 0 ||
		playPacketQueueInit(&is->audioq) < 0 ||
		playPacketQueueInit(&is->subtitleq) < 0)
			goto fail;

	if (!(is->continue_read_thread = SDL_CreateCond()))
	{
		av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
		goto fail;
	}

	playClockInit(&is->vidclk, &is->videoq.serial);
	playClockInit(&is->audclk, &is->audioq.serial);
	playClockInit(&is->extclk, &is->extclk.serial);
	is->audio_clock_serial = -1;

	if (startup_volume < 0)
		av_log(NULL, AV_LOG_WARNING, "-volume=%d < 0, setting to 0\n", startup_volume);
	if (startup_volume > 100)
		av_log(NULL, AV_LOG_WARNING, "-volume=%d > 100, setting to 100\n", startup_volume);

	startup_volume = av_clip(startup_volume, 0, 100);
	startup_volume = av_clip(SDL_MIX_MAXVOLUME * startup_volume / 100, 0, SDL_MIX_MAXVOLUME);
	is->audio_volume = startup_volume;
	is->muted = 0;
	is->av_sync_type = av_sync_type;
	
	is->read_tid     = SDL_CreateThread(playThreadRead, "playerReadingThread", is);
	if (!is->read_tid)
	{
		av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %s\n", SDL_GetError());

fail:
		playPlayerDestory(is);
		return NULL;
	}
	return is;
}


/* Called from the main */
int main(int argc, char **argv)
{
	int flags;
	VideoState *is;

	init_dynload();

	av_log_set_flags(AV_LOG_SKIP_REPEATED);
	parse_loglevel(argc, argv, options);

	/* register all codecs, demux and protocols */
#if CONFIG_AVDEVICE
	avdevice_register_all();
#endif

#if CONFIG_AVFILTER
	avfilter_register_all();
#endif

	av_register_all();
	avformat_network_init();

	init_opts();

	signal(SIGINT , sigterm_handler); /* Interrupt (ANSI).    */
	signal(SIGTERM, sigterm_handler); /* Termination (ANSI).  */

	show_banner(argc, argv, options);

	play_parse_options(NULL, argc, argv, options, play_opt_input_file);

	if (!input_filename)
	{
		av_log(NULL, AV_LOG_FATAL, "An input file must be specified\n");
		av_log(NULL, AV_LOG_FATAL,
		"Use -h to get full help or, even better, run 'man %s'\n", program_name);
		exit(1);
	}

	if (display_disable)
	{
		video_disable = 1;
	}

	flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
	if (audio_disable)
		flags &= ~SDL_INIT_AUDIO;
	else
	{
		/* Try to work around an occasional ALSA buffer underflow issue when the
		* period size is NPOT due to ALSA resampling by forcing the buffer size. */
		if (!SDL_getenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE"))
			SDL_setenv("SDL_AUDIO_ALSA_SET_BUFFER_SIZE","1", 1);
	}
	
	if (display_disable)
		flags &= ~SDL_INIT_VIDEO;

	if (SDL_Init (flags))
	{
		av_log(NULL, AV_LOG_FATAL, "Could not initialize SDL - %s\n", SDL_GetError());
		av_log(NULL, AV_LOG_FATAL, "(Did you set the DISPLAY variable?)\n");
		exit(1);
	}

	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

	/* ??? */
	if (av_lockmgr_register(lockmgr))
	{
		av_log(NULL, AV_LOG_FATAL, "Could not initialize lock manager!\n");
		do_exit(NULL);
	}

	av_init_packet(&flush_pkt);
	flush_pkt.data = (uint8_t *)&flush_pkt;

	is = _playPlayerInit(input_filename, file_iformat);
	if (!is)
	{
		av_log(NULL, AV_LOG_FATAL, "Failed to initialize VideoState!\n");
		do_exit(NULL);
	}

	playSdlEventLoop(is);

	/* never returns */

	return 0;
}

