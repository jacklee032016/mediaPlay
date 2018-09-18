/*
* $Id$
*/

#include "muxSvr.h"

#include "__services.h"

/* poll in event handler */
static int _svrRtspMsgRecv(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *_event )
{
	const char *p, *p1, *p2;
	int ret;
	char line[1024];
	int len;
	rtsp_event_t	*event;

	if( (ret = muxSvrConnectReceive(svrConn) ) < 0 )
	{
		MUX_SVR_ERROR( "'%s' is closed\n", svrConn->name);
		return MUX_SVR_STATE_CLOSING;
	}
	if(ret == 0)
	{
		return MUX_SVR_STATE_CONTINUE;
	}
			
	svrConn->readBuffer.bufferPtr[0] = '\0';
	p = (char *)svrConn->readBuffer.dBuffer;


	muxUtilGetWord(svrConn->method, sizeof(svrConn->method), &p);
	muxUtilGetWord(svrConn->url, sizeof(svrConn->url), &p);
	muxUtilGetWord(svrConn->protocol, sizeof(svrConn->protocol), &p);
	
	MUX_SVR_DEBUG( "RTSP Parse Request : '%s'(method:%s url:%s protocol:%s)'\n", svrConn->readBuffer.dBuffer, 
		svrConn->method, svrConn->url, svrConn->protocol);
	/* check version name */
	if (strcmp(svrConn->protocol, "RTSP/1.0") != 0)
	{
		rtspReplyHeader(svrConn, RTSP_STATUS_VERSION);
		SVR_BUF_RESET(svrConn->readBuffer);
		return MUX_SVR_STATE_CLOSING;
	}
	
	muxSvrRemoveAutheninfoFromUrl(svrConn->url);

	event = av_mallocz( sizeof(rtsp_event_t));
	event->myEvent.myConn = svrConn;
	
//	pstrcpy(event->url, sizeof(event->url), svrConn->url);
	av_url_split(event->protocol, sizeof(event->protocol), NULL, 0, event->host, sizeof(event->host), &event->port, line, sizeof(line), svrConn->url);
	if(line[0] == '/')
		snprintf(event->path, sizeof(event->path), "%s",  line+1);
	else	
		snprintf(event->path, sizeof(event->path), "%s",  line );

	/* parse each header line : skip to next line */
	while (*p != '\n' && *p != '\0')
		p++;
	
	if (*p == '\n')
		p++;
	
	while (*p != '\0')
	{
		p1 = strchr(p, '\n');
		if (!p1)
			break;
		
		p2 = p1;
		if (p2 > p && p2[-1] == '\r')
			p2--;
		
		/* skip empty line */
		if (p2 == p)
			break;
		
		len = p2 - p;
		if (len > sizeof(line) - 1)
			len = sizeof(line) - 1;
		
		memcpy(line, p, len);
		line[len] = '\0';
		ff_rtsp_parse_line(NULL, &event->header, line, NULL, NULL);
		p = p1 + 1;
	}

	/* handle sequence number */
	svrConn->seq = event->header.seq;

	//memset(svrConn->readBuffer.dBuffer, 0 , svrConn->bufferReadSize);
	svrConn->readBuffer.bufferPtr = svrConn->readBuffer.dBuffer;
	svrConn->readBuffer.bufferEnd = svrConn->readBuffer.dBuffer + svrConn->bufferReadSize;
	
	MUX_SVR_DEBUG( "RTSP REQUEST CMD:'%s'; URL:'%s'; PROTOCOL:'%s'", svrConn->method, svrConn->url, svrConn->protocol );

	if (!strcmp(svrConn->method, "DESCRIBE"))
	{
		event->myEvent.event = RTSP_MSG_DESCRIBE;
	}
	else if (!strcmp(svrConn->method, "OPTIONS"))
	{
		event->myEvent.event = RTSP_MSG_OPTIONS;
	}
	else if (!strcmp(svrConn->method, "SETUP"))
	{
		event->myEvent.event = RTSP_MSG_SETUP;
	}
	else if (!strcmp(svrConn->method, "PLAY"))
	{
		event->myEvent.event = RTSP_MSG_PLAY;
	}
	else if (!strcmp(svrConn->method, "PAUSE"))
	{
		event->myEvent.event = RTSP_MSG_PAUSE;
	}
	else if (!strcmp(svrConn->method, "TEARDOWN"))
	{
		event->myEvent.event = RTSP_MSG_TEARDOWN;
	}
	else
	{
		event->myEvent.event = MUX_SVR_EVENT_ERROR;
		event->myEvent.statusCode = RTSP_STATUS_METHOD;
		MUX_SVR_ERROR( "other RTSP request is look as error : %s", svrConn->method);
	}
//	SVR_BUF_RESET(svrConn->readBuffer);
	SET_FSM_EVENT(event->myEvent.event);


	SVR_REPORT_FSM_EVENT(event);
	
	return MUX_SVR_STATE_CONTINUE;
}

