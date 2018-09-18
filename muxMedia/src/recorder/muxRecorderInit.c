/*
 * Recorder Plugin: recorder thread and recording task
 */

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include "libavformat/internal.h"

#include "libavutil/avstring.h"
#include <libavutil/avassert.h>
#include <libavutil/timestamp.h>	/*av_ts2str */

#include "muxRecorder.h"

int muxRecordFfmpegClose(MuxRecorder *muxRecorder)
{
	MuxRecordTask *task = muxRecorder->currentTask;
	if(!task)
	{
		CMN_ABORT("Recording task is not initialized now, so it can't be closed!");
		return EXIT_FAILURE;
	}
	
	AVFormatContext *outCtx = (AVFormatContext *)task->outCtx;

	av_write_trailer(outCtx);

	/* close output */
	if ( outCtx && !( outCtx->oformat->flags & AVFMT_NOFILE))
	{
		avio_closep(&outCtx->pb);
	}

	/* free Ctx and its all AVStreams */
	avformat_free_context(outCtx);

	if(cmnMediaCheckSDCardFile(task->filename) )
	{
		cmnMediaRemountSDCard(FALSE);
	}

	task->status = MUX_RECORDER_STATUS_STOPPED;
	task->recordedTime += (task->startTime!=-1)?cmnGetTimeMs()-task->startTime:0;
	memset(muxRecorder->mediaFile, 0, CMN_NAME_LENGTH);

	return EXIT_SUCCESS;
}

int muxRecordFfmpegOpen(MuxRecorder *muxRecorder)
{
	MuxRecordTask *task = muxRecorder->currentTask;
	MuxMediaConsumer *mediaConsumer = muxRecorder->mediaConsumer;
	MuxMediaCapture *mediaCapture = NULL;
	
	AVDictionary	*headerOptions = NULL;
	AVFormatContext *ctx;
	AVStream *st;
	int i, ret=0;

	if(task == NULL || mediaConsumer == NULL)
	{
		CMN_ABORT("No media consumer or record task is provided!");
		return EXIT_FAILURE;
	}
	mediaCapture = mediaConsumer->mediaCapture;
	if(mediaCapture==NULL)
	{
		CMN_ABORT("No media consumer is defined for RECORDER");
		return EXIT_FAILURE;
	}

	if(cmnMediaCheckSDCardFile(task->filename) )
	{
		cmnMediaRemountSDCard(TRUE);
	}

	
	/* now we can open the relevant output stream */
//	avformat_alloc_output_context2(&ctx, NULL, NULL, task->filename);//"Example_Out.m3u8");
	ctx = avformat_alloc_context();
	if (!ctx)
	{
		MUX_RECORD_ERROR("'%s' Error: %s in alloc avformat context", task->filename, av_err2str(ret));
		return -EXIT_FAILURE;
	}

//	av_dict_copy(&(ctx->metadata), svrConn->stream->metadata, 0);
	if(task->outFormat)
	{
		ctx->oformat = task->outFormat;
	}
	else
	{
		ctx->oformat = av_guess_format(muxRecorder->config.format, NULL, NULL);
	}
	
	if(ctx->oformat == NULL)
	{
		CMN_ABORT("Container format of '%s' can't be created", muxRecorder->config.format);
		return -EXIT_FAILURE;
	}
//	av_assert0(ctx->oformat);

	for(i=0;i< mediaCapture->nbStreams; i++)
	{
		st = avformat_new_stream(ctx, NULL);
		if (!st)
		{
			MUX_RECORD_ERROR("Creating AVStream failed");
			return -EXIT_FAILURE;
		}
			//goto fail;

		void *st_internal;

		st_internal = st->internal;

		cmnMediaInitAvStream(st, mediaCapture->capturedMedias[i] );
		av_assert0(st->priv_data == NULL);
		av_assert0(st->internal == st_internal);

#if MUX_WITH_FLV_AAC_ADTS2ASC
		if(IS_STREAM_FORMAT(ctx->oformat, "flv") && (st->codecpar->codec_id == AV_CODEC_ID_AAC ) )
		{
			ff_stream_add_bitstream_filter(st, "aac_adtstoasc", NULL);
			st->internal->bitstream_checked = 1;
			ctx->flags |= AVFMT_FLAG_AUTO_BSF;
		}
#endif		

#if MUX_WITH_AVI_H264_ANNEXB
		if(IS_STREAM_FORMAT(ctx->oformat, "avi") && (st->codecpar->codec_id == AV_CODEC_ID_H264) )
		{
			ff_stream_add_bitstream_filter(st, "h264_mp4toannexb", NULL);
			st->internal->bitstream_checked = 1;
			ctx->flags |= AVFMT_FLAG_AUTO_BSF;
		}
#endif		

	}

	av_dump_format(ctx, 0, task->filename, 1);
	
	if (!( ctx->oformat->flags & AVFMT_NOFILE))
	{
		ret = avio_open(& ctx->pb, task->filename, AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			MUX_RECORD_ERROR("Could not open output file '%s': %s", task->filename, av_err2str(ret));
			goto fail;
		}
		MUX_RECORD_INFO("Recording media file '%s' created", task->filename);
	}

	ret = avformat_write_header(ctx, &headerOptions) ;
	if( ret < 0)
	{
		MUX_RECORD_ERROR("Write Header for %s failed: %s", task->filename, av_err2str(ret));
fail:
		av_free(ctx);
		return -EXIT_FAILURE;
	}
	
	av_dict_free(&ctx->metadata);

	av_dict_free(&headerOptions);
	
	task->outCtx = ctx;

	task->status = MUX_RECORDER_STATUS_RECORDING;

	return EXIT_SUCCESS;
}

