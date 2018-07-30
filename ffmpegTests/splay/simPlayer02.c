// tutorial02.c
// A pedagogical video player that will stream through every video frame as fast as it can.

// Run using
// tutorial02 myvideofile.mpg
// to play the video stream on your screen.

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <SDL.h>
#include <SDL_thread.h>

#include <stdio.h>

#include "_simPlay.h"

int main(int argc, char *argv[])
{
	AVFormatContext *pFormatCtx = NULL;
	int videoStream;
	unsigned i;
	AVCodecContext *pCodecCtxOrig = NULL;
	AVCodecContext *pCodecCtx = NULL;
	AVCodec *pCodec = NULL;
	AVFrame *pFrame = NULL;
	AVPacket packet;
	struct SwsContext *sws_ctx = NULL;
	
	SDL_Event event;
	SDL_Window *screen;
	SDL_Renderer *renderer;
	SDL_Texture *texture;

	enum AVCodecID     codec_id;
#if FFMPEG_NEW_API
	uint8_t *pointers[4];
	int linesizes[4];
#else
	int frameFinished;
	uint8 *yPlane, *uPlane, *vPlane;
	size_t yPlaneSz, uvPlaneSz;
#endif	
	int uvPitch;

	int64_t startTime;


	int	ret;

	if (argc < 2)
	{
		fprintf(stderr, "Usage: test <file>\n");
		exit(1);
	}

	av_register_all();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
		return -1;

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
		return -1;

	av_dump_format(pFormatCtx, 0, argv[1], 0);

	videoStream = -1;
	for (i = 0; i < pFormatCtx->nb_streams; i++)
#if FFMPEG_NEW_API
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
#else
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
#endif
		{
			videoStream = i;
			break;
		}
		
	if (videoStream == -1)
		return -1;

#if FFMPEG_NEW_API
	codec_id = pFormatCtx->streams[videoStream]->codecpar->codec_id;
#else
	codec_id = pCodecCtxOrig->codec_id;
#endif

	pCodec = avcodec_find_decoder(codec_id);
	if (pCodec == NULL)
	{
		fprintf(stderr, "Unsupported codec!\n");
		return -1;
	}
	pCodecCtx = avcodec_alloc_context3(pCodec);

#if FFMPEG_NEW_API
	ret = avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar );
#else
	ret = avcodec_copy_context(pCodecCtx, pCodecCtxOrig);
#endif
	if(ret < 0)
	{
		fprintf(stderr, "Couldn't copy codec context : %s", av_err2str(ret));
		return -1;
	}

	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
		return -1;

	// Allocate video frame
	pFrame = av_frame_alloc();

	// Make a screen to put our video
	screen = SDL_CreateWindow("FFmpeg Tutorial", SDL_WINDOWPOS_UNDEFINED,  SDL_WINDOWPOS_UNDEFINED, pCodecCtx->width, pCodecCtx->height, 0 );
	if (!screen)
	{
		fprintf(stderr, "SDL: could not create window - exiting\n");
		exit(1);
	}

	renderer = SDL_CreateRenderer(screen, -1, 0);
	if (!renderer)
	{
		fprintf(stderr, "SDL: could not create renderer - exiting\n");
		exit(1);
	}

	// Allocate a place to put our YUV image on that screen
	texture = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height );
	if (!texture)
	{
		fprintf(stderr, "SDL: could not create texture - exiting\n");
		exit(1);
	}

	// initialize SWS context for software scaling
	sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, 
		AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);

	// set up YV12 pixel array (12 bits per pixel)
#if	FFMPEG_NEW_API
	ret= av_image_alloc(pointers, linesizes, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, 1);
#else	
	yPlaneSz = pCodecCtx->width * pCodecCtx->height;
	uvPlaneSz = pCodecCtx->width * pCodecCtx->height / 4;
	yPlane = (Uint8*)malloc(yPlaneSz);
	uPlane = (Uint8*)malloc(uvPlaneSz);
	vPlane = (Uint8*)malloc(uvPlaneSz);
	if (!yPlane || !uPlane || !vPlane)
	{
		fprintf(stderr, "Could not allocate pixel buffers - exiting\n");
		exit(1);
	}
#endif

	uvPitch = pCodecCtx->width / 2;
	startTime = av_gettime();
	while (av_read_frame(pFormatCtx, &packet) >= 0)
	{
		// Is this a packet from the video stream?
		if (packet.stream_index == videoStream)
		{
		
			// Decode video frame
#if FFMPEG_NEW_API
			ret = avcodec_send_packet(pCodecCtx, &packet);
			if(ret != 0)
			{
				continue;
			}

			ret = avcodec_receive_frame(pCodecCtx, pFrame);
			if( ret != 0)
			{
				continue;
			}
#else			
			avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
			// Did we get a video frame?
			if (frameFinished)
#endif
			{

#if FFMPEG_NEW_API
				// Convert the image into YUV format that SDL uses
				sws_scale(sws_ctx, (uint8_t const * const *) pFrame->data, pFrame->linesize, 0, pCodecCtx->height,  pointers, linesizes);

				/* update Y, U, and V and their length respectively */
				SDL_UpdateYUVTexture(
				        texture,
				        NULL,
				        pointers[0],
				        pCodecCtx->width,
				        pointers[1],
				        uvPitch,
				        pointers[2],
				        uvPitch
				    );
#else
				AVPicture pict;
				pict.data[0] = yPlane;
				pict.data[1] = uPlane;
				pict.data[2] = vPlane;
				pict.linesize[0] = pCodecCtx->width;
				pict.linesize[1] = uvPitch;
				pict.linesize[2] = uvPitch;

				// Convert the image into YUV format that SDL uses
				sws_scale(sws_ctx, (uint8_t const * const *) pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
						pict.data, pict.linesize);

				/* update Y, U, and V and their length respectively */
				SDL_UpdateYUVTexture(
				        texture,
				        NULL,
				        yPlane,
				        pCodecCtx->width,
				        uPlane,
				        uvPitch,
				        vPlane,
				        uvPitch
				    );
#endif

				SDL_RenderClear(renderer);
				SDL_RenderCopy(renderer, texture, NULL, NULL);
				SDL_RenderPresent(renderer);

			}
		}

		// Free the packet that was allocated by av_read_frame
#if FFMPEG_NEW_API
		av_packet_unref(&packet);
#else
		av_free_packet(&packet);
#endif
		SDL_PollEvent(&event);
		switch (event.type)
		{
			case SDL_QUIT:
				SDL_DestroyTexture(texture);
				SDL_DestroyRenderer(renderer);
				SDL_DestroyWindow(screen);
				SDL_Quit();
				exit(0);
				break;

			default:
				break;
		}

	}

	av_log(NULL, AV_LOG_INFO, "Play (%lld) seconds\n", (av_gettime()-startTime)/1000000 );

	
	// Free the YUV frame
	av_frame_free(&pFrame);
#if FFMPEG_NEW_API
	av_freep(&pointers[0]);
#else
	free(yPlane);
	free(uPlane);
	free(vPlane);
#endif

	// Close the codec
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtxOrig);

	// Close the video file
	avformat_close_input(&pFormatCtx);

	return 0;
}

