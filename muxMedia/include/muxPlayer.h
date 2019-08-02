
/*
* header file contains all other definiations
*/

#ifndef	__MUX_PLAYER_H__
#define	__MUX_PLAYER_H__

#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>


#ifdef __cplusplus
extern "C" {
#endif


/* we begin to correct av delay at this threshold */


#define	PLAYLIST_UNLIMITED(repeat)	\
							((repeat) == 0)


#define  MUX_PLAY_DEBUG(...)		{CMN_MSG_DEBUG(CMN_LOG_DEBUG, __VA_ARGS__);}


#define  MUX_PLAY_INFO(...)		{CMN_MSG_INFO(CMN_LOG_INFO, __VA_ARGS__);}


#define  MUX_PLAY_WARN(...)		{CMN_MSG_INFO(CMN_LOG_WARNING, __VA_ARGS__);}

#define  MUX_PLAY_ERROR(...)		{CMN_MSG_LOG(CMN_LOG_ERR, __VA_ARGS__);}


//#define	PLAY_DEBUG(play, format, ...)		{CMN_MSG_DEBUG(CMN_LOG_DEBUG, "'%s': "format, play->muxFsm.name, __VA_ARGS__);}
/* this one: no param is needed for format */
#define	PLAY_DEBUG(play, format, msg...)		{CMN_MSG_DEBUG(CMN_LOG_DEBUG, "'%s': "format, play->muxFsm.name, ##msg);}

#define	PLAY_INFO(play, format, msg...)		{CMN_MSG_INFO(CMN_LOG_INFO, "'%s': "format, play->muxFsm.name, ##msg);}

#define	PLAY_WARN(play, format, msg...)		{CMN_MSG_INFO(CMN_LOG_WARNING, "'%s': "format, play->muxFsm.name, ##msg);}

#define	PLAY_ERROR(play, format, msg...)		{CMN_MSG_LOG(CMN_LOG_ERR, "'%s': "format, play->muxFsm.name, ##msg);}


#if WITH_HIGO_DEBUG
#define	OSD_DEBUG(osd, format, msg...)		{CMN_MSG_DEBUG(CMN_LOG_DEBUG, "OSD '%s(%s)' "format, osd->name, osd->cfg->name, ##msg);}
#else
#define	OSD_DEBUG(osd, format, msg...)		{}
#endif

#define	OSD_INFO(osd, format, msg...)		{CMN_MSG_INFO(CMN_LOG_INFO, "OSD '%s(%s)' "format, osd->name, osd->cfg->name, ##msg);}

#define	OSD_WARN(osd, format, msg...)		{CMN_MSG_INFO(CMN_LOG_WARNING, "OSD '%s(%s)' "format, osd->name, osd->cfg->name, ##msg);}

#define	OSD_ERROR(osd, format, msg...)		{CMN_MSG_LOG(CMN_LOG_ERR, "OSD '%s(%s)' "format, osd->name, osd->cfg->name, ##msg);}



struct MUX_RX;

typedef	struct _MuxPlayer
{
	MuxPlayerConfig			playerConfig;
	
	struct MUX_RX			*muxRx;

	MuxMediaCapture			mediaCapture;

	int						samplesPerFrame;	/* audio */
	double					lastVideoSysTime;
	int						lastVideoPts;	/* save PTS from decoder */
	int						lastAudioPts;	/* save PTS from decoder */


	void						*priv;	/* MuxMain */
}MuxPlayer;


extern	CmnThread  	threadPlayer;


//int muxSendDataToCgi(CmnThread *th, CMD_EVENT_T *rawEvent,  TLV_BUFFER_T *tlvBuf);

char *muxPlayerStateName(int state);

extern	TYPE_NAME_T hiPlayerEvents[];
extern	TYPE_NAME_T hiPlayerStates[];

#define	muxPlayerEventName( event) \
		cmnMuxTypeFindName(hiPlayerEvents, (event))


#define	muxHiPlayerStateName( state) \
		cmnMuxTypeFindName(hiPlayerStates, (state))


/* lock for OSD desktop */
#define	OSD_DESKTOP_LOCK(higo)	\
	pthread_mutex_lock(&(higo)->winMutex)

#define	OSD_DESKTOP_TRY_LOCK(higo)	\
	pthread_mutex_trylock(&(higo)->winMutex)

#define	OSD_DESKTOP_UNLOCK(higo) \
	pthread_mutex_unlock(&(higo)->winMutex)


#define	PLAYER_CHECK_STATE( play,  state)	\
			( (play)->muxFsm.currentState == (state))

/* put command in FSM, the command will be driven by async event of FSM */
#define	PLAYER_SET_CURRENT_CMD( play,  cmd, data)	\
			{ (play)->muxFsm.currentCmd = (cmd); \
			(play)->muxFsm.dataOfCurrentCmd = (data); (data)->status = IPCMD_ERR_IN_PROCESSING; }


RECT_CONFIG *muxGetRectConfig(MuxPlayer *muxPlayer, enum RECT_TYPE type);

int muxPlayerReportFsmEvent(void *ownerCtx, int event, int  errorCode, void *data );


#define	SEND_EVEVT_TO_PLAYER( play, event, data)		\
		muxPlayerReportFsmEvent(&(play)->muxFsm, (HI_SVR_PLAYER_EVENT_E)(event), (play)->countOfPlay, (data))


#include "cmnFsm.h"



#ifdef __cplusplus
}
#endif

#endif

