
#include "muxFfmpegRecord.h"

#if 1
int recordInputInit(RECORD_INPUT_T *input, const char *filename)
{
	int ret;
	AVFormatContext *inCtx = NULL;
	AVInputFormat *file_iformat;

	memset(input, 0 , sizeof(RECORD_INPUT_T));
	
	snprintf(input->filename, sizeof(input->filename), "%s", filename);
/*	
	input->readVideo = ;
	input->readAudio = ;
	input->readSubtitle = ;
*/

#if ARCH_X86
	file_iformat = av_find_input_format("h264");
	if (!file_iformat)
	{
		av_log(NULL, AV_LOG_FATAL, "Unknown input format: %s\n", "h264");
		return AVERROR(EINVAL);
	}

	if ((ret = avformat_open_input(&inCtx, filename, file_iformat, 0)) < 0)
	{
		fprintf(stderr, "Could not open input file '%s'", filename);
		return -1;
	}

	if ((ret = avformat_find_stream_info(inCtx, 0)) < 0)
	{
		fprintf(stderr, "Failed to retrieve input stream information");
		return -1;
	}

	av_dump_format( inCtx, 0,  filename, 0);

	input->streamNumber = inCtx->nb_streams;
	input->privData = inCtx;
#endif

	return 0;
}
#else

int recordInputInit(RECORD_INPUT_T *input, const char *filename)
{
//	int ret;
//	AVFormatContext *inCtx = NULL;

	memset(input, 0 , sizeof(RECORD_INPUT_T));
	
	snprintf(input->filename, sizeof(input->filename), "%s", filename);
/*	
	input->readVideo = ;
	input->readAudio = ;
	input->readSubtitle = ;

	if ((ret = avformat_open_input(&inCtx, filename, 0, 0)) < 0)
	{
		fprintf(stderr, "Could not open input file '%s'", filename);
		return -1;
	}

	if ((ret = avformat_find_stream_info(inCtx, 0)) < 0)
	{
		fprintf(stderr, "Failed to retrieve input stream information");
		return -1;
	}

	av_dump_format( inCtx, 0,  filename, 0);

	input->streamNumber = inCtx->nb_streams;
	input->privData = inCtx;
*/

	input->streamNumber = 2;
	input->privData = NULL;

	return 0;
}

#endif

