
#include "libRx.h"

HI_S32 muxParserSubFile(HI_CHAR *pargv, HI_SVR_PLAYER_MEDIA_S *pstMedia)
{
	HI_CHAR *p = NULL, *q = NULL;

	/* ½âÎöÍâ¹Ò×ÖÄ» */
	p = pargv;

	while (NULL != p)
	{
		q = strchr(p, ',');

		if (NULL == q)
		{
			sprintf(pstMedia->aszExtSubUrl[pstMedia->u32ExtSubNum], "%s", p);
			pstMedia->u32ExtSubNum++;
			break;
		}
		else
		{
			memcpy(pstMedia->aszExtSubUrl[pstMedia->u32ExtSubNum], p, q - p);
			pstMedia->u32ExtSubNum++;
		}

		if (HI_FORMAT_MAX_LANG_NUM <= pstMedia->u32ExtSubNum)
		{
			break;
		}

		p = q + 1;
	}

	return HI_SUCCESS;
}


HI_CHAR *muxGetVidFormatStr(HI_U32 u32Format)
{
	switch (u32Format)
	{
		case HI_FORMAT_VIDEO_MPEG2:
			return "MPEG2";
			break;

		case HI_FORMAT_VIDEO_MPEG4:
			return "MPEG4";
			break;

		case HI_FORMAT_VIDEO_AVS:
			return "AVS";
			break;

		case HI_FORMAT_VIDEO_H263:
			return "H263";
			break;

		case HI_FORMAT_VIDEO_H264:
			return "H264";
			break;

		case HI_FORMAT_VIDEO_REAL8:
			return "REAL8";
			break;

		case HI_FORMAT_VIDEO_REAL9:
			return "REAL9";
			break;

		case HI_FORMAT_VIDEO_VC1:
			return "VC1";
			break;

		case HI_FORMAT_VIDEO_VP6:
			return "VP6";
			break;

		case HI_FORMAT_VIDEO_DIVX3:
			return "DIVX3";
			break;

		case HI_FORMAT_VIDEO_RAW:
			return "RAW";
			break;

		case HI_FORMAT_VIDEO_JPEG:
			return "JPEG";
			break;

		case HI_FORMAT_VIDEO_MJPEG:
			return "MJPEG";
			break;
			
		case HI_FORMAT_VIDEO_MJPEGB:
			return "MJPEGB";
			break;
			
		case HI_FORMAT_VIDEO_SORENSON:
			return "SORENSON";
			break;

		case HI_FORMAT_VIDEO_VP6F:
			return "VP6F";
			break;

		case HI_FORMAT_VIDEO_VP6A:
			return "VP6A";
			break;

		case HI_FORMAT_VIDEO_VP8:
			return "VP8";
			break;
			
		case HI_FORMAT_VIDEO_MVC:
			return "MVC";
			break;
			
		case HI_FORMAT_VIDEO_SVQ1:
			return "SORENSON1";
			break;
			
		case HI_FORMAT_VIDEO_SVQ3:
			return "SORENSON3";
			break;
			
		case HI_FORMAT_VIDEO_DV:
			return "DV";
			break;
			
		case HI_FORMAT_VIDEO_WMV1:
			return "WMV1";
			break;
			
		case HI_FORMAT_VIDEO_WMV2:
			return "WMV2";
			break;
			
		case HI_FORMAT_VIDEO_MSMPEG4V1:
			return "MICROSOFT MPEG4V1";
			break;
			
		case HI_FORMAT_VIDEO_MSMPEG4V2:
			return "MICROSOFT MPEG4V2";
			break;
			
		case HI_FORMAT_VIDEO_CINEPAK:
			return "CINEPACK";
			break;
			
		case HI_FORMAT_VIDEO_RV10:
			return "RV10";
			break;
			
		case HI_FORMAT_VIDEO_RV20:
			return "RV20";
			break;
			
		case HI_FORMAT_VIDEO_INDEO4:
			return "INDEO4";
			break;
			
		case HI_FORMAT_VIDEO_INDEO5:
			return "INDEO5";
			break;
			
		case HI_FORMAT_VIDEO_HEVC:
			return "h265";
			
		case HI_FORMAT_VIDEO_VP9:
			return "VP9";
			
		default:
			return "UN-KNOWN";
			break;
	}

	return "UN-KNOWN";
}


