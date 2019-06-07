/*
*  init functions for HiPlayer
*/

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"

#include "cmnFsm.h"
#include <assert.h>

extern	SERVICE_FSM			playerFsm;

HI_S32 _muxInitBufferConfig(MUX_PLAY_T *play)
{
	HI_S32	res = 0;
	/* set max buffer size */
	HI_S64 s64BufMaxSize;

#define VIDEO_BUFFER_TOTAL                (1000)

#define VIDEO_BUFFER_START                (200)

#define VIDEO_BUFFER_ENOUGH          (500)


//	s64BufMaxSize = 50 * 1024 * 1024;
	s64BufMaxSize = 5 * 1024 * 1024;
	res = HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_SET_BUFFER_MAX_SIZE, &s64BufMaxSize);
	if (HI_SUCCESS == res )
	{
		HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_GET_BUFFER_MAX_SIZE, &s64BufMaxSize);
		MUX_PLAY_DEBUG("Set buffer max size to %lld", s64BufMaxSize);
	}
	else
	{
		MUX_PLAY_ERROR("Set buffer max size failed");
		assert(0);
	}


	HI_FORMAT_BUFFER_CONFIG_S stBufConfig;
	/* Set buffer config */
	memset(&stBufConfig, 0, sizeof(HI_FORMAT_BUFFER_CONFIG_S));

	stBufConfig.eType = HI_FORMAT_BUFFER_CONFIG_TIME;

	res = HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_GET_BUFFER_CONFIG, &stBufConfig);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("HI_FORMAT_INVOKE_GET_BUFFER_CONFIG function fail, ret = 0x%x", res );
		assert(0);
	}
	else
	{
		MUX_PLAY_DEBUG("BufferConfig:type(%d)", stBufConfig.eType);
		MUX_PLAY_DEBUG("s64EventStart:%lld", stBufConfig.s64EventStart);
		MUX_PLAY_DEBUG("s64EventEnough:%lld", stBufConfig.s64EventEnough);
		MUX_PLAY_DEBUG("s64Total:%lld", stBufConfig.s64Total);
		MUX_PLAY_DEBUG("s64TimeOut:%lld\n", stBufConfig.s64TimeOut);

#if 0
		/* reconfigure buffer */
		stBufConfig.eType = HI_FORMAT_BUFFER_CONFIG_TIME; //HI_FORMAT_BUFFER_CONFIG_SIZE
		stBufConfig.s64Total = VIDEO_BUFFER_TOTAL; //s64BufMaxSize;//(3*1024*1024);// s64BufMaxSize;//Bytes
		stBufConfig.s64EventStart = VIDEO_BUFFER_START; //(256 * 1024);//Bytes
		stBufConfig.s64EventEnough = VIDEO_BUFFER_ENOUGH; //(2 * 1024 * 1024);//Bytes
		stBufConfig.s64TimeOut = 30000; //ms

		res = HI_SVR_PLAYER_Invoke(muxRx->playerHandler, HI_FORMAT_INVOKE_SET_BUFFER_CONFIG, &stBufConfig);
		if (HI_SUCCESS != res )
		{
			MUX_PLAY_ERROR("HI_FORMAT_INVOKE_SET_BUFFER_CONFIG function fail, ret = 0x%x", res);
		}
		else
#endif			
		{
			res = HI_SVR_PLAYER_Invoke(play->playerHandler, HI_FORMAT_INVOKE_GET_BUFFER_CONFIG, &stBufConfig);
			if (HI_SUCCESS != res)
			{
				CMN_ABORT( "HI_FORMAT_INVOKE_GET_BUFFER_CONFIG function fail, ret = 0x%x", res);
			}
			else
			{
				MUX_PLAY_DEBUG( "BufferConfig:type(%d)", stBufConfig.eType);
				MUX_PLAY_DEBUG( "s64EventStart:%lld", stBufConfig.s64EventStart);
				MUX_PLAY_DEBUG( "s64EventEnough:%lld", stBufConfig.s64EventEnough);
				MUX_PLAY_DEBUG( "s64Total:%lld", stBufConfig.s64Total);
				MUX_PLAY_DEBUG( "s64TimeOut:%lld\n", stBufConfig.s64TimeOut);
			}
		}
	}
	
	return res;
}


