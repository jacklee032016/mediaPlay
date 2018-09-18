/*
*
*/

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>

#include "_muxMedia.h"

static AVCodecParameters *__createParamsVideo(MuxMediaConfig *mediaCaptureConfig)
{
	AVCodecParameters *param = NULL;
	
	/* following data is used for initializing AvStream */	
	//NOTE we previously allocated internal & internal->avctx, these seemed uneeded though
	param = avcodec_parameters_alloc();

	param->codec_type = AVMEDIA_TYPE_VIDEO;
	if(mediaCaptureConfig->videoType == OUT_VIDEO_FORMAT_HEVC)
		param->codec_id = AV_CODEC_ID_HEVC;
	else	
		param->codec_id = AV_CODEC_ID_H264;

	param->format = AV_PIX_FMT_YUV420P;

	switch(mediaCaptureConfig->videoCapLevel)
	{
		case  OUT_VIDEO_SIZE_CIF:
			param->width = 352;
			param->height = 288;
			break;
		case  OUT_VIDEO_SIZE_D1:
			param->width = 720;
			param->height = 576;
			break;
		case  OUT_VIDEO_SIZE_FULLHD:
			param->width = 1920;
			param->height = 1080;
			break;
		case  OUT_VIDEO_SIZE_4K:
			param->width = 1920*2;
			param->height = 1080*2;
			break;
		case  OUT_VIDEO_SIZE_720P:
		default:	
			param->width = 1280;
			param->height = 720;
			break;
	}

	mediaCaptureConfig->width = param->width;
	mediaCaptureConfig->height = param->height;

	param->sample_rate = mediaCaptureConfig->outputFrameRate;

#if 0
	/* add extradata of H264, for SDP of H264 stream: sprop-parameter-sets. jack Lee,  */
	static unsigned char extradata[] = {0x00, 0x00, 0x00, 0x01, 0x67, 0x64, 0x00, 0x1f,0xac, 0xb4, 0x02, 0x80, 0x2d, 0xc8/* SPS*/, 
		0x00, 0x00, 0x00, 0x01, 0x68, 0xee, 0x3c, 0x61, 0x0f, 0xff, 0xf0, 0x87, 0xff, 0xf8, 0x43, 0xff, 0xfc, 0x21, 0xff, 0xfe, 0x10, 0xff, 0xff, 0x08, 0x7f, 0xff, 0xc0 }; 

	param->extradata = extradata;
	param->extradata_size = sizeof(extradata);
#endif

	param->codec_tag = 0;

	return param;
}

static int _createDescriberVideo(MuxMediaCapture *mediaCapture)
{
	MuxMediaDescriber *desc;
	AVCodecParameters *params;
//	int ret = 0;

	desc = av_mallocz(sizeof(*desc));
	if (!desc)
	{
		MUX_ERROR("No memory for MuxMediaDescriber");
		exit(1);
		return EXIT_FAILURE;
	}

	params = __createParamsVideo( &mediaCapture->muxMain->mediaCaptureConfig);
	
	desc->codecPar = params;
	desc->index = MUX_MEDIA_TYPE_VIDEO;
	desc->pts_wrap_bits = 33;

	/* following data is not for creating AvStream */
	snprintf(desc->type, sizeof(desc->type), "%s", av_get_media_type_string(params->codec_type));
	snprintf(desc->codec, sizeof(desc->codec), "%s", avcodec_get_name(params->codec_id) );
	snprintf(desc->name, sizeof(desc->name), "RX769-%s.%s", desc->type, desc->codec);
	desc->bandwidth = 0;
	desc->fps = mediaCapture->muxMain->mediaCaptureConfig.outputFrameRate;
	
	desc->timeBaseNumerator = 1;
#if 0
	desc->timeBaseDenominator = desc->fps;
#else
	desc->timeBaseDenominator = 1000;
#endif
	desc->sampleAspectRatioNumerator = 0;
	desc->sampleAspectRatioDenominator = 0;

	desc->recommended_encoder_configuration = NULL;		/* */

#if 0
	stream->r_frame_rate = (AVRational ){ STREAM_FRAME_RATE, 1};
	stream->avg_frame_rate = (AVRational){ STREAM_FRAME_RATE, 1};
#endif	

	mediaCapture->capturedMedias[mediaCapture->nbStreams++] = desc;

	return EXIT_SUCCESS;
}


