

#include "_simPlay.h"

#include <signal.h>

int	splayWindowCreate(SimPlay *sPlay)
{
	SPlayWindow *sWin = &sPlay->window;
	int ret = 0;

#if 0	
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE))
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE))
#else
	/* support Ctrl+C to exit */
	struct sigaction action;
	sigaction(SIGINT, NULL, &action);
	ret = SDL_Init(SDL_INIT_EVERYTHING);
	sigaction(SIGINT, &action, NULL);
	if(ret)
	{
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}
#endif

#if FFMPEG_NEW_API
	sWin->screen = SDL_CreateWindow(sPlay->name, SDL_WINDOWPOS_UNDEFINED/* startX*/,  SDL_WINDOWPOS_UNDEFINED/* startY*/, sPlay->video_ctx->width,  sPlay->video_ctx->height, 0);
	if(!sWin->screen)
	{
		fprintf(stderr, "SDL: could not create window - exiting\n");
		exit(1);
	}

	sWin->renderer = SDL_CreateRenderer(sWin->screen, -1, SDL_RENDERER_ACCELERATED);
	if(!sWin->renderer)
	{
		fprintf(stderr, "SDL: could not create Renderer - exiting\n");
		exit(1);
	}
#else	
	// Make a screen to put our video
#ifndef __DARWIN__
	screen = SDL_SetVideoMode(640, 480, 0, 0);
#else
	screen = SDL_SetVideoMode(640, 480, 24, 0);
#endif
	if(!screen)
	{
		fprintf(stderr, "SDL: could not set video mode - exiting\n");
		exit(1);
	}
#endif

#if	FFMPEG_NEW_API
//	av_image_alloc(&pointers, sWin->linesizes, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, 1);
#endif

	sWin->screen_mutex = SDL_CreateMutex();

	return 0;
}


void splayWindowDisplay(SimPlay *is)
{
	SDL_Rect rect;
	VideoPicture *vp;
	float aspect_ratio;
	int width, height;
	int w, h, x, y;

	vp = &is->pictq[is->pictq_rindex];
	if(vp->bmp)
	{
#if 0	
		if(is->video_ctx->sample_aspect_ratio.num == 0)
		{
			aspect_ratio = 0;
		}
		else
		{
			aspect_ratio = av_q2d(is->video_ctx->sample_aspect_ratio) *is->video_ctx->width / is->video_ctx->height;
		}
		
		if(aspect_ratio <= 0.0)
		{
			aspect_ratio = (float)is->video_ctx->width /(float)is->video_ctx->height;
		}

		SDL_GetWindowSize(is->window.screen, &width, &height);
		
		h = height;
		w = ((int)rint(h * aspect_ratio)) & -3;

		if(w > width)
		{
			w = width;
			h = ((int)rint(w / aspect_ratio)) & -3;
		}
		
		x = (width - w) / 2;
		y = (height - h) / 2;

		rect.x = x;
		rect.y = y;
		rect.w = w;
		rect.h = h;
#endif
		
		SDL_LockMutex(is->window.screen_mutex);
		
		is->videoRendededCount++;
		
#if FFMPEG_NEW_API
//		SDL_UpdateYUVTexture(vp->bmp, &rect, vp->data[0], is->video_ctx->width, vp->data[1], is->video_ctx->width/2, vp->data[2], is->video_ctx->width/2 );
		SDL_UpdateYUVTexture(vp->bmp, NULL, vp->data[0], is->video_ctx->width, vp->data[1], is->video_ctx->width/2, vp->data[2], is->video_ctx->width/2 );

		SDL_RenderClear(is->window.renderer);
		SDL_RenderCopy(is->window.renderer, vp->bmp, NULL, NULL);
		SDL_RenderPresent(is->window.renderer);

//		av_freep(&pointers);
//		SDL_UpdateTexture(vp->bmp, &rect, data, size);
#else
		SDL_DisplayYUVOverlay(vp->bmp, &rect);
#endif
		SDL_UnlockMutex(is->window.screen_mutex);
	}
	else
	{	
		av_log(NULL, AV_LOG_ERROR, "No image data to render\n");
	}
}



