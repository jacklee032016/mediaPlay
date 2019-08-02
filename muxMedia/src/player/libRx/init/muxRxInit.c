/*
* enter point of init and deinit, interfaces for init/deinit are only defined in this file
*/

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"

int muxRxCreatePlayer(int index, void *element, void *pMux );
int muxRxDestoryPlayer(int index, void *element, void *pMux);


HI_S32 muxRxHigoInit(MUX_RX_T *muxRx);
HI_VOID muxRxHigoDeinit(MUX_RX_T *muxRx);


static int _muxHiPlayerInit(MUX_RX_T *muxRx)
{
	int res = 0;
	
	res = HI_SVR_PLAYER_Init();
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR( "player init fail, ret = 0x%x", res);
		return HI_FAILURE;
	}

	HI_FORMAT_LIB_VERSION_S stPlayerVersion;
	memset(&stPlayerVersion, 0, sizeof(stPlayerVersion));
	HI_SVR_PLAYER_GetVersion(&stPlayerVersion);
	MUX_PLAY_DEBUG("-----HiPlayer V%u.%u.%u.%u", stPlayerVersion.u8VersionMajor, stPlayerVersion.u8VersionMinor, stPlayerVersion.u8Revision, stPlayerVersion.u8Step);

	/* Regist file demuxers */
	res = HI_SVR_PLAYER_RegisterDynamic(HI_SVR_PLAYER_DLL_PARSER, "libformat.so");
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("register file parser libformat.so fail, ret=0x%x", res);
	}

	(HI_VOID)HI_SVR_PLAYER_RegisterDynamic(HI_SVR_PLAYER_DLL_PARSER, "libffmpegformat.so");
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("register file parser libffmpegformat.so fail, ret=0x%x", res);
	}

	return res;
}

/* only dependent on HI_UNF_ENC_FMT_E fmt */
static int _muxRxUnfDeviceInit(MUX_RX_T *muxRx)
{
	HI_S32                   ret = 0;
	//HI_UNF_EDID_BASE_INFO_S  stSinkAttr;

	cmnThreadSetName(MUX_THREAD_NAME_SOUND);
	HI_SYS_Init();

//	cmnThreadSetName("HiMCE");
	HIADP_MCE_Exit();

//	cmnThreadSetName("HiSound");
	ret = HIADP_Snd_Init();
	if (ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR( "call SndInit failed.");
		return HI_FAILURE;
	}

	cmnThreadSetName(MUX_THREAD_NAME_DISPLAY);
	//ret = HIADP_Disp_Init(HI_UNF_ENC_FMT_1080P_50);// muxRx->videoFormat );
	ret = HIADP_Disp_Init( FMT_AUTO(muxRx->videoFormat)?HI_UNF_ENC_FMT_1080P_50:muxRx->videoFormat);
	if (ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR( "call DispInit failed.");
		return HI_FAILURE;
	}

//	cmnThreadSetName("HiVideoOut");
	ret = HIADP_VO_Init(HI_UNF_VO_DEV_MODE_MOSAIC);
	//ret = HIADP_VO_Init(HI_UNF_VO_DEV_MODE_NORMAL);
	if (ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR( "call VoInit failed.");
		return HI_FAILURE;
	}

#if 0
	ret = HI_UNF_HDMI_GetSinkCapability(HI_UNF_HDMI_ID_0, &stSinkAttr);
	if (ret == HI_SUCCESS)
	{
		if (HI_TRUE == stSinkAttr.bDolbySupport)
		{
			ret = HI_UNF_DISP_SetHDRType(HI_UNF_DISPLAY1, HI_UNF_DISP_HDR_TYPE_DOLBY);
			if (HI_SUCCESS != ret)            
			{                
				MUX_PLAY_ERROR("call HI_UNF_DISP_SetHDRType Dolby failed %#x.\n", ret);
			}
		}
		else if (HI_TRUE == stSinkAttr.bHdrSupport)
		{
			ret = HI_UNF_DISP_SetHDRType(HI_UNF_DISPLAY1, HI_UNF_DISP_HDR_TYPE_HDR10);
			if (HI_SUCCESS != ret)
			{
				MUX_PLAY_ERROR("call HI_UNF_DISP_SetHDRType HDR10 failed %#x.\n", ret);
			}
		}
		else
		{
			ret = HI_UNF_DISP_SetHDRType(HI_UNF_DISPLAY1, HI_UNF_DISP_HDR_TYPE_NONE);
			if (HI_SUCCESS != ret)
			{
				MUX_PLAY_ERROR("call HI_UNF_DISP_SetHDRType SDR failed %#x.\n", ret);
			}        
		}
	}
#endif

//	ret = HI_UNF_AVPLAY_RegisterAcodecLib("libHA.AUDIO.BLURAYLPCM.decode.so");
//	ret |= HI_UNF_AVPLAY_RegisterAcodecLib("libHA.AUDIO.FFMPEG_ADEC.decode.so");
	ret |= HIADP_AVPlay_RegADecLib();
	if (ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("call HI_UNF_AVPLAY_RegisterAcodecLib failed.");
	}

#if 0
	ret = HI_UNF_AVPLAY_RegisterVcodecLib("libHV.VIDEO.FFMPEG_VDEC.decode.so");
	if (ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR( "call HI_UNF_AVPLAY_RegisterVcodecLib return 0x%x.", ret);
	}
#endif

//	cmnThreadSetName("HiDMX");
	ret = HI_UNF_DMX_Init();
	if (HI_SUCCESS != ret)
	{
		MUX_PLAY_ERROR("### HI_UNF_DMX_Init init fail.");
		return HI_FAILURE;
	}

//	cmnThreadSetName("HiAvPlayInit");
	ret = HI_UNF_AVPLAY_Init();
	if (ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR( "call HI_UNF_AVPLAY_Init failed.");
		return HI_FAILURE;
	}

//	cmnThreadSetName("HiSOInit");
	ret = HI_UNF_SO_Init();
	if (HI_SUCCESS != ret)
	{
		MUX_PLAY_ERROR("### HI_UNF_SO_Init init fail.");
		return HI_FAILURE;
	}

#if defined (CHARSET_SUPPORT)
	ret = HI_CHARSET_Init();
	if (HI_SUCCESS != ret)
	{
		MUX_PLAY_ERROR("charset init failed");
	}

	ret = HI_CHARSET_RegisterDynamic("libhi_charset.so");
	if (HI_SUCCESS != ret)
	{
		MUX_PLAY_ERROR(": register charset libcharset.so fail, ret = 0x%x", ret);
	}
#endif

//	usleep(3000 * 1000);

	return HI_SUCCESS;
}

