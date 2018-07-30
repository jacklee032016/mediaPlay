
#ifndef	__PLAY_H__
#define	__PLAY_H__

#include "config.h"
#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>

#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/_time.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"

#if CONFIG_AVFILTER
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#endif

#include <SDL.h>
#include <SDL_thread.h>

#include "cmdutils.h"

#include <assert.h>

#define		PLAYER_OPTIONS_DEBUG_SYNC			1


#define MAX_QUEUE_SIZE							(15 * 1024 * 1024)
#define MIN_FRAMES								25
#define EXTERNAL_CLOCK_MIN_FRAMES			2
#define EXTERNAL_CLOCK_MAX_FRAMES			10

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE				512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC		30

/* Step size for volume control in dB */
#define SDL_VOLUME_STEP (0.75)

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN  0.900
#define EXTERNAL_CLOCK_SPEED_MAX  1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB   20

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE					0.01

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

#define CURSOR_HIDE_DELAY						1000000

#define USE_ONEPASS_SUBTITLE_RENDER				1


enum
{
	AV_SYNC_AUDIO_MASTER, /* default choice */
	AV_SYNC_VIDEO_MASTER,
	AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
	AV_SYNC_NONE,				/* no sync, free play */
	
};



typedef struct MyAVPacketList
{
	AVPacket				pkt;
	struct MyAVPacketList	*next;
	int					serial;
} MyAVPacketList;

typedef struct PacketQueue
{
	MyAVPacketList	*first_pkt, *last_pkt;
	int				nb_packets;
	int				size;
	int64_t			duration;
	
	int				abort_request;
	int				serial;
	
	SDL_mutex		*mutex;
	SDL_cond		*cond;
} PacketQueue;

#define VIDEO_PICTURE_QUEUE_SIZE		3
#define SUBPICTURE_QUEUE_SIZE			16
#define SAMPLE_QUEUE_SIZE				9
#define FRAME_QUEUE_SIZE				FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))	/* 16 */

typedef struct AudioParams
{
	int						freq;
	int						channels;
	int64_t					channel_layout;
	
	enum AVSampleFormat		fmt;
	int						frame_size;
	int						bytes_per_sec;
} AudioParams;

typedef struct PlayClock
{
	double	pts;				/* clock base */
	double	pts_drift;		/* clock base minus time at which we updated the clock */
	double	last_updated;
	double	speed;
	int		serial;			/* clock is based on a packet with this serial */
	int		paused;
	int		*queue_serial;	/* pointer to the current packet queue serial, used for obsolete clock detection */
} PlayClock;

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct PlayFrame
{
	AVFrame		*frame;
	AVSubtitle	sub;
	
	int			serial;
	double		pts;			/* presentation timestamp for the frame */
	double		duration;	/* estimated duration of the frame */
	int64_t		pos;		/* byte position of the frame in the input file */
	
	int			width;
	int			height;
	
	int			format;
	AVRational	sar;
	int			uploaded;	/* frame data has been binded in texture */
	int			flip_v;		/* flip vertically */
}PlayFrame;

typedef struct FrameQueue
{
	PlayFrame	queue[FRAME_QUEUE_SIZE];
	
	int		rindex;
	int		windex;

	int		size;
	int		max_size;
	
	int		keep_last;
	int		rindex_shown;	/* show index of read. after read, it is always 1 */

	SDL_mutex	*mutex;
	SDL_cond	*cond;
	
	PacketQueue	*pktq;
} FrameQueue;


typedef struct PlayDecoder
{
	/* keep data of current decoding packet */
	AVPacket			pkt;
	AVPacket			pkt_temp;
	PacketQueue		*queue;
	AVCodecContext	*avctx;
	
	int				pkt_serial;		/* which packet decoder is working on */
	int				finished;			/* the serial of packet which is just finished */
	int				packet_pending;
	SDL_cond		*empty_queue_cond;
	
	int64_t			start_pts;
	AVRational		start_pts_tb;
	int64_t			next_pts;
	AVRational		next_pts_tb;
	
	SDL_Thread		*decoder_tid;
}PlayDecoder;

typedef	enum ShowMode
{
	SHOW_MODE_NONE = -1,
	SHOW_MODE_VIDEO = 0,
	SHOW_MODE_WAVES,
	SHOW_MODE_RDFT,		/* Real Discrete Fourier Transforms in audio display */
	SHOW_MODE_NB
}SHOW_MODE_T;


