
#include "libRx.h"

static HI_S32 __checkPlayerStatus(HI_HANDLE hPlayer, HI_U32 u32Status, HI_U32 u32TimeOutMs)
{
	HI_S32 s32Ret = HI_FAILURE;
	HI_U32 u32Time = (HI_U32)times(NULL) * 10;
	HI_SVR_PLAYER_INFO_S stPlayerInfo;

	if (HI_SVR_PLAYER_STATE_BUTT <= u32Status)
	{
		return HI_FAILURE;
	}

	while (1)
	{
		s32Ret = HI_SVR_PLAYER_GetPlayerInfo(hPlayer, &stPlayerInfo);

		if (HI_SUCCESS == s32Ret && (HI_U32)stPlayerInfo.eStatus == u32Status)
		{
			return HI_SUCCESS;
		}

		if ((((HI_U32)times(NULL) * 10) - u32Time) >= u32TimeOutMs)
		{
			break;
		}

		usleep(SLEEP_TIME);
	}

	return HI_FAILURE;
}

static HI_VOID* _reconnect(HI_VOID *pArg)
{
	HI_S32	ret = HI_FAILURE;
	HI_S64	s64LastPts = HI_FORMAT_NO_PTS;

	HI_FORMAT_FILE_INFO_S	*fileInfo = NULL;
	HI_SVR_PLAYER_MEDIA_S	media;
	HI_SVR_PLAYER_INFO_S	playerInfo;

	MUXLAB_PLAY_T *mux = (MUXLAB_PLAY_T *)pArg;
	
	memset(&media, 0, sizeof(HI_SVR_PLAYER_MEDIA_S));
	memset(&playerInfo, 0, sizeof(HI_SVR_PLAYER_INFO_S));

	mux->s_s32ThreadEnd = HI_FAILURE;
	sprintf(media.aszUrl, "%s", mux->s_aszUrl);
	media.u32ExtSubNum = 0;
	PRINTF("### open media file is %s \n", media.aszUrl);

	(HI_VOID)HI_GO_FillRect( mux->higo.layerSurfaceHandle, NULL, 0x00000000, HIGO_COMPOPT_NONE);
	(HI_VOID)HI_GO_RefreshLayer( mux->higo.layerHandler, NULL);

	muxSubtitleClear(mux);
	ret = HI_SVR_PLAYER_GetPlayerInfo( mux->playerHandler, &playerInfo);
	if (HI_SUCCESS == ret)
	{
		s64LastPts = playerInfo.u64TimePlayed;
		ret = HI_FAILURE;
	}

	(HI_VOID)HI_SVR_PLAYER_Stop(mux->playerHandler);
	__checkPlayerStatus(mux->playerHandler, HI_SVR_PLAYER_STATE_STOP, 100);

	while ( ret == HI_FAILURE && (mux->s_s32ThreadExit == HI_FAILURE))
	{
		ret = muxSetMedia(mux);
		if (HI_SUCCESS != ret)
		{
			continue;
		}
	}

	ret = HI_SVR_PLAYER_GetFileInfo(mux->playerHandler, &fileInfo);
	if (HI_SUCCESS == ret && NULL != fileInfo)
	{
		muxOutputFileInfo( fileInfo);
	}
	else
	{
		PRINTF("\e[31m ERR: get file info fail! \e[0m\n");
	}

	(HI_VOID)HI_SVR_PLAYER_Seek(mux->playerHandler, s64LastPts);
	(HI_VOID)HI_SVR_PLAYER_Play(mux->playerHandler);

	mux->s_s32ThreadEnd = HI_SUCCESS;
	return NULL;
}