static HI_VOID _muxRxUnfDeviceDeinit(HI_VOID)
{
	(HI_VOID)HI_SVR_PLAYER_Deinit();

#if defined (CHARSET_SUPPORT)
	(HI_VOID)HI_CHARSET_Deinit();
#endif

	(HI_VOID)HI_UNF_AVPLAY_DeInit();
	(HI_VOID)HI_UNF_DMX_DeInit();
	(HI_VOID)HI_UNF_SO_DeInit();

	(HI_VOID)HIADP_VO_DeInit();
	(HI_VOID)HIADP_Disp_DeInit();
	(HI_VOID)HIADP_Snd_DeInit();
	(HI_VOID)HI_SYS_DeInit();

	return;
}


HI_S32 muxRxInit(MUX_RX_T *muxRx)
{
	MuxPlayer *muxPlayer = (MuxPlayer *)muxRx->muxPlayer; 
	
	int res = 0;

	muxRx->playerParam.u32Display = HI_UNF_DISPLAY1;
	muxRx->playerParam.u32MixHeight = muxRx->muxPlayer->playerConfig.audioVolume;
	muxRx->playerParam.u32VDecErrCover = 100;
	
	muxRx->videoFormat = HI_UNF_ENC_FMT_720P_50;
//	muxRx->videoFormat = HI_UNF_ENC_FMT_1080i_50;


	if(strlen(muxPlayer->playerConfig.displayFormat))
	{/* video fmt */
		muxRx->noPrintInHdmiATC = HI_TRUE;
//		muxRx->videoFormat = HIADP_Disp_StrToFmt(muxRx->muxPlayer->playerConfig.displayFormat);
		muxRx->videoFormat = muxHdmiFormatCode(muxRx->muxPlayer->playerConfig.displayFormat);
	}
	MUX_PLAY_INFO("Display Format:%s:%d", muxPlayer->playerConfig.displayFormat, muxRx->videoFormat);

	muxRx->initLock = cmn_mutex_init();

	/* sound/display modules and threads */		
	res = _muxRxUnfDeviceInit(muxRx);


//	cmnThreadSetName("HiPlayer");
	res |= _muxHiPlayerInit( muxRx);

	cmnThreadSetName("HiOSD");
	res |= muxRxHigoInit( muxRx);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("local file play, device init fail!");
		return HI_FAILURE;
	}

//	cmnThreadSetName("Players");

	cmn_mutex_lock(muxRx->initLock);
	res |= cmn_list_iterate(&muxPlayer->playerConfig.windows, HI_TRUE, muxRxCreatePlayer, muxRx);
	if (HI_SUCCESS != res)
	{
		cmn_mutex_unlock(muxRx->initLock);
		MUX_PLAY_ERROR("Initialized window failed, please check your configuration!");
		return HI_FAILURE;
	}
	cmn_mutex_unlock(muxRx->initLock);

	muxRxCaptureInitDefault( muxRx );

#if 0
	cmnThreadSetName("HiDevice");
	res = _muxRxUnfDeviceInit(muxRx);
#endif

	cmnThreadSetName("PlayCapture");
	res = muxRxCaptureInit(muxRx);

	res = muxPlayerSetMainWindowTop(muxRx);

	//usleep(10000 * 1000);
	usleep(10 * 1000);

	return HI_SUCCESS;
}


void muxRxDeinit(MUX_RX_T *muxRx)
{
	HI_S32 res = 0;

	muxRxCaptureDeinit(muxRx);
	
	res |= cmn_list_iterate(&muxRx->players, HI_TRUE, muxRxDestoryPlayer, muxRx);

	cmn_list_ofchar_free(&muxRx->players, FALSE);

	/* You must call HI_SVR_PLAYER_Destroy function to release the player resource before call the deinit function. */

	res = HI_SVR_PLAYER_UnRegisterDynamic(HI_SVR_PLAYER_DLL_PARSER, NULL);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR( "### HI_SVR_PLAYER_UnRegisterDynamic err! \n");
	}

	res = HI_SVR_PLAYER_Deinit();
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR( "### HI_SVR_PLAYER_Deinit err! \n");
	}


#if defined (CHARSET_SUPPORT)
	HI_CHARSET_Deinit();
#endif

	muxRxHigoDeinit(muxRx);

	_muxRxUnfDeviceDeinit();

#ifdef DRM_SUPPORT
	drm_clear_handle();
#endif

}


