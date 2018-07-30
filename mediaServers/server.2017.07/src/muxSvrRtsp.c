
#include "muxSvr.h"

/* RTSP handling */

static void __rtspReplyHeader(MuxContext *c, enum RTSPStatusCode error_number)
{
	const char *str;
	time_t ti;
	struct tm *tm;
	char buf2[32];

	str = RTSP_STATUS_CODE2STRING(error_number);
	if (!str)
		str = "Unknown Error";

	avio_printf(c->pb, "RTSP/1.0 %d %s\r\n", error_number, str);
	avio_printf(c->pb, "CSeq: %d\r\n", c->seq);

	/* output GMT time */
	ti = time(NULL);
	tm = gmtime(&ti);
	strftime(buf2, sizeof(buf2), "%a, %d %b %Y %H:%M:%S", tm);
	avio_printf(c->pb, "Date: %s GMT\r\n", buf2);
}

static void _rtspReplyError(MuxContext *c, enum RTSPStatusCode error_number)
{
	__rtspReplyHeader(c, error_number);
	avio_printf(c->pb, "\r\n");
}

static void _rtspCmdOptions(MUX_SVR *muxSvr, MuxContext *c, const char *url)
{
	/* __rtspReplyHeader(c, RTSP_STATUS_OK); */
	avio_printf(c->pb, "RTSP/1.0 %d %s\r\n", RTSP_STATUS_OK, "OK");
	avio_printf(c->pb, "CSeq: %d\r\n", c->seq);
	avio_printf(c->pb, "Public: %s\r\n", "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE");
	avio_printf(c->pb, "\r\n");
}

static void _rtspCmdDescribe(MUX_SVR *muxSvr, MuxContext *c, const char *url)
{
	MuxStream *stream;
	char path1[1024];
	const char *path;
	uint8_t *content;
	int content_length;
	socklen_t len;
	struct sockaddr_in my_addr;

	/* find which URL is asked */
	av_url_split(NULL, 0, NULL, 0, NULL, 0, NULL, path1, sizeof(path1), url);
	path = path1;
	if (*path == '/')
		path++;

	for(stream = muxSvr->config.first_stream; stream; stream = stream->next)
	{
		if (!stream->is_feed && stream->fmt && !strcmp(stream->fmt->name, "rtp") && !strcmp(path, stream->filename))
		{
			goto found;
		}
	}
	/* no stream found */
	_rtspReplyError(c, RTSP_STATUS_NOT_FOUND);
	return;

found:
	/* prepare the media description in SDP format */

	/* get the host IP */
	len = sizeof(my_addr);
	getsockname(c->fd, (struct sockaddr *)&my_addr, &len);
	content_length = muxSvrPrepareSdpDescription(stream, &content, my_addr.sin_addr);
	if (content_length < 0)
	{
		_rtspReplyError(c, RTSP_STATUS_INTERNAL);
		return;
	}
	
	__rtspReplyHeader(c, RTSP_STATUS_OK);
	avio_printf(c->pb, "Content-Base: %s/\r\n", url);
	avio_printf(c->pb, "Content-Type: application/sdp\r\n");
	avio_printf(c->pb, "Content-Length: %d\r\n", content_length);
	avio_printf(c->pb, "\r\n");
	avio_write(c->pb, content, content_length);
	av_free(content);
}

static MuxContext *__findRtpSession(MUX_SVR *muxSvr, const char *session_id)
{
	MuxContext *c;

	if (session_id[0] == '\0')
		return NULL;

	for(c = muxSvr->firstConnectCtx; c; c = c->next)
	{
		if (!strcmp(c->session_id, session_id))
			return c;
	}
	return NULL;
}

static RTSPTransportField *__findTransport(RTSPMessageHeader *h, enum RTSPLowerTransport lower_transport)
{
	RTSPTransportField *th;
	int i;

	for(i=0;i<h->nb_transports;i++)
	{
		th = &h->transports[i];
		if (th->lower_transport == lower_transport)
			return th;
	}
	return NULL;
}

