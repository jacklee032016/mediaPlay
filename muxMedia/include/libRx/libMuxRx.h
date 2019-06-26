/*
* encapsulate all Hisilicon-dependent interfaces in this header file which is not referenced by external functions
*/

#ifndef	__LIB_MUX_RX_H__
#define	__LIB_MUX_RX_H__

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/times.h>

#include "hi_audio_codec.h"
#include "hi_unf_common.h"
#include "hi_unf_avplay.h"
#include "hi_unf_demux.h"
#include "hi_unf_so.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_vo.h"
#include "hi_go.h"
#include "hi_adp_mpi.h"
#include "hi_adp_hdmi.h"
#include "hi_svr_player.h"
#include "hi_svr_format.h"
#include "hi_svr_metadata.h"
#include "hi_svr_assa.h"
#include "hi_unf_edid.h"
#if defined (DRM_SUPPORT)
#include "localplay_drm.h"
#endif

#if defined (CHARSET_SUPPORT)
#include "hi_svr_charset.h"
#include "hi_svr_charset_norm.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

//Gaplessplay include

#define	SLEEP_TIME			(10 * 1000)
#define	CMD_LEN			(1024)
#define	PRINTF				printf

#define	MUX_CONTINUE		0
#define	MUX_QUIT			(-1)

#define	RES_HOME_DIR		"/etc/sys"

#define	FONT_NAME_LENGHT			256


//#define sscanf(cmd, fmt, ...)  -1
#define USE_EXTERNAL_AVPLAY   			1


#if (USE_EXTERNAL_AVPLAY == 1)
#define INVALID_TRACK_HDL     (0xffffffff)
#endif


#define	HI_FMT_INVALDAITE		HI_UNF_ENC_FMT_BUTT+1

#define	FMT_AUTO(enFormat)			((enFormat)==HI_UNF_ENC_FMT_BUTT)


#define	RECORD_VID_FILENAME_PATH			"/mnt/vid.h264"
#define	RECORD_AUD_FILENAME_PATH		"/mnt/aud.aac"

#define	RECORD_MKV_FILENAME_PATH		"/media/usb/record01.mkv"
#define	RECORD_VID_FILE_MAX_LEN			(20*1024*1024)

#define	RECORD_FILE_LEN_LIMIT


#define	FONT_GB2312_DOTFILE			RES_HOME_DIR"/fonts/higo_gb2312.ubf"
#define	FONT_GB2312_TTFFILE			RES_HOME_DIR"/fonts/DroidSansFallbackLegacy.ttf"

#define	COLOR_RED			0xffff0000
#define	COLOR_GREEN		0xff00ff00
#define	COLOR_BLUE			0xff0000FF
#define	COLOR_GRAY		0xff7f7f7f


#define	MARGIN_HORIZON				45
#define	MARGIN_VERTIC					25


#define HELP_INFO  \
" ------------- help info ----------------------------------  \n\n " \
"    help     : help info                                          \n " \
"    play  : play                                               \n " \
"    pause : pause                                              \n " \
"    seek  : seek ms, example: seek 90000                       \n " \
"    metadata : print metadata, example: metadata               \n " \
"    posseek  : seek bytes, example: posseek 90000              \n " \
"    stop  : stop                                               \n " \
"    resume: resume                                             \n " \
"    bw    : backward 2/4/8/16/32/64, example: bw 2             \n " \
"    ff    : forward 2/4/8/16/32/64, example: ff 4              \n " \
"    info  : info of player                                     \n " \
"    set   : example:                                           \n " \
"            set mute 1/0                                       \n " \
"            set vmode 0/1/2/3...                               \n " \
"            set sync v_pts_offset a_pts_offset s_pts_offset    \n " \
"            set vol 50                                         \n " \
"            set track 0/1/2/...                                \n " \
"            set pos 20 20 300 300                              \n " \
"            set id prgid vid aid sid, example set id 0 0 1 0   \n " \
"            set hdmi                                           \n " \
"            set spdif                                          \n " \
"    get   : get bandwidth                                      \n " \
"            get bufferconfig size/time                         \n " \
"            get bufferstatus                                   \n " \
"            ......                                             \n " \
"    open  : open new file, example: open /mnt/1.avi            \n " \
"    dbg   : enable dgb, example: dgb 1/0                       \n " \
"    q     : quite                                              \n " \
"    sethls: sethls track 0/1/2...                              \n " \
"    sdr   : set disp narmal mode                               \n " \
"    hdr10 : set disp hdr10 mode                                \n " \
"    dolby : set disp dolby hdr mode                            \n " \
" ----------------------------------------------------------- \n\n " \

