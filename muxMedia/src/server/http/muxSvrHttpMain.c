/*
* $Id$
*/

#include "muxSvr.h"
#include "__services.h"

/* compute the real filename of a file by matching it without its extensions to all the stream's filenames */
static void _computeRealFilename(MUX_SVR *muxSvr, char *filename, int max_size)
{
	char file1[1024];
	char file2[1024];
	char *p;
	MuxStream *stream;
	int 	i;

	av_strlcpy(file1, filename, sizeof(file1));
	p = strrchr(file1, '.');
	if (p)
		*p = '\0';

	for(i=0 ;i <cmn_list_size(&muxSvr->svrConfig.streams); i++)
	{
		stream = (MuxStream *)cmn_list_get(&muxSvr->svrConfig.streams, i);
		
		av_strlcpy(file2, stream->filename, sizeof(file2));
		
		p = strrchr(file2, '.');
		if (p)
			*p = '\0';
		
		if (!strcmp(file1, file2))
		{
			MUX_SVR_DEBUG("found stream filename is :'%s'\n", stream->filename );
			av_strlcpy(filename, stream->filename, max_size);
			break;
		}
	}
}

static int _httpRedirectParsed(SERVER_CONNECT *svrConn )
{
	char *useragent = NULL;
	char *p;
//	char filename[256];
	enum RedirType redir_type = REDIR_NONE;
	
	for (p = (char *)svrConn->readBuffer.dBuffer; *p && *p != '\r' && *p != '\n'; )
	{
		if (strncasecmp(p, "User-Agent:", 11) == 0)
		{
			useragent = p + 11;
			if (*useragent && *useragent != '\n' && isspace(*useragent))
				useragent++;
			break;
		}
		
		p = strchr(p, '\n');
		if (!p)
			break;

		p++;
	}

#if 0
#if WITH_FFMPEG_2017
	av_url_split(svrConn->protocol, sizeof(svrConn->protocol), NULL, 0, NULL, 0, NULL, svrConn->method, sizeof(svrConn->method), svrConn->url);
#else
	url_split(NULL, 0, svrConn->protocol, sizeof(svrConn->protocol), NULL, svrConn->method, sizeof(svrConn->method), svrConn->url);
#endif
	snprintf(filename, sizeof(filename), "%s", svrConn->method+1);
#endif

	if (av_match_ext(svrConn->filename, "asx"))
	{
		redir_type = REDIR_ASX;
		svrConn->filename[strlen(svrConn->filename)-1] = 'f';
		MUX_SVR_DEBUG("REDIR ASX\n");
	}
	else if (av_match_ext(svrConn->filename, "asf") && (!useragent || av_strncasecmp(useragent, "NSPlayer", 8)))
	{/* if this isn't WMP or lookalike, return the redirector file */
		redir_type = REDIR_ASF;
		MUX_SVR_DEBUG("REDIR ASF\n");
	}
	else if (av_match_ext(svrConn->filename, "rpm,ram"))
	{
		redir_type = REDIR_RAM;
		strcpy(svrConn->filename + strlen(svrConn->filename)-2, "m");
		MUX_SVR_DEBUG("REDIR RAM\n");
	}
	else if (av_match_ext(svrConn->filename, "rtsp"))
	{
		redir_type = REDIR_RTSP;
#if 1		
		_computeRealFilename(svrConn->muxSvr,svrConn->filename, sizeof(svrConn->filename) - 1);
		MUX_SVR_DEBUG("REDIR RTSP:'%s'\n", svrConn->filename);
#else		
		filename[strlen(filename)-5] = '\0';
		snprintf(svrConn->method, sizeof(svrConn->method), "%s.avi", filename );

		MUX_SVR_DEBUG( "redirect filename : '%s(%s)'", svrConn->method, filename );
#endif		
	}
	else if (av_match_ext(svrConn->filename, "sdp"))
	{
		redir_type = REDIR_SDP;
#if 1
		_computeRealFilename(svrConn->muxSvr, svrConn->filename, sizeof(svrConn->filename) - 1);
		MUX_SVR_DEBUG("REDIR SDP:'%s'\n", svrConn->filename);
#else
		filename[strlen(filename)-4] = '\0';
		snprintf(svrConn->method, sizeof(svrConn->method), "%s.avi", filename );
#endif
	}

	return redir_type;

#if 0
	for (p = svrConn->buffer; *p && *p != '\r' && *p != '\n'; )
	{
		if (strncasecmp(p, "Host:", 5) == 0)
		{
			hostinfo = p + 5;
			break;
		}
		
		p = strchr(p, '\n');
		if (!p)
			break;

		p++;
	}

	if (hostinfo)
	{
		char *eoh;
		char hostbuf[260];

		while (isspace(*hostinfo))
			hostinfo++;

		eoh = strchr(hostinfo, '\n');
		if (eoh)
		{
			if (eoh[-1] == '\r')
				eoh--;

			if (eoh - hostinfo < sizeof(hostbuf) - 1)
			{
				memcpy(hostbuf, hostinfo, eoh - hostinfo);
				hostbuf[eoh - hostinfo] = 0;

				c->http_error = 200;
				q = msg;

				return HTTP_EMIT_EVENT(svrConn, HTTP_MSG_REDIRECT_PARSED,  redir_type );
			}
		}

	}
	
	return HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_ERROR,  SRV_HTTP_ERRCODE_REDIRECT_FAILED );
