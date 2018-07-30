
/**
 *
 */

#include "muxFfmpegRecord.h"

static int _initVideoStream(MUX_OUTPUT_STREAM *video)
{
	AVCodecContext *codecCtx;
	enum AVCodecID codec_id = AV_CODEC_ID_H264;
	AVCodec *codec = NULL;
	
	AVCodecParameters *param = NULL;
	int ret;
	
#if 0
	codec = avcodec_find_encoder(codec_id);
	if (!codec)
	{
		fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
		exit(1);
	}
	//codec->
	
	record->videoStream = avformat_new_stream(record->outCtx, codec);
#endif
	video->stream = avformat_new_stream(video->ctx, NULL);
	if (!video->stream)
	{
		fprintf(stderr, "Failed allocating output video stream\n");
		ret = AVERROR_UNKNOWN;
		return -1;
	}

#if 0
	//record->videoStream->time_base = (AVRational){ 1, AV_TIME_BASE };// (AVRational){1, 90000};
	//record->videoStream->r_frame_rate = (AVRational ){1, AV_TIME_BASE/STREAM_FRAME_RATE};
	record->videoStream->time_base = (AVRational){1, 90000};//AV_TIME_BASE};
	//record->videoStream->r_frame_rate = (AVRational ){1, 3600};
	record->videoStream->avg_frame_rate = (AVRational){1, STREAM_FRAME_RATE};
#endif
	video->stream->r_frame_rate = (AVRational ){ STREAM_FRAME_RATE, 1};
	video->stream->avg_frame_rate = (AVRational){ STREAM_FRAME_RATE, 1};
#if 0	
	record->videoStream->time_base = (AVRational){ 1, 25};// AV_TIME_BASE};// (AVRational){1, 90000};
	record->videoStream->time_base = (AVRational){ 1, AV_TIME_BASE};// AV_TIME_BASE};// (AVRational){1, 90000};
#endif	
	//record->videoStream->codec->time_base = (AVRational){ 1, 25 };// (AVRational){1, 90000};


#if 0
//	ost->st->id = oc->nb_streams-1;
	codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx)
	{
		fprintf(stderr, "Could not alloc an encoding context\n");
		exit(1);
	}
	//ost->enc = codecCtx;
#endif

	param = video->stream->codecpar;
	param->codec_type = AVMEDIA_TYPE_VIDEO;
	param->codec_id = AV_CODEC_ID_H264;
	param->format = AV_PIX_FMT_YUV420P;
	param->width = 1280;
	param->height = 720;
//	param->sample_rate = 25;

	param->codec_tag = 0;


#if 0
	/* open the codec */
	ret = avcodec_open2(record->videoStream->codec, NULL, NULL);
	if (ret < 0)
	{
		printf("could not open video codec");
		//exit(1);
	}

#endif

	av_init_packet( &video->pkt);
	//video->pkt.pts = 1;
	video->pkt.stream_index = MUX_RECORD_STREAM_VIDEO;

	printf("video stream ID:%d, pkt stream ID:%d\n", video->stream->index, video->pkt.stream_index);
	
	return 0;
}

static int _initAudioStream(MUX_OUTPUT_STREAM *audio)
{
	AVCodecParameters *param = NULL;
	int ret;
	
	audio->stream = avformat_new_stream(audio->ctx, NULL);
	if (!audio->stream)
	{
		fprintf(stderr, "Failed allocating output audio stream\n");
		ret = AVERROR_UNKNOWN;
		return -1;
	}
	
	param = audio->stream->codecpar;
	param->codec_type = AVMEDIA_TYPE_AUDIO;
	param->codec_id = AV_CODEC_ID_AAC;
//	param->codec_id = AV_CODEC_ID_AC3;
	param->format = AV_SAMPLE_FMT_S16;
	param->sample_rate = 48000;
	
	param->channel_layout = AV_CH_LAYOUT_STEREO;

	audio->stream->time_base = (AVRational){1, param->sample_rate};

	param->codec_tag = 0;

/*
config = 1588 (aac-lc object type = 2; sampling rate 8Khz; number of channels = 1; 1024 samples; this comes out to be 1588 according to ISO14496-3 specs) 
*/

	static unsigned char extradata[] = {0x15, 0x88}; //main feature!!!
	param->extradata = extradata;
	param->extradata_size = sizeof(extradata);
	
	av_init_packet( &audio->pkt);
	audio->pkt.stream_index = MUX_RECORD_STREAM_AUDIO;
	
	printf("Audio stream ID:%d, pkt stream ID:%d\n", audio->stream->index, audio->pkt.stream_index);
	return 0;
}

