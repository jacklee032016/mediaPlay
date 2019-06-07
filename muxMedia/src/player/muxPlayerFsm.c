/*
* FSM implementing all callback of HiPlayer
*/

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#if ARCH_ARM
#include "libMuxRx.h"

#include "cmnFsm.h"

static int __playBlackWindow(MUX_PLAY_T *play)
{
	muxPlayerWindowUpdateHandler(play);

	HI_UNF_VO_ResetWindow(play->windowHandle,  (play->muxRx->muxPlayer->playerConfig.keepLastFrame)?HI_UNF_WINDOW_FREEZE_MODE_LAST: HI_UNF_WINDOW_FREEZE_MODE_BLACK);
	if(play->muxRx->muxPlayer->playerConfig.keepLastFrame == 0)
	{
		int ret;
		
		ret= OSD_DESKTOP_LOCK(&play->muxRx->higo);
		if(ret != 0)
		{
			MUX_PLAY_WARN( "Lock for playing image: %s", strerror(errno) );
			return HI_SUCCESS;
		}
		
		muxOsdImageDisplay(play->osd, FALSE);
		
		ret = OSD_DESKTOP_UNLOCK(&play->muxRx->higo);
		if(ret != 0)
		{
			MUX_PLAY_WARN( "Unlocked for playing image: %s", strerror(errno) );
			return HI_SUCCESS;
		}
	}

	return EXIT_SUCCESS;
}


static int __playRestart(MUX_PLAY_T *play)
{
	FSM_OWNER *_fsm = &play->muxFsm;

	CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)_fsm->dataOfCurrentCmd;
	if(!jsonEvent)
	{/* check whether other media is still in this playlist */
		//CMN_ABORT("No pending OPEN command for %s", play->muxFsm.name);
		MUX_PLAY_DEBUG("Continue other items in playlist");
		if(muxPlayerPlaying(play, FALSE, NULL, 0) == 0)
		{
			__playBlackWindow(play);
		}
		return EXIT_SUCCESS;
	}

	/* following is process for the pending JSON command */
	if(_fsm->currentCmd == CMD_TYPE_OPEN)
	{/* for 'play' command when state is still in PLAYING/IMAGE */

		MUX_PLAY_DEBUG("Continue OPEN command in enter into STOP operation");
		char *media = cmnGetStrFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_MEDIA);
		if(media == NULL || !strlen(media))
		{
			cmnMuxJEventReply(jsonEvent, IPCMD_ERR_DATA_ERROR, "No 'media' is defined in command");
			return EXIT_SUCCESS;
		}
		
		int repeatNumber = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_REPEAT);
		if(repeatNumber == -1)
		{
			MUX_PLAY_ERROR(" No repeat number for this play command");
		}

		if(muxPlayerPlaying(play, TRUE, media, repeatNumber) == 0)
		{/* only indicates SetMedia thread is created OK */
			__playBlackWindow(play);
			cmnMuxJEventReply(jsonEvent, IPCMD_ERR_DATA_ERROR, "'media' can't be played");
		}
		else
		{/* no reply for JSON when in playing (SeekFinished), reply should be in event handler of FAIL/TIMEOUT/OK in state of STOP */

//			MUX_PLUGIN_FSM_JSON_REPLY(jsonEvent, IPCMD_ERR_NOERROR);
		}
//		_fsm->currentCmd = CMD_TYPE_UNKNOWN;
//		_fsm->dataOfCurrentCmd = NULL;
	}

	if(_fsm->currentCmd == CMD_TYPE_STOP || _fsm->currentCmd == CMD_TYPE_CLOSE )
	{
		if(!jsonEvent)
		{
			CMN_ABORT("No pending STOP/CLOSE command for %s", play->muxFsm.name);
		}
		MUX_PLUGIN_FSM_JSON_REPLY(jsonEvent, IPCMD_ERR_NOERROR);

		__playBlackWindow(play);

		/* for OPEN command, JSON object keeps */		
		_fsm->currentCmd = CMD_TYPE_UNKNOWN;
		_fsm->dataOfCurrentCmd = NULL;

	}

	return EXIT_SUCCESS;
}

static int _playerEofHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;

#if PLAYER_DEBUG_HIPLAY_EVENT
	MUX_PLAY_DEBUG( "'%s' event: in state of '%s'", muxPlayerEventName(_event->event), muxPlayerStateName( play->muxFsm.currentState) );
#endif

	if(play->playerHandler == HI_SVR_PLAYER_INVALID_HDL)
	{
		MUX_PLAY_WARN("'EOF' event: PlayerHandler error :0x%x", play->playerHandler);
		exit(1);
	}

	return STATE_CONTINUE;
}

static int _playerSeekFinishHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
#if 0
	HI_SVR_PLAYER_EVENT_S *event = (HI_SVR_PLAYER_EVENT_S *)_event;
#endif

#if PLAYER_DEBUG_HIPLAY_EVENT
	MUX_PLAY_DEBUG("'%s' event is handling in state of '%s'", muxPlayerEventName(_event->event) , muxPlayerStateName( play->muxFsm.currentState) );
#endif

	if(PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_STOP) )
	{/* when receives STOP state, send seek command to start next playing */
		__playRestart(play);
	}
	else
	{/* other reseek commands */
	}

	return STATE_CONTINUE;
}

#if PLAYER_ENABLE_INFO_EVENT


static int _playerProgressHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	HI_SVR_PLAYER_EVENT_S *event = (HI_SVR_PLAYER_EVENT_S *)_event->data;
	
#if PLAYER_DEBUG_HIPLAY_EVENT
	MUX_PLAY_DEBUG( "'%s' event is handling in state of '%s'", muxPlayerEventName(_event->event), muxPlayerStateName( play->muxFsm.currentState) );
#endif
	memcpy(&play->progress, event->pu8Data, sizeof(HI_SVR_PLAYER_PROGRESS_S));//event->u32Len);
	if( play->muxRx->noPrintInHdmiATC == HI_FALSE)
	{
		MUX_PLAY_DEBUG("Current progress is %d, Duration:%lld ms,Buffer size:%lld bytes\n", play->progress.u32Progress, play->progress.s64Duration, play->progress.s64BufferSize);
	}
	
	return STATE_CONTINUE;
}

static int _playerFirstFrameTimeHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	HI_SVR_PLAYER_EVENT_S *event = (HI_SVR_PLAYER_EVENT_S *)_event->data;
	unsigned int  firstFrameTime;
	
	firstFrameTime = *((HI_U32*)event->pu8Data);
#if PLAYER_DEBUG_HIPLAY_EVENT
	MUX_PLAY_DEBUG("'%s' event is handling in state of '%s'", muxPlayerEventName(_event->event) , muxPlayerStateName( play->muxFsm.currentState) );
	MUX_PLAY_DEBUG("the first frame time is %d ms of %s", firstFrameTime, play->muxFsm.name);
