

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"


#define	INGORE_PLAYSTOP_IN_PAUSE_STATE		1

int muxPlayerJSONError(CMN_PLAY_JSON_EVENT *jsonEvent, int errCode, char *msg)
{
	struct DATA_CONN *dataConn =(struct DATA_CONN *)jsonEvent->priv;
	jsonEvent->status = errCode;
	cJSON_AddItemToObject(dataConn->dataObj, MEDIA_CTRL_STATUS_MSG, cJSON_CreateString((msg)));
	return EXIT_SUCCESS;
}


static int	_playSpeeds[] =
{
//	HI_SVR_PLAYER_PLAY_SPEED_64X_FAST_BACKWARD,
	HI_SVR_PLAYER_PLAY_SPEED_32X_FAST_BACKWARD, 
	HI_SVR_PLAYER_PLAY_SPEED_16X_FAST_BACKWARD, 
	HI_SVR_PLAYER_PLAY_SPEED_8X_FAST_BACKWARD, 
	HI_SVR_PLAYER_PLAY_SPEED_4X_FAST_BACKWARD, 
	HI_SVR_PLAYER_PLAY_SPEED_2X_FAST_BACKWARD, 
	HI_SVR_PLAYER_PLAY_SPEED_1X_BACKWARD, 
	HI_SVR_PLAYER_PLAY_SPEED_1X2_SLOW_FORWARD, 
	
/*
	* can't set SPEED_NORMAL, it must be set by RESUME command *
*/	
	HI_SVR_PLAYER_PLAY_SPEED_NORMAL, 
	
	HI_SVR_PLAYER_PLAY_SPEED_2X_FAST_FORWARD, 
	HI_SVR_PLAYER_PLAY_SPEED_4X_FAST_FORWARD, 
	HI_SVR_PLAYER_PLAY_SPEED_8X_FAST_FORWARD, 
	HI_SVR_PLAYER_PLAY_SPEED_16X_FAST_FORWARD, 
	HI_SVR_PLAYER_PLAY_SPEED_32X_FAST_FORWARD, 
//	HI_SVR_PLAYER_PLAY_SPEED_64X_FAST_BACKWARD, 

};

void muxPlayerPlayspeedInit(MUX_PLAY_T *play)
{
	int 	i;

	play->speedCount = sizeof(_playSpeeds)/sizeof(int);
	for(i=0; i< play->speedCount; i++)
	{
		if( _playSpeeds[i] == HI_SVR_PLAYER_PLAY_SPEED_NORMAL )
		{
			play->speedIndex = i;
			play->speedDefault = i;
		}
	}
}

static int	_resumeMedia(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int res;
	
	if( PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_PAUSE) || 
		PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_FORWARD) ||
		PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_BACKWARD) )
	{
		res = HI_SVR_PLAYER_Resume(play->playerHandler);
		if (HI_SUCCESS != res)
		{
			MUX_PLAY_ERROR( "resume fail, ret = 0x%x\n", res);
			cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "resume failed, retry late");
			return EXIT_SUCCESS;
		}
		else
		{
			play->speedIndex = play->speedDefault;
			PLAYER_SET_CURRENT_CMD(play, jsonEvent->event, jsonEvent );
		}
	}
	else
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "Player can't resume in current status, retry late");
	}

	return EXIT_SUCCESS;
}

/* this function is called in following 3 cases: begin to play(jsonEvent is null); forward; backward; */
int muxPlayerPlayspeedSet(MUX_PLAY_T *play, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	int res = HI_SUCCESS;
	int	speed = _playSpeeds[play->speedIndex];
	struct DATA_CONN *dataConn = NULL;
		
	if(jsonEvent)
	{
		dataConn = (struct DATA_CONN *)(jsonEvent)->priv;
		if(!dataConn)
		{
			CMN_ABORT("DATA_CONN is null");
		}
	}
	
//	MUX_PLAY_DEBUG( "speed = %d \n", speed);
	if(speed == HI_SVR_PLAYER_PLAY_SPEED_NORMAL )
	{
		if(jsonEvent)
		{
			jsonEvent->event = CMD_TYPE_RESUME; /* drive FSM to reply these 2 commands */
			return _resumeMedia(play, dataConn, jsonEvent);
		}

		return HI_SUCCESS;
	}
	
	res = HI_SVR_PLAYER_TPlay(play->playerHandler, speed);

		
	if (HI_SUCCESS !=  res )
	{
		MUX_PLAY_WARN("forward/backward fail, ret = 0x%x", res);
		if(jsonEvent)
		{
			
			cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "forward/backward failed, retry late");
		}
		return HI_FAILURE;
	}
	else
	{
		if( IS_MAIN_PLAYER(play) )
		{
			MUX_SUBTITLE_CLEAR_ALL(play);
		}

		if(jsonEvent)
		{
#if 0
			PLAYER_SET_CURRENT_CMD(play, jsonEvent->event, jsonEvent );
#else
			dataConn->errCode = IPCMD_ERR_NOERROR;
#endif
		}
	}
	
	return HI_SUCCESS;
}

