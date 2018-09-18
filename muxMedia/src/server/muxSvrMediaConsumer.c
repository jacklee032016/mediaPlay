/*
* $Id$
* Feed management, common for File Feed and Capture Feed
*/

#include "muxSvr.h"

#include "__services.h"

int	muxSvrFeedSendPacket(MuxFeed *feed, AVPacket *pkt)
{
	SERVER_CONNECT *svrConn;
	int ret;
	int count = 0;
	int total = 0;
	
//	MUX_SVR_DEBUG("AVPacket buf is %ld", pkt.buf );
//	ret = av_packet_ref(&pkt, &pkt);
//	MUX_SVR_DEBUG("After refer to itself, AVPacket buf is %ld", pkt.buf );

	cmn_mutex_lock( (feed)->connsMutex);

	for(svrConn = feed->svrConns; svrConn != NULL; svrConn = svrConn->next)
	{
		AVPacket *copy = av_packet_alloc();

#if	MUX_SVR_DEBUG_DATA_FLOW
		MUX_SVR_DEBUG( "No. %d CONN of '%s' is copying AvPacket", i++, svrConn->name );
#endif
		count++;

		if(!copy)
		{
			MUX_SVR_ERROR( "'%s' Can clone AVPacket", svrConn->name );
			CMN_ABORT("");
			continue;
		}
		
		/* every connection just get the same data buffer with different AVPacket */
		ret = av_packet_ref(copy, pkt);
		if(ret < 0)
		{
			MUX_SVR_ERROR( "'%s' Can ref AVPacket : %s", svrConn->name, av_err2str(ret));
			av_packet_free(&copy);
			CMN_ABORT("");
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
		else
		{
			total ++;
		}

		
	}
	cmn_mutex_unlock( (feed)->connsMutex);

	/* count > total, FIFO is full for some connection */
	return (count > total)? 0:(count==0)?1:count;
	
}


/* 
* return: sucess: > 0, size of data; FIFO full: 0; error : < 0;
* Multiple server connections shared the same data by AvPacket
*/
static int _muxSvrConsumerReceive(MuxMediaConsumer *mediaConsumer, MUX_MEDIA_TYPE type, MuxMediaPacket *mediaPacket)
{
	AVPacket pkt;
	MuxFeed *feed = (MuxFeed *)mediaConsumer->priv;

#define	WITH_COPY		1
	unsigned char *data;
	int ret;
	
	if(!feed)
	{
		CMN_ABORT("No feed is found for this media consumer of '%s'", mediaConsumer->name );
	}

	av_init_packet(&pkt);
#if WITH_COPY	
	data = av_malloc(mediaPacket->size);
	if(!data)
	{
		CMN_ABORT("No memory for data of media packet of Svr Media Consumer");
		return -EXIT_FAILURE;
	}
	memcpy(data, mediaPacket->data, mediaPacket->size);
#else
	data = mediaPacket->data;
#endif

	ret = av_packet_from_data(&pkt, data, mediaPacket->size);
	if(ret)
	{
		MUX_SVR_ERROR( "Error in init AvPaket from data: %s", av_err2str(ret));
		return -EXIT_FAILURE;
	}
	pkt.stream_index = mediaPacket->streamIndex;
	pkt.pts = mediaPacket->pts;
	pkt.dts = mediaPacket->dts;
	
#if	MUX_SVR_DEBUG_DATA_FLOW
	MUX_SVR_DEBUG("Feed '%s' receive packet ", feed->filename);
#endif	
	ret = muxSvrFeedSendPacket(feed, &pkt);
	av_packet_unref(&pkt);
	if(ret == 0)
	{/* Some FIFO is full */
		return 0;
	}

	return ret*mediaPacket->size;
}



/* 
* handle capturing events for existing connection (HTTP/RTSP/RTMP/UDP/HLS)
*/
static int _muxSvrReceiveCaptureNotify(MuxMediaConsumer *mediaConsumer, MUX_CAPTURE_EVENT event, void *data)
{
	MuxFeed *feed = (MuxFeed *)mediaConsumer->priv;
	SERVER_CONNECT *svrConn;
	int ret = EXIT_SUCCESS;
	int i =0;

	if(!feed)
	{
		CMN_ABORT("Invalidate Feed");
		return 0;
	}
	
	cmn_mutex_lock( (feed)->connsMutex);

	for(svrConn = feed->svrConns; svrConn != NULL; svrConn = svrConn->next)
	{

#if	MUX_SVR_DEBUG_DATA_FLOW
		MUX_SVR_DEBUG( "No. %d CONN of '%s' receiving event '%d'", i++, svrConn->name ,event );
#endif
		if(event == MUX_CAPTURE_EVENT_STOP)
		{
			MUX_SVR_DEBUG( "No. %d CONN of '%s' receiving event STOP", i++, svrConn->name);
		}
		else if(event == MUX_CAPTURE_EVENT_PAUSE)
		{
			MUX_SVR_DEBUG( "No. %d CONN of '%s' receiving event PAUSE", i++, svrConn->name);
		}
	}

//failed:
	cmn_mutex_unlock( (feed)->connsMutex);

	return ret;
}

int	muxSvrCreateConsumer4Feed(MuxFeed *feed, MUX_SVR *muxSvr)
{
	MuxMediaConsumer *mediaConsumer = NULL;
	MuxMain *muxMain = (MuxMain *)muxSvr->priv;
	int res;
	
	mediaConsumer = (MuxMediaConsumer *)cmn_malloc(sizeof(MuxMediaConsumer));
	if(!mediaConsumer)
	{
		CMN_ABORT("No memory for Media Consumer of Feed");
	}

	snprintf(mediaConsumer->name, sizeof(mediaConsumer->name), "SvrConsumer-%s", (feed->type== MUX_FEED_TYPE_FILE)?feed->filename:"Captuering"/*"SvrCaptureFeedConsumer"*/);
	mediaConsumer->receiveVideo = _muxSvrConsumerReceive;

	mediaConsumer->receiveNotify = _muxSvrReceiveCaptureNotify;
	
	res = muxMain->registerConsumer( muxMain, mediaConsumer, feed->captureName);
	if(res != EXIT_SUCCESS)
	{
		CMN_ABORT("Media consumer '%s' registering into capturer '%s' failed", mediaConsumer->name, feed->captureName);
		return res;
	}
	
	mediaConsumer->priv = feed;
	feed->mediaConsumer = mediaConsumer;
	feed->mediaCapture = mediaConsumer->mediaCapture;
	
	return EXIT_SUCCESS;
}