#endif

	return STATE_CONTINUE;
}
#endif

static int _playerStateChange(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	HI_SVR_PLAYER_EVENT_S *event = (HI_SVR_PLAYER_EVENT_S *)_event->data;

	int newState = (HI_SVR_PLAYER_STATE_E)*event->pu8Data;

	MUX_PLAY_DEBUG("'%s' event, new state is '%s' on %s in state of '%s'", muxPlayerEventName(_event->event), muxPlayerStateName(newState), play->muxFsm.name, muxPlayerStateName( play->muxFsm.currentState) );
	
	if (newState == HI_SVR_PLAYER_STATE_STOP)
	{/* play end, then black screen */
		/* when enter into 'STOP', then close the file; otherwise SetMedia will segment fault */
		/* comments this line, Dec,15th, 2017 */
//		muxPlayerReset(play);
		/* add a new event to start next playing */
		HI_SVR_PLAYER_Seek(play->playerHandler, 0);
	}
	else if (newState == HI_SVR_PLAYER_STATE_PAUSE && play->pausedCtrl == PLAYER_PAUSE_WAITING)
	{
		play->pausedCtrl = PLAYER_PAUSE_PAUSED;
		MUX_PLAY_DEBUG("Have been paused now");
	}
	else
	{
		play->pausedCtrl = PLAYER_PAUSE_NOT_PAUSE;
	}
	
	return newState;
}

/* only 2 states are possible when SOF event */
static int _playerSofHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	HI_SVR_PLAYER_EVENT_S *event = (HI_SVR_PLAYER_EVENT_S *)_event->data;
	
	int newState = (HI_SVR_PLAYER_STATE_E)*event->pu8Data;

#if PLAYER_DEBUG_HIPLAY_EVENT
	/* new state is still PLAYING. Before that state_change message has make state into PLAYING */
	MUX_PLAY_DEBUG( "'%s' event is handling, new state is '%s' in state of '%s'", muxPlayerEventName(_event->event), muxPlayerStateName(newState), muxPlayerStateName( play->muxFsm.currentState) );
#endif

	if (HI_SVR_PLAYER_STATE_BACKWARD == newState)
	{
#if PLAYER_DEBUG_HIPLAY_EVENT
		MUX_PLAY_DEBUG( "backward to start of file, start play! \n");
#endif
		HI_SVR_PLAYER_Play(play->playerHandler);
	}

#if 0
	if(HI_SVR_PLAYER_STATE_PLAY == newState)
	{
	}
#endif

	return newState;
}

static int _playerStreamChangeHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	HI_SVR_PLAYER_EVENT_S *event = (HI_SVR_PLAYER_EVENT_S *)_event->data;
	HI_SVR_PLAYER_STREAMID_S	*streamId = NULL;

#if PLAYER_DEBUG_HIPLAY_EVENT
	MUX_PLAY_DEBUG( "'%s' event is handling in state of '%s'", muxPlayerEventName(_event->event), muxPlayerStateName( play->muxFsm.currentState) );
#endif

	streamId = (HI_SVR_PLAYER_STREAMID_S*)event->pu8Data;
	if (NULL != streamId)
	{
		if (play->playerStreamIDs.u16SubStreamId != streamId->u16SubStreamId ||
			play->playerStreamIDs.u16ProgramId != streamId->u16ProgramId )
		{
			MUX_SUBTITLE_CLEAR_ALL(play);
#if 0			
			play->playerStreamIDs = *streamId;
#else
			memcpy( &play->playerStreamIDs, streamId, event->u32Len);
#endif
		}

#if PLAYER_DEBUG_HIPLAY_EVENT
		MUX_PLAY_DEBUG( "Stream id change to: ProgramId %d, vid %d, aid %d, sid %d \n", streamId->u16ProgramId, streamId->u16VidStreamId,
			streamId->u16AudStreamId, streamId->u16SubStreamId);
#endif
	}
	
	return STATE_CONTINUE;
}

static int _playerDownloadProgressHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	HI_SVR_PLAYER_EVENT_S *event = (HI_SVR_PLAYER_EVENT_S *)_event->data;
	
#if PLAYER_DEBUG_HIPLAY_EVENT
	MUX_PLAY_DEBUG( "'%s' event is handling in state of '%s'", muxPlayerEventName(_event->event), muxPlayerStateName( play->muxFsm.currentState) );
#endif
	HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_GET_BANDWIDTH, &play->downloadBandWidth);

	memcpy(&play->downloadProgress, event->pu8Data, event->u32Len);
	MUX_PLAY_DEBUG( "download progress:%d, duration:%lld ms, buffer size:%lld bytes, bandwidth = %lld\n", 
		play->downloadProgress.u32Progress, play->downloadProgress.s64Duration, play->downloadProgress.s64BufferSize, play->downloadBandWidth );

	if (play->downloadProgress.u32Progress >= 100 && play->pausedCtrl == PLAYER_PAUSE_PAUSED)
	{
		HI_SVR_PLAYER_Resume(play->playerHandler);
	}
	
	return STATE_CONTINUE;
}


static int _playerErrorHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	HI_SVR_PLAYER_EVENT_S *event = (HI_SVR_PLAYER_EVENT_S *)_event->data;
	
	MUX_PLAY_DEBUG( "'%s' event is handling on %s in state of '%s'",muxPlayerEventName(_event->event), play->muxFsm.name, muxPlayerStateName( play->muxFsm.currentState) );
	play->sysError = (HI_S32)*event->pu8Data;

	if (HI_SVR_PLAYER_ERROR_VID_PLAY_FAIL == play->sysError)
	{
		MUX_PLAY_ERROR("%s: VID start fail!", play->muxFsm.name);
	}
	else if (HI_SVR_PLAYER_ERROR_AUD_PLAY_FAIL == play->sysError)
	{
		MUX_PLAY_ERROR("%s: Aud start fail!", play->muxFsm.name);
	}
	else if (HI_SVR_PLAYER_ERROR_PLAY_FAIL == play->sysError)
	{
		MUX_PLAY_ERROR("%s: Play fail!", play->muxFsm.name);
	}
	else if (HI_SVR_PLAYER_ERROR_NOT_SUPPORT == play->sysError)
	{
		MUX_PLAY_ERROR("%s: Format Not support!", play->muxFsm.name);
	}
	else if (HI_SVR_PLAYER_ERROR_TIMEOUT == play->sysError)
	{
		MUX_PLAY_ERROR("%s: HiPlayer Time Out!", play->muxFsm.name);
	}
	else
	{
		MUX_PLAY_ERROR("%s: HiPlayer unknow Error = 0x%x \n", play->muxFsm.name, play->sysError);
	}
	
	return STATE_CONTINUE;
}

