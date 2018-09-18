
#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"

int muxPlayerSetMainWindowTop(MUX_RX_T *muxRx)
{
	int res = 0;
	
	MUX_PLAY_T *mainPlay = muxPlayerFindByType(muxRx, RECT_TYPE_MAIN);
	if(mainPlay == NULL)
	{
		MUX_PLAY_ERROR("No Main Window is initialized, please check your configuration!");
		return HI_FAILURE;
	}

	res =  HI_UNF_VO_SetWindowZorder(mainPlay->windowHandle, HI_LAYER_ZORDER_MOVETOP);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("Main window can't set to top!");
		return HI_FAILURE;
	}

	return res;
}


static int _swapPlayers(MUX_PLAY_T *currentMainPlayer, MUX_PLAY_T *newMainPlayer)
{
	int res = 0;

	res = HI_UNF_AVPLAY_Pause(currentMainPlayer->avPlayHandler, HI_NULL);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_AVPLAY_Pause 1 failed");
		return HI_FAILURE;
	}
	res = HI_UNF_AVPLAY_Pause(newMainPlayer->avPlayHandler, HI_NULL);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_AVPLAY_Pause 2 failed");
		return HI_FAILURE;
	}


	res = HI_UNF_VO_SetWindowEnable( currentMainPlayer->windowHandle, HI_FALSE);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_SetWindowEnable failed");
		return HI_FAILURE;
	}
	res = HI_UNF_VO_SetWindowEnable( newMainPlayer->windowHandle, HI_FALSE);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_SetWindowEnable failed");
		return HI_FAILURE;
	}

	
	res = HI_UNF_VO_DetachWindow(currentMainPlayer->windowHandle, currentMainPlayer->avPlayHandler);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_DetachWindow failed");
		return HI_FAILURE;
	}
	res = HI_UNF_VO_DetachWindow(newMainPlayer->windowHandle, newMainPlayer->avPlayHandler);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_DetachWindow failed");
		return HI_FAILURE;
	}

	RECT_CONFIG *cfg = currentMainPlayer->cfg;
	HI_HANDLE temp = currentMainPlayer->windowHandle;
	currentMainPlayer->cfg = newMainPlayer->cfg;
	currentMainPlayer->windowHandle = newMainPlayer->windowHandle;

	newMainPlayer->windowHandle = temp;
	newMainPlayer->cfg = cfg;


	res = HI_UNF_VO_AttachWindow(currentMainPlayer->windowHandle, currentMainPlayer->avPlayHandler);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_AttachWindow failed");
		return HI_FAILURE;
	}
	res = HI_UNF_VO_AttachWindow(newMainPlayer->windowHandle, newMainPlayer->avPlayHandler);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_AttachWindow failed");
		return HI_FAILURE;
	}

	/* master window must be enabled at first */
	res = HI_UNF_VO_SetWindowEnable(newMainPlayer->windowHandle, HI_TRUE);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_SetWindowEnable failed");
		return HI_FAILURE;
	}
	res = HI_UNF_VO_SetWindowEnable(currentMainPlayer->windowHandle, HI_TRUE);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_SetWindowEnable failed");
		return HI_FAILURE;
	}


	/* swap track */	
	res = HI_UNF_SND_SetTrackMute( currentMainPlayer->trackHandle, HI_TRUE); /* mute old master track */
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_SND_SetTrackMute failed on old master" );
		return HI_FAILURE;
	}
	
	currentMainPlayer->muxRx->masterTrack = newMainPlayer->trackHandle;
		
	res = HI_UNF_SND_SetTrackMute( newMainPlayer->trackHandle, HI_FALSE); /* enable new master track */
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_SND_SetTrackMute failed on master" );
		return HI_FAILURE;
	}
	
#if 0
	for(i=0; i< count; i++)
	{
		HI_UNF_SYNC_ATTR_S	syncAttr;
	
		res = HI_UNF_VO_SetWindowEnable( avPlays[i]->handleWin, HI_TRUE);
		if (HI_SUCCESS != res)
		{
			MUX_PLAY_ERROR("HI_UNF_VO_SetWindowEnable failed");
			return HI_FAILURE;
		}

		
TRACE();
		res = HI_SVR_PLAYER_Play(plays[i]->hiPlayer);
		res = HI_SVR_PLAYER_Resume(plays[i]->hiPlayer);
		if (HI_SUCCESS != res)
		{
			MUX_PLAY_ERROR("stop fail, ret = 0x%x ", res);
			return HI_FAILURE;
		}
		HI_UNF_AVPLAY_GetAttr(avPlays[i]->handleAvPlay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &syncAttr);
		syncAttr.enSyncRef = HI_UNF_SYNC_REF_AUDIO;
		res = HI_UNF_AVPLAY_SetAttr(avPlays[i]->handleAvPlay, HI_UNF_AVPLAY_ATTR_ID_SYNC, &syncAttr);
		if (HI_SUCCESS != res )
		{
			MUX_PLAY_ERROR("HI_UNF_AVPLAY_SetAttr failed");
			return HI_FAILURE;
		}

	}
