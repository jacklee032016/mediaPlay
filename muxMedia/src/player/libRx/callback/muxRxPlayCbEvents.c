
#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"

MUX_PLAY_T *muxPlayerFindByHandle(HI_HANDLE hPlayer);

#if 0 /* commented, Dec.22nd, 2017 */
static HI_S32 __checkPlayerStatus(MUX_PLAY_T *play, HI_U32 u32Status, HI_U32 u32TimeOutMs)
{
	HI_S32 s32Ret = HI_FAILURE;
	HI_U32 u32Time = (HI_U32)times(NULL) * 10;
	HI_SVR_PLAYER_INFO_S	playerInfo;

	if (HI_SVR_PLAYER_STATE_BUTT <= u32Status)
	{
		return HI_FAILURE;
	}

	while (1)
	{
		s32Ret = HI_SVR_PLAYER_GetPlayerInfo(play->playerHandler, &playerInfo);

		if (HI_SUCCESS == s32Ret && (HI_U32)playerInfo.eStatus == u32Status)
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
#endif

HI_VOID* muxNetworkReconnect(HI_VOID *pArg)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)pArg;
#if 0 /* commented, Dec.22nd, 2017 */
	HI_S32	ret = HI_FAILURE;
	HI_S64	s64LastPts = HI_FORMAT_NO_PTS;

	HI_FORMAT_FILE_INFO_S	*fileInfo = NULL;
	HI_SVR_PLAYER_MEDIA_S	media;
	HI_SVR_PLAYER_INFO_S	playerInfo;

	
	memset(&media, 0, sizeof(HI_SVR_PLAYER_MEDIA_S));
	memset(&playerInfo, 0, sizeof(HI_SVR_PLAYER_INFO_S));

	play->s_s32ThreadEnd = HI_FAILURE;
	sprintf(media.aszUrl, "%s", play->currentUrl);
	media.u32ExtSubNum = 0;
	PLAY_ERROR(play, "### open media file is %s \n", media.aszUrl);

	/* ??? */
#if WITH_HIGO_SURFACE_HANDLER	
	(HI_VOID)HI_GO_FillRect( play->muxRx->higo.layerSurfaceHandle, NULL, 0x00000000, HIGO_COMPOPT_NONE);
#endif

	(HI_VOID)HI_GO_RefreshLayer( play->muxRx->higo.layerHandler, NULL);


	MUX_SUBTITLE_CLEAR_ALL(play);
	ret = HI_SVR_PLAYER_GetPlayerInfo( play->playerHandler, &playerInfo);
	if (HI_SUCCESS == ret)
	{
		s64LastPts = playerInfo.u64TimePlayed;
		ret = HI_FAILURE;
	}

//	(HI_VOID)HI_SVR_PLAYER_Stop(play->playerHandler);
	muxPlayerStopPlaying(play);

	__checkPlayerStatus(play, HI_SVR_PLAYER_STATE_STOP, 100);

	while ( ret == HI_FAILURE && (play->s_s32ThreadExit == HI_FAILURE))
	{
		ret = muxRxPlayerSetMedia(play);
		if (HI_SUCCESS != ret)
		{
			continue;
		}
	}

	ret = HI_SVR_PLAYER_GetFileInfo(play->playerHandler, &fileInfo);
	if (HI_SUCCESS == ret && NULL != fileInfo)
	{
	}
	else
	{
		PLAY_ERROR(play, "get file info fail!");
	}

	(HI_VOID)HI_SVR_PLAYER_Seek(play->playerHandler, s64LastPts);
	(HI_VOID)HI_SVR_PLAYER_Play(play->playerHandler);
#endif

	play->s_s32ThreadEnd = HI_SUCCESS;
	return NULL;
}


HI_S32 muxEventCallBack(HI_HANDLE hPlayer, HI_SVR_PLAYER_EVENT_S *event)
{
#if 1	
	MUX_PLAY_T *play = NULL;
	
	if (0 == hPlayer || NULL == event)
	{
		return HI_SUCCESS;
	}

//#if PLAYER_ENABLE_INFO_EVENT
//		event->eEvent == HI_SVR_PLAYER_EVENT_EOF ||
//		event->eEvent == HI_SVR_PLAYER_EVENT_SEEK_FINISHED||

	if(event->eEvent == HI_SVR_PLAYER_EVENT_PROGRESS ||
		event->eEvent == HI_SVR_PLAYER_EVENT_FIRST_FRAME_TIME )
	{/* ignore too much PROGRESS event and other events which is not handled in FSM */
		return HI_SUCCESS;
	}

	play = muxPlayerFindByHandle(hPlayer);
	if( play == NULL)
	{
		MUX_PLAY_ERROR("can not HiPlayer handle 0x%x", hPlayer);
//		exit(1);
		return HI_FAILURE;
	}
	

	HI_SVR_PLAYER_EVENT_S *newEvent = (HI_SVR_PLAYER_EVENT_S *)cmn_malloc(sizeof(HI_SVR_PLAYER_EVENT_S));
	if(newEvent==NULL)
	{
		PLAY_ERROR(play, "No memory is available");
		exit(1);
	}

	newEvent->eEvent = event->eEvent;
	newEvent->u32Len = event->u32Len;
	
	//PLAY_DEBUG(play,  "Event :'%s'", muxPlayerEventName(event->eEvent) );
	//PLAY_DEBUG(play,  "playing URL :'%s'; Config URL :'%s'", muxPlay.currentUrl, muxPlayer->muxConfig.url);
	//PLAY_DEBUG(play,  "address :'%x'", muxPlayer->muxFsm.fsm );
	//PLAY_DEBUG(play,  "Event : %s, FSM :'%s'\n", muxPlayerEventName(event->eEvent), muxPlay.muxPlayer->muxFsm.fsm->name );
#if PLAYER_DEBUG_HIPLAY_EVENT
	PLAY_DEBUG(play,  "Event : '%s', Current State :'%s'", muxPlayerEventName(newEvent->eEvent), muxPlayerStateName( play->muxFsm.currentState));
#endif

	if(newEvent->u32Len > 0)
	{
#if PLAYER_DEBUG_HIPLAY_EVENT
//		PLAY_DEBUG(play, "Event : '%s', data length %d", muxPlayerEventName(newEvent->eEvent), newEvent->u32Len );
#endif
		newEvent->pu8Data = (HI_U8 *)cmn_malloc(event->u32Len);
		if(newEvent->pu8Data == NULL)
		{
			PLAY_ERROR(play, "No memory for data is available");
			exit(1);
		}
		memcpy(newEvent->pu8Data, event->pu8Data, newEvent->u32Len);
	}


#if 0
	HI_SVR_PLAYER_Invoke(muxPlay.playerHandler, HI_FORMAT_INVOKE_GET_BANDWIDTH, &muxPlay.downloadBandWidth);
	PLAY_DEBUG(play, "DownloadProgress bandwidth:'%lld'", muxPlay.downloadBandWidth);
#endif

	SEND_EVEVT_TO_PLAYER(play, newEvent->eEvent, newEvent);
#endif

	return HI_SUCCESS;
}