static int _initSubtitleStream(MUX_OUTPUT_STREAM *subtitle)
{
	AVCodecParameters *param = NULL;
	int ret;
	
	subtitle->stream = avformat_new_stream(subtitle->ctx, NULL);
	if (! subtitle->stream)
	{
		fprintf(stderr, "Failed allocating output stream\n");
		ret = AVERROR_UNKNOWN;
		return -1;
	}

	param = subtitle->stream->codecpar;
	param->codec_type = AVMEDIA_TYPE_SUBTITLE;
	param->codec_id = AV_CODEC_ID_SSA;
//	param->format = AV_PIX_FMT_YUV420P;

	param->codec_tag = 0;


	return 0;
}


int recordInit(MUX_RECORD *record, char *filename, RECORD_INPUT_T *input)
{
	int ret;
	
	AVOutputFormat *fmt;

	//AVCodec *audio_codec, *video_codec;
	//int have_video = 0, have_audio = 0;
	//int encode_video = 0, encode_audio = 0;
	//AVDictionary *opt = NULL;
	//int i;

	memset(record, 0, sizeof(MUX_RECORD));
	snprintf(record->filename, sizeof(record->filename), "%s", filename );
	
	/* allocate the output media context */
	avformat_alloc_output_context2(&record->outCtx, NULL, "matroska", NULL);
	//avformat_alloc_output_context2(&record->outCtx, NULL, "mov", NULL);
	//avformat_alloc_output_context2(&record->outCtx, NULL, "mp4", NULL);
	if (!record->outCtx)
	{
		printf("Could not deduce output format from file extension: using MPEG.\n");
		return 1;
	}
	

	fmt = record->outCtx->oformat;

	/* Add the audio and video streams using the default format codecs and initialize the codecs. */
	if (fmt->video_codec != AV_CODEC_ID_NONE)
	{
		printf("Video Codec : %d\n", fmt->video_codec);
	}

	if (fmt->audio_codec != AV_CODEC_ID_NONE)
	{
		printf("Audio Codec : %d\n", fmt->audio_codec);
	}

	if (fmt->subtitle_codec != AV_CODEC_ID_NONE)
	{
		printf("Subtitle : %d\n", fmt->subtitle_codec);
	}

	record->gopSize = 100;
	record->video.ctx = record->outCtx;
	record->audio.ctx = record->outCtx;
	record->subtitle.ctx = record->outCtx;

	_initVideoStream(&record->video);

	_initAudioStream(&record->audio);

#if 0	
	_initSubtitleStream(&record->subtitle);
#endif

	av_dump_format(record->outCtx, 0, record->filename, 1);
	
	if (!( record->outCtx->oformat->flags & AVFMT_NOFILE))
	{
		ret = avio_open(& record->outCtx->pb, record->filename, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			fprintf(stderr, "Could not open output file '%s'", record->filename);
			return -1;
		}
	}

	ret = avformat_write_header( record->outCtx, NULL);
	if (ret < 0)
	{
		fprintf(stderr, "Error occurred when opening output file\n");
		return -1;
	}

#if 0
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.size = 0;
	pkt.data = NULL;

	pkt.stream_index = 0;
	pkt.dts = record->videoDts;
	pkt.pts = 0;
	av_write_frame(record->outCtx, &pkt);
#endif
	
	return 0;
}