typedef	enum
{
	PLAYER_PAUSE_NOT_PAUSE =0,
	PLAYER_PAUSE_WAITING,
	PLAYER_PAUSE_PAUSED,
}PLAYER_PAUSE_T;

typedef	struct
{
	/* video params for virtual window */
	HI_UNF_VCODEC_TYPE_E		videoType;
	HI_UNF_VCODEC_CAP_LEVEL_E	capLevel;

	HI_U32 						width;
	HI_U32						height;

	HI_U32						strmBufSize;
	HI_U32						targetBitRate;
	
	HI_U32						inputFrameRate;
	HI_U32						targetFrameRate;

	/* audio params for virtual track */
	HA_CODEC_ID_E				audioType;
}MUX_OUT_CONFIG_T;

#define		MUX_VIRTUAL_SCREEN_WIDTH			1920
#define		MUX_VIRTUAL_SCREEN_HEIGHT			1080
#define		MUX_CANVAS_WIDTH						MUX_VIRTUAL_SCREEN_WIDTH
#define		MUX_CANVAS_HEIGHT					MUX_VIRTUAL_SCREEN_HEIGHT
#define		MUX_DISPLAY_BUFFER_WIDTH			MUX_VIRTUAL_SCREEN_WIDTH
#define		MUX_DISPLAY_BUFFER_HEIGHT			MUX_VIRTUAL_SCREEN_HEIGHT

typedef	enum
{
	MUX_OSD_TYPE_SUBTITLE = 0,
	MUX_OSD_TYPE_ALERT,
	MUX_OSD_TYPE_LOGO,
	MUX_OSD_TYPE_MAIN_WIN,
	MUX_OSD_TYPE_PIP,
	MUX_OSD_TYPE_UNKNOWN
}MUX_OSD_TYPE;

struct _RECT_CONFIG;

struct MUXLAB_HIGO;

/* one OSD is one window of Higo device */
typedef	struct
{
	char						name[128];
	MUX_OSD_TYPE			type;
	HI_BOOL					enable;

	HI_HANDLE				winHandle;	/* handler of window */
	HI_HANDLE				winSurface;	/* surface(render layer) of this window*/

	HI_HANDLE				desktop;	/* handle of layer which this window belong to. it points to higo->layerHandler */
	struct MUXLAB_HIGO		*higo;	
	
	HI_HANDLE				fontHandle;
	
	HI_RECT					rect;
	struct _RECT_CONFIG		*cfg;

}MUX_OSD;

#define		WITH_HIGO_SURFACE_HANDLER		0

typedef	struct MUXLAB_HIGO
{
	struct _MuxPlayer		*muxPlayer;

	/* mutex and timer used to mutex access of alert/subtitle between callback and player thread ( and its timers) 
	*
	* winMutex is mutex for OSD subsystem which is accessed by PLAYER thread (JSON command, playing image, etc), SetMediax-1 thread (subtitle), and timer callback 
	* Dec.22nd, 2017
	*/
	pthread_mutex_t		winMutex;	
	void 				*winTimer;

	/* graphy layer for all OSD window. Refreshing graphy layer, so every window over this layer will been refreshed/shown */
	HI_HANDLE		layerHandler;
#if WITH_HIGO_SURFACE_HANDLER	
	/* mainly for bitmap subtitle */
	HI_HANDLE		layerSurfaceHandle;
#endif

	cmn_list_t		osdList;

	/* following fields refer to data in list for rapid access */
	MUX_OSD		*subtitle;
	MUX_OSD		*alert;
	MUX_OSD		*logo;
#if 0	
	HI_HANDLE		memSurfaceHandle;
#endif

	HI_RECT			rectSubtitle;
}MUXLAB_HIGO_T;

