
#include "libRx.h"

static HI_S32 _higoInit( MUXLAB_HIGO_T *higo)
{
	HI_S32 s32Ret = 0;
	HIGO_LAYER_INFO_S stLayerInfo = {0};

	s32Ret = HI_GO_Init();
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("ERR: failed to init higo! ret = 0x%x !\n", s32Ret);
		return HI_FAILURE;
	}

	(HI_VOID)HI_GO_GetLayerDefaultParam(HIGO_LAYER_HD_0, &stLayerInfo);

	stLayerInfo.LayerFlushType = HIGO_LAYER_FLUSH_DOUBBUFER;
	stLayerInfo.PixelFormat = HIGO_PF_8888;
	s32Ret = HI_GO_CreateLayer(&stLayerInfo, &higo->layerHandler);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("ERR: failed to create the layer sd 0, ret = 0x%x !\n", s32Ret);
		goto RET;
	}

	s32Ret =  HI_GO_GetLayerSurface(higo->layerHandler, &higo->layerSurfaceHandle);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("ERR: failed to get layer surface! s32Ret = 0x%x \n", s32Ret);
		goto RET;
	}

	(HI_VOID)HI_GO_FillRect(higo->layerSurfaceHandle, NULL, 0x00000000, HIGO_COMPOPT_NONE);
	s32Ret = HI_GO_CreateText(SB_FONT_FILE, NULL,  &higo->fontHandle);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("ERR: failed to create the font:higo_gb2312.ubf !\n");
		goto RET;
	}

	(HI_VOID)HI_GO_SetTextColor(higo->fontHandle, 0xffffffff);

	return HI_SUCCESS;

RET:
	return HI_FAILURE;
}

/* only dependent on HI_UNF_ENC_FMT_E fmt */
static HI_S32 _deviceInit(MUXLAB_PLAY_T *mux)
{
	HI_S32                   s32Ret = 0;
	//HI_UNF_EDID_BASE_INFO_S  stSinkAttr;

	HI_SYS_Init();

	HIADP_MCE_Exit();

	s32Ret = HIADP_Snd_Init();
	if (s32Ret != HI_SUCCESS)
	{
		PRINTF("call SndInit failed.\n");
		return HI_FAILURE;
	}

	s32Ret = HIADP_Disp_Init(mux->videoFormat );
	if (s32Ret != HI_SUCCESS)
	{
		PRINTF("call DispInit failed.\n");
		return HI_FAILURE;
	}

	s32Ret = HIADP_VO_Init(HI_UNF_VO_DEV_MODE_NORMAL);
	if (s32Ret != HI_SUCCESS)
	{
		PRINTF("call VoInit failed.\n");
		return HI_FAILURE;
	}

#if 0
	s32Ret = HI_UNF_HDMI_GetSinkCapability(HI_UNF_HDMI_ID_0, &stSinkAttr);
	if (s32Ret == HI_SUCCESS)
	{
		if (HI_TRUE == stSinkAttr.bDolbySupport)
		{
			s32Ret = HI_UNF_DISP_SetHDRType(HI_UNF_DISPLAY1, HI_UNF_DISP_HDR_TYPE_DOLBY);
			if (HI_SUCCESS != s32Ret)            
			{                
				printf("call HI_UNF_DISP_SetHDRType Dolby failed %#x.\n", s32Ret);
			}
		}
		else if (HI_TRUE == stSinkAttr.bHdrSupport)
		{
			s32Ret = HI_UNF_DISP_SetHDRType(HI_UNF_DISPLAY1, HI_UNF_DISP_HDR_TYPE_HDR10);
			if (HI_SUCCESS != s32Ret)
			{
				printf("call HI_UNF_DISP_SetHDRType HDR10 failed %#x.\n", s32Ret);
			}
		}
		else
		{
			s32Ret = HI_UNF_DISP_SetHDRType(HI_UNF_DISPLAY1, HI_UNF_DISP_HDR_TYPE_NONE);
			if (HI_SUCCESS != s32Ret)
			{
				printf("call HI_UNF_DISP_SetHDRType SDR failed %#x.\n", s32Ret);
			}        
		}
	}
#endif

	s32Ret = HI_UNF_AVPLAY_RegisterAcodecLib("libHA.AUDIO.BLURAYLPCM.decode.so");
	s32Ret |= HI_UNF_AVPLAY_RegisterAcodecLib("libHA.AUDIO.FFMPEG_ADEC.decode.so");
	s32Ret |= HIADP_AVPlay_RegADecLib();
	if (s32Ret != HI_SUCCESS)
	{
		PRINTF("call HI_UNF_AVPLAY_RegisterAcodecLib failed.\n");
	}

	s32Ret = HI_UNF_AVPLAY_RegisterVcodecLib("libHV.VIDEO.FFMPEG_VDEC.decode.so");
	if (s32Ret != HI_SUCCESS)
	{
		PRINTF("call HI_UNF_AVPLAY_RegisterVcodecLib return 0x%x.\n", s32Ret);
	}

	s32Ret = HI_UNF_DMX_Init();
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("### HI_UNF_DMX_Init init fail.\n");
		return HI_FAILURE;
	}

	s32Ret = HI_UNF_AVPLAY_Init();
	if (s32Ret != HI_SUCCESS)
	{
		PRINTF("call HI_UNF_AVPLAY_Init failed.\n");
		return HI_FAILURE;
	}

	s32Ret = HI_UNF_SO_Init();
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("### HI_UNF_SO_Init init fail.\n");
		return HI_FAILURE;
	}

	usleep(3000 * 1000);

	return HI_SUCCESS;
}


