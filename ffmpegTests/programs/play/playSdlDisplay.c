
#include "play.h"

SDL_Window *window;
SDL_Renderer *renderer;

/* used in displayImage and displayAudio */
static int _realloc_texture(SDL_Texture **texture, Uint32 new_format, int new_width, int new_height, SDL_BlendMode blendmode, int init_texture)
{
	Uint32 format;
	int access, w, h;
	
	if (SDL_QueryTexture(*texture, &format, &access, &w, &h) < 0 || new_width != w || new_height != h || new_format != format)
	{
		void *pixels;
		int pitch; /* length of one row */
		
		SDL_DestroyTexture(*texture);
		if (!(*texture = SDL_CreateTexture(renderer, new_format, SDL_TEXTUREACCESS_STREAMING, new_width, new_height)))
		{
			return -1;
		}

		if (SDL_SetTextureBlendMode(*texture, blendmode) < 0)
		{
			return -1;
		}
		
		if (init_texture)
		{/* clear this rect with data of zero, for subtitle */
			/* return write-only data of one rect */
			if (SDL_LockTexture(*texture, NULL, &pixels, &pitch) < 0)
				return -1;
			
			memset(pixels, 0, pitch * new_height);
			SDL_UnlockTexture(*texture);
		}
	}
	
	return 0;
}

static void _calculate_display_rect(SDL_Rect *rect,  
	int scr_xleft, int scr_ytop, int scr_width, int scr_height,
	int pic_width, int pic_height, AVRational pic_sar)
{
	float aspect_ratio;
	int width, height, x, y;

	if (pic_sar.num == 0)
		aspect_ratio = 0;
	else
		aspect_ratio = av_q2d(pic_sar);

	if (aspect_ratio <= 0.0)/* invalidate data, so keep the SAR */
		aspect_ratio = 1.0;
	
	aspect_ratio *= (float)pic_width / (float)pic_height;

	/* XXX: we suppose the screen has a 1.0 pixel ratio */
	height = scr_height;
	width = lrint(height * aspect_ratio) & ~1;
	if (width > scr_width)
	{
		width = scr_width;
		height = lrint(width / aspect_ratio) & ~1;
	}

	x = (scr_width - width) / 2;
	y = (scr_height - height) / 2;

	rect->x = scr_xleft + x;
	rect->y = scr_ytop  + y;
	rect->w = FFMAX(width,  1);
	rect->h = FFMAX(height, 1);
}


static int _upload_texture(SDL_Texture *tex, AVFrame *frame, struct SwsContext **img_convert_ctx)
{
	int ret = 0;
	
	switch (frame->format)
	{
		case AV_PIX_FMT_YUV420P:
			if (frame->linesize[0] < 0 || frame->linesize[1] < 0 || frame->linesize[2] < 0)
			{
				av_log(NULL, AV_LOG_ERROR, "Negative linesize is not supported for YUV.\n");
				return -1;
			}
			
			/* UpdateYUVTexture: data of YUV planes can be uncontinous */
			ret = SDL_UpdateYUVTexture(tex, NULL, frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2]);
			break;
		
		case AV_PIX_FMT_BGRA:
			/* UpdateTexture: data of YUV planes is continuous */
			if (frame->linesize[0] < 0)
			{/* flip vertically */
				ret = SDL_UpdateTexture(tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1), -frame->linesize[0]);
			}
			else
			{
				ret = SDL_UpdateTexture(tex, NULL, frame->data[0], frame->linesize[0]);
			}
			break;

		default:
			/* This should only happen if we are not using avfilter... */
			*img_convert_ctx = sws_getCachedContext(*img_convert_ctx, frame->width, frame->height, frame->format, frame->width, frame->height, 
				AV_PIX_FMT_BGRA/* default dest(SDL) format */, sws_flags, NULL, NULL, NULL);
			if (*img_convert_ctx != NULL)
			{
				uint8_t *pixels[4];
				int pitch[4];
				
				if (!SDL_LockTexture(tex, NULL, (void **)pixels, pitch))
				{
					sws_scale(*img_convert_ctx, (const uint8_t * const *)frame->data, frame->linesize, 0, frame->height, pixels, pitch);
					SDL_UnlockTexture(tex);
				}
			}
			else
			{
				av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
				ret = -1;
			}
			
			break;
	}
	
	return ret;
}

