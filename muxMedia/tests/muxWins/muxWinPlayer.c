
#include "_muxWin.h"


int muxWinSetMedia(MWIN_PLAY_T *players, MWIN_PLAYER_T *play, int index)
{
	HI_S32  res;
	HI_HANDLE hSo = HI_SVR_PLAYER_INVALID_HDL;
	HI_HANDLE hAVPlay = HI_SVR_PLAYER_INVALID_HDL;
	HI_HANDLE hWindow = HI_SVR_PLAYER_INVALID_HDL;
	HI_HANDLE hTrack = HI_SVR_PLAYER_INVALID_HDL;
	
	memset(&play->media, 0, sizeof(HI_SVR_PLAYER_MEDIA_S));
	if(index == 0)
	{
//		sprintf(play->media.aszUrl, "%s", MUX_PATH_OF_USBDISK"/iMPACT.Wrestling.2015.11.05.1080p-30.WEBRip.h264-spamTV.mkv");
		sprintf(play->media.aszUrl, "%s", "/g05.mov");
//		sprintf(play->media.aszUrl, "%s", "/transformers-5-big-game-spot-2_h720p.mov");
	}
	else
	{
		sprintf(play->media.aszUrl, "%s", MUX_PATH_OF_USBDISK"/Ducks.Take.Off.2160p.h.265-Zion.mkv");
//		sprintf(play->media.aszUrl, "%s", MUX_PATH_OF_USBDISK"/bbb.mp4");
	}
		
//	sprintf(play->media.aszUrl, "%s", "/media/usb/Marvels.mkv");
	play->media.u32ExtSubNum = 0;
	play->media.u32UserData = 0;

	MUX_DEBUG("### open media file is %s \n", play->media.aszUrl);
	cmnThreadSetName("SetMedia");
	res = HI_SVR_PLAYER_SetMedia( play->hiPlayer, HI_SVR_PLAYER_MEDIA_STREAMFILE, &play->media);
	MUX_DEBUG( "\t\t\t### set media file '%s' return now\n", play->media.aszUrl);
	if (HI_SUCCESS != res )
	{
		MUX_ERROR("HI_SVR_PLAYER_SetMedia err, ret = 0x%x, str:%s!", res, strerror(errno));
		return HI_FAILURE;
	}
	else
	{
		MUX_ERROR("Success: HI_SVR_PLAYER_SetMedia sucess, ret = 0x%x!", res);
	}

	cmnThreadSetName("SetMedia2");
	res |= HI_SVR_PLAYER_GetParam(play->hiPlayer, HI_SVR_PLAYER_ATTR_SO_HDL, &hSo);
	if (HI_SUCCESS != res )
	{
		MUX_ERROR("HI_SVR_PLAYER_GetParam HI_SVR_PLAYER_ATTR_SO_HDL err, ret = 0x%x!", res);
		return HI_FAILURE;
	}

	cmnThreadSetName("SetMedia3");
	res |= HI_SVR_PLAYER_GetParam(play->hiPlayer, HI_SVR_PLAYER_ATTR_AVPLAYER_HDL, &hAVPlay);
	if (HI_SUCCESS != res )
	{
		MUX_ERROR("HI_SVR_PLAYER_GetParam HI_SVR_PLAYER_ATTR_AVPLAYER_HDL err, ret = 0x%x!", res);
		return HI_FAILURE;
	}

	
	MWIN_AVPLAY_T *avPlay = (MWIN_AVPLAY_T *)cmn_list_get(&players->avPlays, index);
	if(avPlay == NULL)
	{
		MUX_ERROR("avPlay == NULL");
		return HI_FAILURE;
	}
	
	if( hAVPlay != avPlay->handleAvPlay)
	{
		MUX_ERROR("Old AvPlay Handle is 0x%x, new is 0x%x", avPlay->handleAvPlay, hAVPlay);
		avPlay->handleAvPlay = hAVPlay;
	}

#if 0	
	res |= HI_SVR_PLAYER_GetParam(play->hiPlayer, HI_SVR_PLAYER_ATTR_WINDOW_HDL, &hWindow);
	if (HI_SUCCESS != res )
	{
		MUX_ERROR("HI_SVR_PLAYER_GetParam HI_SVR_PLAYER_ATTR_WINDOW_HDL err, ret = 0x%x!", res);
	getchar();
		return HI_FAILURE;
	}
	res |= HI_SVR_PLAYER_GetParam(play->hiPlayer, HI_SVR_PLAYER_ATTR_AUDTRACK_HDL, &hTrack);
	if (HI_SUCCESS != res )
	{
		MUX_ERROR( "HI_SVR_PLAYER_GetParam HI_SVR_PLAYER_ATTR_AUDTRACK_HDL err, ret = 0x%x!", res);
		return HI_FAILURE;
	}
	if( hWindow != avPlay->handleWin)
	{
		MUX_ERROR("Old Window Handle is 0x%x, new is 0x%x", avPlay->handleWin, hWindow);
		avPlay->handleAvPlay = hWindow;
	}
	if( hTrack != avPlay->handleTrack)
	{
		MUX_ERROR("Old Track Handle is 0x%x, new is 0x%x", avPlay->handleTrack, hTrack);
		avPlay->handleTrack = hTrack;
	}

	HI_UNF_WINDOW_ATTR_S stWinAttr;
	memset(&stWinAttr, 0, sizeof(HI_UNF_WINDOW_ATTR_S));
	stWinAttr.stOutputRect.s32X = 0;
	stWinAttr.stOutputRect.s32Y = 0 + index*300;
	stWinAttr.stOutputRect.s32Width = 1280;
	stWinAttr.stOutputRect.s32Height = 300;

	res = HI_UNF_VO_SetWindowAttr(avPlay->handleWin, &stWinAttr);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("play fail, ret = 0x%x", res);
		return HI_FAILURE;
	}