static int _playerBufferStateHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	HI_SVR_PLAYER_EVENT_S *event = (HI_SVR_PLAYER_EVENT_S *)_event->data;
	
	HI_SVR_PLAYER_BUFFER_S *pstBufStat = (HI_SVR_PLAYER_BUFFER_S*)event->pu8Data;
#if PLAYER_DEBUG_HIPLAY_EVENT
	MUX_PLAY_DEBUG( "'%s' event is handling in state of '%s'", muxPlayerEventName(_event->event) , muxPlayerStateName( play->muxFsm.currentState) );
	MUX_PLAY_DEBUG( "HI_SVR_PLAYER_EVENT_BUFFER_STATE type:%d, duration:%lld ms, size:%lld bytes\n",
		pstBufStat->eType, pstBufStat->stBufStat.s64Duration, pstBufStat->stBufStat.s64BufferSize);
#endif

	HI_SVR_PLAYER_GetFileInfo(play->playerHandler, &play->fileInfo);

	if (pstBufStat->eType == HI_SVR_PLAYER_BUFFER_EMPTY)
	{
		MUX_PLAY_DEBUG( "### HI_SVR_PLAYER_EVENT_BUFFER_STATE type:HI_SVR_PLAYER_BUFFER_EMPTY, duration:%lld ms, size:%lld bytes\n",
			pstBufStat->stBufStat.s64Duration, pstBufStat->stBufStat.s64BufferSize);
		if ((HI_FORMAT_SOURCE_NET_VOD == play->fileInfo->eSourceType ||
			HI_FORMAT_SOURCE_NET_LIVE == play->fileInfo->eSourceType ) &&
			HI_SVR_PLAYER_STATE_PLAY == play->playerState)
		{
			MUX_PLAY_DEBUG( "Begin to pause");
			play->pausedCtrl = PLAYER_PAUSE_WAITING;
			HI_SVR_PLAYER_Pause(play->playerHandler);
		}
	}
	else if (pstBufStat->eType == HI_SVR_PLAYER_BUFFER_START)
	{
		MUX_PLAY_DEBUG( "### HI_SVR_PLAYER_EVENT_BUFFER_STATE type:HI_SVR_PLAYER_BUFFER_START, duration:%lld ms, size:%lld bytes\n",
			pstBufStat->stBufStat.s64Duration, pstBufStat->stBufStat.s64BufferSize);
		if ((HI_FORMAT_SOURCE_NET_VOD == play->fileInfo->eSourceType ||
			HI_FORMAT_SOURCE_NET_LIVE == play->fileInfo->eSourceType ) &&
			HI_SVR_PLAYER_STATE_PLAY == play->playerState)
		{
			MUX_PLAY_DEBUG("Begin to pause");
			play->pausedCtrl = PLAYER_PAUSE_WAITING;
			HI_SVR_PLAYER_Pause(play->playerHandler);
		}
	}
	else if (pstBufStat->eType == HI_SVR_PLAYER_BUFFER_ENOUGH)
	{
		MUX_PLAY_DEBUG( "### HI_SVR_PLAYER_EVENT_BUFFER_STATE type:HI_SVR_PLAYER_BUFFER_ENOUGH, duration:%lld ms, size:%lld bytes\n",
			pstBufStat->stBufStat.s64Duration, pstBufStat->stBufStat.s64BufferSize);
		if ((HI_FORMAT_SOURCE_NET_VOD == play->fileInfo->eSourceType ||
			HI_FORMAT_SOURCE_NET_LIVE == play->fileInfo->eSourceType ) &&
			PLAYER_PAUSE_PAUSED == play->pausedCtrl)
		{
			MUX_PLAY_INFO( "Begin to resume");
			HI_SVR_PLAYER_Resume(play->playerHandler);
		}
	}
	else if (pstBufStat->eType == HI_SVR_PLAYER_BUFFER_FULL)
	{
		MUX_PLAY_DEBUG( "### HI_SVR_PLAYER_EVENT_BUFFER_STATE type:HI_SVR_PLAYER_BUFFER_FULL, duration:%lld ms, size:%lld bytes\n",
			pstBufStat->stBufStat.s64Duration, pstBufStat->stBufStat.s64BufferSize);
		if ((HI_FORMAT_SOURCE_NET_VOD == play->fileInfo->eSourceType ||
			HI_FORMAT_SOURCE_NET_LIVE == play->fileInfo->eSourceType ) &&
			PLAYER_PAUSE_PAUSED == play->pausedCtrl)
		{
			MUX_PLAY_INFO("Begin to resume");
			HI_SVR_PLAYER_Resume(play->playerHandler);
		}
	}
	
	return STATE_CONTINUE;
}

static int _playerNetworkHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	HI_SVR_PLAYER_EVENT_S *event = (HI_SVR_PLAYER_EVENT_S *)_event->data;
	pthread_t	thread;
	HI_S32 s32Ret = HI_FAILURE;

	HI_FORMAT_NET_STATUS_S *netStat = (HI_FORMAT_NET_STATUS_S*)event->pu8Data;
#if PLAYER_DEBUG_HIPLAY_EVENT
	MUX_PLAY_DEBUG( "'%s' event is handling in state of '%s'", muxPlayerEventName(_event->event) , muxPlayerStateName( play->muxFsm.currentState) );
	MUX_PLAY_DEBUG( "HI_SVR_PLAYER_EVNET_NETWORK_INFO: type:%d, code:%d, str:%s on %s", netStat->eType, netStat->s32ErrorCode, netStat->szProtocals, play->muxFsm.name );
#endif

	if (netStat->eType == HI_FORMAT_MSG_NETWORK_ERROR_UNKNOW && netStat->s32ErrorCode == -111)
	{/* HTTP timeout */
		HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_PRE_CLOSE_FILE, NULL);
	}
	
	if (netStat->eType == HI_FORMAT_MSG_NETWORK_ERROR_DISCONNECT)
	{
		s32Ret = pthread_create(&thread, HI_NULL, muxNetworkReconnect, (HI_VOID*)play);
		if (s32Ret == HI_SUCCESS)
		{
			MUX_PLAY_INFO( "create thread:reconnect successfully\n");
		}
		else
		{
			MUX_PLAY_ERROR("failed to create thread:reconnect\n");
		}
	}
	else if (netStat->eType == HI_FORMAT_MSG_NETWORK_ERROR_CONNECT_FAILED)
	{
		{
			MUX_PLAY_WARN("WARNING: network connection failed on protocol:%s", netStat->szProtocals );
			
			PLAY_ALERT_MSG(play, COLOR_RED,  "network failed on protocol:%s", netStat->szProtocals);
		}
	}

	return STATE_CONTINUE;
}


