/*
* $Id$
*/

#include "muxSvr.h"

int _msParseGetIpAddress(struct in_addr *ipAddress, const char *p)
{
	if (!inet_aton(p, ipAddress))
	{
		fprintf(stderr, "Invalid IP address: %s\n", p);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/* initial operation involved HTTP and RTSP serives defined in this file */

/* new connection is accept in our service port(HTTP and RTSP) */
int _mntrConnPollIn(SERVER_CONNECT *listenConn, MUX_SVR_EVENT *event)
{
	struct sockaddr_in fromAddr;
	int fd, len = sizeof(struct sockaddr_in);
	SERVER_CONNECT *svrConn = NULL;

	MUX_SVR_DEBUG( "New Connection from '%s' has come", listenConn->name );
	memset(&fromAddr, 0, sizeof(struct sockaddr_in));
	fd = accept(listenConn->fd,  (struct sockaddr *)&fromAddr, (unsigned int *)&len);
	if (fd < 0)
		return -EXIT_FAILURE;

	fcntl(fd, F_SETFL, O_NONBLOCK);

	if(listenConn->muxSvr->nbConnections >= listenConn->muxSvr->svrConfig.maxConnections)
	{
		MUX_SVR_ERROR( "Connection upper Limit '%d' has reached", listenConn->muxSvr->svrConfig.maxConnections);
		close(fd);
		return -EXIT_FAILURE;
	}

	svrConn = av_mallocz( sizeof(SERVER_CONNECT));
	if(svrConn == NULL)
	{
		close(fd);
		return -EXIT_FAILURE;
	}

	svrConn->type = listenConn->type;
	svrConn->fd = fd;
	svrConn->muxSvr= listenConn->muxSvr;
	svrConn->myListen = listenConn;
	memcpy(&svrConn->peerAddress, &fromAddr, sizeof(struct sockaddr_in) );

	snprintf(svrConn->name, sizeof(svrConn->name), "%s://%s:%d", (listenConn->type==CONN_TYPE_RTSP)?"RTSP":"HTTP",
		inet_ntoa(svrConn->peerAddress.sin_addr), ntohs(svrConn->peerAddress.sin_port) );

	svrConn->bufferReadSize = IOBUFFER_INIT_SIZE;
	svrConn->readBuffer.dBuffer= av_mallocz(svrConn->bufferReadSize);
	if (!svrConn->readBuffer.dBuffer )
	{
		av_free(svrConn);
		svrConn = NULL;
		return -EXIT_FAILURE;
	}

VTRACE();	
	svrConn->readBuffer.bufferPtr = svrConn->readBuffer.dBuffer;
	svrConn->readBuffer.bufferEnd = svrConn->readBuffer.dBuffer + svrConn->bufferReadSize - 1; /* leave room for '\0' */

//	svrConn->fifo = av_mallocz(sizeof(cmn_fifo_t) );
	svrConn->lastPkt = NULL;
	cmn_fifo_init(svrConn->muxSvr->svrConfig.fifoSize, &svrConn->fifo);
	
	svrConn->timeout = listenConn->muxSvr->currentTime +  (listenConn->type == CONN_TYPE_RTSP)?RTSP_REQUEST_TIMEOUT:HTTP_REQUEST_TIMEOUT;
	svrConn->startTime = svrConn->muxSvr->currentTime;
	svrConn->firstPts = AV_NOPTS_VALUE;
	

	if(listenConn->type == CONN_TYPE_RTSP)
	{
//		svrConn->handle = msSvrRtspMsgRecv;
		svrConn->fsm = &rtspFsm;
		svrConn->state = RTSP_STATE_INIT;
	}
	else
	{
//		svrConn->handle = msSvrHttpMsgRecv;
		svrConn->fsm = &httpFsm;
		svrConn->state = HTTP_STATE_INIT;
	}

	if (avio_open_dyn_buf( &svrConn->dynaBuffer ) < 0)
	{/* XXX: cannot do more */
		return -EXIT_FAILURE;
	}
	svrConn->isInitedDynaBuf = TRUE;

#if 0
	MUX_ADD_SVR_CONNECTION(conn->muxSvr, svrConn);
#else	
	cmn_list_append(&(listenConn->muxSvr)->serverConns, svrConn);
#endif
	MUX_SVR_DEBUG( "New Connection of '%s' has come", svrConn->name );

	listenConn->muxSvr->nbConnections++;
	return EXIT_SUCCESS;
}


static int _mntrConnStop(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	close(svrConn->fd);
	svrConn->fd = -1;
	svrConn->muxSvr->nbConnections--;
	
	SVR_BUF_RESET(svrConn->readBuffer);
	SVR_BUF_RESET(svrConn->cmdBuffer);
	SVR_BUF_RESET(svrConn->dataBuffer);
	
	return HTTP_STATE_INIT;
}

static int _mntrConnStart(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	ip_config_t *address = (ip_config_t *)event->data;
	
	int serverFd, tmp;
	struct sockaddr_in serverAddr;

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons (address->port);

	if(strlen(address->address) == 0)
		serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
	else
		_msParseGetIpAddress(&serverAddr.sin_addr, address->address );
	
	serverFd = socket(AF_INET,SOCK_STREAM,0);
	if (serverFd < 0)
	{
		MUX_SVR_ERROR( "Build Socket failed : %s", strerror(errno) );
		return HTTP_STATE_ERROR;
	}

	tmp = 1;
	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)))
	{
		MUX_SVR_ERROR( "setsockopt SO_REUSEADDR failed\n");
	}

	if (bind (serverFd, (struct sockaddr *) &serverAddr, sizeof (serverAddr) ) < 0)
	{
		MUX_SVR_ERROR( "Socket Bind failed on bind(port %d): %s", address->port, strerror(errno) );
		close(serverFd);
		return HTTP_STATE_ERROR;
	}

	if (listen (serverFd, 5) < 0)
	{
		MUX_SVR_ERROR("Socket Listen failed : %s", strerror(errno) );
		close(serverFd);
		return HTTP_STATE_ERROR;
	}

	if (ff_socket_nonblock(serverFd, 1) < 0)
	{
		MUX_SVR_ERROR( "ff_socket_nonblock failed\n");
	}
	svrConn->fd = serverFd;

	muxSvrGetHostIP(svrConn);

	MUX_SVR_DEBUG( "Monitoring IP : %s(%s):%d on socket %d", address->address, inet_ntoa(svrConn->muxSvr->myAddr.sin_addr), address->port, svrConn->fd );
	
	return HTTP_STATE_DATA;
}



