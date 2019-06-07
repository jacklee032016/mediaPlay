
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

#define	HI_CAPTURE_TIMEOUT_WAIT_FOREVER				0xFFFFFFFF

/* synchronize PTS/DTS of streams between Audio stream and Vidoe stream */
#define	SYNC_PTS_BETWEEN_AV				0

int64_t _gettime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

#define	SYS_TIME()				 _gettime() / 1000.0

static int _muxCaptureVideo(MuxPlayer *muxPlayer)
{
	HI_S32	ret;
	double 	_sysTime;
	
	HI_UNF_VENC_STREAM_S	videoStream;
	unsigned char _videoFrame[640*1024];
	int	_length = 0;

	MuxMediaCapture *muxCapture = &muxPlayer->mediaCapture;

#if		MUX_OPTIONS_DEBUG_CAPTURE
//			TRACE();
#endif

	/*save video stream*/
	ret = HI_UNF_VENC_AcquireStream(muxPlayer->muxRx->videoEncoder, &videoStream, HI_CAPTURE_TIMEOUT_WAIT_FOREVER);
	if ( HI_SUCCESS == ret )
	{

		_sysTime = SYS_TIME();

		
#if		MUX_OPTIONS_DEBUG_CAPTURE
		MUX_PLAY_DEBUG("HisiCapture capturing video: size :%d; pts: %d, videoStream.bFrameEnd: %s", 
			videoStream.u32SlcLen, videoStream.u32PtsMs, (videoStream.bFrameEnd==HI_TRUE)?"TRUE":"FALSE");
//		cmnHexDump( videoStream.pu8Addr, (videoStream.u32SlcLen>64)?64:videoStream.u32SlcLen);

#endif

#if SYNC_PTS_BETWEEN_AV
		if(muxPlayer->mediaCapture.capturedMedias[MUX_MEDIA_TYPE_VIDEO]->firstPts == 0)
		{
			muxPlayer->mediaCapture.capturedMedias[MUX_MEDIA_TYPE_VIDEO]->firstPts = videoStream.u32PtsMs;
		}
#endif				

#if 1
		/* write PPS, SSP, etc. */
		if(videoStream.bFrameEnd != HI_TRUE )
		{/* frame is not ended, means this buffer is SPS or PPS */
//			printf("video frame encoding is not ended!!");
//			continue;

			muxPlayer->muxRx->videoFrameCount++;
			
			if(muxPlayer->muxRx->videoFrameCount == 1)
			{
				muxPlayer->muxRx->sps.size = (videoStream.u32SlcLen > sizeof(muxPlayer->muxRx->sps.ps))?sizeof(muxPlayer->muxRx->sps.ps):videoStream.u32SlcLen;
				memcpy(muxPlayer->muxRx->sps.ps, videoStream.pu8Addr, muxPlayer->muxRx->sps.size);
#if		MUX_OPTIONS_DEBUG_CAPTURE
				MUX_PLAY_DEBUG("HisiCapture SPS: size :%d; pts: %d, videoStream.bFrameEnd: %s", 
					videoStream.u32SlcLen, videoStream.u32PtsMs, (videoStream.bFrameEnd==HI_TRUE)?"TRUE":"FALSE");
				cmnHexDump( muxPlayer->muxRx->sps.ps, muxPlayer->muxRx->sps.size);

#endif
			}

			if(muxPlayer->muxRx->videoFrameCount == 2)
			{
				muxPlayer->muxRx->pps.size = (videoStream.u32SlcLen > sizeof(muxPlayer->muxRx->pps.ps))?sizeof(muxPlayer->muxRx->pps.ps):videoStream.u32SlcLen;
				memcpy(muxPlayer->muxRx->pps.ps, videoStream.pu8Addr, muxPlayer->muxRx->pps.size);
#if		MUX_OPTIONS_DEBUG_CAPTURE
				MUX_PLAY_DEBUG("HisiCapture PPS: size :%d; pts: %d, videoStream.bFrameEnd: %s", 
					videoStream.u32SlcLen, videoStream.u32PtsMs, (videoStream.bFrameEnd==HI_TRUE)?"TRUE":"FALSE");
				cmnHexDump( muxPlayer->muxRx->pps.ps, muxPlayer->muxRx->pps.size);
#endif
			}

			/*frame is not end, so this data has same pts with the next ended frame. If it is send to stream layer, the timestamp collision will happen.
			* so when frame is not, only keey the SPS and SPS to use them in next step
			*/
			//cmnMediaCaptureWrite(&muxPlayer->mediaCapture, MUX_MEDIA_TYPE_VIDEO, videoStream.pu8Addr, videoStream.u32SlcLen, videoStream.u32PtsMs);
		}
		else
		{
			unsigned int _pts = 0;
			int64_t currentTime = 0;
			int64_t delay =0;
			unsigned char *data = NULL;
			int dataLength = 0;
			int isDrop = 0;
			

			/* this is first validate video packet */
			if(muxPlayer->muxRx->firstVideoPts == 0)
			{
				muxPlayer->muxRx->firstVideoPts = videoStream.u32PtsMs;
				muxPlayer->muxRx->startVideoTime = cmnGetTimeMs();

//				muxCapture->videoCurrentPtsTime = cmnGetTimeMs();
			}

			if(muxPlayer->muxRx->videoFrameCount != 0)
			{/* This is IDR frame : the first video data after SPS and PPS */
//				cmnMediaCaptureWrite(&muxPlayer->mediaCapture, MUX_MEDIA_TYPE_VIDEO, videoStream.pu8Addr, videoStream.u32SlcLen, videoStream.u32PtsMs);

				_length =  muxPlayer->muxRx->sps.size +muxPlayer->muxRx->pps.size +videoStream.u32SlcLen;
				if( _length > sizeof(_videoFrame) )
				{
					_length = sizeof(_videoFrame);
				}
				
				memcpy(_videoFrame, muxPlayer->muxRx->sps.ps, muxPlayer->muxRx->sps.size);
				memcpy(_videoFrame+ muxPlayer->muxRx->sps.size, muxPlayer->muxRx->pps.ps, muxPlayer->muxRx->pps.size);
				
				memcpy(_videoFrame+ muxPlayer->muxRx->sps.size +muxPlayer->muxRx->pps.size, videoStream.pu8Addr, _length -(muxPlayer->muxRx->sps.size +muxPlayer->muxRx->pps.size) );

				data = _videoFrame;
				dataLength = _length;
				
			}
			else
			{/* this is P-Slice */
#if 0
				memcpy(_videoFrame, muxPlayer->muxRx->sps.ps, muxPlayer->muxRx->sps.size);
				memcpy(_videoFrame+ muxPlayer->muxRx->sps.size, muxPlayer->muxRx->pps.ps, muxPlayer->muxRx->pps.size);
				memcpy(_videoFrame+ muxPlayer->muxRx->sps.size +muxPlayer->muxRx->pps.size, videoStream.pu8Addr, videoStream.u32SlcLen);
				_length =  muxPlayer->muxRx->sps.size +muxPlayer->muxRx->pps.size +videoStream.u32SlcLen;
				cmnMediaCaptureWrite(&muxPlayer->mediaCapture, MUX_MEDIA_TYPE_VIDEO, _videoFrame, _length, videoStream.u32PtsMs);
#else
				/* send P-Slice directly without add SPS and PPS */
				data = videoStream.pu8Addr;
				dataLength = videoStream.u32SlcLen;
#endif
			}

			currentTime = cmnGetTimeMs();
			if(muxCapture->masterClock == MUX_SYNC_MASTER_NONE || muxCapture->masterClock==MUX_SYNC_MASTER_VIDEO)
			{
				_pts = videoStream.u32PtsMs;
			}
			else if(muxCapture->masterClock == MUX_SYNC_MASTER_SYSTEM)
			{/* wait for extend */
				_pts = videoStream.u32PtsMs;
			}
			else
			{/* sync to audio */
				if(videoStream.u32PtsMs > muxCapture->audioClock || muxCapture->videoCurrentPts > muxCapture->audioClock )
				{
					isDrop = 1;

					if(videoStream.u32PtsMs > muxCapture->audioClock)
					{
						_pts = videoStream.u32PtsMs;
						MUX_PLAY_ERROR("Drop video packet with raw PTS of video packet :%d, is bigger than audio clock is %lld",  
							videoStream.u32PtsMs, muxCapture->audioClock);
					}
					else
					{
						_pts = muxCapture->videoCurrentPts;
						MUX_PLAY_ERROR("Drop video packet with PTS of video stream %lld is bigger than audio clock is %lld",  
							muxCapture->videoCurrentPts, muxCapture->audioClock);
					}

				}
				else
				{
					delay = currentTime - muxCapture->videoCurrentPtsTime;
					_pts = (int)(muxCapture->videoCurrentPts + delay );

					MUX_PLAY_DEBUG( "PTS of video stream :%lld; delay is %lld",  muxCapture->videoCurrentPts, delay );
					if(delay == 0)
					{
						CMN_ABORT("PTS of video stream delay is Zero" );
					}
				}
				
			}
			
			muxCapture->videoPacketCount++;
			muxCapture->videoCurrentPts = (int64_t)_pts;
			muxCapture->videoCurrentPtsTime = currentTime;

			if(! isDrop)
				cmnMediaCaptureWrite(muxCapture, MUX_MEDIA_TYPE_VIDEO, data, dataLength, (int64_t)_pts, (int64_t)_pts);

			if(!isDrop && muxCapture->masterClock == MUX_SYNC_MASTER_AUDIO)
			{
				int i =0;
				while(muxCapture->videoCurrentPts + delay < muxCapture->audioClock )
				{
					_pts = muxCapture->videoCurrentPts + delay;
					cmnMediaCaptureWrite(muxCapture, MUX_MEDIA_TYPE_VIDEO, data, dataLength, (int64_t)_pts, (int64_t)_pts);
					
					muxCapture->videoCurrentPts = (int64_t)_pts;
					muxCapture->videoCurrentPtsTime = cmnGetTimeMs();
					MUX_PLAY_DEBUG("Replicate video packet No.%d, PTS is update to %lld, delay:%lld ", ++i, muxCapture->videoCurrentPts, delay );

				}
			}

			/* for SPS/PPS */
			muxPlayer->muxRx->videoFrameCount = 0;


			muxPlayer->lastVideoPts = videoStream.u32PtsMs;
			muxPlayer->lastVideoSysTime = _sysTime;
			
		}
#endif				

		HI_UNF_VENC_ReleaseStream(muxPlayer->muxRx->videoEncoder, &videoStream);
//				bGotStream = HI_TRUE;
	}
	else if ( HI_ERR_VENC_BUF_EMPTY != ret)
	{
		MUX_PLAY_ERROR( "HI_UNF_VENC_AcquireStream failed:%#x", ret);
	}
#if		MUX_OPTIONS_DEBUG_CAPTURE
	else
	{
//		MUX_PLAY_DEBUG( "HI_UNF_VENC_AcquireStream video empty buffer");
	}
#endif

	return EXIT_SUCCESS;

}

