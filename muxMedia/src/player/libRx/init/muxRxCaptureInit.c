/*
* init all components for recording function
*/

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"

#include "HA.AUDIO.MP3.decode.h"
#include "HA.AUDIO.MP2.decode.h"
#include "HA.AUDIO.AAC.decode.h"
#include "HA.AUDIO.DRA.decode.h"
#include "HA.AUDIO.PCM.decode.h"
#include "HA.AUDIO.WMA9STD.decode.h"
#include "HA.AUDIO.AMRNB.codec.h"
#include "HA.AUDIO.AAC.encode.h"
#include "hi_unf_aenc.h"
#include "hi_unf_venc.h"


static HI_S32 _createVirtualWindowVEncoder(MUX_RX_T *muxRx)
{
	HI_S32  ret;
	HI_UNF_WINDOW_ATTR_S		winAttrVirtual;
	HI_UNF_VENC_CHN_ATTR_S		vEncodeChnAttr;
	
	MuxPlayer *muxPlayer = (MuxPlayer *)muxRx->muxPlayer;
//	MuxMain *muxMain = (MuxMain *)muxPlayer->priv;
	
	MuxMediaConfig *mediaCaptureConfig = &muxPlayer->playerConfig.mediaPlayCaptConfig;
	MuxMediaCapture *muxCapture = &muxRx->muxPlayer->mediaCapture;
	
	RECT_CONFIG  *cfg = muxGetRectConfig(muxPlayer, RECT_TYPE_MAIN);
	if(cfg == NULL)
	{
		MUX_PLAY_ERROR("No window for recording is defined");
		exit(1);
	}
	
	/*create virtual window */
//	winAttrVirtual.enDisp = HI_UNF_DISPLAY0;
	winAttrVirtual.enDisp = muxRx->playerParam.u32Display;
	winAttrVirtual.bVirtual = HI_TRUE;
	winAttrVirtual.enVideoFormat = HI_UNF_FORMAT_YUV_SEMIPLANAR_420;
	winAttrVirtual.stWinAspectAttr.bUserDefAspectRatio = HI_FALSE;
	winAttrVirtual.stWinAspectAttr.enAspectCvrs = HI_UNF_VO_ASPECT_CVRS_IGNORE;
	winAttrVirtual.bUseCropRect = HI_FALSE;
	memset(&winAttrVirtual.stInputRect, 0, sizeof(HI_RECT_S));
	memset(&winAttrVirtual.stOutputRect, 0, sizeof(HI_RECT_S));

	winAttrVirtual.stOutputRect.s32X = cfg->left;
	winAttrVirtual.stOutputRect.s32Y = cfg->top;
	winAttrVirtual.stOutputRect.s32Width  = cfg->width;
	winAttrVirtual.stOutputRect.s32Height = cfg->height;
	
	cmnThreadSetName("CaptureVideo");
	ret = HI_UNF_VO_CreateWindow(&winAttrVirtual, &muxRx->virtualWindow);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_CreateWindow Failed");
		exit(1);
	}

	/*create video encoder*/
	ret = HI_UNF_VENC_Init();
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_CreateWindow Failed");
		exit(1);
	}

	ret = HI_UNF_VENC_GetDefaultAttr(&vEncodeChnAttr);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_CreateWindow Failed");
		exit(1);
	}
	
	vEncodeChnAttr.enVencType = mediaCaptureConfig->videoType;
	vEncodeChnAttr.enVencProfile = mediaCaptureConfig->h264Profile; /* h264 profile */

	vEncodeChnAttr.enCapLevel = mediaCaptureConfig->videoCapLevel;
	vEncodeChnAttr.u32Width = mediaCaptureConfig->width;
	vEncodeChnAttr.u32Height = mediaCaptureConfig->height;
	vEncodeChnAttr.u32StrmBufSize   = mediaCaptureConfig->strmBufSize;
	vEncodeChnAttr.u32TargetBitRate = mediaCaptureConfig->targetBitRate;

	/* bitrates*/
