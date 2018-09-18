/*
* $Id$
* File feed and its thread
*/

#include "muxSvr.h"

#include "__services.h"

#define	FILE_FEED_OUT_SYNC_THRESHOLD		10
#define	FILE_FEED_DELAY_COUNT				1


static int	_initFileFeedsThread(CmnThread *th, void *data)
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
	MUX_SVR_DEBUG("Read to the end of feed '%s(%s)', reseek from beginning", feed->filename, feed->feed_filename );
#endif

	if (feed->inputCtx->iformat->read_seek)
	{
#if 1			
		ret = av_seek_frame(feed->inputCtx, -1, 0, 0);
		if(ret)
		{
			MUX_SVR_ERROR("seek_frame failed: %s", av_err2str(ret));
			return EXIT_FAILURE;
		}
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
//			for(i=0; i< feed->nbInStreams; i++)
			for(i=0; i< feed->mediaCapture->nbStreams; i++)
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
					CMN_ABORT( "No.%d AVStream of '%s' is null", i, svrConn->name);
					cmn_mutex_unlock( (feed)->connsMutex);
					return EXIT_FAILURE;
				}

#if MUX_SVR_DEBUG_TIME_STAMP
				MUX_SVR_DEBUG("set last DTS of stream of %d in '%s' as %s", i, svrConn->name, av_ts2str(st->cur_dts) );
#endif						
				svrConn->lastDTSes[i] = st->cur_dts;
			}
#endif
		}
		cmn_mutex_unlock( (feed)->connsMutex);
	}
	else
	{
		CMN_ABORT("File feed '%s' of format of '%s' not support read_seek",feed->feed_filename, feed->inputCtx->iformat->name );
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
	
}

/* return : <=0, no delay; > 0, delay in ms */
static int _muxSvrFeedCalculateDelay(MuxFeed *feed, int streamIndex)
{
	double avClock, sysClock;
	double delayPerFrame = 0.0;
	int delay = 0; /* ms */
	MuxMediaCapture *mediaCapture = feed->mediaCapture;
	

	if(streamIndex == mediaCapture->videoStreamIndex)
	{
		mediaCapture->clockVideo = (++mediaCapture->videoPacketCount)*mediaCapture->durationVideoFrame;
	}
	else if(streamIndex == mediaCapture->audioStreamIndex)
	{
		mediaCapture->clockAudio = (++mediaCapture->audioPacketCount)*mediaCapture->durationAudioFrame;
	}
	else
	{
		CMN_ABORT("Packet stream index is out of range in feed '%s': streamIndex=%d(videoIndex=%d, audioIndex=%d)", 
			feed->feed_filename, streamIndex, mediaCapture->videoStreamIndex, mediaCapture->audioStreamIndex);
		return -EXIT_FAILURE;
	}

	if(mediaCapture->clockAudio<= mediaCapture->clockVideo)
	{
		avClock = mediaCapture->clockAudio;
		delayPerFrame = mediaCapture->durationAudioFrame;
	}
	else
	{
		avClock = mediaCapture->clockVideo;
		delayPerFrame = mediaCapture->durationVideoFrame;
	}
	
	sysClock = cmnGetTimeMs()/1000.0 - mediaCapture->startTime;

	if( (avClock - sysClock) > feed->outSyncThreshold*delayPerFrame)
	{
		delay = feed->delayCount *delayPerFrame*1000;
	}
	
	return delay;
}