#endif

#if 0
	res =  HI_UNF_VO_SetWindowZorder(newMaster->windowHandle, HI_LAYER_ZORDER_MOVETOP);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("Main window can't set to top!");
		return HI_FAILURE;
	}
	res = muxPlayerSetMainWindowTop(muxRx);
#endif


	res = HI_UNF_AVPLAY_Resume(currentMainPlayer->avPlayHandler, HI_NULL);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_AVPLAY_Resume 1 failed");
		return HI_FAILURE;
	}
	res = HI_UNF_AVPLAY_Resume(newMainPlayer->avPlayHandler, HI_NULL);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_UNF_AVPLAY_Resume 2 failed");
		return HI_FAILURE;
	}

	return res;
}


/*
* webIndex, is come from web, can be 0~ (N-1), if there are N windows (include main window + (n-1)*PIP_Window )
*/
int muxPlayerSwapVideo(MUX_RX_T *muxRx, int webIndex)
{
	int res = 0;
	int i;
	MUX_PLAY_T *oldMaster, *newMaster = NULL, *tmp = NULL;
	RECT_CONFIG *assignedRect;	/* RECT assigned by web interface */

	MUX_PLAY_DEBUG("Current Index of Main Window is %d, new will be %d", muxRx->masterIndex, webIndex );
	if( webIndex <= 0 || webIndex >= cmn_list_size(&muxRx->players) )
	{
		MUX_PLAY_ERROR( "New index No. %d is out of total video of %d", webIndex, cmn_list_size(&muxRx->players) );
		return HI_FAILURE;
	}

#if 0	
	if( webIndex == muxRx->masterIndex)
	{
		MUX_PLAY_WARN("Assign same index for master video");
		return HI_SUCCESS;
	}
#endif

	//oldMaster = (MUX_PLAY_T *)cmn_list_get(&muxRx->players, muxRx->masterIndex);
	oldMaster = muxPlayerFindByType(muxRx, RECT_TYPE_MAIN);

	/* guanratee this is the video playing on that window assigned by web interface */
	assignedRect = (RECT_CONFIG *)cmn_list_get(&muxRx->muxPlayer->playerConfig.windows, webIndex);
	for(i=0; i< cmn_list_size(&muxRx->players); i++)
	{
		tmp = (MUX_PLAY_T *)cmn_list_get(&muxRx->players, i);
		if(tmp->cfg == assignedRect)
		{
			MUX_PLAY_DEBUG("new window index %d is related with %d player", webIndex, i );
			newMaster = tmp;
			break;
		}
	}
	
	if(oldMaster == NULL || newMaster == NULL)
	{
		MUX_PLAY_WARN("Null AvPlay");
		return HI_SUCCESS;
	}

	if(oldMaster == newMaster )
	{
		MUX_PLAY_WARN("Data error: can't swap on the same player!!!");
		return HI_SUCCESS;
	}

	res = _swapPlayers(oldMaster, newMaster);
	muxRx->masterIndex = webIndex;

	res = res;
	return HI_SUCCESS;
}


int muxPlayerWindowRotate(MUX_PLAY_T *play)
{
	int	res = HI_SUCCESS;
	HI_UNF_VO_ROTATION_E mode = HI_UNF_VO_ROTATION_0;
	
	if(play->rotateIndex == ROTATE_TYPE_90)
	{
		mode = HI_UNF_VO_ROTATION_90;
	}
	else if(play->rotateIndex == ROTATE_TYPE_180)
	{
		mode = HI_UNF_VO_ROTATION_180;
	}
	else if(play->cfg->rotateType == ROTATE_TYPE_270)
	{
		mode = HI_UNF_VO_ROTATION_270;
	}

	/* when startup, the playHandler is still not assigned value JL */
//	muxPlayerWindowUpdateHandler(play);
	
	res = HI_UNF_VO_SetRotation(play->windowHandle, mode);
	if( res != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_SetRotation failed : 0x%x", res);
	}

	return res;
}