/* timeout event in playing stream or playing image */
static int _playerTimeoutHandlerInPlaying(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	int ret = 0;
	int newState = STATE_CONTINUE;

	PLAY_ALERT_MSG(play, COLOR_GREEN, "playing '%s' timeout in '%s'", play->currentUrl, muxPlayerStateName( play->muxFsm.currentState) );

	PLAY_LOCK(play);
	play->playTimer = NULL;
	PLAY_UNLOCK(play);

	MUX_PLAY_DEBUG( "'%s' event is handling on '%s' in state of '%s'",muxPlayerEventName(_event->event), play->muxFsm.name, muxPlayerStateName( play->muxFsm.currentState) );

	if(PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE) )
	{
		ret= OSD_DESKTOP_LOCK(&play->muxRx->higo);
		if(ret != 0)
		{
			MUX_PLAY_WARN( "Lock for playing image: %s", strerror(errno) );
			return HI_SUCCESS;
		}
		
		muxOsdImageDisplay(play->osd, FALSE);

		ret = OSD_DESKTOP_UNLOCK(&play->muxRx->higo);
		if(ret != 0)
		{
			MUX_PLAY_WARN( "Unlocked for playing image: %s", strerror(errno) );
			return HI_SUCCESS;
		}


		MUX_PLAY_DEBUG("'%s' will enter STOP state from IMAGE state", play->muxFsm.name );
		/* send SEEK_FINISHED event to start next playing (image/stream) */
		muxPlayerReportFsmEvent(&play->muxFsm, (HI_SVR_PLAYER_EVENT_E)HI_SVR_PLAYER_EVENT_SEEK_FINISHED, 0, NULL);

		newState = HI_SVR_PLAYER_STATE_STOP;
	}
	else
	{
		MUX_PLAY_DEBUG("'%s' will enter STOP state from PLAYING state", play->muxFsm.name );
		muxPlayerStopPlaying(play);

	}

#if 0		
	CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)play->muxFsm.dataOfCurrentCmd;
	if(jsonEvent)
	{
		cmnMuxJEventReply(jsonEvent, IPCMD_ERR_PLUGIN_PLAYER_TIMEOUT,"%s", "Timeout when try to play. Maybe media URL is wrong!");
	}
		
	play->muxFsm.currentCmd = CMD_TYPE_UNKNOWN;
	play->muxFsm.dataOfCurrentCmd = NULL;
#endif

	return newState;
}


/* end event send from JSON play command when it is playing */
static int _playerEndHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	int ret = 0;
	int newState = STATE_CONTINUE;

	PLAY_ALERT_MSG(play, COLOR_RED,  "playing '%s' ended in '%s'", play->currentUrl, muxPlayerStateName( play->muxFsm.currentState) );

	MUX_PLAY_DEBUG( "'%s' event is handling on '%s in state of '%s''",muxPlayerEventName(_event->event), play->muxFsm.name, muxPlayerStateName( play->muxFsm.currentState));

	if(PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE) )
	{
		MUX_PLAY_DEBUG("'%s' send SEEK_FINISHED event in IMAGE state", play->muxFsm.name );
		muxPlayerReportFsmEvent(&play->muxFsm, (HI_SVR_PLAYER_EVENT_E)HI_SVR_PLAYER_EVENT_SEEK_FINISHED, 0, NULL);
		/* when playing image stopped, clear OSD window. Dec.20,2017 */

		ret= OSD_DESKTOP_LOCK(&play->muxRx->higo);
		if(ret != 0)
		{
			MUX_PLAY_WARN( "Lock for playing image: %s", strerror(errno) );
			return HI_SUCCESS;
		}
		
		muxOsdImageDisplay(play->osd, FALSE);

		ret = OSD_DESKTOP_UNLOCK(&play->muxRx->higo);
		if(ret != 0)
		{
			MUX_PLAY_WARN( "Unlocked for playing image: %s", strerror(errno) );
			return HI_SUCCESS;
		}


		newState = HI_SVR_PLAYER_STATE_STOP;
	}
	else
	{
		muxPlayerStopPlaying(play);
	}


	return newState;
}


/* timeout event is come from SetMedia thread, not Callback of HiPlayer */
static int _playerTimeoutHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	int res = 0;
	int newState = STATE_CONTINUE;

	if(PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_STOP)  ||
			PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_INIT) )
	{
#if 1
		/* when setMedia thread is cancelled by timer callback, because timer's callback of setMedia emit FAIL event */
		MUX_PLAY_ERROR( "'%s' event is not handling in state of '%s''",muxPlayerEventName(_event->event), muxPlayerStateName( play->muxFsm.currentState));
#else
		/* when setMedia thread is cancelled by timer callback and there is pending JSON: because timer's callback of setMedia emit timeout event */
		CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)play->muxFsm.dataOfCurrentCmd;
		if(jsonEvent)
		{
			cmnMuxJEventReply(jsonEvent, IPCMD_ERR_PLUGIN_PLAYER_TIMEOUT,"%s", "Timeout when try to play. Maybe media URL is wrong!");
		}
		
		play->muxFsm.currentCmd = CMD_TYPE_UNKNOWN;
		play->muxFsm.dataOfCurrentCmd = NULL;
#endif

		return STATE_CONTINUE;
	}


	PLAY_ALERT_MSG(play, COLOR_RED,  "playing '%s' failed, timeout in '%s'", play->currentUrl, muxPlayerStateName( play->muxFsm.currentState) );

	MUX_PLAY_DEBUG( "'%s' event is handling on '%s in state of '%s''",muxPlayerEventName(_event->event), play->muxFsm.name, muxPlayerStateName( play->muxFsm.currentState));

	if(PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE) )
	{
		MUX_PLAY_DEBUG("'%s' will enter STOP state from IMAGE state", play->muxFsm.name );
		newState = HI_SVR_PLAYER_STATE_STOP;
	}
	else if(PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_PLAY)  )
	{
		muxPlayerStopPlaying(play);
	}

	res = muxPlayerPlaying(play, FALSE, NULL, 0);
	res = res; 

#if 0		
	CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)play->muxFsm.dataOfCurrentCmd;
	if(jsonEvent)
	{
		cmnMuxJEventReply(jsonEvent, IPCMD_ERR_PLUGIN_PLAYER_TIMEOUT,"%s", "Timeout when try to play. Maybe media URL is wrong!");
	}
		
	play->muxFsm.currentCmd = CMD_TYPE_UNKNOWN;
	play->muxFsm.dataOfCurrentCmd = NULL;
#endif

	return newState;
}

/* FAILED event from setMedia() function call or timer's callback of SetMedia thread */
static int _playerFailedHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;
	int res = 0;

