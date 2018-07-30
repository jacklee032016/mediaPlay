/*
* $Id$
* File feed and its thread
*/

#include "muxSvr.h"

/* map stream in media file into stream into Feed */
int	muxSvrFeedFileLoadStream(MuxFeed *feed, AVStream *avStream)
{
	int ignore = TRUE;
	MuxMediaDescriber *desc = NULL;
	
	switch(avStream->codecpar->codec_type)
	{
		case AVMEDIA_TYPE_VIDEO:
			switch (avStream->codecpar->codec_id)
			{
				case AV_CODEC_ID_H264:
				case AV_CODEC_ID_HEVC:
					ignore = FALSE;
					break;
				default:
					break;
			}
			break;
			
		case AVMEDIA_TYPE_AUDIO:
			switch (avStream->codecpar->codec_id)
			{
				case AV_CODEC_ID_AAC:
				case AV_CODEC_ID_MP3:	
					ignore = FALSE;
					break;
				default:
					break;
			}
			break;
		
		case AVMEDIA_TYPE_SUBTITLE:
			switch (avStream->codecpar->codec_id)
			{
				case AV_CODEC_ID_TEXT:
					ignore = FALSE;
					break;
				default:
					break;
			}
			break;
			
		case AVMEDIA_TYPE_DATA:
		case AVMEDIA_TYPE_ATTACHMENT:
			break;
			
		default:
			abort();
			break;
	}

	if(!ignore)
	{
VTRACE();
		desc = muxMediaCreateDescriber(avStream);
		feed->streamIndexes[feed->nbInStreams] = avStream->index;
		feed->feedStreams[feed->nbInStreams++] = desc;
	}
	
	MUX_SVR_DEBUG("Stream %d (%d) (%s/%s) %s is %s", avStream->index, feed->nbInStreams, av_get_media_type_string(avStream->codecpar->codec_type), 
		avcodec_get_name(avStream->codecpar->codec_id), feed->feed_filename, (ignore==FALSE)?"added":"ignored");

	return !(ignore);
}


int	muxSvrFeedInitFile(MuxFeed *feed)
{
	char buf[128];
	AVFormatContext *fmtCtx = NULL;
	int buf_size, i;
	int64_t stream_pos;
	int ret;
	
	buf_size = 0;
	/* compute position (relative time) */
	if (av_find_info_tag(buf, sizeof(buf), "date", "" /*info*/)) /* can be enhanced to parse a string which tag the start time */
	{
		if ((ret = av_parse_time(&stream_pos, buf, TRUE)) < 0)
		{
			MUX_SVR_LOG("Invalid date specification '%s' for stream\n", buf);
			stream_pos = 0;
		}
	}
	else
		stream_pos = 0;
	
	if ( IS_STRING_NULL(feed->feed_filename) )
	{
		MUX_SVR_LOG("No filename was specified for Feed\n");
		return -EXIT_FAILURE;
	}

	/* open stream */
	ret = avformat_open_input(&fmtCtx, feed->feed_filename, feed->inFmt, &feed->inOpts);
	if (ret < 0)
	{
		MUX_SVR_LOG("Could not open input '%s': %s\n", feed->feed_filename, av_err2str(ret));
		return -EXIT_FAILURE;
	}

	/* set buffer size */
	if (buf_size > 0)
	{
		ret = ffio_set_buf_size(fmtCtx->pb, buf_size);
		if (ret < 0)
		{
			MUX_SVR_LOG("Failed to set buffer size as %d\n", buf_size);
			avformat_close_input(&fmtCtx);
			return -EXIT_FAILURE;
		}
	}

	fmtCtx->flags |= AVFMT_FLAG_GENPTS;
	feed->inputCtx = fmtCtx;
	 ret = avformat_find_stream_info(feed->inputCtx, NULL);
	if ( ret < 0)
	{
		MUX_SVR_LOG("Could not find stream info for input '%s'\n", feed->feed_filename);
		avformat_close_input(&fmtCtx);
		return -EXIT_FAILURE;
	}

	av_dump_format(feed->inputCtx, 0,  feed->feed_filename, 0);

	for(i=0; i < feed->inputCtx->nb_streams; i++)
	{
		MUX_SVR_DEBUG("No. %d Stream and codec in Feed '%s'", i, feed->feed_filename);
		muxSvrFeedFileLoadStream(feed, feed->inputCtx->streams[i] );
	}

	/* choose stream as clock source (we favor the video stream if present) for packet sending */
	feed->pts_stream_index = 0;
	
	for(i=0; i< feed->nbInStreams; i++)
	{
		AVCodecParameters *codecPar = (AVCodecParameters *)feed->feedStreams[i]->codecPar;
VTRACE();
		if (feed->pts_stream_index == 0 && codecPar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			feed->pts_stream_index = i;
		}
	}

VTRACE();
	if (feed->inputCtx->iformat->read_seek)
		av_seek_frame(feed->inputCtx, -1, stream_pos, 0);
	
	/* set the start time (needed for maxtime and RTP packet timing) */
	feed->start_time = feed->muxSvr->currentTime;
	feed->first_pts = AV_NOPTS_VALUE;

	return EXIT_SUCCESS;
}

