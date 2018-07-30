/*
 */

#ifndef __MUX_SVR_H__
#define __MUX_SVR_H__


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


#include "config.h"

#if !HAVE_CLOSESOCKET
#define closesocket close
#endif

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
#include "libavutil/timestamp.h"


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


#include "libCmn.h"
#include "libCmnRx.h"
#include "libMedia.h"

#define TUNECPU				generic

#define restrict 				__restrict__

#ifdef __cplusplus
extern "C" {
#endif


/******************************************
* First : build options 
*******************************************/

#define	MUX_SVR_DEBUG_DATA_FLOW		0

#define	MUX_SVR_DEBUG_TIME_STAMP		0

#define	MUX_WITH_CODEC_FIELD				0

#define	MUX_WITH_AVI_H264_ANNEXB		1	/* AVI container save h264 video in annex B format */

/* for timing of (RTSP/RTP) packet and its sending time */
#define	MUX_SVR_TIMING_PACKET			1


#define	WITH_HTTP_AUTHENTICATE 				0
#define	WITH_RTSP_AUTHENTICATE 				0

#define	WITH_SERVER_HTTP_STATS				1
#define	WITH_AV_CODEC_RAW_ENCODE			0
#define	WITH_ADDRESS_ACL						0


#define	WITH_SERVER_WATCHDOG				0


#define	WITH_CONNTION_LOCK					0

#define	WITH_RTSP_UDP_MULTICAST				0


#define	MEDIA_FIFO_LENGTH						250



/******************************************
* Second : Macro constants  
*******************************************/

/* watchdog driver timeout support 1-60s */
#define WDT_MIN_TIME	1
#define WDT_MAX_TIME	60


#define MS_AUTH_PASSWDFILE			"/etc/web/userList.conf"


#define	WATCHDOG_DEVICE				"/dev/watchdog"

#define	RECORD_STREAM_NAME			"RECORDER"


#define IOBUFFER_INIT_SIZE 				8192


#define MUX_PACKET_SIZE				4096
#define MUX_SVR_MAX_STREAMS			20

#define PATH_LENGTH						1024


/* maximum number of simultaneous HTTP connections */
#define HTTP_MAX_CONNECTIONS 			2000

/* timeouts are in ms */
#define HTTP_REQUEST_TIMEOUT			(15 * 1000)	/* 15 seconds */
#define RTSP_REQUEST_TIMEOUT			(3600 * 24 * 1000)	/* one day */


#define MAX_CHILD_ARGS					64

//#if ARCH_ARM
#if ARCH_X86
#define	MUX_SERVER_CONFIG_FILE		ROOT_DIR"/releases/etc/muxServer.conf"
#else
#define	MUX_SERVER_CONFIG_FILE		"/etc/muxServer.conf"
#endif


//#include "cmdutils.h"

#define	S_EXIT_FAILURE			-1
#define	S_EXIT_SUCCESS			0

#define	MUX_SERVER_NAME		"MuxLab Server"



/******************************
 Event and State of all FSM
******************************/
/* state for all FSM */
#define	MUX_SVR_STATE_CLOSING			-1
#define	MUX_SVR_STATE_CONTINUE			0

/* states for HTTP/UMC/RTMP/HLS */
typedef enum 
{
	/* states for HTTP and others conn, except RTSP */
	HTTP_STATE_INIT		=	MUX_SVR_STATE_CONTINUE + 1,
	HTTP_STATE_DATA,          		/* sending TCP or UDP data */
	HTTP_STATE_ERROR,			/* send back content of webpage and then close this connection */

	/* states only for RTSP conn */
	RTSP_STATE_INIT ,
	RTSP_STATE_READY,
	RTSP_STATE_PLAY,
	RTSP_STATE_RECORD,
	
	RTSP_STATE_ERROR,
	
}MUX_SVR_STATE;


/* common events for all FSMs */
typedef	enum _mux_svr_event
{
	MUX_SVR_EVENT_NONE = 0,	/* default value 0: no event happens */
	MUX_SVR_EVENT_ERROR,		/* 1 */
	MUX_SVR_EVENT_TIMEOUT,
	MUX_SVR_EVENT_POLL_IN,
	MUX_SVR_EVENT_POLL_OUT,	/* 4 */
	MUX_SVR_EVENT_POLL_ERR,
	MUX_SVR_EVENT_DATA,		/* 6, virtual event for AVPacket send out */
	MUX_SVR_EVENT_START,		/* 7 */
	MUX_SVR_EVENT_CLOSE,		/* 8 */
}mux_svr_event_t;


typedef	enum _HTTP_MSG
{
	HTTP_MSG_STATUS_PAGE =  MUX_SVR_EVENT_CLOSE + 1,
	HTTP_MSG_LIVE_STREAM,
	HTTP_MSG_REDIRECT_PARSED,
	HTTP_MSG_POST,
	HTTP_MSG_INFO,				/* */
}HTTP_MSG;

typedef	enum _RTSP_MSG
{
	RTSP_MSG_OPTIONS = MUX_SVR_EVENT_CLOSE + 1,
	RTSP_MSG_DESCRIBE,
	RTSP_MSG_SETUP,
	RTSP_MSG_PLAY,
	RTSP_MSG_RECORD,
	RTSP_MSG_PAUSE,
	RTSP_MSG_TEARDOWN,
}RTSP_MSG;


struct _SERVER_CONNECT;
typedef	struct
{
	int						event;
	int						statusCode;
	
	struct _SERVER_CONNECT	*myConn;

	void						*data;	/* private data for this event */
}MUX_SVR_EVENT;


typedef	enum
{
	SRV_HTTP_ERRCODE_NO_ERROR = 0,
	SRV_HTTP_ERRCODE_UNSUPPORT_HTTP_METHOD,
	SRV_HTTP_ERRCODE_UNAUTHORIZED,
	SRV_HTTP_ERRCODE_IP_ACL_DENY,
	SRV_HTTP_ERRCODE_FILE_NOT_FOUND,
	SRV_HTTP_ERRCODE_RESOURCE_UNAVAILABE,
	SRV_HTTP_ERRCODE_UNSUPPORT_PROTOCOL,
	SRV_HTTP_ERRCODE_INVALIDATE_STREAM,
	SRV_HTTP_ERRCODE_REDIRECT_FAILED,
	SRV_HTTP_ERRCODE_CLOSED_BY_PEER,
}SRV_HTTP_ERROR_CODE;

typedef	enum
{
	SRV_HTTP_INFOCODE_REDIRECT,
	SRV_HTTP_INFOCODE_BANDWIDTH_LIMITED,
}SRV_HTTP_INFO_CODE;


enum MuxStreamType
{
	MUX_STREAM_TYPE_LIVE = 0, 		/* default type, include RTSP, HTTP stream */
	MUX_STREAM_TYPE_UMC,			/* UDP MutliCast */
	MUX_STREAM_TYPE_RTMP,		/* RTMP */
	MUX_STREAM_TYPE_HLS,			/* HTTP Live Stream */
	MUX_STREAM_TYPE_STATUS,
	MUX_STREAM_TYPE_REDIRECT,
};

enum MuxFeedType
{
	MUX_FEED_TYPE_FILE = 0, 		/* Feed from media file */
	MUX_FEED_TYPE_CAPTURE,		/* Feed from capture interface in RX player */
	MUX_FEED_TYPE_TX_CAPTURE,	/* Feed from capture interface in TX */
	MUX_FEED_TYPE_UNKNOWN,
};



typedef	enum
{
	CONN_TYPE_HTTP	=	0,
	CONN_TYPE_RTSP,
	CONN_TYPE_MULTICAST,	/* UDP Multicast without RTP */
	CONN_TYPE_RTMP,
	CONN_TYPE_HLS,

	CONN_TYPE_RTP_UDP,
	CONN_TYPE_RTP_TCP,
	CONN_TYPE_RTP_MULTICAST,

	CONN_TYPE_RECORDER,
}conn_type_t;


enum RedirType
{
	REDIR_NONE,
	REDIR_ASX,
	REDIR_RAM,
	REDIR_ASF,
	REDIR_RTSP,
	REDIR_SDP,
};



enum FFServerIPAddressAction
{
	MUX_SVR_ACL_DISABLE = 0,
	MUX_SVR_ACL_IP_ALLOW = 1,
	MUX_SVR_ACL_IP_DENY,
};


typedef	struct
{
	MUX_SVR_EVENT			myEvent;
	
	RTSP_MSG				method;

	RTSPMessageHeader		header;

	/* parse as url : 'protocol://host:port/path' */
	char 					protocol[16];
	char 					host[64];
	int						port;
	char						path[128];
}rtsp_event_t;



struct	_svr_transition_t;
struct _SERVER_CONNECT;

struct _svr_transition_t
{
	int						event;
	int						(*handle)(struct _SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event);
}SVR_TRANSITION;

typedef	struct _svr_transition_t	svr_transition_t;

struct	_svr_transition_table_t
{
	int					state;
	int					size;
	svr_transition_t			*eventHandlers;
	
	void					(*enter_handle)(struct _SERVER_CONNECT *svrConn);
};

typedef	struct	_svr_transition_table_t			svr_statemachine_t;

typedef	struct	_SVR_SERVICE_FSM
{
	int							size;
	svr_statemachine_t			*states;
}SVR_SERVICE_FSM;


typedef struct
{/* data for the last 2 times */
	int64_t	count1,	count2;
	int64_t	time1,	time2;
} DataRateData;


typedef	struct	_svr_buffer
{
	AVIOContext			*dynaBuffer;

	uint8_t 				*dBuffer; /* XXX: use that in all the code */

	uint8_t				*bufferPtr;
	uint8_t				*bufferEnd;
	int					isInitedDynaBuf;
}SVR_BUFFER;	


struct MuxStream;


/* HTTP/RTSP Server Connection */
typedef struct _SERVER_CONNECT
{
	char							name[1024];
	conn_type_t					type;

	int							fd;									/* socket file descriptor */
	
	
	struct _SERVER_CONNECT		*myListen;		/* point to the SVR_CON of monitor socket */

	int							(*handle)(struct _SERVER_CONNECT *);		/* handler of fd, used by MntrThread */


	int						state;
	SVR_SERVICE_FSM			*fsm;
	int						currentEvent;
	
	struct sockaddr_in			peerAddress;		/* origin */

	int						errorCode;

	int						got_key_frame; /* stream 0 => 1, stream 1 => 2, stream 2=> 4 */
	int64_t					dataCount;		/* total bytes for this connection, received or send */

	/* timeout for the new RTSP/HTTP request after request has been come. It is used for DOS; 
	only used in monitor/listen service connection of HTTP/RTSP */
	long						timeout;		

	time_t					timeLast;           /* time of last succ. op. */
	
	long						startTime;            /* In milliseconds - this wraps fairly often */
	int64_t					firstPts;            /* initial pts value */

#if MUX_SVR_TIMING_PACKET	
	int64_t 					currentFramePts;			/* current pts value, in us, relative to the firstPts */
	int64_t					currentFrameDuration;		/* duration of the current frame in us */
	int						currentFrameBytes;			/* output frame size, needed to compute the time at which we send each packet */
#endif
	
	int 						pts_stream_index;       /* stream we choose as clock reference */

	struct MuxStream 			*stream;		/* which MEDIA_STREAM of server this connection used, output format handling  */
	
	AVFormatContext 			*outFmtCtx;		/* for Out media of HTTP connection */
	
	/* -1 is invalid stream */
	int 						feed_streams[MUX_SVR_MAX_STREAMS]; 		/* index of streams in the feed of server */

	int 						switch_feed_streams[MUX_SVR_MAX_STREAMS]; /* index of streams in the feed */
	int 						switch_pending;
	
	int 						last_packet_sent; /* true if last data packet was sent */
	int 						suppress_log;
	DataRateData 			datarate;
	int 						wmp_client_id;
	
	char 					protocol[256];
	char 					method[256];
	char 					url[256];
	char 					info[256]; /* first part of URL after a '?' */
	char						filename[1024];
	
	/* buffer for read : read command from client */
	int 						bufferReadSize;
	SVR_BUFFER					readBuffer;

	/* buffer for output (command and media data )*/	
	AVIOContext					*dynaBuffer;
	int							isInitedDynaBuf;
	
	SVR_BUFFER					cmdBuffer; /* Header and Trailer belong and sent in cmdBuffer */
	
	SVR_BUFFER					dataBuffer;
	int							currentStreamIndex;	/* data buffer for current stream index */

	int 						is_packetized; /* if true, the stream is packetized */
	int 						packet_stream_index; /* current stream for output in state machine */

#if 	0
	uint8_t 						*dBuffer; /* XXX: use that in all the code */

	uint8_t						*bufferPtr, *bufferEnd;
#endif

	cmn_fifo_t					*fifo;
	AVPacket 					*lastPkt;	/* buffering the last pkt for a while */
	
	int							packetCountDrop;	/* drop because of limit of FIFO size */
	int							packetCountTotal;	/* total packet, include droped and sent */

	int 							seq; /* RTSP sequence number */

	URLContext 					*urlContext;		/* shared by 2 data connections of RTMP */
	int							maxPacketSize;


	int64_t						lastDTSes[MUX_SVR_MAX_STREAMS];		/* AvStream->cur_dts when last rewind feed */
	struct _DATA_CONNECT		*dataConns[MUX_SVR_MAX_STREAMS];

	struct _MUX_SVR				*muxSvr;
	
	struct _SERVER_CONNECT		*next;	
} SERVER_CONNECT;


/* context associated with one connection */
typedef struct _DATA_CONNECT
{
	long						timeout;
//	uint8_t					*bufferPtr, *bufferEnd;
	
	struct MuxStream 			*stream;		/* which MEDIA_STREAM of server this connection used, output format handling  */
	int						streamIndex;

	struct _DATA_CONNECT 	*next;

	SERVER_CONNECT			*myParent;

	int						(*sendOut)(struct _DATA_CONNECT *conn, uint8_t *buffer, int length);

	int						got_key_frame; /* stream 0 => 1, stream 1 => 2, stream 2=> 4 */
	int64_t					dataCount;		/* total bytes for this connection, received or send */

	
	int						feed_fd;			/* feed input : HTTP recv POST data and use this fd write data to feed ffm file */

	long						startTime;            /* In milliseconds - this wraps fairly often */
	int64_t					firstPts;            /* initial pts value */
	int64_t 					currentPts;              /* current pts value */
	int 						ptsStreamIndex;       /* stream we choose as clock reference */

	
	/* -1 is invalid stream */
	int 						feed_streams[MUX_SVR_MAX_STREAMS]; 		/* index of streams in the feed of server */
	int 						switch_feed_streams[MUX_SVR_MAX_STREAMS]; /* index of streams in the feed */
	int 						switch_pending;

	int 						last_packet_sent; /* true if last data packet was sent */

	DataRateData 			datarate;
	
	int 						is_packetized; /* if true, the stream is packetized */
	int 						packet_stream_index; /* current stream for output in state machine */

	/* RTSP state specific */
//	uint8_t 					*pDynaBuffer; /* XXX: use that in all the code */

	AVFormatContext 			*outFmtCtx;	/* for RTP, it is rtp_mux, for HTTP, it is avi output format */
	/* RTP short term bandwidth limitation */
	int 						packet_byte_count;
	int 						packet_start_time_us; /* used for short durations (a few seconds max) */

	/* RTP/UDP specific */
	URLContext 				*urlContext;

	enum RTSPLowerTransport 	rtpProtocol;

	char						session_id[32];
	char 					protocol[16];
} DATA_CONNECT;


typedef struct FFServerIPAddressACL
{
	struct FFServerIPAddressACL	*next;
	enum FFServerIPAddressAction	action;
	
	/* These are in host order */
	struct in_addr			first;
	struct in_addr			last;
} FFServerIPAddressACL;

typedef	struct
{
	enum AVCodecID		id;

	int					channels;
	int					bitRate;
	int					sampleRate;
	int					quality;
	
	unsigned char			drivers[64];
}audio_config_t;

typedef struct
{
	enum AVCodecID		id;

	int					maxRate;
	int					minRate;

	int					width;
	int					height;

	int					bitRate;
	int					frameRate;
	int					frameRateBase;
	
	int					gopSize;
	int					sendOnKey;

	unsigned char			drivers[64];
}video_config_t;

#if 0
typedef	struct
{
	char					author[512];
	char					title[512];
	char					copyright[512];
	char					comment[512];
}stream_descript_t;
#endif

typedef	struct ip_config
{
	char					address[20];
	int					port;
}ip_config_t;


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
	
#if MUX_WITH_CODEC_FIELD	
	AVCodecContext		*codec;
#endif
	AVCodecParameters	*codecpar;
	AVRational			time_base;
	int					pts_wrap_bits;
	AVRational			sample_aspect_ratio;
	char					*recommended_encoder_configuration;
} LayeredAVStream;