#endif

}


/* Poll In event handler : parse http request and prepare header */
static int svrHttpMsgRecv(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *_event)
{
	char *p;
	int ret;
	int post;
	MuxStream *stream;

	if( (ret = muxSvrConnectReceive(svrConn) ) < 0 )
	{
		MUX_SVR_ERROR( "'%s' is closed\n", svrConn->name);
#if 1
		return MUX_SVR_STATE_CLOSING;
#else
		HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_ERROR, SRV_HTTP_ERRCODE_UNAUTHORIZED);
		return MUX_SVR_STATE_CONTINUE;
#endif		
	}
	if(ret == 0)
	{
		return MUX_SVR_STATE_CONTINUE;
	}

	MUX_SVR_DEBUG( "parse HTTP request from %s : '%s'", inet_ntoa(svrConn->peerAddress.sin_addr), svrConn->readBuffer.dBuffer);
	//fprintf(stderr, "parse HTTP request from %s : '%s'\n", inet_ntoa(svrConn->peerAddress.sin_addr), svrConn->buffer );

#if WITH_HTTP_AUTHENTICATE
	if(svrConn->muxSvr->svrConfig.enableAuthen)
	{
		if(!msSvrHttpAuthenticate(svrConn->buffer))
		{
			HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_ERROR, SRV_HTTP_ERRCODE_UNAUTHORIZED);
			return MUX_SVR_STATE_CONTINUE;
		}
	}
#endif

	p = (char *)svrConn->readBuffer.dBuffer;
	muxUtilGetWord(svrConn->method, sizeof(svrConn->method), (const char **)&p);

	if (!strcmp(svrConn->method, "GET"))
		post = FALSE;
	else if (!strcmp(svrConn->method, "POST"))
	{
		post = TRUE;
	}
	else
	{
		HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_ERROR, SRV_HTTP_ERRCODE_UNSUPPORT_HTTP_METHOD);
		return MUX_SVR_STATE_CONTINUE;
	}

	muxUtilGetWord(svrConn->url, sizeof(svrConn->url), (const char **)&p);
