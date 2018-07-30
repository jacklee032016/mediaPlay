// A pedagogical video player that really works!
//

#include "_simPlay.h"

#include <math.h>


#define SAMPLE_CORRECTION_PERCENT_MAX	10
#define AUDIO_DIFF_AVG_NB					20


#define DEFAULT_AV_SYNC_TYPE		AV_SYNC_EXTERNAL_MASTER


double get_audio_clock(SimPlay *is)
{
	double pts;
	int hw_buf_size, bytes_per_sec, n;

	pts = is->audio_clock; /* maintained in the audio thread */
	hw_buf_size = is->audio_buf_size - is->audio_buf_index;
	bytes_per_sec = 0;
	n = is->audio_ctx->channels * 2;

	if(is->audio_st)
	{
		bytes_per_sec = is->audio_ctx->sample_rate * n;
	}
	
	if(bytes_per_sec)
	{
		pts -= (double)hw_buf_size / bytes_per_sec;
	}
	return pts;
}


double get_video_clock(SimPlay *is)
{
	double delta;
	delta = (av_gettime() - is->video_current_pts_time) / 1000000.0;
	return is->video_current_pts + delta;
}

double get_external_clock(SimPlay *is)
{
	return av_gettime() / 1000000.0;
}

double get_master_clock(SimPlay *is)
{
	if(is->av_sync_type == AV_SYNC_VIDEO_MASTER)
	{
		return get_video_clock(is);
	}
	else if(is->av_sync_type == AV_SYNC_AUDIO_MASTER)
	{
		return get_audio_clock(is);
	}
//	else
	{
		return get_external_clock(is);
	}
}