HI_S32 _hiPlayInit(MUXLAB_PLAY_T *mux)
{
	HI_S32 ret;
	HI_S32 i = 0;

	ret = HI_SVR_PLAYER_Init();
	if (HI_SUCCESS != ret)
	{
		PRINTF("\e[31m ERR: player init fail, ret = 0x%x \e[0m \n", ret);
		return HI_FAILURE;
	}

	HI_FORMAT_LIB_VERSION_S stPlayerVersion;
	memset(&stPlayerVersion, 0, sizeof(stPlayerVersion));
	HI_SVR_PLAYER_GetVersion(&stPlayerVersion);
	printf("HiPlayer V%u.%u.%u.%u\n", stPlayerVersion.u8VersionMajor, stPlayerVersion.u8VersionMinor, stPlayerVersion.u8Revision, stPlayerVersion.u8Step);

	/* Regist file demuxers */
	ret = HI_SVR_PLAYER_RegisterDynamic(HI_SVR_PLAYER_DLL_PARSER, "libformat.so");
	if (HI_SUCCESS != ret)
	{
		PRINTF("\e[31m ERR: register file parser libformat.so fail, ret=0x%x \e[0m \n", ret);
	}

	(HI_VOID)HI_SVR_PLAYER_RegisterDynamic(HI_SVR_PLAYER_DLL_PARSER, "libffmpegformat.so");
	if (HI_SUCCESS != ret)
	{
		PRINTF("\e[31m ERR: register file parser libffmpegformat.so fail, ret=0x%x \e[0m \n", ret);
	}

	mux->playerParam.hAVPlayer = mux->avPlayHandler;
	mux->playerParam.hDRMClient = HI_SVR_PLAYER_INVALID_HDL;
	
#ifdef DRM_SUPPORT
	mux->playerParam.hDRMClient = (HI_HANDLE)drm_get_handle();
	PRINTF("drm handle created:%#x\n", mux->playerParam.hDRMClient);
#endif

	ret = HI_SVR_PLAYER_Create(& mux->playerParam, &mux->playerHandler);
	if (HI_SUCCESS != ret)
	{
		PRINTF("\e[31m ERR: player open fail, ret = 0x%x \e[0m \n", ret);
		return HI_FAILURE;
	}

#if defined (CHARSET_SUPPORT)
	HI_S32 s32CodeType = HI_CHARSET_CODETYPE_UTF8;
	extern HI_CHARSET_FUN_S g_stCharsetMgr_s;

	ret = HI_SVR_PLAYER_Invoke( mux->playerHandler, HI_FORMAT_INVOKE_SET_CHARSETMGR_FUNC, &g_stCharsetMgr_s);
	if (HI_SUCCESS != ret)
	{
		PRINTF("\e[31m ERR: \e[31m HI_SVR_PLAYER_Invoke set charsetMgr failed \e[0m \n");
	}

	/* Convert subtitle encoding to the utf8 encoding, muxSubtitleOnDrawCallback output must use utf8 character set. */
	ret = HI_SVR_PLAYER_Invoke(mux->playerHandler, HI_FORMAT_INVOKE_SET_DEST_CODETYPE, &s32CodeType);
	if (HI_SUCCESS != ret)
	{
		PRINTF("\e[31m ERR: HI_SVR_PLAYER_Invoke Send Dest CodeType failed \e[0m \n");
	}
#endif

	ret = HI_SVR_PLAYER_RegCallback(mux->playerHandler, muxEventCallBack);
	if (HI_SUCCESS != ret)
	{
		PRINTF("\e[31m ERR: register event callback function fail, ret = 0x%x \e[0m \n", ret);
	}

	/* Fast start hls */
	if (1 == mux->hlsStartMode)
	{
		HI_U32 hlsStarMode = HI_FORMAT_HLS_MODE_FAST;
		HI_SVR_PLAYER_Invoke( mux->playerHandler, HI_FORMAT_INVOKE_SET_HLS_START_MODE, &hlsStarMode);
	}

	mux->playerMedia.u32UserData = 0;

	HI_U32 u32LogLevel = HI_FORMAT_LOG_QUITE;
	HI_SVR_PLAYER_Invoke( mux->playerHandler, HI_FORMAT_INVOKE_SET_LOG_LEVEL, &u32LogLevel);
	//HI_SVR_PLAYER_EnableDbg(HI_FALSE);

	ret = HI_SVR_PLAYER_SetMedia( mux->playerHandler, HI_SVR_PLAYER_MEDIA_STREAMFILE, &mux->playerMedia);
	if (HI_SUCCESS != ret)
	{
		PRINTF("\e[31m ERR: open file fail, ret = %d! \e[0m \n", ret);
		return HI_FAILURE;
	}

	HI_S32 s32HlsStreamNum = 0;
	HI_FORMAT_HLS_STREAM_INFO_S stStreamInfo;
	ret = HI_SVR_PLAYER_Invoke( mux->playerHandler, HI_FORMAT_INVOKE_GET_HLS_STREAM_NUM, &s32HlsStreamNum);
	if (HI_SUCCESS != ret)
		PRINTF("\e[31m ERR: get hls stream num fail, ret = 0x%x! \e[0m \n", ret);
	else
		PRINTF("\e[31m get hls stream num = %d \e[0m \n", s32HlsStreamNum);

	/* Display the hls bandwidth stream info */
	for (i = 0; i < s32HlsStreamNum; i++)
	{
		stStreamInfo.stream_nb = i;

		ret = HI_SVR_PLAYER_Invoke(mux->playerHandler, HI_FORMAT_INVOKE_GET_HLS_STREAM_INFO, &stStreamInfo);
		if (HI_SUCCESS != ret)
			PRINTF("\e[31m ERR: get %d hls stream info fail, ret = 0x%x! \e[0m \n", i, ret);

		PRINTF("\nHls stream number is: %d \n", stStreamInfo.stream_nb);
		PRINTF("URL: %s \n", stStreamInfo.url);
		PRINTF("BandWidth: %lld \n", stStreamInfo.bandwidth);
		PRINTF("SegMentNum: %d \n", stStreamInfo.hls_segment_nb);
	}
	
	HI_SVR_PLAYER_METADATA_S stMetaData;
	memset(&stMetaData, 0, sizeof(HI_SVR_PLAYER_METADATA_S));
	ret = HI_SVR_PLAYER_Invoke( mux->playerHandler, HI_FORMAT_INVOKE_GET_METADATA, &stMetaData);
	if (HI_SUCCESS != ret)
	{
		PRINTF("\e[31m ERR: get metadata fail! \e[0m \n");
	}
	else
	{
		PRINTF("\e[31m get metadata success! \e[0m \n");
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
		ret = drm_check_right_status( mux->playerMedia.aszUrl, pszDrmMimeType);
		while ( ret > 0)
		{
			ret = drm_acquire_right_progress(pszDrmMimeType);
			if ( ret == 100)
			{
				PRINTF("acquiring right done\n");
				break;
			}
			PRINTF("acquiring right progress:%d%%\n", ret);
			sleep(1);
		}
		
		if( ret < 0)
		{
			PRINTF("DRM right invalid, can't play this file, exit now!\n");
			exit(0);
		}
	}
#endif

	/* Set buffer config */
	memset(&mux->bufferConfig, 0, sizeof(HI_FORMAT_BUFFER_CONFIG_S));
	mux->bufferConfig.eType = HI_FORMAT_BUFFER_CONFIG_SIZE;
	ret = HI_SVR_PLAYER_Invoke( mux->playerHandler, HI_FORMAT_INVOKE_GET_BUFFER_CONFIG, &mux->bufferConfig);
	if (HI_SUCCESS != ret)
	{
		PRINTF("\e[31m ERR: HI_SVR_PLAYER_Invoke function HI_FORMAT_INVOKE_GET_BUFFER_CONFIG fail, ret = 0x%x \e[0m \n", ret);
	}

	PRINTF("BufferConfig:type(%d)\n", mux->bufferConfig.eType);
	PRINTF("s64EventStart:%lld\n", mux->bufferConfig.s64EventStart);
	PRINTF("s64EventEnough:%lld\n", mux->bufferConfig.s64EventEnough);
	PRINTF("s64Total:%lld\n",  mux->bufferConfig.s64Total);
	PRINTF("s64TimeOut:%lld\n", mux->bufferConfig.s64TimeOut);

	ret = HI_SVR_PLAYER_GetFileInfo(mux->playerHandler, &mux->fileInfo);
	if (HI_SUCCESS == ret)
	{
		HI_SVR_PLAYER_STREAMID_S streamId = {0};

		muxOutputFileInfo(mux->fileInfo);

		ret = HI_SVR_PLAYER_GetParam( mux->playerHandler, HI_SVR_PLAYER_ATTR_STREAMID, (HI_VOID*)&streamId);
		if ((HI_SUCCESS == ret) && ( mux->fileInfo->u32ProgramNum > 0))
		{
			if ( mux->fileInfo->pastProgramInfo[streamId.u16ProgramId].u32VidStreamNum > 0 &&
				mux->fileInfo->pastProgramInfo[streamId.u16ProgramId].pastVidStream[streamId.u16VidStreamId].u32Format == HI_FORMAT_VIDEO_MVC)
			{
				muxSetVideoMode(mux, HI_UNF_DISP_3D_FRAME_PACKING);
			}

			if ( mux->fileInfo->pastProgramInfo[streamId.u16ProgramId].u32SubStreamNum > 0)
			{
				if ( mux->fileInfo->pastProgramInfo[streamId.u16ProgramId].pastSubStream[streamId.u16SubStreamId].u32Format==	HI_FORMAT_SUBTITLE_HDMV_PGS)
				{
					mux->s_bPgs = HI_TRUE;
				}
				else
				{
					mux->s_s64CurPgsClearTime = -1;
					mux->s_bPgs = HI_FALSE;
				}
			}
		}
	}
	else
	{
		PRINTF("\e[31m ERR: get file info fail! \e[0m \n");
	}

	/* Regist ondraw and onclear function after reopen file */
	ret |= HI_SVR_PLAYER_GetParam( mux->playerHandler, HI_SVR_PLAYER_ATTR_SO_HDL, &mux->soHandler);
	ret |= HI_SVR_PLAYER_GetParam( mux->playerHandler, HI_SVR_PLAYER_ATTR_AVPLAYER_HDL, &mux->avPlayHandler);
	ret |= HI_UNF_SO_RegOnDrawCb( mux->soHandler, muxSubtitleOnDrawCallback, muxSubtitleOnClearCallback, mux); // &mux->playerHandler);
	if (HI_SUCCESS != ret)
	{
		PRINTF("\e[31m ERR: set subtitle draw function fail! \e[0m \n");
	}

	//PRINTF(HELP_INFO);

	return ret;
}