static void _rtspCmdSetup(MUX_SVR *muxSvr, MuxContext *c, const char *url, RTSPMessageHeader *h)
{
	MuxStream *stream;
	int stream_index, rtp_port, rtcp_port;
	char buf[1024];
	char path1[1024];
	const char *path;
	MuxContext *rtp_c;
	RTSPTransportField *th;
	struct sockaddr_in dest_addr;
	RTSPActionServerSetup setup;

VTRACE();
	/* find which URL is asked */
	av_url_split(NULL, 0, NULL, 0, NULL, 0, NULL, path1, sizeof(path1), url);
	path = path1;
	if (*path == '/')
		path++;

	/* now check each stream */
	for(stream = muxSvr->config.first_stream; stream; stream = stream->next)
	{
		if (stream->is_feed || !stream->fmt ||strcmp(stream->fmt->name, "rtp"))
		{
			continue;
		}
		
		/* accept aggregate filenames only if single stream */
		if (!strcmp(path, stream->filename))
		{
			if (stream->nb_streams != 1)
			{
				_rtspReplyError(c, RTSP_STATUS_AGGREGATE);
				return;
			}
			stream_index = 0;
			goto found;
		}

		for(stream_index = 0; stream_index < stream->nb_streams; 	stream_index++)
		{
			snprintf(buf, sizeof(buf), "%s/streamid=%d", stream->filename, stream_index);
			if (!strcmp(path, buf))
				goto found;
		}
	}
	
	/* no stream found */
	_rtspReplyError(c, RTSP_STATUS_SERVICE); /* XXX: right error ? */
	return;
found:

	/* generate session id if needed */
	if (h->session_id[0] == '\0')
	{
		unsigned random0 = av_lfg_get(&muxSvr->random_state);
		unsigned random1 = av_lfg_get(&muxSvr->random_state);
		snprintf(h->session_id, sizeof(h->session_id), "%08x%08x", random0, random1);
	}

	/* find RTP session, and create it if none found */
	rtp_c = __findRtpSession(muxSvr, h->session_id);
	if (!rtp_c)
	{
		/* always prefer UDP */
		th = __findTransport(h, RTSP_LOWER_TRANSPORT_UDP);
		if (!th)
		{
			th = __findTransport(h, RTSP_LOWER_TRANSPORT_TCP);
			if (!th)
			{
				_rtspReplyError(c, RTSP_STATUS_TRANSPORT);
				return;
			}
		}

VTRACE();
		rtp_c = muxSvrConnNewRTP(muxSvr, &c->from_addr, stream, h->session_id, th->lower_transport);
		if (!rtp_c)
		{
			_rtspReplyError(c, RTSP_STATUS_BANDWIDTH);
			return;
		}

		/* open input stream */
		if (muxSvrConnOpenInputStream(rtp_c, "") < 0)
		{
			_rtspReplyError(c, RTSP_STATUS_INTERNAL);
			return;
		}
	}

	/* test if stream is OK (test needed because several SETUP needs
	* to be done for a given file) */
	if (rtp_c->stream != stream)
	{
		_rtspReplyError(c, RTSP_STATUS_SERVICE);
		return;
	}

	/* test if stream is already set up */
	if (rtp_c->rtp_ctx[stream_index])
	{
		_rtspReplyError(c, RTSP_STATUS_STATE);
		return;
	}

	/* check transport */
	th = __findTransport(h, rtp_c->rtp_protocol);
	if (!th || (th->lower_transport == RTSP_LOWER_TRANSPORT_UDP && th->client_port_min <= 0))
	{
		_rtspReplyError(c, RTSP_STATUS_TRANSPORT);
		return;
	}

	/* setup default options */
	setup.transport_option[0] = '\0';
	dest_addr = rtp_c->from_addr;
	dest_addr.sin_port = htons(th->client_port_min);

	/* setup stream */
	if (muxSvrRtpNewAvStream(rtp_c, stream_index, &dest_addr, c) < 0)
	{
		_rtspReplyError(c, RTSP_STATUS_TRANSPORT);
		return;
	}

	/* now everything is OK, so we can send the connection parameters */
	__rtspReplyHeader(c, RTSP_STATUS_OK);
	/* session ID */
	avio_printf(c->pb, "Session: %s\r\n", rtp_c->session_id);

	switch(rtp_c->rtp_protocol)
	{
		case RTSP_LOWER_TRANSPORT_UDP:
			rtp_port = ff_rtp_get_local_rtp_port(rtp_c->rtp_handles[stream_index]);
			rtcp_port = ff_rtp_get_local_rtcp_port(rtp_c->rtp_handles[stream_index]);
			avio_printf(c->pb, "Transport: RTP/AVP/UDP;unicast;" 	"client_port=%d-%d;server_port=%d-%d",
				th->client_port_min, th->client_port_max, rtp_port, rtcp_port);
			break;
			
		case RTSP_LOWER_TRANSPORT_TCP:
			avio_printf(c->pb, "Transport: RTP/AVP/TCP;interleaved=%d-%d", stream_index * 2, stream_index * 2 + 1);
			break;
		
		default:
			break;
	}
	
	if (setup.transport_option[0] != '\0')
		avio_printf(c->pb, ";%s", setup.transport_option);
	avio_printf(c->pb, "\r\n");

	avio_printf(c->pb, "\r\n");
}


