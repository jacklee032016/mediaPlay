
#ifndef	__RECORD_H__
#define	__RECORD_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

/*
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
*/

#define STREAM_DURATION		10.0
#define STREAM_FRAME_RATE		25 /* 25 images/s */
#define STREAM_PIX_FMT			AV_PIX_FMT_YUV420P /* default pix_fmt */

#define	MEDIA_FILENAME_LENGTH			256

#define SCALE_FLAGS SWS_BICUBIC

struct _RecordInput;

typedef	enum _MUX_RECORD_STREAM
{
	MUX_RECORD_STREAM_VIDEO = 0,
	MUX_RECORD_STREAM_AUDIO,
	MUX_RECORD_STREAM_SUBTITLE,
	MUX_RECORD_STREAM_ERROR,
		
}MUX_RECORD_STREAM;

typedef int (*read_stream_number)(struct _RecordInput);

typedef int (*read_video_data)(struct _RecordInput, void *data, int size);
typedef int (*read_audio_data)(struct _RecordInput, void *data, int size);
typedef int (*read_subtitle_data)(struct _RecordInput, void *data, int size);

typedef	struct _RecordInput
{
	void						*privData;
	
	char						filename[MEDIA_FILENAME_LENGTH];
	int						streamNumber;
	
	read_video_data			readVideo;
	read_audio_data			readAudio;
	read_subtitle_data			readSubtitle;
}RECORD_INPUT_T;


typedef	struct _MUX_OUTPUT_STREAM
{
	AVFormatContext		*ctx;

	AVStream			*stream;
	AVPacket				pkt;
	int64_t				nextPts;

	int64_t				count;	/* byte count of output */
	
}MUX_OUTPUT_STREAM;

// a wrapper around a single output AVStream
typedef struct _MUX_RECORD
{

	char						filename[MEDIA_FILENAME_LENGTH];

	AVFormatContext			*outCtx;

	MUX_OUTPUT_STREAM		video;
	MUX_OUTPUT_STREAM		audio;
	MUX_OUTPUT_STREAM		subtitle;
	
	int						gopSize;

}MUX_RECORD;


int recordInit(MUX_RECORD *record, char *filename, RECORD_INPUT_T *input);
void recordClose(MUX_RECORD *record);

int recordInputInit(RECORD_INPUT_T *input, const char *filename);

int recordWriteVideo(MUX_RECORD *record, unsigned char *data, unsigned int size, unsigned int pts);
int recordWriteAudio(MUX_RECORD *record, unsigned char *data, unsigned int size, unsigned int pts);
int recordWriteSubtitle(MUX_RECORD *record, unsigned char *data, unsigned int size, unsigned int pts);


#endif