struct _MUX_SVR;
struct _MuxFeed;

/* description of each stream of the ffserver.conf file */
typedef struct MuxStream
{
	enum MuxStreamType		type;
	
	char						filename[1024];          /* stream filename */
	
	AVDictionary				*in_opts;        /* input parameters */
	AVDictionary				*metadata;       /* metadata to set on the stream */
	
	AVInputFormat			*ifmt;		/* from 'InputFormat' field. if non NULL, force input format */
	AVOutputFormat			*fmt;		/* from 'format' field */
	
	FFServerIPAddressACL		*acl;
	char						dynamic_acl[1024];
	
	int			prebuffer;                /* Number of milliseconds early to start */
	int64_t		max_time;             /* Number of milliseconds to run */
	int			send_on_key;
	
//	int						nbStreams;
//	LayeredAVStream			*streams[MUX_SVR_MAX_STREAMS];
	int					feed_streams[MUX_SVR_MAX_STREAMS]; /* index of streams in the feed */
	char					feed_filename[1024];     /* file name of the feed storage, or input file name for a stream */
	char					rtmpUrl[1024];

	
	struct MuxStream	*next;
	
	unsigned				bandwidth;           /* bandwidth, in kbits/s */

	/* RTSP options */
	char		*rtsp_option;
	
	/* multicast specific */
	int				is_multicast;
	ip_config_t		multicastAddress;

	int				multicast_ttl;

	int				loop;                     /* if true, send the stream in loops (only meaningful if file) */
	char				single_frame;            /* only single frame */

	stream_descript_t		description;
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


	DATA_CONNECT		*dataConns;
	void					*private;
	
	struct _MuxFeed		*feed;
	
	struct _MUX_SVR		*muxSvr;
} MuxStream;