static int	_forwardMedia(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int index = play->speedIndex;

	if( PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE) )
	{
		return cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "Can't forward when playing image");
	}
	
	if(play->speedIndex < play->speedCount-1 )
		play->speedIndex++;
	else
		return EXIT_SUCCESS;

	if(muxPlayerPlayspeedSet(play, jsonEvent) != HI_SUCCESS)
	{
		play->speedIndex = index;
	}

	return EXIT_SUCCESS;
}

static int	_backwardMedia(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int index = play->speedIndex;
	
	if( PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE) )
	{
		return cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "Can't backward when playing image");
	}
	
	if(play->speedIndex > 0 )
		play->speedIndex--;
	else
		return EXIT_SUCCESS;

	if(muxPlayerPlayspeedSet(play, jsonEvent) != HI_SUCCESS)
		play->speedIndex = index;

	return EXIT_SUCCESS;
}


static int	_playMedia(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int res;
	int repeatNumber = 1;

	/* sep.18, 2018 */
#if INGORE_PLAYSTOP_IN_PAUSE_STATE
	if(PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_PAUSE) )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOT_SUPPORT_COMMND, "'play' can't be executed when in PAUSE state");
		return EXIT_SUCCESS;
	}
#endif

	char *media = cmnGetStrFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_MEDIA);
	if(media == NULL || !strlen(media))
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "No 'media' is defined in command");
		return EXIT_SUCCESS;
	}

	repeatNumber = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_REPEAT);
	if(repeatNumber == -1)
	{
		MUX_PLAY_ERROR(" No repeat number for this play command");
	}

	muxPlayerRemovePlayingTimer(play);
	
	if(! PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_STOP) && 
		! PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_INIT) &&
		! PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE) )
	{/* when playing stream */

		muxPlayerStopPlaying(play);

		if( IS_MAIN_PLAYER(play) )
		{
			MUX_SUBTITLE_CLEAR_ALL(play);
		}
	}
	else if( PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE)  )
	{/* when playing image */
		muxPlayerReportFsmEvent(&play->muxFsm, (HI_SVR_PLAYER_EVENT_E)PLAYER_EVENT_END, 0, NULL);
	}
	else
	{
		/* Reopen a file */
#if 0
		res = muxPlayerInitCurrentPlaylist(player, media, repeatNumber);
		res = muxRxPlayerSetMedia(player );
#else
		res = muxPlayerPlaying(play, TRUE, media, repeatNumber);
		res = res;
#endif
	}

	PLAYER_SET_CURRENT_CMD(play, jsonEvent->event, jsonEvent ); /* when enter into STOP state, this CMD is emitted, so re-enter into this function to start play */

	return EXIT_SUCCESS;
}


static int	_stopMedia(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int res;

	/* sep.18, 2018 */
#if INGORE_PLAYSTOP_IN_PAUSE_STATE
	if(PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_PAUSE) )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOT_SUPPORT_COMMND, "'stop' can't be executed when in PAUSE state");
		return EXIT_SUCCESS;
	}