typedef struct VideoState
{
	SDL_Thread		*read_tid;
	
	AVInputFormat		*iformat;
	AVFormatContext		*ic;
	int					realtime;	/* whether is realtime protocol like RTP/RTSP */


	int		abort_request;
	int		force_refresh;
	int		paused;
	int		last_paused;
	int		queue_attachments_req;
	int		seek_req;
	int		seek_flags;
	int64_t	seek_pos;
	int64_t	seek_rel;
	int		read_pause_return;
	
	PlayClock		audclk;
	PlayClock		vidclk;
	PlayClock		extclk;

	FrameQueue	videoFrameQueue;
	FrameQueue	subtitleFrameQueue;
	FrameQueue	audioFrameQueue;

	PlayDecoder		audioDecoder;
	PlayDecoder		videoDecoder;
	PlayDecoder		subtitleDecoder;

	int			audio_stream;

	int			av_sync_type;

	/*audio stream */
	double		audio_clock;
	int			audio_clock_serial;
	double		audio_diff_cum; /* used for AV difference average computation */
	double		audio_diff_avg_coef;
	double		audio_diff_threshold;
	int			audio_diff_avg_count;
	AVStream	*audio_st;
	
	PacketQueue		audioq;
	int			audio_hw_buf_size;
	uint8_t		*audio_buf;
	uint8_t		*audio_buf1;
	unsigned int	audio_buf_size; /* in bytes */
	unsigned int	audio_buf1_size;
	int			audio_buf_index; /* in bytes */
	int			audio_write_buf_size;
	
	int			audio_volume;
	int			muted;
	struct AudioParams	audio_src;
#if CONFIG_AVFILTER
	struct AudioParams	audio_filter_src;
#endif
	struct AudioParams	audio_tgt;
	struct SwrContext		*swr_ctx;

	/* for video frames */
	int		frame_drops_early;	/* frame's pts is small than current master clock: means decoding slowly. process in video decoding */
	int		frame_drops_late;	

	SHOW_MODE_T	show_mode;

	int16_t sample_array[SAMPLE_ARRAY_SIZE];
	int sample_array_index;
	int last_i_start;
	RDFTContext *rdft;
	int rdft_bits;
	FFTSample *rdft_data;
	int xpos;
	double last_vis_time;
	
	SDL_Texture		*vis_texture;
	SDL_Texture		*sub_texture;
	SDL_Texture		*vid_texture;	/* texture of video */

	/* subtitle */
	int subtitle_stream;
	AVStream *subtitle_st;
	PacketQueue		subtitleq;

	/* video stream */
	double				frame_timer;
	double				frame_last_returned_time;
	double				frame_last_filter_delay;
	int					video_stream;
	AVStream			*video_st;
	PacketQueue			videoq;
	double				max_frame_duration;	/* maximum duration of a frame - above this, we consider the jump a timestamp discontinuity */
	struct SwsContext		*img_convert_ctx;
	struct SwsContext		*sub_convert_ctx;
	int eof;

	char		*filename;
	int		width, height, xleft, ytop;
	int		step;		/* play step by step; toggle pause mode */

#if CONFIG_AVFILTER
	int vfilter_idx;
	AVFilterContext *in_video_filter;   // the first filter in the video chain
	AVFilterContext *out_video_filter;  // the last filter in the video chain
	AVFilterContext *in_audio_filter;   // the first filter in the audio chain
	AVFilterContext *out_audio_filter;  // the last filter in the audio chain
	AVFilterGraph *agraph;              // audio filter graph
#endif

	int last_video_stream, last_audio_stream, last_subtitle_stream;

	SDL_cond *continue_read_thread;
} VideoState;

#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)


#define	PLAY_LOG(...)	\
		av_log(NULL, AV_LOG_INFO, ##__VA_ARGS__)

#define	PLAY_TRACE()		\
		av_log(NULL, AV_LOG_INFO, "%s.%s().line%d\n", __FILE__, __FUNCTION__, __LINE__)
			


int playThreadRead(void *arg);
/* 3 decode threads */
int playThreadAudioDecode(void *arg);
int playThreadVideoDecode(void *arg);
int playThreadSubtitleDecode(void *arg);

int64_t get_valid_channel_layout(int64_t channel_layout, int channels);

void step_to_next_frame(VideoState *is);

void playPlayerDestory(VideoState *is);

void playStreamSeek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes);
int playStreamComponentOpen(VideoState *is, int stream_index);
void playStreamComponentClose(VideoState *is, int stream_index);