typedef	enum
{
	SET_MEDIA_STATE_INIT = 0,
	SET_MEDIA_STATE_WAITING,
	SET_MEDIA_STATE_OK,
	SET_MEDIA_STATE_TIMEOUT,
	SET_MEDIA_STATE_UNKNOWN
}SET_MEDIA_STATE;


#define	HISVR_PLAYER_STATE_IMAGE			HI_SVR_PLAYER_STATE_BUTT

#define	PLAYER_EVENT_TIMEOUT		HI_SVR_PLAYER_EVENT_USER_PRIVATE + 10
#define	PLAYER_EVENT_OK			HI_SVR_PLAYER_EVENT_USER_PRIVATE + 11
#define	PLAYER_EVENT_FAIL			HI_SVR_PLAYER_EVENT_USER_PRIVATE + 12	/* playing failed in setMedia */
#define	PLAYER_EVENT_IMAGE_OK	HI_SVR_PLAYER_EVENT_USER_PRIVATE + 13	/* playing image OK */
#define	PLAYER_EVENT_END			HI_SVR_PLAYER_EVENT_USER_PRIVATE + 14	/* playing end from JSON */

struct MUX_RX;

struct _RECT_CONFIG;


/* different from PLAY_LIST, only used in MUX_PLAY_T*/
struct MUX_PLAYLIST
{
	char					name[CMN_NAME_LENGTH];
	int					repeat;
	cmn_list_t			mediaList;	/* local media files or URLs */

	int 					currentIndex;	/* which media in list is playing */
	int					currentRepeat;	/* how many times of repeat the current playing is in */
};


typedef	struct
{
	struct MUX_RX					*muxRx;
	struct _RECT_CONFIG				*cfg;
	MUX_OSD 						*osd;	/* OSD from cfg */

	struct	_FSM_OWNER				muxFsm;

	int								windowIndex;
	int								countOfPlay;		/* count of play, eg. count of start SetMedia Thread, or count of timer for try to play */
	int								countOfTimer;	/* count of timer when playing, eg. count of timer for playing image or playing stream */

	/* speed configuration */
	int								speedCount;		/* count of play speeds */
	int								speedDefault;	/* the index of normal speed */
	int								speedIndex;		/*current play speed, normal is HI_SVR_PLAYER_PLAY_SPEED_NORMAL */

	int								rotateIndex;

	int								volume;

	HI_S32							s_s32ThreadExit;
	HI_S32							s_s32ThreadEnd;
	
	HI_CHAR							currentUrl[HI_FORMAT_MAX_URL_LEN];	/* current media(URL) and its duration */
	int								currentDuration;

	struct MUX_PLAYLIST				currentPlaylist;
	

	/* thread and timer for setMedia() */
	cmn_mutex_t						*mutexLock;


	SET_MEDIA_STATE				mediaState;		/* whether SetMedia thread is waiting */

	/* Media Thread and Media Timer are accessed by PLAYER, media Thread and timer's callback */
#if MUX_THREAD_SUPPORT_DYNAMIC	
	CmnThread						*mediaThread;
#else
	pthread_t						mediaThread;	/* thread used to call setMedia */
#endif
	void 							*mediaTimer;	/* timer for SetMedia thread */

	/* timer for playing image/stream. This timer start in FSM, and handled in timer's callback */
	void 							*playTimer;		

	HI_S64							s_s64CurPgsClearTime;
	HI_BOOL							s_bPgs;

	HI_UNF_ENC_FMT_E				videoFormat;
	
	HI_SVR_PLAYER_MEDIA_S			playerMedia;
	HI_SVR_PLAYER_STREAMID_S		playerStreamIDs;

	HI_U32							hlsStartMode;
	HI_FORMAT_BUFFER_CONFIG_S		bufferConfig;


	HI_HANDLE						playerHandler;
	HI_SVR_PLAYER_STATE_E			playerState;
	HI_S32							pausedCtrl;

	HI_HANDLE						soHandler;
	HI_HANDLE						avPlayHandler;
	
	HI_SO_SUBTITLE_ASS_PARSE_S		*soParseHand;
	HI_SVR_PLAYER_STREAMID_S 		stStreamId;


	HI_HANDLE						trackHandle;
	HI_HANDLE						windowHandle;


	HI_HANDLE						virtualWindow;
	HI_HANDLE						videoEncoder;

	HI_HANDLE						virtualTrack;
	HI_HANDLE						audioEncoder;


	HI_SVR_PLAYER_PROGRESS_S		progress;

	HI_SVR_PLAYER_PROGRESS_S		downloadProgress;
	HI_S64							downloadBandWidth;

	int								sysError;

	HI_FORMAT_FILE_INFO_S			*fileInfo;
	
}MUX_PLAY_T;