static int _muxCaptureAudio(MuxPlayer *muxPlayer)
{
	HI_S32	ret;
	HI_UNF_ES_BUF_S			audioStream;
//	unsigned int	_dts;
	double _sysTime, diff;
	int	newPts;
	MuxMediaCapture *muxCapture = &muxPlayer->mediaCapture;
	double frameDuration = 1000.0/muxPlayer->playerConfig.mediaPlayCaptConfig.outputFrameRate;

	/*save audio stream*/
	ret = HI_UNF_AENC_AcquireStream(muxPlayer->muxRx->audioEncoder, &audioStream, HI_CAPTURE_TIMEOUT_WAIT_FOREVER);
	if ( HI_SUCCESS == ret )
	{
#if		MUX_OPTIONS_DEBUG_CAPTURE
		MUX_PLAY_DEBUG("HisiCapture capturing audio: size :%d; pts: %d", audioStream.u32BufLen, audioStream.u32PtsMs);
#endif

		if(audioStream.u32PtsMs == 0xffffffff)
		{
			MUX_PLAY_WARN("Invalidate PTS of audio:%d", audioStream.u32PtsMs);
			HI_UNF_AENC_ReleaseStream(muxPlayer->muxRx->audioEncoder, &audioStream);
			return EXIT_SUCCESS;
		}

		if(audioStream.u32PtsMs <= muxPlayer->lastAudioPts)
		{
//			MUX_PLAY_ERROR("PTS of current audio packet is smaller than last one:%d:%d!", audioStream.u32PtsMs, muxPlayer->lastAudioPts);
			HI_UNF_AENC_ReleaseStream(muxPlayer->muxRx->audioEncoder, &audioStream);
			muxCapture->audioPacketDrop++;
			return EXIT_SUCCESS;
		}
		
		muxPlayer->lastAudioPts = audioStream.u32PtsMs;
		/* this is first validate audio packet */
		if(muxPlayer->muxRx->firstAudioPts == 0)
		{
			muxPlayer->muxRx->firstAudioPts = audioStream.u32PtsMs;
			muxPlayer->muxRx->startAudioTime = cmnGetTimeMs();
		}

		if(muxCapture->masterClock != MUX_SYNC_MASTER_AUDIO && muxCapture->masterClock!= MUX_SYNC_MASTER_NONE )
		{
			_sysTime = SYS_TIME();
			diff = _sysTime - muxPlayer->lastVideoSysTime;
			if( diff >  frameDuration )
			{
				MUX_PLAY_ERROR("PTS of current audio is larger than next video packet: Diff: %8.4f; Duration:%8.4f!", diff, frameDuration );
				HI_UNF_AENC_ReleaseStream(muxPlayer->muxRx->audioEncoder, &audioStream);
				return EXIT_SUCCESS;
			}

			newPts = (int)diff + muxPlayer->lastVideoPts;
			MUX_PLAY_DEBUG( "sysTime: %8.4f; diff: %8.4f, newPTS:%d", _sysTime, diff, newPts );
		}
		else
		{/* free play or audio as master clock */
#if 0
			_dts = muxPlayer->muxRx->firstAudioPts + (cmnGetTimeMs() -muxPlayer->muxRx->startAudioTime);
#else
			newPts = audioStream.u32PtsMs;
			muxCapture->audioClock = (int64_t)newPts;
		}

#endif

#if SYNC_PTS_BETWEEN_AV
		if(muxPlayer->mediaCapture.capturedMedias[MUX_MEDIA_TYPE_AUDIO]->firstPts == 0 && audioStream.u32PtsMs!= -1)
		{
			muxPlayer->mediaCapture.capturedMedias[MUX_MEDIA_TYPE_AUDIO]->firstPts = audioStream.u32PtsMs;
		}

		if(muxPlayer->mediaCapture.capturedMedias[MUX_MEDIA_TYPE_VIDEO]->firstPts != 0 &&
			muxPlayer->mediaCapture.capturedMedias[MUX_MEDIA_TYPE_AUDIO]->firstPts != 0 &&
			muxPlayer->mediaCapture.ptsDifference == 0)
		{
			muxPlayer->mediaCapture.ptsDifference = muxPlayer->mediaCapture.capturedMedias[MUX_MEDIA_TYPE_VIDEO]->firstPts - muxPlayer->mediaCapture.capturedMedias[MUX_MEDIA_TYPE_AUDIO]->firstPts;
		}
		audioStream.u32PtsMs += muxPlayer->mediaCapture.ptsDifference;
#endif

#if 1
		cmnMediaCaptureWrite(muxCapture, MUX_MEDIA_TYPE_AUDIO, audioStream.pu8Buf, audioStream.u32BufLen, (int64_t)newPts, (int64_t)newPts );
#else
		if(muxPlayer->muxRx->audioLastPts != audioStream.u32PtsMs && muxPlayer->muxRx->audioLastPts != 0 )
		{/* this packet has different pts with last pts, so send last packet */
			/* send last packets */
			cmnMediaCaptureWrite(&muxPlayer->mediaCapture, MUX_MEDIA_TYPE_AUDIO, muxPlayer->muxRx->audioBuffer, muxPlayer->muxRx->audioSize, muxPlayer->muxRx->audioLastPts);

			if( muxPlayer->muxRx->audioLastPts < audioStream.u32PtsMs )
			{/* normally pts of new packet is bigger than last packet */
				/* save current packet */
				muxPlayer->muxRx->audioSize = (audioStream.u32BufLen > sizeof(muxPlayer->muxRx->audioBuffer))?sizeof(muxPlayer->muxRx->audioBuffer):audioStream.u32BufLen;
				memcpy(muxPlayer->muxRx->audioBuffer, audioStream.pu8Buf, muxPlayer->muxRx->audioSize);
			}
			else
			{/* when pts of new packet is smaller than last packet, ignore it*/
				muxPlayer->muxRx->audioSize = 0;
				MUX_PLAY_ERROR("Wrong PTS last PTS:%d, new PTS :%d, ignore this packet",  muxPlayer->muxRx->audioLastPts, audioStream.u32PtsMs);
				muxPlayer->muxRx->audioErrorPts ++;
				audioStream.u32PtsMs = muxPlayer->muxRx->audioLastPts;
			}
		}
		else
		{
			int _length = audioStream.u32BufLen;
			if(_length > (sizeof(muxPlayer->muxRx->audioBuffer)- muxPlayer->muxRx->audioSize))
				_length = sizeof(muxPlayer->muxRx->audioBuffer)- muxPlayer->muxRx->audioSize;
			
			memcpy(muxPlayer->muxRx->audioBuffer+muxPlayer->muxRx->audioSize, audioStream.pu8Buf, _length);
			muxPlayer->muxRx->audioSize += _length;
		}
		
		muxPlayer->muxRx->audioLastPts = audioStream.u32PtsMs;
#endif

		HI_UNF_AENC_ReleaseStream(muxPlayer->muxRx->audioEncoder, &audioStream);
//		bGotStream = HI_TRUE;
	}
	else if ( HI_ERR_AENC_OUT_BUF_EMPTY != ret)
	{
		MUX_PLAY_ERROR("HI_UNF_AENC_AcquireStream failed:%#x", ret);
	}
#if		MUX_OPTIONS_DEBUG_CAPTURE
	else
	{
//		MUX_PLAY_DEBUG("HI_UNF_VENC_AcquireStream audio empty buffer");
	}
#endif

	return EXIT_SUCCESS;
}