HI_CHAR *muxGetAudFormatStr(HI_U32 u32Format)
{
	switch (u32Format)
	{
		case HI_FORMAT_AUDIO_MP2:
			return "MPEG2";
			break;
		
		case HI_FORMAT_AUDIO_MP3:
			return "MPEG3";
			break;
		
		case HI_FORMAT_AUDIO_AAC:
			return "AAC";
			break;
		
		case HI_FORMAT_AUDIO_AC3:
			return "AC3";
			break;
		
		case HI_FORMAT_AUDIO_DTS:
			return "DTS";
			break;
		
		case HI_FORMAT_AUDIO_VORBIS:
			return "VORBIS";
			break;
		
		case HI_FORMAT_AUDIO_DVAUDIO:
			return "DVAUDIO";
			break;
		
		case HI_FORMAT_AUDIO_WMAV1:
			return "WMAV1";
			break;
		
		case HI_FORMAT_AUDIO_WMAV2:
			return "WMAV2";
			break;
		
		case HI_FORMAT_AUDIO_MACE3:
			return "MACE3";
			break;
		
		case HI_FORMAT_AUDIO_MACE6:
			return "MACE6";
			break;
		
		case HI_FORMAT_AUDIO_VMDAUDIO:
			return "VMDAUDIO";
			break;
		
		case HI_FORMAT_AUDIO_SONIC:
			return "SONIC";
			break;
			
		case HI_FORMAT_AUDIO_SONIC_LS:
			return "SONIC_LS";
			break;
		
		case HI_FORMAT_AUDIO_FLAC:
			return "FLAC";
			break;
		
		case HI_FORMAT_AUDIO_MP3ADU:
			return "MP3ADU";
			break;
		
		case HI_FORMAT_AUDIO_MP3ON4:
			return "MP3ON4";
			break;
		
		case HI_FORMAT_AUDIO_SHORTEN:
			return "SHORTEN";
			break;
		
		case HI_FORMAT_AUDIO_ALAC:
			return "ALAC";
			break;
		
		case HI_FORMAT_AUDIO_WESTWOOD_SND1:
			return "WESTWOOD_SND1";
			break;
		
		case HI_FORMAT_AUDIO_GSM:
			return "GSM";
			break;
		
		case HI_FORMAT_AUDIO_QDM2:
			return "QDM2";
			break;
		
		case HI_FORMAT_AUDIO_COOK:
			return "COOK";
			break;
		
		case HI_FORMAT_AUDIO_TRUESPEECH:
			return "TRUESPEECH";
			break;
		
		case HI_FORMAT_AUDIO_TTA:
			return "TTA";
			break;
		
		case HI_FORMAT_AUDIO_SMACKAUDIO:
			return "SMACKAUDIO";
			break;
		
		case HI_FORMAT_AUDIO_QCELP:
			return "QCELP";
			break;
		
		case HI_FORMAT_AUDIO_WAVPACK:
			return "WAVPACK";
			break;
		
		case HI_FORMAT_AUDIO_DSICINAUDIO:
			return "DSICINAUDIO";
			break;
		
		case HI_FORMAT_AUDIO_IMC:
			return "IMC";
			break;
		
		case HI_FORMAT_AUDIO_MUSEPACK7:
			return "MUSEPACK7";
			break;
		
		case HI_FORMAT_AUDIO_MLP:
			return "MLP";
			break;
		
		case HI_FORMAT_AUDIO_GSM_MS:
			return "GSM_MS";
			break;
		
		case HI_FORMAT_AUDIO_ATRAC3:
			return "ATRAC3";
			break;
		
		case HI_FORMAT_AUDIO_VOXWARE:
			return "VOXWARE";
			break;
		
		case HI_FORMAT_AUDIO_APE:
			return "APE";
			break;
		
		case HI_FORMAT_AUDIO_NELLYMOSER:
			return "NELLYMOSER";
			break;
		
		case HI_FORMAT_AUDIO_MUSEPACK8:
			return "MUSEPACK8";
			break;
		
		case HI_FORMAT_AUDIO_SPEEX:
			return "SPEEX";
			break;
		
		case HI_FORMAT_AUDIO_WMAVOICE:
			return "WMAVOICE";
			break;
		
		case HI_FORMAT_AUDIO_WMAPRO:
			return "WMAPRO";
			break;
		
		case HI_FORMAT_AUDIO_WMALOSSLESS:
			return "WMALOSSLESS";
			break;
		
		case HI_FORMAT_AUDIO_ATRAC3P:
			return "ATRAC3P";
			break;
		
		case HI_FORMAT_AUDIO_EAC3:
			return "EAC3";
			break;
		
		case HI_FORMAT_AUDIO_SIPR:
			return "SIPR";
			break;
		
		case HI_FORMAT_AUDIO_MP1:
			return "MP1";
			break;
		
		case HI_FORMAT_AUDIO_TWINVQ:
			return "TWINVQ";
			break;
		
		case HI_FORMAT_AUDIO_TRUEHD:
			return "TRUEHD";
			break;
		
		case HI_FORMAT_AUDIO_MP4ALS:
			return "MP4ALS";
			break;
		
		case HI_FORMAT_AUDIO_ATRAC1:
			return "ATRAC1";
			break;
		
		case HI_FORMAT_AUDIO_BINKAUDIO_RDFT:
			return "BINKAUDIO_RDFT";
			break;
		
		case HI_FORMAT_AUDIO_BINKAUDIO_DCT:
			return "BINKAUDIO_DCT";
			break;
		
		case HI_FORMAT_AUDIO_DRA:
			return "DRA";
			break;

		case HI_FORMAT_AUDIO_PCM: /* various PCM "codecs" */
			return "PCM";
			break;

		case HI_FORMAT_AUDIO_ADPCM: /* various ADPCM codecs */
			return "ADPCM";
			break;

		case HI_FORMAT_AUDIO_AMR_NB: /* AMR */
			return "AMR_NB";
			break;
		
		case HI_FORMAT_AUDIO_AMR_WB:
			return "AMR_WB";
			break;
		
		case HI_FORMAT_AUDIO_AMR_AWB:
			return "AMR_AWB";
			break;

		case HI_FORMAT_AUDIO_RA_144: /* RealAudio codecs*/
			return "RA_144";
			break;
		
		case HI_FORMAT_AUDIO_RA_288:
			return "RA_288";
			break;

		case HI_FORMAT_AUDIO_DPCM: /* various DPCM codecs */
			return "DPCM";
			break;

		case HI_FORMAT_AUDIO_G711:  /* various G.7xx codecs */
			return "G711";
			break;
		
		case HI_FORMAT_AUDIO_G722:
			return "G722";
			break;
		
		case HI_FORMAT_AUDIO_G7231:
			return "G7231";
			break;
		
		case HI_FORMAT_AUDIO_G726:
			return "G726";
			break;
		
		case HI_FORMAT_AUDIO_G728:
			return "G728";
			break;
		
		case HI_FORMAT_AUDIO_G729AB:
			return "G729AB";
			break;
		
		case HI_FORMAT_AUDIO_PCM_BLURAY:
			return "PCM_BLURAY";
			break;
		
		default:
			break;
	}

	return "UN-KNOWN";
}