//	muxSvrRemoveAutheninfoFromUrl(url);

	muxUtilGetWord(svrConn->protocol, sizeof(svrConn->protocol), (const char **)&p);
	if (strcmp(svrConn->protocol, "HTTP/1.0") && strcmp(svrConn->protocol, "HTTP/1.1"))
	{
		return HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_ERROR, SRV_HTTP_ERRCODE_UNSUPPORT_PROTOCOL);
	}

	/* find the filename and the optional info string in the request */
	p = svrConn->url;
	if (*p == '/')
		p++;
	
	p = strchr(p, '?');
	if (p)
	{/* info store the http request after '?' */
		av_strlcpy(svrConn->info, p, sizeof(svrConn->info));
		*p = '\0';
	}
	else
	{
		svrConn->info[0] = '\0';
	}

	av_strlcpy(svrConn->filename, svrConn->url + ((*svrConn->url == '/') ? 1 : 0), sizeof(svrConn->filename)-1);

	MUX_SVR_DEBUG("method:'%s'; protocol:'%s'; URL:'%s'; info:'%s'; filename:'%s'", svrConn->method, svrConn->protocol, svrConn->url, svrConn->info, svrConn->filename);
	
#if 0
	memset(svrConn->readBuffer.dBuffer, 0 , svrConn->bufferReadSize);
	svrConn->readBuffer.bufferPtr = svrConn->readBuffer.dBuffer;
	svrConn->readBuffer.bufferEnd = svrConn->readBuffer.dBuffer + svrConn->bufferReadSize;
#endif

	if( (ret = _httpRedirectParsed( svrConn)) != REDIR_NONE )
	{
		HTTP_EMIT_EVENT(svrConn, HTTP_MSG_REDIRECT_PARSED,  ret );
		return MUX_SVR_STATE_CONTINUE;
	}

	/* "redirect" request to index.html */
	if (!strlen(svrConn->filename))
		av_strlcpy(svrConn->filename, "index.html", sizeof(svrConn->filename) - 1);
	
	stream = muxSvrFoundStream(FALSE, svrConn->filename,  &svrConn->muxSvr->svrConfig);
	if (stream == NULL )
	{
		HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_ERROR, SRV_HTTP_ERRCODE_FILE_NOT_FOUND);
		return MUX_SVR_STATE_CONTINUE;
	}

#if WITH_ADDRESS_ACL
	if(stream->acl != NULL && stream->acl->action != MUX_SVR_ACL_DISABLE)
	{
		if(msSvrValidateAcl(stream, svrConn) == 0)
		{
			fprintf(stderr, "Remote IP Address is denied\n");
			HTTP_EMIT_EVENT(svrConn, MUX_SVR_EVENT_ERROR, SRV_HTTP_ERRCODE_IP_ACL_DENY);
			return MUX_SVR_STATE_CONTINUE;
		}
		else
			fprintf(stderr, "Remote IP Address is allowed\n");
	}
#endif
	
	svrConn->stream = stream;
//	memcpy(svrConn->feed_streams, stream->feed_streams, sizeof(serviceConn->feed_streams));
//	memset(svrConn->switch_feed_streams, -1, sizeof(serviceConn->switch_feed_streams));

	if (stream->type == MUX_STREAM_TYPE_REDIRECT)
	{
		HTTP_EMIT_EVENT(svrConn, HTTP_MSG_INFO, SRV_HTTP_INFOCODE_REDIRECT);
		return MUX_SVR_STATE_CONTINUE;
	}

#if 0
	/* If this is WMP, get the rate information */
	if (extract_rates(ratebuf, sizeof(ratebuf), c->buffer))
	{
		if (modify_current_stream(c, ratebuf))
		{
			for (i = 0; i < sizeof(svrConn->feed_streams) / sizeof(svrConn->feed_streams[0]); i++)
			{
				if (svrConn->switch_feed_streams[i] >= 0)
					do_switch_stream(svrConn, i);
			}
		}
	}