typedef struct _MuxFeed
{
	enum MuxFeedType		type;

	char						filename[1024];
	char						feed_filename[1024];     /* file name of the feed storage, or input file name for a stream */

	/* Streams used to output by protocols */
	int						nbInStreams;	/* number of streams which will be output in protocols */
	MuxMediaDescriber			*feedStreams[MUX_SVR_MAX_STREAMS];
	int						streamIndexes[MUX_SVR_MAX_STREAMS];
	LayeredAVStream			*lavStreams[MUX_SVR_MAX_STREAMS];	/* AVCodec for every stream in this feed */

	int						bandwidth;		/* bitRate for all output streams, sum up of feedStreams, kbps */

	/* primitive streams in media file  */
	AVDictionary				*inOpts;        /* input parameters */
	AVFormatContext			*inputCtx;
	AVInputFormat			*inFmt;		/* from 'InputFormat' field. if non NULL, force input format */
	


	struct MuxStream			*myStreams;

	cmn_mutex_t				*connsMutex;		/* used to protect serverConns */
	SERVER_CONNECT			*svrConns;


	int64_t					start_time;            /* In milliseconds - this wraps fairly often */
	int64_t					first_pts;            /* initial pts value */
	int64_t					cur_pts;             /* current pts value from the stream in us */
	int64_t					cur_frame_duration;  /* duration of the current frame in us */
	int						cur_frame_bytes; /* output frame size, needed to compute the time at which we send each packet */
	
	int						pts_stream_index;        /* stream we choose as clock reference */
	int64_t					cur_clock;           /* current clock reference value in us */

	int64_t					lastTime;	/* in us */


#if 0	
	cmn_mutex_t 				*write_frame_mutex; /* for write file protect */
	cmn_cond_t 				*cond;
#endif

	struct _MUX_SVR			*muxSvr;

	struct _MuxFeed			*next;
} MuxFeed;	