static AVCodecParameters *__createParamsAudio(MuxMediaConfig *mediaCaptureConfig)
{
	static unsigned char extradata[2]; //main feature!!!
	AVCodecParameters *param = NULL;
	
	/* following data is used for initializing AvStream */	
	//NOTE we previously allocated internal & internal->avctx, these seemed uneeded though
	param = avcodec_parameters_alloc();

	param->codec_type = AVMEDIA_TYPE_AUDIO;

	switch(mediaCaptureConfig->audioType)
	{
		default:
			param->codec_id = AV_CODEC_ID_AAC;
			break;
//	param->codec_id = AV_CODEC_ID_AC3;
	}

	if(mediaCaptureConfig->audioFormat == 16)
		param->format = AV_SAMPLE_FMT_S16;
	else
		param->format = AV_SAMPLE_FMT_S32;

	param->format = AV_SAMPLE_FMT_S16;

	param->sample_rate = mediaCaptureConfig->audioSampleRate;

	if(mediaCaptureConfig->audioChannels == 2)
		param->channel_layout = AV_CH_LAYOUT_STEREO;
	else
		param->channel_layout = AV_CH_LAYOUT_MONO;

	param->codec_tag = 0;

/*
config = 1588 (aac-lc object type = 2; sampling rate 8Khz; number of channels = 1; 1024 samples; this comes out to be 1588 according to ISO14496-3 specs) 
*/

//	static unsigned char extradata[] = {0x15, 0x88};
/*
config = 1588 (aac-lc object type = 2; sampling rate 48Khz(0x03); number of channels = 2(2); 1024 samples; this comes out to be 1588 according to ISO14496-3 specs) 
AAC-LC: 00010 (5 bit), Sample Rate 48K: 0011; channels: 0010
*/

/*  ISO/IEC 14496-2:2000 Section 6. P33 ~ P39
*   | 5 bits, ObjType | 4 bits, freq | 4 bits, channel | 2 bit, epConfig | 1 bit directMapping |
ObjType: AAC LC: 00010, 
Freq: 48K, 0x3, 0011; 44.1K, 0x4, 0100; 32K, 0x5, 0101, 
Channel
	| Type       | Freq     | Chan    |      |
	| 0 0 0 1 0 |  0 0 1 ,1 | 0 0 1 0 | 0 0 | 0 |    48K, 0x11 0x90
                      |  0 1 0 ,0 | 0 0 1 0 |                 44K, 0x12 0x10
                      |  0 1 0 ,1 | 0 0 1 0 |                 32K, 0x12 0x90
                      |  0 1 1 ,0 | 0 0 1 0 |                 24K, 0x13 0x10
                      |  0 1 1 ,1 | 0 0 1 0 |                 22K, 0x13 0x90
                      |  1 0 0 ,0 | 0 0 1 0 |                 16K, 0x14 0x10
                      
*/

	if(param->sample_rate == 48000)
	{
		extradata[0] = 0x11;
		extradata[1] = 0x90;
	}
	else if(param->sample_rate == 44100)
	{
		extradata[0] = 0x12;
		extradata[1] = 0x10;
	}
	else if(param->sample_rate == 32000)
	{
		extradata[0] = 0x12;
		extradata[1] = 0x90;
	}
	else if(param->sample_rate == 24000)
	{
		extradata[0] = 0x13;
		extradata[1] = 0x10;
	}
	else if(param->sample_rate == 22000)
	{
		extradata[0] = 0x13;
		extradata[1] = 0x90;
	}
	else/* 16000*/
	{
		extradata[0] = 0x14;
		extradata[1] = 0x10;
	}
	
#if 1	
//	static unsigned char extradata[] = {0x11, 0x98, 0xad, 0xd2}; //main feature!!!
	param->extradata = extradata;
	param->extradata_size = sizeof(extradata);
#endif

	return param;
}