static char *_recordingStatusName(int state)
{
	switch (state)
	{
		case MUX_RECORDER_STATUS_INIT:
			return "INIT";
			break;
		case MUX_RECORDER_STATUS_RECORDING:
			return "RECORDING";
			break;
		case MUX_RECORDER_STATUS_PAUSE:
			return "PAUSE";
			break;
		case MUX_RECORDER_STATUS_STOPPED:
			return "STOPPED";
			break;
		default:
			return "UNKNOWN";
			break;
	}

	return "UNKNOWN";
}


static int _muxRecorderStatus(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jEvent)
{
	MuxRecorder *muxRecorder = (MuxRecorder *)priv;
	int64_t	count = 0;
	int	pkts = 0;
	cJSON *newObject = NULL;
	cJSON *array = NULL;
	MuxRecordTask	*task = muxRecorder->currentTask;
	
	cmn_mutex_lock(muxRecorder->mutexLock);
	if( task == NULL )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "Recording has not startup now");
		goto endl;
	}

	newObject = cJSON_CreateObject();
	array = cJSON_CreateArray();

	cJSON_AddItemToArray(array, newObject);
	dataConn->resultObject = array;

	JEVENT_ADD_STRING(newObject, "filename", task->filename ); /* current recording file */
	JEVENT_ADD_STRING(newObject, "recordStatus",  _recordingStatusName(task->status) );

	if(task->status == MUX_RECORDER_STATUS_RECORDING )
	{
		pkts = task->recordedTime + (task->startTime!= -1)?cmnGetTimeMs() -task->startTime:0;
	}
	else
	{
		pkts = task->recordedTime;
	}
	
	JEVENT_ADD_INTEGER(newObject, "recordTime", pkts/1000);

	count = task->videoBytes+ task->audioBytes + task->subtitleBytes;
	JEVENT_ADD_INTEGER(newObject, "size",  (int)(count/1024));

	pkts = task->videoPkts + task->audioPkts + task->subtitlePkts;
	JEVENT_ADD_INTEGER(newObject, "packets", pkts);
	
	dataConn->errCode = IPCMD_ERR_NOERROR;
	
endl:	
	cmn_mutex_unlock(muxRecorder->mutexLock);
	
	return EXIT_SUCCESS;
}


static int _muxRecorderPause(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jEvent)
{
	MuxRecorder *muxRecorder = (MuxRecorder *)priv;
	int ret = EXIT_SUCCESS;
	
	cmn_mutex_lock(muxRecorder->mutexLock);
	MuxRecordTask *task = muxRecorder->currentTask;
	if(!task)
	{
		MUX_RECORD_ERROR("Recorder error: not task to stop");
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "Recording has not startup now");
		goto endl;
	}

	if(task->status != MUX_RECORDER_STATUS_RECORDING )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "Recording is not RECORDING state in now");
		goto endl;
	}

	task->status = MUX_RECORDER_STATUS_PAUSE;
	task->recordedTime += cmnGetTimeMs() - task->startTime;
	task->startTime = -1;

	dataConn->errCode = IPCMD_ERR_NOERROR;
			