typedef struct MuxServerConfig
{
	char						filename[MEDIA_FILENAME_LENGTH];
	

	cmn_list_t				feeds;			/* only used to parse configuration */
	cmn_list_t				streams;

	unsigned int				maxHttpConnections;/* contains all client */
	unsigned int				maxConnections;	/* max media client*/
	uint64_t					maxBandwidth;

	audio_config_t			audioConfig;
	video_config_t			videoConfig;

	/* watchdog info */
	int 						enableWd;
	int						wdTimeout;
	
	log_stru_t				serverLog;
	
//	int						debug;
	int						bitexact;
	
#if 0	
	struct sockaddr_in		httpAddress;
	struct sockaddr_in		rtspAddress;
#else
	ip_config_t				httpAddress;
	ip_config_t				rtspAddress;
#endif

	int 						enableAuthen;  // authentication(both rtsp and http)

	int						fifoSize;
	int						feedDelay;	/* in ms */
	int						sendDelay;	/* in us */

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

	int				noAudio;
	int				noVideo;
	int				noSubtitle;

	int				line_num;
	int				stream_use_defaults;

	struct _MUX_SVR		*muxSvr;
} MuxServerConfig;



typedef struct RTSPActionServerSetup
{
	uint32_t		ipaddr;
	char			transport_option[512];
} RTSPActionServerSetup;