HI_S32 muxInit(MUXLAB_PLAY_T *mux, HI_S32 argc, HI_CHAR **argv)
{
	HI_S32 s32Ret = 0;
	HI_S32 i = 0;

	//HI_CHAR aszInputCmd[CMD_LEN];
	
	//HI_FORMAT_BUFFER_CONFIG_S stBufConfig;
	
	sprintf(mux->playerMedia.aszUrl, "%s", argv[1]);

	//HI_U32 uHlsStartMode = 0;
	for (i = 2; i < argc; i++)
	{
		if (!strcasecmp(argv[i], "-s"))
		{
			/* Specify the external subtitle */
			muxParserSubFile(argv[i + 1], & mux->playerMedia);
			i += 1;
		}
		else if (!strcasecmp(argv[i], "-f"))
		{
			/* video fmt */
			mux->noPrintInHdmiATC = HI_TRUE;
			mux->videoFormat = HIADP_Disp_StrToFmt(argv[i + 1]);
			i += 1;
		}
#ifdef DRM_SUPPORT
		else if (!strcasecmp(argv[i], "--drm"))
		{
			/*Perform DRM operation before play*/
			drm_get_handle();//initialize drm handle
			int j = i + 1;
			char buf[2048] = "";
			for (; j < argc && j < i + 5; j++)
			{
				strcat(buf, argv[j]);
				strcat(buf, " ");
			}
			drm_cmd(buf);
			i += 1;
		}
#endif
		else if (!strcasecmp(argv[i], "--hls_fast_start"))
		{
			mux->hlsStartMode = 1;
			PRINTF("hls start mode is %d", mux->hlsStartMode );
		}
	}

	/* Gaplessplay function add code */
	HI_S32 ifsuccess = 0;
	s32Ret = playM3U9Main(argv[1]);
	memset(&(mux->playerMedia.aszUrl), 0, sizeof( mux->playerMedia.aszUrl));
	if (HI_SUCCESS == s32Ret)
	{
		sprintf( mux->playerMedia.aszUrl, "%s",  "/data/01.m3u9");
		ifsuccess++;
	}
	else
	{
		sprintf( mux->playerMedia.aszUrl, "%s", argv[1]);
	}
	
	sprintf(mux->s_aszUrl, "%s", argv[1]);

	ifsuccess = 0;

	s32Ret = _deviceInit(mux);
	s32Ret |= _higoInit( &mux->higo);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("ERR: local file play, device init fail! \n");
		return HI_FAILURE;
	}

