
#include "play.h"

/* seek in the stream */
void playStreamSeek(VideoState *is, int64_t pos, int64_t rel, int seek_by_bytes)
{
	if (!is->seek_req)
	{
		is->seek_pos = pos;
		is->seek_rel = rel;
		is->seek_flags &= ~AVSEEK_FLAG_BYTE;
		if (seek_by_bytes)
			is->seek_flags |= AVSEEK_FLAG_BYTE;
		
		is->seek_req = 1;
		SDL_CondSignal(is->continue_read_thread);
	}
}


void playStreamComponentClose(VideoState *is, int stream_index)
{
	AVFormatContext *ic = is->ic;
	AVCodecParameters *codecpar;

	if (stream_index < 0 || stream_index >= ic->nb_streams)
		return;
	codecpar = ic->streams[stream_index]->codecpar;

	switch (codecpar->codec_type)
	{
		case AVMEDIA_TYPE_AUDIO:
			playDecoderAbort(&is->audioDecoder, &is->audioFrameQueue);
			SDL_CloseAudio();
			playDecoderDestroy(&is->audioDecoder);
			swr_free(&is->swr_ctx);
			av_freep(&is->audio_buf1);
			is->audio_buf1_size = 0;
			is->audio_buf = NULL;

			if (is->rdft)
			{
				av_rdft_end(is->rdft);
				av_freep(&is->rdft_data);
				is->rdft = NULL;
				is->rdft_bits = 0;
			}
			break;
		
		case AVMEDIA_TYPE_VIDEO:
			playDecoderAbort(&is->videoDecoder, &is->videoFrameQueue);
			playDecoderDestroy(&is->videoDecoder);
			break;
		
		case AVMEDIA_TYPE_SUBTITLE:
			playDecoderAbort(&is->subtitleDecoder, &is->subtitleFrameQueue);
			playDecoderDestroy(&is->subtitleDecoder);
			break;
		
		default:
			break;
	}

	ic->streams[stream_index]->discard = AVDISCARD_ALL;
	switch (codecpar->codec_type)
	{
		case AVMEDIA_TYPE_AUDIO:
			is->audio_st = NULL;
			is->audio_stream = -1;
			break;
		
		case AVMEDIA_TYPE_VIDEO:
			is->video_st = NULL;
			is->video_stream = -1;
			break;
		
		case AVMEDIA_TYPE_SUBTITLE:
			is->subtitle_st = NULL;
			is->subtitle_stream = -1;
			break;
		
		default:
			break;
	}
}

/* open a given stream. Return 0 if OK */
int playStreamComponentOpen(VideoState *is, int stream_index)
{
	AVFormatContext *ic = is->ic;
	AVCodecContext *avctx;
	AVCodec *codec;
	const char *forced_codec_name = NULL;
	AVDictionary *opts = NULL;
	AVDictionaryEntry *t = NULL;
	int sample_rate, nb_channels;
	int64_t channel_layout;
	int ret = 0;
	int stream_lowres = lowres;

	if (stream_index < 0 || stream_index >= ic->nb_streams)
		return -1;

	avctx = avcodec_alloc_context3(NULL);
	if (!avctx)
		return AVERROR(ENOMEM);

	ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
	if (ret < 0)
		goto fail;
	av_codec_set_pkt_timebase(avctx, ic->streams[stream_index]->time_base);

	codec = avcodec_find_decoder(avctx->codec_id);

	switch(avctx->codec_type)
	{
		case AVMEDIA_TYPE_AUDIO   : 
			is->last_audio_stream    = stream_index; 
			forced_codec_name =    audio_codec_name; 
			break;
			
		case AVMEDIA_TYPE_SUBTITLE:
			is->last_subtitle_stream = stream_index;
			forced_codec_name = subtitle_codec_name;
			break;
			
		case AVMEDIA_TYPE_VIDEO   :
			is->last_video_stream    = stream_index;
			forced_codec_name =    video_codec_name;
			break;
	}
	
	if (forced_codec_name)
		codec = avcodec_find_decoder_by_name(forced_codec_name);
	
	if (!codec)
	{
		if (forced_codec_name)
			av_log(NULL, AV_LOG_WARNING, "No codec could be found with name '%s'\n", forced_codec_name);
		else
			av_log(NULL, AV_LOG_WARNING, "No codec could be found with id %d\n", avctx->codec_id);

		ret = AVERROR(EINVAL);
		goto fail;
	}

	avctx->codec_id = codec->id;
	if(stream_lowres > av_codec_get_max_lowres(codec))
	{
		av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
		av_codec_get_max_lowres(codec));
		stream_lowres = av_codec_get_max_lowres(codec);
	}

	av_codec_set_lowres(avctx, stream_lowres);

