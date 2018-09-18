/* 
* This library is used to encasulate and hide all of ffmpeg
*/
#ifndef	__LIB_MEDIA_H__
#define	__LIB_MEDIA_H__

#include "libCmn.h"
#include "cmnList.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>



#define	MUX_WITH_AVI_H264_ANNEXB		1	/* AVI container save h264 video in annex B format; used by RECORDER and SERVER (only HTTP?) */

#define	MUX_WITH_FLV_AAC_ADTS2ASC		1	/* FLV container save AAC audio ADTS_TO_ASC format; used by RECORDER and SERVER (only HTTP?) */


#define	MUX_MAX_MEDIA_STREAMS		10	/* number of captured media streams */

#define	STREAM_FRAME_RATE			25

#if ARCH_X86
#define	MUX_PATH_OF_SDCARD					"/media/sf_mux/m"
#define	MUX_PATH_OF_USBDISK					"/media/sf_mux/usb"
#else
#define	MUX_PATH_OF_SDCARD					"/media/sd"
#define	MUX_PATH_OF_USBDISK					"/media/usb"
#endif


typedef	enum
{
	OUT_VIDEO_SIZE_CIF = 0,
	OUT_VIDEO_SIZE_D1,
	OUT_VIDEO_SIZE_720P,  /*HI_UNF_VCODEC_CAP_LEVEL_720P */
	OUT_VIDEO_SIZE_FULLHD,
	OUT_VIDEO_SIZE_4K,
	OUT_VIDEO_SIZE_UNKNOWN,
}OUT_VIDEO_SIZE;


typedef	enum
{
	OUT_VIDEO_FORMAT_H264_BASELINE = 0,
	OUT_VIDEO_FORMAT_H264_MAIN,
	OUT_VIDEO_FORMAT_H264_EXTENDED,
	OUT_VIDEO_FORMAT_H264_HIGH,
	OUT_VIDEO_FORMAT_HEVC,
	OUT_VIDEO_FORMAT_UNKNOWN,
}OUT_VIDEO_FORMAT;

typedef	enum
{
	MUX_SYNC_MASTER_NONE =0, 	/* no sync (master) between A/V, play freely */
	MUX_SYNC_MASTER_AUDIO,		/* audio plays as reference clock, dafault for many players */
	MUX_SYNC_MASTER_VIDEO,
	MUX_SYNC_MASTER_SYSTEM,
	MUX_SYNC_MASTER_UNKNOWN
}MUX_SYNC_MASTER;

/* events and states of media capturer, and used by media consumer */

typedef	enum
{
	MUX_CAPTURE_STATE_INITTED = 0,
	MUX_CAPTURE_STATE_RUNNING,
	MUX_CAPTURE_STATE_PAUSED,
	MUX_CAPTURE_STATE_STOPPED,
	MUX_CAPTURE_STATE_UNKNOWN,
}MUX_CAPTURE_STATE;

typedef	enum
{
	MUX_CAPTURE_EVENT_START = 0,
	MUX_CAPTURE_EVENT_STOP,
	MUX_CAPTURE_EVENT_PAUSE,
	MUX_CAPTURE_EVENT_RESUME,
	MUX_CAPTURE_EVENT_UNKNOWN
}MUX_CAPTURE_EVENT;

typedef	struct
{
	char					author[512];
	char					title[512];
	char					copyright[512];
	char					comment[512];
}stream_descript_t;

/* data transfered between Capture and Consumer */
typedef	struct
{
	unsigned char			*data;
	int					size;

	MUX_MEDIA_TYPE		streamIndex;				
	int64_t				pts;
	int64_t				dts;
}MuxMediaPacket;


/* 
* Describe the A/V media which capture module will create
* MainProgram parses configuration file, and create mediaConfig, then Player use it to setup hardware onboard;
* libMedia use mediaConfig to create MediaDescriber;
* Recorder and CaptureFeed use mediaConfig to create MediaConsumer which receive raw data and timestamp from caputuring interface
*/
typedef	struct _MUX_MEDIA_CONFIG /* decribe media (A/V) output by PLAYER(virtual window/track) or TX */
{
	MUX_BOARD_TYPE			boardType;
//	log_level_t				logLevel;

	/* configuration items for video of recorder */
	int						videoType; /* HI_UNF_VCODEC_TYPE_E */
	int						h264Profile;	/* when VideoType is H264, used only in config hw */
	int						videoCapLevel;  /* HI_UNF_VCODEC_CAP_LEVEL_E, 1080, etc. */
	int						videoBitrate;	/* in kbps */
	int						gop;
	
	/* width and height are details of cap level */
	int						width;
	int						height;
	
	int						aspectRatioWindow;
	int						strmBufSize;		/* only used in PLAYER */
	int						targetBitRate;	/* only used in PLAYER */
	
	int						inputFrameRate;		/* capture */
	int						outputFrameRate;	/* video fps of vapture */


	/* configuration items of audio of recorder, eg. virtual track */
	int						audioType; /* output audio type, only AAC is supported by HW, HA_CODEC_ID_E */
	int						audioSampleRate;
	int						audioFormat;  /*32 bit or 16 bit per sample */
	int						audioChannels;	/* 2 or 1 */

	int						avSyncType;
	
	char						usbHome[CMN_NAME_LENGTH];
	char						sdHome[CMN_NAME_LENGTH];

	MEDIA_DEVICE_T			storeType;
	int64_t					recording_time;

	void						*priv;	/* MuxMain */
}MuxMediaConfig;