typedef	struct _MUX_SVR
{
	MuxServerConfig			svrConfig;

	MuxFeed					*feeds;		/* feeds which are added here for service */
	
	cmn_list_t				serverConns;
	cmn_mutex_t				*connsMutex;		/* used to protect serverConns */

	char 					programName[128];


	/* maximum number of simultaneous connections */
	
	unsigned int				nbConnections;

	uint64_t					currentBandwidth;	/* unit of KB */

	/* Making this global saves on passing it around everywhere */
	int64_t					currentTime;	/* in ms */

	fd_set					readFds;
	fd_set					writeFds;
	int						maxFd;


	AVLFG					random_state;

	int						frame_rate_change_flag;
	int						frame_rate_change_value;

	int						do_deinterlace;
	int						using_vhook;

	int						do_hex_dump;
	
	int						need_restart;
	
#if WITH_SERVER_WATCHDOG
	int						wdFd;
	int						wdStart;
#endif

	struct sockaddr_in			myAddr;

	int					g_audiobitrate;
	int					g_videobitrate;

}MUX_SVR;


#define	MUX_KILO(bps)	( ((bps)+999)/1000)


#define	GET_BIT(value, bit)				(((value)>>(bit))&0x01)
#define	SET_BIT(value, bit)				((value) << (bit))

