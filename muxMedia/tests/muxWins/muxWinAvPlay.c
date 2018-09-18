
#include "_muxWin.h"

int muxWinInitMultipleAvPlay(MWIN_PLAY_T *player)
{
	int	res;
	int	i;
	HI_UNF_ENC_FMT_E	vidFormat = HI_UNF_ENC_FMT_1080P_50;
	MWIN_AVPLAY_T *avPlay = NULL;
#if 0
	HI_UNF_SYNC_ATTR_S	syncAttr;
#endif

	cmnThreadSetName("HiSys");
	HI_SYS_Init();

	cmnThreadSetName("MCE");
	HIADP_MCE_Exit();

	cmnThreadSetName("Sound");
	res = HIADP_Snd_Init();
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("call HIADP_Snd_Init failed.");
		return HI_FAILURE;
	}

	cmnThreadSetName("Display");
	res = HIADP_Disp_Init(vidFormat);
	if (HI_SUCCESS != res )
	{
		MUX_ERROR("HIADP_Disp_Init failed");
		return HI_FAILURE;
	}

	cmnThreadSetName("VO");
	res = HIADP_VO_Init(HI_UNF_VO_DEV_MODE_MOSAIC);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HIADP_VO_Init failed");
		return HI_FAILURE;
	}

	res = HIADP_AVPlay_RegADecLib();
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HIADP_AVPlay_RegADecLib failed");
		return HI_FAILURE;
	}

	cmnThreadSetName("DMX");
	res = HI_UNF_DMX_Init();
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_DMX_Init failed");
		goto ERR;
	}

	cmnThreadSetName("AVPlayInit");
	res = HI_UNF_AVPLAY_Init();
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_AVPLAY_Init failed");
		return HI_FAILURE;
	}

	cmnThreadSetName("SO");
	res = HI_UNF_SO_Init();
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_SO_Init failed");
		return HI_FAILURE;
	}


	cmnThreadSetName("HiPlayInit");
	res = HI_SVR_PLAYER_Init();
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("player init fail, ret = 0x%x", res);
		return HI_FAILURE;
	}

	HI_FORMAT_LIB_VERSION_S stPlayerVersion;
	memset(&stPlayerVersion, 0, sizeof(stPlayerVersion));
	HI_SVR_PLAYER_GetVersion(&stPlayerVersion);
	MUX_DEBUG( "HiPlayer V%u.%u.%u.%u", stPlayerVersion.u8VersionMajor, stPlayerVersion.u8VersionMinor, stPlayerVersion.u8Revision, stPlayerVersion.u8Step);

	/* Regist file demuxers */
	res = HI_SVR_PLAYER_RegisterDynamic(HI_SVR_PLAYER_DLL_PARSER, "libformat.so");
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("register file parser libformat.so fail, ret=0x%x", res);
	}

	(HI_VOID)HI_SVR_PLAYER_RegisterDynamic(HI_SVR_PLAYER_DLL_PARSER, "libffmpegformat.so");
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("register file parser libffmpegformat.so fail, ret=0x%x ", res);
	}
	
#if 0
	usleep(3000 * 1000);
