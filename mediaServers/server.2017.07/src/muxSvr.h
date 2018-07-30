/*
 */

#ifndef __MUX_SVR_H__
#define __MUX_SVR_H__

#define MUX_PACKET_SIZE				4096
#define MUX_SVR_MAX_STREAMS			20

#define PATH_LENGTH						1024

#define IOBUFFER_INIT_SIZE				8192

/* timeouts are in ms */
#define HTTP_REQUEST_TIMEOUT			(15 * 1000)	/* 15 seconds */
#define RTSP_REQUEST_TIMEOUT			(3600 * 24 * 1000)	/* one day */

#define SYNC_TIMEOUT					(10 * 1000)			/* 10 seconds */

#define MAX_CHILD_ARGS					64

//#if ARCH_ARM
#if ARCH_X86
#define	MUX_SERVER_CONFIG_FILE		ROOT_DIR"/releases/etc/muxServer.conf"
#else
#define	MUX_SERVER_CONFIG_FILE		"/etc/muxServer.conf"
#endif


#include "config.h"
#if !HAVE_CLOSESOCKET
#define closesocket close
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libavformat/avformat.h"
#include "libavformat/network.h"
/* FIXME: those are internal headers, ffserver _really_ shouldn't use them */
#include "libavformat/rtpproto.h"
#include "libavformat/rtsp.h"
#include "libavformat/avio_internal.h"
#include "libavformat/internal.h"

#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/lfg.h"
#include "libavutil/dict.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/mathematics.h"
#include "libavutil/random_seed.h"
#include "libavutil/rational.h"
#include "libavutil/parseutils.h"
#include "libavutil/opt.h"
#include "libavutil/_time.h"
#include "libavutil/pixdesc.h"


#include <float.h>
#include <stdarg.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/ioctl.h>
#if HAVE_POLL_H
#include <poll.h>
#endif
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

//#include "cmdutils.h"

#define	S_EXIT_FAILURE			-1
#define	S_EXIT_SUCCESS			0

#define	MUX_SERVER_NAME		"MuxLab Server"

/* each generated stream is described here */
enum FFServerStreamType
{
	STREAM_TYPE_LIVE,
	STREAM_TYPE_STATUS,
	STREAM_TYPE_REDIRECT,
};

enum FFServerIPAddressAction
{
	IP_ALLOW = 1,
	IP_DENY,
};

typedef struct FFServerIPAddressACL
{
	struct FFServerIPAddressACL	*next;
	enum FFServerIPAddressAction	action;
	
	/* These are in host order */
	struct in_addr			first;
	struct in_addr			last;
} FFServerIPAddressACL;

/**
 * This holds the stream parameters for an AVStream, it cannot be a AVStream
 * because AVStreams cannot be instanciated without a AVFormatContext, especially
 * not outside libavformat.
 *
 * The fields of this struct have the same semantics as the fields of an AVStream.
 */
typedef struct LayeredAVStream
{
	int					index;
	int					id;
	
	AVCodecParameters	*codecpar;
	AVCodecContext		*codec;
	AVRational			time_base;
	int					pts_wrap_bits;
	AVRational			sample_aspect_ratio;
	char					*recommended_encoder_configuration;
} LayeredAVStream;

struct _MUX_SVR;

/* description of each stream of the ffserver.conf file */
typedef struct MuxStream
{
	enum FFServerStreamType	stream_type;
	
	char						filename[1024];          /* stream filename */
	struct MuxStream		*feed;  /* feed we are using (can be null if coming from file) */
	
	AVDictionary				*in_opts;        /* input parameters */
	AVDictionary				*metadata;       /* metadata to set on the stream */
	
	AVInputFormat			*ifmt;		/* from 'InputFormat' field. if non NULL, force input format */
	AVOutputFormat			*fmt;		/* from 'format' field */
	
	FFServerIPAddressACL		*acl;
	char						dynamic_acl[1024];
	
	int			nb_streams;
	int			prebuffer;                /* Number of milliseconds early to start */
	int64_t		max_time;             /* Number of milliseconds to run */
	int			send_on_key;
	
	LayeredAVStream		*streams[MUX_SVR_MAX_STREAMS];
	int					feed_streams[MUX_SVR_MAX_STREAMS]; /* index of streams in the feed */
	char					feed_filename[1024];     /* file name of the feed storage, or input file name for a stream */

	pid_t				pid;                    /* Of ffmpeg process */
	time_t				pid_start;             /* Of ffmpeg process */
	
	char 				**child_argv;
	struct MuxStream	*next;
	
	unsigned				bandwidth;           /* bandwidth, in kbits/s */

	/* RTSP options */
	char		*rtsp_option;
	
	/* multicast specific */
	int				is_multicast;
	struct in_addr	multicast_ip;
	int				multicast_port;           /* first port used for multicast */
	int				multicast_ttl;

	int				loop;                     /* if true, send the stream in loops (only meaningful if file) */
	char				single_frame;            /* only single frame */

	/* feed specific */
	int				feed_opened;              /* true if someone is writing to the feed */
	int				is_feed;                  /* true if it is a feed */
	int				readonly;                 /* True if writing is prohibited to the file */
	int				truncate;                 /* True if feeder connection truncate the feed file */
	
	int				conns_served;
	int64_t			bytes_served;
	int64_t			feed_max_size;        /* maximum storage size, zero means unlimited */
	int64_t			feed_write_index;     /* current write position in feed (it wraps around) */
	int64_t			feed_size;            /* current size of feed */
	struct MuxStream *next_feed;

	struct _MUX_SVR		*muxSvr;
} MuxStream;

