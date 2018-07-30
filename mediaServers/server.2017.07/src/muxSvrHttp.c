
#include "muxSvr.h"

enum RedirType
{
	REDIR_NONE,
	REDIR_ASX,
	REDIR_RAM,
	REDIR_ASF,
	REDIR_RTSP,
	REDIR_SDP,
};

static int _httpReplyBusy(MuxContext *c)
{
	uint8_t *q;

	c->http_error = 503;
	q = c->buffer;
	snprintf((char *)q, c->buffer_size,
		"HTTP/1.0 503 Server too busy\r\n"
		"Content-type: text/html\r\n"
		"\r\n"
		"<!DOCTYPE html>\n"
		"<html><head><title>Too busy</title></head><body>\r\n"
		"<p>The server is too busy to serve your request at "
		"this time.</p>\r\n"
		"<p>The bandwidth being served (including your stream) "
		"is %"PRIu64"kbit/s, and this exceeds the limit of "
		"%"PRIu64"kbit/s.</p>\r\n"
		"</body></html>\r\n",
		c->muxSvr->current_bandwidth, c->muxSvr->config.max_bandwidth);
	q += strlen((char *)q);
	/* prepare output buffer */
	c->buffer_ptr = c->buffer;
	c->buffer_end = q;
	c->state = HTTPSTATE_SEND_HEADER;

	return 0;
}

static int _httpReplyMoved(MuxContext *c, MuxStream *stream)
{
	uint8_t *q;
	
	c->http_error = 301;
	q = c->buffer;
	snprintf((char *)q, c->buffer_size,
		"HTTP/1.0 301 Moved\r\n"
		"Location: %s\r\n"
		"Content-type: text/html\r\n"
		"\r\n"
		"<!DOCTYPE html>\n"
		"<html><head><title>Moved</title></head><body>\r\n"
		"You should be <a href=\"%s\">redirected</a>.\r\n"
		"</body></html>\r\n",
		stream->feed_filename, stream->feed_filename);
	q += strlen((char *)q);
	/* prepare output buffer */
	c->buffer_ptr = c->buffer;
	c->buffer_end = q;
	c->state = HTTPSTATE_SEND_HEADER;
	return 0;

}

static int _httpReplyError(MuxContext *c, char *errMsg)
{
	uint8_t *q;
	char *encoded_msg = NULL;
	
	c->http_error = 404;
	q = c->buffer;
	if (!htmlencode(errMsg, &encoded_msg))
	{
		MUX_SVR_LOG("Could not encode filename '%s' as HTML\n", errMsg);
	}
	
	snprintf((char *)q, c->buffer_size,
		"HTTP/1.0 404 Not Found\r\n"
		"Content-type: text/html\r\n"
		"\r\n"
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head>\n"
		"<meta charset=\"UTF-8\">\n"
		"<title>404 Not Found</title>\n"
		"</head>\n"
		"<body>%s</body>\n"
		"</html>\n", encoded_msg? encoded_msg : "File not found");
	q += strlen((char *)q);
	/* prepare output buffer */
	c->buffer_ptr = c->buffer;
	c->buffer_end = q;
	c->state = HTTPSTATE_SEND_HEADER;
	av_freep(&encoded_msg);
	
	return 0;
}


static int _extractRates(char *rates, int ratelen, const char *request)
{
	const char *p;

	for (p = request; *p && *p != '\r' && *p != '\n'; )
	{
		if (av_strncasecmp(p, "Pragma:", 7) == 0)
		{
			const char *q = p + 7;

			while (*q && *q != '\n' && av_isspace(*q))
				q++;

			if (av_strncasecmp(q, "stream-switch-entry=", 20) == 0)
			{
				int stream_no;
				int rate_no;

				q += 20;

				memset(rates, 0xff, ratelen);

				while (1)
				{
					while (*q && *q != '\n' && *q != ':')
						q++;

					if (sscanf(q, ":%d:%d", &stream_no, &rate_no) != 2)
						break;

					stream_no--;
					if (stream_no < ratelen && stream_no >= 0)
						rates[stream_no] = rate_no;

					while (*q && *q != '\n' && !av_isspace(*q))
						q++;
				}

				return 1;
			}
		}
		p = strchr(p, '\n');
		if (!p)
			break;

		p++;
	}

	return 0;
}