int playFrameQueueInit(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last);
void playFrameQueueDestory(FrameQueue *f);
void playFrameQueueSignal(FrameQueue *f);
PlayFrame *playFrameQueuePeek(FrameQueue *f);
PlayFrame *playFrameQueuePeekNext(FrameQueue *f);
PlayFrame *playFrameQueuePeekLast(FrameQueue *f);
PlayFrame *playFrameQueuePeekWritable(FrameQueue *f);
PlayFrame *playFrameQueuePeekReadable(FrameQueue *f);
void playFrameQueuePush(FrameQueue *f);
void playFrameQueueNext(FrameQueue *f);

int playFrameQueueNbRemaining(FrameQueue *f);
int64_t playFrameQueueLastPos(FrameQueue *f);


int playPacketQueuePut(PacketQueue *q, AVPacket *pkt);
int playPacketQueuePutNullPacket(PacketQueue *q, int stream_index);
int playPacketQueueGet(PacketQueue *q, AVPacket *pkt, int block, int *serial);

int playPacketQueueInit(PacketQueue *q);
void playPacketQueueDestroy(PacketQueue *q);

void playPacketQueueAbort(PacketQueue *q);
void playPacketQueueStart(PacketQueue *q);
void playPacketQueueFlush(PacketQueue *q);


double playClockGet(PlayClock *c);
void playClockSetAt(PlayClock *c, double pts, int serial, double time);
void playClockSet(PlayClock *c, double pts, int serial);
void playClockSpeedSet(PlayClock *c, double speed);
void playClockInit(PlayClock *c, int *queue_serial);
void playClockSyncToSlave(PlayClock *c, PlayClock *slave);
double playClockGetMaster(VideoState *is);

int get_master_sync_type(VideoState *is);

void do_exit(VideoState *is);


#if CONFIG_AVFILTER
int configure_filtergraph(AVFilterGraph *graph, const char *filtergraph,
                                 AVFilterContext *source_ctx, AVFilterContext *sink_ctx);
int configure_video_filters(AVFilterGraph *graph, VideoState *is, const char *vfilters, AVFrame *frame);
int configure_audio_filters(VideoState *is, const char *afilters, int force_output_format);
int opt_add_vfilter(void *optctx, const char *opt, const char *arg);

#endif  /* CONFIG_AVFILTER */


void playDecoderInit(PlayDecoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond);
int playDecoderStart(PlayDecoder *d, int (*fn)(void *), void *arg);

int playDecoderDecodeFrame(PlayDecoder *d, AVFrame *frame, AVSubtitle *sub);
void playDecoderDestroy(PlayDecoder *d);
void playDecoderAbort(PlayDecoder *d, FrameQueue *fq);



void play_opt_input_file(void *optctx, const char *filename);
void play_parse_options(void *optctx, int argc, char **argv, const OptionDef *options,
                   void (*parse_arg_function)(void *, const char*));


void set_default_window_size(int width, int height, AVRational sar);

void playVideoRefresh(void *opaque, double *remaining_time);
void stream_toggle_pause(VideoState *is);

void playSdlVideoDisplay(VideoState *is);
void playSdlDestory(void);

void playSdlEventLoop(VideoState *cur_stream);


int playSdlAudioOpen(void *opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct AudioParams *audio_hw_params);

extern	AVPacket flush_pkt;

extern	int autorotate;

extern	int decoder_reorder_pts;

extern	AVInputFormat *file_iformat;

extern	const char *input_filename;

extern	const char *window_title;
extern	int default_width;
extern	int default_height;

extern	int screen_width;
extern	int screen_height;

extern	int av_sync_type;
extern	int64_t start_time;
extern	int64_t duration;

extern	const char *audio_codec_name;
extern	const char *subtitle_codec_name;
extern	const char *video_codec_name;

extern	SHOW_MODE_T show_mode;
extern	int is_full_screen;

extern	int audio_disable;
extern	int video_disable;
extern	int subtitle_disable;

extern	const char* wanted_stream_spec[AVMEDIA_TYPE_NB];
extern	int seek_by_bytes;
extern	int display_disable;
extern	int borderless;
extern	int startup_volume;
extern	int show_status;

extern	int fast;
extern	int genpts;
extern	int lowres;


extern	int autoexit;
extern	int exit_on_keydown;
extern	int exit_on_mousedown;
extern	int loop;
extern	int framedrop;
extern	int infinite_buffer;


extern	SDL_Window *window;


extern	unsigned sws_flags;

extern	double rdftspeed;
extern	int64_t cursor_last_shown;
extern	int cursor_hidden;
#if CONFIG_AVFILTER
extern	const char **vfilters_list;
extern	int nb_vfilters;
extern	char *afilters;
#endif

extern	int64_t audio_callback_time;

extern	const OptionDef options[]; // from playOpts.c

#endif


