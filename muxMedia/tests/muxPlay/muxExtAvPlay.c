
#include "libRx.h"

#if (USE_EXTERNAL_AVPLAY == 1)
HI_S32 muxOpenVidChannel(MUXLAB_PLAY_T *mux )
{
	HI_S32 s32Ret = HI_SUCCESS;
	HI_UNF_WINDOW_ATTR_S stWinAttr;
	HI_HANDLE hWindow = HI_SVR_PLAYER_INVALID_HDL;
	HI_UNF_AVPLAY_OPEN_OPT_S stOpenOpt;

	memset(&stWinAttr, 1, sizeof(stWinAttr));
	stWinAttr.enDisp  = mux->playerParam.u32Display;
	stWinAttr.bVirtual = HI_FALSE;
	stWinAttr.stWinAspectAttr.enAspectCvrs        = HI_UNF_VO_ASPECT_CVRS_LETTERBOX;//HI_UNF_VO_ASPECT_CVRS_IGNORE;//HI_UNF_VO_ASPECT_CVRS_LETTERBOX
	stWinAttr.stWinAspectAttr.bUserDefAspectRatio = HI_FALSE;
	stWinAttr.bUseCropRect      = HI_FALSE;
	stWinAttr.stInputRect.s32X  = 0;
	stWinAttr.stInputRect.s32Y  = 0;
	stWinAttr.stInputRect.s32Width  = 0;
	stWinAttr.stInputRect.s32Height = 0;
	stWinAttr.stOutputRect.s32X = mux->playerParam.x;
	stWinAttr.stOutputRect.s32Y = mux->playerParam.y;
	stWinAttr.stOutputRect.s32Width  = mux->playerParam.w;
	stWinAttr.stOutputRect.s32Height = mux->playerParam.h;
	
	s32Ret = HI_UNF_VO_CreateWindow(&stWinAttr, &hWindow);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("create window fail, return %#x\n", s32Ret);
		return s32Ret;
	}

	stOpenOpt.enDecType  = HI_UNF_VCODEC_DEC_TYPE_NORMAL;
	stOpenOpt.enCapLevel = HI_UNF_VCODEC_CAP_LEVEL_FULLHD;
	stOpenOpt.enProtocolLevel = HI_UNF_VCODEC_PRTCL_LEVEL_H264;
	s32Ret = HI_UNF_AVPLAY_ChnOpen(mux->avPlayHandler, HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stOpenOpt);
	if (HI_SUCCESS != s32Ret)
	{
		HI_UNF_VO_DestroyWindow(hWindow);
		PRINTF("open vid channel failed, return %#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = HI_UNF_VO_AttachWindow(hWindow, mux->avPlayHandler);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("attach window failed, return %#x\n", s32Ret);
		HI_UNF_AVPLAY_ChnClose(mux->avPlayHandler, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
		HI_UNF_VO_DestroyWindow(hWindow);
		return s32Ret;
	}

	s32Ret = HI_UNF_VO_SetWindowEnable(hWindow, HI_TRUE);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("enable window failed, return %#x\n", s32Ret);
		HI_UNF_VO_DetachWindow(hWindow, mux->avPlayHandler);
		HI_UNF_AVPLAY_ChnClose(mux->avPlayHandler, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
		HI_UNF_VO_DestroyWindow(hWindow);
		return s32Ret;
	}
	
	mux->windowHandle = hWindow;
	printf("open video channel success!\n");

	return HI_SUCCESS;
}


HI_S32 muxCloseVidChannel(MUXLAB_PLAY_T *mux )
{
	HI_S32 s32Ret = HI_SUCCESS;
	HI_S32 s32RetTmp = HI_SUCCESS;

	if ( mux->windowHandle != (HI_HANDLE)HI_SVR_PLAYER_INVALID_HDL)
	{
		s32RetTmp = HI_UNF_VO_SetWindowEnable( mux->windowHandle, HI_FALSE);
		if (HI_SUCCESS !=  s32RetTmp)
		{
			PRINTF("disable window failed, return %#x\n", s32RetTmp);
			s32Ret = s32RetTmp;
		}
		s32RetTmp = HI_UNF_VO_DetachWindow(mux->windowHandle, mux->avPlayHandler);
		if (HI_SUCCESS !=  s32RetTmp)
		{
			PRINTF("detach window failed, return %#x\n", s32RetTmp);
			s32Ret = s32RetTmp;
		}
		
		s32RetTmp = HI_UNF_VO_DestroyWindow(mux->windowHandle);
		if (HI_SUCCESS !=  s32RetTmp)
		{
			PRINTF("destroy window fail, return %#x\n", s32RetTmp);
			s32Ret = s32RetTmp;
		}
		
		mux->windowHandle = (HI_HANDLE)HI_SVR_PLAYER_INVALID_HDL;
	}

	s32RetTmp = HI_UNF_AVPLAY_ChnClose(  mux->avPlayHandler, HI_UNF_AVPLAY_MEDIA_CHAN_VID);
	if (HI_SUCCESS !=  s32RetTmp)
	{
		PRINTF("close video channel failed, return %#x\n", s32RetTmp);
		s32Ret = s32RetTmp;
	}

	return s32Ret;
}


HI_S32 muxOpenAudChannel(MUXLAB_PLAY_T *mux)
{
	HI_S32 s32Ret = HI_SUCCESS;
	HI_UNF_AUDIOTRACK_ATTR_S stTrackAttr;
	HI_UNF_SND_GAIN_ATTR_S stGain;
	HI_HANDLE hTrack = (HI_HANDLE)INVALID_TRACK_HDL;

	memset(&stTrackAttr, 0, sizeof(stTrackAttr));
	stTrackAttr.enTrackType = HI_UNF_SND_TRACK_TYPE_MASTER;
	s32Ret = HI_UNF_SND_GetDefaultTrackAttr(stTrackAttr.enTrackType, &stTrackAttr);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("get default track attr fail ,return 0x%x\n", s32Ret);
		return s32Ret;
	}

	/*Warning:There must be only one master track on a sound device.*/
	stTrackAttr.enTrackType = HI_UNF_SND_TRACK_TYPE_MASTER;
	s32Ret = HI_UNF_SND_CreateTrack(HI_UNF_SND_0, &stTrackAttr, &hTrack);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("create track fail ,return 0x%x\n", s32Ret);
		return s32Ret;
	}

	stGain.bLinearMode = HI_TRUE;
	stGain.s32Gain = mux->playerParam.u32MixHeight;
	s32Ret = HI_UNF_SND_SetTrackWeight(hTrack, &stGain);
	if (HI_SUCCESS == s32Ret)
	{
		PRINTF("set sound track mixer weight failed, return 0x%x\n", s32Ret);
	}

	s32Ret = HI_UNF_AVPLAY_ChnOpen(mux->avPlayHandler, HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
	if (HI_SUCCESS != s32Ret)
	{
		PRINTF("open audio channel fail, return %#x \n", s32Ret);
		HI_UNF_SND_DestroyTrack(hTrack);
		return s32Ret;
	}
	
	s32Ret = HI_UNF_SND_Attach(hTrack, mux->avPlayHandler );
	if (HI_SUCCESS != s32Ret)
	{
		HI_UNF_SND_DestroyTrack(hTrack);
		HI_UNF_AVPLAY_ChnClose( mux->avPlayHandler, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
		PRINTF("attach audio track fail, return %#x \n", s32Ret);
		return s32Ret;
	}
	
	mux->trackHandle = hTrack;
	printf("open audio channel success!\n");

	return HI_SUCCESS;
}

HI_S32 muxCloseAudChannel(MUXLAB_PLAY_T *mux)
{
	HI_S32 s32Ret = HI_SUCCESS;
	HI_S32 s32RetTmp = HI_SUCCESS;

	if ( mux->trackHandle != (HI_HANDLE)INVALID_TRACK_HDL)
	{
		s32RetTmp = HI_UNF_SND_Detach( mux->trackHandle, mux->avPlayHandler);
		if (HI_SUCCESS != s32RetTmp)
		{
			s32Ret = s32RetTmp;
			PRINTF("detach audio track fail, return %#x \n", s32RetTmp);
		}
		
		s32RetTmp = HI_UNF_SND_DestroyTrack(mux->trackHandle);
		if (HI_SUCCESS != s32RetTmp)
		{
			s32Ret = s32RetTmp;
			PRINTF("destroy audio track fail, return %#x \n", s32RetTmp);
		}

		mux->trackHandle = (HI_HANDLE)INVALID_TRACK_HDL;
	}
	
	s32RetTmp = HI_UNF_AVPLAY_ChnClose( mux->avPlayHandler, HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
	if (HI_SUCCESS != s32RetTmp)
	{
		s32Ret = s32RetTmp;
		PRINTF("close audio channel fail, return %#x \n", s32RetTmp);
	}

	return s32Ret;
}

#endif