#if FF_API_EMU_EDGE
	if(stream_lowres) avctx->flags |= CODEC_FLAG_EMU_EDGE;
#endif
	if (fast)
		avctx->flags2 |= AV_CODEC_FLAG2_FAST;
#if FF_API_EMU_EDGE
	if(codec->capabilities & AV_CODEC_CAP_DR1)
		avctx->flags |= CODEC_FLAG_EMU_EDGE;
#endif

	opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
	if (!av_dict_get(opts, "threads", NULL, 0))
		av_dict_set(&opts, "threads", "auto", 0);

	if (stream_lowres)
		av_dict_set_int(&opts, "lowres", stream_lowres, 0);

	if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
		av_dict_set(&opts, "refcounted_frames", "1", 0);

	if ((ret = avcodec_open2(avctx, codec, &opts)) < 0)
	{
		goto fail;
	}

	if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX)))
	{
		av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
		ret =  AVERROR_OPTION_NOT_FOUND;
		goto fail;
	}

	is->eof = 0;
	ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
	switch (avctx->codec_type)
	{
		case AVMEDIA_TYPE_AUDIO:
#if CONFIG_AVFILTER
			{
				AVFilterContext *sink;

				is->audio_filter_src.freq           = avctx->sample_rate;
				is->audio_filter_src.channels       = avctx->channels;
				is->audio_filter_src.channel_layout = get_valid_channel_layout(avctx->channel_layout, avctx->channels);
				is->audio_filter_src.fmt            = avctx->sample_fmt;
				if ((ret = configure_audio_filters(is, afilters, 0)) < 0)
					goto fail;
				
				sink = is->out_audio_filter;
				sample_rate    = av_buffersink_get_sample_rate(sink);
				nb_channels    = av_buffersink_get_channels(sink);
				channel_layout = av_buffersink_get_channel_layout(sink);
			}
#else
			sample_rate    = avctx->sample_rate;
			nb_channels    = avctx->channels;
			channel_layout = avctx->channel_layout;
#endif

			/* prepare audio output */
			if ((ret = playSdlAudioOpen(is, channel_layout, nb_channels, sample_rate, &is->audio_tgt)) < 0)
				goto fail;
			
			is->audio_hw_buf_size = ret;
			is->audio_src = is->audio_tgt;
			is->audio_buf_size  = 0;
			is->audio_buf_index = 0;

			/* init averaging filter */
			is->audio_diff_avg_coef  = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
			is->audio_diff_avg_count = 0;
			
			/* since we do not have a precise anough audio FIFO fullness,
			we correct audio sync only if larger than this threshold */
			is->audio_diff_threshold = (double)(is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;

			is->audio_stream = stream_index;
			is->audio_st = ic->streams[stream_index];

			playDecoderInit(&is->audioDecoder, avctx, &is->audioq, is->continue_read_thread);
			if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !is->ic->iformat->read_seek)
			{
				is->audioDecoder.start_pts = is->audio_st->start_time;
				is->audioDecoder.start_pts_tb = is->audio_st->time_base;
			}
			if ((ret = playDecoderStart(&is->audioDecoder, playThreadAudioDecode, is)) < 0)
				goto out;
			
			SDL_PauseAudio(0);
			break;
			
		case AVMEDIA_TYPE_VIDEO:
			is->video_stream = stream_index;
			is->video_st = ic->streams[stream_index];

			playDecoderInit(&is->videoDecoder, avctx, &is->videoq, is->continue_read_thread);
			if ((ret = playDecoderStart(&is->videoDecoder, playThreadVideoDecode, is)) < 0)
				goto out;
			
			is->queue_attachments_req = 1;
			break;
			
		case AVMEDIA_TYPE_SUBTITLE:
			is->subtitle_stream = stream_index;
			is->subtitle_st = ic->streams[stream_index];

			playDecoderInit(&is->subtitleDecoder, avctx, &is->subtitleq, is->continue_read_thread);
			if ((ret = playDecoderStart(&is->subtitleDecoder, playThreadSubtitleDecode, is)) < 0)
				goto out;
			break;
			
		default:
			break;
	}
	
goto out;

fail:
	avcodec_free_context(&avctx);
out:
	av_dict_free(&opts);

	return ret;
}