#define	FF_SET_BIT(flags, bitPosition)	\
		flags |= SET_BIT(0x01, (bitPosition) ) 

#define	FF_CLEAR_BIT(flags, bitPosition)	\
		flags &= ~(SET_BIT(0x01, (bitPosition) ) )	

#define	FF_CHECK_BIT(flags, bitPosition)	\
		( (flags&SET_BIT(0x01,(bitPosition) ) )!=0 )

#define	FF_SET_FLAGS(flags, value)	\
		flags |= (value) 

#define	FF_CLEAR_FLAGS(flags, value)	\
		flags &= ~((value) ) )	

#define	FF_CHECK_FLAGS(flags, value)	\
		( (flags&(value) )!=0 )


/* this is key frame */
#define	IS_FRAME_KEY(pkt) \
		( (pkt)->flags & AV_PKT_FLAG_KEY)



#define	SVR_BUF_LENGTH(svrBuf)		((svrBuf).bufferEnd -(svrBuf).bufferPtr )

#define	SVR_BUF_RESET(svrBuf)		{ (svrBuf).bufferEnd = (svrBuf).bufferPtr = (svrBuf).dBuffer; }



#define	MUX_ADD_SVR_CONNECTION(muxSvr, svrConn) \
	{	SERVER_CONNS_LOCK( (muxSvr) );\
		cmn_list_append(&(muxSvr)->serverConns, (svrConn));	\
		SERVER_CONNS_UNLOCK((muxSvr));	}


#define	ENABLE_SVR_CONN( feed, svrConn)	\
	{ cmn_mutex_lock( (feed)->connsMutex);	\
	ADD_ELEMENT( (feed)->svrConns, (svrConn));	\
	cmn_mutex_unlock((feed)->connsMutex);}


#define	DISABLE_SVR_CONN(feed, svrConn)	\
	{ cmn_mutex_lock( (feed)->connsMutex);	\
	REMOVE_ELEMENT( (feed)->svrConns, (svrConn), SERVER_CONNECT);	\
	cmn_mutex_unlock((feed)->connsMutex);}


#define	XMIT_CONNECTION_EVENT(conn, event)	\
	(conn)->currentEvent = (event)


