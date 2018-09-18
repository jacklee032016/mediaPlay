/*
 *
 */

#include "muxWeb.h"

/*
#define USE_TCPNODELAY
#define NO_RATE_LIMIT
#define DIE_ON_ERROR_TUNING_SNDBUF
*/

#if WEB_OPTIONS_DEBUG_FSM
static const char* _eventName(WEB_EVENT_T _event)
{
	switch (_event)
	{
		case WEB_EVENT_NONE:
			return "WEB_EVENT_NONE";
		case WEB_EVENT_READY:
			return "WEB_EVENT_READY";
		case WEB_EVENT_TIMEOUT:
			return "WEB_EVENT_TIMEOUT";
		case WEB_EVENT_ERROR:
			return "WEB_EVENT_ERROR";
		default:
			return "UNKNOWN EVENT";	
	}

	return NULL;
}
#endif


const char* ecpwsStateName(WEB_CLIENT_STATUS _status)
{
	switch (_status)
	{
		case WEB_CLIENT_S_CONTINUE:
			return "WEB_CLIENT_S_CONTINUE";
		case WEB_CLIENT_S_READ_HEADER:
			return "WEB_CLIENT_S_READ_HEADER";
		case WEB_CLIENT_S_HTTP_WRITE:
			return "WEB_CLIENT_S_HTTP_WRITE";
		case WEB_CLIENT_S_BODY_READ:
			return "WEB_CLIENT_S_BODY_READ";
		case WEB_CLIENT_S_BODY_WRITE:
			return "WEB_CLIENT_S_BODY_WRITE";
		case WEB_CLIENT_S_CGI_READ:
			return "WEB_CLIENT_S_CGI_READ";
		case WEB_CLIENT_S_CGI_WRITE:
			return "WEB_CLIENT_S_CGI_WRITE";
		case WEB_CLIENT_S_IO_SHUFFLE:
			return "IO_SHUFFLE";
		case WEB_CLIENT_S_DONE:
			return "WEB_CLIENT_S_DONE";
		case WEB_CLIENT_S_ERROR:
		default:
			return "WEB_CLIENT_S_ERROR";	
	}

	return NULL;
}

static fsm_event_t readRequestEvents[]=
{
	{
		WEB_EVENT_READY,
		ecpwsHttpHandlerReadHeader,
	},
	{
		WEB_EVENT_TIMEOUT,
		ecpwsHandlerTimeout,	
	},
};

static fsm_event_t writeStaticEvents[]=
{
	{
		WEB_EVENT_READY,
		ecpwsHttpHandlerWriteHtml,
	},
};

static fsm_event_t cgiReadEvents[]=
{
	{
		WEB_EVENT_READY,
		ecpwsCgiHandlerReadPipe,
	},
};

static fsm_event_t cgiWriteEvents[]=
{
	{
		WEB_EVENT_READY,
		ecpwsCgiHandlerSendout,
	},
};

static fsm_event_t postReadEvents[]=
{
	{
		WEB_EVENT_READY,
		ecpwsHttpHandlerReadPost,
	},
};

static fsm_event_t postWriteEvents[]=
{
	{
		WEB_EVENT_READY,
		ecpwsHttpHandlerWritePost,
	},
};

static fsm_event_t errorEvents[]=
{
	{
		WEB_EVENT_READY,
		ecpwsHttpHandlerError,
	},
};

static fsm_req_status_t	httpServiceStates[] =
{
	{
		WEB_CLIENT_S_READ_HEADER,
		sizeof( readRequestEvents)/sizeof(fsm_event_t),
		readRequestEvents,
	},
	{
		WEB_CLIENT_S_HTTP_WRITE,
		sizeof( writeStaticEvents)/sizeof(fsm_event_t),
		writeStaticEvents,
	},


	{
		WEB_CLIENT_S_CGI_READ,
		sizeof( cgiReadEvents)/sizeof(fsm_event_t),
		cgiReadEvents,
	},
	{
		WEB_CLIENT_S_CGI_WRITE,
		sizeof( cgiWriteEvents)/sizeof(fsm_event_t),
		cgiWriteEvents,
	},
	{
		WEB_CLIENT_S_BODY_READ,
		sizeof( postReadEvents)/sizeof(fsm_event_t),
		postReadEvents,
	},
	{
		WEB_CLIENT_S_BODY_WRITE,
		sizeof( postWriteEvents)/sizeof(fsm_event_t),
		postWriteEvents,
	},
#if 0	
	{
		WEB_CLIENT_S_IO_SHUFFLE,
#ifdef HAVE_SENDFILE
		io_shuffle_sendfile,
#else
		io_shuffle,
#endif
	},

#endif

	{
		WEB_CLIENT_S_ERROR,
		sizeof( errorEvents)/sizeof(fsm_event_t),
		errorEvents,
	},

};

SERVICE_FSM		_httpFsm =
{
	sizeof( httpServiceStates)/sizeof(fsm_req_status_t),
	httpServiceStates,
};

static WEB_CLIENT_STATUS _fsmHandlers(SVR_WEB *webSvr,  WEB_CLIENT_CONN *conn )
{
	int		i, j;
	WEB_CLIENT_STATUS newState = WEB_CLIENT_S_READ_HEADER;
	SERVICE_FSM *fsm = &_httpFsm;

	for(j=0; j< fsm->size; j++)
	{
		if(fsm->states[j].state == conn->status)
		{
			for(i =0; i< fsm->states[j].size; i++)
			{
				if(fsm->states[j].events[i].event == conn->event )
				{
#if WEB_OPTIONS_DEBUG_FSM
					ECPWS_DEBUG_INFO("CONN%d : handle event %s in state %s\n", conn->index, _eventName(conn->event) , ecpwsStateName(fsm->states[j].state) );
#endif
					newState = fsm->states[j].events[i].handler( conn);
#if WEB_OPTIONS_DEBUG_FSM
					ECPWS_DEBUG_INFO("CONN%d : new state %s\n", conn->index, ecpwsStateName(newState) );
#endif
					break;
				}
			}
		}
	}
	return newState;
}


/*
 * Description: Iterates through the ready queue, passing each request
 * to the appropriate handler for processing.  It monitors the
 * return value from handler functions, all of which return -1
 * to indicate a block, 0 on completion and 1 to remain on the
 * ready list for more procesing.
 */
void process_requests(SVR_WEB *webSvr)
{
	WEB_CLIENT_CONN *current, *next;
	WEB_CLIENT_STATUS newState, oldState;

	current = QUEUE_FIRST_READY(webSvr);
	while (current)
	{
		time(&webSvr->current_time);
		
		next = current->next;
		if(current->event != WEB_EVENT_NONE)
		{
			oldState = current->status;
			newState = _fsmHandlers(webSvr, current);
#if WEB_OPTIONS_DEBUG_FSM
			ECPWS_DEBUG_INFO("Conn %d : From State %s ==> New State %s\n", current->index, ecpwsStateName(oldState), ecpwsStateName(newState) );
#endif
			if( (newState != oldState) && (newState != WEB_CLIENT_S_CONTINUE) )
			{
				current->status = newState;
			}
		}
		
		current->time_last = webSvr->current_time;
		current = next;
	}
	
}