static int	_initAllFeeds(CMN_THREAD_INFO *th, void *data)
{
//	MUX_SVR *muxSvr = (MUX_SVR *)data;
	
	th->data = data;

	return EXIT_SUCCESS;
}


/* rewind one feed of media file  */
static int  _rewindOneFeed(MuxFeed *feed)
{
	SERVER_CONNECT *svrConn;
	int ret;
	int i = 0;

#if MUX_SVR_DEBUG_DATA_FLOW
	MUX_SVR_DEBUG("Read to the end of feed '%s', reseek from beginning", feed->filename );
#endif
	if (feed->inputCtx->iformat->read_seek)
	{
#if 1			
		av_seek_frame(feed->inputCtx, -1, 0, 0);
#else
		/* sometimes, stream 0 or 1 maybe subtitle or data stream. so rewind this FormatInout, not its streams  */
		feed->inputCtx->iformat->read_seek(feed->inputCtx, 0, 0, SEEK_SET);
		feed->inputCtx->iformat->read_seek(feed->inputCtx, 1, 0, SEEK_SET);
#endif

		cmn_mutex_lock( (feed)->connsMutex);
		for(svrConn = feed->svrConns; svrConn != NULL; svrConn = svrConn->next)
		{
#if 0
			/* update cur_dts of all streams in this FormatCtx to 0. Client works fine with this method */
			ff_update_cur_dts(svrConn->outFmtCtx, svrConn->outFmtCtx->streams[0], 0);
#else
#if MUX_SVR_DEBUG_TIME_STAMP
			MUX_SVR_DEBUG("set last DTS of '%s'", svrConn->name );
#endif
			/* number of streams in Feed is not equal to number of streams for every SVR_CONN
			* for example, only 2 streams in TS CONN, more than 2 streams in MKV CONN when more than 2 streams in Feed
			*/
			for(i=0; i< feed->nbInStreams; i++)
			{
				AVStream *st;
				if( IS_RTSP_CONNECT(svrConn) )
				{
					st = svrConn->dataConns[i]->outFmtCtx->streams[0];
				}
				else
				{
					st = svrConn->outFmtCtx->streams[i];
				}

				if(st == NULL)
				{
					MUX_SVR_LOG(ERROR_TEXT_BEGIN"No.%d AVStream of '%s' is null"ERROR_TEXT_END, i, svrConn->name);
					abort();
					cmn_mutex_unlock( (feed)->connsMutex);
					return EXIT_FAILURE;
				}

#if MUX_SVR_DEBUG_TIME_STAMP
				MUX_SVR_DEBUG("set last DTS of stream %d in '%s' as %s", i, svrConn->name, av_ts2str(st->cur_dts) );
#endif						
				svrConn->lastDTSes[i] = st->cur_dts;
			}
#endif
		}
		cmn_mutex_unlock( (feed)->connsMutex);
	}
	
//	av_packet_unref(&pkt);	/* free data in this packet */
	
	return EXIT_SUCCESS;
	
}


static int _loopAllFeeds(CMN_THREAD_INFO *th)
{
	MUX_SVR 	*muxSvr = muxSvr = (MUX_SVR *)th->data;
	MuxFeed *feed = muxSvr->feeds;
	AVPacket 	pkt;
	int ret;


/*
	if( SYSTEM_IS_EXITING() || ( feed->abort_request == TRUE))
	{
		MUX_SVR_LOG( "Task %s recv EXIT signal\n", th->name );
		return -EXIT_FAILURE;
	}
*/
	while(feed)
	{
		if(feed->type == MUX_FEED_TYPE_FILE)
		{
			feed->lastTime = av_gettime();
			
			av_init_packet(&pkt);
			
#if 0
	pkt = av_packet_alloc();
	if(!pkt)
	{
		MUX_SVR_LOG("AVPacket is null on feed '%s'", feed->filename );
		return EXIT_SUCCESS;
	}
#endif	
			
			/* every time, the data of this packet for last time will invalidate  */
			if ( (ret=av_read_frame(feed->inputCtx, &pkt)) < 0)
			{
		//		file_table[grabInfo->inStream->fileIndex].eof_reached = 1;
				if(ret == AVERROR_EOF )
				{
					ret = _rewindOneFeed(feed);
				}
				else
				{
					MUX_SVR_LOG(ERROR_TEXT_BEGIN"Read feed '%s', failed : %s"ERROR_TEXT_END, feed->filename, av_err2str(ret));
					exit(1);
				}
			}

			if(pkt.stream_index < feed->nbInStreams)
			{
		 		muxSvrFeedSendPacket(feed, &pkt);
			}
			av_packet_unref(&pkt);	/* free data in this packet */

		}
		feed = feed->next;
	}

//	cmn_usleep(12*1000);
	cmn_usleep(muxSvr->svrConfig.feedDelay*1000);
	
	return EXIT_SUCCESS;
}

CMN_THREAD_INFO  threadFileFeed =
{
	name		:	"FileFeed",
	init			:	_initAllFeeds,
	mainLoop		:	_loopAllFeeds,
	eventHandler	:	NULL,
	destory		:	NULL,
	data			:	NULL,
};