static svr_transition_t	_monitorStateInit[] =
{
	{
		MUX_SVR_EVENT_START,
		_mntrConnStart,
	},
};


static svr_transition_t	_monitorStateData[] =
{
	{
		MUX_SVR_EVENT_POLL_IN,
		_mntrConnPollIn,
	},
	{
		MUX_SVR_EVENT_ERROR,
		_mntrConnStop,
	},
	{
		MUX_SVR_EVENT_CLOSE,
		_mntrConnStop,
	},
};


static svr_transition_t	_monitorStateError[] =
{
	{
		MUX_SVR_EVENT_ERROR,
		_mntrConnStop,
	},
	{
		MUX_SVR_EVENT_CLOSE,
		_mntrConnStop,
	},
};




static svr_statemachine_t	_monitorFsmStates[] =
{
	{
		HTTP_STATE_INIT,
		sizeof(_monitorStateInit)/sizeof(svr_transition_t),
		_monitorStateInit,
		NULL,
	},
	{
		HTTP_STATE_DATA,
		sizeof(_monitorStateData)/sizeof(svr_transition_t),
		_monitorStateData,
		NULL,
	},
	{
		HTTP_STATE_ERROR,
		sizeof(_monitorStateError)/sizeof(svr_transition_t),
		_monitorStateError,
		NULL,
	},
};

static SVR_SERVICE_FSM		_monitorFsm =
{
	sizeof( _monitorFsmStates)/sizeof(svr_statemachine_t),
	_monitorFsmStates,
};


int _addOneMonitorConnect(MUX_SVR *muxSvr, conn_type_t type, ip_config_t *address)
{
	SERVER_CONNECT	*svrConn;

	svrConn = av_mallocz( sizeof(SERVER_CONNECT) );
	if(svrConn==NULL)
		return EXIT_FAILURE;
	
	svrConn->timeout = muxSvr->currentTime +  (type == CONN_TYPE_RTSP)?RTSP_REQUEST_TIMEOUT:HTTP_REQUEST_TIMEOUT;

	snprintf(svrConn->name, sizeof(svrConn->name), "%sMonitor",  (type == CONN_TYPE_RTSP)?"Rtsp":"Http");

	svrConn->fd = -1;
	svrConn->muxSvr = muxSvr;
	svrConn->type = type;
	svrConn->handle = NULL;//__newServerConn;
	
	svrConn->fsm = &_monitorFsm;
	svrConn->state = HTTP_STATE_INIT;

	muxSvrServiceReportEvent(svrConn, MUX_SVR_EVENT_START, 0, address);
	
	/* no lock is needed */
	MUX_ADD_SVR_CONNECTION(muxSvr, svrConn);

	return EXIT_SUCCESS;
}


int muxSvrCtrlInit(CmnThread *th, void *data)
{
	MUX_SVR *muxSvr =(MUX_SVR *)data;
	int ret = 0;
	
	ret = _addOneMonitorConnect(muxSvr, CONN_TYPE_HTTP, &muxSvr->svrConfig.httpAddress );
	ret |= _addOneMonitorConnect(muxSvr, CONN_TYPE_RTSP, &muxSvr->svrConfig.rtspAddress );
	if( ret != EXIT_SUCCESS)
	{
		return -EXIT_FAILURE;
	}

#if WITH_SERVER_WATCHDOG
	if(muxSvr->svrConfig.enableWd)
	{
		muxSvr->wdFd = open(WATCHDOG_DEVICE,O_RDONLY);		
		if(muxSvr->wdFd < 0)
		{
			MUX_SVR_ERROR( "Open watchdog failed\n");
			exit(1);
		}
	
		if(muxSvr->svrConfig.wdTimeout < WDT_MIN_TIME || muxSvr->svrConfig.wdTimeout > WDT_MAX_TIME)
		{
			MUX_SVR_ERROR( "WatchDogTimer is not support %ds, now use default timeout 5s\n", muxSvr->svrConfig.wdTimeout);
			fprintf(stderr, "WatchDogTimer is not support %ds, now use default timeout 5s\n", muxSvr->svrConfig.wdTimeout);
			muxSvr->svrConfig.wdTimeout = 5;
		}
		
		setTime_WDT_IOCTL(muxSvr->wdFd, muxSvr->svrConfig.wdTimeout);
		feedDog_WDT_IOCTL(muxSvr->wdFd);
		
		muxSvr->wdStart = TRUE;
	}
#endif

	th->data = data;

	return EXIT_SUCCESS;
}


