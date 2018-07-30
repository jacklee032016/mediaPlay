
#ifndef	__TUTORIAL_H__
#define	__TUTORIAL_H__

#define	FFMPEG_NEW_API		1
#define	WITH_SDL_2				1

#define	ARCH_X86				1

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#include <SDL.h>
#include <SDL_thread.h>


#include <stdio.h>
#include <assert.h>

#include "libavutil/imgutils.h"
#include "libavutil/avstring.h"

#if 0
/* Jack, Aug.7, 2017 */
#include "libavutil/time.h"
#else
#include "libavutil/_time.h"
#endif


/* buffer size of one channel in SDL audio, send to SDL audio library */
#define SDL_AUDIO_BUFFER_SIZE		1024
#define MAX_AUDIO_FRAME_SIZE		192000


#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)


#define FF_REFRESH_EVENT				(SDL_USEREVENT)
#define FF_QUIT_EVENT					(SDL_USEREVENT + 1)

#define SIMPLAY_VIDEO_PICTURE_QUEUE_SIZE		1



#define AV_SYNC_THRESHOLD				0.01
#define AV_NOSYNC_THRESHOLD			10.0


#define	SIM_PLAY_OPTION_DEBUG_AUDIO		0

#define	SIM_PLAY_OPTION_DEBUG_VIDEO		0


typedef	enum
{
	SPLAY_TYPE_1 = 0,
	SPLAY_TYPE_2,
	SPLAY_TYPE_3,
	SPLAY_TYPE_4,
	SPLAY_TYPE_5,
	SPLAY_TYPE_6,
	SPLAY_TYPE_7
}SPLAY_TYPE_T;


enum
{
	AV_SYNC_AUDIO_MASTER = 0,
	AV_SYNC_VIDEO_MASTER,
	AV_SYNC_EXTERNAL_MASTER,
};



struct SimPlay;

typedef struct PacketQueue
{
	char			name[256];
	
	AVPacketList	*first_pkt, *last_pkt;
	int			nb_packets;
	int			size;
	
	SDL_mutex	*mutex;
	SDL_cond	*cond;

	struct SimPlay	*sPlay;
} PacketQueue;


typedef	struct
{
#if FFMPEG_NEW_API
	SDL_Window		*screen;
	SDL_Renderer		*renderer;
	SDL_Texture		*texture;
#else
	SDL_Surface     	*screen;
#endif

	SDL_mutex		*screen_mutex;
}SPlayWindow;



typedef struct VideoPicture
{
#if	FFMPEG_NEW_API
	SDL_Texture		*bmp;

	uint8_t			*data[4];
	int				linesizes[4];

#else
	SDL_Overlay		*bmp;
#endif

	int				width, height; /* source height & width */
	int				allocated;
	double			pts;				/* PTS for this frame/picture */
} VideoPicture;


typedef struct SimPlay
{
	SPLAY_TYPE_T			type;
	char						name[256];

	int64_t					startTime;

	SPlayWindow				window;
	
	AVFormatContext		*pFormatCtx;
	int					videoStream, audioStream;
	
	double				audio_clock;
	AVStream			*audio_st;
	AVCodecContext		*audio_ctx;
	PacketQueue			audioq;

	struct SwrContext		*auConvertCtx;

	
//	uint8_t				audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	uint8_t				audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
	unsigned int			audio_buf_size;
	unsigned int			audio_buf_index;
	
	AVFrame				audio_frame;
	AVPacket				audio_pkt;
	uint8_t				*audio_pkt_data;
	int					audio_pkt_size;
	
	AVStream			*video_st;
	AVCodecContext		*video_ctx;
	PacketQueue			videoq;
	struct SwsContext		*sws_ctx;

	int					videoPacketCount;	/* count of video packet, used to exit when playing ends */
	int					videoDecodedCount;
	int					videoRendededCount;
	int					videoDropCount;
	int					isReadEnd;

	/* for SimPlay05: video synchron */
	/* in seconds, float type */
	double				playStartRime;
	double				firstPts;
	
	double				frame_timer;
	double				frame_last_pts;
	double				frame_last_delay;
	double				video_clock; 	/* pts of last decoded (current decoding) frame / predicted pts of next decoded frame */

	/* SimPlayer06 */
	double				video_current_pts; ///<current displayed pts (different from video_clock if frame fifos are used)
	int64_t				video_current_pts_time;  ///<time (av_gettime) at which we updated video_current_pts - used to have running video pts

	int					av_sync_type;
	double				external_clock; /* external clock base */
	int64_t				external_clock_time;

	int					audio_hw_buf_size;

	double				audio_diff_cum; /* used for AV difference average computation */
	double				audio_diff_avg_coef;
	double				audio_diff_threshold;
	int					audio_diff_avg_count;



	VideoPicture			pictq[SIMPLAY_VIDEO_PICTURE_QUEUE_SIZE];
	int					pictq_size, pictq_rindex, pictq_windex;
	
	SDL_mutex			*pictq_mutex;
	SDL_cond			*pictq_cond;

	SDL_Thread			*readThread;
	SDL_Thread			*videoThread;

	char					filename[1024];
	int					quit;
}SimPlay;


#define	IS_KEY_FRAME(packet)	\
			(((packet)->flags & AV_PKT_FLAG_KEY) != 0) 

			

#define	strace()		fprintf(stderr, "%s.%s.%d\n", __FILE__, __FUNCTION__, __LINE__)


void splayQueueInit(PacketQueue *q, SimPlay *sPlay, const char *name);

int splayQueuePut(PacketQueue *q, AVPacket *pkt);
int splayQueueGet(PacketQueue *q, AVPacket *pkt, int block);
void sPlayQueueFlush(PacketQueue *q);

int splayQueuePutPicture(SimPlay *is, AVFrame *pFrame, double pts);

void schedule_refresh(SimPlay *is, int delayMs);

int splayReadThread(void *arg);
int	splayInit(SimPlay *is);

void sPlayExit(SimPlay *sPlay);


int splayVideoDecodThread(void *arg);


int	splayWindowCreate(SimPlay *splay);
void splayWindowDisplay(SimPlay *is);


int splayAudioDecodeFrame(SimPlay *is, uint8_t *audio_buf, int buf_size, double *pts_ptr);


/* following simPlayerXXX functions are implemented by every Player */
double simPlayerSynchronizeVideo(SimPlay *is, AVFrame *src_frame, double pts);


#endif

