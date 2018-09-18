
#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"



static HI_S32 _getClosely3DDisplayFormat(HI_HANDLE hPlayer, HI_UNF_DISP_3D_E en3D,
	HI_UNF_ENC_FMT_E *pen3DEncodingFormat, HI_UNF_VIDEO_FRAME_PACKING_TYPE_E  *penFrameType )
{
	HI_S32 s32Ret = HI_SUCCESS;
	HI_FORMAT_FILE_INFO_S *pstFileInfo = NULL;
	HI_FORMAT_VID_INFO_S *pstVidStream = NULL;
	HI_SVR_PLAYER_STREAMID_S stStreamId;
	HI_U16 u16Fps;

	if (pen3DEncodingFormat == NULL ||
		penFrameType == NULL ||
		HI_SVR_PLAYER_GetParam(hPlayer, HI_SVR_PLAYER_ATTR_STREAMID, (HI_VOID*)&stStreamId) != HI_SUCCESS ||
		HI_SVR_PLAYER_GetFileInfo(hPlayer, &pstFileInfo) != HI_SUCCESS ||
		pstFileInfo->u32ProgramNum <= 0 ||
		pstFileInfo->pastProgramInfo[stStreamId.u16ProgramId].u32VidStreamNum <= 0)
	{
		return HI_FAILURE;
	}

	pstVidStream = &pstFileInfo->pastProgramInfo[stStreamId.u16ProgramId].pastVidStream[stStreamId.u16VidStreamId];
	u16Fps = pstVidStream->u16FpsInteger;

	switch(en3D)
	{
		case HI_UNF_DISP_3D_FRAME_PACKING:
			if (u16Fps <= 50)
			{
				*pen3DEncodingFormat = HI_UNF_ENC_FMT_1080P_24_FRAME_PACKING;
			}
			else if (u16Fps <= 60)
			{
				*pen3DEncodingFormat = HI_UNF_ENC_FMT_720P_50_FRAME_PACKING;
			}
			else
			{
				*pen3DEncodingFormat = HI_UNF_ENC_FMT_720P_60_FRAME_PACKING;
			}
			*penFrameType = HI_UNF_FRAME_PACKING_TYPE_TIME_INTERLACED;
			break;

		case HI_UNF_DISP_3D_SIDE_BY_SIDE_HALF:
			if (u16Fps <= 60)
			{
				*pen3DEncodingFormat = HI_UNF_ENC_FMT_1080i_50;
			}
			else
			{
				*pen3DEncodingFormat = HI_UNF_ENC_FMT_1080i_60;
			}
			*penFrameType = HI_UNF_FRAME_PACKING_TYPE_SIDE_BY_SIDE;
			break;

		case HI_UNF_DISP_3D_TOP_AND_BOTTOM:
			if (u16Fps <= 50)
			{
				*pen3DEncodingFormat = HI_UNF_ENC_FMT_1080P_24;
			}
			else if (u16Fps <= 60)
			{
				*pen3DEncodingFormat = HI_UNF_ENC_FMT_720P_50;
			}
			else
			{
				*pen3DEncodingFormat = HI_UNF_ENC_FMT_720P_60;
			}
			
			*penFrameType = HI_UNF_FRAME_PACKING_TYPE_TOP_AND_BOTTOM;
			break;

		case HI_UNF_DISP_3D_SIDE_BY_SIDE_FULL:
		default:
			*pen3DEncodingFormat = HI_UNF_ENC_FMT_BUTT;
			*penFrameType = HI_UNF_FRAME_PACKING_TYPE_BUTT;
			s32Ret = HI_FAILURE;
			MUX_PLAY_WARN( "3D Mode %d not supported now\n", en3D);
	}

	return s32Ret;
}

