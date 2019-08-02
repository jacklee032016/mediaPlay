/*
* Call SDK APIs or HiPlayer APIS to set/get attributes of HiPlayer and other UNF devices
*/


#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"

HI_S32 muxSetAttr(MUX_PLAY_T *play, HI_CHAR *pszCmd)
{
	HI_S32 s32Ret = 0;
	HI_HANDLE hdl = 0;
	HI_U32 u32Tmp = 0;
	HI_SVR_PLAYER_SYNC_ATTR_S stSyncAttr;
	HI_UNF_WINDOW_ATTR_S stWinAttr;
	HI_SVR_PLAYER_STREAMID_S stStreamId;
	HI_UNF_DISP_ASPECT_RATIO_S stAspectRatio;
	HI_UNF_SND_GAIN_ATTR_S stGain;

	if (NULL == pszCmd)
		return HI_FAILURE;

	if (0 == strncmp("sync", pszCmd, 4))
	{
		if (3 != sscanf(pszCmd, "sync %d %d %d", 
			&stSyncAttr.s32VidFrameOffset, &stSyncAttr.s32AudFrameOffset, &stSyncAttr.s32SubTitleOffset))
		{
			PLAY_ERROR(play, "input err, example: set sync vidptsadjust audptsadjust, subptsadjust!");
			return HI_FAILURE;
		}

		s32Ret = HI_SVR_PLAYER_SetParam(play->playerHandler, HI_SVR_PLAYER_ATTR_SYNC, (HI_VOID*)&stSyncAttr);
	}
	else if (0 == strncmp("vol", pszCmd, 3))
	{
#if (USE_EXTERNAL_AVPLAY == 1)
		if ((HI_HANDLE)INVALID_TRACK_HDL == play->trackHandle)
		{
			PLAY_ERROR(play, "audio track handle is invalid!");
			return HI_FAILURE;
		}
		hdl = play->trackHandle;
#else
		s32Ret = HI_SVR_PLAYER_GetParam( play->playerHandler, HI_SVR_PLAYER_ATTR_AUDTRACK_HDL, &hdl);
		if (HI_SUCCESS != s32Ret)
		{
			PLAY_ERROR(play, "get audio track hdl fail!");
			return HI_FAILURE;
		}
#endif

		if (1 != sscanf(pszCmd, "vol %d", &u32Tmp))
		{
			PLAY_ERROR(play, "not input volume!");
			return HI_FAILURE;
		}
		stGain.bLinearMode = HI_TRUE;
		stGain.s32Gain = u32Tmp;

		s32Ret = HI_UNF_SND_SetTrackWeight(hdl, &stGain);
		if (HI_SUCCESS != s32Ret)
		{
			PLAY_ERROR(play, "set volume failed!");
			return HI_FAILURE;
		}
	}
	else if (0 == strncmp("track", pszCmd, 5))
	{
		if (1 != sscanf(pszCmd, "track %d", &u32Tmp))
		{
			PLAY_ERROR(play, "not input track mode!");
			return HI_FAILURE;
		}

		s32Ret = HI_UNF_SND_SetTrackMode(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_ALL,u32Tmp);
	}
	else if (0 == strncmp("mute", pszCmd, 4))
	{
		if (1 != sscanf(pszCmd, "mute %d", &u32Tmp))
		{
			PLAY_ERROR(play, "not input mute val!");
			return HI_FAILURE;
		}

		s32Ret = HI_UNF_SND_SetMute(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_ALL,u32Tmp);
	}
	else if (0 == strncmp("pos", pszCmd, 3))
	{
		s32Ret = HI_SVR_PLAYER_GetParam( play->playerHandler, HI_SVR_PLAYER_ATTR_WINDOW_HDL, &hdl);
		s32Ret |= HI_UNF_VO_GetWindowAttr(hdl, &stWinAttr);
		if (HI_SUCCESS != s32Ret)
		{
			PLAY_ERROR(play, "get win attr fail!");
			return HI_FAILURE;
		}

		if (4 != sscanf(pszCmd, "pos %d %d %d %d",
			&stWinAttr.stOutputRect.s32X,
			&stWinAttr.stOutputRect.s32Y,
			&stWinAttr.stOutputRect.s32Width,
			&stWinAttr.stOutputRect.s32Height))
		{
			PLAY_ERROR(play, "input err, example: set pos x y width height!");
			return HI_FAILURE;
		}

		s32Ret = HI_UNF_VO_SetWindowAttr(hdl, &stWinAttr);
	}
	else if (0 == strncmp("aspect", pszCmd, 6))
	{
		s32Ret = HI_SVR_PLAYER_GetParam(play->playerHandler, HI_SVR_PLAYER_ATTR_WINDOW_HDL, &hdl);
		if (HI_SUCCESS != s32Ret)
		{
			PLAY_ERROR(play, "get win attr fail!");
			return HI_FAILURE;
		}

		if (1 != sscanf(pszCmd, "aspect %d", &u32Tmp))
		{
			PLAY_ERROR(play, "not input aspectration val!");
			return HI_FAILURE;
		}
		stAspectRatio.enDispAspectRatio = (HI_UNF_DISP_ASPECT_RATIO_E)u32Tmp;
		if (stAspectRatio.enDispAspectRatio== HI_UNF_DISP_ASPECT_RATIO_USER)
		{
			PLAY_ERROR(play, "set aspect ratio fail, use user-aspect instead.");
			return HI_FAILURE;
		}
		s32Ret = HI_UNF_DISP_SetAspectRatio(hdl, &stAspectRatio);
		if (s32Ret != HI_SUCCESS)
		{
			PLAY_ERROR(play, "set aspect ratio fail.");
			return HI_FAILURE;
		}
	}
	else if (0 == strncmp("user-aspect", pszCmd, 11))
	{
		s32Ret = HI_SVR_PLAYER_GetParam(play->playerHandler, HI_SVR_PLAYER_ATTR_WINDOW_HDL, &hdl);
		if (HI_SUCCESS != s32Ret)
		{
			PLAY_ERROR(play, "get win attr fail!");
			return HI_FAILURE;
		}

		if (3 != sscanf(pszCmd, "user-aspect %u %u %u", &u32Tmp,
			&stAspectRatio.u32UserAspectWidth, &stAspectRatio.u32UserAspectHeight))
		{
			PLAY_ERROR(play, "not input user-aspectration vals!");
			return HI_FAILURE;
		}
		
		stAspectRatio.enDispAspectRatio = (HI_UNF_DISP_ASPECT_RATIO_E)u32Tmp;
		if (stAspectRatio.enDispAspectRatio != HI_UNF_DISP_ASPECT_RATIO_USER)
		{
			PLAY_ERROR(play, "set user-aspect ratio fail, use aspect instead.");
			return HI_FAILURE;
		}
		
		s32Ret = HI_UNF_DISP_SetAspectRatio(hdl, &stAspectRatio);
		if (s32Ret != HI_SUCCESS)
		{
			PLAY_ERROR(play, "set user-aspect ratio fail.");
			return HI_FAILURE;
		}
	}
	else if (0 == strncmp("zorder", pszCmd, 6))
	{
		s32Ret = HI_SVR_PLAYER_GetParam(play->playerHandler, HI_SVR_PLAYER_ATTR_WINDOW_HDL, &hdl);

		if (HI_SUCCESS != s32Ret)
		{
			PLAY_ERROR(play, "get win hdl fail!");
			return HI_FAILURE;
		}

		if (1 != sscanf(pszCmd, "zorder %d", &u32Tmp))
		{
			PLAY_ERROR(play, "not input zorder val!");
			return HI_FAILURE;
		}

		s32Ret = HI_UNF_VO_SetWindowZorder(hdl, u32Tmp);
	}
	else if (0 == strncmp("id", pszCmd, 2))
	{
		HI_FORMAT_FILE_INFO_S *pstFileInfo = NULL;

		s32Ret = HI_SVR_PLAYER_GetFileInfo(play->playerHandler, &pstFileInfo);
		if (HI_SUCCESS != s32Ret)
		{
			PLAY_ERROR(play, "get file info fail!");
			return HI_FAILURE;
		}

		if (4 != sscanf(pszCmd, "id %hu %hu %hu %hu",
			&stStreamId.u16ProgramId,
			&stStreamId.u16VidStreamId,
			&stStreamId.u16AudStreamId,
			&stStreamId.u16SubStreamId))
		{
			PLAY_ERROR(play, "input err, example: set id 0 0 1 0, set id prgid videoid audioid subtitleid! ");
			return HI_FAILURE;
		}

		if (stStreamId.u16ProgramId >= pstFileInfo->u32ProgramNum ||
			(stStreamId.u16VidStreamId != 0 &&
			stStreamId.u16VidStreamId >= pstFileInfo->pastProgramInfo[stStreamId.u16ProgramId].u32VidStreamNum) ||
			(stStreamId.u16AudStreamId != 0 &&
			stStreamId.u16AudStreamId >= pstFileInfo->pastProgramInfo[stStreamId.u16ProgramId].u32AudStreamNum) ||
			(stStreamId.u16SubStreamId != 0 &&
			stStreamId.u16SubStreamId >= pstFileInfo->pastProgramInfo[stStreamId.u16ProgramId].u32SubStreamNum))
		{
			PLAY_ERROR(play, "invalid stream id");
			return HI_FAILURE;
		}

		s32Ret = HI_SVR_PLAYER_SetParam(play->playerHandler, HI_SVR_PLAYER_ATTR_STREAMID, (HI_VOID*)&stStreamId);

		if (HI_SUCCESS == s32Ret &&
			pstFileInfo->u32ProgramNum > 0 &&
			pstFileInfo->pastProgramInfo[stStreamId.u16ProgramId].u32SubStreamNum > 0)
		{
			if (pstFileInfo->pastProgramInfo[stStreamId.u16ProgramId].pastSubStream[stStreamId.u16SubStreamId].u32Format ==
				HI_FORMAT_SUBTITLE_HDMV_PGS)
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
	else if (0 == strncmp("vmode", pszCmd, 5))
	{
		if (1 != sscanf(pszCmd, "vmode %u", &u32Tmp))
		{
			PLAY_ERROR(play, "no input val for set video mode!(%d-2d,%d-framepacking,%d-sbs,%d-tab...) ",
				HI_UNF_DISP_3D_NONE, HI_UNF_DISP_3D_FRAME_PACKING, HI_UNF_DISP_3D_SIDE_BY_SIDE_HALF,
				HI_UNF_DISP_3D_TOP_AND_BOTTOM);
			PLAY_DEBUG(play, "for example:set vmode 1");
			return HI_FAILURE;
		}
		else if (u32Tmp >= HI_UNF_DISP_3D_BUTT)
		{
			PLAY_ERROR(play, "invalid video mode (%u),should < %d", u32Tmp,  HI_UNF_DISP_3D_BUTT);
		}

		muxSetVideoMode(play, u32Tmp);
	}
	else if (0 == strncmp("hdmi", pszCmd, 4))
	{
		static int hdmi_toggle =0;
		hdmi_toggle++;
		if(hdmi_toggle&1)
		{
			HI_UNF_SND_SetHdmiMode(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_HDMI0, HI_UNF_SND_HDMI_MODE_RAW);
			printf("hmdi pass-through on!\n");
		}
		else
		{
			HI_UNF_SND_SetHdmiMode(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_HDMI0, HI_UNF_SND_HDMI_MODE_LPCM);
			printf("hmdi pass-through off!\n");
		}

	}
	else if (0 == strncmp("spdif", pszCmd, 5))
	{
		static int spdif_toggle = 0;
		spdif_toggle++;
		if (spdif_toggle & 1)
		{
			HI_UNF_SND_SetSpdifMode(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_SPDIF0, HI_UNF_SND_SPDIF_MODE_RAW);
			printf("spdif pass-through on!\n");
		}
		else
		{
			HI_UNF_SND_SetSpdifMode(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_SPDIF0, HI_UNF_SND_SPDIF_MODE_LPCM);
			printf("spdif pass-through off!\n");
		}
	}
	else
	{
		PLAY_ERROR(play, "not support commond! \n");
	}

	return HI_SUCCESS;
}

HI_S32 muxGetRunInfo(MUX_PLAY_T *play)
{
#if 0
	HI_S32 s32Ret = 0;
	HI_S64 s64Tmp = 0;
	HI_CHAR cTmp[512];
	HI_FORMAT_BUFFER_CONFIG_S stBufConfig;
	HI_FORMAT_BUFFER_STATUS_S stBufStatus;
	int streamIndex = play->runInfo.hlsStreamIndex;

	s32Ret = HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_GET_BANDWIDTH, (HI_VOID*)&s64Tmp);
	if (HI_SUCCESS != s32Ret)
	{
		PLAY_ERROR(play, "get bandwidth fail! \n");
		return HI_FAILURE;
	}

	runInfo->bandwidthKbps = (int)(s64Tmp/1024);
	PLAY_DEBUG(play, "get bandwidth is %lld(%s) K bps", s64Tmp/1024, runInfo->bandwidthKbps);
	
	memset(&stBufConfig, 0, sizeof(HI_FORMAT_BUFFER_CONFIG_S));
	stBufConfig.eType = HI_FORMAT_BUFFER_CONFIG_TIME;

	s32Ret = HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_GET_BUFFER_CONFIG, (HI_VOID*)&stBufConfig);
	if (HI_SUCCESS != s32Ret)
	{
		PLAY_ERROR(play, "get bufferconfig fail!");
		return HI_FAILURE;
	}
	else
	{
		runInfo->bufMode = stBufConfig.eType;
		runInfo->bufStart = (int)stBufConfig.s64EventStart;
		runInfo->bufEnough = (int)stBufConfig.s64EventEnough;
		runInfo->bufTotal = (int)stBufConfig.s64Total;
		runInfo->bufTimeOut = (int)stBufConfig.s64TimeOut;
		
		PLAY_INFO(play, "BufferConfig:type(%d):%s",stBufConfig.eType, cTmp);
		PLAY_INFO(play, "s64EventStart:%lld(%d)", stBufConfig.s64EventStart, runInfo->bufStart);
		PLAY_INFO(play, "s64EventEnough:%lld(%d)", stBufConfig.s64EventEnough, runInfo->bufEnough);
		PLAY_INFO(play, "s64Total:%lld(%d)", stBufConfig.s64Total, runInfo->bufTotal);
		PLAY_INFO(play, "s64TimeOut:%lld(%d)", stBufConfig.s64TimeOut, runInfo->bufTimeOut);
	}	

	s32Ret = HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_GET_BUFFER_STATUS, (HI_VOID*)&stBufStatus);
	if (HI_SUCCESS != s32Ret)
	{
		PLAY_INFO(play, "get Buffer status fail!");
		return HI_FAILURE;
	}
	else
	{
		runInfo->bufStatusDuration = (int)stBufStatus.s64Duration;
		runInfo->bufStatusSize = (int)stBufStatus.s64BufferSize;
		PLAY_INFO(play, "BufferStatus:");
		PLAY_INFO(play, "s64Duration:%lld(%d) ms", stBufStatus.s64Duration, runInfo->bufStatusDuration);
		PLAY_INFO(play, "s64BufferSize:%lld(%d) bytes", stBufStatus.s64BufferSize, runInfo->bufStatusSize);
	}
	

	HI_FORMAT_HLS_STREAM_INFO_S stHlsInfo;
	stHlsInfo.stream_nb = streamIndex;
	s32Ret = HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_GET_HLS_STREAM_INFO, (HI_VOID*)&stHlsInfo);
	if (HI_SUCCESS != s32Ret)
	{
		PLAY_ERROR(play, "get HLS streaminfo fail!");
		return HI_FAILURE;
	}
	else
	{
		runInfo->hlsStreamIndex =  stHlsInfo.stream_nb;
		runInfo->hlsSegmentCount = stHlsInfo.hls_segment_nb;
		snprintf(runInfo->hlsUrl, sizeof(runInfo->hlsUrl), "%s",  stHlsInfo.url );
		PLAY_INFO(play,  "hls streamindex:%d", stHlsInfo.stream_nb);
		PLAY_INFO(play,  "hls segments number in current stream:%d", stHlsInfo.hls_segment_nb);
		PLAY_INFO(play,  "hls stream url:%s", stHlsInfo.url);
	}

	HI_FORMAT_HLS_SEGMENT_INFO_S stHlsSegInfo;
	s32Ret = HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_GET_HLS_SEGMENT_INFO, (HI_VOID*)&stHlsSegInfo);
	if (HI_SUCCESS != s32Ret)
	{
		PLAY_ERROR(play, "get HLS segment info fail! ");
		return HI_FAILURE;
	}
	else
	{
		runInfo->hlsSegStreamIndex = stHlsSegInfo.stream_num;
		runInfo->hlsSegTotalTime = stHlsSegInfo.total_time;
		runInfo->hlsSegSeqNum = stHlsSegInfo.seq_num;
		runInfo->hlsSegBandwidthKbps = stHlsSegInfo.bandwidth;
		snprintf(runInfo->hlsSegUrl, sizeof(runInfo->hlsSegUrl), "%s", stHlsSegInfo.url);
		
		PLAY_INFO(play, "hls stream of current segment:%d", stHlsSegInfo.stream_num);
		PLAY_INFO(play, "hls current segment duration :%d", stHlsSegInfo.total_time);
		PLAY_INFO(play, "hls index of current segment :%d", stHlsSegInfo.seq_num);
		PLAY_INFO(play, "hls bandwidth required       :%lld", stHlsSegInfo.bandwidth);
		PLAY_INFO(play, "hls current segment url      :%s", stHlsSegInfo.url);
	}
#endif

	return HI_SUCCESS;
}