#endif		
	
	muxPlayerRemovePlayingTimer(play);
	
	if(! PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_STOP) && 
		! PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_INIT) && 
		! PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE) )
	{
		res = muxPlayerStopPlaying(play);
		if (EXIT_SUCCESS != res)
		{
			MUX_PLAY_ERROR(" stop fail, ret = 0x%x", res);
			cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "stop failed, retry late");
			return EXIT_SUCCESS;
		}

		if( IS_MAIN_PLAYER(play) )
		{
			MUX_SUBTITLE_CLEAR_ALL(play);
		}
		PLAYER_SET_CURRENT_CMD(play, jsonEvent->event, jsonEvent ); /* when enter into STOP state, this CMD is emitted, so re-enter into this function to start play */
	}
	else if( PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE)  )
	{
		muxPlayerReportFsmEvent(&play->muxFsm, (HI_SVR_PLAYER_EVENT_E)PLAYER_EVENT_TIMEOUT, 0, NULL);
		PLAYER_SET_CURRENT_CMD(play, jsonEvent->event, jsonEvent ); /* when enter into STOP state, this CMD is emitted, so re-enter into this function to start play */
	}
	else
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "Player has been in stop status");
	}

	return EXIT_SUCCESS;
}

static int	_pauseMedia(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int res;

	/* sep.18, 2018 */
	if(PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_PAUSE) )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOT_SUPPORT_COMMND, "'play' can't be executed when in PAUSE state");
		return EXIT_SUCCESS;
	}
	
	if(! PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_FORWARD) && 
		! PLAYER_CHECK_STATE(play, HI_SVR_PLAYER_STATE_BACKWARD) && 
		! PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE) )
	{

		res = HI_SVR_PLAYER_Pause( play->playerHandler);
		if (HI_SUCCESS != res)
		{
			MUX_PLAY_ERROR( "pause fail, ret = 0x%x", res);
			cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "pause failed, retry late");
			return EXIT_SUCCESS;
		}
		else
		{
			PLAYER_SET_CURRENT_CMD(play, jsonEvent->event, jsonEvent );
		}
	}
	else
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "Player can't pause in current status, retry late");
	}

	return EXIT_SUCCESS;
}


/* following are synchronous commands */
static int _getPlayerInfo(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int res;
	
	HI_SVR_PLAYER_INFO_S stPlayerInfo;
	/* Output player information */
	res = HI_SVR_PLAYER_GetPlayerInfo(play->playerHandler, &stPlayerInfo);
	if (HI_SUCCESS != res)
	{
//		MUX_PLAY_ERROR("HI_SVR_PLAYER_GetPlayerInfo fail!");
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "No player info is available now. Maybe Player never play yet or failed last time");
		return HI_SUCCESS;
	}

	JEVENT_ADD_STRING( jsonEvent->object, _MUX_JSON_NAME_MEDIA, play->currentUrl);
	JEVENT_ADD_STRING( jsonEvent->object, "playlist", play->currentPlaylist.name );
	JEVENT_ADD_INTEGER(jsonEvent->object, "progress", stPlayerInfo.u32Progress);

	int timeInSecond = (int)(stPlayerInfo.u64TimePlayed/1000);
	JEVENT_ADD_INTEGER(jsonEvent->object, "playedTime", timeInSecond); /* played time */

	JEVENT_ADD_STRING(jsonEvent->object, "playStatus", muxPlayerStateName(stPlayerInfo.eStatus));
	
	JEVENT_ADD_INTEGER(jsonEvent->object, "speed", stPlayerInfo.s32Speed); /* speed */

#if 0	
	timeInSecond = (int)(play->fileInfo->s64Duration/1000);
	cmnTlvWriteInteger(tlvBuf, timeInSecond);	/* total time */
	MUX_PLAY_DEBUG( "  Time played:  %lld:%lld:%lld, Total time: %lld:%lld:%lld ",
		stPlayerInfo.u64TimePlayed / (1000 * 3600),
		(stPlayerInfo.u64TimePlayed % (1000 * 3600)) / (1000 * 60),
		((stPlayerInfo.u64TimePlayed % (1000 * 3600)) % (1000 * 60)) / 1000,
		play->fileInfo->s64Duration / (1000 * 3600),
		(play->fileInfo->s64Duration % (1000 * 3600)) / (1000 * 60),
		(( play->fileInfo->s64Duration % (1000 * 3600)) % (1000 * 60)) / 1000);
	MUX_PLAY_DEBUG("  Speed:          %d ", stPlayerInfo.s32Speed);
	MUX_PLAY_DEBUG("  Status:         %d ", stPlayerInfo.eStatus);

	ret = HI_SVR_PLAYER_GetParam(play->playerHandler, HI_SVR_PLAYER_ATTR_STREAMID, (HI_VOID*)& play->playerStreamIDs);
	MUX_PLAY_DEBUG("  StreamId: program id(%d), video id(%d), audio id(%d), subtitle id(%d) \n",
		play->playerStreamIDs.u16ProgramId, play->playerStreamIDs.u16VidStreamId, 
		play->playerStreamIDs.u16AudStreamId, play->playerStreamIDs.u16SubStreamId);