static svr_transition_t	rtspStateInit[] =
{
	{
		MUX_SVR_EVENT_POLL_IN,
		_svrRtspMsgRecv,
	},
	{
		MUX_SVR_EVENT_ERROR,
		rtspEventError,
	},
	{
		MUX_SVR_EVENT_CLOSE,
		rtspEventClosing,
	},
	{
		RTSP_MSG_OPTIONS,
		rtspEventOptions,
	},
	{
		RTSP_MSG_DESCRIBE,
		rtspEventDescribe,
	},
	{
		RTSP_MSG_SETUP,
		rtspEventSetup,
	},
};

static svr_transition_t	rtspStateReady[] =
{
	{
		MUX_SVR_EVENT_POLL_IN,
		_svrRtspMsgRecv,
	},
	{
		MUX_SVR_EVENT_ERROR,
		rtspEventError,
	},
	{
		MUX_SVR_EVENT_CLOSE,
		rtspEventClosing,
	},
	{
		RTSP_MSG_PLAY,
		rtspEventPlay,
	},
	{
		RTSP_MSG_RECORD,
		rtspEventRecord,
	},
	{
		RTSP_MSG_SETUP,
		rtspEventSetup,
	},
};

static svr_transition_t	rtspStatePlay[] =
{
	{
		MUX_SVR_EVENT_POLL_IN,
		_svrRtspMsgRecv,
	},

	{
		MUX_SVR_EVENT_DATA,
		muxSvrConnSendPacket,
	},
	{
		MUX_SVR_EVENT_ERROR,
		rtspEventError,
	},
	{
		MUX_SVR_EVENT_CLOSE,
		rtspEventClosing,
	},
	{/* when playing(data state), reply OPTIONS command from client */
		RTSP_MSG_OPTIONS,
		rtspEventOptions,
	},
	{
		RTSP_MSG_PAUSE,
		rtspEventPause,
	},
	{
		RTSP_MSG_TEARDOWN,
		rtspEventTeardown,
	},
	{
		RTSP_MSG_SETUP,
		rtspEventSetup,
	},
	
	{
		RTSP_MSG_PLAY,
		rtspEventPlay,
	},
};

static svr_transition_t	rtspStateRecord[] =
{
	{
		MUX_SVR_EVENT_POLL_IN,
		_svrRtspMsgRecv,
	},
	{
		MUX_SVR_EVENT_ERROR,
		rtspEventError,
	},
	{
		MUX_SVR_EVENT_CLOSE,
		rtspEventClosing,
	},
	{
		RTSP_MSG_PAUSE,
		rtspEventPause,
	},
	{
		RTSP_MSG_TEARDOWN,
		rtspEventTeardown,
	},
};

static svr_statemachine_t	rtspFsmStates[] =
{
	{
		RTSP_STATE_INIT,
		sizeof(rtspStateInit)/sizeof(svr_transition_t),
		rtspStateInit,
		NULL,// ext_idle_state_enter,
	},
	{
		RTSP_STATE_READY,
		sizeof(rtspStateReady)/sizeof(svr_transition_t),
		rtspStateReady,
		NULL,
	},
	{
		RTSP_STATE_PLAY,
		sizeof(rtspStatePlay)/sizeof(svr_transition_t),
		rtspStatePlay,
		NULL,
	},
	{
		RTSP_STATE_RECORD,
		sizeof(rtspStateRecord)/sizeof(svr_transition_t),
		rtspStateRecord,
		NULL,
	},
};

SVR_SERVICE_FSM		rtspFsm =
{
	sizeof(rtspFsmStates)/sizeof(svr_statemachine_t),
	rtspFsmStates,
};

