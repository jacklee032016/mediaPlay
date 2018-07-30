
#include "play.h"

static double _vp_duration(VideoState *is, PlayFrame *vp, PlayFrame *nextvp)
{
	if (vp->serial == nextvp->serial)
	{
		double duration = nextvp->pts - vp->pts;
		
		if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration)
			return vp->duration;
		else
			return duration;
	}
	else
	{
		return 0.0;
	}
}

/* why? */
static void _check_external_clock_speed(VideoState *is)
{
	if (is->video_stream >= 0 && is->videoq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES ||
		is->audio_stream >= 0 && is->audioq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES)
	{
		playClockSpeedSet(&is->extclk, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, is->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
	}
	else if ((is->video_stream < 0 || is->videoq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
		(is->audio_stream < 0 || is->audioq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES))
	{
		playClockSpeedSet(&is->extclk, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, is->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
	}
	else
	{
		double speed = is->extclk.speed;
		if (speed != 1.0)
			playClockSpeedSet(&is->extclk, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
	}
}


static double _compute_target_delay(double delay, VideoState *is)
{
	double sync_threshold, diff = 0;

	/* update delay to follow master synchronisation source */
	if (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)
	{
		/* if video is slave, we try to correct big delays by duplicating or deleting a frame */
		diff = playClockGet(&is->vidclk) - playClockGetMaster(is);

		/* skip or repeat frame. We take into account the
		delay to compute the threshold. I still don't know
		if it is the best guess */
		sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
		if (!isnan(diff) && fabs(diff) < is->max_frame_duration)
		{
			if (diff <= -sync_threshold)
				delay = FFMAX(0, delay + diff);
			else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
				delay = delay + diff;
			else if (diff >= sync_threshold)
				delay = 2 * delay;
		}
	}

	PLAY_LOG( "video: delay=%0.3f A-V=%f\n", delay, -diff);

	return delay;
}

static void _update_video_pts(VideoState *is, double pts, int64_t pos, int serial)
{
	/* update current video pts */
	playClockSet(&is->vidclk, pts, serial);
	playClockSyncToSlave(&is->extclk, &is->vidclk);
}

static void debugVideoStatistics(VideoState *is)
{
	if (show_status)
	{
		static int64_t last_time;
		int64_t cur_time;
		int aqsize, vqsize, sqsize;
		double av_diff;

		cur_time = av_gettime_relative();
		if (!last_time || (cur_time - last_time) >= 30000)
		{
			aqsize = 0;
			vqsize = 0;
			sqsize = 0;
			if (is->audio_st)
				aqsize = is->audioq.size;
			if (is->video_st)
				vqsize = is->videoq.size;
			if (is->subtitle_st)
				sqsize = is->subtitleq.size;
			
			av_diff = 0;
			if (is->audio_st && is->video_st)
			{/* diff between A-V when both streams exist */
				av_diff = playClockGet(&is->audclk) - playClockGet(&is->vidclk);
			}
			else if (is->video_st)
			{/* only one stream, so diff between master clock and the existed stream */
				av_diff = playClockGetMaster(is) - playClockGet(&is->vidclk);
			}
			else if (is->audio_st)
				av_diff = playClockGetMaster(is) - playClockGet(&is->audclk);
			
			PLAY_LOG( "%7.2f %s:%7.3f fd=%4d(Early:%d;Late:%d) aq=%5dKB vq=%5dKB sq=%5dB f=%"PRId64"/%"PRId64"   \n",
				playClockGetMaster(is), (is->audio_st && is->video_st) ? "A-V" : (is->video_st ? "M-V" : (is->audio_st ? "M-A" : "   ")),
				av_diff, 
				is->frame_drops_early + is->frame_drops_late, is->frame_drops_early, is->frame_drops_late,
				aqsize / 1024,
				vqsize / 1024,
				sqsize,
				is->video_st ? is->videoDecoder.avctx->pts_correction_num_faulty_dts : 0,
				is->video_st ? is->videoDecoder.avctx->pts_correction_num_faulty_pts : 0);
			fflush(stdout);
			last_time = cur_time;
		}
	}
}

/* called in SDL event loop, to display each frame. update the remaining_time to delay for next video refresh */
void playVideoRefresh(void *opaque, double *remaining_time)
{
	VideoState *is = opaque;
	double time;

	PlayFrame *sp, *sp2;

	if (!is->paused && get_master_sync_type(is) == AV_SYNC_EXTERNAL_CLOCK && is->realtime)
		_check_external_clock_speed(is);

	if (!display_disable && is->show_mode != SHOW_MODE_VIDEO && is->audio_st)
	{
		time = av_gettime_relative() / 1000000.0;
		if (is->force_refresh || is->last_vis_time + rdftspeed < time)
		{/* refresh display with last VP in frame queue */
			playSdlVideoDisplay(is);
			is->last_vis_time = time;
		}
		*remaining_time = FFMIN(*remaining_time, is->last_vis_time + rdftspeed - time);
	}

	if (is->video_st)
	{
retry:
		if (playFrameQueueNbRemaining(&is->videoFrameQueue) == 0)
		{
		// nothing to do, no picture to display in the queue
		}
		else
		{
			double last_duration, duration, delay;
			PlayFrame *vp, *lastvp;

			/* dequeue the picture */
			lastvp = playFrameQueuePeekLast(&is->videoFrameQueue);
			vp = playFrameQueuePeek(&is->videoFrameQueue);

			if (vp->serial != is->videoq.serial)
			{/* if vp is not current one, then move the index and try again */
				playFrameQueueNext(&is->videoFrameQueue);
				goto retry;
			}

			if (lastvp->serial != vp->serial)
				is->frame_timer = av_gettime_relative() / 1000000.0;

			if (is->paused)
				goto display;

			/* compute nominal last_duration */
			last_duration = _vp_duration(is, lastvp, vp);
			delay = _compute_target_delay(last_duration, is);

			time= av_gettime_relative()/1000000.0;
			if (time < is->frame_timer + delay)
			{
				*remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
				goto display;
			}

			is->frame_timer += delay;
			if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX)
				is->frame_timer = time;

			SDL_LockMutex(is->videoFrameQueue.mutex);
			if (!isnan(vp->pts))
				_update_video_pts(is, vp->pts, vp->pos, vp->serial);
			SDL_UnlockMutex(is->videoFrameQueue.mutex);

			if (playFrameQueueNbRemaining(&is->videoFrameQueue) > 1)
			{
				PlayFrame *nextvp = playFrameQueuePeekNext(&is->videoFrameQueue);
				duration = _vp_duration(is, vp, nextvp);
				
				if(!is->step && (framedrop>0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) && time > is->frame_timer + duration)
				{/* drop happen only when master clock is not video */
					is->frame_drops_late++;
					PLAY_LOG("One video removed because of late, total %d video frame have been removed for later\n", is->frame_drops_late);
					playFrameQueueNext(&is->videoFrameQueue);
					goto retry;
				}
			}

			if (is->subtitle_st)
			{
				while (playFrameQueueNbRemaining(&is->subtitleFrameQueue) > 0)
				{
					sp = playFrameQueuePeek(&is->subtitleFrameQueue);

					if (playFrameQueueNbRemaining(&is->subtitleFrameQueue) > 1)
						sp2 = playFrameQueuePeekNext(&is->subtitleFrameQueue);
					else
						sp2 = NULL;

					if (sp->serial != is->subtitleq.serial
						|| (is->vidclk.pts > (sp->pts + ((float) sp->sub.end_display_time / 1000)))
						|| (sp2 && is->vidclk.pts > (sp2->pts + ((float) sp2->sub.start_display_time / 1000))))
					{
						if (sp->uploaded)
						{/* clear the sect of subtitle */
							int i;
							for (i = 0; i < sp->sub.num_rects; i++)
							{
								AVSubtitleRect *sub_rect = sp->sub.rects[i];
								uint8_t *pixels;
								int pitch, j;

								if (!SDL_LockTexture(is->sub_texture, (SDL_Rect *)sub_rect, (void **)&pixels, &pitch))
								{
									for (j = 0; j < sub_rect->h; j++, pixels += pitch)
										memset(pixels, 0, sub_rect->w << 2);
									SDL_UnlockTexture(is->sub_texture);
								}
							}
						}
						
						playFrameQueueNext(&is->subtitleFrameQueue);
					}
					else
					{
						break;
					}
				}
			}

			playFrameQueueNext(&is->videoFrameQueue);
			is->force_refresh = 1;

			if (is->step && !is->paused)
				stream_toggle_pause(is);
		}
		
display:
		/* display picture */
		if (!display_disable && is->force_refresh && is->show_mode == SHOW_MODE_VIDEO && is->videoFrameQueue.rindex_shown)
		{
			playSdlVideoDisplay(is);
		}
	}
	
	is->force_refresh = 0;

	debugVideoStatistics( is);
	
}

