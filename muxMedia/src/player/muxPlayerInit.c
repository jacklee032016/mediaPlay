/*
* $Id$
*/

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/resource.h>
#include <signal.h>

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"

#if ARCH_X86
#else
#include "libMuxRx.h"
#endif


MuxPlayer			_muxPlayer;

RECT_CONFIG *muxGetRectConfig(MuxPlayer *muxPlayer, enum RECT_TYPE type)
{
	RECT_CONFIG *rect = NULL;
	int i;
	cmn_list_t *list = &muxPlayer->playerConfig.osds;

	if(type == RECT_TYPE_MAIN || type == RECT_TYPE_PIP)
	{
		list = &muxPlayer->playerConfig.windows;
	}

	for(i=0; i< cmn_list_size( list); i++ ) 
	{
		rect = (RECT_CONFIG *)cmn_list_get(list, i);
		if( rect == NULL)
		{
			MUX_PLAY_WARN( "No %d rect config is NULL" , i);
			exit(1);
		}

		if(rect->type == type)
			return rect;
	}

	MUX_PLAY_ERROR( "Can't found RECT for '%d'", type );
	return NULL;
}

static int _captureMain(CmnThread *th)
{
	MuxPlayer 	*muxPlayer = (MuxPlayer *)th->data;
	int res;
	
	cmnThreadMask(th->name);
	
	res = muxRxPlayCaptureVideo(muxPlayer);
#if 0
	if (SYSTEM_IS_EXITING())
	{
		MUX_PLAY_WARN( "'%s' Thread recv EXIT signal\n", th->name );
		return -EXIT_FAILURE;
	}
#endif
	return res;
}

static int _initCapture2(CmnThread *th, void *data)
{
//	MuxPlayer *muxPlayer =(MuxPlayer *)data;
//	int res = muxRxPlayCaptureStart(muxPlayer);


	th->data = data;	
	return EXIT_SUCCESS;
}


static int _captureMain2(CmnThread *th)
{
	MuxPlayer 	*muxPlayer = (MuxPlayer *)th->data;
	int res;

	cmnThreadMask(th->name);
	
	res = muxRxPlayCaptureAudio(muxPlayer);
#if 0
	if (SYSTEM_IS_EXITING())
	{
		MUX_PLAY_WARN( "'%s' Thread recv EXIT signal\n", th->name );
		return -EXIT_FAILURE;
	}
#endif
	return res;
}


/*  */
static int _initCapture(CmnThread *th, void *data)
{
	MuxPlayer *muxPlayer =(MuxPlayer *)data;

	int res = muxRxPlayCaptureStart(muxPlayer);

	th->data = data;	
	return res;//EXIT_SUCCESS;
}

static void _destoryCapture(struct _CmnThread *th)
{
	MuxPlayer 	*muxPlayer = (MuxPlayer *)th->data;

	muxRxPlayCaptureStop(muxPlayer);
}


CmnThread  threadRxCapture =
{
	name		:	"VideoCap",
		
	init			:	_initCapture,
	mainLoop		:	_captureMain,
	eventHandler	:	NULL,
	destory		:	_destoryCapture,
	
	data			:	NULL,
};

CmnThread  threadRxCapture2 =
{
	name		:	"AudioCap",
		
	init			:	_initCapture2,
	mainLoop		:	_captureMain2,
	eventHandler	:	NULL,
	destory		:	NULL,
	
	data			:	NULL,
};