typedef struct MuxServerConfig
{
	char				*filename;
	
	MuxStream	*first_feed;   /* contains only feeds */
	MuxStream	*first_stream; /* contains all streams, including feeds */

	unsigned int		nb_max_http_connections;
	unsigned int		nb_max_connections;
	uint64_t			max_bandwidth;
	
	int				debug;
	int				bitexact;
	
	char					logfilename[1024];
	
	struct sockaddr_in		http_addr;
	struct sockaddr_in		rtsp_addr;
	
	int				errors;
	int				warnings;
	int				use_defaults;
	
	// Following variables MUST NOT be used outside configuration parsing code.
	enum AVCodecID guessed_audio_codec_id;
	enum AVCodecID guessed_video_codec_id;
	
	AVDictionary		*video_opts;     /* AVOptions for video encoder */
	AVDictionary		*audio_opts;     /* AVOptions for audio encoder */

	AVCodecContext	*dummy_actx;   /* Used internally to test audio AVOptions. */
	AVCodecContext	*dummy_vctx;   /* Used internally to test video AVOptions. */

	int				no_audio;
	int				no_video;

	int				line_num;
	int				stream_use_defaults;

	struct _MUX_SVR		*muxSvr;
} MuxServerConfig;

void muxUtilGetArg(char *buf, int buf_size, const char **pp);
int muxUtilResolveHost(struct in_addr *sin_addr, const char *hostname);

void muxSvrParseAclRow(MuxStream *stream, MuxStream* feed, FFServerIPAddressACL *ext_acl, const char *p, const char *filename, int line_num);

int muxSvrParseConfig(const char *filename, MuxServerConfig *config);

void ffserver_free_child_args(void *argsp);


enum HTTPState
{
	HTTPSTATE_WAIT_REQUEST,
	HTTPSTATE_SEND_HEADER,
	HTTPSTATE_SEND_DATA_HEADER,
	HTTPSTATE_SEND_DATA,          /* sending TCP or UDP data */
	HTTPSTATE_SEND_DATA_TRAILER,
	HTTPSTATE_RECEIVE_DATA,
	HTTPSTATE_WAIT_FEED,          /* wait for data from the feed */
	HTTPSTATE_READY,

	RTSPSTATE_WAIT_REQUEST,
	RTSPSTATE_SEND_REPLY,
	RTSPSTATE_SEND_PACKET,
};


typedef struct RTSPActionServerSetup
{
	uint32_t		ipaddr;
	char			transport_option[512];
} RTSPActionServerSetup;

typedef struct
{
	int64_t	count1,	count2;
	int64_t	time1,	time2;
} DataRateData;

/* context associated with one connection */
typedef struct MuxContext
{
	enum HTTPState	state;
	
	int					fd; /* socket file descriptor */
	struct sockaddr_in		from_addr; /* origin */
	struct pollfd			*poll_entry; /* used when polling */
	int64_t				timeout;
	
	uint8_t				*buffer_ptr, *buffer_end;
	int			http_error;
	int			post;	/* HTTP request is POST */
	
	int			chunked_encoding;
	int			chunk_size;/* 0 if it needs to be read */
	
	struct MuxContext	*next;
	
	int	got_key_frame; /* stream 0 => 1, stream 1 => 2, stream 2=> 4 */
	int64_t data_count;
	
	/* feed input */
	int			feed_fd;
	/* input format handling */
	AVFormatContext	*fmt_in;
	
	int64_t	start_time;            /* In milliseconds - this wraps fairly often */
	int64_t	first_pts;            /* initial pts value */
	int64_t		cur_pts;             /* current pts value from the stream in us */
	int64_t		cur_frame_duration;  /* duration of the current frame in us */
	int			cur_frame_bytes; /* output frame size, needed to compute the time at which we send each packet */
	
	int		pts_stream_index;        /* stream we choose as clock reference */
	int64_t cur_clock;           /* current clock reference value in us */

	/* output format handling */
	struct MuxStream	*stream;
	/* -1 is invalid stream */
	int		feed_streams[MUX_SVR_MAX_STREAMS]; /* index of streams in the feed */
	int		switch_feed_streams[MUX_SVR_MAX_STREAMS]; /* index of streams in the feed */
	int		switch_pending;
	AVFormatContext	*pfmt_ctx; /* instance of MuxStream for one user */
	int		last_packet_sent; /* true if last data packet was sent */
	int		suppress_log;
	
	DataRateData datarate;
	int wmp_client_id;
	char		protocol[16];
	char		method[16];
	char		url[128];
	char		clean_url[128*7];
	int		buffer_size;
	uint8_t	*buffer;
	int		is_packetized; /* if true, the stream is packetized, eg.UDP; TCP is streamized */
	int		packet_stream_index; /* current stream for output in state machine */

	/* RTSP state specific */
	uint8_t		*pb_buffer; /* XXX: use that in all the code */
	AVIOContext	*pb;
	int			seq; /* RTSP sequence number */

	/* RTP state specific */
	enum RTSPLowerTransport	rtp_protocol;
	char						session_id[32]; /* session id */
	AVFormatContext			*rtp_ctx[MUX_SVR_MAX_STREAMS];

	/* RTP/UDP specific */
	URLContext				*rtp_handles[MUX_SVR_MAX_STREAMS];

	/* RTP/TCP specific */
	struct MuxContext			*rtsp_c;
	uint8_t	*packet_buffer, *packet_buffer_ptr, *packet_buffer_end;

	struct _MUX_SVR		*muxSvr;

} MuxContext;