#define	CONNECT_CHECK_TIMEOUT(conn)	\
	if ((conn->timeout - conn->server->cur_time) < 0){ \
		XMIT_CONNECTION_EVENT(conn, CONN_EVENT_TIMEOUT); }

#define	CONNECT_IS_RTP(conn)	\
	(conn->is_packetized != 0 )

/* service time of this connection, in unit of us */
#define	CONN_SERVICE_TIME(svrConn)	\
		(((svrConn)->muxSvr->currentTime - (svrConn)->startTime) * 1000)



#define	IS_SERVICE_CONNECT(conn)	\
	(conn->myListen!= NULL)

#define 	IS_SVR_CONN_TYPE(svrConn, _type)		\
		( (svrConn)->type == (_type) )


#define	IS_RTSP_CONNECT(svrConn)	\
			IS_SVR_CONN_TYPE((svrConn), CONN_TYPE_RTSP )

#define	IS_HTTP_CONNECT(svrConn)	\
			IS_SVR_CONN_TYPE((svrConn), CONN_TYPE_HTTP )

#define	IS_MULTICAST_CONNECT(svrConn)	\
			IS_SVR_CONN_TYPE((svrConn), CONN_TYPE_MULTICAST )

#define	IS_RTMP_CONNECT(svrConn)	\
			IS_SVR_CONN_TYPE((svrConn), CONN_TYPE_RTMP )


#define	IS_STREAM_FORMAT( fmt, _name)  \
		( ((fmt) != NULL) && (! strcmp( (fmt)->name, _name)) )


#define	IS_STREAM_TYPE(stream, _type)	\
		( (stream)->type == (_type))

#define	IS_STREAM_MULTICAST(stream)	\
		( IS_STREAM_TYPE((stream), (MUX_STREAM_TYPE_UMC))



#define ADD_ELEMENT(header, element)	\
	if (header == NULL){					\
		header  = element;				\
		element->next   = NULL;			\
	}									\
	else	{								\
		element->next   = header;		\
		header = element;				\
	}


#define REMOVE_ELEMENT(header, element, type)	\
{type **cp, *c1; cp = &header; \
	while ((*cp) != NULL){  c1 = *cp; \
		if (c1 == element ) \
		{ *cp = element->next;} \
		else	{ cp = &c1->next;} \
	}; }



#define	SYSTEM_SIGNAL_EXIT() \
		{ recvSigalTerminal = TRUE; \
		}

#define	SYSTEM_IS_EXITING() \
		(recvSigalTerminal == TRUE)



#define	REPORT_FSM_EVENT(event)	\
	cmnThreadInfoAddEvent( &threadController,  event)


#define	SERVER_CONNS_LOCK( muxSvr) \
	cmn_mutex_lock( (muxSvr)->connsMutex)

#define	SERVER_CONNS_UNLOCK( muxSvr) \
	cmn_mutex_unlock( (muxSvr)->connsMutex)


#define	HTTP_EMIT_EVENT(httpConn, event, errorcode)		\
				muxSvrServiceReportEvent((httpConn), (event), (errorcode), NULL)

typedef void CONNECTION_HANDLER(SERVER_CONNECT *svrConn);




#ifndef __CMN_RELEASE__
#define MUX_SVR_DEBUG(...) \
		{CMN_MSG_DEBUG(CMN_LOG_DEBUG, __VA_ARGS__);}

#define  MUX_SVR_LOG(...)		{CMN_MSG_DEBUG(CMN_LOG_DEBUG, __VA_ARGS__);}

/*
		{muxSvrLog("%s.%s().%d | ", __FILE__, __FUNCTION__, __LINE__); \
			muxSvrLog(__VA_ARGS__);}
*/
#else
#define MUX_SVR_DEBUG(...)  {} 

#define  MUX_SVR_LOG(...)		{muxSvrLog(__VA_ARGS__);}
#endif



#define	VTRACE()	\
		MUX_SVR_DEBUG("")
		



#define CONFIG_ERROR(...) muxCfgError(svrConfig->filename, svrConfig->line_num,\
                                       AV_LOG_ERROR, &svrConfig->errors, __VA_ARGS__)
#define CONFIG_WARNING(...) muxCfgError(svrConfig->filename, svrConfig->line_num,\
                                         AV_LOG_WARNING, &svrConfig->warnings, __VA_ARGS__)