/* used in commands and initialization of HiPlayer */
HI_S32 muxSetVideoMode(MUX_PLAY_T *play, HI_UNF_DISP_3D_E en3D)
{
	HI_S32 s32Ret, s32Ret1;
	HI_UNF_DISP_3D_E enCur3D;
	HI_UNF_ENC_FMT_E enCurEncFormat, enEncFormat = HI_UNF_ENC_FMT_720P_50;
	HI_UNF_VIDEO_FRAME_PACKING_TYPE_E enFrameType = HI_UNF_FRAME_PACKING_TYPE_NONE;
	HI_UNF_EDID_BASE_INFO_S stSinkAttr;
	HI_BOOL bSetMode = HI_FALSE;
	HI_HANDLE hAVPlay = HI_SVR_PLAYER_INVALID_HDL;

	s32Ret = HI_UNF_DISP_Get3DMode(play->muxRx->playerParam.u32Display, &enCur3D, &enCurEncFormat);
	if (s32Ret == HI_SUCCESS && enCur3D == en3D)
	{
		MUX_PLAY_WARN("Already in Mode:%d!", en3D);
		return HI_SUCCESS;
	}

	
	if (en3D == HI_UNF_DISP_3D_NONE)
	{/*switch to 2D Mode*/
		enEncFormat = HI_UNF_ENC_FMT_720P_50;
		bSetMode = HI_TRUE;
		enFrameType = HI_UNF_FRAME_PACKING_TYPE_NONE;
	}
	else
	{/*switch to 3D Mode*/
		s32Ret = HI_UNF_HDMI_GetSinkCapability(HI_UNF_HDMI_ID_0, &stSinkAttr);
		if (s32Ret == HI_SUCCESS && stSinkAttr.st3DInfo.bSupport3D == HI_TRUE)
		{
			if (_getClosely3DDisplayFormat(play->playerHandler, en3D, &enEncFormat, &enFrameType) == HI_SUCCESS)
			{
			bSetMode = HI_TRUE;
			}
		}
		else if (s32Ret != HI_SUCCESS)
		{
			MUX_PLAY_ERROR( "Get HDMI sink capability ret error:%#x", s32Ret);
		}
		else
		{
			MUX_PLAY_WARN("HDMI Sink video mode not supported.\n");
		}
	}

	if (bSetMode == HI_TRUE)
	{
		s32Ret1 = HI_UNF_HDMI_SetAVMute(HI_UNF_HDMI_ID_0, HI_TRUE);
		if (s32Ret1 != HI_SUCCESS)
		{
			MUX_PLAY_ERROR( "Set Hdmi av mute ret error,%#x ", s32Ret1);
		}

		s32Ret = HI_SVR_PLAYER_GetParam(play->playerHandler, HI_SVR_PLAYER_ATTR_AVPLAYER_HDL, &hAVPlay);
		if (s32Ret == HI_SUCCESS)
		{/*mvc don't need to set framepack type*/
			s32Ret = HI_UNF_AVPLAY_SetAttr(hAVPlay, HI_UNF_AVPLAY_ATTR_ID_FRMPACK_TYPE, (HI_VOID*)&enFrameType);
			s32Ret |= HI_UNF_DISP_Set3DMode(play->muxRx->playerParam.u32Display, en3D, enEncFormat);
		}

		s32Ret1 = HI_UNF_HDMI_SetAVMute(HI_UNF_HDMI_ID_0, HI_FALSE);
		if (s32Ret1 != HI_SUCCESS)
		{
			MUX_PLAY_ERROR("Set Hdmi av unmute ret error,%#x \n", s32Ret1);
		}
	}
	else
	{
		s32Ret = HI_FAILURE;
	}

	MUX_PLAY_DEBUG( "set videomode from(mode:%d,fmt:%d) to (mode:%d,format:%d) ret %#x\n", enCur3D, enCurEncFormat, en3D, enEncFormat, s32Ret);

	return s32Ret;
}

/* it is called in the context of timer, eg. context of CmnTimer thread;
* return value: next schedule interval for this timer
*/
static int _mediaTimeoutCallback(int interval, void *param)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)param;

	PLAY_LOCK(play);
	MUX_PLAY_DEBUG("timer callback : canceling Media Thread...");

//	HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_PRE_CLOSE_FILE, NULL);
#if MUX_THREAD_SUPPORT_DYNAMIC	
	if(!play->mediaThread)
	{
		PLAY_UNLOCK(play);

		MUX_PLAY_ERROR("timer callback error, the Media Thread has removed");
//		CMN_ABORT("timer callback error, the Media Thread has removed");
		return 0;
	}

#if 0	
	MUX_PLAY_WARN(" MediaTimer for '%s' callback handler is running, send signal to thread %s....", play->muxFsm.name, play->mediaThread->name );
	if(pthread_kill(play->mediaThread->id, SIGUSR1) != 0 )
	{
		MUX_PLAY_ERROR("Send signal SIGUSR1 to thread '%s(%d)' failed: %s", play->mediaThread->name, play->mediaThread->pId, strerror(errno));
	}
#else
	if(pthread_cancel(play->mediaThread->id) )
	{
		MUX_PLAY_ERROR("Cancel to thread '%s(%d)' failed: %s", play->mediaThread->name, play->mediaThread->pId, strerror(errno));
	}
#endif

	play->mediaThread = NULL;
	play->mediaTimer = NULL;
#else
	MUX_PLAY_WARN("MediaTimer for '%s' callback handler is running, send signal to thread %d....", play->muxFsm.name, play->mediaThread );
	if(pthread_kill(play->mediaThread, SIGUSR1) != 0 )
	{
		MUX_PLAY_ERROR("Send signal SIGUSR1 to thread '%s(%d)' failed: %s", play->muxFsm.name, play->mediaThread, strerror(errno));
	}
	play->mediaThread = -1;