endl:	
	cmn_mutex_unlock(muxRecorder->mutexLock);
	
	return ret;
}

static int _muxRecorderResume(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jEvent)
{
	MuxRecorder *muxRecorder = (MuxRecorder *)priv;
	int ret = EXIT_SUCCESS;
	
	cmn_mutex_lock(muxRecorder->mutexLock);
	MuxRecordTask *task = muxRecorder->currentTask;
	if(!task)
	{
		MUX_RECORD_ERROR("Recorder error: not task to stop");
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Recording has not startup now");
		goto endl;
	}

	if(task->status != MUX_RECORDER_STATUS_PAUSE )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Recording is not PAUSE state in now");
		goto endl;
	}

	task->status = MUX_RECORDER_STATUS_RECORDING;
	task->startTime = cmnGetTimeMs();
	
	dataConn->errCode = IPCMD_ERR_NOERROR;
	
endl:	
	cmn_mutex_unlock(muxRecorder->mutexLock);
	
	return ret;
}


static int _muxRecorderStop(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jEvent)
{
	MuxRecorder *muxRecorder = (MuxRecorder *)priv;
	int ret = EXIT_SUCCESS;
		
	cmn_mutex_lock(muxRecorder->mutexLock);
	MuxRecordTask *task = muxRecorder->currentTask;
	if(!task)
	{
		MUX_RECORD_ERROR("Recorder error: not task to stop");
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Recording has not startup now");
		goto endl;
	}

	if(task->status != MUX_RECORDER_STATUS_RECORDING && task->status != MUX_RECORDER_STATUS_PAUSE )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Recording is not in RECORDING or PAUSE state");
		goto endl;
	}

	ret = muxRecordFfmpegClose(muxRecorder);

	dataConn->errCode = IPCMD_ERR_NOERROR;
	
endl:	
	cmn_mutex_unlock(muxRecorder->mutexLock);

	MUX_RECORD_DEBUG( "Record stopped");

	return ret;
}

static int _muxRecorderStart(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jEvent)
{
	MuxRecorder *muxRecorder = (MuxRecorder *)priv;
	MuxMain *muxMain = (MuxMain *)muxRecorder->priv;
	MuxMediaCapture *mediaCapture = muxRecorder->mediaConsumer->mediaCapture;
	MuxRecordTask *task = muxRecorder->currentTask;

	int	duration = -1;
	
	int		ret = EXIT_SUCCESS;
	struct tm *_time;
	time_t	currentTime;
	char		*path = NULL;
	char *media = NULL;

#define	PARAM_IS_MANDIDATE		1

	cmn_mutex_lock(muxRecorder->mutexLock);

	if(!mediaCapture ||mediaCapture->getCaptureState(mediaCapture) != MUX_CAPTURE_STATE_RUNNING)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Capturer is not running now");
		goto endl;
	}
	
	if( task!=NULL && task->status == MUX_RECORDER_STATUS_RECORDING )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "Recording has been started now");
		goto endl;
	}

	media = cmnGetStrFromJsonObject(jEvent->object, _MUX_JSON_NAME_MEDIA);
#if PARAM_IS_MANDIDATE
	if(media == NULL || !strlen(media))
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'media' is not defined for recording");
		goto endl;
	}
#endif

	duration = cmnGetIntegerFromJsonObject(jEvent->object, "duration");
#if PARAM_IS_MANDIDATE
	if(duration == -1)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'duration' is not defined for recording");
		goto endl;
	}