int muxRxPlayCaptureVideo(MuxPlayer *muxPlayer)
{
	HI_S32	ret;
//	MuxMain *muxMain = (MuxMain *)muxPlayer->priv;
	
	HI_BOOL bGotStream = HI_FALSE;

#if		MUX_OPTIONS_DEBUG_CAPTURE
//		TRACE();
#endif

	ret = _muxCaptureVideo(muxPlayer);

//	ret = _muxCaptureAudio(muxPlayer);

	if ( HI_FALSE == bGotStream )
	{
//			usleep(10 * 1000);
	}

	return ret;
}

int muxRxPlayCaptureAudio(MuxPlayer *muxPlayer)
{
	HI_S32	ret;
//	MuxMain *muxMain = (MuxMain *)muxPlayer->priv;
	
//	ret = _muxCaptureVideo(muxPlayer);
	ret = _muxCaptureAudio(muxPlayer);

	return ret;
}


HI_S32 muxRxPlayCaptureStart(MuxPlayer *muxPlayer)
{
	HI_S32 ret;
#if 0	
	struct tm *_time;
	time_t	currentTime;

	time(&currentTime);
	_time = localtime( &currentTime);
#endif
	ret = HI_UNF_SND_Attach(muxPlayer->muxRx->virtualTrack, muxPlayer->muxRx->masterAvPlayer);
	
	ret = HI_UNF_VO_AttachWindow( muxPlayer->muxRx->virtualWindow, muxPlayer->muxRx->masterAvPlayer);

	ret = HI_UNF_VO_SetWindowEnable(muxPlayer->muxRx->virtualWindow, HI_TRUE);

	muxPlayer->muxRx->isCapturingStream = HI_TRUE;

	muxPlayer->muxRx->videoFrameCount= 0;
	memset(&muxPlayer->muxRx->pps, 0, sizeof(MuxRxEncodePS));
	memset(&muxPlayer->muxRx->sps, 0, sizeof(MuxRxEncodePS));


	muxPlayer->muxRx->firstVideoPts = 0;
	muxPlayer->muxRx->firstAudioPts = 0;

	muxPlayer->muxRx->audioLastPts = 0;
	muxPlayer->muxRx->audioSize = 0;
	muxPlayer->muxRx->audioErrorPts = 0;

	MUX_PLAY_INFO( "PlayCapture started");

	return ret;
}

HI_S32 muxRxPlayCaptureStop(MuxPlayer *muxPlayer)
{
	HI_S32 ret;
	
	muxPlayer->muxRx->isCapturingStream = HI_FALSE;

	ret = HI_UNF_SND_Detach(muxPlayer->muxRx->virtualTrack, muxPlayer->muxRx->masterAvPlayer);

	ret = HI_UNF_VO_SetWindowEnable(muxPlayer->muxRx->virtualWindow, HI_FALSE);
	ret = HI_UNF_VO_DetachWindow(muxPlayer->muxRx->virtualWindow, muxPlayer->muxRx->masterAvPlayer);

	MUX_PLAY_INFO( "PlayCapture stopped");

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