#endif
	
	dataConn->errCode = IPCMD_ERR_NOERROR;
	return EXIT_SUCCESS;
}


static int _getMediaInfo(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int res;
	int i,j;
	
	HI_FORMAT_FILE_INFO_S			*fileInfo;
	cJSON *programs = NULL;

	res = HI_SVR_PLAYER_GetFileInfo(play->playerHandler, &fileInfo);
	if (HI_SUCCESS != res)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "No media info is available now. Maybe Player never play yet or failed last time");
		return HI_SUCCESS;
	}

	JEVENT_ADD_STRING(jsonEvent->object, "streamType", (HI_FORMAT_STREAM_TS == fileInfo->eStreamType)? "TS":"ES");

	char *sourceType;
	if (HI_FORMAT_SOURCE_NET_VOD == fileInfo->eSourceType)
		sourceType = "NETWORK VOD";
	else if (HI_FORMAT_SOURCE_NET_LIVE == fileInfo->eSourceType)
		sourceType = "NETWORK LIVE";
	else if (HI_FORMAT_SOURCE_LOCAL == fileInfo->eSourceType)
		sourceType = "LOCAL";
	else
		sourceType = "UNKNOWN";
	JEVENT_ADD_STRING(jsonEvent->object,"sourceType",sourceType);
	
	JEVENT_ADD_INTEGER(jsonEvent->object, "fileSize", fileInfo->s64FileSize);
	JEVENT_ADD_INTEGER(jsonEvent->object, "startTime", fileInfo->s64StartTime /1000);	/* seconds */
	JEVENT_ADD_INTEGER(jsonEvent->object, "duration", fileInfo->s64Duration /1000 * 3600); /* seconds */
	JEVENT_ADD_INTEGER(jsonEvent->object,  "bps", fileInfo->u32Bitrate);

#if 0
	JEVENT_ADD_ARRAY(jsonEvent->object, "programs", programs);
#else
	programs = jsonEvent->object;
