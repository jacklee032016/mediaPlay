/*
* $Id$
* HTTP and RTSP Server handlers defined in this file
*/

#include "muxSvr.h"


/* read a key without blocking */
static int read_key(void)
{
	int n = 1;
	unsigned char ch;
	
#ifndef CONFIG_BEOS_NETSERVER
	struct timeval tv;
	fd_set rfds;

	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	n = select(1, &rfds, NULL, NULL, &tv);
#endif
	if (n > 0)
	{
		n = read(0, &ch, 1);
		if (n == 1)
			return ch;

		return n;
	}
	return -1;
}

int __handle_key_input(MUX_SVR *muxSvr)
{
	int 	key;
#if 0
	//, i;
	int bit_rate[]={1024,768,512,256,128,64,32,16};
#endif
	static int index = 0;
	
	if (!muxSvr->svrConfig.serverLog.isDaemonized )
	{/* read_key() returns 0 on EOF */
		key = read_key();
		if (key == 'q')
		{
			SYSTEM_SIGNAL_EXIT();
			return 1;
		}

#if 0
		if (key == 'r')
		{
			static int multiple = 1;
			printf("key pressed is r\n");
			fprintf(stderr,"key pressed is r\n");
			if(multiple > 30)
			{
				multiple = 1;
			}
		}
		
		if(key == 'm')
		{
			motion_detection = !motion_detection;
		}
#endif
#if 1
		if(key == 'b')
		{
#if 0
			AVStream *avStream = mpegFoundAvStream4IOFile(CODEC_TYPE_VIDEO, TRUE, feed);
			//printf("before set bitrate, quant is %d\n",RateControlGetQ());
			if(avStream != NULL )
			{
				change_bitrate(feed, &avStream->codec, bit_rate[index]);
			}
#endif			
			index++;
			if(index>7)
			{
				index=0;
			}
		}
#endif
	}

	return 0;
}

static void __closeConnection(SERVER_CONNECT *svrConn)
{
	int i;
	AVPacket *pkt;

	int nbStreams;
	AVFormatContext *ctx;
#if 0
	URLContext *h;
#endif

	MUX_SVR_DEBUG( "Closing connection '%s' - -\"%s %s %s\" (%lld bytes)", svrConn->name , 
		svrConn->method, svrConn->url, svrConn->protocol,  svrConn->dataCount);

	VTRACE();
	pkt = (AVPacket *)cmn_fifo_tryget(svrConn->fifo);
	VTRACE();
	while(pkt)
	{
		av_packet_free(&pkt);
		pkt = (AVPacket *)cmn_fifo_tryget(svrConn->fifo);
	}
	
	cmn_fifo_free(svrConn->fifo);
	
	VTRACE();
	if(svrConn->isInitedDynaBuf == TRUE)
	{
		avio_close_dyn_buf(svrConn->dynaBuffer, &svrConn->cmdBuffer.dBuffer);
		av_free(svrConn->cmdBuffer.dBuffer);
	}

	VTRACE();
	/* free RTP output streams if any */
	nbStreams = 0;
	if (svrConn->stream && ! IS_STREAM_TYPE(svrConn->stream, MUX_STREAM_TYPE_STATUS)) 
	{
		nbStreams = svrConn->stream->feed->nbInStreams;
	}

#if 1
	for(i=0;i<nbStreams;i++)
	{
		DATA_CONNECT *dataConn = svrConn->dataConns[i];
		if(dataConn)
		{
			MUX_SVR_DEBUG("'%s' : No.%d data conn is destroying", svrConn->name, i);
			muxSvrRtpDataConnClose(dataConn);
		}
	}

	if (svrConn->stream && ! IS_STREAM_TYPE(svrConn->stream, MUX_STREAM_TYPE_STATUS))
		svrConn->muxSvr->currentBandwidth -= svrConn->stream->bandwidth;
	
//	av_freep(&c->pb_buffer);
//	av_freep(&c->packet_buffer);
#endif

	VTRACE();
	if(!IS_SVR_CONN_TYPE(svrConn, CONN_TYPE_RTSP) && svrConn->outFmtCtx )
	{
		ctx = svrConn->outFmtCtx;
	VTRACE();

		
	 	if (0)//Conn->last_packet_sent)
		{/* can't send any data out, even call following function */
			if ( ctx->oformat)
			{
				if (avio_open_dyn_buf(&ctx->pb) >= 0)
				{
	VTRACE();
					av_write_trailer(ctx);
	VTRACE();
					avio_close_dyn_buf(ctx->pb, &svrConn->dataBuffer.dBuffer);
				}
			}
		}

 #if 0
		VTRACE();
		for(i=0; i< ctx->nb_streams; i++) 
			av_free(ctx->streams[i]) ; 
#else
		/* free Ctx and its all AVStreams */
	VTRACE();
		avformat_free_context(ctx);
#endif
	}

	/* remove connection associated resources after av_write_trailer */
	if (svrConn->fd >= 0)
		close(svrConn->fd);

	VTRACE();
	if(svrConn->readBuffer.dBuffer)
	{
		av_freep(&svrConn->readBuffer.dBuffer);
	}
	svrConn->muxSvr->nbConnections--;

	VTRACE();
}