static int _loopAllFileFeeds(CmnThread *th)
{
	MUX_SVR 	*muxSvr = muxSvr = (MUX_SVR *)th->data;
	MuxFeed *feed = muxSvr->feeds;
	AVPacket 	pkt;
	int ret = EXIT_FAILURE;
	int delay = 0; /* ms */

/*
	if( SYSTEM_IS_EXITING() || ( feed->abort_request == TRUE))
	{
		MUX_SVR_ERROR( "Task %s recv EXIT signal\n", th->name );
		return -EXIT_FAILURE;
	}
*/

#if MUX_SVR_DEBUG_DATA_FLOW
	TRACE();
#endif
	while(feed)
	{
		if(feed->type == MUX_FEED_TYPE_FILE)
		{
#if MUX_SVR_DEBUG_DATA_FLOW
	TRACE();
#endif
retry:
			feed->lastTime = av_gettime();
			
#if MUX_SVR_DEBUG_DATA_FLOW
	TRACE();
#endif
//			av_init_packet(&pkt);
			
			/* every time, the data of this packet for last time will invalidate  */
			if ( (ret=av_read_frame(feed->inputCtx, &pkt)) < 0)
			{
		//		file_table[grabInfo->inStream->fileIndex].eof_reached = 1;
				if(ret == AVERROR_EOF )
				{
#if MUX_SVR_DEBUG_DATA_FLOW
	TRACE();
#endif
					ret = _rewindOneFeed(feed);
					if(ret == EXIT_SUCCESS)
					{
						MUX_SVR_ERROR("Rewind '%s(%s)' OK!retry...", feed->filename, feed->feed_filename);
						goto retry;
					}
					CMN_ABORT("Rewinding feed file '%s' failed", feed->filename);
				}
				else
				{
					MUX_SVR_ERROR( "Rewind feed file '%s', failed : %s", feed->filename, av_err2str(ret));
					exit(1);
				}
			}

//			if(pkt.stream_index < feed->nbInStreams)
			/* after capture is inited, it can be used */
			if( feed->mediaCapture && (pkt.stream_index < feed->mediaCapture->nbStreams) )
			{
				int delayOfFeed = 0;
				delayOfFeed = _muxSvrFeedCalculateDelay(feed, pkt.stream_index);
				delay = (delay< delayOfFeed)?delayOfFeed:delay;
				
#if MUX_SVR_DEBUG_DATA_FLOW
				MUX_SVR_DEBUG( "FileFeed send packet: size :%d; pts: %s, DTS: %s, stream %d", pkt.size, av_ts2str(pkt.pts), av_ts2str(pkt.dts), pkt.stream_index);
#endif

#if MUX_SVR_OPT_FILE_FEED_2_CAPTURE
				ret = cmnMediaCaptureWrite(feed->mediaCapture, pkt.stream_index, pkt.data, pkt.size, pkt.pts, pkt.dts);
#else
		 		muxSvrFeedSendPacket(feed, &pkt);
#endif
#if MUX_SVR_DEBUG_DATA_FLOW
				TRACE();
#endif
			}
			
#if MUX_SVR_DEBUG_DATA_FLOW
				TRACE();
#endif
			av_packet_unref(&pkt);	/* free data in this packet */

		}
		
		feed = feed->next;
	}

#if MUX_SVR_DEBUG_DATA_FLOW
	TRACE();
#endif

#if 0
	if( ret == 0)
	{/* FIFO is full */
		cmn_usleep(5*1000);
	}
	
	cmn_usleep(muxSvr->svrConfig.feedDelay*1000);
#else
	if(delay > 0)
	{
		cmn_usleep(delay*1000);
	}
#endif
	return EXIT_SUCCESS;
}

static CmnThread  _threadFileFeed =
{
	name		:	"FileFeed",
	init			:	_initFileFeedsThread,
	mainLoop		:	_loopAllFileFeeds,
	eventHandler	:	NULL,
	destory		:	NULL,
	data			:	NULL,
};


int	_muxSvrStartFileFeed(struct MuxMediaCapture *mediaCapture)
{
	MuxFeed	 *feed = (MuxFeed *)mediaCapture->priv;
	int	ret = EXIT_SUCCESS;
	
	if(feed->muxSvr->feedThreadId <= 0) //mediaCapture->nbConsumers )
	{
		MuxMain *muxMain = (MuxMain *)feed->muxSvr->priv;

		ret = muxMain->initThread(muxMain, &_threadFileFeed, feed->muxSvr);
		if(ret > 0)
		{
			feed->muxSvr->feedThreadId = ret;
			return EXIT_SUCCESS;
		}
		else
		{
			return EXIT_FAILURE;
		}
	}
	else
	{
		MUX_SVR_DEBUG( "Capture '%s' has been running", mediaCapture->name );
	}
	
	return  ret;
}

int	_muxSvrFileFeedGetStatus(struct MuxMediaCapture *mediaCapture)
{
	return  MUX_CAPTURE_STATE_RUNNING;
}



/* get count of frames in audio stream */
static int	_muxMediaGuessAudioFrames(AVStream *avStream)
{
	int frames = 0;
	
	AVDictionaryEntry *tag = NULL;
	AVDictionary *metaData = avStream->metadata;
	
	tag = av_dict_get(metaData, "NUMBER_OF_FRAMES", NULL, AV_DICT_IGNORE_SUFFIX);
	if(tag )
	{
		frames = atoi(tag->value);
	}
	
	if(frames == 0)
	{
		CMN_ABORT("Can't find frame count of audio stream");
	}
	MUX_SVR_DEBUG("Frame count of audio stream: %d", frames);
	return frames;
}