void recordClose(MUX_RECORD *record)
{
	av_write_trailer( record->outCtx);

L_TRACE();
	/* free AvStream resources ??? */
//	avformat_close_input(&ifmt_ctx);

	/* close output */
	if ( record->outCtx && !(record->outCtx->oformat->flags & AVFMT_NOFILE))
		avio_closep(&record->outCtx->pb);

L_TRACE();
//	avformat_free_context(record->outCtx);
L_TRACE();

}


int recordWriteVideo(MUX_RECORD *record, unsigned char *data, unsigned int size, unsigned int pts)
{
	int ret;
	MUX_OUTPUT_STREAM *video = &record->video;
//	AVPacket pkt;
	
//	av_init_packet( &pkt);
//	pkt.pts = 1;
//	pkt.stream_index = MUX_RECORD_STREAM_VIDEO;
#if 0	
	void *p;
	AVFrame *frame = record->videoFrame;

	p = memcpy(frame->data, data, size);
	if( p == NULL)
	{
		fprintf(stderr, "writing video failed, need %d bytes\n", size);
	}
	frame->pts = pts;

#if 0	
	ret = av_interleaved_write_uncoded_frame(record->outCtx, MUX_RECORD_STREAM_VIDEO, frame);
#else
	ret = av_write_uncoded_frame(record->outCtx, MUX_RECORD_STREAM_VIDEO, frame);
#endif
#endif	

//	int av_new_packet(AVPacket *pkt, int size)

	ret = av_packet_from_data(&video->pkt, data, size);
	if (ret< 0)
	{
		fprintf(stderr, "init video packet failed: ret:%d\n", ret);
		return -1;
	}

	//record->videoPkt.pts = pts;
	//++record->videoPkt.pts;
#if 0
	//record->videoStream->duration += record->videoDuration;
	pkt.dts = record->videoDts;
	//pkt.duration = record->videoDuration;

	pkt.pts = pkt.dts + record->videoDuration;
	//record->videoDts = pkt.pts;
	record->videoDts += record->videoDuration;

#else

#if 0
	pkt.pts = (int64_t)pts*(int64_t)record->videoStream->time_base.num * (int64_t)record->videoStream->time_base.den;
	pkt.dts = (int64_t)pts*(int64_t)record->videoStream->time_base.num * (int64_t)record->videoStream->time_base.den;
	pkt.duration = (int64_t)pts*(int64_t)record->videoStream->time_base.num * (int64_t)record->videoStream->time_base.den;
#endif

	//if(pkt.pts != AV_NOPTS_VALUE)
	{
/*
		pkt.pts = av_rescale_q_rnd(ptsInc++, record->videoStream->codec->time_base, record->videoStream->time_base, 
			(enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(ptsInc, record->videoStream->codec->time_base, record->videoStream->time_base, 
			(enum AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
if (encodedPacket.pts != AV_NOPTS_VALUE)
     encodedPacket.pts =  av_rescale_q(encodedPacket.pts, outStream->codec->time_base, outStream->time_base);
 if (encodedPacket.dts != AV_NOPTS_VALUE)
     encodedPacket.dts = av_rescale_q(encodedPacket.dts, outStream->codec->time_base, outStream->time_base);
*/

		//pkt.dts = ptsInc;
		//ptsInc += record->videoStream->time_base.den / STREAM_FRAME_RATE;// record->videoStream->time_base.den;
		video->pkt.pts = video->nextPts;
		video->nextPts += 40;//= record->videoStream->codec->time_base.den*record->videoStream->time_base.den;
	}
#endif

#if 0
	if (record->videoStream->first_dts == AV_NOPTS_VALUE)
		record->videoStream->first_dts = pkt.dts;
#endif

	ret = av_write_frame(video->ctx, &video->pkt);
	if (ret< 0)
	{
		fprintf(stderr, "write video frame failed on video stream %d: ret:%d, PTS:%d ms, PTS %"PRId64" \n", video->pkt.stream_index, ret, pts, video->pkt.pts );
		return -1;

	}
	//av_interleaved_write_frame(context, &packet);
	//put_flush_packet(context->pb);
	
	video->count += size;
	printf("video stream %d data size :%d, total %"PRId64", TS:%d; return length %d\n", video->pkt.stream_index, size, video->count, pts, ret);
	return 0;
}