HI_S32 muxEventCallBack(HI_HANDLE hPlayer, HI_SVR_PLAYER_EVENT_S *event)
{
	HI_SVR_PLAYER_STATE_E		eEventBk = HI_SVR_PLAYER_STATE_BUTT;
	HI_SVR_PLAYER_STREAMID_S	*streamId = NULL;
	HI_HANDLE					hWindow = HI_SVR_PLAYER_INVALID_HDL;
	HI_FORMAT_FILE_INFO_S		*fileInfo;
	
	HI_S32 s32Ret = HI_FAILURE;

	MUXLAB_PLAY_T *mux = &muxPlay;;

	if (0 == hPlayer || NULL == event)
	{
		return HI_SUCCESS;
	}

	if (HI_SVR_PLAYER_EVENT_STATE_CHANGED == (HI_SVR_PLAYER_EVENT_E)event->eEvent)
	{
		eEventBk = (HI_SVR_PLAYER_STATE_E)*event->pu8Data;
		PRINTF("Status change to %d from %d\n", eEventBk, mux->playerState);
		mux->playerState = eEventBk;

		if (eEventBk == HI_SVR_PLAYER_STATE_STOP)
		{
			HI_SVR_PLAYER_Seek(hPlayer, 0);
			HI_SVR_PLAYER_GetParam(hPlayer, HI_SVR_PLAYER_ATTR_WINDOW_HDL, &hWindow);
			HI_UNF_VO_ResetWindow(hWindow, HI_UNF_WINDOW_FREEZE_MODE_BLACK);
		}

		if (eEventBk == HI_SVR_PLAYER_STATE_PAUSE && mux->pausedCtrl == 1)
		{
			mux->pausedCtrl = 2;
		}
		else
		{
			mux->pausedCtrl = 0;
		}

	}
	else if (HI_SVR_PLAYER_EVENT_SOF == (HI_SVR_PLAYER_EVENT_E)event->eEvent)
	{
		PRINTF("File position is start of file! \n");
		/* 快退到头重新启动播放 */
		eEventBk = (HI_SVR_PLAYER_STATE_E)*event->pu8Data;

		if (HI_SVR_PLAYER_STATE_BACKWARD == eEventBk)
		{
			PRINTF("backward to start of file, start play! \n");
			HI_SVR_PLAYER_Play(hPlayer);
		}
	}
	else if (HI_SVR_PLAYER_EVENT_EOF == (HI_SVR_PLAYER_EVENT_E)event->eEvent)
	{
		PRINTF("File postion is end of file, clear last frame and replay! \n");
		HI_SVR_PLAYER_GetParam(hPlayer, HI_SVR_PLAYER_ATTR_WINDOW_HDL, &hWindow);
		HI_UNF_VO_ResetWindow(hWindow, HI_UNF_WINDOW_FREEZE_MODE_BLACK);
		muxSubtitleClear(mux);
		
		/*rtsp/mms/rtmp need to redo setting media when stop->play/replay*/
		if (strstr(mux->s_aszUrl, "rtsp") ||strstr(mux->s_aszUrl, "mms")  || strstr(mux->s_aszUrl, "rtmp"))
		{
			s32Ret = muxSetMedia(mux);
			if (HI_SUCCESS != s32Ret)
			{
				return HI_FAILURE;
			}
		}

		HI_SVR_PLAYER_Play(hPlayer);
	}
	else if (HI_SVR_PLAYER_EVENT_STREAMID_CHANGED == (HI_SVR_PLAYER_EVENT_E)event->eEvent)
	{
		streamId = (HI_SVR_PLAYER_STREAMID_S*)event->pu8Data;

		if (NULL != streamId)
		{
			if (mux->playerStreamIDs.u16SubStreamId != streamId->u16SubStreamId ||
				mux->playerStreamIDs.u16ProgramId != streamId->u16ProgramId )
			{
				muxSubtitleClear(mux);
				mux->playerStreamIDs = *streamId;
			}

			PRINTF("Stream id change to: ProgramId %d, vid %d, aid %d, sid %d \n", streamId->u16ProgramId, streamId->u16VidStreamId,
				streamId->u16AudStreamId, streamId->u16SubStreamId);
		}
	}
	else if (HI_SVR_PLAYER_EVENT_PROGRESS == (HI_SVR_PLAYER_EVENT_E)event->eEvent)
	{
		HI_SVR_PLAYER_PROGRESS_S progress;
		memcpy(&progress, event->pu8Data, event->u32Len);
		if( mux->noPrintInHdmiATC == HI_FALSE)
		{
			PRINTF("Current progress is %d, Duration:%lld ms,Buffer size:%lld bytes\n", progress.u32Progress, progress.s64Duration, progress.s64BufferSize);
		}
	}
	else if (HI_SVR_PLAYER_EVENT_ERROR == (HI_SVR_PLAYER_EVENT_E)event->eEvent)
	{
		HI_S32 s32SysError = (HI_S32)*event->pu8Data;

		if (HI_SVR_PLAYER_ERROR_VID_PLAY_FAIL == s32SysError)
		{
			PRINTF("\e[31m ERR: Vid start fail! \e[0m \n");
		}
		else if (HI_SVR_PLAYER_ERROR_AUD_PLAY_FAIL == s32SysError)
		{
			PRINTF("\e[31m ERR: Aud start fail! \e[0m \n");
		}
		else if (HI_SVR_PLAYER_ERROR_PLAY_FAIL == s32SysError)
		{
			PRINTF("\e[31m ERR: Play fail! \e[0m \n");
		}
		else if (HI_SVR_PLAYER_ERROR_NOT_SUPPORT == s32SysError)
		{
			PRINTF("\e[31m ERR: Not support! \e[0m \n");
		}
		else if (HI_SVR_PLAYER_ERROR_TIMEOUT == s32SysError)
		{
			PRINTF("\e[31m ERR: Time Out! \e[0m \n");
		}
		else
		{
			PRINTF("unknow Error = 0x%x \n", s32SysError);
		}
	}
	else if (HI_SVR_PLAYER_EVENT_NETWORK_INFO == (HI_SVR_PLAYER_EVENT_E)event->eEvent)
	{
		HI_FORMAT_NET_STATUS_S *netStat = (HI_FORMAT_NET_STATUS_S*)event->pu8Data;
		PRINTF("HI_SVR_PLAYER_EVNET_NETWORK_INFO: type:%d, code:%d, str:%s\n", netStat->eType, netStat->s32ErrorCode, netStat->szProtocals);
		
		if (netStat->eType == HI_FORMAT_MSG_NETWORK_ERROR_DISCONNECT)
		{
			s32Ret = pthread_create(&mux->thread, HI_NULL, _reconnect, (HI_VOID*)mux);
			if (s32Ret == HI_SUCCESS)
			{
				PRINTF("create thread:reconnect successfully\n");
			}
			else
			{
				PRINTF("ERR:failed to create thread:reconnect\n");
			}
		}
	}
	else if (HI_SVR_PLAYER_EVENT_FIRST_FRAME_TIME == (HI_SVR_PLAYER_EVENT_E)event->eEvent)
	{
		HI_U32 u32Time = *((HI_U32*)event->pu8Data);
		PRINTF("the first frame time is %d ms\n",u32Time);
	}
	else if (HI_SVR_PLAYER_EVENT_DOWNLOAD_PROGRESS == (HI_SVR_PLAYER_EVENT_E)event->eEvent)
	{
		HI_SVR_PLAYER_PROGRESS_S stDown;
		HI_S64 s64BandWidth = 0;

		HI_SVR_PLAYER_Invoke(hPlayer, HI_FORMAT_INVOKE_GET_BANDWIDTH, &s64BandWidth);

		memcpy(&stDown, event->pu8Data, event->u32Len);
		PRINTF("download progress:%d, duration:%lld ms, buffer size:%lld bytes, bandwidth = %lld\n", stDown.u32Progress, stDown.s64Duration, stDown.s64BufferSize, s64BandWidth);
		if (stDown.u32Progress >= 100 && mux->pausedCtrl == 2)
		{
			HI_SVR_PLAYER_Resume(hPlayer);
		}
	}
	else if (HI_SVR_PLAYER_EVENT_BUFFER_STATE == (HI_SVR_PLAYER_EVENT_E)event->eEvent)
	{
		HI_SVR_PLAYER_BUFFER_S *pstBufStat = (HI_SVR_PLAYER_BUFFER_S*)event->pu8Data;
		PRINTF("HI_SVR_PLAYER_EVENT_BUFFER_STATE type:%d, duration:%lld ms, size:%lld bytes\n",pstBufStat->eType,
			pstBufStat->stBufStat.s64Duration, pstBufStat->stBufStat.s64BufferSize);

		HI_SVR_PLAYER_GetFileInfo(hPlayer, &fileInfo);

		if (pstBufStat->eType == HI_SVR_PLAYER_BUFFER_EMPTY)
		{
			PRINTF("### HI_SVR_PLAYER_EVENT_BUFFER_STATE type:HI_SVR_PLAYER_BUFFER_EMPTY, duration:%lld ms, size:%lld bytes\n",
				pstBufStat->stBufStat.s64Duration, pstBufStat->stBufStat.s64BufferSize);
			if ((HI_FORMAT_SOURCE_NET_VOD == fileInfo->eSourceType ||
				HI_FORMAT_SOURCE_NET_LIVE == fileInfo->eSourceType ) &&
				HI_SVR_PLAYER_STATE_PLAY == mux->playerState)
			{
				mux->pausedCtrl = 1;
				HI_SVR_PLAYER_Pause(hPlayer);
			}
		}
		else if (pstBufStat->eType == HI_SVR_PLAYER_BUFFER_START)
		{
			PRINTF("### HI_SVR_PLAYER_EVENT_BUFFER_STATE type:HI_SVR_PLAYER_BUFFER_START, duration:%lld ms, size:%lld bytes\n",
				pstBufStat->stBufStat.s64Duration, pstBufStat->stBufStat.s64BufferSize);
			if ((HI_FORMAT_SOURCE_NET_VOD == fileInfo->eSourceType ||
				HI_FORMAT_SOURCE_NET_LIVE == fileInfo->eSourceType ) &&
				HI_SVR_PLAYER_STATE_PLAY == mux->playerState)
			{
				mux->pausedCtrl = 1;
				HI_SVR_PLAYER_Pause(hPlayer);
			}
		}
		else if (pstBufStat->eType == HI_SVR_PLAYER_BUFFER_ENOUGH)
		{
			PRINTF("### HI_SVR_PLAYER_EVENT_BUFFER_STATE type:HI_SVR_PLAYER_BUFFER_ENOUGH, duration:%lld ms, size:%lld bytes\n",
				pstBufStat->stBufStat.s64Duration, pstBufStat->stBufStat.s64BufferSize);
			if ((HI_FORMAT_SOURCE_NET_VOD == fileInfo->eSourceType ||
				HI_FORMAT_SOURCE_NET_LIVE == fileInfo->eSourceType ) &&
				2 == mux->pausedCtrl)
			{
				HI_SVR_PLAYER_Resume(hPlayer);
			}
		}
		else if (pstBufStat->eType == HI_SVR_PLAYER_BUFFER_FULL)
		{
			PRINTF("### HI_SVR_PLAYER_EVENT_BUFFER_STATE type:HI_SVR_PLAYER_BUFFER_FULL, duration:%lld ms, size:%lld bytes\n",
				pstBufStat->stBufStat.s64Duration, pstBufStat->stBufStat.s64BufferSize);
			if ((HI_FORMAT_SOURCE_NET_VOD == fileInfo->eSourceType ||
				HI_FORMAT_SOURCE_NET_LIVE == fileInfo->eSourceType ) &&
				2 == mux->pausedCtrl)
			{
				HI_SVR_PLAYER_Resume(hPlayer);
			}
		}
	}
	else if (HI_SVR_PLAYER_EVENT_SEEK_FINISHED == (HI_SVR_PLAYER_EVENT_E)event->eEvent)
	{
		PRINTF("seek finish! \n");
	}
	else
	{
		PRINTF("unknow event type is %d\n",event->eEvent);
	}

	return HI_SUCCESS;
}


