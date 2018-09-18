/* 
* Recorder
*/
#ifndef	__MUX_RECORDER_H__
#define	__MUX_RECORDER_H__

struct _MuxRecorder;


typedef	enum
{
	MUX_RECORDER_STATUS_INIT = 0,
	MUX_RECORDER_STATUS_RECORDING,
	MUX_RECORDER_STATUS_PAUSE,
	MUX_RECORDER_STATUS_STOPPED,
	MUX_RECORDER_SYATUS_UNKNOWN
}MUX_RECORDER_STATUS;



typedef	struct	MuxRecordTask
{
	char						filename[CMN_NAME_LENGTH];

	MUX_RECORDER_STATUS	status;

	AVFormatContext 			*outCtx;		/*  */
	AVOutputFormat			*outFormat;

	
	int						recordedTime;	/* in ms */
	int						startTime;

	int64_t					videoBytes;
	int						videoPkts;

	int64_t					audioBytes;
	int						audioPkts;

	int64_t					subtitleBytes;
	int						subtitlePkts;
}MuxRecordTask;

// a wrapper around a single output AVStream
typedef struct _MuxRecorder
{
	char						name[CMN_NAME_LENGTH];

	CmnMuxRecordConfig		config;

	/* parameters from client API */
	char						mediaFile[CMN_NAME_LENGTH];		/* media file saved */
	int						duration;	/*in msecond */

	/* following field is for Capture Feed*/
	MuxMediaConsumer		*mediaConsumer;
	
	cmn_mutex_t				*mutexLock;  /* lock between recorder thread (start/stop recording task) and capture thread (save data) */


	MuxRecordTask			*currentTask;

	int						gopSize;

	void						*priv;		/* MuxMain */
}MuxRecorder;

#define  MUX_RECORD_DEBUG(...)		{CMN_MSG_DEBUG(CMN_LOG_DEBUG, __VA_ARGS__);}

#define  MUX_RECORD_INFO(...)		{CMN_MSG_LOG(CMN_LOG_INFO, __VA_ARGS__);}

#define  MUX_RECORD_WARN(...)		{CMN_MSG_LOG(CMN_LOG_WARNING, __VA_ARGS__);}

#define  MUX_RECORD_ERROR(...)		{CMN_MSG_LOG(CMN_LOG_ERR, __VA_ARGS__);}


#endif

