
#include "_simPlay.h"

static Uint32 sdl_refresh_timer_cb(Uint32 interval, void *opaque)
{
	SDL_Event event;
	event.type = FF_REFRESH_EVENT;
	event.user.data1 = opaque;
	SDL_PushEvent(&event);
	return 0; /* 0 means stop timer */
}

/* schedule a video refresh in 'delay' ms */
void schedule_refresh(SimPlay *is, int delayMs)
{
	SDL_AddTimer(delayMs, sdl_refresh_timer_cb, is);
}

static void sdlAudioCallback(void *userdata, Uint8 *stream, int len)
{
	SimPlay *is = (SimPlay *)userdata;
	int len1, audio_size;
	double pts;
	int isMute = 0;

//	fprintf(stderr, "SDL need %d bytes audio data\n", len);

	SDL_memset(stream, 0, len);

	while(len > 0)
	{
		/* read end of last time */
		if(is->audio_buf_index >= is->audio_buf_size)
		{
			/* We have already sent all our data; get more */
			audio_size = splayAudioDecodeFrame(is, is->audio_buf, sizeof(is->audio_buf), &pts);
			if(audio_size <= 0)
			{/* If error, output silence */
				is->audio_buf_size = 2048*2;
				memset(is->audio_buf, 0, is->audio_buf_size);
				isMute = 1;
				
				is->audio_buf_size = 0;
				is->audio_buf_index = 0;
			}
			else
			{
				is->audio_buf_size = audio_size;
				if(is->type >= SPLAY_TYPE_6)
				{
					audio_size = synchronize_audio(is, (int16_t *)is->audio_buf, audio_size, pts);
				}
			}
			
			is->audio_buf_index = 0;
		}

		if(isMute )
		{
			/* mute */
#if SIM_PLAY_OPTION_DEBUG_AUDIO
			fprintf(stderr, "%d bytes silence audio data\n", len);
#endif
			memset(stream, 0, len);
			return;
		}
		
		len1 = is->audio_buf_size - is->audio_buf_index;
#if SIM_PLAY_OPTION_DEBUG_AUDIO
		fprintf(stderr, "%d bytes audio data available, SDL need %d bytes data\n", len1, len);
#endif
		if(len1 > len)
			len1 = len;
//		memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
		SDL_MixAudio(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1, SDL_MIX_MAXVOLUME);

		len -= len1;
		stream += len1;
		
		is->audio_buf_index += len1;
	}

#if SIM_PLAY_OPTION_DEBUG_AUDIO
	fprintf(stderr, "SDL audio callback OK\n");
#endif
}