/**
 * find an RTP connection by using the session ID. Check consistency with filename
 */
static MuxContext *__findRtpSessionWithUrl(MUX_SVR *muxSvr, const char *url, const char *session_id)
{
	MuxContext *rtp_c;
	char path1[1024];
	const char *path;
	char buf[1024];
	int s, len;

	rtp_c = __findRtpSession(muxSvr, session_id);
	if (!rtp_c)
		return NULL;

	/* find which URL is asked */
	av_url_split(NULL, 0, NULL, 0, NULL, 0, NULL, path1, sizeof(path1), url);
	path = path1;
	if (*path == '/')
		path++;
	if(!strcmp(path, rtp_c->stream->filename))
		return rtp_c;
	
	for(s=0; s<rtp_c->stream->nb_streams; ++s)
	{
		snprintf(buf, sizeof(buf), "%s/streamid=%d", rtp_c->stream->filename, s);
		if(!strncmp(path, buf, sizeof(buf)))
			/* XXX: Should we reply with RTSP_STATUS_ONLY_AGGREGATE
			* if nb_streams>1? */
			return rtp_c;
	}
	
	len = strlen(path);
	if (len > 0 && path[len - 1] == '/' && 	!strncmp(path, rtp_c->stream->filename, len - 1))
		return rtp_c;
	
	return NULL;
}

static void _rtspCmdPlay(MUX_SVR *muxSvr, MuxContext *c, const char *url, RTSPMessageHeader *h)
{
	MuxContext *rtp_c;

VTRACE();
	rtp_c = __findRtpSessionWithUrl(muxSvr, url, h->session_id);
	if (!rtp_c)
	{
		_rtspReplyError(c, RTSP_STATUS_SESSION);
		return;
	}

	if (rtp_c->state != HTTPSTATE_SEND_DATA && rtp_c->state != HTTPSTATE_WAIT_FEED && rtp_c->state != HTTPSTATE_READY)
	{
		_rtspReplyError(c, RTSP_STATUS_STATE);
		return;
	}

	rtp_c->state = HTTPSTATE_SEND_DATA;

	/* now everything is OK, so we can send the connection parameters */
	__rtspReplyHeader(c, RTSP_STATUS_OK);
	/* session ID */
	avio_printf(c->pb, "Session: %s\r\n", rtp_c->session_id);
	avio_printf(c->pb, "\r\n");
}