#endif
	if(media==NULL || !strlen(media) )
	{
		memset(muxRecorder->mediaFile, 0, CMN_NAME_LENGTH);
	}
	else
	{
		snprintf(muxRecorder->mediaFile, CMN_NAME_LENGTH, "%s", media);
	}
	muxRecorder->duration = duration*1000;


	time(&currentTime);
	_time = localtime( &currentTime);

	if(muxMain->mediaCaptureConfig.storeType == MEDIA_DEVICE_SDCARD)
	{
		path = muxMain->mediaCaptureConfig.sdHome;
	}
	else
	{
		path = muxMain->mediaCaptureConfig.usbHome;
	}

	if(task)
	{
		cmn_free(task);
	}
	
	task = cmn_malloc(sizeof(MuxRecordTask));
	if(!task)
	{
		CMN_ABORT("No memory for Record Task");
	}
	muxRecorder->currentTask = task;

	if(IS_STRING_NULL(muxRecorder->mediaFile ) )
	{
		snprintf(task->filename, CMN_NAME_LENGTH, "%s/record_%02d.%02d.%02d.%02d.mkv", path,
			_time->tm_mon+1, _time->tm_mday, _time->tm_hour, _time->tm_min );
	}
	else
	{
		snprintf(task->filename, CMN_NAME_LENGTH, "%s", muxRecorder->mediaFile );
	}
	
	MUX_RECORD_DEBUG( "record file name:'%s'", task->filename);

	task->outFormat = av_guess_format(NULL, task->filename, NULL);
	if(!task->outFormat)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'filename' is not validate format");
		goto endl;
	}

	ret = muxRecordFfmpegOpen(muxRecorder);
	if(ret == EXIT_SUCCESS)
	{
		task->status = MUX_RECORDER_STATUS_RECORDING;
		task->startTime = cmnGetTimeMs();
		
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}
	else
	{
		task->status = MUX_RECORDER_STATUS_STOPPED;
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Can't start encoder of recording, Please re-try later");
	}
	MUX_RECORD_DEBUG( "Record started");

endl:	
	cmn_mutex_unlock(muxRecorder->mutexLock);

	return ret;
}


static PluginJSonHandler _jsonRecorderHandlers[] =
{
	{
		.type	= CMD_TYPE_REC_START,
		.name 	= "start",
		handler	: _muxRecorderStart
	},
	{
		.type	= CMD_TYPE_REC_STOP,
		.name 	= "stop",
		handler	: _muxRecorderStop
	},
	{
		.type	= CMD_TYPE_REC_PAUSE,
		.name 	= "pause",
		handler	: _muxRecorderPause
	},
	{
		.type	= CMD_TYPE_REC_RESUME,
		.name 	= "resume",
		handler	: _muxRecorderResume
	},
	{
		.type	= CMD_TYPE_REC_STATUS,
		.name 	= "status",
		handler	: _muxRecorderStatus
	},
	{
		.type	= CMD_TYPE_UNKNOWN,
		.name	= NULL,
		.handler	= NULL
	}
};



static int	_recorderHandler(struct _CmnThread *th, void *_event)
{
	MuxRecorder *muxRecorder =(MuxRecorder *)th->data;
//	MuxRecordTask *task = muxRecorder->currentTask;

	CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)_event;
	int res = EXIT_SUCCESS;

	res = cmnMuxJSonPluginHandle(muxRecorder, _jsonRecorderHandlers, jsonEvent);

#if 0	
	if (SYSTEM_IS_EXITING())
	{
		MUX_PLAY_WARN( "'%s' Thread recv EXIT signal\n", th->name );
		return -EXIT_FAILURE;
	}
#endif
	return res;
}


/*  */
static int _initRecorder(CmnThread *th, void *data)
{
//	MuxRecorder *muxRecorder =(MuxRecorder *)data;

	th->data = data;	
	return EXIT_SUCCESS;
}


CmnThread  threadRecorder =
{
	name		:	"RECORDER",
	flags		:	SET_BIT(1, CMN_THREAD_FLAG_WAIT),
	
	init			:	_initRecorder,
	mainLoop		:	NULL,
	eventHandler	:	_recorderHandler,
	destory		:	NULL,
	
	data			:	NULL,
};