#endif	
	for (i = 0; i < fileInfo->u32ProgramNum; i++)
	{
		cJSON *medias = NULL;

		if(fileInfo->pastProgramInfo[i].u32VidStreamNum > 0)
		{

			JEVENT_ADD_ARRAY(programs, "videos", medias);

			for (j = 0; j < fileInfo->pastProgramInfo[i].u32VidStreamNum; j++)
			{
				if (HI_FORMAT_INVALID_STREAM_ID != fileInfo->pastProgramInfo[i].pastVidStream[j].s32StreamIndex)
				{
					cJSON *video = cJSON_CreateObject();
					cJSON_AddItemToArray(medias, video);
					JEVENT_ADD_INTEGER(video, "streamIdx", fileInfo->pastProgramInfo[i].pastVidStream[j].s32StreamIndex);
					JEVENT_ADD_STRING(video, "format", muxGetVidFormatStr(fileInfo->pastProgramInfo[i].pastVidStream[j].u32Format));
					JEVENT_ADD_INTEGER(video, "width", fileInfo->pastProgramInfo[i].pastVidStream[j].u16Width);
					JEVENT_ADD_INTEGER(video, "height", fileInfo->pastProgramInfo[i].pastVidStream[j].u16Height);
					JEVENT_ADD_INTEGER(video, "fps", fileInfo->pastProgramInfo[i].pastVidStream[j].u16FpsInteger);//fileInfo->pastProgramInfo[i].pastVidStream[j].u16FpsDecimal);
					JEVENT_ADD_INTEGER(video, "bps", fileInfo->pastProgramInfo[i].pastVidStream[j].u32Bitrate);
				}
			}
		}
		else
		{
			JEVENT_ADD_STRING(programs, "videos", "video stream is null.");
		}

		if(fileInfo->pastProgramInfo[i].u32AudStreamNum >0)
		{

			JEVENT_ADD_ARRAY(programs, "audios", medias);
			
			for (j = 0; j < fileInfo->pastProgramInfo[i].u32AudStreamNum; j++)
			{
				cJSON *audio = cJSON_CreateObject();
				cJSON_AddItemToArray(medias, audio);
				JEVENT_ADD_INTEGER(audio, "streamIdx", fileInfo->pastProgramInfo[i].pastAudStream[j].s32StreamIndex);
				JEVENT_ADD_STRING(audio, "format", muxGetAudFormatStr(fileInfo->pastProgramInfo[i].pastAudStream[j].u32Format));
				JEVENT_ADD_INTEGER(audio, "profile", fileInfo->pastProgramInfo[i].pastAudStream[j].u32Profile);
				JEVENT_ADD_INTEGER(audio, "samplerate", fileInfo->pastProgramInfo[i].pastAudStream[j].u32SampleRate);
				JEVENT_ADD_INTEGER(audio, "bitpersample", fileInfo->pastProgramInfo[i].pastAudStream[j].u16BitPerSample);
				JEVENT_ADD_INTEGER(audio, "channels", fileInfo->pastProgramInfo[i].pastAudStream[j].u16Channels);
				JEVENT_ADD_INTEGER(audio, "bps", fileInfo->pastProgramInfo[i].pastAudStream[j].u32Bitrate);
				JEVENT_ADD_STRING(audio, "language", fileInfo->pastProgramInfo[i].pastAudStream[j].aszLanguage);
			}
		}

		if(fileInfo->pastProgramInfo[i].u32SubStreamNum > 0)
		{
			JEVENT_ADD_ARRAY(programs, "subtitles", medias);

			for (j = 0; j < fileInfo->pastProgramInfo[i].u32SubStreamNum; j++)
			{
				cJSON *subtitle = cJSON_CreateObject();
				cJSON_AddItemToArray(medias, subtitle);

				JEVENT_ADD_INTEGER(subtitle, "streamIdx", fileInfo->pastProgramInfo[i].pastSubStream[j].s32StreamIndex);
				JEVENT_ADD_STRING(subtitle, "format", muxGetSubtitleFormatStr(fileInfo->pastProgramInfo[i].pastSubStream[j].u32Format));
				JEVENT_ADD_INTEGER(subtitle, "extSub", fileInfo->pastProgramInfo[i].pastSubStream[j].bExtSub);
				JEVENT_ADD_INTEGER(subtitle, "originalWidth", fileInfo->pastProgramInfo[i].pastSubStream[j].u16OriginalFrameWidth);
				JEVENT_ADD_INTEGER(subtitle, "originalHeight", fileInfo->pastProgramInfo[i].pastSubStream[j].u16OriginalFrameHeight);
			}
		}
	}


	dataConn->errCode = IPCMD_ERR_NOERROR;
	return res;
}


static int _swapWindow(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int res = EXIT_SUCCESS;

	if ( play->windowIndex <= 0 || play->windowIndex >= cmn_list_size(&play->muxRx->muxPlayer->playerConfig.windows) )
	{
		MUX_PLAY_WARN("Window index '%d' is out of range", play->windowIndex );
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "Window index '%d' is out of range", play->windowIndex );
		return res;
	}

	res = muxPlayerSwapVideo(play->muxRx, play->windowIndex);
	if(res != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Please re-try later");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

 	return res;
}

static int _rotateWindow(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int res;

	play->rotateIndex++;
	if(play->rotateIndex >= ROTATE_TYPE_UNKNOWN)
	{
		play->rotateIndex = ROTATE_TYPE_0;
	}
	
	res = muxPlayerWindowRotate( play);
	if(res != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Please re-try later");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

 	return res;
}

static int _locateWindow(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int res;
	HI_RECT_S rect;

	rect.s32X = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_LOCATION_X);
	rect.s32Y = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_LOCATION_Y);
	rect.s32Width = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_LOCATION_W);
	rect.s32Height = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_LOCATION_H);
	play->cfg->left = rect.s32X;
	play->cfg->top = rect.s32Y;
	play->cfg->width = rect.s32Width;
	play->cfg->height = rect.s32Height;

	/* check validation of location parameters*/
		
	res = muxPlayerWindowLocate( play, &rect);
	if(res != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Please re-try later");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

 	return res;
}