#if defined (CHARSET_SUPPORT)
	s32Ret = HI_CHARSET_Init();
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("\e[31m ERR: charset init failed \e[0m \n");
	}

	s32Ret = HI_CHARSET_RegisterDynamic("libhi_charset.so");
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("\e[31m ERR: register charset libcharset.so fail, ret = 0x%x \e[0m \n", s32Ret);
	}
#endif

    /**<This piece of code is used to simulate switching from dvb ts playing to hiplayer playing.
      when creating hiplayer with the avplay handle which was created out of hiplayer, hiplayer will not created
      avplay by itself anymore.Setting the marco to 0 and s_stParam.hAVPlayer to HI_SVR_PLAYER_INVALID_HDL,
      hiplayer will created avplay internal by itself.
    */

#if (USE_EXTERNAL_AVPLAY == 1)
    /**<Use external created avplay handle to create hiplayer.You must do below things all by yourself.
      1-create the avplay;
      2-create the track and adjust the volume;
      3-create the window and adjust it's size and position.
      4-open the video channel and audio channel;
      5-attach the track/window to avplay,enable the window;
      6-create hiplayer with the avplay handle created in step 1.
      when switching with dvb play:
      dvb->hiplayer: stop avplay, set s_stParam.hAVPlayer's value with the avplay handle,and then creat hiplayer
          with HI_SVR_PLAYER_Create.
      hiplayer->dvb:destroy hiplayer with HI_SVR_PLAYER_Destroy,hiplay may close the av channel, then dvb need to
          reopen it.
    */
    /* CNcomment:使用外部创建的avplay句柄，该avplay必须已经绑定好window, sound设备，且需将window位置、大小设置好，
       sound音量大小设置好，HiPlayer内部对window,sound不做任何操作。
       a、dvb -> HiPlayer: 停止avplay，将avplay句柄付给s_stParam.hAVPlayer，调用HI_SVR_PLAYER_Create创建HiPlayer，
          HiPlayer会将avplay模式切换到想要的模式，并且会重新开关音视频通道；

       b、HiPlayer -> dvb: 调用HI_SVR_PLAYER_Destroy接口销毁HiPlayer，HiPlayer会关闭掉音视频通道，dvb需重新调用
          HI_UNF_AVPLAY_ChnOpen接口开启音视频通道；
    */

	s32Ret = HI_UNF_AVPLAY_GetDefaultConfig(& mux->avPlayAttr, HI_UNF_AVPLAY_STREAM_TYPE_ES); /* Jack Lee ??? */

	mux->avPlayAttr.stStreamAttr.u32VidBufSize = (20 * 1024 * 1024);
	mux->avPlayAttr.stStreamAttr.u32AudBufSize = (4 *1024 * 1024);
	s32Ret |= HI_UNF_AVPLAY_Create(&mux->avPlayAttr, &mux->avPlayHandler);
	if (HI_SUCCESS == s32Ret)
	{
		s32Ret = muxOpenVidChannel( mux );
		if (HI_SUCCESS != muxOpenAudChannel( mux) && HI_SUCCESS != s32Ret)
		{
			PRINTF("open video channel and audio channel failed, use internal avplay!\n");
			HI_UNF_AVPLAY_Destroy( mux->avPlayHandler );
			mux->avPlayHandler = HI_SVR_PLAYER_INVALID_HDL;
		}

		s32Ret = muxRecorderInit(mux);

	}
	else
	{
		PRINTF("create avplay fail! \n");
		mux->avPlayHandler = HI_SVR_PLAYER_INVALID_HDL;
	}