//	vEncodeChnAttr.u32TargetBitRate = mediaCaptureConfig->videoBitrate*1024;
	/* GOP */
	vEncodeChnAttr.u32Gop = mediaCaptureConfig->gop;
	
	/*?????*/
	vEncodeChnAttr.u32InputFrmRate  = mediaCaptureConfig->inputFrameRate;
	vEncodeChnAttr.u32TargetFrmRate = mediaCaptureConfig->outputFrameRate;


	ret = HI_UNF_VENC_Create( &muxRx->videoEncoder, &vEncodeChnAttr);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("HI_UNF_VO_CreateWindow Failed");
		exit(1);
	}
	
	ret = HI_UNF_VENC_AttachInput(muxRx->videoEncoder, muxRx->virtualWindow);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("Window for recording Failed");
		exit(1);
	}

	muxCapture->durationVideoFrame = 1.0/mediaCaptureConfig->outputFrameRate;
	
	muxCapture->videoCurrentPtsTime = cmnGetTimeMs();
	muxCapture->videoCurrentPts = 0.0;

	return ret;
}

static HI_S32 _createVirtualTrackAEncoder(MUX_RX_T *muxRx)
{
	HI_S32  ret;
	
	AAC_ENC_CONFIG				stPrivateConfig;
	HI_UNF_AENC_ATTR_S			stAencAttr;
	HI_UNF_AUDIOTRACK_ATTR_S	stTrackAttr;
	
	MuxPlayer *muxPlayer = (MuxPlayer *)muxRx->muxPlayer;
	MuxMediaConfig *mediaCaptureConfig = &muxPlayer->playerConfig.mediaPlayCaptConfig;
	MuxMediaCapture *muxCapture = &muxRx->muxPlayer->mediaCapture;

	memset(&stPrivateConfig, 0, sizeof(AAC_ENC_CONFIG));
	memset(&stAencAttr, 0, sizeof(HI_UNF_AENC_ATTR_S));
	memset(&stTrackAttr, 0, sizeof(HI_UNF_AUDIOTRACK_ATTR_S));
	
	/*create virtual track */
	ret = HI_UNF_SND_GetDefaultTrackAttr(HI_UNF_SND_TRACK_TYPE_VIRTUAL, &stTrackAttr);
	
//	cmnThreadSetName("CapAudioTrack");
	stTrackAttr.u32OutputBufSize = 256 * 1024;
	ret = HI_UNF_SND_CreateTrack(HI_UNF_SND_0, &stTrackAttr, &muxRx->virtualTrack);

	/*create aenc*/
//	cmnThreadSetName("AudioEncoder");
	ret = HI_UNF_AENC_Init();
	ret |= HI_UNF_AENC_RegisterEncoder("libHA.AUDIO.AAC.encode.so");

	/*create audio decoder*/
	stAencAttr.enAencType = mediaCaptureConfig->audioType;
	
	HA_AAC_GetDefaultConfig(&stPrivateConfig);

	stPrivateConfig.sampleRate = mediaCaptureConfig->audioSampleRate;

	HA_AAC_GetEncDefaultOpenParam(&(stAencAttr.sOpenParam),(HI_VOID *)&stPrivateConfig);
	
	cmnThreadSetName(MUX_THREAD_NAME_CAPTURE_AUDIO_ENCODER);
	ret = HI_UNF_AENC_Create(&stAencAttr,&muxRx->audioEncoder);
	
	/*attach audio decoder and virtual track*/
	ret |= HI_UNF_AENC_AttachInput(muxRx->audioEncoder, muxRx->virtualTrack);

	/* this is hard code from SDK */
	if(stPrivateConfig.bitsPerSample == AAC_FORMAT_EAAC || stPrivateConfig.bitsPerSample == AAC_FORMAT_EAACPLUS )
		muxPlayer->samplesPerFrame = 1024<<1;
	else
		muxPlayer->samplesPerFrame = 1024;

	MUX_PLAY_DEBUG( "Audio sample rate:%d (%d)", stAencAttr.sOpenParam.u32DesiredSampleRate, mediaCaptureConfig->audioSampleRate);

	muxCapture->durationAudioFrame = muxRx->muxPlayer->samplesPerFrame/mediaCaptureConfig->audioSampleRate/1.0;
	
	return ret;
}

HI_S32 muxRxCaptureInit(MUX_RX_T *muxRx)
{
	HI_S32 ret = HI_SUCCESS;

	ret = _createVirtualWindowVEncoder( muxRx);
	ret |= _createVirtualTrackAEncoder(muxRx);
	
	return ret;
}


