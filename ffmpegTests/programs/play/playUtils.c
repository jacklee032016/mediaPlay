
#include "play.h"

void playDecoderInit(PlayDecoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond)
{
	memset(d, 0, sizeof(PlayDecoder));
	d->avctx = avctx;
	d->queue = queue;
	d->empty_queue_cond = empty_queue_cond;
	d->start_pts = AV_NOPTS_VALUE;
}


int playDecoderStart(PlayDecoder *d, int (*fn)(void *), void *arg)
{
	playPacketQueueStart(d->queue);
	d->decoder_tid = SDL_CreateThread(fn, "decoder", arg);
	if (!d->decoder_tid)
	{
		av_log(NULL, AV_LOG_ERROR, "SDL_CreateThread(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
	
	return 0;
}


int playDecoderDecodeFrame(PlayDecoder *d, AVFrame *frame, AVSubtitle *sub)
{
	int got_frame = 0;

	do
	{
		int ret = -1;

		if (d->queue->abort_request)
			return -1;

		if (!d->packet_pending || d->queue->serial != d->pkt_serial)
		{
			AVPacket pkt;

			do
			{
				if (d->queue->nb_packets == 0)
					SDL_CondSignal(d->empty_queue_cond);
				
				if (playPacketQueueGet(d->queue, &pkt, 1, &d->pkt_serial) < 0)
					return -1;
				
				if (pkt.data == flush_pkt.data)
				{/* this is flush_pkt, so continue to get next packet */
					avcodec_flush_buffers(d->avctx);
					d->finished = 0;
					d->next_pts = d->start_pts;
					d->next_pts_tb = d->start_pts_tb;
					
				}
			} while (pkt.data == flush_pkt.data || d->queue->serial != d->pkt_serial);
			
			av_packet_unref(&d->pkt);
			/* keep data of current packet */
			d->pkt_temp = d->pkt = pkt;
			d->packet_pending = 1;
		}

		switch (d->avctx->codec_type)
		{
			case AVMEDIA_TYPE_VIDEO:
				ret = avcodec_decode_video2(d->avctx, frame, &got_frame, &d->pkt_temp);
				if (got_frame)
				{
					if (decoder_reorder_pts == -1)
					{
						frame->pts = av_frame_get_best_effort_timestamp(frame);
					}
					else if (!decoder_reorder_pts)
					{
						frame->pts = frame->pkt_dts;
					}
				}
				break;
			
			case AVMEDIA_TYPE_AUDIO:
				ret = avcodec_decode_audio4(d->avctx, frame, &got_frame, &d->pkt_temp);
				if (got_frame)
				{
					AVRational tb = (AVRational){1, frame->sample_rate};
					if (frame->pts != AV_NOPTS_VALUE)
						frame->pts = av_rescale_q(frame->pts, av_codec_get_pkt_timebase(d->avctx), tb);
					else if (d->next_pts != AV_NOPTS_VALUE)
						frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
					
					if (frame->pts != AV_NOPTS_VALUE)
					{
						d->next_pts = frame->pts + frame->nb_samples;
						d->next_pts_tb = tb;
					}
				}
				break;
			
			case AVMEDIA_TYPE_SUBTITLE:
				ret = avcodec_decode_subtitle2(d->avctx, sub, &got_frame, &d->pkt_temp);
				break;
		}

		if (ret < 0)
		{/* decode failed */
			d->packet_pending = 0;
		}
		else
		{
			d->pkt_temp.dts =
			d->pkt_temp.pts = AV_NOPTS_VALUE;
			if (d->pkt_temp.data)
			{/* no error in decoder, and frame is returned from decoder */
				if (d->avctx->codec_type != AVMEDIA_TYPE_AUDIO)
					ret = d->pkt_temp.size;
				
				d->pkt_temp.data += ret;
				d->pkt_temp.size -= ret;
				if (d->pkt_temp.size <= 0)
					d->packet_pending = 0;
			}
			else
			{/* no error in decoder, but no frame returned from decoder this time. so return form this function */
				if (!got_frame)
				{
					d->packet_pending = 0;
					d->finished = d->pkt_serial;
				}
			}
		}
	} while (!got_frame && !d->finished);	/**/

	return got_frame;
}

void playDecoderDestroy(PlayDecoder *d)
{
	av_packet_unref(&d->pkt);
	avcodec_free_context(&d->avctx);
}

void playDecoderAbort(PlayDecoder *d, FrameQueue *fq)
{
	playPacketQueueAbort(d->queue);
	
	playFrameQueueSignal(fq);
	SDL_WaitThread(d->decoder_tid, NULL);
	d->decoder_tid = NULL;
	playPacketQueueFlush(d->queue);
}

double playClockGet(PlayClock *c)
{
	if (*c->queue_serial != c->serial)
		return NAN;
	
	if (c->paused)
	{
		return c->pts;
	}
	else
	{
		double time = av_gettime_relative() / 1000000.0;
		return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
	}
}

void playClockSetAt(PlayClock *c, double pts, int serial, double time)
{
	c->pts = pts;
	c->last_updated = time;
	c->pts_drift = c->pts - time;
	c->serial = serial;
}

void playClockSet(PlayClock *c, double pts, int serial)
{
	double time = av_gettime_relative() / 1000000.0;
	playClockSetAt(c, pts, serial, time);
}

void playClockSpeedSet(PlayClock *c, double speed)
{
	playClockSet(c, playClockGet(c), c->serial);
	c->speed = speed;
}

void playClockInit(PlayClock *c, int *queue_serial)
{
	c->speed = 1.0;
	c->paused = 0;
	c->queue_serial = queue_serial;
	playClockSet(c, NAN, -1);
}


void playClockSyncToSlave(PlayClock *c, PlayClock *slave)
{
	double clock = playClockGet(c);
	double slave_clock = playClockGet(slave);
	if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
		playClockSet(c, slave_clock, slave->serial);
}


/* get the current master clock value */
double playClockGetMaster(VideoState *is)
{
	double val;

	switch (get_master_sync_type(is))
	{
		case AV_SYNC_VIDEO_MASTER:
			val = playClockGet(&is->vidclk);
			break;
		
		case AV_SYNC_AUDIO_MASTER:
			val = playClockGet(&is->audclk);
			break;
		
		default:
			val = playClockGet(&is->extclk);
			break;
	}
	
	return val;
}



int get_master_sync_type(VideoState *is)
{
	if (is->av_sync_type == AV_SYNC_VIDEO_MASTER)
	{
		if (is->video_st)
			return AV_SYNC_VIDEO_MASTER;
		else
			return AV_SYNC_AUDIO_MASTER;
	}
	else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER)
	{
		if (is->audio_st)
			return AV_SYNC_AUDIO_MASTER;
		else
			return AV_SYNC_EXTERNAL_CLOCK;
	}
	else
	{
		return AV_SYNC_EXTERNAL_CLOCK;
	}
}


void do_exit(VideoState *is)
{
	if (is)
	{
		playPlayerDestory(is);
	}

	playSdlDestory();

	av_lockmgr_register(NULL);
	uninit_opts();
	
#if CONFIG_AVFILTER
	av_freep(&vfilters_list);
#endif
	avformat_network_deinit();
	if (show_status)
		printf("\n");

	SDL_Quit();
	av_log(NULL, AV_LOG_QUIET, "%s", "");
	exit(0);
}


//static inline 
int64_t get_valid_channel_layout(int64_t channel_layout, int channels)
{
    if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels)
        return channel_layout;
    else
        return 0;
}