static int	_playerEventHandler(struct _CmnThread *th, void *_event)
{
	MuxPlayer *muxPlayer =(MuxPlayer *)th->data;

	CMD_EVENT_T *cgiEvent = (CMD_EVENT_T *)_event;
	int res = EXIT_SUCCESS;

//	MUX_PLAY_DEBUG( "Player '%s' receiving event", th->name);

	if(CHECK_FSM_EVENT( cgiEvent->type))
	{/* FSM events come from HiPlayer */
		EVENT	*event = (EVENT *)_event;
		
		FSM_OWNER *fsm = (FSM_OWNER *)event->ownerCtx;
		MUX_PLAY_T *play = (MUX_PLAY_T *)fsm->ctx;
#ifndef   __CMN_RELEASE__
		int oldState = play->muxFsm.currentState;
#endif
		CLAER_FSM_EVENT_FLAGS(event->event);

		res = cmnFsmHandle( play, event);
		if (res < 0)
		{
			MUX_PLAY_ERROR( "'%s' on '%s' Thread FSM Failed\n", play->muxFsm.name, th->name );
		}

#if ARCH_ARM
		MUX_PLAY_DEBUG( "FSM Event '%s' has been processed on '%s' in state of '%s', enter into '%s'", muxPlayerEventName(event->event), play->muxFsm.name, 
			muxPlayerStateName( oldState), muxPlayerStateName( play->muxFsm.currentState) );
#endif
		if(event->data)
		{
#if PLAYER_DEBUG_HIPLAY_EVENT
			MUX_PLAY_DEBUG( "FSM Event '%s' free HiPlayer Event", muxPlayerEventName(event->event) );
#endif
			HI_SVR_PLAYER_EVENT_S *playerEvent = (HI_SVR_PLAYER_EVENT_S *)event->data;

			if(playerEvent->pu8Data)
			{
#if PLAYER_DEBUG_HIPLAY_EVENT
				MUX_PLAY_DEBUG( "FSM Event '%s' free data of PlayerEvent : %p", muxPlayerEventName(event->event), playerEvent->pu8Data );
#endif
				cmn_free(playerEvent->pu8Data);
			}
			
#if PLAYER_DEBUG_HIPLAY_EVENT
			MUX_PLAY_DEBUG( "'%s' free HiPlayer Event: %p", muxPlayerEventName(event->event),  playerEvent);
#endif
			cmn_free(playerEvent);
		}
		
#if PLAYER_DEBUG_HIPLAY_EVENT
		MUX_PLAY_DEBUG( "Player '%s' free FSM event :%p", play->muxFsm.name, event);
#endif
		cmn_free(event);
//		TRACE();
	}
	else
	{/* JSON command come from Controller */

//		MUX_PLAY_DEBUG( "Player '%s' receiving JSON event", th->name);
		muxPlayerJSONHandle(muxPlayer->muxRx, (CMN_PLAY_JSON_EVENT *)_event);
	}
#if 0
	if (SYSTEM_IS_EXITING())
	{
		MUX_PLAY_WARN("'%s' Thread recv EXIT signal\n", th->name );
		return -EXIT_FAILURE;
	}
#endif
	if(res < 0)
	{
		MUX_PLAY_WARN("'%s' Task exit now!", th->name );
		exit(1);
	}
	
//	TRACE();
	return res;
}


static int _playerInitThread(CmnThread *th, void *data)
{
	int res = 0;

	MuxPlayer *muxPlayer = (MuxPlayer	*)data;

	res = muxRxInit( muxPlayer->muxRx );

#if 0
	sigset_t set;

	/* Block SIGQUIT and SIGUSR1; other threads created by main()
	will inherit a copy of the signal mask. */
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	res = pthread_sigmask(SIG_BLOCK, &set, NULL);
	if (res != 0)
	{
		MUX_PLAY_ERROR( "Signal Block for thread failed :%s", th->name);
		exit(1);
	}
#else
	/* mask in the context of main thread, not player thread 
	* after this call, all threads will mask this signal
	*/
//	cmnThreadMask(th->name);
#endif

	th->data = data;

	cmnThreadSetName("main");

//	res =res;
	return res; /* PLAYER thread must run no matter what happened */
}


CmnThread  threadPlayer =
{
	name		:	"PLAYER",
	flags		:	SET_BIT(1, CMN_THREAD_FLAG_WAIT),
		
	init			:	_playerInitThread,

	/* minimize the CPU resource of this thread */
	mainLoop		:	NULL,
	eventHandler	:	_playerEventHandler,

	destory		:	NULL,
	data			:	NULL,
};

MUX_RX_T  		_muxRx;