//	PLAY_ALERT_MSG(play, COLOR_RED,  "playing '%s' failed, maybe media URL is wrong", play->currentUrl, muxPlayerStateName( play->muxFsm.currentState) );
	PLAY_ALERT_MSG(play, COLOR_RED,  "playing '%s' failed in '%s'", play->currentUrl, muxPlayerStateName( play->muxFsm.currentState) );

	res= OSD_DESKTOP_LOCK(&play->muxRx->higo);
	if(res != 0)
	{
		MUX_PLAY_WARN( "Lock PlayFailed for AlertMsg: %s", strerror(errno) );
		return HI_SUCCESS;
	}
	muxOutputAlert((play)->muxRx, COLOR_RED, ALERT_DEFAULT_LAYOUT, "playing '%s' failed", play->currentUrl);//, muxPlayerStateName( play->muxFsm.currentState));
	res= OSD_DESKTOP_UNLOCK(&play->muxRx->higo);
	if(res != 0)
	{
		MUX_PLAY_WARN( "Unlock PlayFailed for AlertMsg: %s", strerror(errno) );
		return HI_SUCCESS;
	}
	
	MUX_PLAY_DEBUG( "'%s' event is handling on '%s' in state of '%s'",muxPlayerEventName(_event->event), play->muxFsm.name, muxPlayerStateName( play->muxFsm.currentState) );

	CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)play->muxFsm.dataOfCurrentCmd;
	if(jsonEvent)
	{
		cmnMuxJEventReply(jsonEvent, IPCMD_ERR_PLUGIN_PLAYER_FAILED, "%s: playing failed. Maybe media URL is wrong or media is invalidate format!", play->muxFsm.name );
	}
		
	play->muxFsm.currentCmd = CMD_TYPE_UNKNOWN;
	play->muxFsm.dataOfCurrentCmd = NULL;

	cmn_delay((play->muxRx->muxPlayer->playerConfig.timeoutErrorMsg>0)?play->muxRx->muxPlayer->playerConfig.timeoutErrorMsg*1000:1000*10);
	
	res= OSD_DESKTOP_LOCK(&play->muxRx->higo);
	if(res != 0)
	{
		MUX_PLAY_WARN( "Lock PlayFailed for OsdClear: %s", strerror(errno) );
		return HI_SUCCESS;
	}
//	muxOsdClear(play->muxRx->higo.alert);
	muxOutputAlert((play)->muxRx, COLOR_RED, ALERT_DEFAULT_LAYOUT, "");//, muxPlayerStateName( play->muxFsm.currentState));
	res= OSD_DESKTOP_UNLOCK(&play->muxRx->higo);
	if(res != 0)
	{
		MUX_PLAY_WARN( "Unlock PlayFailed for OsdClear: %s", strerror(errno) );
		return HI_SUCCESS;
	}

#if 1
	if(PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_STOP) )
	{/* when receives STOP state, send seek command to start next playing */
		__playRestart(play);
	}
#else
	muxPlayerStopPlaying(play);
#endif

	return STATE_CONTINUE;
}



/* timeout event is come from SetMedia thread, not Callback of HiPlayer */
static int _playerOKHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;

	MUX_PLAY_DEBUG("'%s' event is handling on %s in state of %s", muxPlayerEventName(_event->event), play->muxFsm.name, muxPlayerStateName( play->muxFsm.currentState)  );
	PLAY_ALERT_MSG(play, COLOR_GREEN,  "playing '%s' OK, in '%s'", play->currentUrl, muxPlayerStateName( play->muxFsm.currentState) );

	/* start timer */
	muxPlayerStartPlayingTimer(play);

	if(play->muxRx->muxPlayer->playerConfig.enableLowDelay)
	{
		muxPlayPreloadImage(play);
	}

	CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)play->muxFsm.dataOfCurrentCmd;
	if(jsonEvent)
	{
		MUX_PLUGIN_FSM_JSON_REPLY(jsonEvent, IPCMD_ERR_NOERROR);
	}
		
	play->muxFsm.currentCmd = CMD_TYPE_UNKNOWN;
	play->muxFsm.dataOfCurrentCmd = NULL;

	return STATE_CONTINUE;
}

/* event IMAGE_OK handler */
static int _playerImageOKHandler(void *ctx, EVENT *_event)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)ctx;

	MUX_PLAY_DEBUG("'%s' event is handling on %s in state of %s", muxPlayerEventName(_event->event), play->muxFsm.name, muxPlayerStateName( play->muxFsm.currentState) );
	PLAY_ALERT_MSG(play, COLOR_GREEN,  "playing Image '%s' OK, in '%s'", play->currentUrl, muxPlayerStateName( play->muxFsm.currentState) );

	/* start timer */
	muxPlayerStartPlayingTimer(play);
	if(play->muxRx->muxPlayer->playerConfig.enableLowDelay)
	{
		muxPlayPreloadImage(play);
	}

	CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)play->muxFsm.dataOfCurrentCmd;
	if(jsonEvent)
	{
		MUX_PLUGIN_FSM_JSON_REPLY(jsonEvent, IPCMD_ERR_NOERROR);
	}
		
	play->muxFsm.currentCmd = CMD_TYPE_UNKNOWN;
	play->muxFsm.dataOfCurrentCmd = NULL;

	return HISVR_PLAYER_STATE_IMAGE;
}



static transition_t	playerStateInit[] =
{
	{
		HI_SVR_PLAYER_EVENT_EOF,
		"EOF",
		_playerEofHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_SEEK_FINISHED,
		"SEEK_FINISHED",
		_playerSeekFinishHandler,
	},
#if PLAYER_ENABLE_INFO_EVENT
	{
		HI_SVR_PLAYER_EVENT_PROGRESS,
		"PROGRESS",
		_playerProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_FIRST_FRAME_TIME,
		"FIRST_FRAME_TIME",
		_playerFirstFrameTimeHandler,
	},
#endif

	{
		HI_SVR_PLAYER_EVENT_STATE_CHANGED,
		"STATE_CHANGE",
		_playerStateChange,
	},
	{
		HI_SVR_PLAYER_EVENT_SOF,
		"SOF",
		_playerSofHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_STREAMID_CHANGED,
		"STREAMID_CHANGED",
		_playerStreamChangeHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_ERROR,
		"ERROR",
		_playerErrorHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_DOWNLOAD_PROGRESS,
		"DOWNLOAD_PROGRESS",
		_playerDownloadProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_BUFFER_STATE,
		"BUFFER_STATE",
		_playerBufferStateHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_NETWORK_INFO,
		"NETWORK_INFO",
		_playerNetworkHandler,
	},
	{
		PLAYER_EVENT_TIMEOUT,
		"TIMEOUT",
		_playerTimeoutHandler,
	},
	{
		PLAYER_EVENT_FAIL,
		"FAILED",
		_playerFailedHandler,
	},
	{
		PLAYER_EVENT_OK,
		"OK",
		_playerOKHandler,
	},
	{
		PLAYER_EVENT_IMAGE_OK,
		"IMAGE_OK",
		_playerImageOKHandler,
	},
};