#else
	mux->avPlayHandler = HI_SVR_PLAYER_INVALID_HDL;
#endif

	if ((HI_HANDLE)HI_SVR_PLAYER_INVALID_HDL != mux->avPlayHandler )
	{
		PRINTF("\e[31m use external avplay! \e[0m \n");
	}

	s32Ret = _hiPlayInit( mux);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("\e[31m ERR: player init fail, ret = 0x%x \e[0m \n", s32Ret);
		return HI_FAILURE;
	}

	//PRINTF(HELP_INFO);

	s32Ret = HI_SVR_PLAYER_Play(mux->playerHandler);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("\e[31m ERR: play fail, ret = 0x%x \e[0m \n", s32Ret);
		return HI_FAILURE;
	}

	pthread_create(&mux->recordThread, HI_NULL, (HI_VOID *)muxRecorderThread, mux);

	return HI_SUCCESS;
}

static HI_VOID _deviceDeinit(HI_VOID)
{
	(HI_VOID)HI_UNF_AVPLAY_DeInit();
	(HI_VOID)HI_UNF_DMX_DeInit();
	(HI_VOID)HI_UNF_SO_DeInit();

	(HI_VOID)HIADP_VO_DeInit();
	(HI_VOID)HIADP_Disp_DeInit();
	(HI_VOID)HIADP_Snd_DeInit();
	(HI_VOID)HI_SYS_DeInit();

	return;
}