struct _MuxPlayer;


typedef	struct
{
	int				size;
	unsigned char		ps[CMN_NAME_LENGTH];	/* parameter set */
}MuxRxEncodePS;	/* Serial and Picture Parameters Set for H.26x */

/* play can be multiple threads for multiple players, so some configure items are global, and some are local for one player */	

/* player runtime for RX board */
typedef	struct MUX_RX
{
	struct _MuxPlayer				*muxPlayer;
	
	MUXLAB_HIGO_T					higo;

	HI_SVR_PLAYER_PARAM_S			playerParam;	/* for all players */
	cmn_list_t						players;

	/* add this lock to make callback of HDMI plugin should wait for everything has been inited. 
	* But it is not the reason HDMI thread is blocked: maybe some configuration(Format, Depth, Color Space)  of HDMI is wrong, so it is blocked:
	* After change the sequences and commands, the block of threads disappeared. JL 06.06, 2019
	*/
	cmn_mutex_t						*initLock;		


	HI_BOOL							isMute;

	
	volatile HI_BOOL					isCapturingStream;
	
	HI_S64							s_s64CurPgsClearTime;
	HI_BOOL							s_bPgs;

	HI_CHAR							aszInputCmd[CMD_LEN];

	HI_BOOL							noPrintInHdmiATC;
	HI_UNF_ENC_FMT_E				videoFormat;


	int								masterIndex;	/* which media is the master track. Actually it is not used. Jun,17, 2017 */
	HI_HANDLE						masterAvPlayer;	/* which is used to connect recorder's track and window */
	HI_HANDLE						masterTrack;	/* only master track can be not mute at any time */
	
	/* handles for record of video/audio */
	HI_HANDLE						virtualWindow;
	HI_HANDLE						videoEncoder;

	/* for Video encoder */
	int								videoFrameCount;
	MuxRxEncodePS					pps;	/* Picture Parameter Set */
	MuxRxEncodePS					sps;		/* sequence Parameter Set */
	
	int								firstVideoPts;
	int								startVideoTime;

	int								firstAudioPts;
	int								startAudioTime;

	/* for audio encoder */
	int								audioLastPts;
	unsigned char						audioBuffer[4096];
	int								audioSize;
	int								audioErrorPts;		/* count the count of packet with wrong pts */


	HI_HANDLE						virtualTrack;
	HI_HANDLE						audioEncoder;

}MUX_RX_T;

/* util functions */
HI_S32 muxParserSubFile(HI_CHAR *pargv, HI_SVR_PLAYER_MEDIA_S *pstMedia);
HI_CHAR *muxGetVidFormatStr(HI_U32 u32Format);
HI_CHAR *muxGetAudFormatStr(HI_U32 u32Format);
HI_CHAR *muxGetSubtitleFormatStr(HI_U32 u32Format);

char *muxHdmiFormatName(HI_UNF_ENC_FMT_E fmt);

int muxHdmiFormatCode(HI_CHAR *pszFmt);

char *muxHdmiHdcpVersion(int version);
char *muxHdmiVideoModeName(int mode);
char *muxHdmiDeepColorName(int color);

int muxHdmiCecCheckStatus(void );

void muxHdmiPrintSinkCap(void);

HI_U32 muxHdmiCECCommand(HI_U8 srcAddr, HI_U8 destAddr, HI_U8 u8Opcode, HI_U8 userCode, HI_U8 *data, HI_U8 Datalength);

int muxHdmiCECSetDeviceOsdName(void);

void muxHdmiReplugMonitor(HI_UNF_ENC_FMT_E enForm);
int muxHdmiConfigDeepColor(HI_UNF_HDMI_DEEP_COLOR_E deepColor, int isConfig);