int splayAudioDecodeFrame(SimPlay *sPlay, uint8_t *audio_buf, int buf_size, double *pts_ptr)
{
	int	dataSize = 0;
	int	outSize = 0;

	AVCodecContext *aCodecCtx = sPlay->audio_ctx;
	AVFrame *frame = &sPlay->audio_frame;
	AVPacket *pkt = &sPlay->audio_pkt;
	double pts;
	
	int	ret;

	frame = av_frame_alloc();

	for(;;)
	{
		av_init_packet(pkt);
		av_frame_unref(frame);
		
		if(sPlay->quit)
		{
			return -1;
		}

		if(splayQueueGet(&sPlay->audioq, pkt, 0) <= 0)
		{
			return -1;
		}
		else 
		{
#if SIM_PLAY_OPTION_DEBUG_AUDIO
			fprintf(stderr, "\tGet audio packet of %d bytes from queue\n", pkt->size );
#endif
			if(pkt->pts != AV_NOPTS_VALUE)
			{
				sPlay->audio_clock = av_q2d( sPlay->audio_st->time_base)*pkt->pts;
			}

			ret = avcodec_send_packet(aCodecCtx, pkt);
			if(ret != 0)
			{
				av_log(NULL, AV_LOG_ERROR, "send audio packet to decoder failed: %s\n", av_err2str(ret));
//				continue;
			}
		}


		dataSize = 0;

		ret = avcodec_receive_frame(aCodecCtx, frame );
		if( ret != 0)
		{
			av_log(NULL, AV_LOG_ERROR, "receive audio frame from decoder failed: %s\n", av_err2str(ret));
			return -1;
//			continue;
		}
		else
		{
			dataSize = av_samples_get_buffer_size(NULL, av_frame_get_channels(frame), frame->nb_samples, frame->format, 1);
			if(dataSize <= 0)
			{/* No data yet, get more frames */
//				continue;
				return -1;
			}

#if SIM_PLAY_OPTION_DEBUG_AUDIO
			fprintf(stderr, "packet length is %d, data size of %d, nb_samples:%d(%d.%d)\n", pkt->size, dataSize, frame->nb_samples, frame->linesize[0], frame->linesize[1]);
#endif			

			if (sPlay->auConvertCtx )
			{
	//			ret = swr_convert(sPlay->auConvertCtx, &audio_buf, frame->nb_samples, frame->data, frame->nb_samples);
				ret = swr_convert(sPlay->auConvertCtx, &audio_buf, frame->nb_samples, (const uint8_t **)frame->extended_data, frame->nb_samples);
				if (ret < 0)
				{
					av_log(NULL, AV_LOG_ERROR, "swr_convert() failed %s\n", av_err2str(ret) );
					return -1;
				}
#if SIM_PLAY_OPTION_DEBUG_AUDIO
				fprintf(stderr, "swr converter output nb_samples : %d\n", ret);
#endif			
				outSize = dataSize/(av_get_bytes_per_sample(frame->format)/2);
			}
			else
			{
				assert(dataSize <= buf_size);
				memcpy(audio_buf, frame->data[0], dataSize);
				outSize = dataSize;
			}
			
			if(sPlay->type == SPLAY_TYPE_5 ||sPlay->type == SPLAY_TYPE_6)
			{
#if 0			
				int n;
				pts = sPlay->audio_clock;
				*pts_ptr = pts;
				n = av_get_bytes_per_sample(frame->format)*sPlay->audio_ctx->channels;
				sPlay->audio_clock += (double)dataSize /(double)(n * sPlay->audio_ctx->sample_rate);
#else
				*pts_ptr = sPlay->audio_clock;
				sPlay->audio_clock = pkt->pts*av_q2d(sPlay->audio_ctx->time_base) + frame->nb_samples/frame->sample_rate;
#endif				
			}
		
			av_frame_unref(frame);
		}
		
		av_packet_unref( pkt);
		
		/* We have data, return it and come back for more later */
#if SIM_PLAY_OPTION_DEBUG_AUDIO
		fprintf(stderr, "audio decoder return audio data of %d bytes\n", outSize );
#endif

		return outSize;
	}

	return 0;
}