static void sdlDisplayImage(VideoState *is)
{
	PlayFrame *vp;
	PlayFrame *sp = NULL;
	SDL_Rect rect;

	vp = playFrameQueuePeekLast(&is->videoFrameQueue);
	if (is->subtitle_st)
	{/* display subtitle */
		if (playFrameQueueNbRemaining(&is->subtitleFrameQueue) > 0)
		{
			sp = playFrameQueuePeek(&is->subtitleFrameQueue);

			if (vp->pts >= sp->pts + ((float) sp->sub.start_display_time / 1000))
			{
				if (!sp->uploaded)
				{
					uint8_t* pixels[4];
					int pitch[4];
					int i;
					if (!sp->width || !sp->height)
					{
						sp->width = vp->width;
						sp->height = vp->height;
					}
					
					if (_realloc_texture(&is->sub_texture, SDL_PIXELFORMAT_ARGB8888, sp->width, sp->height, SDL_BLENDMODE_BLEND, 1) < 0)
						return;

					for (i = 0; i < sp->sub.num_rects; i++)
					{
						AVSubtitleRect *sub_rect = sp->sub.rects[i];

						sub_rect->x = av_clip(sub_rect->x, 0, sp->width );
						sub_rect->y = av_clip(sub_rect->y, 0, sp->height);
						sub_rect->w = av_clip(sub_rect->w, 0, sp->width  - sub_rect->x);
						sub_rect->h = av_clip(sub_rect->h, 0, sp->height - sub_rect->y);

						is->sub_convert_ctx = sws_getCachedContext(is->sub_convert_ctx,  
							sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8,
							sub_rect->w, sub_rect->h, AV_PIX_FMT_BGRA,
							0, NULL, NULL, NULL);

						if (!is->sub_convert_ctx)
						{
							av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
							return;
						}
						
						if (!SDL_LockTexture(is->sub_texture, (SDL_Rect *)sub_rect, (void **)pixels, pitch))
						{
							sws_scale(is->sub_convert_ctx, (const uint8_t * const *)sub_rect->data, sub_rect->linesize, 0, sub_rect->h, pixels, pitch);
							SDL_UnlockTexture(is->sub_texture);
						}
					}
					sp->uploaded = 1;
					
				}
			} 
			else
				sp = NULL;
		}
	}

	_calculate_display_rect(&rect, is->xleft, is->ytop, is->width, is->height, vp->width, vp->height, vp->sar);

	if (!vp->uploaded)
	{
		int sdl_pix_fmt = vp->frame->format == AV_PIX_FMT_YUV420P ? SDL_PIXELFORMAT_YV12 : SDL_PIXELFORMAT_ARGB8888;
		if (_realloc_texture(&is->vid_texture, sdl_pix_fmt, vp->frame->width, vp->frame->height, SDL_BLENDMODE_NONE, 0) < 0)
			return;
		if (_upload_texture(is->vid_texture, vp->frame, &is->img_convert_ctx) < 0)
			return;
		
		vp->uploaded = 1;
		vp->flip_v = vp->frame->linesize[0] < 0;
	}

	SDL_RenderCopyEx(renderer, is->vid_texture, NULL/* src rect */, &rect /* dest rect */, 0/* angle*/, NULL/* center point */, vp->flip_v ? SDL_FLIP_VERTICAL : 0);
	if (sp)
	{
#if USE_ONEPASS_SUBTITLE_RENDER
		SDL_RenderCopy(renderer, is->sub_texture, NULL, &rect);
#else
		int i;
		double xratio = (double)rect.w / (double)sp->width;
		double yratio = (double)rect.h / (double)sp->height;
		for (i = 0; i < sp->sub.num_rects; i++)
		{
			SDL_Rect *sub_rect = (SDL_Rect*)sp->sub.rects[i];
			SDL_Rect target = {.x = rect.x + sub_rect->x * xratio,
				   .y = rect.y + sub_rect->y * yratio,
				   .w = sub_rect->w * xratio,
				   .h = sub_rect->h * yratio};
			SDL_RenderCopy(renderer, is->sub_texture, sub_rect, &target);
		}
#endif
	}
}