static transition_t	playerStateDeinit[] =
{
#if PLAYER_ENABLE_INFO_EVENT
	{
		HI_SVR_PLAYER_EVENT_FIRST_FRAME_TIME,
		"FIRST_FRAME_TIME",
		_playerFirstFrameTimeHandler,
	},
#endif

	{
		HI_SVR_PLAYER_EVENT_STATE_CHANGED,
		"STATE_CHANGED",
		_playerStateChange,
	},
	{
		HI_SVR_PLAYER_EVENT_ERROR,
		"ERROR",
		_playerErrorHandler,
	},
	{
		PLAYER_EVENT_TIMEOUT,
		"TIMEOUT",
		_playerTimeoutHandler,
	},
	{
		PLAYER_EVENT_FAIL,
		"FAILED",
		_playerFailedHandler,
	},
	{
		PLAYER_EVENT_OK,
		"OK",
		_playerOKHandler,
	},
	{
		PLAYER_EVENT_IMAGE_OK,
		"IMAGE_OK",
		_playerImageOKHandler,
	},
};

void		_playerPlayingStateEnter(void *ownerCtx)
{
	FSM_OWNER *_fsm = (FSM_OWNER *)ownerCtx;
	MUX_PLAY_T *play = (MUX_PLAY_T *)_fsm->ctx;

	if(IS_MAIN_PLAYER(play) )
	{
		cmnMediaCapturerSendNotify(&play->muxRx->muxPlayer->mediaCapture, MUX_CAPTURE_EVENT_START, play);
	}

	if(_fsm->currentCmd != CMD_TYPE_UNKNOWN)
	{
		CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)_fsm->dataOfCurrentCmd;
		if(_fsm->currentCmd == CMD_TYPE_OPEN || _fsm->currentCmd == CMD_TYPE_RESUME )
		{
			MUX_PLUGIN_FSM_JSON_REPLY(jsonEvent, IPCMD_ERR_NOERROR);
		}
		else
		{
			char *cmd = muxPlayerJsonCmdName(_fsm->currentCmd);
			cmnMuxJEventReply(jsonEvent, IPCMD_ERR_NOT_SUPPORT_COMMND, "%s", cmd);
		}
	}

	_fsm->currentCmd = CMD_TYPE_UNKNOWN;
	_fsm->dataOfCurrentCmd = NULL;

	PLAY_ALERT_MSG(play, COLOR_GREEN, "'%s' is playing %s now", _fsm->name, play->currentUrl);
}



static transition_t	playerStatePlaying[] =
{
	{
		HI_SVR_PLAYER_EVENT_EOF,
		"EOF",
		_playerEofHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_SEEK_FINISHED,
		"SEEK_FINISHED",
		_playerSeekFinishHandler,
	},
#if PLAYER_ENABLE_INFO_EVENT
	{
		HI_SVR_PLAYER_EVENT_PROGRESS,
		"PROGRESS",
		_playerProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_FIRST_FRAME_TIME,
		"FIRST_FRAME_TIME",
		_playerFirstFrameTimeHandler,
	},
#endif

	{
		HI_SVR_PLAYER_EVENT_STATE_CHANGED,
		"STATE_CHANGED",
		_playerStateChange,
	},
	{
		HI_SVR_PLAYER_EVENT_SOF,
		"SOF",
		_playerSofHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_STREAMID_CHANGED,
		"STREAMID_CHANGED",
		_playerStreamChangeHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_ERROR,
		"ERROR",
		_playerErrorHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_DOWNLOAD_PROGRESS,
		"DOWNLOAD_PROGRESS",
		_playerDownloadProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_BUFFER_STATE,
		"BUFFER_STATE",
		_playerBufferStateHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_NETWORK_INFO,
		"NETWORK_INFO",
		_playerNetworkHandler,
	},
	{
		PLAYER_EVENT_TIMEOUT,
		"TIMEOUT",
		_playerTimeoutHandlerInPlaying,
	},
	{
		PLAYER_EVENT_FAIL,
		"FAILED",
		_playerFailedHandler,
	},
	{
		PLAYER_EVENT_OK,
		"OK",
		_playerOKHandler,
	},
	{
		PLAYER_EVENT_IMAGE_OK,
		"IMAGE_OK",
		_playerImageOKHandler,
	},
	{
		PLAYER_EVENT_END,
		"END",
		_playerEndHandler,
	},
};

void		_playerForwardStateEnter(void *ownerCtx)
{
	FSM_OWNER *_fsm = (FSM_OWNER *)ownerCtx;
//	MUX_PLAY_T *play = (MUX_PLAY_T *)_fsm->ctx;
	
	if(_fsm->currentCmd != CMD_TYPE_UNKNOWN)
	{
		CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)_fsm->dataOfCurrentCmd;
		if(_fsm->currentCmd == CMD_TYPE_FORWARD)
		{
			MUX_PLUGIN_FSM_JSON_REPLY(jsonEvent, IPCMD_ERR_NOERROR);
		}
		else
		{
			char *cmd = muxPlayerJsonCmdName(_fsm->currentCmd);
			cmnMuxJEventReply(jsonEvent, IPCMD_ERR_NOT_SUPPORT_COMMND, "%s", cmd);
		}
	}
	
	_fsm->currentCmd = CMD_TYPE_UNKNOWN;
	_fsm->dataOfCurrentCmd = NULL;
}


static transition_t	playerStateForward[] =
{
	{
		HI_SVR_PLAYER_EVENT_SEEK_FINISHED,
		"SEEK_FINISHED",
		_playerSeekFinishHandler,
	},
#if PLAYER_ENABLE_INFO_EVENT
	{
		HI_SVR_PLAYER_EVENT_PROGRESS,
		"PROGRESS",
		_playerProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_FIRST_FRAME_TIME,
		"FIRST_FRAME_TIME",
		_playerFirstFrameTimeHandler,
	},
#endif

	{
		HI_SVR_PLAYER_EVENT_STATE_CHANGED,
		"STATE_CHANGED",
		_playerStateChange,
	},
	{
		HI_SVR_PLAYER_EVENT_STREAMID_CHANGED,
		"STREAMID_CHANGED",
		_playerStreamChangeHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_ERROR,
		"ERROR",
		_playerErrorHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_DOWNLOAD_PROGRESS,
		"DOWNLOAD_PROGRESS",
		_playerDownloadProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_BUFFER_STATE,
		"BUFFER_STATE",
		_playerBufferStateHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_NETWORK_INFO,
		"NETWORK_INFO",
		_playerNetworkHandler,
	},
	{
		PLAYER_EVENT_TIMEOUT,
		"TIMEOUT",
		_playerTimeoutHandler,
	},
	{
		PLAYER_EVENT_FAIL,
		"FAILED",
		_playerFailedHandler,
	},
	{
		PLAYER_EVENT_OK,
		"OK",
		_playerOKHandler,
	},
	{
		PLAYER_EVENT_IMAGE_OK,
		"IMAGE_OK",
		_playerImageOKHandler,
	},
};