static int __findStreamInFeed(MuxStream *feed, AVCodecParameters *codec, int bit_rate)
{
	int i;
	int best_bitrate = 100000000;
	int best = -1;

	for (i = 0; i < feed->nb_streams; i++)
	{
		AVCodecParameters *feed_codec = feed->streams[i]->codecpar;

		if (feed_codec->codec_id != codec->codec_id ||
			feed_codec->sample_rate != codec->sample_rate ||
			feed_codec->width != codec->width ||
			feed_codec->height != codec->height)
			continue;

		/* Potential stream */

		/* We want the fastest stream less than bit_rate, or the slowest
		* faster than bit_rate
		*/

		if (feed_codec->bit_rate <= bit_rate)
		{
			if (best_bitrate > bit_rate ||feed_codec->bit_rate > best_bitrate)
			{
				best_bitrate = feed_codec->bit_rate;
				best = i;
			}
			continue;
		}
		
		if (feed_codec->bit_rate < best_bitrate)
		{
			best_bitrate = feed_codec->bit_rate;
			best = i;
		}
	}
	return best;
}

static int _modifyCurrentStream(MuxContext *c, char *rates)
{
	int i;
	MuxStream *req = c->stream;
	int action_required = 0;

	/* Not much we can do for a feed */
	if (!req->feed)
		return 0;

	for (i = 0; i < req->nb_streams; i++)
	{
		AVCodecParameters *codec = req->streams[i]->codecpar;

		switch(rates[i])
		{
			case 0:
				c->switch_feed_streams[i] = req->feed_streams[i];
				break;
			
			case 1:
				c->switch_feed_streams[i] = __findStreamInFeed(req->feed, codec, codec->bit_rate / 2);
				break;
			
			case 2:
				/* Wants off or slow */
				c->switch_feed_streams[i] = __findStreamInFeed(req->feed, codec, codec->bit_rate / 4);
#ifdef WANTS_OFF
				/* This doesn't work well when it turns off the only stream! */
				c->switch_feed_streams[i] = -2;
				c->feed_streams[i] = -2;
#endif
				break;
			}

		if (c->switch_feed_streams[i] >= 0 && c->switch_feed_streams[i] != c->feed_streams[i])
		{
			action_required = 1;
		}
	}

	return action_required;
}

/**
 * compute the real filename of a file by matching it without its
 * extensions to all the stream's filenames
 */
static void _computeRealFilename(MUX_SVR *muxSvr, char *filename, int max_size)
{
	char file1[1024];
	char file2[1024];
	char *p;
	MuxStream *stream;

	av_strlcpy(file1, filename, sizeof(file1));
	p = strrchr(file1, '.');
	if (p)
		*p = '\0';
	
	for(stream = muxSvr->config.first_stream; stream; stream = stream->next)
	{
		av_strlcpy(file2, stream->filename, sizeof(file2));
		
		MUX_SVR_LOG("stream filename is :'%s'\n", file2);
		p = strrchr(file2, '.');
		if (p)
			*p = '\0';
		
		if (!strcmp(file1, file2))
		{
			av_strlcpy(filename, stream->filename, max_size);
			break;
		}
	}
}