/* write data in the context of capturing thread, such as Player's capturer and Feed Thread in SERVER. so dats is not copied, just use it
*/
static int _muxRecorderConsumerReceivePkt(MuxMediaConsumer *mediaConsumer, MUX_MEDIA_TYPE type, MuxMediaPacket *mediaPacket )
{
	AVPacket pkt;
	unsigned char *data;
	MuxRecorder *muxRecorder = (MuxRecorder *)mediaConsumer->priv;
	MuxRecordTask *task = NULL;
	int ret = EXIT_SUCCESS;
	int duration = 0;

	if(!muxRecorder)
	{
		CMN_ABORT("Invalidate data for RECORDER");
	}
	
	cmn_mutex_lock(muxRecorder->mutexLock);
	task = muxRecorder->currentTask;
	if( task==NULL || task->status != MUX_RECORDER_STATUS_RECORDING)
	{
		ret = mediaPacket->size;	/* means writing success */
		goto failed;
	}
	
	av_init_packet(&pkt);

	/* allocate memory for AVPacket, and copy data */
	data = av_malloc(mediaPacket->size);
	if(!data)
	{
		CMN_ABORT("No memory for data of media packet");
		ret = -EXIT_FAILURE;
		goto failed;
	}
	memcpy(data, mediaPacket->data, mediaPacket->size);
	
	ret = av_packet_from_data(&pkt, data, mediaPacket->size);
	if(ret)
	{
		MUX_RECORD_ERROR( "Error in init AvPaket from data: %s", av_err2str(ret));
		ret = -EXIT_FAILURE;
		goto failed;
	}
	pkt.stream_index = mediaPacket->streamIndex;
	pkt.pts = mediaPacket->pts;
	pkt.dts = mediaPacket->dts;

	ret = av_write_frame(task->outCtx, &pkt);
	if (ret< 0)
	{
		MUX_RECORD_ERROR( "write frame failed on stream %d: ret:%s(%d); ( PktPts: %s%"PRId64",)\n", pkt.stream_index, av_err2str(ret), ret, av_ts2str(pkt.pts), pkt.pts );
		ret = -EXIT_FAILURE;
		goto failed;
	}
	ret = mediaPacket->size;/* return size of data when success */

	if(pkt.stream_index == MUX_MEDIA_TYPE_VIDEO)
	{
		task->videoBytes += pkt.size;
		task->videoPkts ++;
	}
	else if(pkt.stream_index == MUX_MEDIA_TYPE_AUDIO)
	{
		task->audioBytes += pkt.size;
		task->audioPkts ++;
	}
	else if(pkt.stream_index == MUX_MEDIA_TYPE_SUBTITLE)
	{
		task->subtitleBytes += pkt.size;
		task->subtitlePkts ++;
	}
	else
	{
		MUX_RECORD_ERROR("No.%d is invalidate stream index", pkt.stream_index);
	}

	/* data of this packet will be freed here */
	av_packet_unref(&pkt);

	duration = cmnGetTimeMs()-task->startTime + task->recordedTime;
#if MUX_SVR_DEBUG_DATA_FLOW
	MUX_RECORD_DEBUG("Recording %d ms, need %d ms!", duration, muxRecorder->duration);
#endif

	if(duration >= muxRecorder->duration)
	{
		MUX_RECORD_INFO("Recording %d seconds, recording task end!", muxRecorder->duration);
		task->recordedTime = muxRecorder->duration;
		ret = muxRecordFfmpegClose(muxRecorder);
	}

failed:

	cmn_mutex_unlock(muxRecorder->mutexLock);

	return ret;
}


/* return: < 0, fail; =0, not used; > 1, received correctly
*/
static int _muxRecorderReceiveCaptureNotify(MuxMediaConsumer *mediaConsumer, MUX_CAPTURE_EVENT event, void *data)
{
	MuxRecorder *muxRecorder = (MuxRecorder *)mediaConsumer->priv;
	MuxRecordTask *task = NULL;
	int ret = EXIT_SUCCESS;

	if(!muxRecorder)
	{
		CMN_ABORT("Invalidate data for RECORDER");
		return EXIT_SUCCESS;
	}
	
	cmn_mutex_lock(muxRecorder->mutexLock);
	task = muxRecorder->currentTask;
	if( task==NULL || task->status != MUX_RECORDER_STATUS_RECORDING)
	{
		goto failed;
	}

	if(event == MUX_CAPTURE_EVENT_STOP)
	{
		if(task->status == MUX_RECORDER_STATUS_RECORDING ||
			task->status == MUX_RECORDER_STATUS_PAUSE )
		{
			ret = muxRecordFfmpegClose(muxRecorder);
			if(ret == EXIT_SUCCESS)
				ret = 1;
		}
	}
	else if(event == MUX_CAPTURE_EVENT_PAUSE)
	{
		if(task->status == MUX_RECORDER_STATUS_RECORDING )
		{
			task->status = MUX_RECORDER_STATUS_PAUSE;
			task->recordedTime += cmnGetTimeMs() - task->startTime;
			task->startTime = -1;
			ret = 1;
		}
	}

failed:
	cmn_mutex_unlock(muxRecorder->mutexLock);

	return ret;
}