static int _createDescriberAudio(MuxMediaCapture *mediaCapture)
{
	MuxMediaDescriber *desc;
	AVCodecParameters *params;

	desc = av_mallocz(sizeof(*desc));
	if (!desc)
	{
		MUX_ERROR("No memory for MuxMediaDescriber");
		exit(1);
		return EXIT_FAILURE;
	}

	params = __createParamsAudio( &mediaCapture->muxMain->mediaCaptureConfig);
	
	desc->codecPar = params;
	desc->index = MUX_MEDIA_TYPE_AUDIO;
	desc->pts_wrap_bits = 33;
	
	snprintf(desc->type, sizeof(desc->type), "%s", av_get_media_type_string(params->codec_type));
	snprintf(desc->codec, sizeof(desc->codec), "%s", avcodec_get_name(params->codec_id) );
	snprintf(desc->name, sizeof(desc->name), "RX769-%s.%s", desc->type, desc->codec);
	desc->bandwidth = 0;
	
	desc->timeBaseNumerator = 1;
	desc->timeBaseDenominator = params->sample_rate;
	
	mediaCapture->capturedMedias[mediaCapture->nbStreams++] = desc;
	
	return EXIT_SUCCESS;
}

static int _createDescriberSubtitle(MuxMediaCapture *mediaCapture)
{
#if 0
	MuxMediaDescriber *desc = NULL;
	AVCodecParameters *params;

	desc = av_mallocz(sizeof(*desc));
	if (!desc)
	{
		MUX_ERROR("No memory for MuxMediaDescriber");
		exit(1);
		return NULL;
	}
	params = avcodec_parameters_alloc();

	params->codec_type = AVMEDIA_TYPE_SUBTITLE;
	params->codec_id = AV_CODEC_ID_TEXT;

	params->codec_tag = 0;
	
	desc->codecPar = params;
	snprintf(desc->type, sizeof(desc->type), "%s", av_get_media_type_string(params->codec_type));
	snprintf(desc->codec, sizeof(desc->codec), "%s", avcodec_get_name(params->codec_id) );
	desc->bandwidth = 0;
	mediaRuntime->capturedMedias[mediaRuntime->nbStreams++] = desc;
#endif

	return EXIT_SUCCESS;
}

/* create Media Describers and add into MediaCapture for Capture medias. Only for Player, created from Media Configuration */
int cmnMediaCreateDescribersForPlayer(MuxMediaCapture *mediaCapture)
{
	if(_createDescriberVideo(mediaCapture) )
	{
		return EXIT_FAILURE;
	}

	if(_createDescriberAudio(mediaCapture) )
	{
		return EXIT_FAILURE;
	}

	if(_createDescriberSubtitle(mediaCapture) )
	{
		return EXIT_FAILURE;
	}

	MUX_DEBUG("MediaCapture '%s' has %d stream", mediaCapture->name, mediaCapture->nbStreams);
	return EXIT_SUCCESS;
}

int	cmnMediaInit(MuxMediaConfig *mediaCaptureConfig)
{
	int ret = 0;
	
#if 0	
	int level = AV_LOG_WARNING;
	av_register_all();
	ret = avformat_network_init();

	if(mediaCaptureConfig->logLevel == CMN_LOG_DEBUG)
	{
		level = AV_LOG_TRACE;
	}
	else if(mediaCaptureConfig->logLevel == CMN_LOG_INFO)
	{
		level = AV_LOG_DEBUG;
	}
	else if(mediaCaptureConfig->logLevel == CMN_LOG_NOTICE)
	{
		level = AV_LOG_INFO;
	}
	else if(mediaCaptureConfig->logLevel == CMN_LOG_WARNING)
	{
		level = AV_LOG_WARNING;
	}
	else if(mediaCaptureConfig->logLevel == CMN_LOG_ERR)
	{
		level = AV_LOG_ERROR;
	}
	av_log_set_level( level);
#else
	MuxMain *muxMain = (MuxMain *)mediaCaptureConfig->priv;
	MUX_INIT_FFMPEG(muxMain->muxLog.llevel);
#endif

//	mediaRuntime->isInited = TRUE;

	/* enhancement here, redirect ffmpeg log to log */
//	av_log_set_callback(_avLogCallback);

	
	cmnMediaScanInit(mediaCaptureConfig);

	return ret;
}