void		_playerBackwardStateEnter(void *ownerCtx)
{
	FSM_OWNER *_fsm = (FSM_OWNER *)ownerCtx;
//	MUX_PLAY_T *play = (MUX_PLAY_T *)_fsm->ctx;
	
	if(_fsm->currentCmd != CMD_TYPE_UNKNOWN)
	{
		CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)_fsm->dataOfCurrentCmd;
		if(_fsm->currentCmd == CMD_TYPE_BACKWARD)
		{
			MUX_PLUGIN_FSM_JSON_REPLY(jsonEvent, IPCMD_ERR_NOERROR);
		}
		else
		{
			char *cmd = muxPlayerJsonCmdName(_fsm->currentCmd);
			cmnMuxJEventReply(jsonEvent, IPCMD_ERR_NOT_SUPPORT_COMMND, "%s", cmd);
		}
	}
	
	_fsm->currentCmd = CMD_TYPE_UNKNOWN;
	_fsm->dataOfCurrentCmd = NULL;
}


static transition_t	playerStateBackward[] =
{
	{
		HI_SVR_PLAYER_EVENT_SEEK_FINISHED,
		"SEEK_FINISHED",
		_playerSeekFinishHandler,
	},
#if PLAYER_ENABLE_INFO_EVENT

	{
		HI_SVR_PLAYER_EVENT_PROGRESS,
		"PROGRESS",
		_playerProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_FIRST_FRAME_TIME,
		"FIRST_FRAME_TIME",
		_playerFirstFrameTimeHandler,
	},
#endif

	{
		HI_SVR_PLAYER_EVENT_STATE_CHANGED,
		"STATE_CHANGED",
		_playerStateChange,
	},
	{
		HI_SVR_PLAYER_EVENT_SOF,
		"SOF",
		_playerSofHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_STREAMID_CHANGED,
		"STREAMID_CHANGED",
		_playerStreamChangeHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_ERROR,
		"ERROR",
		_playerErrorHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_DOWNLOAD_PROGRESS,
		"DOWNLOAD_PROGRESS",
		_playerDownloadProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_BUFFER_STATE,
		"BUFFER_STATE",
		_playerBufferStateHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_NETWORK_INFO,
		"NETWORK_INFO",
		_playerNetworkHandler,
	},
	{
		PLAYER_EVENT_TIMEOUT,
		"TIMEOUT",
		_playerTimeoutHandler,
	},
	{
		PLAYER_EVENT_FAIL,
		"FAILED",
		_playerFailedHandler,
	},
	{
		PLAYER_EVENT_OK,
		"OK",
		_playerOKHandler,
	},
	{
		PLAYER_EVENT_IMAGE_OK,
		"IMAGE_OK",
		_playerImageOKHandler,
	},
};

void		_playerPauseStateEnter(void *ownerCtx)
{
	FSM_OWNER *_fsm = (FSM_OWNER *)ownerCtx;
	MUX_PLAY_T *play = (MUX_PLAY_T *)_fsm->ctx;
	
	if(IS_MAIN_PLAYER(play) )
	{
		cmnMediaCapturerSendNotify(&play->muxRx->muxPlayer->mediaCapture, MUX_CAPTURE_EVENT_PAUSE, play);
	}

	if(_fsm->currentCmd != CMD_TYPE_UNKNOWN)
	{
		CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)_fsm->dataOfCurrentCmd;
		if(_fsm->currentCmd == CMD_TYPE_PAUSE )
		{
			MUX_PLUGIN_FSM_JSON_REPLY(jsonEvent, IPCMD_ERR_NOERROR);
		}
		else
		{
			char *cmd = muxPlayerJsonCmdName(_fsm->currentCmd);
			cmnMuxJEventReply(jsonEvent, IPCMD_ERR_NOT_SUPPORT_COMMND, "%s", cmd);
		}
	}
	
	_fsm->currentCmd = CMD_TYPE_UNKNOWN;
	_fsm->dataOfCurrentCmd = NULL;
}


static transition_t	playerStatePause[] =
{
	{
		HI_SVR_PLAYER_EVENT_SEEK_FINISHED,
		"SEEK_FINISHED",
		_playerSeekFinishHandler,
	},
#if PLAYER_ENABLE_INFO_EVENT
	{
		HI_SVR_PLAYER_EVENT_PROGRESS,
		"PROGRESS",
		_playerProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_FIRST_FRAME_TIME,
		"FIRST_FRAME_TIME",
		_playerFirstFrameTimeHandler,
	},
#endif

	{
		HI_SVR_PLAYER_EVENT_STATE_CHANGED,
		"STATE_CHANGED",
		_playerStateChange,
	},
	{
		HI_SVR_PLAYER_EVENT_STREAMID_CHANGED,
		"STREAMID_CHANGED",
		_playerStreamChangeHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_ERROR,
		"ERROR",
		_playerErrorHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_DOWNLOAD_PROGRESS,
		"DOWNLOAD_PROGRESS",
		_playerDownloadProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_BUFFER_STATE,
		"BUFFER_STATE",
		_playerBufferStateHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_NETWORK_INFO,
		"NETWORK_INFO",
		_playerNetworkHandler,
	},
	{
		PLAYER_EVENT_TIMEOUT,
		"TIMEOUT",
		_playerTimeoutHandler,
	},
	{
		PLAYER_EVENT_FAIL,
		"FAILED",
		_playerFailedHandler,
	},
	{
		PLAYER_EVENT_OK,
		"OK",
		_playerOKHandler,
	},
	{
		PLAYER_EVENT_IMAGE_OK,
		"IMAGE_OK",
		_playerImageOKHandler,
	},
};

void		_playerStopStateEnter(void *ownerCtx)
{
	FSM_OWNER *_fsm = (FSM_OWNER *)ownerCtx;
	MUX_PLAY_T *play = (MUX_PLAY_T *)_fsm->ctx;

	muxPlayerRemovePlayingTimer(play);

	HI_UNF_VO_ResetWindow(play->windowHandle,  (play->muxRx->muxPlayer->playerConfig.keepLastFrame)?HI_UNF_WINDOW_FREEZE_MODE_LAST: HI_UNF_WINDOW_FREEZE_MODE_BLACK);
	if(IS_MAIN_PLAYER(play) )
	{
		cmnMediaCapturerSendNotify(&play->muxRx->muxPlayer->mediaCapture, MUX_CAPTURE_EVENT_STOP, play);
	}
	
}


