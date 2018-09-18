
#include "libRx.h"

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

#define	FFMPEG_SUPPORT	1

	MUX_RECORD _record;
	MUX_RECORD *record = &_record;
	RECORD_INPUT_T _input;
	RECORD_INPUT_T *input = &_input;
	
HI_VOID* muxRecorderThread(HI_VOID *data)
{
	MUXLAB_PLAY_T *mux = (MUXLAB_PLAY_T *)data;
	HI_S32	ret;


#if FFMPEG_SUPPORT
	recordInputInit(input, RECORD_MKV_FILENAME_PATH);
	recordInit(record, RECORD_MKV_FILENAME_PATH, input);
#else	
	FILE *pVidFile,*pAudFile;
	pVidFile = fopen(RECORD_VID_FILENAME_PATH, "w");
	if ( HI_NULL == pVidFile )
	{
		printf("open file %s failed\n", RECORD_VID_FILENAME_PATH);
		return HI_NULL;
	}

	pAudFile = fopen(RECORD_AUD_FILENAME_PATH, "w");
	if ( HI_NULL == pVidFile )
	{
		printf("open file %s failed\n", RECORD_AUD_FILENAME_PATH);
		fclose(pVidFile);
		return HI_NULL;
	}
#endif


	mux->isStopSaveThread = HI_FALSE;
	
	while(!mux->isStopSaveThread)
	{
		HI_BOOL bGotStream = HI_FALSE;

		if ( mux->isSaveStream)
		{
			HI_UNF_VENC_STREAM_S	videoStream;
			HI_UNF_ES_BUF_S			audioStream;

	pthread_mutex_lock(&mux->recordMutex);

#if FFMPEG_SUPPORT
			/*save video stream*/
			ret = HI_UNF_VENC_AcquireStream(mux->videoEncoder, &videoStream, 0);
			if ( HI_SUCCESS == ret )
			{
#if 0
				/* write PPS, SSP, etc. */
				if(videoStream.bFrameEnd != HI_TRUE )
				{
					printf("video frame encoding is not ended!!");
					continue;
				}
#endif				
				recordWriteVideo(record, videoStream.pu8Addr, videoStream.u32SlcLen, videoStream.u32PtsMs);
				HI_UNF_VENC_ReleaseStream(mux->videoEncoder, &videoStream);
				bGotStream = HI_TRUE;
			}
			else if ( HI_ERR_VENC_BUF_EMPTY != ret)
			{
				printf("HI_UNF_VENC_AcquireStream failed:%#x\n", ret);
			}

			/*save audio stream*/
			ret = HI_UNF_AENC_AcquireStream(mux->audioEncoder, &audioStream, 0);
			if ( HI_SUCCESS == ret )
			{
				recordWriteAudio(record, audioStream.pu8Buf, audioStream.u32BufLen, audioStream.u32PtsMs );
				HI_UNF_AENC_ReleaseStream(mux->audioEncoder, &audioStream);
				bGotStream = HI_TRUE;
			}
			else if ( HI_ERR_AENC_OUT_BUF_EMPTY != ret)
			{
				printf("HI_UNF_AENC_AcquireStream failed:%#x\n", ret);
			}

#else			
#ifdef RECORD_FILE_LEN_LIMIT
			if (ftell(pVidFile) >= RECORD_VID_FILE_MAX_LEN)
			{
				fclose(pVidFile);
				fclose(pAudFile);
				exit(1);
				pVidFile = fopen(RECORD_VID_FILENAME_PATH,"w");
				pAudFile = fopen(RECORD_AUD_FILENAME_PATH,"w");
				printf("stream files are truncated to zero\n");
			}
#endif

			/*save video stream*/
			ret = HI_UNF_VENC_AcquireStream(mux->videoEncoder, &videoStream, 0);
			if ( HI_SUCCESS == ret )
			{
				fwrite(videoStream.pu8Addr, 1, videoStream.u32SlcLen, pVidFile);
				fflush(pVidFile);
				
				HI_UNF_VENC_ReleaseStream(mux->videoEncoder, &videoStream);
				bGotStream = HI_TRUE;
			}
			else if ( HI_ERR_VENC_BUF_EMPTY != ret)
			{
				printf("HI_UNF_VENC_AcquireStream failed:%#x\n", ret);
			}

			/*save audio stream*/
			ret = HI_UNF_AENC_AcquireStream(mux->audioEncoder, &audioStream, 0);
			if ( HI_SUCCESS == ret )
			{
				fwrite( audioStream.pu8Buf, 1, audioStream.u32BufLen, pAudFile);
				fflush(pAudFile);
				
				HI_UNF_AENC_ReleaseStream(mux->audioEncoder, &audioStream);
				bGotStream = HI_TRUE;
			}
			else if ( HI_ERR_AENC_OUT_BUF_EMPTY != ret)
			{
				printf("HI_UNF_AENC_AcquireStream failed:%#x\n", ret);
			}
#endif
	pthread_mutex_unlock(&mux->recordMutex);

		}

		if ( HI_FALSE == bGotStream )
		{
//			usleep(10 * 1000);
		}
	}

#if FFMPEG_SUPPORT
#else
	fclose(pVidFile);
	fclose(pAudFile);
#endif

	return HI_NULL;
}