static int _stream_component_open(SimPlay *sPlay, int stream_index)
{
	AVFormatContext *pFormatCtx = sPlay->pFormatCtx;
	AVCodecContext *codecCtx = NULL;
	AVCodec *codec = NULL;
	SDL_AudioSpec wanted_spec, spec;
	int	ret;

	if(stream_index < 0 || stream_index >= pFormatCtx->nb_streams)
	{
		return -1;
	}

	codec = avcodec_find_decoder(pFormatCtx->streams[stream_index]->codecpar->codec_id);
	if(!codec)
	{
		fprintf(stderr, "Unsupported codec!\n");
		return -1;
	}

	codecCtx = avcodec_alloc_context3(codec);
	ret = avcodec_parameters_to_context(codecCtx, pFormatCtx->streams[stream_index]->codecpar );
	if(ret < 0)
	{
		fprintf(stderr, "Couldn't init codec param context :%s", av_err2str(ret));
		return -1;
	}

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

	if(codecCtx->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		int64_t channelLayout = av_get_default_channel_layout(codecCtx->channels);
		enum AVSampleFormat out_sample_fmt=AV_SAMPLE_FMT_S16;
		
		// Set audio settings from codec info
		wanted_spec.freq = codecCtx->sample_rate;
		wanted_spec.format = AUDIO_S16SYS;
//		wanted_spec.format = AUDIO_S32;
//		wanted_spec.format = AUDIO_S16;
//		wanted_spec.format = AUDIO_S32SYS;
		wanted_spec.channels = codecCtx->channels;
		wanted_spec.silence = 0;
		wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
//		wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
		wanted_spec.callback = sdlAudioCallback;
		wanted_spec.userdata = sPlay;

		av_log(NULL, AV_LOG_INFO, "sample format:%d, sample rate:%d\n", codecCtx->sample_fmt , codecCtx->sample_rate);

		if( codecCtx->sample_fmt != out_sample_fmt )
		{
			sPlay->auConvertCtx = swr_alloc();
			if(!sPlay->auConvertCtx)
			{
				av_log(NULL, AV_LOG_ERROR, "audio converter swr_alloc_set_opts failed:\n");
				exit(1);
			}

#if 0
			av_opt_set_int(sPlay->auConvertCtx, "in_channel_layout",  codecCtx->channel_layout, 0);
			av_opt_set_int(sPlay->auConvertCtx, "out_channel_layout", codecCtx->channel_layout,  0);
			av_opt_set_int(sPlay->auConvertCtx, "in_sample_rate",     codecCtx->sample_rate, 0);
			av_opt_set_int(sPlay->auConvertCtx, "out_sample_rate",    codecCtx->sample_rate, 0);
			av_opt_set_sample_fmt(sPlay->auConvertCtx, "in_sample_fmt",  AV_SAMPLE_FMT_FLTP, 0);
			av_opt_set_sample_fmt(sPlay->auConvertCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16,  0);
#else
			sPlay->auConvertCtx = swr_alloc_set_opts(NULL,channelLayout, out_sample_fmt, codecCtx->sample_rate,
				channelLayout, codecCtx->sample_fmt , codecCtx->sample_rate,0, NULL);
#endif
				
			ret = swr_init(sPlay->auConvertCtx);
			if(ret < 0)
			{
				av_log(NULL, AV_LOG_ERROR, "audio converter initialization failed:%s\n", av_err2str(ret));
				exit(1);
			}
		}

		if(SDL_OpenAudio(&wanted_spec, &spec) < 0)
		{
			fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
			return -1;
		}
		
		if (spec.format != AUDIO_S16SYS)
		{
			av_log(NULL, AV_LOG_ERROR, "SDL advised audio format %d is not supported!\n", spec.format);
			return -1;
		}
		
		if (spec.channels != wanted_spec.channels)
		{
	//		wanted_channel_layout = av_get_default_channel_layout(spec.channels);
	//		if (!wanted_channel_layout)
			{
				av_log(NULL, AV_LOG_ERROR, "SDL advised channel count %d is not supported!\n", spec.channels);
				return -1;
			}
		}
	}
	
	if(avcodec_open2(codecCtx, codec, NULL) < 0)
	{
		fprintf(stderr, "Unsupported codec!\n");
		return -1;
	}

	switch(codecCtx->codec_type)
	{
		case AVMEDIA_TYPE_AUDIO:
			sPlay->audioStream = stream_index;
			sPlay->audio_st = pFormatCtx->streams[stream_index];
			sPlay->audio_ctx = codecCtx;
			sPlay->audio_buf_size = 0;
			sPlay->audio_buf_index = 0;

			memset(sPlay->audio_buf, 0, sizeof(sPlay->audio_buf));
			
			av_init_packet(&sPlay->audio_pkt);
			splayQueueInit(&sPlay->audioq, sPlay, "AudioQ");
			if(!sPlay->audioq.sPlay)
			{
				fprintf(stderr, "NULL PLAY objec\nt");
				exit(1);
			}
			
//			SDL_PauseAudio(0);
			break;
			
		case AVMEDIA_TYPE_VIDEO:
			sPlay->videoStream = stream_index;
			sPlay->video_st = pFormatCtx->streams[stream_index];
			sPlay->video_ctx = codecCtx;
			splayQueueInit(&sPlay->videoq, sPlay, "VideoQ");

			sPlay->frame_timer = (double)av_gettime() / 1000000.0;
			sPlay->frame_last_delay = 40e-3;
			sPlay->video_current_pts_time = av_gettime();

			if(sPlay->type >= SPLAY_TYPE_4)
			{
				sPlay->videoThread = SDL_CreateThread(splayVideoDecodThread, "VideoDecode", sPlay);
			}
			
			sPlay->sws_ctx = sws_getContext(sPlay->video_ctx->width, sPlay->video_ctx->height, sPlay->video_ctx->pix_fmt, 
				sPlay->video_ctx->width, sPlay->video_ctx->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);
			break;
		
		default:
			break;
	}

	return 0;
}