#endif


	for (i = 0; i < player->count; i++)
	{
		HI_RECT_S WinRect;


		avPlay = cmn_malloc(sizeof(MWIN_AVPLAY_T));
		if(avPlay == NULL)
		{
			MUX_ERROR("mem allocation for MUX_AVPLAY failed");
			exit(1);
		}
		avPlay->handleAvPlay = HI_INVALID_HANDLE;
		avPlay->handleWin= HI_INVALID_HANDLE;
		avPlay->handleTrack= HI_INVALID_HANDLE;

#if WITH_HI_AVPLAY
		avPlay->handleAvPlay = HI_SVR_PLAYER_INVALID_HDL;
#else
		/******* init AvPlay for this media */
		HI_UNF_AVPLAY_ATTR_S        AvplayAttr;

		HI_UNF_AVPLAY_GetDefaultConfig(&AvplayAttr, HI_UNF_AVPLAY_STREAM_TYPE_TS);

		AvplayAttr.u32DemuxId = i;
		AvplayAttr.u32DemuxId = 0;
		AvplayAttr.stStreamAttr.u32VidBufSize = 8 * 1024 * 1024;

		cmnThreadSetName("AvPlay%d", i);
		res = HI_UNF_AVPLAY_Create(&AvplayAttr, &avPlay->handleAvPlay );
		if (HI_SUCCESS != res)
		{
			MUX_ERROR("HI_UNF_AVPLAY_Create failed");
			goto ERR;
		}
#endif

		/******* init window for this media */
		if(player->count == 1)
		{
			WinRect.s32Width    = 1920;
			WinRect.s32Height   = 720;
			WinRect.s32X = 0;
			WinRect.s32Y = 0;
		}
		else
		{
			WinRect.s32Width    = 1920;//1280 / (countWin/ 2);
			WinRect.s32Height   = 1080 / 2;
			WinRect.s32X = 0; //i % (countWin / 2) * WinRect.s32Width;
			WinRect.s32Y = i / (player->count / 2) * WinRect.s32Height;
		}

		MUX_DEBUG( "x=%d, y=%d, w=%d, h=%d", WinRect.s32X, WinRect.s32Y, WinRect.s32Width, WinRect.s32Height);

		cmnThreadSetName("VOWindow");
		res = HIADP_VO_CreatWin(&WinRect, &avPlay->handleWin);
		if (HI_SUCCESS != res)
		{
			MUX_ERROR("HIADP_VO_CreatWin failed");
			goto ERR;
		}

	
		/******* init track for this media */
		HI_UNF_AUDIOTRACK_ATTR_S	trackAttr;
		if( i == player->masterIndex )
		{
			trackAttr.enTrackType = HI_UNF_SND_TRACK_TYPE_MASTER;
			res = HI_UNF_SND_GetDefaultTrackAttr(HI_UNF_SND_TRACK_TYPE_MASTER, &trackAttr);
			if (HI_SUCCESS != res)
			{
				MUX_ERROR("HI_UNF_SND_GetDefaultTrackAttr failed");
				goto ERR;
			}

		}
		else
		{
			trackAttr.enTrackType = HI_UNF_SND_TRACK_TYPE_SLAVE;
			res = HI_UNF_SND_GetDefaultTrackAttr(HI_UNF_SND_TRACK_TYPE_SLAVE, &trackAttr);
			if (HI_SUCCESS != res)
			{
				MUX_ERROR("HI_UNF_SND_GetDefaultTrackAttr failed");
				goto ERR;
			}
		}

		cmnThreadSetName("SNDTrack");
		res = HI_UNF_SND_CreateTrack(HI_UNF_SND_0, &trackAttr, &avPlay->handleTrack);
		if (HI_SUCCESS != res)
		{
			MUX_ERROR("HI_UNF_SND_CreateTrack failed");
			goto ERR;
		}

		if(i == player->masterIndex)
		{
			player->masterTrack = avPlay->handleTrack;
		}
		else
		{
			res = HI_UNF_SND_SetTrackMute(avPlay->handleTrack, HI_TRUE);
			if (HI_SUCCESS != res)
			{
				MUX_ERROR("HI_UNF_SND_SetTrackMute failed");
				goto ERR;
			}
		}

#if WITH_HI_AVPLAY
#else			
		res = muxWinConnectAvPlay(avPlay);
		if(res != HI_SUCCESS)
			goto ERR;
#endif

#if 0
			HI_UNF_AVPLAY_GetAttr(avPlay->handleAvPlay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &syncAttr);

			syncAttr.enSyncRef = HI_UNF_SYNC_REF_AUDIO;
			res = HI_UNF_AVPLAY_SetAttr(avPlay->handleAvPlay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &syncAttr);
			if (HI_SUCCESS != res )
			{
				MUX_ERROR("HI_UNF_AVPLAY_SetAttr failed");
				goto ERR;
			}
#endif

		cmn_list_append(&player->avPlays, avPlay);

		avPlay = NULL;
	}

	return HI_SUCCESS;

ERR:
	if(avPlay)
	{
		cmn_free(avPlay);
	}

	return HI_FAILURE;
}