int _createHiPlayer(MUX_PLAY_T *play)
{
	HI_S32 ret;

	HI_SVR_PLAYER_PARAM_S		param;
	HI_HANDLE					handle = HI_SVR_PLAYER_INVALID_HDL;

	memset(&param, 0, sizeof(HI_SVR_PLAYER_PARAM_S));
	param.hAVPlayer = play->avPlayHandler;
	param.hDRMClient = HI_SVR_PLAYER_INVALID_HDL;
	
#ifdef DRM_SUPPORT
	param.hDRMClient = (HI_HANDLE)drm_get_handle();
	MUX_PLAY_DEBUG("drm handle created:%#x\n", param.hDRMClient);
#endif

	cmnThreadSetName("HiPlayer-%d", play->windowIndex );

	ret = HI_SVR_PLAYER_Create(&param, &handle);
	if (HI_SUCCESS != ret)
	{
		MUX_PLAY_ERROR(" player open fail, ret = 0x%x", ret);
		assert(0);
		return HI_FAILURE;
	}

#if defined (CHARSET_SUPPORT)
	HI_S32 s32CodeType = HI_CHARSET_CODETYPE_UTF8;
	extern HI_CHARSET_FUN_S g_stCharsetMgr_s;

	ret = HI_SVR_PLAYER_Invoke( handle, HI_FORMAT_INVOKE_SET_CHARSETMGR_FUNC, &g_stCharsetMgr_s);
	if (HI_SUCCESS != ret)
	{
		MUX_PLAY_ERROR("HI_SVR_PLAYER_Invoke set charsetMgr failed");
		assert(0);
	}

	/* Convert subtitle encoding to the utf8 encoding, muxSubtitleOnDrawCallback output must use utf8 character set. */
	ret = HI_SVR_PLAYER_Invoke(handle, HI_FORMAT_INVOKE_SET_DEST_CODETYPE, &s32CodeType);
	if (HI_SUCCESS != ret)
	{
		MUX_PLAY_ERROR("HI_SVR_PLAYER_Invoke Send Dest CodeType failed");
		assert(0);
	}
#endif

	ret = HI_SVR_PLAYER_RegCallback(handle, muxEventCallBack);
	if (HI_SUCCESS != ret)
	{
		MUX_PLAY_ERROR("register event callback function fail, ret = 0x%x", ret);
		assert(0);
	}

	/* Fast start hls */
	if (1 == play->hlsStartMode)
	{
		HI_U32 hlsStarMode = HI_FORMAT_HLS_MODE_FAST;
		HI_SVR_PLAYER_Invoke( handle, HI_FORMAT_INVOKE_SET_HLS_START_MODE, &hlsStarMode);
	}

	HI_U32 u32LogLevel = HI_FORMAT_LOG_QUITE;
	
	{
		MuxMain *muxMain = (MuxMain *)play->muxRx->muxPlayer->priv;
		if(muxMain->muxLog.llevel == CMN_LOG_DEBUG 
			|| muxMain->muxLog.llevel == CMN_LOG_INFO 
			|| muxMain->muxLog.llevel == CMN_LOG_NOTICE)
		{
//			u32LogLevel = HI_FORMAT_LOG_VERBOSE;
			u32LogLevel = HI_FORMAT_LOG_ERROR;//HI_FORMAT_LOG_INFO;
		}
		else if(muxMain->muxLog.llevel <= CMN_LOG_WARNING)
		{
			u32LogLevel = HI_FORMAT_LOG_ERROR;
		}
		
#if PLAYER_DEBUG_HIPLAY_LOG		
		u32LogLevel = HI_FORMAT_LOG_INFO;
#endif
	}

#if 1//def   __CMN_RELEASE__
	u32LogLevel = HI_FORMAT_LOG_QUITE;
	HI_SVR_PLAYER_Invoke( handle, HI_FORMAT_INVOKE_SET_LOG_LEVEL, &u32LogLevel);
	HI_SVR_PLAYER_EnableDbg(HI_TRUE);
#else
	HI_SVR_PLAYER_Invoke( handle, HI_FORMAT_INVOKE_SET_LOG_LEVEL, &u32LogLevel);
	HI_SVR_PLAYER_EnableDbg(HI_TRUE);
#endif

	{
		/* disable warn info from HiPlayer. Dec.27, 2017 */
		HI_S32 cacheTime = 10;
		char		*referer = "MuxAgent";
		ret = HI_SVR_PLAYER_Invoke(handle, HI_FORMAT_INVOKE_SET_CACHE_TIME, &cacheTime);
		if (HI_SUCCESS != ret)
		{
			MUX_ERROR("HI_FORMAT_INVOKE_SET_CACHE_TIME failed: 0x%x", ret );
		}

		ret = HI_SVR_PLAYER_Invoke(handle, HI_FORMAT_INVOKE_SET_REFERER, referer);
		if (HI_SUCCESS != ret)
		{
		
			MUX_ERROR("HI_FORMAT_INVOKE_SET_REFERER failed: 0x%x", ret);
		}

	}


//	_muxInitBufferConfig(muxRx);
	play->playerHandler = handle;
	play->osd = (MUX_OSD *)play->cfg->private;
	if(!play->osd)
	{
		CMN_ABORT("OSD of window '%s' is null", play->muxFsm.name );
	}

	play->muxFsm.currentCmd = CMD_TYPE_OPEN;	/* indicates this player is busy now */

	ret = muxPlayerPlaying(play, TRUE, play->cfg->url, -1);

	return EXIT_SUCCESS;
}