/* init AVStream from MuxMediaDesriber */
void cmnMediaInitAvStream(void *avStream, MuxMediaDescriber *desc)
{
	AVCodecParameters *parms = desc->codecPar;
	AVStream *st = (AVStream *)avStream;
	st->index = desc->index;
	st->id = desc->id;

	avcodec_parameters_copy(st->codecpar, parms);
	
	st->time_base.num = desc->timeBaseNumerator;
	st->time_base.den = desc->timeBaseDenominator;
	
	st->sample_aspect_ratio.num = desc->sampleAspectRatioNumerator;
	st->sample_aspect_ratio.den= desc->sampleAspectRatioDenominator;

	st->codecpar->sample_aspect_ratio.num = desc->sampleAspectRatioNumerator;
	st->codecpar->sample_aspect_ratio.den= desc->sampleAspectRatioDenominator;
	
	st->pts_wrap_bits = desc->pts_wrap_bits;
	st->recommended_encoder_configuration = desc->recommended_encoder_configuration;

}


static int	_muxMediaGetBandwidth(AVStream *avStream)
{
	int bandwidth = 0;
	
	bandwidth += avStream->codecpar->bit_rate;
	if(bandwidth == 0)
	{
		AVDictionaryEntry *tag = NULL;
		AVDictionary *metaData = avStream->metadata;
		
		tag = av_dict_get(metaData, "BPS", NULL, AV_DICT_IGNORE_SUFFIX);
		if(tag )
		{
			bandwidth = atoi(tag->value);
		}
	}

	return bandwidth;
}


/* create MuMediaDescirber from AVStream. Only used by SERVER */
MuxMediaDescriber *cmnMediaCreateDescriberFromStream(void *st )
{
	MuxMediaDescriber *desc;
	AVCodecParameters *parms;
	
	AVStream *avStream = (AVStream *)st;

	desc = av_mallocz(sizeof(*desc));
	if (!desc)
	{
		MUX_ERROR("No memory for MuxMediaDescriber");
		exit(1);
		return NULL;
	}

	/* following data is not for creating AvStream */
	snprintf(desc->type, sizeof(desc->type), "%s", av_get_media_type_string(avStream->codecpar->codec_type));
	snprintf(desc->codec, sizeof(desc->codec), "%s", avcodec_get_name(avStream->codecpar->codec_id) );
	desc->bandwidth = _muxMediaGetBandwidth( avStream);
	
	//	AVRational *framerate = &avStream->avg_frame_rate;
	AVRational *framerate = &avStream->r_frame_rate;
	int fps = framerate->den && framerate->num;
	if(fps && avStream->codecpar->codec_id != AV_CODEC_ID_MJPEG )
	{
		desc->fps = (double)framerate->num/(double)framerate->den;
	}

	/* following data is used for initializing AvStream */	
	//NOTE we previously allocated internal & internal->avctx, these seemed uneeded though
	parms = avcodec_parameters_alloc();
	avcodec_parameters_copy(parms, avStream->codecpar);

		
	desc->codecPar = parms;
	desc->index = avStream->index;
	desc->pts_wrap_bits = 33;

	desc->timeBaseNumerator = avStream->time_base.num;
	desc->timeBaseDenominator = avStream->time_base.den;
	/* codecpar->sample_aspect_ratio has been copied in codecpar. ??? */
	desc->sampleAspectRatioNumerator = avStream->codecpar->sample_aspect_ratio.num;
	desc->sampleAspectRatioDenominator = avStream->codecpar->sample_aspect_ratio.den;

	desc->recommended_encoder_configuration = NULL;		/* */
	
	return desc;
}

#if 0
static MuxMediaPacket *cmnMediaPacketCreate(MUX_MEDIA_TYPE type, unsigned char *data, unsigned int size, int64_t pts, int64_t dts )
{
	MuxMediaPacket  *mediaPacket;
	unsigned char *_data;

	if(type >= MUX_MEDIA_TYPE_SUBTITLE)
	{
		CMN_ABORT("Media stream index is out of range : %d", type);
		return NULL;
	}
	
	mediaPacket = cmn_malloc(sizeof(MuxMediaPacket) );
	_data = cmn_malloc(size);
	if(mediaPacket == NULL || _data == NULL)
	{
		CMN_ABORT("No memory for MediaPacket");
		return NULL;
	}

	mediaPacket->streamIndex = type;
	mediaPacket->data = _data;
	memcpy(mediaPacket->data, data, size);
	mediaPacket->size = size;

	mediaPacket->pts = pts;
#if 0
	mediaPacket->dts = dts;
#else
	mediaPacket->dts = AV_NOPTS_VALUE;
#endif
	
	return mediaPacket;
}