HI_S32 muxRecorderStart(MUXLAB_PLAY_T *mux)
{
	HI_S32 ret;

	pthread_mutex_lock(&mux->recordMutex);

	ret = HI_UNF_SND_Attach(mux->virtualTrack, mux->avPlayHandler);
	
	ret = HI_UNF_VO_AttachWindow( mux->virtualWindow, mux->avPlayHandler);

	ret = HI_UNF_VO_SetWindowEnable(mux->virtualWindow, HI_TRUE);

	mux->isSaveStream = HI_TRUE;

#if FFMPEG_SUPPORT
	recordInputInit(input, RECORD_MKV_FILENAME_PATH);
	recordInit(record, RECORD_MKV_FILENAME_PATH, input);
#endif

	pthread_mutex_unlock(&mux->recordMutex);
	printf("start transcode\n");

	return ret;
}

HI_S32 muxRecorderStop(MUXLAB_PLAY_T *mux)
{
	HI_S32 ret;
	
	pthread_mutex_lock(&mux->recordMutex);

#if FFMPEG_SUPPORT
	recordClose(record);
#endif

	mux->isSaveStream = HI_FALSE;

	ret = HI_UNF_SND_Detach(mux->virtualTrack, mux->avPlayHandler);

	ret = HI_UNF_VO_SetWindowEnable(mux->virtualWindow, HI_FALSE);
	ret = HI_UNF_VO_DetachWindow(mux->virtualWindow, mux->avPlayHandler);
	pthread_mutex_unlock(&mux->recordMutex);
	printf("stop transcode\n");

#if 0
	if ( HI_TRUE == g_bSaveStream )
	{
		g_bSaveStream = HI_FALSE;
		HI_UNF_SND_Detach(hEncTrack, hAvplay);
		HI_UNF_VO_SetWindowEnable(hEncWin, HI_FALSE);
		HI_UNF_VO_DetachWindow(hEncWin, hAvplay);
		printf("stop transcode\n");
	}
#endif

	return ret;
}


static HI_S32 _startVirtualWindow(MUXLAB_PLAY_T *mux)
{
	HI_S32  ret;
	HI_UNF_WINDOW_ATTR_S		winAttrVirtual;
	HI_UNF_VENC_CHN_ATTR_S		vEncodeChnAttr;
	
	/*create virtual window */
	winAttrVirtual.enDisp = HI_UNF_DISPLAY0;
	winAttrVirtual.bVirtual = HI_TRUE;
	winAttrVirtual.enVideoFormat = HI_UNF_FORMAT_YUV_SEMIPLANAR_420;
	winAttrVirtual.stWinAspectAttr.bUserDefAspectRatio = HI_FALSE;
	winAttrVirtual.stWinAspectAttr.enAspectCvrs = HI_UNF_VO_ASPECT_CVRS_IGNORE;
	winAttrVirtual.bUseCropRect = HI_FALSE;
	memset(&winAttrVirtual.stInputRect, 0, sizeof(HI_RECT_S));
	memset(&winAttrVirtual.stOutputRect, 0, sizeof(HI_RECT_S));
	
	ret = HI_UNF_VO_CreateWindow(&winAttrVirtual, &mux->virtualWindow);

	/*create video encoder*/
	ret = HI_UNF_VENC_Init();

	ret = HI_UNF_VENC_GetDefaultAttr(&vEncodeChnAttr);
	vEncodeChnAttr.enVencType = mux->outCfg.videoType;
	vEncodeChnAttr.enCapLevel = mux->outCfg.capLevel;
	vEncodeChnAttr.u32Width = mux->outCfg.width;
	vEncodeChnAttr.u32Height = mux->outCfg.height;
	vEncodeChnAttr.u32StrmBufSize   = mux->outCfg.strmBufSize;
	vEncodeChnAttr.u32TargetBitRate = mux->outCfg.targetBitRate;
	
	/*?????*/
	vEncodeChnAttr.u32InputFrmRate  = mux->outCfg.inputFrameRate;
	vEncodeChnAttr.u32TargetFrmRate = mux->outCfg.targetFrameRate;

	ret = HI_UNF_VENC_Create( &mux->videoEncoder, &vEncodeChnAttr);
	ret = HI_UNF_VENC_AttachInput(mux->videoEncoder, mux->virtualWindow);

	return ret;
}