int _destoryHiPlayer(MUX_PLAY_T *play)
{
	HI_S32 res = HI_SUCCESS;
	
	res = HI_SVR_PLAYER_Destroy(play->playerHandler);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("### HI_SVR_PLAYER_Destroy err!");
		assert(0);
	}

	if (NULL != play->soParseHand)
	{
		HI_SO_SUBTITLE_ASS_DeinitParseHand( play->soParseHand);
		play->soParseHand = NULL;
	}

	return res;
}

int _createWindowChannel(MUX_PLAY_T *play )
{
	HI_S32 res = HI_SUCCESS;
	HI_UNF_WINDOW_ATTR_S stWinAttr;
	HI_HANDLE hWindow = HI_INVALID_HANDLE;

	memset(&stWinAttr, 0, sizeof(stWinAttr));
	stWinAttr.enDisp  = play->muxRx->playerParam.u32Display;
	stWinAttr.bVirtual = HI_FALSE;
	stWinAttr.stWinAspectAttr.enAspectCvrs = HI_UNF_VO_ASPECT_CVRS_IGNORE;//HI_UNF_VO_ASPECT_CVRS_IGNORE;//HI_UNF_VO_ASPECT_CVRS_LETTERBOX


	if( play->muxRx->muxPlayer->playerConfig.aspectRatioWindow != 0)
	{
		stWinAttr.stWinAspectAttr.bUserDefAspectRatio = HI_TRUE;
		stWinAttr.stWinAspectAttr.u32UserAspectHeight = play->cfg->height;
		stWinAttr.stWinAspectAttr.u32UserAspectWidth = play->cfg->width;
	}
	else
	{
		stWinAttr.stWinAspectAttr.bUserDefAspectRatio = HI_FALSE;
	}


	stWinAttr.bUseCropRect      = HI_FALSE;
	stWinAttr.stInputRect.s32X  = 0;
	stWinAttr.stInputRect.s32Y  = 0;
	stWinAttr.stInputRect.s32Width  = 0;
	stWinAttr.stInputRect.s32Height = 0;

	stWinAttr.stOutputRect.s32X = play->cfg->left;
	stWinAttr.stOutputRect.s32Y = play->cfg->top;
	stWinAttr.stOutputRect.s32Width  = play->cfg->width;
	stWinAttr.stOutputRect.s32Height = play->cfg->height;


	MUX_PLAY_DEBUG("create window#%d [(%d,%d), (%d, %d)]", play->windowIndex, play->cfg->left, play->cfg->top, play->cfg->width, play->cfg->height);

	cmnThreadSetName("Window-%d", play->windowIndex);

	/* When multiple WINDOWs are created, only one WINDOW is allowed to overlap with other WINDOWs; otherwise 
	an error of MPI will happen when createWindow or enableWindow: 0x80110044, operation denied */
	res = HI_UNF_VO_CreateWindow(&stWinAttr, &hWindow);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("create window fail, return %#x", res);
		assert(0);
		return res;
	}


	res = HI_UNF_VO_AttachWindow(hWindow, play->avPlayHandler);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR( "attach window failed, return %#x", res);
		HI_UNF_VO_DestroyWindow(hWindow);
		assert(0);
		return res;
	}

	res = HI_UNF_VO_SetWindowEnable(hWindow, HI_TRUE);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("enable window failed, return %#x", res);
		HI_UNF_VO_DetachWindow(hWindow, play->avPlayHandler);
		HI_UNF_VO_DestroyWindow(hWindow);
		assert(0);
		return res;
	}

	play->windowHandle = hWindow;
	play->rotateIndex = play->cfg->rotateType;
	muxPlayerWindowRotate(play);

	if(play->cfg->displayMode != MUX_DISPLAY_MODE_DEFAULT)
	{
		muxPlayerWindowAspect(play, play->cfg->displayMode);
	}
	