typedef	struct _MUX_SVR
{
	MuxServerConfig	config;

	char 			programName[128];


	int				no_launch;
	int				need_to_start_children;

	/* maximum number of simultaneous connections */
	
	unsigned int		nb_connections;
	MuxContext		*firstConnectCtx;


	uint64_t			current_bandwidth;

	/* Making this global saves on passing it around everywhere */
	int64_t			currentTime;

	AVLFG			random_state;

}MUX_SVR;


int muxSvrSendData(MuxContext *c);
int muxSvrReceiveData(MuxContext *c);
int muxSvrStartReceiveData(MuxContext *c);

void muxSvrConnClose(MuxContext *c);
int muxSvrConnOpenInputStream(MuxContext *c, const char *info);
MuxContext *muxSvrConnNewRTP(MUX_SVR *muxSvr, struct sockaddr_in *from_addr, MuxStream *stream, const char *session_id, enum RTSPLowerTransport rtp_protocol);


int muxSvrRtspParseRequest(MUX_SVR *muxSvr, MuxContext *c);
int muxSvrHttpParseRequest(MUX_SVR *muxSvr, MuxContext *c);
void muxSvrHttpStatusPage(MUX_SVR *muxSvr, MuxContext *c);


/* SDP handling */
int muxSvrPrepareSdpDescription(MuxStream *stream, uint8_t **pbuffer, struct in_addr my_ip);

int muxSvrRtpNewAvStream(MuxContext *c, int stream_index, struct sockaddr_in *dest_addr, MuxContext *rtsp_c);

/* utils */
size_t htmlencode (const char *src, char **dest);

void muxUtilGetWord(char *buf, int buf_size, const char **pp);

void muxSvrUpdateDatarate(MUX_SVR *muxSvr, DataRateData *drd, int64_t count);
int muxSvrComputeDatarate(MUX_SVR *muxSvr, DataRateData *drd, int64_t count);

int muxSvrValidateAcl(MuxStream *stream, MuxContext *c);


int64_t ffm_read_write_index(int fd);
int ffm_write_write_index(int fd, int64_t pos);
void ffm_set_write_index(AVFormatContext *s, int64_t pos, int64_t file_size);
void unlayer_stream(AVStream *st, LayeredAVStream *lst);


void muxSvrBuildFileStreams(MUX_SVR *muxSvr);
int muxSvrBuildFeedStreams(MUX_SVR *muxSvr);

int muxSvrServer(MUX_SVR *muxSvr);



#define	SVR_ADD_NEW_CONN(muxSvr, conn) \
	{ (conn)->next = (muxSvr)->firstConnectCtx; (muxSvr)->firstConnectCtx = (conn); \
		(muxSvr)->nb_connections++; (conn)->muxSvr = (muxSvr); }

#define	DEBUG	1

void muxSvrLog(const char *fmt, ...);

#define	VTRACE()	\
		muxSvrLog("%s.%s().%d\n", __FILE__, __FUNCTION__, __LINE__)
		

#if DEBUG
#define MUX_SVR_LOG(...) \
		{muxSvrLog("%s.%s().%d | ", __FILE__, __FUNCTION__, __LINE__); \
			muxSvrLog(__VA_ARGS__);}
#else
void MUX_SVR_LOG(const char *fmt, ...)		{muxSvrLog(__VA_ARGS__);}
#endif

void muxSvrInitLog(MUX_SVR *muxSvr);
void muxCfgError(const char *filename, int line_num, int log_level, int *errors, const char *fmt, ...);



#define CONFIG_ERROR(...) muxCfgError(config->filename, config->line_num,\
                                       AV_LOG_ERROR, &config->errors, __VA_ARGS__)
#define CONFIG_WARNING(...) muxCfgError(config->filename, config->line_num,\
                                         AV_LOG_WARNING, &config->warnings, __VA_ARGS__)

#define MUX_ARRAY_ELEMS(a) 	(sizeof(a) / sizeof((a)[0]))


const char *muxSvrStatusName(int status);

#endif