#endif
	play->mediaState = SET_MEDIA_STATE_TIMEOUT;

	/* schedule timeout event and then send back reply if there is JSON command before this FAIL event */
	muxPlayerReportFsmEvent(&play->muxFsm, (HI_SVR_PLAYER_EVENT_E)PLAYER_EVENT_FAIL, 0, NULL);

	PLAY_UNLOCK(play);
	return 0; /* return current timer */
}

#if 0
static void _mediaThreadSignalHandler(int signum)
{
#if CMN_TIMER_DEBUG
	MUX_PLAY_DEBUG("SetMedia Thread '%s' exit in signalHandler!\n", cmnThreadGetName() );
#endif

	pthread_exit(NULL);
}
#endif

#if 0
/* operations after setMedia has been successful */
static int _opsAfterSetMedia(MUX_PLAY_T *play)
{
	int i;
	int res;
//	MuxPlayer *muxPlayer = (MuxPlayer *)play->muxRx->muxPlayer;
	
	HI_S32 s32HlsStreamNum = 0;
	HI_FORMAT_HLS_STREAM_INFO_S stStreamInfo;
	res = HI_SVR_PLAYER_Invoke( play->playerHandler, HI_FORMAT_INVOKE_GET_HLS_STREAM_NUM, &s32HlsStreamNum);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("get hls stream num fail, ret = 0x%x!", res);
	}
	else
	{
		MUX_PLAY_DEBUG("get hls stream num = %d", s32HlsStreamNum);

		/* Display the hls bandwidth stream info */
		for (i = 0; i < s32HlsStreamNum; i++)
		{
			stStreamInfo.stream_nb = i;

			res = HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_GET_HLS_STREAM_INFO, &stStreamInfo);
			if (HI_SUCCESS != res)
			{
				MUX_PLAY_ERROR("get %d hls stream info fail, ret = 0x%x!", i, res);
			}

			MUX_PLAY_INFO("\nHls stream number is: %d", stStreamInfo.stream_nb);
			MUX_PLAY_INFO("URL: %s", stStreamInfo.url);
			MUX_PLAY_INFO("BandWidth: %lld", stStreamInfo.bandwidth);
			MUX_PLAY_INFO("SegMentNum: %d", stStreamInfo.hls_segment_nb);
		}
	}

	HI_SVR_PLAYER_METADATA_S stMetaData;

	memset(&stMetaData, 0, sizeof(HI_SVR_PLAYER_METADATA_S));
	res = HI_SVR_PLAYER_Invoke( play->playerHandler, HI_FORMAT_INVOKE_GET_METADATA, &stMetaData);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("get metadata fail!");
	}
	else
	{
		MUX_PLAY_DEBUG("get metadata success!");
		HI_SVR_META_PRINT(&stMetaData);
	}


	/*Check drm status*/
#ifdef DRM_SUPPORT
	HI_CHAR* pszDrmMimeType = NULL;
	for (i = 0; i < stMetaData.u32KvpUsedNum; i++)
	{
		if (!strcasecmp(stMetaData.pstKvp[i].pszKey, "DRM_MIME_TYPE"))
		{
			pszDrmMimeType = (HI_CHAR*)stMetaData.pstKvp[i].unValue.pszValue;
			break;
		}
	}
	
	if (pszDrmMimeType)
	{
		/*DRM opened, check DRM right status*/
		res = drm_check_right_status( play->playerMedia.aszUrl, pszDrmMimeType);
		while ( res > 0)
		{
			res = drm_acquire_right_progress(pszDrmMimeType);
			if ( res == 100)
			{
				MUX_PLAY_INFO( "acquiring right done\n");
				break;
			}
			MUX_PLAY_INFO( "acquiring right progress:%d%%\n", res);
			sleep(1);
		}
		
		if( res < 0)
		{
			MUX_PLAY_ERROR("DRM right invalid, can't play this file, exit now!\n");
			exit(0);
		}
	}