void muxDeinit(MUXLAB_PLAY_T *mux)
{
	HI_S32 s32Ret = 0;

	muxRecorderDeinit(mux);
	
	s32Ret = HI_SVR_PLAYER_Destroy(mux->playerHandler);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("### HI_SVR_PLAYER_Destroy err! \n");
	}

#if (USE_EXTERNAL_AVPLAY == 1)
	if ( mux->avPlayHandler == HI_SVR_PLAYER_INVALID_HDL)
	{
		muxCloseVidChannel(mux);
		muxCloseAudChannel(mux);
		HI_UNF_AVPLAY_Destroy(mux->avPlayHandler);
		mux->avPlayHandler = HI_SVR_PLAYER_INVALID_HDL;
	}
#endif

	s32Ret = HI_SVR_PLAYER_UnRegisterDynamic(HI_SVR_PLAYER_DLL_PARSER, NULL);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("### HI_SVR_PLAYER_UnRegisterDynamic err! \n");
	}

	/* You must call HI_SVR_PLAYER_Destroy function to release the player resource before call the deinit function. */

	s32Ret = HI_SVR_PLAYER_Deinit();
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("### HI_SVR_PLAYER_Deinit err! \n");
	}

	if (NULL != mux->soParseHand)
	{
		HI_SO_SUBTITLE_ASS_DeinitParseHand( mux->soParseHand);
		mux->soParseHand = NULL;
	}

#if defined (CHARSET_SUPPORT)
	HI_CHARSET_Deinit();
#endif

	if (HIGO_INVALID_HANDLE != mux->higo.layerHandler)
	{
		HI_GO_DestroyLayer( mux->higo.layerHandler);
		mux->higo.layerHandler = HIGO_INVALID_HANDLE;
	}

	if (HIGO_INVALID_HANDLE != mux->higo.fontHandle)
	{
		HI_GO_DestroyText(mux->higo.fontHandle);
		mux->higo.fontHandle = HIGO_INVALID_HANDLE;
	}

	mux->higo.layerSurfaceHandle = HIGO_INVALID_HANDLE;

	(HI_VOID)HI_GO_Deinit();

	_deviceDeinit();

#ifdef DRM_SUPPORT
	drm_clear_handle();
#endif

}