static int _aspectWindow(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int res;
	int mode;

	mode = cmnGetIntegerFromJsonObject(jsonEvent->object, MUX_JSON_NAME_ASPECT_MODE);
	
	if(mode <MUX_DISPLAY_MODE_DEFAULT || mode >= MUX_DISPLAY_MODE_UNKNOWN)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Parameter 'mode' %d is not validate (%d-%d)", mode, MUX_DISPLAY_MODE_DEFAULT, MUX_DISPLAY_MODE_SQEEZE_PART);
		return EXIT_SUCCESS;
	}

	res = muxPlayerWindowAspect( play, mode);
	if(res != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Please re-try later");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

 	return res;
}


static int	__muxSetVolume(MUX_PLAY_T *play, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	int res = EXIT_SUCCESS;
	HI_UNF_SND_GAIN_ATTR_S gain;
	
	gain.bLinearMode = HI_TRUE;
	gain.s32Gain = play->volume;

	res = HI_UNF_SND_SetTrackWeight(play->trackHandle, &gain);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("set volume failed! \n");
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Please re-try later");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}
	
	return res;
}


static int	_muxVolumePlus(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;

	if(play->volume < 100)
		play->volume++;
	return __muxSetVolume(play, dataConn, jsonEvent);
}


static int	_muxVolumeMinus(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;

	if(play->volume > 0)
		play->volume--;
	return __muxSetVolume(play, dataConn, jsonEvent);
}


static int	_muxMute(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int res = EXIT_SUCCESS;
	if(play->cfg->audioEnable)
		play->cfg->audioEnable = FALSE;
	else
		play->cfg->audioEnable = TRUE;

	res = HI_UNF_SND_SetTrackMute(play->trackHandle, !play->cfg->audioEnable);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_WARN("Set window No.%d mute failed : %#x", play->windowIndex, res);
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Please re-try later");
	}
	else
	{
		JEVENT_ADD_STRING(jsonEvent->object, "AudioEnable", STR_BOOL_VALUE(play->cfg->audioEnable));
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return res;
}

static int	_muxAudio(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)priv;
	int res = EXIT_SUCCESS;
	char	*strState;

	strState = cmnGetStrFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_MEDIA);
	if(IS_STRING_NULL(strState))
	{
		MUX_PLAY_WARN("No 'Enable' parameter for audio channel in 'audio' command");
		return cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "No 'Enable' parameter for audio channel in 'audio' command");
	}

	if( !IS_STRING_EQUAL(strState, "YES") 
		&&  !IS_STRING_EQUAL(strState, "NO")  )
	{
		return cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'Enable' parameter for audio channel is not 'YES' or 'NO'");
	}
	
	if(IS_STRING_EQUAL(strState, "YES"))
		play->cfg->audioEnable = TRUE;
	else
		play->cfg->audioEnable = FALSE;

	res = HI_UNF_SND_SetTrackMute(play->trackHandle, !play->cfg->audioEnable);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_WARN("Set window No.%d mute failed : %#x", play->windowIndex, res);
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Please re-try later");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return res;
}



static PluginJSonHandler _jsonPlayerActionHandlers[] =
{
	{
		type	: CMD_TYPE_FORWARD,
		name 	: "forward",
		handler	: _forwardMedia
	},
	{
		type	: CMD_TYPE_BACKWARD,
		name 	: "backward",
		handler	: _backwardMedia
	},
	{
		type	: CMD_TYPE_OPEN,
		name 	: "play",
		handler	: _playMedia
	},
	{
		type	: CMD_TYPE_STOP,
		name 	: "stop",
		handler	: _stopMedia
	},
	{
		type	: CMD_TYPE_PAUSE,
		name 	: "pause",
		handler	: _pauseMedia
	},
	{
		type	: CMD_TYPE_RESUME,
		name 	: "resume",
		handler	: _resumeMedia
	},

	/* synchronous command */
	{
		type	: CMD_TYPE_INFO,
		name 	: "playerInfo",
		handler	: _getPlayerInfo
	},
	{
		type	: CMD_TYPE_GET_MEDIA,
		name 	: "mediaInfo",
		handler	: _getMediaInfo
	},
	
	/* window control, synchronous command*/	
	{
		type	: CMD_TYPE_SWAP_WINDOW,
		name 	: "swapWindow",
		handler	: _swapWindow
	},
	{
		type	: CMD_TYPE_ROTATE_WINDOW,
		name 	: "rotateWindow",
		handler	: _rotateWindow
	},
	{
		type	: CMD_TYPE_LOCATE_WINDOW,
		name 	: "locateWindow",
		handler	: _locateWindow
	},

	{
		type	: CMD_TYPE_ASPECT_WINDOW,
		name 	: IPCMD_NAME_ASPECT_WINDOW,
		handler	: _aspectWindow
	},
	
	{
		type	: CMD_TYPE_AUDIO,
		name 	: IPCMD_NAME_PLAYER_AUDIO,
		handler	: _muxAudio
	},
	{
		type	: CMD_TYPE_MUTE,
		name 	: IPCMD_NAME_PLAYER_MUTE,
		handler	: _muxMute
	},
	{
		type	: CMD_TYPE_VOLUME_PLUS,
		name 	: "vol+",
		handler	: _muxVolumePlus
	},
	{
		type	: CMD_TYPE_VOLUME_MINUS,
		name 	: "vol-",
		handler	: _muxVolumeMinus
	},

	{
		type	: CMD_TYPE_UNKNOWN,
		name	: NULL,
		handler	: NULL
	}
};