void muxRxCaptureDeinit(MUX_RX_T *muxRx)
{
	/* audio deinit */
	if ( muxRx->isCapturingStream)
	{
		HI_UNF_SND_Detach( muxRx->virtualTrack, muxRx->masterAvPlayer);
	}

	HI_UNF_AENC_DetachInput( muxRx->audioEncoder);
	HI_UNF_AENC_Destroy(muxRx->audioEncoder );
	HI_UNF_SND_DestroyTrack(muxRx->virtualTrack );

	/* video deinit */
	HI_UNF_VO_SetWindowEnable( muxRx->virtualWindow, HI_FALSE);
	if (muxRx->isCapturingStream )
	{
		HI_UNF_VO_DetachWindow(muxRx->virtualWindow, muxRx->masterAvPlayer);
	}

	HI_UNF_VENC_DetachInput( muxRx->videoEncoder);
	HI_UNF_VENC_Destroy( muxRx->videoEncoder );
	HI_UNF_VO_DestroyWindow( muxRx->virtualWindow);

}

/* mapping params to definitions of Hisilicon */
void muxRxCaptureInitDefault(MUX_RX_T *muxRx)
{
	MuxPlayer *muxPlayer = (MuxPlayer *)muxRx->muxPlayer;
	MuxMain *muxMain = (MuxMain *)muxRx->muxPlayer->priv;
	
	MuxMediaConfig *dest = &muxPlayer->playerConfig.mediaPlayCaptConfig;
	MuxMediaConfig *src = &muxMain->mediaCaptureConfig;
	memcpy(dest, src, sizeof(MuxMediaConfig) );

	if(src->videoType == OUT_VIDEO_FORMAT_HEVC)
	{
		dest->videoType = HI_UNF_VCODEC_TYPE_HEVC;
		dest->h264Profile = 0;
	}
	else
	{
		dest->videoType = HI_UNF_VCODEC_TYPE_H264;
		
		if(src->videoType == OUT_VIDEO_FORMAT_H264_BASELINE)
		{
			dest->h264Profile = HI_UNF_H264_PROFILE_BASELINE;
		}
		else if(src->videoType == OUT_VIDEO_FORMAT_H264_MAIN)
		{
			dest->h264Profile = HI_UNF_H264_PROFILE_MAIN;
		}
		else if(src->videoType == OUT_VIDEO_FORMAT_H264_EXTENDED)
		{
			dest->h264Profile = HI_UNF_H264_PROFILE_EXTENDED;
		}
		else 
		{
			dest->h264Profile = HI_UNF_H264_PROFILE_HIGH;
		}
	}

	switch(dest->videoCapLevel)
	{
		case OUT_VIDEO_SIZE_CIF:
			dest->videoCapLevel = HI_UNF_VCODEC_CAP_LEVEL_CIF;
			break;
		case OUT_VIDEO_SIZE_D1:
			dest->videoCapLevel = HI_UNF_VCODEC_CAP_LEVEL_D1;
			break;
		case OUT_VIDEO_SIZE_FULLHD:
			dest->videoCapLevel = HI_UNF_VCODEC_CAP_LEVEL_FULLHD;
			break;
		case OUT_VIDEO_SIZE_4K:
			MUX_PLAY_ERROR("4K Video encoder is not support");
//			break;
		case OUT_VIDEO_SIZE_720P:
		default:
			dest->videoCapLevel = HI_UNF_VCODEC_CAP_LEVEL_720P;
			break;
	}

	dest->strmBufSize   = dest->width *dest->height * 2;
	dest->targetBitRate = 4 * 1024 * 1024;
	
	dest->inputFrameRate  = 50;


	/* following audio params */

	/* only AAC is support in RX762/769 */
#if 0
	recordCfg->audioType = HA_AUDIO_ID_MP3;
#else
	dest->audioType = HA_AUDIO_ID_AAC;
#endif

	MUX_PLAY_DEBUG("Audio sample rate src:dest %d:%d", src->audioSampleRate, dest->audioSampleRate);

//	if(dest->audioSampleRate != 48000 || /* not support 48000 */
	if( dest->audioSampleRate != 44100 && 
		dest->audioSampleRate != 16000 && 
		dest->audioSampleRate != 22000  &&
		dest->audioSampleRate != 24000  &&
		dest->audioSampleRate != 32000  )
	{
		dest->audioSampleRate = 44100;
	}

}