//	MUX_PLAY_DEBUG("open video channel success!");

	return HI_SUCCESS;
}


int _destoryWindowChannel(MUX_PLAY_T *play )
{
	HI_S32 s32Ret = HI_SUCCESS;
	HI_S32 s32RetTmp = HI_SUCCESS;

	if ( play->windowHandle != (HI_HANDLE)HI_SVR_PLAYER_INVALID_HDL)
	{
		s32RetTmp = HI_UNF_VO_SetWindowEnable( play->windowHandle, HI_FALSE);
		if (HI_SUCCESS !=  s32RetTmp)
		{
			MUX_PLAY_ERROR("disable window failed, return %#x", s32RetTmp);
			s32Ret = s32RetTmp;
			assert(0);
		}
		s32RetTmp = HI_UNF_VO_DetachWindow(play->windowHandle, play->avPlayHandler);
		if (HI_SUCCESS !=  s32RetTmp)
		{
			MUX_PLAY_ERROR( "detach window failed, return %#x", s32RetTmp);
			s32Ret = s32RetTmp;
		}
		
		s32RetTmp = HI_UNF_VO_DestroyWindow(play->windowHandle);
		if (HI_SUCCESS !=  s32RetTmp)
		{
			MUX_PLAY_ERROR( "destroy window fail, return %#x", s32RetTmp);
			s32Ret = s32RetTmp;
		}
		
		play->windowHandle = (HI_HANDLE)HI_SVR_PLAYER_INVALID_HDL;
	}


	return s32Ret;
}


