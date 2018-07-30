
#include "play.h"

static inline int _cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1,
                   enum AVSampleFormat fmt2, int64_t channel_count2)
{
	/* If channel count == 1, planar and non-planar formats are the same */
	if (channel_count1 == 1 && channel_count2 == 1)
		return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
	else
		return channel_count1 != channel_count2 || fmt1 != fmt2;
}

int playThreadAudioDecode(void *arg)
{
	VideoState *is = arg;
	AVFrame *frame = av_frame_alloc();
	PlayFrame *af;
#if CONFIG_AVFILTER
	int last_serial = -1;
	int64_t dec_channel_layout;
	int reconfigure;
#endif
	int got_frame = 0;
	AVRational tb;
	int ret = 0;

	if (!frame)
		return AVERROR(ENOMEM);

	do
	{
		if ((got_frame = playDecoderDecodeFrame(&is->audioDecoder, frame, NULL)) < 0)
			goto the_end;

		if (got_frame)
		{
			tb = (AVRational){1, frame->sample_rate};

#if CONFIG_AVFILTER
			dec_channel_layout = get_valid_channel_layout(frame->channel_layout, av_frame_get_channels(frame));

			reconfigure = 	_cmp_audio_fmts(is->audio_filter_src.fmt, is->audio_filter_src.channels,
				frame->format, av_frame_get_channels(frame))    ||
				is->audio_filter_src.channel_layout != dec_channel_layout ||
				is->audio_filter_src.freq           != frame->sample_rate ||
				is->audioDecoder.pkt_serial               != last_serial;

			if (reconfigure)
			{
				char buf1[1024], buf2[1024];

				av_get_channel_layout_string(buf1, sizeof(buf1), -1, is->audio_filter_src.channel_layout);
				av_get_channel_layout_string(buf2, sizeof(buf2), -1, dec_channel_layout);
				
				av_log(NULL, AV_LOG_DEBUG, "Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d fmt:%s layout:%s serial:%d\n",
					is->audio_filter_src.freq, is->audio_filter_src.channels, av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1, last_serial,
					frame->sample_rate, av_frame_get_channels(frame), av_get_sample_fmt_name(frame->format), buf2, is->audioDecoder.pkt_serial);

				is->audio_filter_src.fmt            = frame->format;
				is->audio_filter_src.channels       = av_frame_get_channels(frame);
				is->audio_filter_src.channel_layout = dec_channel_layout;
				is->audio_filter_src.freq           = frame->sample_rate;
				last_serial                         = is->audioDecoder.pkt_serial;

				if ((ret = configure_audio_filters(is, afilters, 1)) < 0)
					goto the_end;
			}

			if ((ret = av_buffersrc_add_frame(is->in_audio_filter, frame)) < 0)
				goto the_end;

			while ((ret = av_buffersink_get_frame_flags(is->out_audio_filter, frame, 0)) >= 0)
			{
				tb = av_buffersink_get_time_base(is->out_audio_filter);
#endif

				if (!(af = playFrameQueuePeekWritable(&is->audioFrameQueue)))
					goto the_end;

				af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
				af->pos = av_frame_get_pkt_pos(frame);
				af->serial = is->audioDecoder.pkt_serial;
				af->duration = av_q2d((AVRational){frame->nb_samples, frame->sample_rate});

				/* copy source avFrame to dest avFrame */
				av_frame_move_ref(af->frame, frame);
				playFrameQueuePush(&is->audioFrameQueue);

#if CONFIG_AVFILTER
				if (is->audioq.serial != is->audioDecoder.pkt_serial)
					break;
			}

			if (ret == AVERROR_EOF)
				is->audioDecoder.finished = is->audioDecoder.pkt_serial;
#endif
		}
	} while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
	
the_end:
#if CONFIG_AVFILTER
	avfilter_graph_free(&is->agraph);
#endif
	av_frame_free(&frame);
	return ret;
}


static int _getVideoAndDecodeFrame(VideoState *is, AVFrame *frame)
{
	int got_picture;

	if ((got_picture = playDecoderDecodeFrame(&is->videoDecoder, frame, NULL)) < 0)
		return -1;

	if (got_picture)
	{
		double dpts = NAN;

		if (frame->pts != AV_NOPTS_VALUE)
			dpts = av_q2d(is->video_st->time_base) * frame->pts;

		frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);

		if (framedrop>0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER))
		{
			if (frame->pts != AV_NOPTS_VALUE)
			{
				double _mstClk = playClockGetMaster(is);
				double diff = dpts - _mstClk;

				/* when pts of this packet is smaller than current master clock */
				if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
					diff - is->frame_last_filter_delay < 0 &&
					is->videoDecoder.pkt_serial == is->vidclk.serial &&
					is->videoq.nb_packets)
				{
					is->frame_drops_early++;

					PLAY_LOG("frame removed because of early, total %d frames removed for early:pts %8.4f: clock:%8.4f\n", is->frame_drops_early, dpts, _mstClk);
					av_frame_unref(frame);
					got_picture = 0;
				}
			}
		}
	}

	return got_picture;
}