static void cmnMediaPacketFree(MuxMediaPacket *mediaPacket)
{
	cmn_free(mediaPacket->data);
	cmn_free(mediaPacket);
}
#endif

/* 
* return: sucess: > 0, size of data; FIFO full: 0; error : < 0;
* 2 type of consumers: recorder: NO fifo full; server: all cases maybe happens
*/
int cmnMediaCaptureWrite(MuxMediaCapture *muxCapture, MUX_MEDIA_TYPE type, unsigned char *data, unsigned int size, int64_t pts, int64_t dts)
{
#define	WITHOUT_COPY		1
	MuxMediaConsumer *mediaConsumer = muxCapture->mediaConsumers;
	int ret, retLength;
//	int count =0;
#if WITHOUT_COPY
	MuxMediaPacket mediaPacket;
#else
	MuxMediaPacket *mediaPacket;
#endif

	ret = size;
	if(!mediaConsumer)
		return EXIT_FAILURE;

	if(type >= MUX_MEDIA_TYPE_SUBTITLE)
	{
		return size;
	}
	
	/* every media packet for every consumer */
#if WITHOUT_COPY
	mediaPacket.data = data;
	mediaPacket.size = size;
	mediaPacket.pts = pts;
	mediaPacket.dts = dts;
	mediaPacket.streamIndex = type;
#else
	mediaPacket = cmnMediaPacketCreate(type, data, size, (int64_t)pts, (int64_t)dts);
#endif

#if	MUX_OPTIONS_DEBUG_CAPTURE
	if(0)// !strcasecmp( muxCapture->name, "RX769Capture" ))
	{
		MUX_DEBUG("Cature %s: write %s, size:%d, pts:%lld, dts:%lld (type:%d)", muxCapture->name,(type==MUX_MEDIA_TYPE_VIDEO)?"Video":"Audio", size, pts , dts, type);
	}
#endif
	while(mediaConsumer)
	{
#if WITHOUT_COPY
		retLength = mediaConsumer->receiveVideo(mediaConsumer, type, &mediaPacket);
#else
		retLength = mediaConsumer->receiveVideo(mediaConsumer, type, mediaPacket);
#endif
		if (retLength< 0)
		{
			MUX_ERROR("write captured data to consumer '%s' failed on stream index %d: ret:%d", mediaConsumer->name, type, ret );
			ret = -EXIT_FAILURE;
		}
		else if(retLength == 0)
		{
			MUX_ERROR("write captured data to consumer '%s', but FIFO is full on stream index %d", mediaConsumer->name, type);
			ret = 0;
		}
		
		muxCapture->capturedMedias[type]->count ++;
		
#if	MUX_SVR_DEBUG_DATA_FLOW
		MUX_DEBUG("No.%d consumer: Stream of %d on Consumer '%s' received %d", ++count, type, mediaConsumer->name, retLength);
#endif
		mediaConsumer = mediaConsumer->next;
	}	

#if WITHOUT_COPY
#else
	cmnMediaPacketFree( mediaPacket);
#endif

	return ret;
}

int cmnMediaCapturerSendNotify(MuxMediaCapture *muxCapture, MUX_CAPTURE_EVENT event, void *data)
{
	int ret = 0;
	MuxMediaConsumer *mediaConsumer = muxCapture->mediaConsumers;
	
	while(mediaConsumer)
	{
		ret += mediaConsumer->receiveNotify(mediaConsumer, event, data);
		if (ret < 0)
		{
			MUX_ERROR("Send event '%d' to consumer '%s' failed: ret:%d", event, mediaConsumer->name, ret );
			ret = -EXIT_FAILURE;
		}
		else if(ret == 0)
		{
			MUX_WARN("Captured event '%d' to consumer '%s' is not used", event, mediaConsumer->name );
			ret = 0;
		}
		
#if	MUX_SVR_DEBUG_DATA_FLOW
		MUX_DEBUG("Consumer '%s' received event '%d'", mediaConsumer->name, event);
#endif
		mediaConsumer = mediaConsumer->next;
	}	

	return ret;
}


