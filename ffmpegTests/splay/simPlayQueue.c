
#include "_simPlay.h"

void splayQueueInit(PacketQueue *q, SimPlay *sPlay, const char *name)
{
	memset(q, 0, sizeof(PacketQueue));
	q->mutex = SDL_CreateMutex();
	q->cond = SDL_CreateCond();

	q->sPlay = sPlay;
	memcpy(q->name, name, strlen(name));
}


int splayQueuePut(PacketQueue *q, AVPacket *pkt)
{
	AVPacketList *pktList;
	int ret;
	
	pktList = av_mallocz(sizeof(AVPacketList));
	if (!pktList)
		return -1;

	if ((ret = av_copy_packet(&pktList->pkt, pkt)) < 0)
	{
		fprintf(stderr, "Add packet into queue failed:%s\n", q->name);
		av_freep(&pktList);
		return ret;
	}

	SDL_LockMutex(q->mutex);

	if (!q->last_pkt)
		q->first_pkt = pktList;
	else
		q->last_pkt->next = pktList;

	q->last_pkt = pktList;
	q->nb_packets++;
	q->size += pktList->pkt.size;
#if SIM_PLAY_OPTION_DEBUG_AUDIO
	fprintf(stderr, "Add packet with length of %d bytes into queue '%s' with length of %d\n", pkt->size, q->name, q->nb_packets);
#endif

	SDL_CondSignal(q->cond);
	SDL_UnlockMutex(q->mutex);
	
	return 0;
}


int splayQueueGet(PacketQueue *q, AVPacket *pkt, int block)
{
	AVPacketList *pkt1;
	int ret = 0;

	SDL_LockMutex(q->mutex);

	for(;;)
	{
#if 0	
		if(!q->sPlay)
		{
			fprintf(stderr, "NULL PLAY object\n");
			exit(1);
		}
#endif		
		if(q->sPlay->quit)
		{
			ret = -1;
			break;
		}

#if SIM_PLAY_OPTION_DEBUG_AUDIO
		fprintf(stderr, "Length of audio queue '%s' is %d\n", q->name, q->nb_packets);
#endif
		pkt1 = q->first_pkt;
		if (pkt1)
		{

			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
			{
				q->last_pkt = NULL;
			}	
			
			q->nb_packets--;
			q->size -= pkt1->pkt.size;

			if ((ret = av_copy_packet(pkt, &pkt1->pkt)) < 0)
			{
				av_log(NULL, AV_LOG_ERROR, "Copy AvPacket from queue '%s' failed: %s\n", q->name, av_err2str(ret) );
				return -1;
			}

			av_packet_unref(&pkt1->pkt);

			av_freep(&pkt1);
			ret = 1;
			break;
		}
		else if (!block)
		{
			ret = 0;
			break;
		}
		else
		{
			SDL_CondWait(q->cond, q->mutex);
		}
	}
	
	SDL_UnlockMutex(q->mutex);
	return ret;
}

void sPlayQueueFlush(PacketQueue *q)
{
	AVPacketList *pkt, *pkt1;

	SDL_LockMutex(q->mutex);
	for(pkt = q->first_pkt; pkt != NULL; pkt = pkt1)
	{
		pkt1 = pkt->next;
//		av_free_packet(&pkt->pkt);
		av_packet_unref(&pkt->pkt);
		av_freep(&pkt);
	}
	
	q->last_pkt = NULL;
	q->first_pkt = NULL;
	q->nb_packets = 0;
	q->size = 0;
	SDL_UnlockMutex(q->mutex);
}


static void _alloc_picture(void *userdata)
{
	SimPlay *is = (SimPlay *)userdata;
	VideoPicture *vp;

	vp = &is->pictq[is->pictq_windex];
	if(vp->bmp)
	{// we already have one make another, bigger/smaller
#if FFMPEG_NEW_API
		av_freep(&vp->data);
		SDL_DestroyTexture( vp->bmp);
#else		
		SDL_FreeYUVOverlay(vp->bmp);
#endif
	}
	
	// Allocate a place to put our YUV image on that screen
	SDL_LockMutex(is->window.screen_mutex);

#if FFMPEG_NEW_API
	av_image_alloc(vp->data, vp->linesizes, is->video_ctx->width, is->video_ctx->height, AV_PIX_FMT_YUV420P, 1);
	vp->bmp = SDL_CreateTexture( is->window.renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, is->video_ctx->width, is->video_ctx->height );
#else
	vp->bmp = SDL_CreateYUVOverlay(is->video_ctx->width, is->video_ctx->height, SDL_YV12_OVERLAY, is->window.screen);
#endif
	SDL_UnlockMutex(is->window.screen_mutex);

	vp->width = is->video_ctx->width;
	vp->height = is->video_ctx->height;
	vp->allocated = 1;
}


int splayQueuePutPicture(SimPlay *is, AVFrame *pFrame, double pts)
{
	VideoPicture *vp;
#if	FFMPEG_NEW_API
#else
	int dst_pix_fmt;
	AVPicture pict;
#endif
	/* wait until we have space for a new pic */
	SDL_LockMutex(is->pictq_mutex);
	
	while(is->pictq_size >= SIMPLAY_VIDEO_PICTURE_QUEUE_SIZE && ! is->quit)
	{
		SDL_CondWait(is->pictq_cond, is->pictq_mutex);
	}
	SDL_UnlockMutex(is->pictq_mutex);

	if(is->quit)
		return -1;

	// windex is set to 0 initially
	vp = &is->pictq[is->pictq_windex];

	/* allocate or resize the buffer! */
	if(!vp->bmp ||vp->width != is->video_ctx->width ||vp->height != is->video_ctx->height)
	{
//		SDL_Event event;

		vp->allocated = 0;
		_alloc_picture(is);
		if(is->quit)
		{
			return -1;
		}
	}

	/* We have a place to put our picture on the queue */
	if(vp->bmp)
	{
		vp->pts = pts;
#if	FFMPEG_NEW_API
		sws_scale(is->sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0,  is->video_ctx->height, vp->data, vp->linesizes);
#else
		SDL_LockYUVOverlay(vp->bmp);

		dst_pix_fmt = AV_PIX_FMT_YUV420P;
		/* point pict at the queue */

		pict.data[0] = vp->bmp->pixels[0];
		pict.data[1] = vp->bmp->pixels[2];
		pict.data[2] = vp->bmp->pixels[1];

		pict.linesize[0] = vp->bmp->pitches[0];
		pict.linesize[1] = vp->bmp->pitches[2];
		pict.linesize[2] = vp->bmp->pitches[1];

		// Convert the image into YUV format that SDL uses
		sws_scale(is->sws_ctx, (uint8_t const * const *)pFrame->data,  pFrame->linesize, 0, is->video_ctx->height, pict.data, pict.linesize);

		SDL_UnlockYUVOverlay(vp->bmp);
#endif

		/* now we inform our display thread that we have a pic ready */
		if(++is->pictq_windex == SIMPLAY_VIDEO_PICTURE_QUEUE_SIZE)
		{
			is->pictq_windex = 0;
		}
		
		SDL_LockMutex(is->pictq_mutex);
		is->pictq_size++;
		SDL_UnlockMutex(is->pictq_mutex);
	}
	
	return 0;
}