/**
 * This holds the stream parameters and used to create AVStream.
 * Conceal all ffmpeg and refers in multiple program, such as Player, Server and Recorder.
 * The fields of this struct have the same semantics as the fields of an AVStream.
 */
typedef struct MuxMediaDescriber
{
	char					name[CMN_NAME_LENGTH];
	
	char					type[CMN_NAME_LENGTH];
	char					codec[CMN_NAME_LENGTH];

	int					firstPts;

	int64_t				nextPts;

	int64_t				count;	/* byte count of output */

	
	int					index; /* stream index*/
	int					id;

	int					bandwidth;		/* bps */
	double				fps;				/* frame per second */

	/* number of AvPacket for this stream, read from metadata of this stream.
	Used to calculate the delay of FileFeedThread */
	int					packetCount;
#if 0
	AVCodecParameters	*codecpar;
	AVRational			time_base;
	AVRational			sample_aspect_ratio;
#else
	void					*codecPar;

	int					timeBaseNumerator;
	int					timeBaseDenominator;
	
	int					sampleAspectRatioNumerator;
	int					sampleAspectRatioDenominator;
#endif

	int					pts_wrap_bits;
	
	char					*recommended_encoder_configuration;
}MuxMediaDescriber;

struct _MuxMediaConsumer;
struct MuxMediaCapture;

typedef	struct _MuxMediaConsumer
{
	char name[CMN_NAME_LENGTH];
	struct	MuxMediaCapture		*mediaCapture;
	
	/* 
	* return: sucess: > 0, size of data; FIFO full: 0; error : < 0;
	* 2 type of consumers: recorder: NO fifo full; server: all cases maybe happens
	*/
	int	(*receiveVideo)(struct _MuxMediaConsumer *, MUX_MEDIA_TYPE type, MuxMediaPacket *mediaPacket);

	/* receive notify from capturer. return: < 0, fail; =0, not used; > 1, received correctly */
	int	(*receiveNotify)(struct _MuxMediaConsumer *, MUX_CAPTURE_EVENT , void *data);

	void		*avContext;	/* AvFormatContext */

	void		*priv;			/* Feed or Recorder */
	struct _MuxMediaConsumer		*next;	
}MuxMediaConsumer;

struct _MuxMain;

struct MuxMediaCapture;

typedef	struct MuxMediaCapture
{
	char 					name[CMN_NAME_LENGTH];
	
	int						nbStreams;
	MuxMediaDescriber			*capturedMedias[MUX_MAX_MEDIA_STREAMS];
	int						ptsDifference;
	int						videoStreamIndex;
	int						audioStreamIndex;

	int						nbConsumers;
	MuxMediaConsumer		*mediaConsumers;


	/* used in synchronization between audio and video */
	MUX_SYNC_MASTER		masterClock;

	int64_t					audioClock;

	int64_t					videoCurrentPts;		/* as video clock, PTS */
	int64_t					videoCurrentPtsTime;

	double					sysClock; /* external clock base */
	int64_t					sysClockTime;

	/*used in feed delay, in second, float to minimize wrapping */
	double 					clockAudio;
	double					clockVideo;
	double					startTime;

	double					durationAudioFrame;		/* in second */
	double					durationVideoFrame;		/* in second */


	int						audioPacketCount;	/* count of video packet */
	int						audioPacketDrop;
	int						videoPacketCount;	/* count of video packet */
	int						videoPacketDrop;


	/* create media describers for every stream. implemented by PLAYER and FileFeed */
	int	(*createMediaDescribers)(struct MuxMediaCapture *);
	/* EXIT_SUCCESS: OK; other : failed */
	int	(*startCapture)(struct MuxMediaCapture *);	/* start capturing data from its source */

	int	(*getCaptureState)(struct MuxMediaCapture *);	/* check whether capturer is usable, for example player is playing */

	struct _MuxMain			*muxMain;

	struct MuxMediaCapture	*next;
	void						*priv;		/* point to the provider of capture, eg. PLAYER or FEED */
}MuxMediaCapture;



