/*
* $Id$
* Feed management, common for File Feed and Capture Feed
*/

#include "muxSvr.h"

/* one interface of Feed */
static LayeredAVStream *__saveAvCodec(AVStream *avStream )
{
	LayeredAVStream *fst;

	fst = av_mallocz(sizeof(*fst));
	if (!fst)
	{
		MUX_SVR_LOG("No memory for LayeredAVStream");
		exit(1);
		return NULL;
	}
	
#if MUX_WITH_CODEC_FIELD	
	fst->codec = avcodec_alloc_context3(avStream->codec->codec);
	if (!fst->codec)
	{
		av_free(fst);
		return NULL;
	}
	avcodec_copy_context(fst->codec, avStream->codec);
#endif

	//NOTE we previously allocated internal & internal->avctx, these seemed uneeded though
	fst->codecpar = avcodec_parameters_alloc();
	avcodec_parameters_copy(fst->codecpar, avStream->codecpar);
	
	fst->index = avStream->index;
	fst->pts_wrap_bits = 33;
#if MUX_WITH_CODEC_FIELD	
	fst->time_base = avStream->codec->time_base;
	fst->sample_aspect_ratio = avStream->codec->sample_aspect_ratio;
#else
	fst->time_base = avStream->time_base;
	fst->sample_aspect_ratio = avStream->codecpar->sample_aspect_ratio;
#endif	
	/* only codec->bit_rate has value which is not 0 */
//	MUX_SVR_DEBUG("Bitrate of codec:%d; of codecpar:%d", avStream->codec->bit_rate, avStream->codecpar->bit_rate);

	return fst;
}


int	muxMediaGetBandwidth(AVStream *avStream)
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

MuxMediaDescriber *muxMediaCreateDescriber(AVStream *avStream )
{
	MuxMediaDescriber *desc;
	AVCodecParameters *parms;

	desc = av_mallocz(sizeof(*desc));
	if (!desc)
	{
		MUX_SVR_LOG("No memory for MuxMediaDescriber");
		exit(1);
		return NULL;
	}

	/* following data is not for creating AvStream */
	snprintf(desc->type, sizeof(desc->type), "%s", av_get_media_type_string(avStream->codecpar->codec_type));
	snprintf(desc->codec, sizeof(desc->codec), "%s", avcodec_get_name(avStream->codecpar->codec_id) );
	desc->bandwidth = muxMediaGetBandwidth( avStream);
	
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



void muxMediaInitAvStream(AVStream *st, MuxMediaDescriber *desc)
{
	AVCodecParameters *parms = desc->codecPar;
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


int	muxSvrFeedSendPacket(MuxFeed *feed, AVPacket *pkt)
{
	SERVER_CONNECT *svrConn;
	int ret;
	int count = 0;
	
//	MUX_SVR_DEBUG("AVPacket buf is %ld", pkt.buf );
//	ret = av_packet_ref(&pkt, &pkt);
//	MUX_SVR_DEBUG("After refer to itself, AVPacket buf is %ld", pkt.buf );

	cmn_mutex_lock( (feed)->connsMutex);
	for(svrConn = feed->svrConns; svrConn != NULL; svrConn = svrConn->next)
	{
		
		AVPacket *copy = av_mallocz(sizeof(AVPacket));

//		MUX_SVR_DEBUG("No. %d CONN is '%s'", i++, svrConn->name );
		if(!copy)
		{
			MUX_SVR_LOG(ERROR_TEXT_BEGIN"'%s' Can clone AVPacket"ERROR_TEXT_END, svrConn->name );
			continue;
		}
		
		/* copy data to new packet */
		ret = av_packet_ref(copy, pkt);
		if(ret < 0)
		{
			MUX_SVR_LOG(ERROR_TEXT_BEGIN"'%s' Can ref AVPacket : %s"ERROR_TEXT_END, svrConn->name, av_err2str(ret));
			av_freep(&copy);
			continue;
		}

#if 0		
		if(svrConn->lastPkt)
		{
			cmn_fifo_add(svrConn->fifo, svrConn->lastPkt);
			svrConn->lastPkt = NULL;
		}
		if(copy->data == pkt.data )//copy->buf)
		{
//			MUX_SVR_DEBUG("'%s' AVPacket refered", svrConn->name );
		}
		else
		{
//			MUX_SVR_DEBUG("'%s' AVPacket independent", svrConn->name );
		}
#endif

		
		svrConn->packetCountTotal ++;

		/* enhancement: only drop non-key video frame, not key video frame or audio frame */
		if(cmn_fifo_add(svrConn->fifo, copy) < 0)
		{
			svrConn->packetCountDrop ++;
			MUX_SVR_DEBUG("'%s' : FIFO is full, drop this packet with %d bytes, total %d packets droped in period of %ld ms", 
				svrConn->name, copy->size, svrConn->packetCountDrop, CONN_SERVICE_TIME(svrConn)/1000);
#if MUX_SVR_DEBUG_DATA_FLOW
#endif
//			svrConn->lastPkt = copy;
			av_packet_free(&copy);
#if 0
			cmn_mutex_unlock( (feed)->connsMutex);
			return EXIT_SUCCESS;
#else
			continue;
#endif
		}

		count++;
		
	}
	cmn_mutex_unlock( (feed)->connsMutex);

	return count;
	
}

int	muxSvrFeedInitCapture(MuxFeed *feed)
{

	return -EXIT_FAILURE;
}

int	muxSvrFeedInit(int index, void *ele, void *priv)
{
	MuxFeed *feed = (MuxFeed *)ele;
	char buf[128];
	AVFormatContext *fmtCtx = NULL;
	int buf_size, i;
	int64_t stream_pos;
	int ret = EXIT_SUCCESS;

	switch(feed->type)
	{
		case MUX_FEED_TYPE_FILE:
			ret = muxSvrFeedInitFile(feed);
			break;
		case MUX_FEED_TYPE_CAPTURE:
			ret = muxSvrFeedInitCapture(feed);
			break;
		case MUX_FEED_TYPE_TX_CAPTURE:
			ret = -EXIT_FAILURE;
			break;
		case MUX_FEED_TYPE_UNKNOWN:
		default:
			return -EXIT_FAILURE;
			break;
	}

	if(ret == EXIT_SUCCESS)
	{
		feed->connsMutex = cmn_mutex_init();
		ADD_ELEMENT(feed->muxSvr->feeds, feed);
VTRACE();
		for(i=0; i< feed->nbInStreams; i++)
		{
			feed->bandwidth += feed->feedStreams[i]->bandwidth;
		}
		feed->bandwidth = MUX_KILO(feed->bandwidth);
		
		MUX_SVR_LOG("No.%d Feed %s(%s): Streams :%d, bandwidth: %d Kbps", index, feed->filename, feed->feed_filename, feed->nbInStreams, feed->bandwidth);
	}

	return ret;
}