int recordWriteAudio(MUX_RECORD *record, unsigned char *data, unsigned int size, unsigned int pts)
{
	int ret;
	MUX_OUTPUT_STREAM *audio = &record->audio;
#if 0	
	void *p;
	AVFrame *frame = record->videoFrame;

	p = memcpy(frame->data, data, size);
	if( p == NULL)
	{
		fprintf(stderr, "writing audio failed, need %d bytes\n", size);
	}
	frame->pts = pts;

#if 0	
	ret = av_interleaved_write_uncoded_frame(record->outCtx, MUX_RECORD_STREAM_AUDIO, frame);
#else
	ret = av_write_uncoded_frame(record->outCtx, MUX_RECORD_STREAM_AUDIO, frame);
	if (ret< 0)
	{
		fprintf(stderr, "write audio frame failed: ret:%d\n", ret);
		return -1;
	}
#endif	
#endif

	//printf("1 audio stream %d\n", record->audioPkt.stream_index);

	ret = av_packet_from_data(&audio->pkt, data, size);
	if (ret< 0)
	{
		fprintf(stderr, "init audio packet failed: ret:%d\n", ret);
		return -1;
	}
	//printf("2 audio stream %d\n", record->audioPkt.stream_index);

//	record->audioPkt.pts = pts;
//	record->audioPkt.pts = av_rescale_q(pts, (AVRational){1, record->audioStream->codecpar->sample_rate}, (AVRational){1, 9000});
//	record->audioPkt.pts = av_rescale_q(pts, AV_TIME_BASE_Q, (AVRational){1, 9000});
//	record->audioPkt.pts = av_rescale_q(pts, (AVRational){1, record->audioStream->codecpar->sample_rate}, AV_TIME_BASE_Q);
//	record->audioPkt.pts = av_rescale_q(pts,(AVRational){1, record->audioStream->codecpar->sample_rate}, AV_TIME_BASE_Q);

#if 1
//	record->audioPkt.pts = (int64_t)pts*(int64_t)record->audioStream->time_base.num * (int64_t)record->audioStream->time_base.den;
//	record->audioPkt.dts = (int64_t)pts*(int64_t)record->audioStream->time_base.num * (int64_t)record->audioStream->time_base.den;
//	record->audioPkt.duration = (int64_t)pts*(int64_t)record->audioStream->time_base.num * (int64_t)record->audioStream->time_base.den;
	//	pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);

	audio->nextPts += size;
	audio->pkt.pts = audio->nextPts;//size(int64_t)pts*(int64_t)record->audioStream->time_base.num * (int64_t)record->audioStream->time_base.den;
	//pkt.stream_index = MUX_RECORD_STREAM_VIDEO;

	//printf("3 audio stream %d\n", record->audioPkt.stream_index);
	ret = av_write_frame(audio->ctx, &audio->pkt);
	if (ret< 0)
	{
		fprintf(stderr, "write audio frame failed on audio stream %d: ret:%d; (PTS:%d ms; PktPts: %"PRId64",)\n", audio->pkt.stream_index, ret, pts, audio->pkt.pts );
		return -1;
	}
#endif

	printf("audio stream %d data size :%d, TS:%d, return length %d\n", audio->pkt.stream_index, size, pts, ret);

	return 0;
}


int recordWriteSubtitle(MUX_RECORD *record, unsigned char *data, unsigned int size, unsigned int pts)
{
	int ret;
	void *p;

	return 0;
}