#endif

	TRACE();
#if WITH_HI_AVPLAY
	res = muxWinConnectAvPlay(avPlay);
#endif	
	TRACE();

#if 0
			res |= HI_SVR_PLAYER_Invoke(play->hiPlayer, HI_FORMAT_INVOKE_PRE_CLOSE_FILE, NULL);
			if(res != HI_SUCCESS)
			{
				MUX_ERROR("Invoke Close File in HiPlayer failed :0x%x", res);
			}
#endif

	cmnThreadSetName("Play");
	res = HI_SVR_PLAYER_Play(play->hiPlayer);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("play fail, ret = 0x%x", res);
		return HI_FAILURE;
	}
	TRACE();

	return HI_SUCCESS;
}


int muxWinInitMultipleHiplayer(MWIN_PLAY_T *players)
{
	HI_S32 ret = 0;
	HI_S32 i = 0;

	int count = cmn_list_size(&players->avPlays);

	for(i=0; i< count; i++)
	{

		MWIN_AVPLAY_T *avPlay = (MWIN_AVPLAY_T *)cmn_list_get(&players->avPlays, i);
		if(avPlay == NULL)
		{
			MUX_ERROR(" AvPlay in list is null" );
			return HI_FAILURE;
		}

		MWIN_PLAYER_T *play = (MWIN_PLAYER_T *)cmn_malloc(sizeof(MWIN_PLAYER_T));

		memset(&play->playerParam, 0, sizeof(HI_SVR_PLAYER_PARAM_S));

#if WITH_HI_AVPLAY
		play->playerParam.hAVPlayer = HI_SVR_PLAYER_INVALID_HDL;
#else
		play->playerParam.hAVPlayer = avPlay->handleAvPlay;
#endif
		play->playerParam.hDRMClient = HI_SVR_PLAYER_INVALID_HDL;
		
		cmnThreadSetName("HiPlay%d", i);
		ret = HI_SVR_PLAYER_Create(& play->playerParam, &play->hiPlayer);
		if (HI_SUCCESS != ret)
		{
			MUX_ERROR("player open fail, ret = 0x%x ", ret);
			return HI_FAILURE;
		}
#if 0
		HI_U32 u32LogLevel = HI_FORMAT_LOG_INFO;
#else
		HI_U32 u32LogLevel = HI_FORMAT_LOG_ERROR;
#endif
		HI_SVR_PLAYER_Invoke( play->hiPlayer, HI_FORMAT_INVOKE_SET_LOG_LEVEL, &u32LogLevel);
		HI_SVR_PLAYER_EnableDbg(HI_TRUE);


		ret = HI_SVR_PLAYER_RegCallback(play->hiPlayer, muxWinPlayerEventCallBack);
		if (HI_SUCCESS != ret)
		{
			MUX_ERROR( "register event callback function fail, ret = 0x%x", ret);
		}



//	_muxInitBufferConfig(mux);
		ret = muxWinSetMedia( players, play, i);

		cmn_list_append( &players->players, play);

	}
	
	return ret;
}