/* parse HTTP request and prepare header */
int muxSvrHttpParseRequest(MUX_SVR *muxSvr, MuxContext *c)
{
	const char *p;
	char *p1;
	enum RedirType redir_type;
	char cmd[32];
	char info[1024], filename[1024];
	char url[1024], *q;
	char protocol[32];
	char msg[1024];
	const char *mime_type;
	MuxStream *stream;
	int i;
	char ratebuf[32];
	const char *useragent = 0;

	p = (const char *)c->buffer;
	muxUtilGetWord(cmd, sizeof(cmd), &p);
	av_strlcpy(c->method, cmd, sizeof(c->method));

	if (!strcmp(cmd, "GET"))
		c->post = 0;
	else if (!strcmp(cmd, "POST"))
		c->post = 1;
	else
		return -1;

	muxUtilGetWord(url, sizeof(url), &p);
	av_strlcpy(c->url, url, sizeof(c->url));

	muxUtilGetWord(protocol, sizeof(protocol), (const char **)&p);
	if (strcmp(protocol, "HTTP/1.0") && strcmp(protocol, "HTTP/1.1"))
		return -1;

	av_strlcpy(c->protocol, protocol, sizeof(c->protocol));

	if (1)//muxSvr->config.debug)
		MUX_SVR_LOG("%s - - New connection: %s %s\n", inet_ntoa(c->from_addr.sin_addr), cmd, url);

	/* find the filename and the optional info string in the request */
	p1 = strchr(url, '?');
	if (p1)
	{
		av_strlcpy(info, p1, sizeof(info));
		*p1 = '\0';
	}
	else
		info[0] = '\0';

	av_strlcpy(filename, url + ((*url == '/') ? 1 : 0), sizeof(filename)-1);

	for (p = (const char *)c->buffer; *p && *p != '\r' && *p != '\n'; )
	{
		if (av_strncasecmp(p, "User-Agent:", 11) == 0)
		{
			useragent = p + 11;
			if (*useragent && *useragent != '\n' && av_isspace(*useragent))
				useragent++;
			break;
		}
		
		p = strchr(p, '\n');
		if (!p)
			break;

		p++;
	}

	redir_type = REDIR_NONE;
	if (av_match_ext(filename, "asx"))
	{
		redir_type = REDIR_ASX;
		filename[strlen(filename)-1] = 'f';
		MUX_SVR_LOG("REDIR ASX\n");
	}
	else if (av_match_ext(filename, "asf") && (!useragent || av_strncasecmp(useragent, "NSPlayer", 8)))
	{
		/* if this isn't WMP or lookalike, return the redirector file */
		redir_type = REDIR_ASF;
		MUX_SVR_LOG("REDIR ASF\n");
	}
	else if (av_match_ext(filename, "rpm,ram"))
	{
		redir_type = REDIR_RAM;
		strcpy(filename + strlen(filename)-2, "m");
		MUX_SVR_LOG("REDIR RAM\n");
	}
	else if (av_match_ext(filename, "rtsp"))
	{
		redir_type = REDIR_RTSP;
		_computeRealFilename(muxSvr,filename, sizeof(filename) - 1);
		MUX_SVR_LOG("REDIR RTSP:'%s'\n", filename);
	}
	else if (av_match_ext(filename, "sdp"))
	{
		redir_type = REDIR_SDP;
		_computeRealFilename(muxSvr, filename, sizeof(filename) - 1);
		MUX_SVR_LOG("REDIR SDP:'%s'\n", filename);
	}

	/* "redirect" request to index.html */
	if (!strlen(filename))
		av_strlcpy(filename, "index.html", sizeof(filename) - 1);

	stream = muxSvr->config.first_stream;
	while (stream)
	{
		if (!strcmp(stream->filename, filename) && muxSvrValidateAcl(stream, c))
			break;
		stream = stream->next;
	}
	
	if (!stream)
	{
		snprintf(msg, sizeof(msg), "File '%s' not found", url);
		MUX_SVR_LOG("File '%s' not found\n", url);
		return _httpReplyError(c, msg);
	}

	c->stream = stream;
	memcpy(c->feed_streams, stream->feed_streams, sizeof(c->feed_streams));
	memset(c->switch_feed_streams, -1, sizeof(c->switch_feed_streams));

	if (stream->stream_type == STREAM_TYPE_REDIRECT)
	{
		return _httpReplyMoved(c, stream);
	}

	/* If this is WMP, get the rate information */
	if (_extractRates(ratebuf, sizeof(ratebuf), (const char *)c->buffer))
	{
		if (_modifyCurrentStream(c, ratebuf))
		{
			for (i = 0; i < MUX_ARRAY_ELEMS(c->feed_streams); i++)
			{
				if (c->switch_feed_streams[i] >= 0)
					c->switch_feed_streams[i] = -1;
			}
		}
	}

	if (c->post == 0 && stream->stream_type == STREAM_TYPE_LIVE)
		muxSvr->current_bandwidth += stream->bandwidth;

	/* If already streaming this feed, do not let another feeder start */
	if (stream->feed_opened)
	{
		snprintf(msg, sizeof(msg), "This feed is already being received.");
		MUX_SVR_LOG("Feed '%s' already being received\n", stream->feed_filename);
		return _httpReplyError(c, msg);
	}

	if (c->post == 0 && muxSvr->config.max_bandwidth < muxSvr->current_bandwidth)
	{
		return _httpReplyBusy(c);
	}

	if (redir_type != REDIR_NONE)
	{
		const char *hostinfo = 0;

		for (p = (const char *)c->buffer; *p && *p != '\r' && *p != '\n'; )
		{
			if (av_strncasecmp(p, "Host:", 5) == 0)
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

			while (av_isspace(*hostinfo))
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
					q = (char *)c->buffer;
					
					switch(redir_type)
					{
						case REDIR_ASX:
							snprintf(q, c->buffer_size, "HTTP/1.0 200 ASX Follows\r\n"
								"Content-type: video/x-ms-asf\r\n"
								"\r\n"
								"<ASX Version=\"3\">\r\n"
								//"<!-- Autogenerated by ffserver -->\r\n"
								"<ENTRY><REF HREF=\"http://%s/%s%s\"/></ENTRY>\r\n"
								"</ASX>\r\n", hostbuf, filename, info);
								
							MUX_SVR_LOG("redirect to ASX:\n%s\n", q);
							q += strlen(q);
							break;
						
						case REDIR_RAM:
							snprintf(q, c->buffer_size, "HTTP/1.0 200 RAM Follows\r\n"
								"Content-type: audio/x-pn-realaudio\r\n"
								"\r\n"
								"# Autogenerated by MuxServer\r\n"
								"http://%s/%s%s\r\n", hostbuf, filename, info);
							
							MUX_SVR_LOG("redirect to RAM:\n%s\n", q);
							q += strlen(q);
							break;
						
						case REDIR_ASF:
							snprintf(q, c->buffer_size,
								"HTTP/1.0 200 ASF Redirect follows\r\n"
								"Content-type: video/x-ms-asf\r\n"
								"\r\n"
								"[Reference]\r\n"
								"Ref1=http://%s/%s%s\r\n", hostbuf, filename, info);
							
							MUX_SVR_LOG("redirect to ASF:\n%s\n", q);
							q += strlen(q);
							break;
						
						case REDIR_RTSP:
						{
							char hostname[256], *p;
							/* extract only hostname */
							av_strlcpy(hostname, hostbuf, sizeof(hostname));
							p = strrchr(hostname, ':');
							if (p)
								*p = '\0';
							snprintf(q, c->buffer_size, "HTTP/1.0 200 RTSP Redirect follows\r\n"
								/* XXX: incorrect MIME type ? */
								"Content-type: application/x-rtsp\r\n"
								"\r\n"
								"rtsp://%s:%d/%s\r\n", hostname, ntohs(muxSvr->config.rtsp_addr.sin_port), filename);
							
							MUX_SVR_LOG("redirect to RTSP:\n%s\n", q);
							q += strlen(q);
						}
						break;
						
						case REDIR_SDP:
						{
							uint8_t *sdp_data;
							int sdp_data_size;
							socklen_t len;
							struct sockaddr_in my_addr;

							snprintf(q, c->buffer_size, "HTTP/1.0 200 OK\r\n"
								"Content-type: application/sdp\r\n"
								"\r\n");
							q += strlen(q);

							len = sizeof(my_addr);

							/* XXX: Should probably fail? */
							if (getsockname(c->fd, (struct sockaddr *)&my_addr, &len))
								MUX_SVR_LOG("getsockname() failed\n");

							/* XXX: should use a dynamic buffer */
							sdp_data_size = muxSvrPrepareSdpDescription(stream, &sdp_data, my_addr.sin_addr);
							if (sdp_data_size > 0)
							{
								memcpy(q, sdp_data, sdp_data_size);
								q += sdp_data_size;
								*q = '\0';
								av_freep(&sdp_data);
							}
							MUX_SVR_LOG("redirect to SDP:\n%s\n", c->buffer);
						}
						break;

						default:
							abort();
							break;
					}

					/* prepare output buffer */
					c->buffer_ptr = c->buffer;
					c->buffer_end = (uint8_t *)q;
					c->state = HTTPSTATE_SEND_HEADER;
					return 0;
				}
			}
		}

		snprintf(msg, sizeof(msg), "ASX/RAM file not handled");
		return _httpReplyError(c, msg);
	}

	stream->conns_served++;

	/* XXX: add there authenticate and IP match */

	if (c->post)
	{
		/* if post, it means a feed is being sent */
		if (!stream->is_feed)
		{
			/* However it might be a status report from WMP! Let us log the
			* data as it might come handy one day. */
			const char *logline = 0;
			int client_id = 0;

			for (p = (const char *)c->buffer; *p && *p != '\r' && *p != '\n'; )
			{
				if (av_strncasecmp(p, "Pragma: log-line=", 17) == 0)
				{
					logline = p;
					break;
				}
				
				if (av_strncasecmp(p, "Pragma: client-id=", 18) == 0)
					client_id = strtol(p + 18, 0, 10);
				p = strchr(p, '\n');
				if (!p)
					break;

				p++;
			}

			if (logline)
			{
				char *eol = strchr(logline, '\n');

				logline += 17;

				if (eol)
				{
					if (eol[-1] == '\r')
						eol--;
					MUX_SVR_LOG("%.*s\n", (int) (eol - logline), logline);
					c->suppress_log = 1;
				}
			}

#ifdef DEBUG
			MUX_SVR_LOG("\nGot request:\n%s\n", c->buffer);
#endif

			if (client_id && _extractRates(ratebuf, sizeof(ratebuf), (const char *)c->buffer))
			{
				MuxContext *wmpc;

				/* Now we have to find the client_id */
				for (wmpc = muxSvr->firstConnectCtx; wmpc; wmpc = wmpc->next)
				{
					if (wmpc->wmp_client_id == client_id)
						break;
				}

				if (wmpc && _modifyCurrentStream(wmpc, ratebuf))
					wmpc->switch_pending = 1;
			}

			snprintf(msg, sizeof(msg), "POST command not handled");
			c->stream = 0;
			return _httpReplyError(c, msg);
		}

		if (muxSvrStartReceiveData(c) < 0)
		{
			snprintf(msg, sizeof(msg), "could not open feed");
			return _httpReplyError(c, msg);
		}
		
		c->http_error = 0;
		c->state = HTTPSTATE_RECEIVE_DATA;
		return 0;
	}