int	splayInit(SimPlay *is)
{
	AVFormatContext *pFormatCtx = NULL;
	int video_index = -1;
	int audio_index = -1;
	int i;
	int	ret;

	is->videoStream=-1;
	is->audioStream=-1;

	av_register_all();
	avformat_network_init();
//	av_log_set_level( AV_LOG_TRACE);


	fprintf(stderr, "Open '%s'....\n", is->filename);
	ret = avformat_open_input(&pFormatCtx, is->filename, NULL, NULL);
	if(ret !=0)
	{
		fprintf(stderr, "%s: could not open file :%s\n", is->filename, av_err2str(ret));
		return -1;
	}

	is->pFormatCtx = pFormatCtx;

	if(avformat_find_stream_info(pFormatCtx, NULL)<0)
		return -1;

	av_dump_format(pFormatCtx, 0, is->filename, 0);

	for(i=0; i<pFormatCtx->nb_streams; i++)
	{
		if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO && video_index < 0)
		{
			video_index=i;
		}
		
		if(pFormatCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO && audio_index < 0)
		{
			audio_index=i;
		}
	}
	
	if(audio_index >= 0)
	{
		_stream_component_open(is, audio_index);
	}
	
	if(video_index >= 0)
	{
		_stream_component_open(is, video_index);
	}   

	if(is->videoStream < 0 || is->audioStream < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "%s: could not open codecs\n", is->filename);
		return -1;
	}

	is->pictq_mutex = SDL_CreateMutex();
	is->pictq_cond = SDL_CreateCond();

	SDL_PauseAudio(0);
	av_log(NULL, AV_LOG_INFO, "SimPlayer initted!\n");
	return 0;
}


double splayGetPacketTimestamp(AVPacket *pkt, AVFormatContext *ctx, int isPts)
{
	double timeStamp;
	AVRational frameRate;

	AVStream *avStream = ctx->streams[pkt->stream_index];
	if(!avStream)
	{
		av_log(NULL, AV_LOG_ERROR, "No stream found for stream No.%d\n", pkt->stream_index);
		return 0.0;
	}
	
	frameRate = av_guess_frame_rate(ctx, avStream, NULL);

	if(isPts)
		timeStamp = pkt->pts*av_q2d(avStream->time_base);
	else
		timeStamp = pkt->dts*av_q2d(avStream->time_base);

	return timeStamp;//*1.0/av_q2d(frameRate);
}


int splayReadThread(void *arg)
{
	SimPlay *is = (SimPlay *)arg;
	AVPacket pkt1, *packet = &pkt1;

	av_log(NULL, AV_LOG_INFO, "Read Thread running.....\n" );
	is->startTime = av_gettime();
	for(;;)
	{
		if(is->quit)
		{
			break;
		}
		
		// seek stuff goes here
		if(is->audioq.size > MAX_AUDIOQ_SIZE ||is->videoq.size > MAX_VIDEOQ_SIZE)
		{
			SDL_Delay(10);
			continue;
		}
		
		if(av_read_frame(is->pFormatCtx, packet) < 0)
		{
#if 0		
			if(is->pFormatCtx->pb->error == 0)
			{
				SDL_Delay(100); /* no error; wait for user input */
				continue;
			}
			else
#endif				
			{
				break;
			}
		}
		
		// Is this a packet from the video stream?
		if(packet->stream_index == is->videoStream)
		{
			if((packet->flags & AV_PKT_FLAG_KEY) != 0)
			{
				fprintf(stderr, "%d Video packet is key frame\n", is->videoPacketCount );
			}
			splayQueuePut(&is->videoq, packet);
			is->videoPacketCount ++;
#if 1//SIM_PLAY_OPTION_DEBUG_VIDEO
			fprintf(stderr, "Read: No.%d video packet of %d bytes send, PTS:%8.4f; DTS:%8.4f\n", is->videoPacketCount, packet->size,  
				splayGetPacketTimestamp(packet, is->pFormatCtx, 1), splayGetPacketTimestamp(packet, is->pFormatCtx, 0) );
#endif
		}
		else if(packet->stream_index == is->audioStream)
		{
#if SIM_PLAY_OPTION_DEBUG_AUDIO
			fprintf(stderr, "Read: Audio packet %d bytes send\n", packet->size );
#endif
			splayQueuePut(&is->audioq, packet);
		}
		else
		{
			av_packet_unref(packet);
		}
	}

	is->isReadEnd = 1;
	
	/* all done - wait for it */
	while(!is->quit)
	{
		SDL_Delay(100);
	}

	return 0;
}