int muxWinSwapVideo(MWIN_PLAY_T *player, int newMasterIndex)
{
	int res;
	MWIN_AVPLAY_T *oldMaster, *newMaster;

	MUX_DEBUG("New index No. %d of total video of %d", newMasterIndex, player->count );
	if(newMasterIndex >= player->count)
	{
		MUX_ERROR("New index No. %d is out of total video of %d", newMasterIndex, player->count );
		return HI_FAILURE;
	}
	
	if(newMasterIndex == player->masterIndex)
	{
		MUX_ERROR("Assign same index for master video");
		return HI_SUCCESS;
	}

	oldMaster = (MWIN_AVPLAY_T *)cmn_list_get(&player->avPlays, player->masterIndex);
	newMaster = (MWIN_AVPLAY_T *)cmn_list_get(&player->avPlays, newMasterIndex);
	if(oldMaster == NULL || newMaster == NULL)
	{
		MUX_ERROR( "Null AvPlay");
		return HI_SUCCESS;
	}

TRACE();
	res = HI_UNF_AVPLAY_Pause(oldMaster->handleAvPlay, HI_NULL);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_AVPLAY_Pause 1 failed");
		return HI_FAILURE;
	}
	res = HI_UNF_AVPLAY_Pause(newMaster->handleAvPlay, HI_NULL);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_AVPLAY_Pause 2 failed");
		return HI_FAILURE;
	}

TRACE();
	res = HI_UNF_VO_SetWindowEnable( oldMaster->handleWin, HI_FALSE);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_VO_SetWindowEnable failed");
		return HI_FAILURE;
	}

TRACE();
	res = HI_UNF_VO_SetWindowEnable( newMaster->handleWin, HI_FALSE);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_VO_SetWindowEnable failed");
		return HI_FAILURE;
	}
	
	
TRACE();
	res = HI_UNF_VO_DetachWindow(oldMaster->handleWin, oldMaster->handleAvPlay);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_VO_DetachWindow failed");
		return HI_FAILURE;
	}


TRACE();
	res = HI_UNF_VO_DetachWindow(newMaster->handleWin, newMaster->handleAvPlay);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_VO_DetachWindow failed");
		return HI_FAILURE;
	}

	HI_HANDLE temp = oldMaster->handleWin;
	oldMaster->handleWin = newMaster->handleWin;
	newMaster->handleWin = temp;


TRACE();
	res = HI_UNF_VO_AttachWindow(oldMaster->handleWin, oldMaster->handleAvPlay);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR( "HI_UNF_VO_AttachWindow failed");
		return HI_FAILURE;
	}

TRACE();
	res = HI_UNF_VO_AttachWindow(newMaster->handleWin, newMaster->handleAvPlay);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_VO_AttachWindow failed");
		return HI_FAILURE;
	}
	
TRACE();
	res = HI_UNF_VO_SetWindowEnable(newMaster->handleWin, HI_TRUE);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_VO_SetWindowEnable failed");
		return HI_FAILURE;
	}

TRACE();
	res = HI_UNF_VO_SetWindowEnable(oldMaster->handleWin, HI_TRUE);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_VO_SetWindowEnable failed");
		return HI_FAILURE;
	}
	
TRACE();
	res = HI_UNF_SND_SetTrackMute( player->masterTrack, HI_TRUE); /* mute old master track */
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_SND_SetTrackMute failed on old master" );
		return HI_FAILURE;
	}
	
	player->masterTrack = newMaster->handleTrack;
	player->masterIndex = newMasterIndex;
		
	res = HI_UNF_SND_SetTrackMute( player->masterTrack, HI_FALSE); /* enable new master track */
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_SND_SetTrackMute failed on master" );
		return HI_FAILURE;
	}
	