#define MUX_ARRAY_ELEMS(a) 	(sizeof(a) / sizeof((a)[0]))


void muxSvrLog(const char *fmt, ...);

void muxSvrInitLog(MUX_SVR *muxSvr);
void muxCfgError(const char *filename, int line_num, int log_level, int *errors, const char *fmt, ...);


/* SDP handling */
int muxSvrPrepareSdpDescription(MuxStream *stream, uint8_t **pbuffer, struct in_addr my_ip);

/* utils */
size_t htmlencode (const char *src, char **dest);

void muxUtilGetWord(char *buf, int buf_size, const char **pp);

void muxSvrUpdateDatarate(MUX_SVR *muxSvr, DataRateData *drd, int64_t count);
int muxSvrComputeDatarate(MUX_SVR *muxSvr, DataRateData *drd, int64_t count);

void muxSvrMediaCodec(AVStream *st, LayeredAVStream *lst);


void muxSvrBuildFileStreams(MUX_SVR *muxSvr);

int muxSvrServer(MUX_SVR *muxSvr);

const char *muxSvrStatusName(int status);


void muxUtilGetArg(char *buf, int buf_size, const char **pp);
int muxUtilResolveHost(struct in_addr *sin_addr, const char *hostname);

void muxSvrParseAclRow(MuxStream *stream, MuxStream* feed, FFServerIPAddressACL *ext_acl, const char *p, const char *filename, int line_num);

int muxSvrConfigParse(const char *filename, MuxServerConfig *config);
void muxSvrConfigFree(MuxServerConfig *svrConfig);


extern	CMN_THREAD_INFO  	threadFileFeed;
extern	CMN_THREAD_INFO  	threadController;

extern	CMN_THREAD_INFO  	webThread;

extern	SVR_SERVICE_FSM			httpFsm;
extern	SVR_SERVICE_FSM			rtspFsm;

extern AVFrame	dummy_frame;
extern volatile int recvSigalTerminal;


int muxSvrCtrlInit(CMN_THREAD_INFO *th, void *data);

MuxFeed  *muxSvrFoundFeed(int type, char *feedName, MUX_SVR *muxSvr);
MuxStream  *muxSvrFoundStream(int isRtsp, char *feedName, MUX_SVR *muxSvr);

int muxSvrPrepareSdpDescription(MuxStream *stream, uint8_t **sdpDescription, struct in_addr my_ip);

void muxSvrRemoveAutheninfoFromUrl(char* url);

DATA_CONNECT *muxSvrRtpFindConnectWithUrl(char *path, SERVER_CONNECT *rtspConn);
int muxSvrRtpNewOutHandler(DATA_CONNECT *rtpConn, int streamIndex, struct sockaddr_in *destAddr );
DATA_CONNECT *muxSvrRtpNewConnection(SERVER_CONNECT *rtspConn, int streamIndex, RTSPMessageHeader *rtspHeader );

int muxSvrRtspEventErrorInfo(SERVER_CONNECT *rtspConn, int statusCode);

long gettime_ms(void);
char *muxGetCtime1(char *buf2, size_t buf_size);

RTSPTransportField *find_transport(RTSPMessageHeader *h, enum RTSPLowerTransport protocol);

int muxSvrServiceReportEvent(SERVER_CONNECT *svrConn, int event, int  errorCode, void *data );


int	muxSvrServerConnectionIterate( CONNECTION_HANDLER *connHandler, MUX_SVR *muxSvr);

int muxSvrGetHostIP(SERVER_CONNECT *svrConn);


int muxSvrServiceConnectsInit(MUX_SVR *muxSvr);
void muxSvrServiceConnectsDestroy(MUX_SVR *muxSvr);



int	muxSvrConnectDataWriteFd(SERVER_CONNECT *svrConn, uint8_t *buffer, int length );

int muxSvrConnSendPacket(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *_event);

void muxSvrRtpDataConnClose(DATA_CONNECT *dataConn);



int	muxSvrFeedInit(int index, void *ele, void *priv);
int	muxSvrFeedInitFile(MuxFeed *feed);
int	muxSvrFeedInitCapture(MuxFeed *feed);


int svrWebGetPort(void);


#ifdef __cplusplus
}
#endif

#endif

