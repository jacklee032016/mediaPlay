/*
* 
*/

#include "_simPlay.h"

#include <math.h>

int synchronize_audio(SimPlay *is, short *samples, int samples_size, double pts)
{
	return 0;
}

double get_audio_clock(SimPlay *is)
{
	double pts;
	int hw_buf_size, bytes_per_sec, n;

	pts = is->audio_clock; /* maintained in the audio thread */
	hw_buf_size = is->audio_buf_size - is->audio_buf_index;
	bytes_per_sec = 0;
	
	n = is->audio_ctx->channels * av_get_bytes_per_sample(is->audio_ctx->sample_fmt);
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


void video_refresh_timer(void *userdata)
{
	SimPlay *is = (SimPlay *)userdata;
	VideoPicture *vp;
	double actual_delay, delay, sync_threshold, ref_clock, diff; /* in second */

	if(is->video_st)
	{
		if(is->pictq_size == 0)
		{
			schedule_refresh(is, 1);
		}
		else
		{
			vp = &is->pictq[is->pictq_rindex];

			delay = vp->pts - is->frame_last_pts; /* the pts from last time */
			if(delay <= 0 || delay >= 1.0)
			{/* if incorrect delay, use previous one */
				delay = is->frame_last_delay;
			}
			
			/* save for next time */
			is->frame_last_delay = delay;
			is->frame_last_pts = vp->pts;

			/* update delay to sync to audio */
			ref_clock = get_audio_clock(is);
			diff = vp->pts - ref_clock; /* difference between video and audio */

			/* Skip or repeat the frame. Take delay into account FFPlay still doesn't "know if this is the best guess." */
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
			
			is->frame_timer += delay;
			/* computer the REAL delay */
			actual_delay = is->frame_timer - (av_gettime() / 1000000.0);
			if(actual_delay < 0.010)
			{/* Really it should skip the picture instead */
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
	sPlay->type = SPLAY_TYPE_5;

	av_strlcpy(sPlay->name, "SimPlayer05",  sizeof( sPlay->name));
	av_strlcpy(sPlay->filename, argv[1],  sizeof( sPlay->filename));

	if(splayInit(sPlay) )
	{
		return 0;
	}

	if( splayWindowCreate(sPlay) )
	{
		return 1;
	}

	schedule_refresh(sPlay, 33);

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

