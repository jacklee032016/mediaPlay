// tutorial03.c
// A pedagogical video player that will stream through every video frame as fast as it can
// and play audio (out of sync).

#include "_simPlay.h"


double simPlayerSynchronizeVideo(SimPlay *is, AVFrame *src_frame, double pts)
{
	return 0.0;
}

int synchronize_audio(SimPlay *is, short *samples, int samples_size, double pts)
{
	return 0;
}


int main(int argc, char *argv[])
{
	AVFrame         *pFrame = NULL;
	AVPacket        packet;
	VideoPicture		vp;
	int				ret;

	SimPlay	_simPlay;
	SimPlay *sPlay;
	
	SDL_Event       event;

	if(argc < 2)
	{
		fprintf(stderr, "Usage: test <file>\n");
		exit(1);
	}

	sPlay = &_simPlay;
	memset(sPlay, 0, sizeof(SimPlay));
	sPlay->type = SPLAY_TYPE_3;

	av_strlcpy(sPlay->name, "SimPlayer03",  sizeof( sPlay->name));
	av_strlcpy(sPlay->filename, argv[1],  sizeof( sPlay->filename));

	fprintf(stderr, "<file>:%s:%s\n", argv[1], sPlay->filename);
	if(splayInit(sPlay))
	{
		return 1;
	}

	if( splayWindowCreate(sPlay) )
	{
		return 1;
	}

	// Allocate video frame
	pFrame=av_frame_alloc();
	av_image_alloc( vp.data, vp.linesizes, sPlay->video_ctx->width, sPlay->video_ctx->height, AV_PIX_FMT_YUV420P, 1);
	vp.bmp = SDL_CreateTexture(sPlay->window.renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, sPlay->video_ctx->width, sPlay->video_ctx->height );
	if (!vp.bmp)
	{
		fprintf(stderr, "SDL: could not create texture - exiting\n");
		exit(1);
	}

	sPlay->startTime = av_gettime();
	while(av_read_frame(sPlay->pFormatCtx, &packet)>=0)
	{
		// Is this a packet from the video stream?
		if(packet.stream_index== sPlay->videoStream )
		{
#if 1
			ret = avcodec_send_packet(sPlay->video_ctx, &packet);
			if(ret != 0)
			{
				av_log(NULL, AV_LOG_ERROR,  "send video packet to decoder failed: %s\n", av_err2str(ret));
				continue;
			}

//			fprintf(stderr, "read video packet of %d bytes\n", packet.size);
			ret = avcodec_receive_frame(sPlay->video_ctx, pFrame);
			if( ret != 0)
			{
				av_log(NULL, AV_LOG_ERROR, "receive video frame from decoder failed: %s\n", av_err2str(ret));
				continue;
			}

			sws_scale(sPlay->sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0, sPlay->video_ctx->height, vp.data, vp.linesizes);
			
			SDL_UpdateYUVTexture(vp.bmp, NULL, vp.data[0], sPlay->video_ctx->width, vp.data[1], sPlay->video_ctx->width/2, vp.data[2], sPlay->video_ctx->width/2 );

			SDL_RenderClear(sPlay->window.renderer);
			SDL_RenderCopy(sPlay->window.renderer, vp.bmp, NULL, NULL);
			SDL_RenderPresent(sPlay->window.renderer);
#endif

//				av_packet_unref(&packet);
//		SDL_Delay(10);

		}
		else if(packet.stream_index== sPlay->audioStream )
		{
#if SIM_PLAY_OPTION_DEBUG_AUDIO
			fprintf(stderr, "MAIN : read audio packet of %d bytes\n", packet.size);
#endif
			splayQueuePut(&sPlay->audioq, &packet);
		}
		else
		{
			av_packet_unref(&packet);
		}

//		SDL_Delay(20);
//		SDL_Delay(10);
		
		// Free the packet that was allocated by av_read_frame
		SDL_PollEvent(&event);
		switch(event.type)
		{
			case SDL_QUIT:
				sPlay->quit = 1;
				SDL_Quit();
				exit(0);
				break;

			default:
				break;
		}

	}

	av_log(NULL, AV_LOG_INFO, "Playing costs %lld seconds\n", (av_gettime()-sPlay->startTime)/1000000 );

	// Free the YUV frame
	av_frame_free(&pFrame);

#if 0
	// Close the codecs
	avcodec_close(pCodecCtxOrig);
	avcodec_close(pCodecCtx);
	avcodec_close(aCodecCtxOrig);
	avcodec_close(aCodecCtx);

	// Close the video file
	avformat_close_input(&pFormatCtx);
#endif

	return 0;
}