MUX_PLAY_T *muxPlayerFindByHandle(HI_HANDLE hPlayer)
{
	MUX_RX_T *muxRx = (MUX_RX_T *)_muxPlayer.muxRx;
	MUX_PLAY_T *play = NULL;
	int i;

	for(i=0; i< cmn_list_size( &muxRx->players); i++)
	{
		play = (MUX_PLAY_T *)cmn_list_get(&muxRx->players, i);

		if(play->playerHandler == hPlayer)
		{
			return play;
		}
	}

	return NULL;
}

MUX_PLAY_T *muxPlayerFindByType(MUX_RX_T *muxRx, enum RECT_TYPE type)
{
	MUX_PLAY_T *play = NULL;
	int i;

	for(i=0; i< cmn_list_size( &muxRx->players); i++)
	{
		play = (MUX_PLAY_T *)cmn_list_get(&muxRx->players, i);

		if(play->cfg->type == type)
		{
			return play;
		}
	}

	return NULL;
}

MUX_PLAY_T *muxPlayerFindByIndex(MUX_RX_T *muxRx, int rectIndex)
{
	MUX_PLAY_T *play = NULL;
	RECT_CONFIG *rect = NULL;
	int i;

	rect = (RECT_CONFIG *)cmn_list_get(&muxRx->muxPlayer->playerConfig.windows, rectIndex);
	if(rect == NULL)
		return NULL;

	for(i=0; i< cmn_list_size( &muxRx->players); i++)
	{
		play = (MUX_PLAY_T *)cmn_list_get(&muxRx->players, i);

		if(play->cfg == rect )
		{
			return play;
		}
	}

	return NULL;
}

/* it must be run before pthread_join in muxMain */
int	_muxRxPlayStartCapture(struct MuxMediaCapture *mediaCapture)
{
	MuxPlayer	*muxPlayer = (MuxPlayer *)mediaCapture->priv;
	int	ret = EXIT_FAILURE;
	
	if(mediaCapture->nbConsumers )
	{
		MuxMain *muxMain = (MuxMain *)muxPlayer->priv;

		ret = muxMain->initThread(muxMain, &threadRxCapture, muxPlayer);
	//	usleep(10 * 1000);
		ret += muxMain->initThread(muxMain, &threadRxCapture2, muxPlayer);
	}
	else
	{
		MUX_PLAY_WARN( "No consumer is registered, so Capturer '%s' is not run", mediaCapture->name );
		ret = EXIT_SUCCESS;
	}

	if(ret > 0)
		return EXIT_SUCCESS;
		
	return  EXIT_FAILURE;
}

int	_muxRxPlayGetCaptureStatus(struct MuxMediaCapture *mediaCapture)
{
	MuxPlayer	*muxPlayer = (MuxPlayer *)mediaCapture->priv;
	int	ret = EXIT_FAILURE;

	MUX_PLAY_T *play = muxPlayerFindByType(muxPlayer->muxRx, RECT_TYPE_MAIN);
	if(!play)	
	{
		MUX_PLAY_WARN( "No MAIN player is found, so Capturer '%s' can not work", mediaCapture->name );
		return MUX_CAPTURE_STATE_UNKNOWN;
	}

	if( play->muxFsm.currentState == HI_SVR_PLAYER_STATE_PLAY)
	{
		MUX_PLAY_DEBUG("RUNNING state");
		return MUX_CAPTURE_STATE_RUNNING;
	}
	else if( play->muxFsm.currentState == HI_SVR_PLAYER_STATE_STOP)
	{
		MUX_PLAY_DEBUG("STOPPED state");
		return MUX_CAPTURE_STATE_STOPPED;
	}
	else if(play->muxFsm.currentState == HI_SVR_PLAYER_STATE_PAUSE)
	{
		MUX_PLAY_DEBUG("PAUSE state");
		return MUX_CAPTURE_STATE_PAUSED;
	}
	else// if(play->muxFsm.currentState == HI_SVR_PLAYER_STATE_PLAY)
	{
		MUX_PLAY_DEBUG("INITTED state");
		return MUX_CAPTURE_STATE_INITTED;
	}
	
	return  ret;
}