void muxSvrCloseConnect(MUX_SVR *muxSvr)
{
	SERVER_CONNECT *svrConn;
	int i;
	int svrCount = cmn_list_size(&muxSvr->serverConns) ;
	
	SERVER_CONNS_LOCK(muxSvr);
	for(i = 0; i < cmn_list_size(&muxSvr->serverConns) ; i++)
	{
		svrConn = (SERVER_CONNECT *)cmn_list_get(&muxSvr->serverConns, i);
		if(IS_SERVICE_CONNECT(svrConn) )
		{
		
			if( svrConn->state == MUX_SVR_STATE_CLOSING )
			{/* close and free the connection */
			
	VTRACE();
				/* first unload from queues*/
				cmn_list_remove(&muxSvr->serverConns, i);
	VTRACE();
				if (svrConn->stream && ! IS_STREAM_TYPE(svrConn->stream, MUX_STREAM_TYPE_STATUS)) 
				{
					DISABLE_SVR_CONN(svrConn->stream->feed, svrConn);
				}

	VTRACE();
				/* free resources used for this svrConn */
				__closeConnection(svrConn);

				av_free(svrConn);

				svrConn = NULL;

				/* continue other closing connection when next loop */
//					break;
			}
		}
		
	}
	
	SERVER_CONNS_UNLOCK(muxSvr);

	if(svrCount != cmn_list_size(&muxSvr->serverConns))
	{
		MUX_SVR_DEBUG("Number of SvrConn changed to %d from %d", cmn_list_size(&muxSvr->serverConns), svrCount);
	}
}



static void _monitorConnection(SERVER_CONNECT *svrConn)
{
	if(svrConn->fd <= 0 )
		return;

//	MUX_SVR_DEBUG("'%s' is monitored on socket %d", svrConn->name, svrConn->fd);
	FD_SET(svrConn->fd, &svrConn->muxSvr->readFds );
	FD_SET(svrConn->fd, &svrConn->muxSvr->writeFds );
	
	if(svrConn->fd > svrConn->muxSvr->maxFd)
	{
		svrConn->muxSvr->maxFd = svrConn->fd;
	}
}

static void _handleConnection(SERVER_CONNECT *svrConn)
{
	if(svrConn->fd <= 0 )
	{
		return;
	}
	
	if (FD_ISSET(svrConn->fd, &svrConn->muxSvr->readFds))
	{
#if 0
		if(svrConn->handle)
		{
			if(svrConn->handle(svrConn) < 0)
			{/* close this connection (Listen or Server)*/
				if(IS_SERVICE_CONNECT(svrConn))
				{/* link has been broken, so this event is the last event of this link */
//					close(svrConn->fd) ;
//					svrConn->fd = -1;
					HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_CLOSE, 0);
				}
#if 	0			
				else
				{
					MUX_SVR_LOG( "%s listening connection is closed", svrConn->name );
					exit(1);
				}
#endif				
			}
		}
#endif
//		MUX_SVR_DEBUG("'%s' found POLL_IN", svrConn->name );
		HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_POLL_IN, 0);
	}

	if (FD_ISSET(svrConn->fd, &svrConn->muxSvr->writeFds))
	{
//		MUX_SVR_DEBUG("'%s' found POLL_OUT", svrConn->name );
		HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_POLL_OUT, 0);
	}
}


static int __fsmStateHandle(svr_statemachine_t *state, SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event)
{
	int	i;
	svr_transition_t *handle = state->eventHandlers;
	
	for(i=0; i< state->size; i++)
	{
		if(event->event == handle->event )
		{
//			MUX_SVR_DEBUG("FSM of '%s' is handling event of %d in state of %d", svrConn->name, event->event, svrConn->state );
			return (handle->handle)(svrConn, event );
		}
		
		handle++;
	}
	
	return MUX_SVR_STATE_CONTINUE;
}


