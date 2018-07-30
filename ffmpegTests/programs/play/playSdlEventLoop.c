
#include "play.h"

/* call refresh video */
static void _refresh_loop_wait_event(VideoState *is, SDL_Event *event)
{
	double remaining_time = 0.0;
	SDL_PumpEvents();
	
	while (!SDL_PeepEvents(event, 1/*number of events*/, SDL_GETEVENT /*action*/, SDL_FIRSTEVENT, SDL_LASTEVENT))
	{/* no SDL event, then loop in this while */
		if (!cursor_hidden && av_gettime_relative() - cursor_last_shown > CURSOR_HIDE_DELAY)
		{
			SDL_ShowCursor(0);
			cursor_hidden = 1;
		}
		
		if (remaining_time > 0.0)
			av_usleep((int64_t)(remaining_time * 1000000.0));
		
		remaining_time = REFRESH_RATE;		/* set default value of 10 ms */
		if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
			playVideoRefresh(is, &remaining_time);
		
		SDL_PumpEvents();
	}
}

static void _toggle_pause(VideoState *is)
{
	stream_toggle_pause(is);
	is->step = 0;
}

static void _toggle_mute(VideoState *is)
{
	is->muted = !is->muted;
}

static void _update_volume(VideoState *is, int sign, double step)
{
	double volume_level = is->audio_volume ? (20 * log(is->audio_volume / (double)SDL_MIX_MAXVOLUME) / log(10)) : -1000.0;
	int new_volume = lrint(SDL_MIX_MAXVOLUME * pow(10.0, (volume_level + sign * step) / 20.0));
	is->audio_volume = av_clip(is->audio_volume == new_volume ? (is->audio_volume + sign) : new_volume, 0, SDL_MIX_MAXVOLUME);
}


static void _stream_cycle_channel(VideoState *is, int codec_type)
{
	AVFormatContext *ic = is->ic;
	int start_index, stream_index;
	int old_index;
	AVStream *st;
	AVProgram *p = NULL;
	int nb_streams = is->ic->nb_streams;

	if (codec_type == AVMEDIA_TYPE_VIDEO)
	{
		start_index = is->last_video_stream;
		old_index = is->video_stream;
	}
	else if (codec_type == AVMEDIA_TYPE_AUDIO)
	{
		start_index = is->last_audio_stream;
		old_index = is->audio_stream;
	}
	else
	{
		start_index = is->last_subtitle_stream;
		old_index = is->subtitle_stream;
	}
	stream_index = start_index;

	if (codec_type != AVMEDIA_TYPE_VIDEO && is->video_stream != -1)
	{
		p = av_find_program_from_stream(ic, NULL, is->video_stream);
		if (p)
		{
			nb_streams = p->nb_stream_indexes;
			for (start_index = 0; start_index < nb_streams; start_index++)
				if (p->stream_index[start_index] == stream_index)
					break;
				
			if (start_index == nb_streams)
				start_index = -1;
			
			stream_index = start_index;
		}
	}

	for (;;)
	{
		if (++stream_index >= nb_streams)
		{
			if (codec_type == AVMEDIA_TYPE_SUBTITLE)
			{
				stream_index = -1;
				is->last_subtitle_stream = -1;
				goto the_end;
			}
			
			if (start_index == -1)
				return;
			
			stream_index = 0;
		}
		
		if (stream_index == start_index)
			return;
		
		st = is->ic->streams[p ? p->stream_index[stream_index] : stream_index];
		if (st->codecpar->codec_type == codec_type)
		{
			/* check that parameters are OK */
			switch (codec_type)
			{
				case AVMEDIA_TYPE_AUDIO:
					if (st->codecpar->sample_rate != 0 &&
					st->codecpar->channels != 0)
					goto the_end;
					break;
				
				case AVMEDIA_TYPE_VIDEO:
				case AVMEDIA_TYPE_SUBTITLE:
					goto the_end;
				
				default:
					break;
			}
		}
	}
	
the_end:
	if (p && stream_index != -1)
		stream_index = p->stream_index[stream_index];
	
	av_log(NULL, AV_LOG_INFO, "Switch %s stream from #%d to #%d\n",	av_get_media_type_string(codec_type), old_index, stream_index);

	playStreamComponentClose(is, old_index);
	playStreamComponentOpen(is, stream_index);
}