#endif

	/* Set buffer config */
	memset(&play->bufferConfig, 0, sizeof(HI_FORMAT_BUFFER_CONFIG_S));

	play->bufferConfig.eType = HI_FORMAT_BUFFER_CONFIG_SIZE;
	res = HI_SVR_PLAYER_Invoke( play->playerHandler, HI_FORMAT_INVOKE_GET_BUFFER_CONFIG, &play->bufferConfig);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_SVR_PLAYER_Invoke function HI_FORMAT_INVOKE_GET_BUFFER_CONFIG fail, ret = 0x%x", res);
	}
	else
	{
		MUX_PLAY_DEBUG( "BufferConfig:type(%d)", play->bufferConfig.eType);
		MUX_PLAY_DEBUG( "s64EventStart:%lld", play->bufferConfig.s64EventStart);
		MUX_PLAY_DEBUG( "s64EventEnough:%lld", play->bufferConfig.s64EventEnough);
		MUX_PLAY_DEBUG( "s64Total:%lld",  play->bufferConfig.s64Total);
		MUX_PLAY_DEBUG( "s64TimeOut:%lld", play->bufferConfig.s64TimeOut);
	}
	
	res = HI_SVR_PLAYER_GetFileInfo(play->playerHandler, &play->fileInfo);
	if (HI_SUCCESS == res)
	{
		HI_SVR_PLAYER_STREAMID_S streamId = {0};

		res = HI_SVR_PLAYER_GetParam( play->playerHandler, HI_SVR_PLAYER_ATTR_STREAMID, (HI_VOID*)&streamId);
		if ((HI_SUCCESS == res) && ( play->fileInfo->u32ProgramNum > 0))
		{
			if ( play->fileInfo->pastProgramInfo[streamId.u16ProgramId].u32VidStreamNum > 0 &&
				play->fileInfo->pastProgramInfo[streamId.u16ProgramId].pastVidStream[streamId.u16VidStreamId].u32Format == HI_FORMAT_VIDEO_MVC)
			{
				muxSetVideoMode(play, HI_UNF_DISP_3D_FRAME_PACKING);
			}

			if ( play->fileInfo->pastProgramInfo[streamId.u16ProgramId].u32SubStreamNum > 0)
			{
				if ( play->fileInfo->pastProgramInfo[streamId.u16ProgramId].pastSubStream[streamId.u16SubStreamId].u32Format== HI_FORMAT_SUBTITLE_HDMV_PGS)
				{
					play->s_bPgs = HI_TRUE;
				}
				else
				{
					play->s_s64CurPgsClearTime = -1;
					play->s_bPgs = HI_FALSE;
				}
			}
		}
	}
	else
	{
		MUX_PLAY_ERROR("get file info fail!");
	}

	return res;
}
#endif


int muxPlayerCheckStatus(MUX_PLAY_T *play, int state )
{
	HI_S32 s32Ret = HI_FAILURE;
	HI_SVR_PLAYER_INFO_S stPlayerInfo;

	if (HI_SVR_PLAYER_STATE_BUTT <= state )
	{
		return 0;
	}
	
	/* only in PLAYING state, it is ok */
	s32Ret = HI_SVR_PLAYER_GetPlayerInfo(play->playerHandler, &stPlayerInfo);
	if (HI_SUCCESS == s32Ret && (HI_U32)stPlayerInfo.eStatus == state)
	{
		return 1;
	}
	else
	{/* when player is in state of 'STOP', this operation fail */
		MUX_PLAY_ERROR("GetPlayerInfo failed: %s current status is %d", play->muxFsm.name, (stPlayerInfo.eStatus));
		if( PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_INIT) || PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_STOP) )
		{/* make SetMedia goes on */
			return 1;
		}
	}

	return 0;
}

static int _checkNetworkMedia(char *playUrl)
{
	char *local = "/media/";
	
	if(!strncasecmp(playUrl, local, strlen(local)))
	{
		return HI_FALSE;
	}

	return HI_TRUE;
}


#if MUX_THREAD_SUPPORT_DYNAMIC	
int _setMediaThread(CmnThread *th)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)th->data;
#else
int _setMediaThread(void *data)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)data;
#endif
	char timeName[128];
	HI_SVR_PLAYER_MEDIA_S			playerMedia;
	
	HI_S32  res, ret;
	HI_HANDLE hSo = HI_SVR_PLAYER_INVALID_HDL;
	HI_HANDLE hAVPlay = HI_SVR_PLAYER_INVALID_HDL;

	MuxPlayer *muxPlayer = (MuxPlayer *)play->muxRx->muxPlayer;
	

	int _timeout = (_checkNetworkMedia(play->currentUrl)==HI_TRUE)?muxPlayer->playerConfig.playTimeoutNetwork:muxPlayer->playerConfig.playTimeoutLocal;

#if CMN_TIMER_DEBUG
	MUX_PLAY_DEBUG("Timeout is %d seconds", _timeout);
#endif
	snprintf(timeName, sizeof(timeName), "MediaTimer%d-%d", play->windowIndex, play->countOfPlay );

	PLAY_LOCK(play);
	play->mediaTimer = cmn_add_timer(_timeout*1000, _mediaTimeoutCallback, play, timeName );
	
#if 0
	cmnThreadSetSignalHandler(SIGUSR1, _mediaThreadSignalHandler);
#else
	cmnThreadMaskAllSignals();