/****  Higo functions ********/
/* subtitile callback functions */
HI_S32 muxSubtitleOnDrawCallback(HI_VOID * u32UserData, const HI_UNF_SO_SUBTITLE_INFO_S *pstInfo, HI_VOID *pArg);
HI_S32 muxSubtitleOnClearCallback(HI_VOID * u32UserData, HI_VOID *pArg);
HI_S32 muxSubtitleClear(MUX_PLAY_T *play, HI_UNF_SO_CLEAR_PARAM_S *param);

int muxOsdOutputText(MUX_OSD *osd, int align, const HI_CHAR* pszText);
int muxOsdClear(MUX_OSD *osd);
int muxOutputAlert(MUX_RX_T *muxRx, int color, int align, const char* frmt,...);

#define	ALERT_DEFAULT_LAYOUT				(LAYOUT_WRAP|LAYOUT_HCENTER|LAYOUT_VCENTER)


#define	MUX_SUBTITLE_CLEAR_ALL(play)		\
		muxSubtitleClear((play), NULL)

#define	IS_MAIN_PLAYER(player)		\
		((player)->windowIndex==0)

#define	IS_PLAYER_BUSY(player)		\
		((player)->muxFsm.currentCmd != CMD_TYPE_UNKNOWN )


#define	SET_PLAYER_DEFAULT_SPEED(player)		\
		{((player)->speedIndex = (player)->speedDefault ); muxPlayerPlayspeedSet(player, NULL);}



#define	PLAY_ALERT_MSG( play, color, ...)	\
			{	if((play)->muxRx->muxPlayer->playerConfig.enableScreenDebug) 	{ \
					muxOutputAlert((play)->muxRx, (color), ALERT_DEFAULT_LAYOUT, __VA_ARGS__); }	}


#define	PLAY_LOCK(play)			cmn_mutex_lock((play)->mutexLock )

#define	PLAY_UNLOCK(play)		cmn_mutex_unlock((play)->mutexLock )


int muxPlayerJSONError(CMN_PLAY_JSON_EVENT *jsonEvent, int errCode, char *msg);


HI_S32 muxEventCallBack(HI_HANDLE hPlayer, HI_SVR_PLAYER_EVENT_S *pstruEvent);


HI_S32 muxSetVideoMode(MUX_PLAY_T *play, HI_UNF_DISP_3D_E en3D);
HI_S32 muxRxPlayerSetMedia(MUX_PLAY_T *play);
int	muxPlayerPlaying(MUX_PLAY_T *play, int isInit, char *media, int repeatNumber);

int	muxPlayerStartPlayingTimer(MUX_PLAY_T *play);
int	muxPlayerRemovePlayingTimer(MUX_PLAY_T *play);

int	muxPlayPreloadImage(MUX_PLAY_T *play);

int	muxPlayerStopPlaying(MUX_PLAY_T *play);
int muxPlayerReset(MUX_PLAY_T *play);


HI_S32 muxRxInit(MUX_RX_T *muxRx );
void muxRxDeinit(MUX_RX_T *muxRx);


/* functions for recorder */
HI_S32 muxRxCaptureInit(MUX_RX_T *muxRx);
void muxRxCaptureDeinit(MUX_RX_T *muxRx);

int muxRxPlayCaptureVideo(MuxPlayer *muxPlayer);
int muxRxPlayCaptureAudio(MuxPlayer *muxPlayer);
HI_S32 muxRxPlayCaptureStart(MuxPlayer *muxPlayer);
HI_S32 muxRxPlayCaptureStop(MuxPlayer *muxPlayer);


/***  DRM  */

/* get the internal handle */
void* drm_get_handle();

/* clear the internal drm handle */
void drm_clear_handle();
void drm_install();
void drm_cmd(const char* arg);



/**
 * return
 *  0 ok
 *  -1 right invalid
 *  1 acquiring right
 */
int drm_check_right_status(const char* path, const char* mime_type);
int drm_acquire_right_progress(const char* mime_type);

/* M3U9 */
int playM3U9Main(char * path);

HI_VOID* muxNetworkReconnect(HI_VOID *pArg);

