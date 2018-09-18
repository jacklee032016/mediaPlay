
#include "libRx.h"

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
			PRINTF("3D Mode %d not supported now\n", en3D);
	}

	return s32Ret;
}

HI_S32 muxSetVideoMode(MUXLAB_PLAY_T *mux, HI_UNF_DISP_3D_E en3D)
{
	HI_S32 s32Ret, s32Ret1;
	HI_UNF_DISP_3D_E enCur3D;
	HI_UNF_ENC_FMT_E enCurEncFormat, enEncFormat = HI_UNF_ENC_FMT_720P_50;
	HI_UNF_VIDEO_FRAME_PACKING_TYPE_E enFrameType = HI_UNF_FRAME_PACKING_TYPE_NONE;
	HI_UNF_EDID_BASE_INFO_S stSinkAttr;
	HI_BOOL bSetMode = HI_FALSE;
	HI_HANDLE hAVPlay = HI_SVR_PLAYER_INVALID_HDL;

	s32Ret = HI_UNF_DISP_Get3DMode(mux->playerParam.u32Display, &enCur3D, &enCurEncFormat);
	if (s32Ret == HI_SUCCESS && enCur3D == en3D)
	{
		PRINTF("Already in Mode:%d!\n", en3D);
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
			if (_getClosely3DDisplayFormat(mux->playerHandler, en3D, &enEncFormat, &enFrameType) == HI_SUCCESS)
			{
			bSetMode = HI_TRUE;
			}
		}
		else if (s32Ret != HI_SUCCESS)
		{
			PRINTF("Get HDMI sink capability ret error:%#x\n", s32Ret);
		}
		else
		{
			PRINTF("HDMI Sink video mode not supported.\n");
		}
	}

	if (bSetMode == HI_TRUE)
	{
		s32Ret1 = HI_UNF_HDMI_SetAVMute(HI_UNF_HDMI_ID_0, HI_TRUE);
		if (s32Ret1 != HI_SUCCESS)
		{
			PRINTF("Set Hdmi av mute ret error,%#x \n", s32Ret1);
		}

		s32Ret = HI_SVR_PLAYER_GetParam(mux->playerHandler, HI_SVR_PLAYER_ATTR_AVPLAYER_HDL, &hAVPlay);
		if (s32Ret == HI_SUCCESS)
		{/*mvc don't need to set framepack type*/
			s32Ret = HI_UNF_AVPLAY_SetAttr(hAVPlay, HI_UNF_AVPLAY_ATTR_ID_FRMPACK_TYPE, (HI_VOID*)&enFrameType);
			s32Ret |= HI_UNF_DISP_Set3DMode(mux->playerParam.u32Display, en3D, enEncFormat);
		}

		s32Ret1 = HI_UNF_HDMI_SetAVMute(HI_UNF_HDMI_ID_0, HI_FALSE);
		if (s32Ret1 != HI_SUCCESS)
		{
			PRINTF("Set Hdmi av unmute ret error,%#x \n", s32Ret1);
		}
	}
	else
	{
		s32Ret = HI_FAILURE;
	}

	PRINTF("set videomode from(mode:%d,fmt:%d) to (mode:%d,format:%d) ret %#x\n", enCur3D, enCurEncFormat, en3D, enEncFormat, s32Ret);

	return s32Ret;
}

/* reset media file and re-register subtitile callback function */
HI_S32 muxSetMedia(MUXLAB_PLAY_T *mux)
{
	HI_S32 s32Ret;
	HI_SVR_PLAYER_MEDIA_S stMedia;
	HI_HANDLE hSo = HI_SVR_PLAYER_INVALID_HDL;
	HI_HANDLE hAVPlay = HI_SVR_PLAYER_INVALID_HDL;

	memset(&stMedia, 0, sizeof(HI_SVR_PLAYER_MEDIA_S));
	sprintf(stMedia.aszUrl, "%s", mux->s_aszUrl);
	stMedia.u32ExtSubNum = 0;

	PRINTF("### open media file is %s \n", stMedia.aszUrl);
	s32Ret = HI_SVR_PLAYER_SetMedia( mux->playerHandler, HI_SVR_PLAYER_MEDIA_STREAMFILE, &stMedia);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("\e[31m ERR: HI_SVR_PLAYER_SetMedia err, ret = 0x%x! \e[0m\n", s32Ret);
		return s32Ret;
	}
	else
	{
		PRINTF("\e[31m ERR: HI_SVR_PLAYER_SetMedia su, ret = 0x%x! \e[0m\n", s32Ret);
	}

	s32Ret |= HI_SVR_PLAYER_GetParam(mux->playerHandler, HI_SVR_PLAYER_ATTR_SO_HDL, &hSo);
	s32Ret |= HI_SVR_PLAYER_GetParam(mux->playerHandler, HI_SVR_PLAYER_ATTR_AVPLAYER_HDL, &hAVPlay);
	s32Ret |= HI_UNF_SO_RegOnDrawCb(hSo, muxSubtitleOnDrawCallback, muxSubtitleOnClearCallback, &mux->playerHandler);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("\e[31m ERR: set subtitle draw function fail! \e[0m\n");
	}

	return s32Ret;
}