#endif
	
	memset(&playerMedia, 0, sizeof(HI_SVR_PLAYER_MEDIA_S));
	snprintf(playerMedia.aszUrl, HI_FORMAT_MAX_URL_LEN, "%s", play->currentUrl);
	playerMedia.s32PlayMode = HI_SVR_PLAYER_PLAYMODE_NORMAL;
	playerMedia.u32ExtSubNum = 0;
	playerMedia.u32UserData = 0;


	play->mediaState = SET_MEDIA_STATE_WAITING;
	MUX_PLAY_INFO("%s-%d: ### open media file '%s'...", play->muxFsm.name, play->countOfPlay, playerMedia.aszUrl);

	PLAY_UNLOCK(play);

	/* this code line is used to test cancelling setMedia thread, and set localTimeout is smaller than 2500 */
//	cmn_delay(2500);

	/* make it async-cancelable, then the HiMedia Thread wil be affected???? */
	/* make it can be cancel-async */
#if 1	
	if(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) != 0)
	{
		MUX_WARN("Set thread '%s' as CANCEL_ASYNC failed : %s", strerror(errno));
	}
#endif

	while (1)
	{

#if WITH_HIPLAYER_STATE_CHECK
		if( muxPlayerCheckStatus(play, HI_SVR_PLAYER_STATE_STOP) ||
			muxPlayerCheckStatus(play, HI_SVR_PLAYER_STATE_INIT) ||
			muxPlayerCheckStatus(play, HI_SVR_PLAYER_STATE_DEINIT) || play->countOfPlay == 1 )
#else
		if( PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_STOP) ||
			PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_INIT) ||
			PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_DEINIT)  )
#endif
		{
			res = HI_SVR_PLAYER_SetMedia( play->playerHandler, HI_SVR_PLAYER_MEDIA_STREAMFILE, &playerMedia);
			break;
		}
		usleep(SLEEP_TIME);
	}

#if 1
	if(pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) != 0)
	{
		MUX_WARN("Set thread '%s' as CANCEL_ASYNC failed : %s", strerror(errno));
	}
#endif	

#if 0
	{/* test timer and exit of setMedia timerThread */
		cmn_usleep(35000*1000); /*more than delay for both network and local media */
	}
#endif	

	PLAY_LOCK(play);
	MUX_PLAY_DEBUG("%s: ### open media file '%s' continuing.....", play->muxFsm.name, playerMedia.aszUrl);

	/* return, then cancel timer at once. */
	/* add mutex lock for these data?? .Jack Lee */
	play->mediaState = SET_MEDIA_STATE_OK;
	ret = cmn_remove_timer( play->mediaTimer);
	if(ret <= 0)
	{
		if(ret == FALSE)
		{
			MUX_PLAY_ERROR("Can't find the timer : %p for %s", play->mediaTimer, play->muxFsm.name );
		}
		else
		{
			MUX_PLAY_ERROR("timer is NULL : %p for %s", play->mediaTimer, play->muxFsm.name );
		}
		CMN_ABORT("Timer can't be removed");
	}
	play->mediaTimer = NULL;

//	MUX_PLAY_DEBUG("### set media file '%s' on %s return now", playerMedia.aszUrl, play->muxFsm.name );
	if (HI_SUCCESS != res )
	{
		MUX_PLAY_ERROR("%s-%d SetMedia '%s' err, ret = 0x%x!", play->muxFsm.name, play->countOfPlay, playerMedia.aszUrl, res);
#if 0
		/* no need for stop command */
		res = HI_SVR_PLAYER_Stop(play->playerHandler); /* added, 13, 07, 2017*/
#else
		muxPlayerStopPlaying(play);
#endif

		muxPlayerReportFsmEvent(&play->muxFsm, (HI_SVR_PLAYER_EVENT_E)PLAYER_EVENT_FAIL, 0, NULL);
#if MUX_THREAD_SUPPORT_DYNAMIC	
		play->mediaThread = NULL;
#else
		play->mediaThread = -1;
#endif

		PLAY_UNLOCK(play);

		return -EXIT_FAILURE;
	}
	else
	{
		MUX_PLAY_INFO("Success: SetMedia '%s' on %s-%d sucess",play->currentUrl, play->muxFsm.name, play->countOfPlay);
	}

	res |= HI_SVR_PLAYER_GetParam( play->playerHandler, HI_SVR_PLAYER_ATTR_SO_HDL, &hSo);
	if (HI_SUCCESS != res )
	{
		MUX_PLAY_ERROR("%s: failed ATTR_SO_HDL, ret = 0x%x!", play->muxFsm.name, res);
//		return NULL;
	}
	else
	{
		if(hSo != play->soHandler)
		{
//			MUX_PLAY_DEBUG("Old SO Handle is 0x%x, new is 0x%x", play->soHandler, hSo);
			if(play->soHandler != 0xffffffff )
			{/* subtitle handler or subtitle output handler */
//				HI_UNF_SUBT_Destroy(play->soHandler);
			}
			
			play->soHandler = hSo;
		}
	}
	
	res |= HI_SVR_PLAYER_GetParam(play->playerHandler, HI_SVR_PLAYER_ATTR_AVPLAYER_HDL, &hAVPlay);
	if (HI_SUCCESS != res )
	{
		MUX_PLAY_ERROR("%s: failed AVPLAYER_HDL, ret = 0x%x!", play->muxFsm.name, res);
//		return NULL;
	}
	else
	{
		if( hAVPlay != play->avPlayHandler)
		{
			MUX_PLAY_DEBUG("Old AvPlay Handle is %d, new is %d on %s", play->avPlayHandler, hAVPlay, play->muxFsm.name);
			if(play->avPlayHandler != 0xffffffff )
			{
				HI_UNF_AVPLAY_Destroy(play->avPlayHandler);
			}
			play->avPlayHandler = hAVPlay;
		}
	}

	muxPlayerWindowUpdateHandler(play);