static inline int _compute_mod(int a, int b)
{
	return a < 0 ? a%b + b : a%b;
}

/* used in wave display when video is not available */
static inline void _fillRectangle(int x, int y, int w, int h)
{
	SDL_Rect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	
	if (w && h)
	{
		SDL_RenderFillRect(renderer, &rect);
	}
}

/* when video show is disable and audio data is available */
static void sdlDisplayAudio(VideoState *s)
{
	int i, i_start, x, y1, y, ys, delay, n, nb_display_channels;
	int ch, channels, h, h2;
	int64_t time_diff;
	int rdft_bits, nb_freq;

	for (rdft_bits = 1; (1 << rdft_bits) < 2 * s->height; rdft_bits++)
		;
	
	nb_freq = 1 << (rdft_bits - 1);

	/* compute display index : center on currently output samples */
	channels = s->audio_tgt.channels;
	nb_display_channels = channels;
	
	if (!s->paused)
	{
		int data_used= s->show_mode == SHOW_MODE_WAVES ? s->width : (2*nb_freq);
		n = 2 * channels;
		delay = s->audio_write_buf_size;
		delay /= n;

		/* to be more precise, we take into account the time spent since
		the last buffer computation */
		if (audio_callback_time)
		{
			time_diff = av_gettime_relative() - audio_callback_time;
			delay -= (time_diff * s->audio_tgt.freq) / 1000000;
		}

		delay += 2 * data_used;
		if (delay < data_used)
			delay = data_used;

		i_start= x = _compute_mod(s->sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);
		if (s->show_mode == SHOW_MODE_WAVES)
		{
			h = INT_MIN;
			for (i = 0; i < 1000; i += channels)
			{
				int idx = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
				int a = s->sample_array[idx];
				int b = s->sample_array[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
				int c = s->sample_array[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
				int d = s->sample_array[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
				int score = a - d;

				if (h < score && (b ^ c) < 0)
				{
					h = score;
					i_start = idx;
				}
			}
		}

		s->last_i_start = i_start;
	}
	else
	{
		i_start = s->last_i_start;
	}

	if (s->show_mode == SHOW_MODE_WAVES)
	{
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

		/* total height for one channel */
		h = s->height / nb_display_channels;
		/* graph height / 2 */
		h2 = (h * 9) / 20;
		for (ch = 0; ch < nb_display_channels; ch++)
		{
			i = i_start + ch;
			y1 = s->ytop + ch * h + (h / 2); /* position of center line */

			for (x = 0; x < s->width; x++)
			{
				y = (s->sample_array[i] * h2) >> 15;
				if (y < 0)
				{
					y = -y;
					ys = y1 - y;
				}
				else
				{
					ys = y1;
				}
				
				_fillRectangle(s->xleft + x, ys, 1, y);
				i += channels;
				if (i >= SAMPLE_ARRAY_SIZE)
					i -= SAMPLE_ARRAY_SIZE;
			}
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

		for (ch = 1; ch < nb_display_channels; ch++)
		{
			y = s->ytop + ch * h;
			_fillRectangle(s->xleft, y, s->width, 1);
		}
	}
	else
	{/* Real Discrete Fourier Transforms */
		if (_realloc_texture(&s->vis_texture, SDL_PIXELFORMAT_ARGB8888, s->width, s->height, SDL_BLENDMODE_NONE, 1) < 0)
			return;

		nb_display_channels= FFMIN(nb_display_channels, 2);
		if (rdft_bits != s->rdft_bits)
		{
			av_rdft_end(s->rdft);
			av_free(s->rdft_data);
			s->rdft = av_rdft_init(rdft_bits, DFT_R2C);
			s->rdft_bits = rdft_bits;
			s->rdft_data = av_malloc_array(nb_freq, 4 *sizeof(*s->rdft_data));
		}
		
		if (!s->rdft || !s->rdft_data)
		{
			av_log(NULL, AV_LOG_ERROR, "Failed to allocate buffers for RDFT, switching to waves display\n");
			s->show_mode = SHOW_MODE_WAVES;
		}
		else
		{
			FFTSample *data[2];
			SDL_Rect rect = {.x = s->xpos, .y = 0, .w = 1, .h = s->height};
			uint32_t *pixels;
			int pitch;

			for (ch = 0; ch < nb_display_channels; ch++)
			{
				data[ch] = s->rdft_data + 2 * nb_freq * ch;
				i = i_start + ch;
				for (x = 0; x < 2 * nb_freq; x++)
				{
					double w = (x-nb_freq) * (1.0 / nb_freq);
					data[ch][x] = s->sample_array[i] * (1.0 - w * w);
					i += channels;
					
					if (i >= SAMPLE_ARRAY_SIZE)
						i -= SAMPLE_ARRAY_SIZE;
				}
				av_rdft_calc(s->rdft, data[ch]);
			}
			
			/* Least efficient way to do this, we should of course directly access it but it is more than fast enough. */
			if (!SDL_LockTexture(s->vis_texture, &rect, (void **)&pixels, &pitch))
			{
				pitch >>= 2;
				pixels += pitch * s->height;
				for (y = 0; y < s->height; y++)
				{
					double w = 1 / sqrt(nb_freq);
					int a = sqrt(w * sqrt(data[0][2 * y + 0] * data[0][2 * y + 0] + data[0][2 * y + 1] * data[0][2 * y + 1]));
					int b = (nb_display_channels == 2 ) ? sqrt(w * hypot(data[1][2 * y + 0], data[1][2 * y + 1]))
					                            : a;
					a = FFMIN(a, 255);
					b = FFMIN(b, 255);
					pixels -= pitch;
					*pixels = (a << 16) + (b << 8) + ((a+b) >> 1);
				}
				SDL_UnlockTexture(s->vis_texture);
			}
			
			SDL_RenderCopy(renderer, s->vis_texture, NULL, NULL);
		}
		
		if (!s->paused)
			s->xpos++;
		if (s->xpos >= s->width)
			s->xpos= s->xleft;
	}
}


static int _video_open(VideoState *is)
{
	int w,h;

	if (screen_width)
	{
		w = screen_width;
		h = screen_height;
	}
	else
	{
		w = default_width;
		h = default_height;
	}

	if (!window)
	{
		int flags = SDL_WINDOW_SHOWN;
		if (!window_title)
			window_title = input_filename;
		
		if (is_full_screen)
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		if (borderless)
			flags |= SDL_WINDOW_BORDERLESS;
		else
			flags |= SDL_WINDOW_RESIZABLE;
		
		window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, flags);
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
		
		if (window)
		{
			SDL_RendererInfo info;
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (!renderer)
			{
				av_log(NULL, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError());
				renderer = SDL_CreateRenderer(window, -1, 0);
			}

			if (renderer)
			{
				if (!SDL_GetRendererInfo(renderer, &info))
					av_log(NULL, AV_LOG_VERBOSE, "Initialized %s renderer.\n", info.name);
			}
		}
	}
	else
	{
		SDL_SetWindowSize(window, w, h);
	}

	if (!window || !renderer)
	{
		av_log(NULL, AV_LOG_FATAL, "SDL: could not set video mode - exiting\n");
		do_exit(is);
	}

	is->width  = w;
	is->height = h;

	return 0;
	}

/* display the current picture, if any, called in  */
void playSdlVideoDisplay(VideoState *is)
{
	if (!window)
		_video_open(is);

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	
	if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO)
		sdlDisplayAudio(is);
	else if (is->video_st)
		sdlDisplayImage(is);
	
	SDL_RenderPresent(renderer);
}

/* SAR: sample aspect ratio, come from AVFrame or CodecCtx/AVStream */
void set_default_window_size(int width, int height, AVRational sar)
{
	SDL_Rect rect;
	_calculate_display_rect(&rect, 0, 0, INT_MAX, height, width, height, sar);
	default_width  = rect.w;
	default_height = rect.h;
}


void playSdlDestory(void)
{
	if (renderer)
		SDL_DestroyRenderer(renderer);
	if (window)
		SDL_DestroyWindow(window);
}