/* map stream in media file into stream into Feed */
int	muxSvrFeedFileLoadStream(MuxFeed *feed, AVStream *avStream, int index)
{
	int ignore = TRUE;
	MuxMediaDescriber *desc = NULL;
	AVRational framerate;
	int frames = 0;

	switch(avStream->codecpar->codec_type)
	{
		case AVMEDIA_TYPE_VIDEO:
			switch (avStream->codecpar->codec_id)
			{
				case AV_CODEC_ID_H264:
				case AV_CODEC_ID_HEVC:
					ignore = FALSE;
					framerate = av_guess_frame_rate(feed->inputCtx, avStream, NULL);
					if(framerate.num == 0)
					{
						CMN_ABORT("Can't get video Framerate from media file");
					}
					feed->mediaCapture->durationVideoFrame = 1.0/av_q2d(framerate);
					feed->mediaCapture->videoStreamIndex = index;

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
					frames = _muxMediaGuessAudioFrames(avStream);
					if(frames == 0)
					{
						CMN_ABORT("Can't get audio Frame count from media file");
					}
					
					feed->mediaCapture->durationAudioFrame = 1.0*feed->durationSecond/frames;
					feed->mediaCapture->audioStreamIndex = index;
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
			CMN_ABORT("Invalidate codec type : '%s'", avcodec_get_name(avStream->codecpar->codec_id));
			break;
	}

	if(!ignore)
	{
VTRACE();
		desc = cmnMediaCreateDescriberFromStream(avStream);
		snprintf(desc->name, sizeof(desc->name), "%s-%s.%s", feed->filename, desc->type, desc->codec);
		feed->mediaCapture->capturedMedias[feed->mediaCapture->nbStreams++] = desc;
#if 0		
		feed->streamIndexes[feed->nbInStreams] = avStream->index;
		feed->feedStreams[feed->nbInStreams++] = desc;
		MUX_SVR_DEBUG("Stream %d (%d) (%s/%s) %s is %s", avStream->index, feed->nbInStreams, av_get_media_type_string(avStream->codecpar->codec_type), 
			avcodec_get_name(avStream->codecpar->codec_id), feed->feed_filename, (ignore==FALSE)?"added":"ignored");
#endif		
		MUX_SVR_DEBUG("Stream %d (%d) (%s/%s) %s is %s", avStream->index, feed->mediaCapture->nbStreams, av_get_media_type_string(avStream->codecpar->codec_type), 
			avcodec_get_name(avStream->codecpar->codec_id), feed->feed_filename, (ignore==FALSE)?"added":"ignored");
	}
	

	return !(ignore);
}


int	muxSvrFeedInitFile(MuxFeed *feed, MUX_SVR *muxSvr)
{
	char buf[128];
	AVFormatContext *fmtCtx = NULL;
	int buf_size, i;
	int64_t stream_pos;
	int ret;
	MuxMediaCapture *mediaCapture = NULL;
	MuxMain *muxMain = (MuxMain *)muxSvr->priv;

	buf_size = 0;
	/* compute position (relative time) */
	if (av_find_info_tag(buf, sizeof(buf), "date", "" /*info*/)) /* can be enhanced to parse a string which tag the start time */
	{
		if ((ret = av_parse_time(&stream_pos, buf, TRUE)) < 0)
		{
			MUX_SVR_ERROR( "Invalid date specification '%s' for stream\n", buf);
			stream_pos = 0;
		}
	}
	else
		stream_pos = 0;
	
	if ( IS_STRING_NULL(feed->feed_filename) )
	{
		MUX_SVR_ERROR( "No filename was specified for Feed\n");
		return -EXIT_FAILURE;
	}

	/* open stream */
	ret = avformat_open_input(&fmtCtx, feed->feed_filename, feed->inFmt, &feed->inOpts);
	if (ret < 0)
	{
		MUX_SVR_ERROR( "Could not open input '%s': %s\n", feed->feed_filename, av_err2str(ret));
		return -EXIT_FAILURE;
	}

	/* set buffer size */
	if (buf_size > 0)
	{
		ret = ffio_set_buf_size(fmtCtx->pb, buf_size);
		if (ret < 0)
		{
			MUX_SVR_ERROR("Failed to set buffer size as %d\n", buf_size);
			avformat_close_input(&fmtCtx);
			return -EXIT_FAILURE;
		}
	}

	fmtCtx->flags |= AVFMT_FLAG_GENPTS;
	feed->inputCtx = fmtCtx;
	 ret = avformat_find_stream_info(feed->inputCtx, NULL);
	if ( ret < 0)
	{
		MUX_SVR_ERROR( "Could not find stream info for input '%s'\n", feed->feed_filename);
		avformat_close_input(&fmtCtx);
		return -EXIT_FAILURE;
	}

	av_dump_format(feed->inputCtx, 0,  feed->feed_filename, 0);

	mediaCapture = cmn_malloc(sizeof(MuxMediaCapture));
	if(!mediaCapture)
	{
		CMN_ABORT("No memory for Media Capturer");
	}

	snprintf(mediaCapture->name, sizeof(mediaCapture->name), "%s", feed->captureName);
	mediaCapture->priv = feed;
	mediaCapture->startCapture = _muxSvrStartFileFeed;
	mediaCapture->getCaptureState = _muxSvrFileFeedGetStatus;
	mediaCapture->createMediaDescribers = NULL;
	feed->mediaCapture = mediaCapture;

	if (fmtCtx->duration != AV_NOPTS_VALUE)
	{
            int64_t duration = fmtCtx->duration + (fmtCtx->duration <= INT64_MAX - 5000 ? 5000 : 0);
            feed->durationSecond  = (int)(duration / AV_TIME_BASE);
       }
	else
	{
		CMN_ABORT("Can't found duration of this media file: '%s'", feed->feed_filename);
	}
	MUX_SVR_DEBUG("Duration of media: %d (%lld)",  feed->durationSecond,  fmtCtx->duration );

	
	for(i=0; i < feed->inputCtx->nb_streams; i++)
	{
		MUX_SVR_DEBUG("No. %d Stream and codec in Feed '%s'", i, feed->feed_filename);
		muxSvrFeedFileLoadStream(feed, feed->inputCtx->streams[i], i);
	}

	/* choose stream as clock source (we favor the video stream if present) for packet sending */
	feed->pts_stream_index = 0;
	
	for(i=0; i< feed->mediaCapture->nbStreams; i++)
	{
//		AVCodecParameters *codecPar = (AVCodecParameters *)feed->feedStreams[i]->codecPar;
		AVCodecParameters *codecPar = (AVCodecParameters *)feed->mediaCapture->capturedMedias[i]->codecPar;
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

	feed->outSyncThreshold = FILE_FEED_OUT_SYNC_THRESHOLD;
	feed->delayCount = FILE_FEED_DELAY_COUNT;
	
	mediaCapture->videoCurrentPtsTime = cmnGetTimeMs();
	mediaCapture->videoCurrentPts = 0.0;

	mediaCapture->startTime = cmnGetTimeMs()/1000.0;

	MUX_SVR_DEBUG("frame duration audio :%6.4f, video:%6.4f ", mediaCapture->durationAudioFrame, mediaCapture->durationVideoFrame);

	ret = muxMain->addCapture(muxMain, feed->mediaCapture);
	
	return ret;
}

int	muxSvrFeedInit(int index, void *ele, void *priv)
{
	MuxFeed *feed = (MuxFeed *)ele;
	MUX_SVR *muxSvr = (MUX_SVR *)priv;
	
//	char buf[128];
//	AVFormatContext *fmtCtx = NULL;
	int i;
//	int64_t stream_pos;
	int ret = EXIT_SUCCESS;

	switch(feed->type)
	{
		case MUX_FEED_TYPE_FILE:
			ret = muxSvrFeedInitFile(feed, muxSvr);
			
#if MUX_SVR_OPT_FILE_FEED_2_CAPTURE
			ret = muxSvrCreateConsumer4Feed(feed, muxSvr);
#endif
			break;
		case MUX_FEED_TYPE_CAPTURE:
			ret = muxSvrCreateConsumer4Feed(feed, muxSvr);
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
		
		if(feed->type == MUX_FEED_TYPE_FILE)
		{/* add this list to be looped by FileFeed thread */
			ADD_ELEMENT(feed->muxSvr->feeds, feed);
		}
		
#if 1
		if(feed->mediaCapture)
		{
			for(i=0; i< feed->mediaCapture->nbStreams; i++)
			{
VTRACE();
				feed->bandwidth += feed->mediaCapture->capturedMedias[i]->bandwidth;
			}
#else		
			for(i=0; i< feed->nbInStreams; i++)
			{
				feed->bandwidth += feed->feedStreams[i]->bandwidth;
			}
#endif
			feed->bandwidth = MUX_KILO(feed->bandwidth);
			
			MUX_SVR_DEBUG( "No.%d Feed %s(%s): Streams :%d, bandwidth: %d Kbps", index, feed->filename, feed->feed_filename, feed->mediaCapture->nbStreams, feed->bandwidth);
		}
		else
		{
			MUX_SVR_ERROR( "ERROR: No.%d Feed %s(%s): media capture is not defined now", index, feed->filename, feed->captureName);
		}
	}

	return ret;
}