int _createTrackChannel(int index, MUX_PLAY_T *play)
{
	HI_S32 res = HI_SUCCESS;
	HI_UNF_AUDIOTRACK_ATTR_S stTrackAttr;
	HI_UNF_SND_GAIN_ATTR_S stGain;
	HI_HANDLE hTrack = (HI_HANDLE)INVALID_TRACK_HDL;
	HI_UNF_SND_TRACK_TYPE_E type = HI_UNF_SND_TRACK_TYPE_MASTER;

	cmnThreadSetName("TrackChannel-%d", play->windowIndex);

	memset(&stTrackAttr, 0, sizeof(stTrackAttr));
	if(play->cfg->type == RECT_TYPE_MAIN )
		type = HI_UNF_SND_TRACK_TYPE_MASTER;
	else if(play->cfg->type == RECT_TYPE_PIP)
		type = HI_UNF_SND_TRACK_TYPE_SLAVE;
	else
	{
		MUX_PLAY_ERROR("RECT type %d can be used to create window", play->cfg->type);
		return HI_FAILURE;
	}
	
	stTrackAttr.enTrackType = type;
	res = HI_UNF_SND_GetDefaultTrackAttr(stTrackAttr.enTrackType, &stTrackAttr);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR( "get default track attr fail ,return 0x%x", res);
		assert(0);
		return res;
	}

	/*Warning:There must be only one master track on a sound device.*/
	stTrackAttr.enTrackType = type;
	res = HI_UNF_SND_CreateTrack(HI_UNF_SND_0, &stTrackAttr, &hTrack);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("create track fail ,return 0x%x", res);
		assert(0);
		return res;
	}

	stGain.bLinearMode = HI_TRUE;
	stGain.s32Gain = play->muxRx->playerParam.u32MixHeight;
	res = HI_UNF_SND_SetTrackWeight(hTrack, &stGain);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR( "set sound track mixer weight failed, return 0x%x", res);
	}
	
	res = HI_UNF_SND_Attach(hTrack, play->avPlayHandler );
	if (HI_SUCCESS != res)
	{
		HI_UNF_SND_DestroyTrack(hTrack);
		MUX_PLAY_ERROR("attach audio track fail, return %#x", res);
		assert(0);
		return res;
	}

	if( type == HI_UNF_SND_TRACK_TYPE_MASTER)
	{
		play->muxRx->masterTrack = hTrack;
		play->muxRx->masterIndex = index;
	}
	else
	{
#if 0
		res = HI_UNF_SND_SetTrackMute(hTrack, HI_TRUE);
		if (HI_SUCCESS != res)
		{
			HI_UNF_SND_DestroyTrack(hTrack);
			MUX_PLAY_ERROR("HI_UNF_SND_SetTrackMute failed");
			assert(0);
			return res;
		}
#endif		
	}

	if(!play->cfg->audioEnable)
	{
		res = HI_UNF_SND_SetTrackMute(hTrack, HI_TRUE);
		if (HI_SUCCESS != res)
		{
			HI_UNF_SND_DestroyTrack(hTrack);
			MUX_PLAY_ERROR("HI_UNF_SND_SetTrackMute failed");
			assert(0);
			return res;
		}
	}

#if  0
	s32Ret = HIADP_AVPlay_SetAdecAttr(muxRx->avPlayHandler, HA_AUDIO_ID_AC3PASSTHROUGH, HD_DEC_MODE_THRU,1);
	if (s32Ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("call HI_UNF_AVPLAY_SetAttr failed.");
	}
#endif

	play->trackHandle = hTrack;
	play->volume = play->muxRx->playerParam.u32MixHeight;
//	MUX_PLAY_DEBUG("open audio channel success!");

	return HI_SUCCESS;
}

int _destoryTrackChannel(MUX_PLAY_T *play)
{
	HI_S32 s32Ret = HI_SUCCESS;
	HI_S32 s32RetTmp = HI_SUCCESS;

	if ( play->trackHandle != (HI_HANDLE)INVALID_TRACK_HDL)
	{
		s32RetTmp = HI_UNF_SND_Detach( play->trackHandle, play->avPlayHandler);
		if (HI_SUCCESS != s32RetTmp)
		{
			s32Ret = s32RetTmp;
			MUX_PLAY_ERROR("detach audio track fail, return %#x", s32RetTmp);
			assert(0);
		}
		
		s32RetTmp = HI_UNF_SND_DestroyTrack(play->trackHandle);
		if (HI_SUCCESS != s32RetTmp)
		{
			s32Ret = s32RetTmp;
			MUX_PLAY_ERROR("destroy audio track fail, return %#x", s32RetTmp);
			assert(0);
		}

		play->trackHandle = (HI_HANDLE)INVALID_TRACK_HDL;
	}
	

	return s32Ret;
}