int muxPlayerWindowLocate(MUX_PLAY_T *play, HI_RECT_S *rect)
{
	int res = HI_SUCCESS;
	HI_UNF_WINDOW_ATTR_S winAttr;

	muxPlayerWindowUpdateHandler(play);

	res = HI_UNF_VO_GetWindowAttr(play->windowHandle, &winAttr);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("get win attr of %s fail: 0x%x!", play->muxFsm.name, res );
		return res;
	}

	memcpy( &winAttr.stOutputRect, rect, sizeof(HI_RECT_S));

	res = HI_UNF_VO_SetWindowAttr(play->windowHandle, &winAttr);
	if (HI_SUCCESS != res)
	{
		if(res == HI_ERR_VO_OPERATION_DENIED )
		{
			MUX_PLAY_ERROR("relocate window of %s to [(%d,%d),(%d,%d)] is denied, because of multi-window overlayed!", play->muxFsm.name, rect->s32X, rect->s32Y, rect->s32Width, rect->s32Height );
		}
		else
		{
			MUX_PLAY_ERROR("relocate window of %s to [(%d,%d),(%d,%d)] fail: 0x%x!", play->muxFsm.name, rect->s32X, rect->s32Y, rect->s32Width, rect->s32Height, res);
		}
	}

	res = OSD_DESKTOP_LOCK(&play->muxRx->higo);
	if(res != 0)
	{
		MUX_PLAY_WARN( "Higo is locked for locate %s: %s", play->muxFsm.name, strerror(errno) );
		return HI_SUCCESS;
	}

	muxOsdPosition(play->osd, rect, play);
	res = OSD_DESKTOP_UNLOCK(&play->muxRx->higo);
	if(res != 0)
	{
		MUX_PLAY_WARN( "Higo is unlocked for locate %s: %s", play->muxFsm.name, strerror(errno) );
		return HI_SUCCESS;
	}
	
	return res;
}


int muxPlayerWindowAspect(MUX_PLAY_T *play, int displayMode)
{
	int res = HI_SUCCESS;
	HI_UNF_WINDOW_ATTR_S winAttr;

	if(displayMode < HI_UNF_VO_ASPECT_CVRS_IGNORE || displayMode>= HI_UNF_VO_ASPECT_CVRS_BUTT)
	{
		return EXIT_FAILURE;
	}

	muxPlayerWindowUpdateHandler(play);

	res = HI_UNF_VO_GetWindowAttr(play->windowHandle, &winAttr);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("get win attr of %s for setting aspect fail: 0x%x!", play->muxFsm.name, res );
		return res;
	}

	MUX_PLAY_DEBUG("Set aspect of window of %s as %d", play->muxFsm.name, displayMode);

	winAttr.stWinAspectAttr.enAspectCvrs = displayMode;

	res = HI_UNF_VO_SetWindowAttr(play->windowHandle, &winAttr);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("Set aspect of window of %s fail: 0x%x!", play->muxFsm.name, res);
	}

	return res;
}


int muxPlayerWindowUpdateHandler(MUX_PLAY_T *play)
{
	int res;
	
	HI_HANDLE hWindow = HI_SVR_PLAYER_INVALID_HDL;
	HI_HANDLE hTrack = HI_SVR_PLAYER_INVALID_HDL;

	res = HI_SVR_PLAYER_GetParam(play->playerHandler, HI_SVR_PLAYER_ATTR_WINDOW_HDL, &hWindow);
	if (HI_SUCCESS != res )
	{
//		MUX_PLAY_ERROR("%s: Get WINDOW_HDL failed, ret = 0x%x: 0x%x!", play->muxFsm.name, res, play->playerHandler);
		return EXIT_FAILURE;
	}
	if( hWindow != play->windowHandle )
	{
		MUX_PLAY_INFO("%s: Old Window Handle is 0x%x, new is 0x%x", play->muxFsm.name, play->windowHandle, hWindow);
		play->windowHandle = hWindow;
	}
	
	
	res = HI_SVR_PLAYER_GetParam(play->playerHandler, HI_SVR_PLAYER_ATTR_AUDTRACK_HDL, &hTrack);
	if (HI_SUCCESS != res )
	{
//		MUX_PLAY_ERROR( "%: Get AUDTRACK_HDL failed, ret = 0x%x!", play->muxFsm.name, res);
		return EXIT_FAILURE;
	}
	
	if( hTrack != play->trackHandle)
	{
		MUX_INFO("%s: Old Track Handle is 0x%x, new is 0x%x", play->muxFsm.name, play->trackHandle, hTrack);
		play->trackHandle = hTrack;
	}

	return EXIT_SUCCESS;
}