#endif

	if (post == FALSE && stream->type == MUX_STREAM_TYPE_LIVE)
	{
		svrConn->muxSvr->currentBandwidth += stream->bandwidth;
	}

	if (post == FALSE && svrConn->muxSvr->svrConfig.maxBandwidth <= svrConn->muxSvr->currentBandwidth)
	{
		HTTP_EMIT_EVENT(svrConn, HTTP_MSG_INFO, SRV_HTTP_INFOCODE_BANDWIDTH_LIMITED);
		return MUX_SVR_STATE_CONTINUE;
	}


	stream->conns_served++;

	/* XXX: add there authenticate and IP match */
	if (post)
	{
		HTTP_EMIT_EVENT(svrConn, HTTP_MSG_POST, 0);
		return MUX_SVR_STATE_CONTINUE;
	}

#ifdef DEBUG_WMP
	if (strcmp(stream->filename + strlen(stream->filename) - 4, ".asf") == 0)
	{
		MUX_SVR_DEBUG( "Got request:\n%s\n", svrConn->buffer);
	}
#endif

	if (svrConn->stream->type == MUX_STREAM_TYPE_STATUS)
	{
//		stream->conns_served--; //
		HTTP_EMIT_EVENT(svrConn, HTTP_MSG_STATUS_PAGE, 0);
		return MUX_SVR_STATE_CONTINUE;
	}

	/* should copy 'info' into event process */
	HTTP_EMIT_EVENT(svrConn, HTTP_MSG_LIVE_STREAM, 0);
	return MUX_SVR_STATE_CONTINUE;
}


static svr_transition_t	httpStateInit[] =
{
	{
		MUX_SVR_EVENT_POLL_IN,
		svrHttpMsgRecv,
	},

#if WITH_SERVER_HTTP_STATS
	{
		HTTP_MSG_STATUS_PAGE,
		httpEventStatusPage,
	},
#endif

	{
		HTTP_MSG_REDIRECT_PARSED,
		httpEventRedirectParsed,
	},

	{
		HTTP_MSG_INFO,
		httpEventInfo,
	},
	{
		MUX_SVR_EVENT_ERROR,
		httpEventError,
	},
	{
		MUX_SVR_EVENT_CLOSE,
		httpEventClosing,
	},
	{
		HTTP_MSG_LIVE_STREAM,
		httpEventLiveStream,
	},
};


static svr_transition_t	httpStateSendData[] =
{
	{
		MUX_SVR_EVENT_POLL_IN,
		svrHttpMsgRecv,
	},
	{
		MUX_SVR_EVENT_POLL_OUT,
		muxSvrConnSendPacket,
	},
/*
	{
		MUX_SVR_EVENT_DATA,
		muxSvrConnSendPacket,
	},
*/
	{
		MUX_SVR_EVENT_ERROR,
		httpEventError,
	},
	{
		MUX_SVR_EVENT_CLOSE,
		httpEventClosing,
	},
	
};

static svr_transition_t	httpStateError[] =
{
/*
	{
		MUX_SVR_EVENT_POLL_IN,
		svrHttpMsgRecv,
	},
	{
		MUX_SVR_EVENT_ERROR,
		httpEventError,
	},
*/	
	{
		MUX_SVR_EVENT_CLOSE,
		httpEventClosing,
	},
};



static svr_statemachine_t	httpFsmStates[] =
{
	{
		HTTP_STATE_INIT,
		sizeof(httpStateInit)/sizeof(svr_transition_t),
		httpStateInit,
		NULL,// ext_idle_state_enter,
	},
	{
		HTTP_STATE_DATA,
		sizeof(httpStateSendData)/sizeof(svr_transition_t),
		httpStateSendData,
		NULL, //httpLiveSendDataHeader,
	},
	{
		HTTP_STATE_ERROR,
		sizeof(httpStateError)/sizeof(svr_transition_t),
		httpStateError,
		NULL, //httpLiveSendDataTailer,
	},
};

SVR_SERVICE_FSM		httpFsm =
{
	sizeof( httpFsmStates)/sizeof(svr_statemachine_t),
	httpFsmStates,
};