static void _toggle_full_screen(VideoState *is)
{
	is_full_screen = !is_full_screen;
	SDL_SetWindowFullscreen(window, is_full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

static void _toggle_audio_display(VideoState *is)
{
	int next = is->show_mode;
	
	do
	{
		next = (next + 1) % SHOW_MODE_NB;
	} while (next != is->show_mode && (next == SHOW_MODE_VIDEO && !is->video_st || next != SHOW_MODE_VIDEO && !is->audio_st));
	
	if (is->show_mode != next)
	{
		is->force_refresh = 1;
		is->show_mode = next;
	}
}

static void _seek_chapter(VideoState *is, int incr)
{
	int64_t pos = playClockGetMaster(is) * AV_TIME_BASE;
	int i;

	if (!is->ic->nb_chapters)
		return;

	/* find the current chapter */
	for (i = 0; i < is->ic->nb_chapters; i++)
	{
		AVChapter *ch = is->ic->chapters[i];
		if (av_compare_ts(pos, AV_TIME_BASE_Q, ch->start, ch->time_base) < 0)
		{
			i--;
			break;
		}
	}

	i += incr;
	i = FFMAX(i, 0);
	if (i >= is->ic->nb_chapters)
		return;

	av_log(NULL, AV_LOG_VERBOSE, "Seeking to chapter %d.\n", i);
	playStreamSeek(is, av_rescale_q(is->ic->chapters[i]->start, is->ic->chapters[i]->time_base, AV_TIME_BASE_Q), 0, 0);
}


/* handle an event sent by the GUI */
void playSdlEventLoop(VideoState *cur_stream)
{
	SDL_Event event;
	double incr, pos, frac;

	for (;;)
	{
		double x;
		_refresh_loop_wait_event(cur_stream, &event);

		switch (event.type)
		{
			case SDL_KEYDOWN:
				if (exit_on_keydown)
				{
					do_exit(cur_stream);
					break;
				}

			/* keyboard event */
			switch (event.key.keysym.sym)
			{
				case SDLK_ESCAPE:
				case SDLK_q:
					do_exit(cur_stream);
					break;
				
				case SDLK_f:
					_toggle_full_screen(cur_stream);
					cur_stream->force_refresh = 1;
					break;
				
				case SDLK_p:
				case SDLK_SPACE:
					_toggle_pause(cur_stream);
					break;
				
				case SDLK_m:
					_toggle_mute(cur_stream);
					break;
				
				case SDLK_KP_MULTIPLY:
				case SDLK_0:
					_update_volume(cur_stream, 1, SDL_VOLUME_STEP);
					break;
				
				case SDLK_KP_DIVIDE:
				case SDLK_9:
					_update_volume(cur_stream, -1, SDL_VOLUME_STEP);
					break;
				
				case SDLK_s: // S: Step to next frame
					step_to_next_frame(cur_stream);
					break;
				
				case SDLK_a:
					_stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
					break;
				
				case SDLK_v:
					_stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
					break;
				
				case SDLK_c:
					_stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
					_stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
					_stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
					break;
				
				case SDLK_t:
					_stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
					break;
				
				case SDLK_w:
#if CONFIG_AVFILTER
					if (cur_stream->show_mode == SHOW_MODE_VIDEO && cur_stream->vfilter_idx < nb_vfilters - 1)
					{
						if (++cur_stream->vfilter_idx >= nb_vfilters)
							cur_stream->vfilter_idx = 0;
					}
					else
					{
						cur_stream->vfilter_idx = 0;
						_toggle_audio_display(cur_stream);
					}
#else
					_toggle_audio_display(cur_stream);
#endif
					break;

				case SDLK_PAGEUP:
					if (cur_stream->ic->nb_chapters <= 1)
					{
						incr = 600.0;
						goto do_seek;
					}
					_seek_chapter(cur_stream, 1);
					break;
				
				case SDLK_PAGEDOWN:
					if (cur_stream->ic->nb_chapters <= 1)
					{
						incr = -600.0;
						goto do_seek;
					}
					_seek_chapter(cur_stream, -1);
					break;
				
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
do_seek:
					if (seek_by_bytes)
					{
						pos = -1;
						if (pos < 0 && cur_stream->video_stream >= 0)
							pos = playFrameQueueLastPos(&cur_stream->videoFrameQueue);
						if (pos < 0 && cur_stream->audio_stream >= 0)
							pos = playFrameQueueLastPos(&cur_stream->audioFrameQueue);
						if (pos < 0)
							pos = avio_tell(cur_stream->ic->pb);
						
						if (cur_stream->ic->bit_rate)
							incr *= cur_stream->ic->bit_rate / 8.0;
						else
							incr *= 180000.0;
						
						pos += incr;
						playStreamSeek(cur_stream, pos, incr, 1);
					}
					else
					{
						pos = playClockGetMaster(cur_stream);
						if (isnan(pos))
							pos = (double)cur_stream->seek_pos / AV_TIME_BASE;
						
						pos += incr;
						if (cur_stream->ic->start_time != AV_NOPTS_VALUE && pos < cur_stream->ic->start_time / (double)AV_TIME_BASE)
							pos = cur_stream->ic->start_time / (double)AV_TIME_BASE;
						
						playStreamSeek(cur_stream, (int64_t)(pos * AV_TIME_BASE), (int64_t)(incr * AV_TIME_BASE), 0);
					}
					break;
				
				default:
					break;
			}
			break;

			/* mouse event */
			case SDL_MOUSEBUTTONDOWN:
				if (exit_on_mousedown)
				{
					do_exit(cur_stream);
					break;
				}
				
				if (event.button.button == SDL_BUTTON_LEFT)
				{
					static int64_t last_mouse_left_click = 0;
					if (av_gettime_relative() - last_mouse_left_click <= 500000)
					{
						_toggle_full_screen(cur_stream);
						cur_stream->force_refresh = 1;
						last_mouse_left_click = 0;
					}
					else
					{
						last_mouse_left_click = av_gettime_relative();
					}
				}
			
			case SDL_MOUSEMOTION:
				if (cursor_hidden)
				{
					SDL_ShowCursor(1);
					cursor_hidden = 0;
				}
				
				cursor_last_shown = av_gettime_relative();
				if (event.type == SDL_MOUSEBUTTONDOWN)
				{
					if (event.button.button != SDL_BUTTON_RIGHT)
						break;
					x = event.button.x;
				}
				else
				{
					if (!(event.motion.state & SDL_BUTTON_RMASK))
						break;
					x = event.motion.x;
				}
				
				if (seek_by_bytes || cur_stream->ic->duration <= 0)
				{
					uint64_t size =  avio_size(cur_stream->ic->pb);
					playStreamSeek(cur_stream, size*x/cur_stream->width, 0, 1);
				}
				else
				{
					int64_t ts;
					int ns, hh, mm, ss;
					int tns, thh, tmm, tss;
					tns  = cur_stream->ic->duration / 1000000LL;
					thh  = tns / 3600;
					tmm  = (tns % 3600) / 60;
					tss  = (tns % 60);
					frac = x / cur_stream->width;
					ns   = frac * tns;
					hh   = ns / 3600;
					mm   = (ns % 3600) / 60;
					ss   = (ns % 60);
					
					av_log(NULL, AV_LOG_INFO, "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n", frac*100,
								hh, mm, ss, thh, tmm, tss);
					ts = frac * cur_stream->ic->duration;
					if (cur_stream->ic->start_time != AV_NOPTS_VALUE)
						ts += cur_stream->ic->start_time;
					playStreamSeek(cur_stream, ts, 0, 0);
				}
				break;
			
			case SDL_WINDOWEVENT:
				
				switch (event.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
						screen_width  = cur_stream->width  = event.window.data1;
						screen_height = cur_stream->height = event.window.data2;
						if (cur_stream->vis_texture)
						{
							SDL_DestroyTexture(cur_stream->vis_texture);
							cur_stream->vis_texture = NULL;
						}
					
					case SDL_WINDOWEVENT_EXPOSED:
						cur_stream->force_refresh = 1;
				}
				break;
			
			case SDL_QUIT:
			case FF_QUIT_EVENT:
				do_exit(cur_stream);
				break;

			default:
				break;
		}
	}
}

void step_to_next_frame(VideoState *is)
{
	/* if the stream is paused unpause it, then step */
	if (is->paused)
		stream_toggle_pause(is);
	is->step = 1;
}