int	muxRecorderInitConsumer(MuxRecorder *muxRecorder)
{
	MuxMediaConsumer *mediaConsumer = NULL;
	MuxMain *muxMain = (MuxMain *)muxRecorder->priv;
	int res;
	
	mediaConsumer = (MuxMediaConsumer *)cmn_malloc(sizeof(MuxMediaConsumer));
	if(!mediaConsumer)
	{
		CMN_ABORT("No memory for Media Consumer of RECORDER");
	}

	snprintf(mediaConsumer->name, sizeof(mediaConsumer->name), "%s", "RecordConsumer");
	mediaConsumer->receiveVideo = _muxRecorderConsumerReceivePkt;
	mediaConsumer->receiveNotify = _muxRecorderReceiveCaptureNotify;
	
	res = muxMain->registerConsumer( muxMain, mediaConsumer, muxRecorder->config.captureName);
	if(res != EXIT_SUCCESS)
	{
		return res;
	}

	muxRecorder->mutexLock = cmn_mutex_init();
	
	mediaConsumer->priv = muxRecorder;
	muxRecorder->mediaConsumer = mediaConsumer;
	
	return res;
}

static MuxRecorder _muxRecorder;

static int	_muxRecorderSignalExit(struct _MuxPlugin *plugin, int signal)
{
	
	return EXIT_SUCCESS;
}

static int _muxRecorderReportEvent(struct _MuxPlugin *plugin, CMN_PLAY_JSON_EVENT *jEvent)
{
	return cmnThreadAddEvent(&threadRecorder, jEvent);
}

static void _muxRecorderDestory(MuxPlugIn *plugin)
{
	MuxRecorder *muxRecorder = (MuxRecorder *)plugin->priv;

	if(muxRecorder->mediaConsumer)
	{
		cmn_free(muxRecorder->mediaConsumer);
	}

	if(muxRecorder->currentTask)
	{
		/* more should be do dependent on status of TASK */
		cmn_free(muxRecorder->currentTask);
	}

	cmn_mutex_destroy(muxRecorder->mutexLock);
	
}

#if 0
			if(muxPlayer->recorder.duration != -1)
			{
				if(muxPlayer->recorder.recordTime >= muxPlayer->recorder.duration)
				{
					MUX_RECORD_DEBUG("record duration reached:%d (%d) ms", muxPlayer->recorder.recordTime, muxPlayer->recorder.duration);
					muxRecorderStop(muxPlayer);

					return -EXIT_FAILURE; /* force recording thread exit */
				}
			}
#endif


int init( MuxPlugIn *plugin)
{
	int ret = EXIT_FAILURE;
	MuxRecorder *muxRecorder = &_muxRecorder;
	memset(muxRecorder, 0, sizeof(MuxRecorder));

	plugin->priv = muxRecorder;
	plugin->type = MUX_PLUGIN_TYPE_RECORDER;
	plugin->signalExit = _muxRecorderSignalExit;
	plugin->reportEvent = _muxRecorderReportEvent;
	plugin->destory = _muxRecorderDestory;
	snprintf(plugin->name, sizeof(plugin->name), "%s", CMN_MODULE_RECORDER_NAME );
	snprintf(plugin->version, sizeof(plugin->version), "%s", CMN_VERSION_INFO(CMN_MODULE_RECORDER_NAME) );
	muxRecorder->priv = plugin->muxMain;

	if ((ret = cmnMuxConfigParseRecord(MUX_RECORDER_CONFIG_FILE, &muxRecorder->config)) < 0)
	{
		fprintf(stderr, "Error reading configuration file '%s': %s\n", MUX_RECORDER_CONFIG_FILE, av_err2str(ret));
		_muxRecorderDestory(plugin);
		return EXIT_FAILURE;
	}

	MUX_INIT_FFMPEG(CMN_LOG_WARNING);

	ret = muxRecorderInitConsumer(muxRecorder);
	if(ret != EXIT_SUCCESS)
	{
		MUX_RECORD_ERROR("Media consumer of RECORD initialization failed");
		return ret;
	}

	ret = plugin->muxMain->initThread(plugin->muxMain, &threadRecorder, muxRecorder);
	if(ret < 0)
	{
		MUX_RECORD_ERROR("Recorder thread initialization failed: %d", ret);
		exit(1);
	}
	
	return EXIT_SUCCESS;
}