static int _ctrlFsmOnEvent(CMN_THREAD_INFO *th, void *_event)
{
	int		j;
	int		newState = MUX_SVR_STATE_CONTINUE;

	MUX_SVR_EVENT	*event = (MUX_SVR_EVENT *)_event;
	SERVER_CONNECT *svrConn = event->myConn;
	
	if(event->event == MUX_SVR_EVENT_NONE )
		return -EXIT_FAILURE;

	for(j=0; j< svrConn->fsm->size; j++)
	{
		if(svrConn->fsm->states[j].state == svrConn->state )
		{
			newState = __fsmStateHandle( &svrConn->fsm->states[j], svrConn, event);
			break;
		}
	}
	
//	MUX_SVR_DEBUG("'%s' return new state of %d from state of %d", svrConn->name, newState, svrConn->state );
	if(newState != MUX_SVR_STATE_CONTINUE && newState != svrConn->state )
	{
		for(j=0; j< svrConn->fsm->size; j++)
		{
			if( svrConn->fsm->states[j].state == newState )
			{
				/* clean old timer for this device and new timer is set in enter_handle */
				svrConn->timeout = 0;
				
				if(svrConn->fsm->states[j].enter_handle!= NULL)
					(svrConn->fsm->states[j].enter_handle)( svrConn);
				
				break;
			}
		}
		
		MUX_SVR_DEBUG("'%s' enter into new state of %d from state of %d", svrConn->name, newState, svrConn->state );
		svrConn->state = newState;
	}


	return EXIT_SUCCESS;
}

int _iterateOneFeed(int index, void *ele, void *priv)
{
	MuxFeed *feed = (MuxFeed *)ele;
//	MUX_SVR *muxSvr = (MUX_SVR *)priv;

	SERVER_CONNECT	*svrConn;

	for(svrConn = feed->svrConns; svrConn != NULL; svrConn = svrConn->next)
	{
		if( (IS_SVR_CONN_TYPE(svrConn, CONN_TYPE_RTSP) && svrConn->state == RTSP_STATE_PLAY) ||
			(!IS_SVR_CONN_TYPE(svrConn, CONN_TYPE_RTSP) &&svrConn->state == HTTP_STATE_DATA ) )
		{
			muxSvrServiceReportEvent(svrConn, MUX_SVR_EVENT_DATA, 0, NULL );
		}
	}
	
//VTRACE();

	return EXIT_SUCCESS;
}

static int _muxSvrCtrlMain(CMN_THREAD_INFO *th)
{
	MUX_SVR *muxSvr =(MUX_SVR *)th->data;
	struct timeval		tv;
	int				ret;

	__handle_key_input(muxSvr);
	if( SYSTEM_IS_EXITING() )
	{
		MUX_SVR_LOG( "Server Task recv EXIT signal\n" );
		
#if WITH_SERVER_WATCHDOG
		/* when server normal exit, watchdog exit normally */
		if(muxSvr->wdStart)
		{
			MUX_SVR_LOG( "WatchDog recv EXIT signal\n" );
			disable_WDT_IOCTL(muxSvr->wdFd);
			close(muxSvr->wdFd);
		}
#endif

		return -EXIT_FAILURE;
	}

	FD_ZERO( &muxSvr->readFds );
	FD_ZERO( &muxSvr->writeFds );
	muxSvr->maxFd = -1;

	muxSvrServerConnectionIterate(_monitorConnection, muxSvr );
	
	if(muxSvr->maxFd > 0)
	{
		tv.tv_sec = 0;
//		tv.tv_usec = 100;
		tv.tv_usec = muxSvr->svrConfig.sendDelay;

		if ( ( ret = select( muxSvr->maxFd +1, &muxSvr->readFds, &muxSvr->writeFds, 0, &tv ) ) < 0 )
		{
			MUX_SVR_LOG( "select() returned error %d: %s", ret , strerror(errno));
			/* what is the appropriate thing to do here on EBADF */
			if (errno == EINTR)
			{
			//	break;       /* while(1) */
			}
			else if (errno != EBADF)
			{
				MUX_SVR_LOG("select() failed!");
				exit(1);
			}
		}
		else
		{/*  */
			muxSvrServerConnectionIterate(_handleConnection, muxSvr);
		}
	}

	muxSvr->currentTime = gettime_ms();
	muxSvr->currentTime = av_gettime()/1000;
	
#if WITH_SERVER_WATCHDOG
		if(muxSvr->wdStart)
		{/* feed dog here */
			feedDog_WDT_IOCTL(muxSvr->wdFd);
		}
#endif


	cmn_list_iterate(&muxSvr->svrConfig.feeds, TRUE, _iterateOneFeed, muxSvr );

	/* use up all events in queue one by one in this loop : in one loop, there maybe multiple events for one connection */
	{
		void *event;
		event = cmn_fifo_tryget( &th->queue);

		/* only one event processed in one loop, so performance of send is optimzied */
		while(event )
		{
			ret = _ctrlFsmOnEvent(th, event);
			if(ret < 0)
			{
				MUX_SVR_LOG( "FSM returned error %d, program exit", ret );
				return ret;
			}
			av_free(event);
			
			event = cmn_fifo_tryget( &th->queue);
		}
	}

	/* CLOSE */
	muxSvrCloseConnect(muxSvr);
	
	return EXIT_SUCCESS;
}




CMN_THREAD_INFO  threadController =
{
	name		:	"Controller",
	init			:	muxSvrCtrlInit,
	mainLoop		:	_muxSvrCtrlMain,
	eventHandler	:	NULL,
	destory		:	NULL,
	data			:	NULL,
};