#if 0
	for(i=0; i< count; i++)
	{
		HI_UNF_SYNC_ATTR_S	syncAttr;
	
		res = HI_UNF_VO_SetWindowEnable( avPlays[i]->handleWin, HI_TRUE);
		if (HI_SUCCESS != res)
		{
			MUX_ERROR("HI_UNF_VO_SetWindowEnable failed");
			return HI_FAILURE;
		}

		
TRACE();
		res = HI_SVR_PLAYER_Play(plays[i]->hiPlayer);
		res = HI_SVR_PLAYER_Resume(plays[i]->hiPlayer);
		if (HI_SUCCESS != res)
		{
			MUX_ERROR("stop fail, ret = 0x%x", res);
			return HI_FAILURE;
		}
		HI_UNF_AVPLAY_GetAttr(avPlays[i]->handleAvPlay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &syncAttr);
		syncAttr.enSyncRef = HI_UNF_SYNC_REF_AUDIO;
		res = HI_UNF_AVPLAY_SetAttr(avPlays[i]->handleAvPlay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &syncAttr);
		if (HI_SUCCESS != res )
		{
			MUX_ERROR("HI_UNF_AVPLAY_SetAttr failed");
			return HI_FAILURE;
		}

	}
#endif

	res = HI_UNF_AVPLAY_Resume(oldMaster->handleAvPlay, HI_NULL);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_AVPLAY_Resume 1 failed");
		return HI_FAILURE;
	}
	res = HI_UNF_AVPLAY_Resume(newMaster->handleAvPlay, HI_NULL);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_AVPLAY_Resume 2 failed");
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

int muxWinRotateVideo(MWIN_PLAY_T *player, HI_UNF_VO_ROTATION_E mode)
{
	int res;
	int i;

	for(i=0; i< player->count; i++)
	{
		MWIN_AVPLAY_T *avPlay = (MWIN_AVPLAY_T *)cmn_list_get(&player->avPlays, i);
		if(avPlay == NULL)
		{
			MUX_ERROR("AvPlay in list is null\e[0m \n");
			return HI_FAILURE;
		}
		
		res = HI_UNF_VO_SetRotation(avPlay->handleWin, mode);
		if(res != HI_SUCCESS)
		{
			MUX_ERROR("HI_UNF_VO_SetRotation failed : 0x%x", res);
			return HI_FAILURE;
		}
	}

	return HI_SUCCESS;
}

/* AvPlay and window/track are created indepdently, then connect them together. This can be used in both external AvPlay or internal AvPlay of HiPlayer */
int muxWinConnectAvPlay(MWIN_AVPLAY_T *avPlay)
{
	int res = HI_SUCCESS;

	/* connect window channel */
#if 0
	HI_UNF_AVPLAY_OPEN_OPT_S    AvPlayOpenOpt;

	AvPlayOpenOpt.enDecType         = HI_UNF_VCODEC_DEC_TYPE_NORMAL;
	AvPlayOpenOpt.enCapLevel        = HI_UNF_VCODEC_CAP_LEVEL_D1;
	AvPlayOpenOpt.enProtocolLevel   = HI_UNF_VCODEC_PRTCL_LEVEL_H264;
	res = HI_UNF_AVPLAY_ChnOpen(avPlay->handleAvPlay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, &AvPlayOpenOpt);
#else	
	res = HI_UNF_AVPLAY_ChnOpen(avPlay->handleAvPlay, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);
#endif	
	if (HI_SUCCESS != res )
	{
		MUX_ERROR( "HI_UNF_AVPLAY_ChnOpen for VID failed : 0x%x", res);
		return HI_FAILURE;
	}

	res = HI_UNF_VO_AttachWindow(avPlay->handleWin, avPlay->handleAvPlay);
	if (HI_SUCCESS != res )
	{
		MUX_ERROR("HI_UNF_VO_AttachWindow failed");
		return HI_FAILURE;
	}

	res = HI_UNF_VO_SetWindowEnable( avPlay->handleWin, HI_TRUE);
	if (HI_SUCCESS != res)
	{
		MUX_ERROR( "HI_UNF_VO_SetWindowEnable failed");
		return HI_FAILURE;
	}

	/* connect track */
	res = HI_UNF_AVPLAY_ChnOpen(avPlay->handleAvPlay, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
	if (HI_SUCCESS != res )
	{
		MUX_ERROR("HI_UNF_AVPLAY_ChnOpen for AID failed : 0x%x", res);
		return HI_FAILURE;
	}

	res = HI_UNF_SND_Attach(avPlay->handleTrack, avPlay->handleAvPlay );
	if (HI_SUCCESS != res)
	{
		MUX_ERROR("HI_UNF_SND_Attach failed");
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

