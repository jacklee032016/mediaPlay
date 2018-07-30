// tutorial04.c
// A pedagogical video player that will stream through every video frame as fast as it can,
// and play audio (out of sync).
//
// to play the video stream on your screen.

#include "_simPlay.h"

#include <math.h>

int synchronize_audio(SimPlay *is, short *samples, int samples_size, double pts)
{
	return 0;
}


void video_refresh_timer(void *userdata)
{
	SimPlay *is = (SimPlay *)userdata;

	if(is->video_st)
	{
		if(is->pictq_size == 0)
		{
			schedule_refresh(is, 1);
		}
		else
		{
//			vp = &is->pictq[is->pictq_rindex];
			
			/* Now, normally here goes a ton of code	about timing, etc. we're just going to
			guess at a delay for now. You can increase and decrease this value and hard code
			the timing - but I don't suggest that ;) We'll learn how to do it for real later.
			*/
			/* for video with fps=24, */
			schedule_refresh(is, 1);

			/* show the picture! */
			splayWindowDisplay(is);

			/* update queue for next picture! */
			if(++is->pictq_rindex == SIMPLAY_VIDEO_PICTURE_QUEUE_SIZE)
			{
				is->pictq_rindex = 0;
			}
			
			SDL_LockMutex(is->pictq_mutex);
			is->pictq_size--;
			SDL_CondSignal(is->pictq_cond); /* wake up decodeing thread */
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
	return 0.0;
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
	sPlay->type = SPLAY_TYPE_4;

strace();
	av_strlcpy(sPlay->name, "SimPlayer04",  sizeof( sPlay->name));
	av_strlcpy(sPlay->filename, argv[1],  sizeof( sPlay->filename));

	if(splayInit(sPlay) )
	{
		return 0;
	}

	if( splayWindowCreate(sPlay) )
	{
		return 1;
	}

	schedule_refresh(sPlay, 10);

	sPlay->readThread = SDL_CreateThread(splayReadThread, "Read Thread", sPlay);
	if(!sPlay->readThread )
	{
		av_free(sPlay);
		return -1;
	}
	
	for(;;)
	{
#if 0	
		SDL_WaitEvent(&event);
		
		switch(event.type)
		{
			case FF_QUIT_EVENT:
			case SDL_QUIT:
				sPlay->quit = 1;
				av_log(NULL, AV_LOG_INFO, "Playing costs %lld seconds\n", (av_gettime()-sPlay->startTime)/1000000 );
				SDL_Quit();
				return 0;
				break;

			case FF_REFRESH_EVENT:
				video_refresh_timer(event.user.data1);
				break;

			default:
			break;
		}
#else
		video_refresh_timer(sPlay);
		
#endif

	}
	
	return 0;
}