#ifdef DEBUG
	if (strcmp(stream->filename + strlen(stream->filename) - 4, ".asf") == 0)
		MUX_SVR_LOG("\nGot request:\n%s\n", c->buffer);
#endif

	if (c->stream->stream_type == STREAM_TYPE_STATUS)
		goto send_status;

	/* open input stream */
	if (muxSvrConnOpenInputStream(c, info) < 0)
	{
		snprintf(msg, sizeof(msg), "Input stream corresponding to '%s' not found", url);
		return _httpReplyError(c, msg);
	}

	MUX_SVR_LOG("Live stream: '%s'\n", stream->filename);
	
	/* prepare HTTP header */
	c->buffer[0] = 0;
	av_strlcatf((char *)c->buffer, c->buffer_size, "HTTP/1.0 200 OK\r\n");
	mime_type = c->stream->fmt->mime_type;
	if (!mime_type)
		mime_type = "application/x-octet-stream";
	av_strlcatf((char *)c->buffer, c->buffer_size, "Pragma: no-cache\r\n");

	/* for asf, we need extra headers */
	if (!strcmp(c->stream->fmt->name,"asf_stream"))
	{
		/* Need to allocate a client id */
		c->wmp_client_id = av_lfg_get(&muxSvr->random_state);
		av_strlcatf((char *)c->buffer, c->buffer_size, "Server: Cougar 4.1.0.3923\r\nCache-Control: no-cache\r\nPragma: client-id=%d\r\nPragma: features=\"broadcast\"\r\n", c->wmp_client_id);
	}
	
	av_strlcatf((char *)c->buffer, c->buffer_size, "Content-Type: %s\r\n", mime_type);
	av_strlcatf((char *)c->buffer, c->buffer_size, "\r\n");
	q = (char *)c->buffer + strlen((char *)c->buffer);

	/* prepare output buffer */
	c->http_error = 0;
	c->buffer_ptr = c->buffer;
	c->buffer_end = (uint8_t *)q;
	c->state = HTTPSTATE_SEND_HEADER;
	return 0;

send_status:
VTRACE();
	muxSvrHttpStatusPage(muxSvr, c);
	/* horrible: we use this value to avoid
	* going to the send data state */
	c->http_error = 200;
	c->state = HTTPSTATE_SEND_HEADER;
	return 0;
}