typedef	struct _MEDIA_FILE
{
	char 			name[CMN_NAME_LENGTH];
	char				path[CMN_NAME_LENGTH];

	char				formatName[128];
#if 0	
	FILE_VIDEO_TYPE_E		type;
#else
	int						type;
#endif
	int			size;		/* File size in Kbytes, -1 if unknown. */
	int			duration;	/* in unit of second. 0 if unknown */

	unsigned char		hasVideo;
	unsigned char		hasAudio;
	unsigned char		hasSubtitle;

	/**< Time of last modification in microseconds since unix epoch, -1 if unknown. */
	int			modification_timestamp;
	/**< Time of last access in microseconds since unix epoch, -1 if unknown. */
	int			access_timestamp; 
	
	/**< Time of last status change in microseconds since unix epoch, -1 if unknown. */
	int			status_change_timestamp;
	
}MEDIA_FILE_T;


typedef int MEDIA_FILE_HANDLER(MEDIA_FILE_T *file, void *data);


struct _MEDIA_FILE_LIST;

typedef	struct _MEDIA_FILE_LIST
{
	char						deviceName[CMN_NAME_LENGTH];

	MEDIA_DEVICE_T			type;
	struct cmn_list			*list;
#if 0
	char						*fullPath; /* directory name of full path */
#else
	char						fullPath[CMN_NAME_LENGTH]; /* directory name of full path */
#endif
	time_t					scanTime;
	time_t					lastTime;

	int						countSubDirs; /* total number of subdirectories scanned */
	int						countSubFiles; /* total number of files scanned */ 

	struct _MEDIA_FILE_LIST	*next;
}MEDIA_FILE_LIST_T;


/****************************** part 2: macros **************************/
#define	IS_STREAM_FORMAT( fmt, _name)  \
		( ((fmt) != NULL) && (! strcmp( (fmt)->name, _name)) )


#define	IS_KEY_PACKET(pkt)	\
		((pkt)->flags & AV_PKT_FLAG_KEY)
		
#if 0
#define	IS_LOCAL_IMAGE_FILE(imageFile)		\
			( cmnMediaCheckLocalFile((imageFile)) && cmnMediaCheckCheckImageFile((imageFile)))
#else
#define	IS_LOCAL_IMAGE_FILE(imageFile)		\
			( cmnMediaCheckCheckImageFile((imageFile)))
#endif



//#if	DEBUG_FFMPEG
#if 0//ndef   __CMN_RELEASE__
#define	MUX_INIT_FFMPEG(logLevel)	\
	{ int level =AV_LOG_WARNING; 	av_register_all(); ret = avformat_network_init(); \
		if( (logLevel) == CMN_LOG_DEBUG) 	{ level = AV_LOG_TRACE; 	} \
		else if( (logLevel) == CMN_LOG_INFO) { level = AV_LOG_DEBUG; } \
		else if( (logLevel) == CMN_LOG_NOTICE) { level = AV_LOG_INFO; } \
		else if( (logLevel) == CMN_LOG_WARNING) { level = AV_LOG_WARNING; } \
		else if( (logLevel) == CMN_LOG_ERR) {level = AV_LOG_ERROR;	} \
		av_log_set_level( level); 	}
#else
#define	MUX_INIT_FFMPEG(logLevel)	\
	{ int level =AV_LOG_WARNING; 	av_register_all(); ret = avformat_network_init(); \
		if( (logLevel) == CMN_LOG_DEBUG) 	{ level = AV_LOG_TRACE; 	} \
		else if( (logLevel) == CMN_LOG_INFO) { level = AV_LOG_DEBUG; } \
		else if( (logLevel) == CMN_LOG_NOTICE) { level = AV_LOG_INFO; } \
		else if( (logLevel) == CMN_LOG_WARNING) { level = AV_LOG_WARNING; } \
		else if( (logLevel) == CMN_LOG_ERR) {level = AV_LOG_ERROR;	} level= CMN_LOG_ERR;  \
		av_log_set_level( level); 	}
#endif


/****************************** part 3: functions declarations **************************/
/* common interfaces from libMedia */
int	cmnMediaInit(MuxMediaConfig *mediaCaptureConfig);

void cmnMediaInitAvStream(void *avStream, MuxMediaDescriber *desc);
MuxMediaDescriber *cmnMediaCreateDescriberFromStream(void *avStream );

int cmnMediaCreateDescribersForPlayer(MuxMediaCapture *mediaCapture);

int cmnMediaCaptureWrite(MuxMediaCapture *muxCapture, MUX_MEDIA_TYPE type, unsigned char *data, unsigned int size,  int64_t pts, int64_t dts);

int cmnMediaCapturerSendNotify(MuxMediaCapture *muxCapture, MUX_CAPTURE_EVENT event, void *data);

int cmnMediaConsumerReceive(MuxMediaCapture *muxCapture, MUX_MEDIA_TYPE type, void *packet);

/* export functions of scan disk */
cmn_list_t	*cmnMediaScanGetFiles(void);

int	cmnMediaCheckLocalFile(char *localImageFile);
int	cmnMediaCheckCheckImageFile(char *imageFile);

int	cmnMediaCheckSDCardFile(char *fileName);
int	cmnMediaRemountSDCard(int isWrite);

#endif