static void _rtspCmdInterrupt(MUX_SVR *muxSvr, MuxContext *c, const char *url, RTSPMessageHeader *h, int pause_only)
{
	MuxContext *rtp_c;

VTRACE();
	rtp_c = __findRtpSessionWithUrl(muxSvr, url, h->session_id);
	if (!rtp_c)
	{
		_rtspReplyError(c, RTSP_STATUS_SESSION);
		return;
	}

	if (pause_only)
	{
		if (rtp_c->state != HTTPSTATE_SEND_DATA && rtp_c->state != HTTPSTATE_WAIT_FEED)
		{
			_rtspReplyError(c, RTSP_STATUS_STATE);
			return;
		}
		rtp_c->state = HTTPSTATE_READY;
		rtp_c->first_pts = AV_NOPTS_VALUE;
	}

	/* now everything is OK, so we can send the connection parameters */
	__rtspReplyHeader(c, RTSP_STATUS_OK);
	/* session ID */
	avio_printf(c->pb, "Session: %s\r\n", rtp_c->session_id);
	avio_printf(c->pb, "\r\n");

	if (!pause_only)
		muxSvrConnClose(rtp_c);
}


int muxSvrRtspParseRequest(MUX_SVR *muxSvr, MuxContext *c)
{
	const char *p, *p1, *p2;
	char cmd[32];
	char url[1024];
	char protocol[32];
	char line[1024];
	int len;
	RTSPMessageHeader header1 = { 0 }, *header = &header1;
 	

	c->buffer_ptr[0] = '\0';
	p = (const char *)c->buffer;

	muxUtilGetWord(cmd, sizeof(cmd), &p);
	muxUtilGetWord(url, sizeof(url), &p);
	muxUtilGetWord(protocol, sizeof(protocol), &p);

	av_strlcpy(c->method, cmd, sizeof(c->method));
	av_strlcpy(c->url, url, sizeof(c->url));
	av_strlcpy(c->protocol, protocol, sizeof(c->protocol));

	if (avio_open_dyn_buf(&c->pb) < 0)
	{/* XXX: cannot do more */
		c->pb = NULL; /* safety */
		return -1;
	}

	/* check version name */
	if (strcmp(protocol, "RTSP/1.0"))
	{
		_rtspReplyError(c, RTSP_STATUS_VERSION);
		goto the_end;
	}

	/* parse each header line */
	/* skip to next line */
	while (*p != '\n' && *p != '\0')
		p++;
	if (*p == '\n')
		p++;
	
	while (*p != '\0')
	{
		p1 = memchr(p, '\n', (char *)c->buffer_ptr - p);
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
		ff_rtsp_parse_line(NULL, header, line, NULL, NULL);
		p = p1 + 1;
	}

	/* handle sequence number */
	c->seq = header->seq;

	if (!strcmp(cmd, "DESCRIBE"))
		_rtspCmdDescribe(muxSvr, c, url);
	else if (!strcmp(cmd, "OPTIONS"))
		_rtspCmdOptions(muxSvr, c, url);
	else if (!strcmp(cmd, "SETUP"))
		_rtspCmdSetup(muxSvr, c, url, header);
	else if (!strcmp(cmd, "PLAY"))
		_rtspCmdPlay(muxSvr, c, url, header);
	else if (!strcmp(cmd, "PAUSE"))
		_rtspCmdInterrupt(muxSvr, c, url, header, 1);
	else if (!strcmp(cmd, "TEARDOWN"))
		_rtspCmdInterrupt(muxSvr, c, url, header, 0);
	else
		_rtspReplyError(c, RTSP_STATUS_METHOD);

the_end:
	len = avio_close_dyn_buf(c->pb, &c->pb_buffer);
	c->pb = NULL; /* safety */
	if (len < 0)
		/* XXX: cannot do more */
		return -1;

	c->buffer_ptr = c->pb_buffer;
	c->buffer_end = c->pb_buffer + len;
	c->state = RTSPSTATE_SEND_REPLY;
	return 0;
}

