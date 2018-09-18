
#include "libRx.h"

HI_VOID _setHlsInfo(HI_HANDLE hPlayer, HI_CHAR *pszCmd)
{
	if (NULL == pszCmd)
		return;
	
	if (0 == strncmp("track", pszCmd, 5))
	{
		HI_S32 s32HlsStreamNum = 0;
		(HI_VOID)HI_SVR_PLAYER_Invoke(hPlayer, HI_FORMAT_INVOKE_GET_HLS_STREAM_NUM, &s32HlsStreamNum);

		HI_S32 s32Track = -1;
		if (1 != sscanf(pszCmd, "track %d", &s32Track))
		{
			PRINTF("ERR: input err, example: sethls track 0 \n");
			return;
		}
		
		if (s32Track >= s32HlsStreamNum || s32Track < -1)
		{
			PRINTF("ERR: track id must from -1 and to streamnum %d !\n", s32HlsStreamNum);
			return;
		}
		HI_SVR_PLAYER_Invoke(hPlayer, HI_FORMAT_INVOKE_SET_HLS_STREAM, (HI_VOID*)&s32Track);
	}
	else
	{
		PRINTF("ERR: not support commond1! \n");
	}
}

HI_S32 muxPlayCmdsHandle(MUXLAB_PLAY_T *mux)
{
	HI_CHAR aszInputCmd[CMD_LEN];
	HI_S32 ret;

	SAMPLE_GET_INPUTCMD(aszInputCmd);

	PRINTF(">>> [input cmd] %s \n", aszInputCmd);

	if (0 == strncmp("help", aszInputCmd, 4))
	{
		PRINTF(HELP_INFO);
	}
	else if (0 == strncmp("play", aszInputCmd, 4))
	{
		/*rtsp/mms/rtmp need to redo setting media when stop->play/replay*/
		if (strstr(mux->s_aszUrl, "rtsp") ||
			strstr(mux->s_aszUrl, "mms")  ||
			strstr(mux->s_aszUrl, "rtmp"))
		{
			ret = muxSetMedia(mux);
			if (HI_SUCCESS != ret)
			{
				return MUX_CONTINUE;
			}
		}

		ret = HI_SVR_PLAYER_Play(mux->playerHandler);
		if (HI_SUCCESS != ret)
			PRINTF("\e[31m ERR: play fail, ret = 0x%x \e[0m \n", ret);
	}
	else if (0 == strncmp("pause", aszInputCmd, 5))
	{
		ret = HI_SVR_PLAYER_Pause( mux->playerHandler);
		if (HI_SUCCESS != ret)
			PRINTF("\e[31m ERR: pause fail, ret = 0x%x \e[0m \n", ret);
	}
	else if (0 == strncmp("seek", aszInputCmd, 4))
	{
		HI_S64 s64SeekTime = 0;

		if (1 != sscanf(aszInputCmd, "seek %lld", &s64SeekTime))
		{
			PRINTF("\e[31m ERR: not input seek time, example: seek 8000! \e[0m \n");
			return MUX_CONTINUE;
		}

		ret = HI_SVR_PLAYER_Seek(mux->playerHandler, s64SeekTime);
		if (HI_SUCCESS != ret)
			PRINTF("\e[31m ERR: seek fail, ret = 0x%x \e[0m \n", ret);
		else
			muxSubtitleClear(mux);
	}
	else if(0 == strncmp("posseek",aszInputCmd,7))
	{
		HI_S64 s64SeekPos = 0;
		if (1 != sscanf(aszInputCmd, "posseek %lld", &s64SeekPos))
		{
			PRINTF("\e[31m ERR: not input seek position, example: pos 8000! \e[0m \n");
			return MUX_CONTINUE;
		}

		ret = HI_SVR_PLAYER_SeekPos(mux->playerHandler, s64SeekPos);
		if (HI_SUCCESS != ret)
			PRINTF("\e[31m ERR: seek pos fail, ret = 0x%x \e[0m \n", ret);
	}
	else if (0 == strncmp("metadata", aszInputCmd, 8))
	{
		HI_SVR_PLAYER_METADATA_S *pstMetadata = NULL;

		pstMetadata = HI_SVR_META_Create();

		ret = HI_SVR_PLAYER_Invoke(mux->playerHandler, HI_FORMAT_INVOKE_GET_METADATA, pstMetadata);
		if (HI_FAILURE == ret)
		{
			PRINTF("Get Metadata fail!\n");
			return MUX_CONTINUE;
		}

		HI_SVR_META_PRINT(pstMetadata);
		HI_SVR_META_Free(&pstMetadata);
	}
	else if (0 == strncmp("stop", aszInputCmd, 4))
	{
		ret = HI_SVR_PLAYER_Stop(mux->playerHandler);
		if (HI_SUCCESS != ret)
			PRINTF("\e[31m ERR: stop fail, ret = 0x%x \e[0m \n", ret);
		else
			muxSubtitleClear(mux);
	}
	else if (0 == strncmp("resume", aszInputCmd, 6))
	{
		ret = HI_SVR_PLAYER_Resume(mux->playerHandler);

		if (HI_SUCCESS != ret)
			PRINTF("\e[31m ERR: resume fail, ret = 0x%x \e[0m \n", ret);
	}
	else if (0 == strncmp("bw", aszInputCmd, 2))
	{
		HI_S32 s32Speed = 0;

		if (1 != sscanf(aszInputCmd, "bw %d", &s32Speed))
		{
			PRINTF("\e[31m ERR: not input tplay speed, example: bw 2! \e[0m \n");
			return MUX_CONTINUE;
		}

		s32Speed *= HI_SVR_PLAYER_PLAY_SPEED_NORMAL;

		if (s32Speed > HI_SVR_PLAYER_PLAY_SPEED_32X_FAST_FORWARD)
		{
			PRINTF("\e[31m ERR: not support tplay speed! \e[0m \n");
			return MUX_CONTINUE;
		}

		s32Speed = 0 - s32Speed;

		PRINTF("backward speed = %d \n", s32Speed);

		ret = HI_SVR_PLAYER_TPlay(mux->playerHandler, s32Speed);

		if (HI_SUCCESS == ret)
			muxSubtitleClear(mux);
		else
			PRINTF("\e[31m ERR: tplay fail, ret = 0x%x \e[0m \n", ret);
	}
	else if (0 == strncmp("ff", aszInputCmd, 2))
	{
		HI_S32 s32Speed = 0;

		if (1 != sscanf(aszInputCmd, "ff %d", &s32Speed))
		{
			PRINTF("\e[31m ERR: not input tplay speed, example: ff 4! \e[0m \n");
			return MUX_CONTINUE;
		}

		s32Speed *= HI_SVR_PLAYER_PLAY_SPEED_NORMAL;

		if (s32Speed > HI_SVR_PLAYER_PLAY_SPEED_32X_FAST_FORWARD)
		{
			PRINTF("\e[31m ERR: not support tplay speed! \e[0m  \n");
			return MUX_CONTINUE;
		}

		PRINTF("forward speed = %d \n", s32Speed);

		ret = HI_SVR_PLAYER_TPlay(mux->playerHandler, s32Speed);

		if (HI_SUCCESS == ret)
			muxSubtitleClear(mux);
		else
			PRINTF("\e[31m ERR: tplay fail, ret = 0x%x \e[0m \n", ret);
	}
	else if (0 == strncmp("info", aszInputCmd, 4))
	{
		HI_SVR_PLAYER_INFO_S stPlayerInfo;
		/* Output player information */
		ret = HI_SVR_PLAYER_GetPlayerInfo(mux->playerHandler, &stPlayerInfo);
		if (HI_SUCCESS != ret)
		{
			PRINTF("\e[31m ERR: HI_SVR_PLAYER_GetPlayerInfo fail! \e[0m\n");
			return MUX_CONTINUE;
		}

		PRINTF("PLAYER INFO: \n");
		PRINTF("  Cur progress:   %d \n", stPlayerInfo.u32Progress);
		PRINTF("  Time played:  %lld:%lld:%lld, Total time: %lld:%lld:%lld \n",
			stPlayerInfo.u64TimePlayed / (1000 * 3600),
			(stPlayerInfo.u64TimePlayed % (1000 * 3600)) / (1000 * 60),
			((stPlayerInfo.u64TimePlayed % (1000 * 3600)) % (1000 * 60)) / 1000,
			mux->fileInfo->s64Duration / (1000 * 3600),
			(mux->fileInfo->s64Duration % (1000 * 3600)) / (1000 * 60),
			(( mux->fileInfo->s64Duration % (1000 * 3600)) % (1000 * 60)) / 1000);
		PRINTF("  Speed:          %d \n", stPlayerInfo.s32Speed);
		PRINTF("  Status:         %d \n", stPlayerInfo.eStatus);

		ret = HI_SVR_PLAYER_GetParam(mux->playerHandler, HI_SVR_PLAYER_ATTR_STREAMID, (HI_VOID*)& mux->playerStreamIDs);
		PRINTF("  StreamId: program id(%d), video id(%d), audio id(%d), subtitle id(%d) \n",
			mux->playerStreamIDs.u16ProgramId, mux->playerStreamIDs.u16VidStreamId, 
			mux->playerStreamIDs.u16AudStreamId, mux->playerStreamIDs.u16SubStreamId);
	}
	else if (0 == strncmp("set ", aszInputCmd, 4))
	{
		muxSetAttr(mux, aszInputCmd + 4);
	}
	else if (0 == strncmp("get ", aszInputCmd, 4))
	{
		muxGetAttr(mux, aszInputCmd + 4);
	}
	else if (0 == strncmp("dbg", aszInputCmd, 3))
	{
		HI_S32 s32Dbg = 0;

		if (1 != sscanf(aszInputCmd, "dbg %d", &s32Dbg))
		{
			PRINTF("\e[31m ERR: not dbg enable balue, example: dbg 1! \e[0m\n");
			return MUX_CONTINUE;
		}

		HI_SVR_PLAYER_EnableDbg(s32Dbg);
	}
	else if (0 == strncmp("open", aszInputCmd, 4))
	{
		HI_S32 s32Len;
		HI_CHAR aszUrl[HI_FORMAT_MAX_URL_LEN];
		HI_SVR_PLAYER_STREAMID_S stStreamId;

		/* Reopen a file */
		ret = HI_SUCCESS;
		(HI_VOID)HI_GO_FillRect(mux->higo.layerSurfaceHandle, NULL, 0x00000000, HIGO_COMPOPT_NONE);
		(HI_VOID)HI_GO_RefreshLayer(mux->higo.layerHandler, NULL);

		s32Len = strlen(aszInputCmd + 5);
		if (s32Len >= sizeof(aszUrl))
		{
			return MUX_CONTINUE;
		}

#if 0
		memcpy(aszUrl, aszInputCmd + 5, s32Len - 1);
		aszUrl[s32Len - 1] = 0;
		ret = muxSetMedia(mux, aszUrl);
#else
		memcpy(mux->s_aszUrl, aszInputCmd + 5, s32Len - 1);
		mux->s_aszUrl[s32Len - 1] = 0;
		ret = muxSetMedia(mux );
#endif
		if (HI_SUCCESS != ret)
		{
			return MUX_CONTINUE;
		}

		ret = HI_SVR_PLAYER_GetFileInfo(mux->playerHandler, &mux->fileInfo);
		if (HI_SUCCESS == ret)
		{
			muxOutputFileInfo(mux->fileInfo);
			if (mux->fileInfo->u32ProgramNum > 0 &&
				HI_SVR_PLAYER_GetParam(mux->playerHandler, HI_SVR_PLAYER_ATTR_STREAMID, (HI_VOID*)&stStreamId) == HI_SUCCESS &&
				mux->fileInfo->pastProgramInfo[stStreamId.u16ProgramId].u32VidStreamNum > 0 &&
				mux->fileInfo->pastProgramInfo[stStreamId.u16ProgramId].pastVidStream[stStreamId.u16VidStreamId].u32Format
				== HI_FORMAT_VIDEO_MVC)
			{
				muxSetVideoMode(mux, HI_UNF_DISP_3D_FRAME_PACKING);
			}
		}
		else
		{
			PRINTF("\e[31m ERR: get file info fail! \e[0m\n");
		}
	}
	else if (0 == strncmp("close", aszInputCmd, 4))
	{
		ret = HI_SVR_PLAYER_Invoke(mux->playerHandler, HI_FORMAT_INVOKE_PRE_CLOSE_FILE, NULL);
		if (HI_SUCCESS != ret)
		{
			PRINTF("\e[31m ERR: HI_FORMAT_INVOKE_PRE_CLOSE_FILE fail! \e[0m\n");
		}
	}
	else if (0 == strncmp("sub", aszInputCmd, 3))
	{
		/* Set an external subtitle file in playing state */

		ret = HI_SUCCESS;
		memset(&mux->playerMedia, 0, sizeof(mux->playerMedia));
		memcpy(mux->playerMedia.aszExtSubUrl[0], aszInputCmd + 4, strlen(aszInputCmd + 4) - 1);
		mux->playerMedia.u32ExtSubNum = 1;

		PRINTF("### open external sub file is %s \n", mux->playerMedia.aszExtSubUrl[0]);

		ret = HI_SVR_PLAYER_SetMedia(mux->playerHandler, HI_SVR_PLAYER_MEDIA_SUBTITLE, &mux->playerMedia);
		if (HI_SUCCESS != ret)
		{
			PRINTF("\e[31m ERR: HI_SVR_PLAYER_SetMedia err, ret = 0x%x! \e[0m\n", ret);
			return MUX_CONTINUE;
		}

		ret = HI_SVR_PLAYER_GetFileInfo(mux->playerHandler, &mux->fileInfo);
		if (HI_SUCCESS == ret)
		{
			muxOutputFileInfo(mux->fileInfo);
		}
		else
		{
			PRINTF("\e[31m ERR: get file info fail! \e[0m\n");
		}
	}
	else if (0 == strncmp("q", aszInputCmd, 1))
	{
		if ( mux->s_s32ThreadEnd == HI_FAILURE)
		{
			mux->s_s32ThreadExit = HI_SUCCESS;
			HI_SVR_PLAYER_Invoke(mux->playerHandler, HI_FORMAT_INVOKE_PRE_CLOSE_FILE, NULL);
			pthread_join( mux->thread, NULL);
		}
		
		return MUX_QUIT;
	}
	else if (0 == strncmp("sethls ", aszInputCmd, 7))
	{
		_setHlsInfo(mux->playerHandler, aszInputCmd + 7);
	}
#ifdef DRM_SUPPORT
	else if (0 == strncmp("drm", aszInputCmd, 3))
	{
		drm_cmd(aszInputCmd + 4);
	}
#endif
	else if (0 == strncmp("dolby", aszInputCmd, 5))
	{
		ret = HI_UNF_DISP_SetHDRType(HI_UNF_DISPLAY1, HI_UNF_DISP_HDR_TYPE_DOLBY);
		if (HI_SUCCESS == ret)
		{
			PRINTF("DolbyHDR display mode\n");
		}
	}
	else if (0 == strncmp("hdr10", aszInputCmd, 5))
	{
		ret = HI_UNF_DISP_SetHDRType(HI_UNF_DISPLAY1, HI_UNF_DISP_HDR_TYPE_HDR10);
		if (HI_SUCCESS == ret)
		{
			PRINTF("HDR10 display mode\n");
		}
	}
	else if (0 == strncmp("sdr", aszInputCmd, 3))
	{
		ret = HI_UNF_DISP_SetHDRType(HI_UNF_DISPLAY1, HI_UNF_DISP_HDR_TYPE_NONE);
		if (HI_SUCCESS == ret)
		{
			PRINTF("SDR display mode\n");
		}            
	}
	else if (0 == strncmp("record", aszInputCmd, 6))
	{
		ret = muxRecorderStart(mux);
		if (HI_SUCCESS == ret)
		{
			PRINTF("Recorder start OK\n");
		}            
	}
	else if (0 == strncmp("recstop", aszInputCmd, 7))
	{
		ret = muxRecorderStop(mux);
		if (HI_SUCCESS == ret)
		{
			PRINTF("Recorder stop OK\n");
		}            
	}
	else
	{
		PRINTF("\e[31m ERR: Record not stop operation! \e[0m\n");
	}
	
	return MUX_CONTINUE;
}