static int _queue_picture(VideoState *is, AVFrame *src_frame, double pts, double duration, int64_t pos, int serial)
{
	PlayFrame *vp;

#if PLAYER_OPTIONS_DEBUG_SYNC
	PLAY_LOG("video frame type=%c pts=%0.3f into frame queue\n", av_get_picture_type_char(src_frame->pict_type), pts);
#endif

	if (!(vp = playFrameQueuePeekWritable(&is->videoFrameQueue)))
		return -1;

	vp->sar = src_frame->sample_aspect_ratio;
	vp->uploaded = 0;

	vp->width = src_frame->width;
	vp->height = src_frame->height;
	vp->format = src_frame->format;

	vp->pts = pts;
	vp->duration = duration;
	vp->pos = pos;
	vp->serial = serial;

	set_default_window_size(vp->width, vp->height, vp->sar);

	av_frame_move_ref(vp->frame, src_frame);
	playFrameQueuePush(&is->videoFrameQueue);
	return 0;
}


int playThreadVideoDecode(void *arg)
{
	VideoState *is = arg;
	AVFrame *frame = av_frame_alloc();
	double pts;
	double duration;
	int ret;
	AVRational tb = is->video_st->time_base;
	AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);

#if CONFIG_AVFILTER
	AVFilterGraph *graph = avfilter_graph_alloc();
	AVFilterContext *filt_out = NULL, *filt_in = NULL;
	int last_w = 0;
	int last_h = 0;
	enum AVPixelFormat last_format = -2;
	int last_serial = -1;
	int last_vfilter_idx = 0;
	if (!graph)
	{
		av_frame_free(&frame);
		return AVERROR(ENOMEM);
	}

#endif

	if (!frame)
	{
#if CONFIG_AVFILTER
		avfilter_graph_free(&graph);
#endif
		return AVERROR(ENOMEM);
	}

	for (;;)
	{
		ret = _getVideoAndDecodeFrame(is, frame);
		if (ret < 0)
			goto the_end;
		
		if (!ret)
			continue;

#if CONFIG_AVFILTER
		if (   last_w != frame->width
			|| last_h != frame->height
			|| last_format != frame->format
			|| last_serial != is->videoDecoder.pkt_serial
			|| last_vfilter_idx != is->vfilter_idx)
		{
			av_log(NULL, AV_LOG_DEBUG,  "Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s serial:%d\n",
				last_w, last_h,
				(const char *)av_x_if_null(av_get_pix_fmt_name(last_format), "none"), last_serial,
				frame->width, frame->height,
				(const char *)av_x_if_null(av_get_pix_fmt_name(frame->format), "none"), is->videoDecoder.pkt_serial);

			avfilter_graph_free(&graph);
			graph = avfilter_graph_alloc();
			if ((ret = configure_video_filters(graph, is, vfilters_list ? vfilters_list[is->vfilter_idx] : NULL, frame)) < 0)
			{
				SDL_Event event;
				event.type = FF_QUIT_EVENT;
				event.user.data1 = is;
				SDL_PushEvent(&event);
				goto the_end;
			}
			
			filt_in  = is->in_video_filter;
			filt_out = is->out_video_filter;
			last_w = frame->width;
			last_h = frame->height;
			last_format = frame->format;
			last_serial = is->videoDecoder.pkt_serial;
			last_vfilter_idx = is->vfilter_idx;
			frame_rate = av_buffersink_get_frame_rate(filt_out);
		}

		ret = av_buffersrc_add_frame(filt_in, frame);
		if (ret < 0)
			goto the_end;

		while (ret >= 0)
		{
			is->frame_last_returned_time = av_gettime_relative() / 1000000.0;

			ret = av_buffersink_get_frame_flags(filt_out, frame, 0);
			if (ret < 0)
			{
				if (ret == AVERROR_EOF)
					is->videoDecoder.finished = is->videoDecoder.pkt_serial;
				ret = 0;
				break;
			}

			is->frame_last_filter_delay = av_gettime_relative() / 1000000.0 - is->frame_last_returned_time;
			if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0)
				is->frame_last_filter_delay = 0;
			tb = av_buffersink_get_time_base(filt_out);
#endif

			duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
			pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
			ret = _queue_picture(is, frame, pts, duration, av_frame_get_pkt_pos(frame), is->videoDecoder.pkt_serial);
			av_frame_unref(frame);

#if CONFIG_AVFILTER
		}
#endif

		if (ret < 0)
			goto the_end;
	}
	
 the_end:
#if CONFIG_AVFILTER
	avfilter_graph_free(&graph);
#endif
	av_frame_free(&frame);
	return 0;
}

int playThreadSubtitleDecode(void *arg)
{
	VideoState *is = arg;
	PlayFrame *sp;
	int got_subtitle;
	double pts;

	for (;;)
	{
		if (!(sp = playFrameQueuePeekWritable(&is->subtitleFrameQueue)))
			return 0;

		if ((got_subtitle = playDecoderDecodeFrame(&is->subtitleDecoder, NULL, &sp->sub)) < 0)
			break;

		pts = 0;

		if (got_subtitle && sp->sub.format == 0)
		{
			if (sp->sub.pts != AV_NOPTS_VALUE)
				pts = sp->sub.pts / (double)AV_TIME_BASE;
			
			sp->pts = pts;
			sp->serial = is->subtitleDecoder.pkt_serial;
			sp->width = is->subtitleDecoder.avctx->width;
			sp->height = is->subtitleDecoder.avctx->height;
			sp->uploaded = 0;

			/* now we can update the picture count */
			playFrameQueuePush(&is->subtitleFrameQueue);
		}
		else if (got_subtitle)
		{
			avsubtitle_free(&sp->sub);
		}
	}
	return 0;
	}