MUX_PLAY_T *muxPlayerFindByType(MUX_RX_T *muxRx, enum RECT_TYPE type);
MUX_PLAY_T *muxPlayerFindByIndex(MUX_RX_T *muxRx, int rectIndex);

int muxPlayerSetMainWindowTop(MUX_RX_T *muxRx);

int muxPlayerSwapVideo(MUX_RX_T *muxRx, int newMasterIndex);


int	muxPlayerInitCurrentPlaylist(MUX_PLAY_T *player, char *fileOrPlaylist, int repeatNumber);
int	muxPlayerUpdateCurrentPlaylist(MUX_PLAY_T *player);


//int muxPlayerAddEvent(char *action, cJSON *data, void *dataConn);

/* handle JSON commands of one player */
int muxPlayerJSONHandle(MUX_RX_T *muxRx, CMN_PLAY_JSON_EVENT *jsonEvent);

char *muxPlayerJsonCmdName(int type);

void muxPlayerPlayspeedInit(MUX_PLAY_T *player);
int muxPlayerPlayspeedSet(MUX_PLAY_T *player, CMN_PLAY_JSON_EVENT *jsonEvent);

int muxPlayerWindowRotate(MUX_PLAY_T *player);
int muxPlayerWindowLocate(MUX_PLAY_T *player, HI_RECT_S *rect);
int muxPlayerWindowAspect(MUX_PLAY_T *play, int displayMode);

int muxPlayerWindowUpdateHandler(MUX_PLAY_T *play);

MUX_OSD *muxOsdFind(MUXLAB_HIGO_T *higo, MUX_OSD_TYPE type);

int	muxOsdCreateFont(MUX_OSD	*osd);

int muxOsdLogo(MUX_OSD *logoOsd, char *imageFile);
int muxOsdImageLoad(MUX_OSD *osd, char *imageFile);

int muxOsdImageShow(MUX_OSD *osd, char *imageFile);
int muxOsdImageDisplay(MUX_OSD *osd, int isShow);

int muxOsdPosition(MUX_OSD *osd, HI_RECT_S *rect, MUX_PLAY_T *play);

int muxOsdToggleEnable(MUX_OSD *osd);
int muxOsdSetBackground(MUX_OSD *osd, int backgroundColor);
int muxOsdSetTransparency(MUX_OSD *osd, char alpha);

int muxOsdSetFontSize(MUX_OSD *osd, int fontsize);
int muxOsdSetFontColor(MUX_OSD *osd, int fontcolor);



void muxRxCaptureInitDefault(MUX_RX_T *muxRx);


extern	MUX_RX_T  		_muxRx;

/* auto for Color Depth and Color Space */
#define	_COLOR_DEPETH_SPACE_AUTO	100

/* used when auto is selected */
typedef	struct
{
	HI_UNF_ENC_FMT_E			format;
	
	HI_UNF_HDMI_VIDEO_MODE_E	colorSpace;
	HI_UNF_HDMI_DEEP_COLOR_E	colorDepth;

	HI_UNF_DISP_HDR_TYPE_E		hdrType;
}MUX_HDMI_CFG_T;

int muxHdmiGetAutoConfig(MUX_HDMI_CFG_T *autoCfg);

HI_UNF_HDMI_DEEP_COLOR_E muxHdmiFindNewColorDepth(HI_UNF_HDMI_DEEP_COLOR_E colorDepthCfg, HI_UNF_EDID_DEEP_COLOR_S *sinkCapColorDepthes);
HI_UNF_HDMI_VIDEO_MODE_E muxHdmiFindNewColorSpace(HI_UNF_HDMI_VIDEO_MODE_E colorSpaceCfg, HI_UNF_EDID_COLOR_SPACE_S *sinkCapColorSpace);

int muxHdmiConfigFormat(HI_UNF_ENC_FMT_E enForm, int isConfig);
int muxHdmiConfigColorSpace(HI_UNF_HDMI_VIDEO_MODE_E colorSpace);

extern HI_S32 HI_UNF_HDMI_GetDeepColor(HI_UNF_HDMI_ID_E enHdmi, HI_UNF_HDMI_DEEP_COLOR_E *penDeepColor);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* end header declaration */