static HI_S32 _startVirtualTrack(MUXLAB_PLAY_T *mux)
{
	HI_S32  ret;
	
	AAC_ENC_CONFIG				stPrivateConfig;
	HI_UNF_AENC_ATTR_S			stAencAttr;
	HI_UNF_AUDIOTRACK_ATTR_S	stTrackAttr;

	/*create virtual track */
	ret = HI_UNF_SND_GetDefaultTrackAttr(HI_UNF_SND_TRACK_TYPE_VIRTUAL, &stTrackAttr);
	
	stTrackAttr.u32OutputBufSize = 256 * 1024;
	ret = HI_UNF_SND_CreateTrack(HI_UNF_SND_0, &stTrackAttr, &mux->virtualTrack);

	/*create aenc*/
	ret = HI_UNF_AENC_Init();
	ret |= HI_UNF_AENC_RegisterEncoder("libHA.AUDIO.AAC.encode.so");

	/*create audio decoder*/
	stAencAttr.enAencType = mux->outCfg.audioType;
	HA_AAC_GetDefaultConfig(&stPrivateConfig);
	HA_AAC_GetEncDefaultOpenParam(&(stAencAttr.sOpenParam),(HI_VOID *)&stPrivateConfig);
	
	ret = HI_UNF_AENC_Create(&stAencAttr,&mux->audioEncoder);
	
	/*attach audio decoder and virtual track*/
	ret |= HI_UNF_AENC_AttachInput(mux->audioEncoder, mux->virtualTrack);
	
	return ret;
}

HI_S32 muxRecorderInit(MUXLAB_PLAY_T *mux)
{
	HI_S32 ret;

	ret = _startVirtualWindow( mux);

	ret = _startVirtualTrack(mux);

	pthread_mutex_init(&mux->recordMutex, NULL);
	//mux->isStopSaveThread = HI_FALSE;
	
	return ret;
}


void muxRecorderDeinit(MUXLAB_PLAY_T *mux)
{
	pthread_mutex_destroy(&mux->recordMutex);

	/* audio deinit */
	if ( mux->isSaveStream)
	{
		HI_UNF_SND_Detach( mux->virtualTrack, mux->avPlayHandler);
	}

	HI_UNF_AENC_DetachInput( mux->audioEncoder);
	HI_UNF_AENC_Destroy(mux->audioEncoder );
	HI_UNF_SND_DestroyTrack(mux->virtualTrack );

	/* video deinit */
	HI_UNF_VO_SetWindowEnable( mux->virtualWindow, HI_FALSE);
	if (mux->isSaveStream )
	{
		HI_UNF_VO_DetachWindow(mux->virtualWindow, mux->avPlayHandler);
	}

	HI_UNF_VENC_DetachInput( mux->videoEncoder);
	HI_UNF_VENC_Destroy( mux->videoEncoder );
	HI_UNF_VO_DestroyWindow( mux->virtualWindow);

}


void muxRecorderInitDefault(MUX_OUT_CONFIG_T *outCfg)
{

	outCfg->videoType = HI_UNF_VCODEC_TYPE_H264;
	outCfg->capLevel = HI_UNF_VCODEC_CAP_LEVEL_720P;

	outCfg->width = 1280;
	outCfg->height = 720;

#if 0
	outCfg->strmBufSize   = 1280 * 720 * 2;
#else
	outCfg->strmBufSize   = outCfg->width *outCfg->height * 2;
#endif
	outCfg->targetBitRate = 4 * 1024 * 1024;
	
	outCfg->inputFrameRate  = 50;
	outCfg->targetFrameRate = 25;

	outCfg->audioType = HA_AUDIO_ID_AAC;
}