#if 1
	{
		HI_UNF_SYNC_ATTR_S	syncAttr;

		HI_UNF_AVPLAY_GetAttr(play->avPlayHandler, HI_UNF_AVPLAY_ATTR_ID_SYNC, &syncAttr);
//		syncAttr.enSyncRef = HI_UNF_SYNC_REF_AUDIO;
		syncAttr.enSyncRef = HI_UNF_SYNC_REF_NONE;
		res = HI_UNF_AVPLAY_SetAttr(play->avPlayHandler, HI_UNF_AVPLAY_ATTR_ID_SYNC, &syncAttr);
		if (HI_SUCCESS != res )
		{
			MUX_PLAY_ERROR("HI_UNF_AVPLAY_SetAttr failed");
		}
	}
#endif

#if PLAYER_ENABLE_SUBTITLE
	if(play->cfg->type == RECT_TYPE_MAIN)
	{/* only main window output subtitle */
		MUX_PLAY_DEBUG("register subtitle call function on %s (0x%x).....", play->muxFsm.name , play);
		res |= HI_UNF_SO_RegOnDrawCb(hSo, muxSubtitleOnDrawCallback, muxSubtitleOnClearCallback, play);
		if (HI_SUCCESS != res)
		{
			MUX_PLAY_ERROR("set subtitle draw function on %s fail!", play->muxFsm.name );
//			return NULL;
		}
	}
#endif


#if 0
	_opsAfterSetMedia(play);

	HI_SVR_PLAYER_STREAMID_S stStreamId;
	res = HI_SVR_PLAYER_GetFileInfo(play->playerHandler,  &play->fileInfo);
	if (HI_SUCCESS == res)
	{
		if (play->fileInfo->u32ProgramNum > 0 &&
			HI_SVR_PLAYER_GetParam(play->playerHandler, HI_SVR_PLAYER_ATTR_STREAMID, (HI_VOID*)&stStreamId) == HI_SUCCESS &&
			play->fileInfo->pastProgramInfo[stStreamId.u16ProgramId].u32VidStreamNum > 0 &&
			play->fileInfo->pastProgramInfo[stStreamId.u16ProgramId].pastVidStream[stStreamId.u16VidStreamId].u32Format
			== HI_FORMAT_VIDEO_MVC)
		{
			muxSetVideoMode(play, HI_UNF_DISP_3D_FRAME_PACKING);
		}
	}
	else
	{
		MUX_PLAY_ERROR("get file info fail!");
	}
#endif

	res = HI_GO_SetWindowOpacity(play->osd->winHandle, 0);

	res = HI_SVR_PLAYER_Play(play->playerHandler);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("play on %s fail, ret = 0x%x ", play->muxFsm.name, res);
	}
	else
	{
	}

	muxPlayerReportFsmEvent(&play->muxFsm, (HI_SVR_PLAYER_EVENT_E)PLAYER_EVENT_OK, 0, NULL);

#if CMN_TIMER_DEBUG
	MUX_PLAY_DEBUG("setMedia thread for %s exit sucessfully now", play->muxFsm.name );
#endif

#if MUX_THREAD_SUPPORT_DYNAMIC	
	play->mediaThread = NULL;
#else
	play->mediaThread = -1;
#endif

	PLAY_UNLOCK(play);
	//return play;
	return -EXIT_FAILURE;
}

