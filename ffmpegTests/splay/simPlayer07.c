// tutorial05.c
// A pedagogical video player that really works!
//
// gcc -o tutorial05 tutorial05.c -lavformat -lavcodec -lswscale -lz -lm `sdl-config --cflags --libs`
// to build (assuming libavformat and libavcodec are correctly installed, 
// and assuming you have sdl-config. Please refer to SDL docs for your installation.)
//
// Run using
// tutorial04 myvideofile.mpg
//
// to play the video stream on your screen.

#include "_simPlay.h"

#include <math.h>


#define SAMPLE_CORRECTION_PERCENT_MAX 10
#define AUDIO_DIFF_AVG_NB 20

#define DEFAULT_AV_SYNC_TYPE AV_SYNC_VIDEO_MASTER



typedef struct SimPlay7 {

  AVFormatContext *pFormatCtx;
  int             videoStream, audioStream;

  int             av_sync_type;
  double          external_clock; /* external clock base */
  int64_t         external_clock_time;
  int             seek_req;
  int             seek_flags;
  int64_t         seek_pos;

  double          audio_clock;
  AVStream        *audio_st;
  AVCodecContext  *audio_ctx;
  PacketQueue     audioq;
  uint8_t         audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
  unsigned int    audio_buf_size;
  unsigned int    audio_buf_index;
  AVFrame         audio_frame;
  AVPacket        audio_pkt;
  uint8_t         *audio_pkt_data;
  int             audio_pkt_size;
  int             audio_hw_buf_size;
  double          audio_diff_cum; /* used for AV difference average computation */
  double          audio_diff_avg_coef;
  double          audio_diff_threshold;
  int             audio_diff_avg_count;
  double          frame_timer;
  double          frame_last_pts;
  double          frame_last_delay;
  double          video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame
  double          video_current_pts; ///<current displayed pts (different from video_clock if frame fifos are used)
  int64_t         video_current_pts_time;  ///<time (av_gettime) at which we updated video_current_pts - used to have running video pts
  AVStream        *video_st;
  AVCodecContext  *video_ctx;
  PacketQueue     videoq;
  struct SwsContext *sws_ctx;

  VideoPicture    pictq[SIMPLAY_VIDEO_PICTURE_QUEUE_SIZE];
  int             pictq_size, pictq_rindex, pictq_windex;
  SDL_mutex       *pictq_mutex;
  SDL_cond        *pictq_cond;
  
  SDL_Thread      *parse_tid;
  SDL_Thread      *video_tid;

  char            filename[1024];
  int             quit;
} SimPlay7;


/* Since we only have one decoding thread, the Big Struct
   can be global in case we need it. */
SimPlay *global_video_state;
AVPacket flush_pkt;


/* Add or subtract samples to get a better sync, return new
   audio buffer size */