int _createAvPlay(MUX_PLAY_T *play)
{
	HI_UNF_AVPLAY_ATTR_S		avPlayAttr;
	HI_HANDLE	handle = HI_INVALID_HANDLE;
	int res;

	cmnThreadSetName("AvPlay-%d", play->windowIndex);

	res = HI_UNF_AVPLAY_GetDefaultConfig(&avPlayAttr, HI_UNF_AVPLAY_STREAM_TYPE_TS);//HI_UNF_AVPLAY_STREAM_TYPE_ES); /* Jack Lee ??? */

	avPlayAttr.u32DemuxId = 0;
#if 0
	/* when buffer size is too large, AVPlay can't be created */
	avPlayAttr.stStreamAttr.u32VidBufSize = 8 * 1024 * 1024;
	avPlayAttr.stStreamAttr.u32VidBufSize = (20 * 1024 * 1024);
#else
	avPlayAttr.stStreamAttr.u32VidBufSize = (8 * 1024 * 1024);
#endif
//	avPlayAttr.stStreamAttr.u32AudBufSize = (4 *1024 * 1024);	//don't set audio buffer
	res |= HI_UNF_AVPLAY_Create(&avPlayAttr, &handle);
	if( HI_SUCCESS != res )
	{
		MUX_PLAY_ERROR("Create AvPlay failed!");
		assert(0);
		return HI_FAILURE;
	}

	HI_UNF_AVPLAY_OPEN_OPT_S stOpenOpt;
#if 0
	stOpenOpt.enDecType  = HI_UNF_VCODEC_DEC_TYPE_NORMAL;
	stOpenOpt.enCapLevel = HI_UNF_VCODEC_CAP_LEVEL_FULLHD;
	stOpenOpt.enProtocolLevel = HI_UNF_VCODEC_PRTCL_LEVEL_H264;
#else
	stOpenOpt.enDecType         = HI_UNF_VCODEC_DEC_TYPE_NORMAL;
	stOpenOpt.enCapLevel        = HI_UNF_VCODEC_CAP_LEVEL_D1;
//	stOpenOpt.enCapLevel        = HI_UNF_VCODEC_CAP_LEVEL_4096x2160;
	stOpenOpt.enProtocolLevel   = HI_UNF_VCODEC_PRTCL_LEVEL_H264;
#endif

	cmnThreadSetName("AvpVideo-%d", play->windowIndex);
//	res = HI_UNF_AVPLAY_ChnOpen(handle, HI_UNF_AVPLAY_MEDIA_CHAN_VID, HI_NULL);// &stOpenOpt);
	res = HI_UNF_AVPLAY_ChnOpen(handle, HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stOpenOpt);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("open vid channel failed, return %#x", res);
		res = HI_UNF_AVPLAY_Destroy(handle );
		assert(0);
		return res;
	}

	cmnThreadSetName("AvpAudio-%d", play->windowIndex);
	res = HI_UNF_AVPLAY_ChnOpen( handle, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("open audio channel fail, return %#x ", res);
		res = HI_UNF_AVPLAY_Destroy(handle );
		assert(0);
		return res;
	}

	play->avPlayHandler = handle;
	if(play->cfg->type == RECT_TYPE_MAIN)
	{
		play->muxRx->masterAvPlayer = play->avPlayHandler;
	}

	return HI_SUCCESS;
}