/* reset media file and re-register subtitile callback function. used in command and hiplayer event callback */
HI_S32 muxRxPlayerSetMedia(MUX_PLAY_T *play)
{
	play->mediaState = SET_MEDIA_STATE_INIT;
	char threadName[16];

	snprintf(threadName, sizeof(threadName), "SetMedia%d-%d", play->windowIndex, play->countOfPlay );
#if MUX_THREAD_SUPPORT_DYNAMIC	
	PLAY_LOCK(play);

	while( play->mediaThread )
	{
		PLAY_UNLOCK(play);
		cmn_delay(1);
		PLAY_LOCK(play);
	}

	play->mediaThread = cmnThreadCreateThread(_setMediaThread, play, threadName);
	if(!play->mediaThread)
	{
		CMN_ABORT("'%s' failed when creating", threadName);
	}
	PLAY_UNLOCK(play);
#else
	if(pthread_create(&play->mediaThread, HI_NULL, (HI_VOID *)_setMediaThread, play) != 0)
	{
		MUX_PLAY_ERROR("Error in creating a new thread '%s': %s", threadName, strerror(errno) );
		return EXIT_SUCCESS;
	}

	if( pthread_setname_np(play->mediaThread, threadName) != 0)
	{
		MUX_PLAY_WARN("Error in set thread name '%s': %s", threadName, strerror(errno) );
		return EXIT_SUCCESS;
	}
#endif

#if CMN_TIMER_DEBUG
	MUX_PLAY_DEBUG("SetMedia Thread for %s (%s)", play->muxFsm.name, threadName);
#endif

	return HI_SUCCESS;
}


/*
* isInit: is it initialize from playlist or play the current playlist;
* repeatNumber: only used when isInit == TRUE;
* return : 0: stop current playlist; others: continue playing
*/
int	muxPlayerPlaying(MUX_PLAY_T *play, int isInit, char *media, int repeatNumber)
{
	int ret = EXIT_SUCCESS;
	int isContinue = TRUE;

	if(isInit)
	{/* it is for initialization: load currentPlaylist */
		muxPlayerInitCurrentPlaylist(play, media, repeatNumber);
	}
	else
	{/* load another media in current playlist */
		if(muxPlayerUpdateCurrentPlaylist(play) != EXIT_SUCCESS)
		{
			isContinue = FALSE;
		}
	}

	if(!isContinue)
	{/* end of current playing */
		PLAY_ALERT_MSG(play, COLOR_GREEN,  "'%s' playing ends", play->currentPlaylist.name);

		/* stop playing stream */
		muxPlayerStopPlaying(play);

		/* stop playing image when enter into STOP state */
		return EXIT_SUCCESS;
	}

	if(IS_LOCAL_IMAGE_FILE(play->currentUrl) )
	{
		int ret2;
		play->countOfPlay++;
		
		MUX_PLAY_INFO("'%s-%d' No.%d playing : local image file '%s' in OSD", play->muxFsm.name, play->countOfPlay, play->countOfPlay, play->currentUrl);

		ret2= OSD_DESKTOP_LOCK(&play->muxRx->higo);
		if(ret2 != 0)
		{
			MUX_PLAY_WARN( "Lock for playing image: %s", strerror(errno) );
			return HI_SUCCESS;
		}

		if(isInit ||play->muxRx->muxPlayer->playerConfig.enableLowDelay==0 )
		{
			MUX_PLAY_DEBUG("OSD Decoding image file '%s'", play->currentUrl );
			PLAY_ALERT_MSG(play, COLOR_GREEN,  "OSD Decoding and showing image file '%s'", play->currentUrl );
			ret = muxOsdImageShow(play->osd, play->currentUrl);
			snprintf(play->osd->cfg->url, 1024, "%s", play->currentUrl);

		}
		else
		{
			{
				MUX_PLAY_DEBUG("OSD showing preloaded image file '%s'", play->currentUrl );
				PLAY_ALERT_MSG(play, COLOR_GREEN,  "OSD showing preloaded image file '%s'", play->currentUrl );
				muxOsdImageDisplay(play->osd, TRUE);
			}
		}

		ret2 = OSD_DESKTOP_UNLOCK(&play->muxRx->higo);
		if(ret2 != 0)
		{
			MUX_PLAY_WARN( "Unlocked for playing image: %s", strerror(errno) );
			return HI_SUCCESS;
		}


		if(ret == EXIT_SUCCESS)
		{/* event make state into PLAY_IMAGE */
			muxPlayerReportFsmEvent(&play->muxFsm, (HI_SVR_PLAYER_EVENT_E)PLAYER_EVENT_IMAGE_OK, 0, NULL);
		}
		else
		{
			muxPlayerReportFsmEvent(&play->muxFsm, (HI_SVR_PLAYER_EVENT_E)PLAYER_EVENT_FAIL, 0, NULL);
		}
	}
	else
	{
		if(cmnMediaCheckCheckImageFile(play->currentUrl))
		{
			/* stop playing image when enter into STOP state */
			MUX_PLAY_WARN("media file '%s' is a local image file, but not in mobile device", play->currentUrl);
			return EXIT_SUCCESS;
		}
		else
		{
			play->countOfPlay++;
			MUX_PLAY_DEBUG("'%s-%d' playing : stream '%s' in play", play->muxFsm.name, play->countOfPlay, play->currentUrl);
			ret = muxRxPlayerSetMedia(play);
			ret = ret;
		}
	}

	return 1;
}