char *muxPlayerJsonCmdName(int type)
{
	PluginJSonHandler *handler = _jsonPlayerActionHandlers;

	while( handler->handler)
	{
		if(handler->type ==type )
		{
			return handler->name;
		}
		
		handler++;
	}

	return "UnknowCommand";
}

extern	PluginJSonHandler jsonMuxPlayActionHandlers[];

int muxPlayerJSONHandle(MUX_RX_T *muxRx, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	int winIndex;
	int ret = EXIT_SUCCESS;
	
	jsonEvent->status = IPCMD_ERR_NOT_SUPPORT_COMMND; 
	winIndex = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_WINDOW);
	if(winIndex == -1)
	{/* JSON handlers not related with PLAYER */
		return cmnMuxJSonPluginHandle(muxRx, jsonMuxPlayActionHandlers, jsonEvent);
	}
	else
	{/* JSON handlers for PLAYER, some command will delay for several seconds to reply */
		struct DATA_CONN *dataConn =(struct DATA_CONN *)jsonEvent->priv;
		if(!dataConn)
		{
			MUX_ERROR("DataConn is null for JSON reply");
			exit(1);
		}
		cmn_mutex_lock(dataConn->mutexLock);
		dataConn->errCode = IPCMD_ERR_NOT_SUPPORT_COMMND;

		MUX_PLAY_T *play = muxPlayerFindByIndex( muxRx, winIndex);
		if(play == NULL)
		{
			MUX_PLAY_ERROR("No window with index %d is found", winIndex);
			cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "Window index is invalidate");
		}
		else
		{
			PluginJSonHandler *handler = _jsonPlayerActionHandlers;

			if(IS_PLAYER_BUSY(play))
			{
				MUX_PLAY_ERROR( "Player %s failed on '%s': is busy for last command '%s'", play->muxFsm.name,  jsonEvent->action, muxPlayerJsonCmdName(play->muxFsm.currentCmd) );
//				MUX_PLAY_ERROR( "Player %s failed : is busy for last command '%s'", play->muxFsm.name,  muxPlayerJsonCmdName(play->muxFsm.currentCmd) );
				cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_IS_BUSY_NOW, "Player %s failed on %s: is still processing last command '%s'", play->muxFsm.name,  jsonEvent->action, muxPlayerJsonCmdName(play->muxFsm.currentCmd) );
			}
			else
			{
#if 1			
	//			jsonEvent->status = IPCMD_ERR_NOT_SUPPORT_COMMND; 
				while( handler->handler)
				{
					if(!strcasecmp( handler->name, jsonEvent->action))
					{
						MUX_PLAY_DEBUG("'%s' is processing '%s' action", play->muxFsm.name, handler->name );
						dataConn->errCode = IPCMD_ERR_IN_PROCESSING; 
						jsonEvent->status = IPCMD_ERR_IN_PROCESSING; 
						jsonEvent->event = handler->type;
						
						ret = handler->handler(play, dataConn, jsonEvent);

						break;
					}
					
					handler++;
				}
#else
#endif
			}

		}

		cmn_mutex_unlock(dataConn->mutexLock);

		cmnMuxJsonPluginReplay(dataConn, jsonEvent);
	}

	ret = EXIT_SUCCESS;
	return ret;
}