/* Add or subtract samples to get a better sync, return new audio buffer size */
int synchronize_audio(SimPlay *is, short *samples, int samples_size, double pts)
{
	int n;
	double ref_clock;

	n = 2 * is->audio_ctx->channels;

	if(is->av_sync_type != AV_SYNC_AUDIO_MASTER)
	{
		double diff, avg_diff;
		int wanted_size, min_size, max_size /*, nb_samples */;

		ref_clock = get_master_clock(is);
		diff = get_audio_clock(is) - ref_clock;

		if(diff < AV_NOSYNC_THRESHOLD)
		{
			// accumulate the diffs
			is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
			if(is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB)
			{
				is->audio_diff_avg_count++;
			}
			else
			{
				avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);
				if(fabs(avg_diff) >= is->audio_diff_threshold)
				{
					wanted_size = samples_size + ((int)(diff * is->audio_ctx->sample_rate) * n);
					min_size = samples_size * ((100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100);
					max_size = samples_size * ((100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100);
					
					if(wanted_size < min_size)
					{
						wanted_size = min_size;
					}
					else if (wanted_size > max_size)
					{
						wanted_size = max_size;
					}
					
					if(wanted_size < samples_size)
					{
						/* remove samples */
						samples_size = wanted_size;
					}
					else if(wanted_size > samples_size)
					{
						uint8_t *samples_end, *q;
						int nb;

						/* add samples by copying final sample*/
						nb = (samples_size - wanted_size);
						samples_end = (uint8_t *)samples + samples_size - n;
						q = samples_end + n;
						while(nb > 0)
						{
							memcpy(q, samples_end, n);
							q += n;
							nb -= n;
						}
						
						samples_size = wanted_size;
					}
				}
			}
		}
		else
		{
			/* difference is TOO big; reset diff stuff */
			is->audio_diff_avg_count = 0;
			is->audio_diff_cum = 0;
		}
	}
	
	return samples_size;
}

#if 0
void audio_callback(void *userdata, Uint8 *stream, int len)
{
	SimPlay *is = (SimPlay *)userdata;
	int len1, audio_size;
	double pts;

	while(len > 0)
	{
		if(is->audio_buf_index >= is->audio_buf_size)
		{
			/* We have already sent all our data; get more */
			audio_size = splayAudioDecodeFrame(is, is->audio_buf, sizeof(is->audio_buf), &pts);
			if(audio_size < 0)
			{
				/* If error, output silence */
				is->audio_buf_size = 1024;
				memset(is->audio_buf, 0, is->audio_buf_size);
			}
			else
			{
				audio_size = synchronize_audio(is, (int16_t *)is->audio_buf, audio_size, pts);
				is->audio_buf_size = audio_size;
			}
			
			is->audio_buf_index = 0;
		}
		
		len1 = is->audio_buf_size - is->audio_buf_index;
		if(len1 > len)
			len1 = len;
		
		memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
		len -= len1;
		stream += len1;
		is->audio_buf_index += len1;
		
	}
}
#endif

void video_refresh_timer(void *userdata)
{
	SimPlay *is = (SimPlay *)userdata;
	VideoPicture *vp;
	double actual_delay, delay, sync_threshold, ref_clock, diff;

	if(is->video_st)
	{
		if(is->pictq_size == 0)
		{
			schedule_refresh(is, 1);
		}
		else
		{
			vp = &is->pictq[is->pictq_rindex];

			is->video_current_pts = vp->pts;
			is->video_current_pts_time = av_gettime();
			delay = vp->pts - is->frame_last_pts; /* the pts from last time */
			if(delay <= 0 || delay >= 1.0)
			{
				/* if incorrect delay, use previous one */
				delay = is->frame_last_delay;
			}
			/* save for next time */
			is->frame_last_delay = delay;
			is->frame_last_pts = vp->pts;



			/* update delay to sync to audio if not master source */
			if(is->av_sync_type != AV_SYNC_VIDEO_MASTER)
			{
				ref_clock = get_master_clock(is);
				diff = vp->pts - ref_clock;

				/* Skip or repeat the frame. Take delay into account
				FFPlay still doesn't "know if this is the best guess." */
				sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
				if(fabs(diff) < AV_NOSYNC_THRESHOLD)
				{
					if(diff <= -sync_threshold)
					{
						delay = 0;
					}
					else if(diff >= sync_threshold)
					{
						delay = 2 * delay;
					}
				}
			}
			
			is->frame_timer += delay;
			/* computer the REAL delay */
			actual_delay = is->frame_timer - (av_gettime() / 1000000.0);
			if(actual_delay < 0.010)
			{
				/* Really it should skip the picture instead */
				actual_delay = 0.010;
			}
			schedule_refresh(is, (int)(actual_delay * 1000 + 0.5));

			/* show the picture! */
			splayWindowDisplay(is);

			/* update queue for next picture! */
			if(++is->pictq_rindex == SIMPLAY_VIDEO_PICTURE_QUEUE_SIZE)
			{
				is->pictq_rindex = 0;
			}
			
			SDL_LockMutex(is->pictq_mutex);
			is->pictq_size--;
			SDL_CondSignal(is->pictq_cond);
			SDL_UnlockMutex(is->pictq_mutex);
		}
	}
	else
	{
		schedule_refresh(is, 100);
	}
}
      

double simPlayerSynchronizeVideo(SimPlay *is, AVFrame *src_frame, double pts)
{
	double frame_delay;

	if(pts != 0)
	{/* if we have pts, set video clock to it */
		is->video_clock = pts;
	}
	else
	{/* if we aren't given a pts, set it to the clock */
		pts = is->video_clock;
	}

	/* update the video clock */
	frame_delay = av_q2d(is->video_ctx->time_base);
	/* if we are repeating a frame, adjust clock accordingly */
	frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
	is->video_clock += frame_delay;

	return pts;
}

int main(int argc, char *argv[])
{
	SDL_Event       event;
	SimPlay _sPlay;
	SimPlay      *sPlay = NULL;

	if(argc < 2)
	{
		fprintf(stderr, "Usage: test <file>\n");
		exit(1);
	}
	
	sPlay = &_sPlay;
	memset(sPlay, 0, sizeof(SimPlay));
	sPlay->type = SPLAY_TYPE_6;
	sPlay->av_sync_type = DEFAULT_AV_SYNC_TYPE;

strace();
	av_strlcpy(sPlay->name, "SimPlayer06",  sizeof( sPlay->name));
	av_strlcpy(sPlay->filename, argv[1],  sizeof( sPlay->filename));

	if(splayInit(sPlay) )
	{
		return 0;
	}

	if( splayWindowCreate(sPlay) )
	{
		return 1;
	}

	schedule_refresh(sPlay, 40);

	sPlay->readThread = SDL_CreateThread(splayReadThread, "Read Thread", sPlay);
	if(!sPlay->readThread )
	{
		av_free(sPlay);
		return -1;
	}
	
	for(;;)
	{
		SDL_WaitEvent(&event);
		
		switch(event.type)
		{
			case FF_QUIT_EVENT:
			case SDL_QUIT:
				sPlay->quit = 1;
				SDL_Quit();
				return 0;
				break;

			case FF_REFRESH_EVENT:
				video_refresh_timer(event.user.data1);
				break;

			default:
			break;
		}
	}
	
	return 0;
}