static int	_muxPlayerSignalExit(struct _MuxPlugin *plugin, int signal)
{
	return EXIT_SUCCESS;
}


static int _muxPlayerReportEvent(struct _MuxPlugin *plugin, CMN_PLAY_JSON_EVENT *jEvent)
{
	MUX_PLAY_DEBUG( "Player receiving JSON event: '%s'", jEvent->action);

	cmnThreadAddEvent( &threadPlayer,  jEvent);

	return EXIT_SUCCESS;
}

static void _muxPlayerDestory(MuxPlugIn *plugin)
{
	MuxPlayer *muxPlayer = (MuxPlayer *)plugin->priv;

	muxRxDeinit( muxPlayer->muxRx );
}


int init(MuxPlugIn *plugin)
{
//	struct sigaction sigact;
	MuxPlayer		*muxPlayer = &_muxPlayer;
	MUX_RX_T		*muxRx = &_muxRx;
	MuxPlayerConfig 	*playerConfig;
	int res = EXIT_SUCCESS;

	memset(muxRx, 0 , sizeof(MUX_RX_T) );
	memset(muxPlayer, 0 , sizeof(MuxPlayer) );
	
	muxPlayer->muxRx = muxRx;
	muxRx->muxPlayer = muxPlayer;

	playerConfig = &muxPlayer->playerConfig;
	playerConfig->parent = muxPlayer;

	plugin->priv = muxPlayer;
	plugin->type = MUX_PLUGIN_TYPE_PLAYER;
	plugin->signalExit = _muxPlayerSignalExit;
	plugin->reportEvent = _muxPlayerReportEvent;
	plugin->destory = _muxPlayerDestory;
	snprintf(plugin->name, sizeof(plugin->name), "%s", CMN_MODULE_PLAYER_NAME );
	snprintf(plugin->version, sizeof(plugin->version), "%s", CMN_VERSION_INFO(CMN_MODULE_PLAYER_NAME) );
	muxPlayer->priv = plugin->muxMain;
	
	srandom(cmnGetTimeMs() + (getpid() << 16));

	if( cmnMuxPlayerParseConfig(MUX_PLAYER_CONFIG_FILE, playerConfig ) )
	{
		fprintf(stderr, "Incorrect config file : %s - exiting.\n", MUX_PLAYER_CONFIG_FILE);
		exit(1);
	}

	/* initialize and add media capture into muxMain */
	snprintf(muxPlayer->mediaCapture.name, sizeof(muxPlayer->mediaCapture.name), "%s", playerConfig->captureName);
	muxPlayer->mediaCapture.priv = muxPlayer;
	muxPlayer->mediaCapture.masterClock = plugin->muxMain->mediaCaptureConfig.avSyncType;
	
	muxPlayer->mediaCapture.startCapture = _muxRxPlayStartCapture;
	muxPlayer->mediaCapture.getCaptureState = _muxRxPlayGetCaptureStatus;
	muxPlayer->mediaCapture.createMediaDescribers = cmnMediaCreateDescribersForPlayer;
	
	plugin->muxMain->addCapture(plugin->muxMain, &muxPlayer->mediaCapture);
	
	res = plugin->muxMain->initThread(plugin->muxMain, &threadPlayer, muxPlayer);
//	usleep(10*1000);
	if(res < 0 )
	{
		MUX_PLAY_ERROR("failed when PLAYER initializing");
		exit(1);
	}

	/* the last step of RxInit is initialize all play(setMedia), so the event will emitted and added to threadPlayer in this last step. ThreadPlayer created first.
	* it should be enhanced to start thread after device inited. 10.09.2017 
	*/

	
#if 0

#ifdef JASON_RM_BENCHMARK
	ti = av_gettime();
#endif

#ifdef JASON_RM_BENCHMARK
	ti = av_gettime() - ti;
	if (do_benchmark)
	{
		printf("bench: utime=%0.3fs\n", ti / 1000000.0);
	}
#endif
#endif

	MUX_PLAY_INFO( CMN_MODULE_PLAYER_NAME" initializing OK!\n" );
	
	return EXIT_SUCCESS;
}

