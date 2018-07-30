
/**
 *
 */


#include "muxFfmpegRecord.h"

static void _record_log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
	AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

	printf("%s: pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
		tag,  av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
			av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
			av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base), pkt->stream_index);
}

int main(int argc, char **argv)
{
	MUX_RECORD _record;
	MUX_RECORD *record = &_record;
	RECORD_INPUT_T _input;
	RECORD_INPUT_T *input = &_input;

	AVOutputFormat *ofmt = NULL;
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket videoPkt, audioPkt;
	const char *in_filename;
	int ret, i;
	int stream_index = 0;
	int *stream_mapping = NULL;
	int stream_mapping_size = 0;

	if (argc < 3)
	{
		printf("usage: %s input output\n"
			"API example program to remux a media file with libavformat and libavcodec.\n"
			"The output format is guessed according to the file extension.\n"
			"\n", argv[0]);
		return 1;
	}

	in_filename  = argv[1];

	av_register_all();
	
	if(recordInputInit(input, in_filename) < 0)
	{
		printf("Open Input failed\n");
		exit(1);
	}

L_TRACE();
	recordInit(record, argv[2], input);

	ifmt_ctx = (AVFormatContext *)input->privData;
	ofmt_ctx = record->outCtx;
	
L_TRACE();


	AVFormatContext *inAudioCtx = NULL;
	AVInputFormat *fileAudioFormat;
	char		*fileAudio = "../aud.aac";

	fileAudioFormat = av_find_input_format("aac");
	if (!fileAudioFormat)
	{
		av_log(NULL, AV_LOG_FATAL, "Unknown input format: %s\n", "aac");
		return AVERROR(EINVAL);
	}

	if ((ret = avformat_open_input(&inAudioCtx, fileAudio, fileAudioFormat, 0)) < 0)
	{
		fprintf(stderr, "Could not open input file '%s'", fileAudio);
		return -1;
	}

	if ((ret = avformat_find_stream_info(inAudioCtx, 0)) < 0)
	{
		fprintf(stderr, "Failed to retrieve input stream information");
		return -1;
	}

	av_dump_format( inAudioCtx, 0,  fileAudio, 0);

	
	while (1)
	{
		AVStream *in_stream, *out_stream;

L_TRACE();
		ret = av_read_frame(ifmt_ctx, &videoPkt);
		if (ret < 0)
		{
			printf("failed reading video frame\n");
			break;
		}

		ret = av_read_frame(inAudioCtx, &audioPkt);
		if (ret < 0)
		{
			printf("failed reading audio frame\n");
			break;
		}
L_TRACE();

		ret = recordWriteVideo( record, videoPkt.data, videoPkt.size, 0);
		if (ret < 0)
		{
			printf("failed writing video frame\n");
			break;
		}
		
		ret = recordWriteAudio( record, audioPkt.data, audioPkt.size, 0);
		if (ret < 0)
		{
			printf("failed writing video frame\n");
			break;
		}
		

#if 0		
		//pkt.stream_index = stream_mapping[pkt.stream_index];

		//out_stream = ofmt_ctx->streams[pkt.stream_index];
L_TRACE();

		_record_log_packet(ifmt_ctx, &pkt, "in");

L_TRACE();
		/* copy packet */
		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
		pkt.pos = -1;
		_record_log_packet(ofmt_ctx, &pkt, "out");

		ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
		if (ret < 0)
		{
			fprintf(stderr, "Error muxing packet\n");
			break;
		}
#endif

		//av_packet_unref(&pkt);
	}


	recordClose(record);
 
	return 0;
}