int splayVideoDecodThread(void *arg)
{
	SimPlay *is = (SimPlay *)arg;
	AVPacket pkt1, *packet = &pkt1;
	int		ret;
	AVFrame *pFrame;

	double pts;
	double frameDelay;

	double packetLeapTime, sysLeapTime;

	pFrame = av_frame_alloc();


	AVRational framerate = av_guess_frame_rate(is->pFormatCtx, is->video_st, NULL);

	frameDelay = 1.0/av_q2d(framerate);
	
	av_log(NULL, AV_LOG_INFO, "Video Decode Thread running.....:duration:%8.4f\n", frameDelay);
	for(;;)
	{
		ret =splayQueueGet(&is->videoq, packet, 1);
		if( ret< 0)
		{// means we quit getting packets
			break;
		}
		else if( ret== 0)
		{// means we quit getting packets
			continue;
		}

		if(is->videoDecodedCount==0)
		{
//			is->firstPts = packet->pts*av_q2d(is->video_st->time_base);
			is->playStartRime = av_gettime()/1000000.0;
			av_log(NULL, AV_LOG_INFO, "First PTS :%8.4f, playingStartTime:%8.4f: frameDelay %10.8f (%10.6f, %d/%d)\n", 
				is->firstPts, is->playStartRime, av_q2d(is->video_st->time_base), packet->pts*av_q2d(is->video_st->time_base), is->video_st->time_base.num, is->video_st->time_base.den);
		}
		
		is->videoDecodedCount++;

		packetLeapTime = packet->pts *av_q2d(is->video_st->time_base) ;
		sysLeapTime = av_gettime()/1000000.0;
		 ;
//		if( packetLeapTime - is->firstPts < sysLeapTime  && (packet->flags & AV_PKT_FLAG_KEY) != 0) 
//		if( (packetLeapTime - is->firstPts > 2.0*frameDelay) && (IS_KEY_FRAME(packet)) )
		av_log(NULL, AV_LOG_INFO, "sysLeapTime :%8.4f\n", (sysLeapTime-is->playStartRime) );
		if( ( sysLeapTime-is->playStartRime- 2*frameDelay> 0.0) && (!IS_KEY_FRAME(packet)) )
		{
			is->videoDropCount ++;
			
			av_log(NULL, AV_LOG_ERROR, "Decode/Render too slow, drop No.%d video packet :%8.4f:%8.4f\n", is->videoDecodedCount, packetLeapTime, sysLeapTime);
			av_packet_unref(packet);
			if(is->videoDecodedCount == is->videoPacketCount && is->isReadEnd )
			{
				av_log(NULL, AV_LOG_INFO, "Program exit, %d packets read, %d frames decoded, %d frames rendered have been processed in %lld seconds!\n", 
					is->videoPacketCount, is->videoDecodedCount, is->videoRendededCount, (av_gettime()-is->startTime)/1000000 );
				sPlayExit(is);
			}
			continue;
		}
		
		is->firstPts = packetLeapTime;
		is->playStartRime = sysLeapTime;//av_gettime()/1000000.0;
		
		ret = avcodec_send_packet(is->video_ctx, packet);
		if(ret != 0)
		{
			av_log(NULL, AV_LOG_ERROR, "send video packet to decoder failed: %s\n", av_err2str(ret));
			continue;
		}

		ret = avcodec_receive_frame(is->video_ctx, pFrame);
		if( ret != 0)
		{
			av_log(NULL, AV_LOG_ERROR, "receive video frame from decoder failed: %s\n", av_err2str(ret));
			continue;
		}

		pts = 0;
		
		if(is->type >= SPLAY_TYPE_5)
		{
			if((pts = av_frame_get_best_effort_timestamp(pFrame)) == AV_NOPTS_VALUE)
			{
				av_log(NULL, AV_LOG_INFO, "Not get best effort timestamp\n");
				pts = 0;
			}

			/* this is real time */
			pts *= av_q2d(is->video_st->time_base);
			pts = simPlayerSynchronizeVideo(is, pFrame, pts);
		}
		
		if(splayQueuePutPicture(is, pFrame, pts) < 0)
		{
			break;
		}

		av_packet_unref(packet);

#if 1//SIM_PLAY_OPTION_DEBUG_VIDEO		
		av_log(NULL, AV_LOG_INFO, "Decoded No.%d in %d read video packets (rendered %d frames, video size:%d)\n", 
			is->videoDecodedCount, is->videoPacketCount, is->videoRendededCount, is->pictq_size );
#endif
		if(is->videoDecodedCount == is->videoPacketCount && is->isReadEnd )
		{
			av_log(NULL, AV_LOG_INFO, "Program exit, %d packets read, %d frames decoded, %d frames rendered have been processed in %lld seconds!\n", 
				is->videoPacketCount, is->videoDecodedCount, is->videoRendededCount, (av_gettime()-is->startTime)/1000000 );
			sPlayExit(is);
		}
	}
	
	av_frame_free(&pFrame);
	return 0;
}

void sPlayExit(SimPlay *sPlay)
{
	SDL_Event event;
	
	event.type = FF_QUIT_EVENT;
	event.user.data1 = sPlay;
	SDL_PushEvent(&event);
}