int synchronize_audio(SimPlay *is, short *samples,
		      int samples_size, double pts) {
  int n;
  double ref_clock;

  n = 2 * is->audio_ctx->channels;
  
  if(is->av_sync_type != AV_SYNC_AUDIO_MASTER) {
    double diff, avg_diff;
    int wanted_size, min_size, max_size /*, nb_samples */;
    
    ref_clock = get_master_clock(is);
    diff = get_audio_clock(is) - ref_clock;

    if(diff < AV_NOSYNC_THRESHOLD) {
      // accumulate the diffs
      is->audio_diff_cum = diff + is->audio_diff_avg_coef
	* is->audio_diff_cum;
      if(is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
	is->audio_diff_avg_count++;
      } else {
	avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);
	if(fabs(avg_diff) >= is->audio_diff_threshold) {
	  wanted_size = samples_size + ((int)(diff * is->audio_ctx->sample_rate) * n);
	  min_size = samples_size * ((100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100);
	  max_size = samples_size * ((100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100);
	  if(wanted_size < min_size) {
	    wanted_size = min_size;
	  } else if (wanted_size > max_size) {
	    wanted_size = max_size;
	  }
	  if(wanted_size < samples_size) {
	    /* remove samples */
	    samples_size = wanted_size;
	  } else if(wanted_size > samples_size) {
	    uint8_t *samples_end, *q;
	    int nb;

	    /* add samples by copying final sample*/
	    nb = (samples_size - wanted_size);
	    samples_end = (uint8_t *)samples + samples_size - n;
	    q = samples_end + n;
	    while(nb > 0) {
	      memcpy(q, samples_end, n);
	      q += n;
	      nb -= n;
	    }
	    samples_size = wanted_size;
	  }
	}
      }
    } else {
      /* difference is TOO big; reset diff stuff */
      is->audio_diff_avg_count = 0;
      is->audio_diff_cum = 0;
    }
  }
  return samples_size;
}

int audio_decode_frame(SimPlay *is, uint8_t *audio_buf, int buf_size, double *pts_ptr) {

  int len1, data_size = 0;
  AVPacket *pkt = &is->audio_pkt;
  double pts;
  int n;

  for(;;) {
    while(is->audio_pkt_size > 0) {
      int got_frame = 0;
      len1 = avcodec_decode_audio4(is->audio_ctx, &is->audio_frame, &got_frame, pkt);
      if(len1 < 0) {
	/* if error, skip frame */
	is->audio_pkt_size = 0;
	break;
      }
      data_size = 0;
      if(got_frame) {
	data_size = av_samples_get_buffer_size(NULL, 
					       is->audio_ctx->channels,
					       is->audio_frame.nb_samples,
					       is->audio_ctx->sample_fmt,
					       1);
	assert(data_size <= buf_size);
	memcpy(audio_buf, is->audio_frame.data[0], data_size);
      }
      is->audio_pkt_data += len1;
      is->audio_pkt_size -= len1;
      if(data_size <= 0) {
	/* No data yet, get more frames */
	continue;
      }
      pts = is->audio_clock;
      *pts_ptr = pts;
      n = 2 * is->audio_ctx->channels;
      is->audio_clock += (double)data_size /(double)(n * is->audio_ctx->sample_rate);
      /* We have data, return it and come back for more later */
      return data_size;
    }
    if(pkt->data)
      av_free_packet(pkt);

    if(is->quit) {
      return -1;
    }
    /* next packet */
    if(splayQueueGet(&is->audioq, pkt, 1) < 0) {
      return -1;
    }
    if(pkt->data == flush_pkt.data) {
      avcodec_flush_buffers(is->audio_ctx);
      continue;
    }
    is->audio_pkt_data = pkt->data;
    is->audio_pkt_size = pkt->size;
    /* if update, update the audio clock w/pts */
    if(pkt->pts != AV_NOPTS_VALUE) {
      is->audio_clock = av_q2d(is->audio_st->time_base)*pkt->pts;
    }
  }
}

void audio_callback(void *userdata, Uint8 *stream, int len) {

  SimPlay *is = (SimPlay *)userdata;
  int len1, audio_size;
  double pts;

  while(len > 0) {
    if(is->audio_buf_index >= is->audio_buf_size) {
      /* We have already sent all our data; get more */
      audio_size = audio_decode_frame(is, is->audio_buf, sizeof(is->audio_buf), &pts);
      if(audio_size < 0) {
	/* If error, output silence */
	is->audio_buf_size = 1024;
	memset(is->audio_buf, 0, is->audio_buf_size);
      } else {
	audio_size = synchronize_audio(is, (int16_t *)is->audio_buf,
				       audio_size, pts);
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

void video_refresh_timer(void *userdata) {

  SimPlay *is = (SimPlay *)userdata;
  VideoPicture *vp;
  double actual_delay, delay, sync_threshold, ref_clock, diff;
  
  if(is->video_st) {
    if(is->pictq_size == 0) {
      schedule_refresh(is, 1);
    } else {
      vp = &is->pictq[is->pictq_rindex];
      
      is->video_current_pts = vp->pts;
      is->video_current_pts_time = av_gettime();
      delay = vp->pts - is->frame_last_pts; /* the pts from last time */
      if(delay <= 0 || delay >= 1.0) {
	/* if incorrect delay, use previous one */
	delay = is->frame_last_delay;
      }
      /* save for next time */
      is->frame_last_delay = delay;
      is->frame_last_pts = vp->pts;



      /* update delay to sync to audio if not master source */
      if(is->av_sync_type != AV_SYNC_VIDEO_MASTER) {
	ref_clock = get_master_clock(is);
	diff = vp->pts - ref_clock;
	
	/* Skip or repeat the frame. Take delay into account
	   FFPlay still doesn't "know if this is the best guess." */
	sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
	if(fabs(diff) < AV_NOSYNC_THRESHOLD) {
	  if(diff <= -sync_threshold) {
	    delay = 0;
	  } else if(diff >= sync_threshold) {
	    delay = 2 * delay;
	  }
	}
      }
      is->frame_timer += delay;
      /* computer the REAL delay */
      actual_delay = is->frame_timer - (av_gettime() / 1000000.0);
      if(actual_delay < 0.010) {
	/* Really it should skip the picture instead */
	actual_delay = 0.010;
      }
      schedule_refresh(is, (int)(actual_delay * 1000 + 0.5));
      
      /* show the picture! */
      splayWindowDisplay(is);
      
      /* update queue for next picture! */
      if(++is->pictq_rindex == SIMPLAY_VIDEO_PICTURE_QUEUE_SIZE) {
	is->pictq_rindex = 0;
      }
      SDL_LockMutex(is->pictq_mutex);
      is->pictq_size--;
      SDL_CondSignal(is->pictq_cond);
      SDL_UnlockMutex(is->pictq_mutex);
    }
  } else {
    schedule_refresh(is, 100);
  }
}

double simPlayerSynchronizeVideo(SimPlay *is, AVFrame *src_frame, double pts) {

  double frame_delay;

  if(pts != 0) {
    /* if we have pts, set video clock to it */
    is->video_clock = pts;
  } else {
    /* if we aren't given a pts, set it to the clock */
    pts = is->video_clock;
  }
  /* update the video clock */
  frame_delay = av_q2d(is->video_ctx->time_base);
  /* if we are repeating a frame, adjust clock accordingly */
  frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
  is->video_clock += frame_delay;
  return pts;
}

int video_thread(void *arg) {
  SimPlay *is = (SimPlay *)arg;
  AVPacket pkt1, *packet = &pkt1;
  int frameFinished;
  AVFrame *pFrame;
  double pts;

  pFrame = av_frame_alloc();

  for(;;) {
    if(splayQueueGet(&is->videoq, packet, 1) < 0) {
      // means we quit getting packets
      break;
    }
    if(splayQueueGet(&is->videoq, packet, 1) < 0) {
      // means we quit getting packets
      break;
    }
    pts = 0;

    // Decode video frame
    avcodec_decode_video2(is->video_ctx, pFrame, &frameFinished, packet);

    if((pts = av_frame_get_best_effort_timestamp(pFrame)) == AV_NOPTS_VALUE) {
      pts = av_frame_get_best_effort_timestamp(pFrame);
    } else {
      pts = 0;
    }
    pts *= av_q2d(is->video_st->time_base);

    // Did we get a video frame?
    if(frameFinished) {
      pts = simPlayerSynchronizeVideo(is, pFrame, pts);
      if(queue_picture(is, pFrame, pts) < 0) {
	break;
      }
    }
    av_free_packet(packet);
  }
  av_frame_free(&pFrame);
  return 0;
}

int stream_component_open(SimPlay *is, int stream_index) {

  AVFormatContext *pFormatCtx = is->pFormatCtx;
  AVCodecContext *codecCtx = NULL;
  AVCodec *codec = NULL;
  SDL_AudioSpec wanted_spec, spec;

  if(stream_index < 0 || stream_index >= pFormatCtx->nb_streams) {
    return -1;
  }

  codec = avcodec_find_decoder(pFormatCtx->streams[stream_index]->codec->codec_id);
  if(!codec) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1;
  }

  codecCtx = avcodec_alloc_context3(codec);
  if(avcodec_copy_context(codecCtx, pFormatCtx->streams[stream_index]->codec) != 0) {
    fprintf(stderr, "Couldn't copy codec context");
    return -1; // Error copying codec context
  }


  if(codecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
    // Set audio settings from codec info
    wanted_spec.freq = codecCtx->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = codecCtx->channels;
    wanted_spec.silence = 0;
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = is;
    
    if(SDL_OpenAudio(&wanted_spec, &spec) < 0) {
      fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
      return -1;
    }
    is->audio_hw_buf_size = spec.size;
  }
  if(avcodec_open2(codecCtx, codec, NULL) < 0) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1;
  }

  switch(codecCtx->codec_type) {
  case AVMEDIA_TYPE_AUDIO:
    is->audioStream = stream_index;
    is->audio_st = pFormatCtx->streams[stream_index];
    is->audio_ctx = codecCtx;
    is->audio_buf_size = 0;
    is->audio_buf_index = 0;
    memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
    packet_queue_init(&is->audioq);
    SDL_PauseAudio(0);
    break;
  case AVMEDIA_TYPE_VIDEO:
    is->videoStream = stream_index;
    is->video_st = pFormatCtx->streams[stream_index];
    is->video_ctx = codecCtx;

    is->frame_timer = (double)av_gettime() / 1000000.0;
    is->frame_last_delay = 40e-3;
    is->video_current_pts_time = av_gettime();

    packet_queue_init(&is->videoq);
    is->video_tid = SDL_CreateThread(video_thread, is);
    is->sws_ctx = sws_getContext(is->video_ctx->width, is->video_ctx->height,
				 is->video_ctx->pix_fmt, is->video_ctx->width,
				 is->video_ctx->height, PIX_FMT_YUV420P,
				 SWS_BILINEAR, NULL, NULL, NULL
				 );
    break;
  default:
    break;
  }
}

int decode_thread(void *arg) {

  SimPlay *is = (SimPlay *)arg;
  AVFormatContext *pFormatCtx;
  AVPacket pkt1, *packet = &pkt1;

  int video_index = -1;
  int audio_index = -1;
  int i;

  is->videoStream=-1;
  is->audioStream=-1;

  global_video_state = is;

  // Open video file
  if(avformat_open_input(&pFormatCtx, is->filename, NULL, NULL)!=0)
    return -1; // Couldn't open file

  is->pFormatCtx = pFormatCtx;
  
  // Retrieve stream information
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    return -1; // Couldn't find stream information
  
  // Dump information about file onto standard error
  av_dump_format(pFormatCtx, 0, is->filename, 0);
  
  // Find the first video stream

  for(i=0; i<pFormatCtx->nb_streams; i++) {
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO &&
       video_index < 0) {
      video_index=i;
    }
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO &&
       audio_index < 0) {
      audio_index=i;
    }
  }
  if(audio_index >= 0) {
    stream_component_open(is, audio_index);
  }
  if(video_index >= 0) {
    stream_component_open(is, video_index);
  }   

  if(is->videoStream < 0 || is->audioStream < 0) {
    fprintf(stderr, "%s: could not open codecs\n", is->filename);
    goto fail;
  }

  // main decode loop

  for(;;) {
    if(is->quit) {
      break;
    }
    // seek stuff goes here
    if(is->seek_req) {
      int stream_index= -1;
      int64_t seek_target = is->seek_pos;

      if     (is->videoStream >= 0) stream_index = is->videoStream;
      else if(is->audioStream >= 0) stream_index = is->audioStream;

      if(stream_index>=0){
	seek_target= av_rescale_q(seek_target, AV_TIME_BASE_Q,
				  pFormatCtx->streams[stream_index]->time_base);
      }
      if(av_seek_frame(is->pFormatCtx, stream_index, 
		       seek_target, is->seek_flags) < 0) {
	fprintf(stderr, "%s: error while seeking\n",
		is->pFormatCtx->filename);
      } else {

	if(is->audioStream >= 0) {
	  sPlayQueueFlush(&is->audioq);
	  splayQueuePut(&is->audioq, &flush_pkt);
	}
	if(is->videoStream >= 0) {
	  sPlayQueueFlush(&is->videoq);
	  splayQueuePut(&is->videoq, &flush_pkt);
	}
      }
      is->seek_req = 0;
    }

    if(is->audioq.size > MAX_AUDIOQ_SIZE ||
       is->videoq.size > MAX_VIDEOQ_SIZE) {
      SDL_Delay(10);
      continue;
    }
    if(av_read_frame(is->pFormatCtx, packet) < 0) {
      if(is->pFormatCtx->pb->error == 0) {
	SDL_Delay(100); /* no error; wait for user input */
	continue;
      } else {
	break;
      }
    }
    // Is this a packet from the video stream?
    if(packet->stream_index == is->videoStream) {
      splayQueuePut(&is->videoq, packet);
    } else if(packet->stream_index == is->audioStream) {
      splayQueuePut(&is->audioq, packet);
    } else {
      av_free_packet(packet);
    }
  }
  /* all done - wait for it */
  while(!is->quit) {
    SDL_Delay(100);
  }

 fail:
  if(1){
    SDL_Event event;
    event.type = FF_QUIT_EVENT;
    event.user.data1 = is;
    SDL_PushEvent(&event);
  }
  return 0;
}

void stream_seek(SimPlay *is, int64_t pos, int rel) {

  if(!is->seek_req) {
    is->seek_pos = pos;
    is->seek_flags = rel < 0 ? AVSEEK_FLAG_BACKWARD : 0;
    is->seek_req = 1;
  }
}

int main(int argc, char *argv[]) {

  SDL_Event       event;

  SimPlay      *is;

  is = av_mallocz(sizeof(SimPlay));

  if(argc < 2) {
    fprintf(stderr, "Usage: test <file>\n");
    exit(1);
  }
  // Register all formats and codecs
  av_register_all();
  
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
    exit(1);
  }

  // Make a screen to put our video
#ifndef __DARWIN__
        screen = SDL_SetVideoMode(640, 480, 0, 0);
#else
        screen = SDL_SetVideoMode(640, 480, 24, 0);
#endif
  if(!screen) {
    fprintf(stderr, "SDL: could not set video mode - exiting\n");
    exit(1);
  }

  screen_mutex = SDL_CreateMutex();

  av_strlcpy(is->filename, argv[1], sizeof(is->filename));

  is->pictq_mutex = SDL_CreateMutex();
  is->pictq_cond = SDL_CreateCond();

  schedule_refresh(is, 40);

  is->av_sync_type = DEFAULT_AV_SYNC_TYPE;
  is->parse_tid = SDL_CreateThread(decode_thread, is);
  if(!is->parse_tid) {
    av_free(is);
    return -1;
  }

  av_init_packet(&flush_pkt);
  flush_pkt.data = "FLUSH";

  for(;;) {
    double incr, pos;
    SDL_WaitEvent(&event);
    switch(event.type) {
    case SDL_KEYDOWN:
      switch(event.key.keysym.sym) {
      case SDLK_LEFT:
	incr = -10.0;
	goto do_seek;
      case SDLK_RIGHT:
	incr = 10.0;
	goto do_seek;
      case SDLK_UP:
	incr = 60.0;
	goto do_seek;
      case SDLK_DOWN:
	incr = -60.0;
	goto do_seek;
      do_seek:
	if(global_video_state) {
	  pos = get_master_clock(global_video_state);
	  pos += incr;
	  stream_seek(global_video_state, (int64_t)(pos * AV_TIME_BASE), incr);
	}
	break;
      default:
	break;
      }
      break;
    case FF_QUIT_EVENT:
    case SDL_QUIT:
      is->quit = 1;
      /*
       * If the video has finished playing, then both the picture and
       * audio queues are waiting for more data.  Make them stop
       * waiting and terminate normally.
       */
      SDL_CondSignal(is->audioq.cond);
      SDL_CondSignal(is->videoq.cond);
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