static HI_CHAR *_getSubtitleFormatStr(HI_U32 u32Format)
{
	switch (u32Format)
	{
		case HI_FORMAT_SUBTITLE_ASS:
			return "ASS";		
			break;
		
		case HI_FORMAT_SUBTITLE_LRC:
			return "LRC";
			break;
		
		case HI_FORMAT_SUBTITLE_SRT:
			return "SRT";
			break;
		
		case HI_FORMAT_SUBTITLE_SMI:
			return "SMI";
			break;
		
		case HI_FORMAT_SUBTITLE_SUB:
			return "SUB";
			break;
		
		case HI_FORMAT_SUBTITLE_TXT:
			return "TEXT";
			break;
		
		case HI_FORMAT_SUBTITLE_HDMV_PGS:
			return "HDMV_PGS";
			break;
		
		case HI_FORMAT_SUBTITLE_DVB_SUB:
			return "DVB_SUB_BMP";
			break;
		
		case HI_FORMAT_SUBTITLE_DVD_SUB:
			return "DVD_SUB_BMP";
			break;
		
		default:
			return "UN-KNOWN";
			break;
	}

	return "UN-KNOWN";
}


HI_S32 muxOutputFileInfo(HI_FORMAT_FILE_INFO_S *pstFileInfo)
{
	HI_S32 i = 0, j = 0;

	PRINTF("\n\n*************************************************\n");

	if (HI_FORMAT_STREAM_TS == pstFileInfo->eStreamType)
		PRINTF("Stream type: %s \n", "TS");
	else
		PRINTF("Stream type: %s \n", "ES");

	if (HI_FORMAT_SOURCE_NET_VOD == pstFileInfo->eSourceType)
		PRINTF("Source type: %s \n", "NETWORK VOD");
	else if (HI_FORMAT_SOURCE_NET_LIVE == pstFileInfo->eSourceType)
		PRINTF("Source type: %s \n", "NETWORK LIVE");
	else if (HI_FORMAT_SOURCE_LOCAL == pstFileInfo->eSourceType)
		PRINTF("Source type: %s \n", "LOCAL");
	else
		PRINTF("Source type: %s \n", "UNKNOWN");

	PRINTF("File size:   %lld bytes \n", pstFileInfo->s64FileSize);
	PRINTF("Start time:  %lld:%lld:%lld \n",
		pstFileInfo->s64StartTime / (1000 * 3600),
		(pstFileInfo->s64StartTime % (1000 * 3600)) / (1000 * 60),
		((pstFileInfo->s64StartTime % (1000 * 3600)) % (1000 * 60)) / 1000);
	PRINTF("Duration:    %lld:%lld:%lld \n",
		pstFileInfo->s64Duration / (1000 * 3600),
		(pstFileInfo->s64Duration % (1000 * 3600)) / (1000 * 60),
		((pstFileInfo->s64Duration % (1000 * 3600)) % (1000 * 60)) / 1000);
	PRINTF("bps:         %d bits/s \n", pstFileInfo->u32Bitrate);

	for (i = 0; i < pstFileInfo->u32ProgramNum; i++)
	{
		PRINTF("\nProgram %d: \n", i);
		PRINTF("   video info: \n");

		for (j = 0; j < pstFileInfo->pastProgramInfo[i].u32VidStreamNum; j++)
		{
			if (HI_FORMAT_INVALID_STREAM_ID != pstFileInfo->pastProgramInfo[i].pastVidStream[j].s32StreamIndex)
			{
				PRINTF("     stream idx:   %d \n", pstFileInfo->pastProgramInfo[i].pastVidStream[j].s32StreamIndex);
				PRINTF("     format:       %s \n", muxGetVidFormatStr(pstFileInfo->pastProgramInfo[i].pastVidStream[j].u32Format));
				PRINTF("     w * h:        %d * %d \n", pstFileInfo->pastProgramInfo[i].pastVidStream[j].u16Width,
					pstFileInfo->pastProgramInfo[i].pastVidStream[j].u16Height);
				PRINTF("     fps:          %d.%d \n", pstFileInfo->pastProgramInfo[i].pastVidStream[j].u16FpsInteger,
					pstFileInfo->pastProgramInfo[i].pastVidStream[j].u16FpsDecimal);
				PRINTF("     bps:          %d bits/s \n", pstFileInfo->pastProgramInfo[i].pastVidStream[j].u32Bitrate);
			}
		}

		if (pstFileInfo->pastProgramInfo[i].u32VidStreamNum <= 0)
		{
			PRINTF("     video stream is null. \n");
		}

		for (j = 0; j < pstFileInfo->pastProgramInfo[i].u32AudStreamNum; j++)
		{
			PRINTF("   audio %d info: \n", j);
			PRINTF("     stream idx:   %d \n", pstFileInfo->pastProgramInfo[i].pastAudStream[j].s32StreamIndex);
			PRINTF("     format:       %s \n", muxGetAudFormatStr(pstFileInfo->pastProgramInfo[i].pastAudStream[j].u32Format));
			PRINTF("     profile:      0x%x \n", pstFileInfo->pastProgramInfo[i].pastAudStream[j].u32Profile);
			PRINTF("     samplerate:   %d Hz \n", pstFileInfo->pastProgramInfo[i].pastAudStream[j].u32SampleRate);
			PRINTF("     bitpersample: %d \n", pstFileInfo->pastProgramInfo[i].pastAudStream[j].u16BitPerSample);
			PRINTF("     channels:     %d \n", pstFileInfo->pastProgramInfo[i].pastAudStream[j].u16Channels);
			PRINTF("     bps:          %d bits/s \n", pstFileInfo->pastProgramInfo[i].pastAudStream[j].u32Bitrate);
			PRINTF("     lang:         %s \n", pstFileInfo->pastProgramInfo[i].pastAudStream[j].aszLanguage);
		}

		for (j = 0; j < pstFileInfo->pastProgramInfo[i].u32SubStreamNum; j++)
		{
			PRINTF("   subtitle %d info: \n", j);
			PRINTF("     stream idx:     %d \n", pstFileInfo->pastProgramInfo[i].pastSubStream[j].s32StreamIndex);
			PRINTF("     sub type:       %s \n", _getSubtitleFormatStr(pstFileInfo->pastProgramInfo[i].pastSubStream[j].u32Format));
			PRINTF("     be ext sub:     %d \n", pstFileInfo->pastProgramInfo[i].pastSubStream[j].bExtSub);
			PRINTF("     original width: %d \n", pstFileInfo->pastProgramInfo[i].pastSubStream[j].u16OriginalFrameWidth);
			PRINTF("     original height:%d \n", pstFileInfo->pastProgramInfo[i].pastSubStream[j].u16OriginalFrameHeight);
		}
	}

	PRINTF("\n*************************************************\n\n");

	return HI_SUCCESS;
}