/* timeout callback both for playing image or playing stream */
static int _mediaPlayingTimeoutCallback(int interval, void *param)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)param;

	/* for playing timer, so protect is needed, because it does not check the statut of play */
	PLAY_LOCK(play);

#if 0
	int	ret = EXIT_SUCCESS;

	if( PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_PLAY) )
	{
		ret = HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_PRE_CLOSE_FILE, NULL);
	}
	else if( PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE) )
	{
		ret = HI_GO_SetWindowOpacity( play->osd->winHandle, 0);
	}
#endif

	play->mediaState = SET_MEDIA_STATE_TIMEOUT;
	
	muxPlayerReportFsmEvent(&play->muxFsm, (HI_SVR_PLAYER_EVENT_E)PLAYER_EVENT_TIMEOUT, 0, NULL);

	play->playTimer = NULL;
	PLAY_UNLOCK(play);
	
	return 0; /* return current timer */
}

int	muxPlayerStartPlayingTimer(MUX_PLAY_T *play)
{
	if( play->currentDuration > 0 )
	{
		char _name[32];
		snprintf(_name, sizeof(_name), "PlayTimer%d-%d", play->windowIndex, play->countOfTimer++ );

		PLAY_LOCK(play);
		while(play->playTimer)
		{
			PLAY_UNLOCK(play);
			cmn_delay(1);
			PLAY_LOCK(play);
		}

		MUX_PLAY_INFO("start the timer %s of %d seconds", _name, play->currentDuration );
		
		play->playTimer = cmn_add_timer(play->currentDuration*1000, _mediaPlayingTimeoutCallback, play, _name );
		PLAY_UNLOCK(play);
	}

	return EXIT_SUCCESS;
}

int	muxPlayerRemovePlayingTimer(MUX_PLAY_T *play)
{
	int ret;

	PLAY_LOCK(play);
	if(play->playTimer== NULL)
	{
		PLAY_UNLOCK(play);
		return EXIT_SUCCESS;
	}
	
	/* return, then cancel timer at once. */
	ret = cmn_remove_timer( play->playTimer);
	if(ret <= 0)
	{
		if(ret == FALSE)
		{
			MUX_PLAY_ERROR("Can't find the timer : %p for %s", play->playTimer, play->muxFsm.name );
		}
		else
		{
			MUX_PLAY_ERROR("timer is NULL : %p for %s", play->mediaTimer, play->muxFsm.name );
		}
		CMN_ABORT("PlayTimer can't be removed");
	}
	play->playTimer = NULL;

	PLAY_UNLOCK(play);

	return EXIT_SUCCESS;
}



int	muxPlayerStopPlaying(MUX_PLAY_T *play)
{
	int ret = HI_SUCCESS;
	
#if WITH_HIPLAYER_STATE_CHECK
	if(muxPlayerCheckStatus(play, HI_SVR_PLAYER_STATE_PLAY) )
#else
	if(PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_PLAY)  )
#endif
	{
		ret = HI_SVR_PLAYER_Stop(play->playerHandler);
		if (HI_SUCCESS != ret)
		{/* sometimes, it is not playing when calling this function because timeout event */
			MUX_PLAY_ERROR("Play '%s' stop fail, ret = 0x%x", play->muxFsm.name, ret);
		}
	}
	else
	{
//		MUX_PLAY_WARN("Play '%s' in state %s, stop is not permitted", muxPlayerStateName(play->muxFsm.currentState), play->muxFsm.name);
	}

	return EXIT_SUCCESS;
}


int muxPlayerReset(MUX_PLAY_T *play)
{
	int res = HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_PRE_CLOSE_FILE, NULL);
	if(res != HI_SUCCESS)
	{
		MUX_PLAY_WARN("'%s' invoke PRE CLOSE FILE :0x%x", play->muxFsm.name, res);
	}
//	res = HI_UNF_VO_ResetWindow(play->windowHandle,  (play->muxRx->muxPlayer->playerConfig.keepLastFrame)?HI_UNF_WINDOW_FREEZE_MODE_LAST: HI_UNF_WINDOW_FREEZE_MODE_BLACK);
	res = HI_UNF_VO_ResetWindow(play->windowHandle, HI_UNF_WINDOW_FREEZE_MODE_LAST);
	MUX_SUBTITLE_CLEAR_ALL(play);

	return EXIT_SUCCESS;
}