static transition_t	playerStateStop[] =
{
	{
		HI_SVR_PLAYER_EVENT_EOF,
		"EOF",
		_playerEofHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_SEEK_FINISHED,
		"SEEK_FINISHED",
		_playerSeekFinishHandler,
	},
#if PLAYER_ENABLE_INFO_EVENT
	{
		HI_SVR_PLAYER_EVENT_PROGRESS,
		"PROGRESS",
		_playerProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_FIRST_FRAME_TIME,
		"FIRST_FRAME_TIME",
		_playerFirstFrameTimeHandler,
	},
#endif

	{
		HI_SVR_PLAYER_EVENT_STATE_CHANGED,
		"STATE_CHANGED",
		_playerStateChange,
	},
	{
		HI_SVR_PLAYER_EVENT_STREAMID_CHANGED,
		"STREAMID_CHANGED",
		_playerStreamChangeHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_ERROR,
		"ERROR",
		_playerErrorHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_DOWNLOAD_PROGRESS,
		"DOWNLOAD_PROGRESS",
		_playerDownloadProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_BUFFER_STATE,
		"BUFFER_STATE",
		_playerBufferStateHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_NETWORK_INFO,
		"NETWORK_INFO",
		_playerNetworkHandler,
	},
	{
		PLAYER_EVENT_TIMEOUT,
		"TIMEOUT",
		_playerTimeoutHandler,
	},
	{
		PLAYER_EVENT_FAIL,
		"FAILED",
		_playerFailedHandler,
	},
	{
		PLAYER_EVENT_OK,
		"OK",
		_playerOKHandler,
	},
	{
		PLAYER_EVENT_IMAGE_OK,
		"IMAGE_OK",
		_playerImageOKHandler,
	},
	{
		PLAYER_EVENT_END,
		"END",
		_playerEndHandler,
	},
};

static transition_t	playerStatePrepare[] =
{
	{
		HI_SVR_PLAYER_EVENT_SEEK_FINISHED,
		"SEEK_FINISHED",
		_playerSeekFinishHandler,
	},
#if PLAYER_ENABLE_INFO_EVENT
	{
		HI_SVR_PLAYER_EVENT_PROGRESS,
		"PROGRESS",
		_playerProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_FIRST_FRAME_TIME,
		"FIRST_FRAME_TIME",
		_playerFirstFrameTimeHandler,
	},
#endif

	{
		HI_SVR_PLAYER_EVENT_STATE_CHANGED,
		"STATE_CHANGED",
		_playerStateChange,
	},
	{
		HI_SVR_PLAYER_EVENT_STREAMID_CHANGED,
		"STREAMID_CHANGED",
		_playerStreamChangeHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_ERROR,
		"ERROR",
		_playerErrorHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_DOWNLOAD_PROGRESS,
		"DOWNLOAD_PROGRESS",
		_playerDownloadProgressHandler,
	},
	{
		HI_SVR_PLAYER_EVENT_NETWORK_INFO,
		"NETWORK_INFO",
		_playerNetworkHandler,
	},
	{
		PLAYER_EVENT_TIMEOUT,
		"TIMEOUT",
		_playerTimeoutHandler,
	},
	{
		PLAYER_EVENT_FAIL,
		"FAILED",
		_playerFailedHandler,
	},
	{
		PLAYER_EVENT_OK,
		"OK",
		_playerOKHandler,
	},
	{
		PLAYER_EVENT_IMAGE_OK,
		"IMAGE_OK",
		_playerImageOKHandler,
	},
};

/* state of play image */
static transition_t	playerStateImage[] =
{
#if PLAYER_ENABLE_INFO_EVENT
#endif

	{
		HI_SVR_PLAYER_EVENT_STATE_CHANGED,
		"STATE_CHANGED",
		_playerStateChange,
	},
	{
		HI_SVR_PLAYER_EVENT_ERROR,
		"ERROR",
		_playerErrorHandler,
	},
	{
		PLAYER_EVENT_TIMEOUT,
		"TIMEOUT",
		_playerTimeoutHandlerInPlaying,
	},
	{
		PLAYER_EVENT_FAIL,
		"FAILED",
		_playerFailedHandler,
	},
	{
		PLAYER_EVENT_OK,
		"OK",
		_playerOKHandler,
	},
	{
		PLAYER_EVENT_END,
		"END",
		_playerEndHandler,
	},
};


static statemachine_t	playerFsmStates[] =
{
	{
		HI_SVR_PLAYER_STATE_INIT,
		"INIT",
		sizeof(playerStateInit)/sizeof(transition_t),
		playerStateInit,
		NULL,
	},
	{
		HI_SVR_PLAYER_STATE_DEINIT,
		"DEINIT",	
		sizeof(playerStateDeinit)/sizeof(transition_t),
		playerStateDeinit,
		NULL,
	},
	{
		HI_SVR_PLAYER_STATE_PLAY,
		"PLAYING",
		sizeof(playerStatePlaying)/sizeof(transition_t),
		playerStatePlaying,
		_playerPlayingStateEnter,
	},
	
	{
		HI_SVR_PLAYER_STATE_FORWARD,
		"FORWARD",	
		sizeof(playerStateForward)/sizeof(transition_t),
		playerStateForward,
		_playerForwardStateEnter,
	},
	{
		HI_SVR_PLAYER_STATE_BACKWARD,
		"BACKWARD",	
		sizeof(playerStateBackward)/sizeof(transition_t),
		playerStateBackward,
		_playerBackwardStateEnter,
	},
	{
		HI_SVR_PLAYER_STATE_PAUSE,
		"PAUSE",	
		sizeof(playerStatePause)/sizeof(transition_t),
		playerStatePause,
		_playerPauseStateEnter,
	},
	
	{
		HI_SVR_PLAYER_STATE_STOP,
		"STOP",	
		sizeof(playerStateStop)/sizeof(transition_t),
		playerStateStop,
		_playerStopStateEnter,
	},
	{
		HI_SVR_PLAYER_STATE_PREPARING,
		"PREPARE",
		sizeof(playerStatePrepare)/sizeof(transition_t),
		playerStatePrepare,
		NULL,
	},
	{
		HISVR_PLAYER_STATE_IMAGE,
		"PLAY_IMAGE",
		sizeof(playerStateImage)/sizeof(transition_t),
		playerStateImage,
		NULL,
	},
};

SERVICE_FSM		playerFsm =
{
	sizeof( playerFsmStates)/sizeof(statemachine_t),
	"HiPlayer.FSM",
	playerFsmStates,
};
#else
SERVICE_FSM		playerFsm =
{
	0,
	"NULL.FSM",
	NULL,
};
#endif

int muxPlayerReportFsmEvent(void *ownerCtx, int eventType, int  errorCode, void *data )
{
	EVENT	*event;

	event = cmn_malloc( sizeof(EVENT) );
	event->event = eventType;
	SET_FSM_EVENT((event->event));
	event->statusCode = errorCode;
	event->data = data;
	event->ownerCtx = ownerCtx;

	cmnThreadAddEvent( &threadPlayer,  event);

	return EXIT_SUCCESS;
}


char *muxPlayerStateName(int state)
{
	statemachine_t *_table = playerFsm.states;
	int i=0;

	for (i=0; i< playerFsm.size; i++)
	{
		if(_table->state == state)
			return _table->name;

		_table++;
	}

	return "Unknown State";
}