int _destoryAvPlay(MUX_PLAY_T *play)
{
	int res;
	
	res = HI_UNF_AVPLAY_ChnClose( play->avPlayHandler, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("close audio channel fail, return %#x", res);
		assert(0);
	}
	
	res = HI_UNF_AVPLAY_ChnClose(  play->avPlayHandler, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
	if (HI_SUCCESS !=  res)
	{
		MUX_PLAY_ERROR("close video channel failed, return %#x", res);
		assert(0);
	}
	
	res = HI_UNF_AVPLAY_Destroy( play->avPlayHandler );
	if (HI_SUCCESS !=  res)
	{
		MUX_PLAY_ERROR("close video channel failed, return %#x", res);
		assert(0);
	}
	return res;
}


void _initDefaultPlayer(MUX_PLAY_T *play)
{
	play->playerHandler = HI_SVR_PLAYER_INVALID_HDL;
	play->playerState = HI_SVR_PLAYER_STATE_BUTT;
	play->pausedCtrl = PLAYER_PAUSE_NOT_PAUSE;
	
	play->soHandler  = HI_INVALID_HANDLE;
	play->avPlayHandler = HI_INVALID_HANDLE;

	play->soParseHand = NULL;	
	
	play->s_s64CurPgsClearTime = -1;

	play->hlsStartMode = 0;

	memset(&play->playerStreamIDs, 0, sizeof(HI_SVR_PLAYER_STREAMID_S));

	play->trackHandle = (HI_HANDLE)INVALID_TRACK_HDL;
	play->windowHandle = (HI_HANDLE)HI_INVALID_HANDLE;

	muxPlayerPlayspeedInit(play);
}


int muxRxCreatePlayer(int index, void *element, void *pMux )
{
	MUX_PLAY_T	*play = NULL;
	int res;

	RECT_CONFIG *cfg = (RECT_CONFIG *)element;
	MUX_RX_T *muxRx = (MUX_RX_T *)pMux;

	if((cfg->type != RECT_TYPE_MAIN) && (cfg->type != RECT_TYPE_PIP) )
	{
		assert(0);
		return HI_SUCCESS;
	}
	
//TRACE();
	play = cmn_malloc(sizeof(MUX_PLAY_T));
	if(play == NULL)
	{
		MUX_PLAY_ERROR("No memory is available for PLAYER");
		exit(1);
	}
	_initDefaultPlayer(play);

	play->muxRx = muxRx;
	play->cfg = cfg;
	play->windowIndex = index;

	play->muxFsm.fsm = &playerFsm;
	snprintf(play->muxFsm.name, CMN_NAME_LENGTH, "Player%d(%s)", index, cfg->name);
	play->muxFsm.ctx = play;
	play->muxFsm.currentState = HI_SVR_PLAYER_STATE_INIT;
	play->mutexLock = cmn_mutex_init();

//	MUX_PLAY_DEBUG("FSM :'%s': address:%x\n", play->muxFsm.fsm->name, play->muxFsm.fsm );
	
	if(muxRx->muxPlayer->playerConfig.subtitleFiles)
	{/* Specify the external subtitle */
		SUBTITLE_FILE_LIST_T *subtitle = muxRx->muxPlayer->playerConfig.subtitleFiles;

		while(subtitle)
		{
			muxParserSubFile(subtitle->name, & play->playerMedia);
			subtitle = subtitle->next;
		}
	}
	
	play->hlsStartMode = muxRx->muxPlayer->playerConfig.isHlsStartMode;
//	MUX_PLAY_DEBUG("hls start mode is %d", play->hlsStartMode );
	
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

#if 0
	sprintf(play->playerMedia.aszUrl, "%s", cfg->url);
	
	/* Gaplessplay function add code */
	res = playM3U9Main( cfg->url);
	memset(&(play->playerMedia.aszUrl), 0, sizeof( play->playerMedia.aszUrl));
	if (HI_SUCCESS == res)
	{
		sprintf( play->playerMedia.aszUrl, "%s",  "/data/01.m3u9");
	}
	else
	{
		sprintf( play->playerMedia.aszUrl, "%s", cfg->url );
	}
	MUX_PLAY_DEBUG( "Start URL '%s'", play->playerMedia.aszUrl );
	sprintf(play->currentUrl, "%s", cfg->url );
#endif
	res = _createAvPlay( play);
	if(res != HI_SUCCESS)
	{
		assert(0);
		goto ERR;
	}

	res = _createWindowChannel( play);
	if( res != HI_SUCCESS)
	{
		assert(0);
		goto ERR;
	}

#if 0
	if(cmn_list_size(&muxRx->players) <= 6)
#endif		
	{
		if (HI_SUCCESS != _createTrackChannel( cmn_list_size(&muxRx->players), play) )
		{
			MUX_PLAY_ERROR("open audio channel failed, use internal avplay!");
		assert(0);
			goto ERR;
		}
	}

//	MUX_PLAY_DEBUG( "AvHandle:0x%x; Window Handle:0x%x; Track Handle:0x%x", play->avPlayHandler, play->windowHandle, play->trackHandle );

	res = _createHiPlayer(play);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("player init fail, ret = 0x%x", res);
		assert(0);
		goto ERR;
	}
	

	cmn_list_append(&muxRx->players, play);
	
	return HI_SUCCESS;

ERR:
	_destoryTrackChannel(play);
	_destoryWindowChannel(play);
	_destoryAvPlay(play);
		
	if(play)
		cmn_free(play);

	return HI_FAILURE;
}

int muxRxDestoryPlayer(int index, void *element, void *pMux)
{
	MUX_PLAY_T	*play = (MUX_PLAY_T *)element;
	int res;
//	MUX_RX_T *muxRx = (MUX_RX_T *)pMux;
	
	res = _destoryHiPlayer( play);
	res |= _destoryTrackChannel( play);
	res |= _destoryWindowChannel( play);
	res |= _destoryAvPlay( play);

	cmn_mutex_destroy(play->mutexLock);

	return res;
}

