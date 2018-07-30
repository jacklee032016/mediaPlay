
#include "muxSvr.h"

static void _startChildren(MuxStream *feed)
{
	char *pathname;
	char *slash;
	int i;
	size_t cmd_length;

	if (feed->muxSvr->no_launch)
		return;

	cmd_length = strlen(feed->muxSvr->programName);

	/**
	* FIXME: WIP Safeguard. Remove after clearing all harcoded
	* '1024' path lengths
	*/
	if (cmd_length > PATH_LENGTH - 1)
	{
		MUX_SVR_LOG("Could not start children. Command line: '%s' exceeds path length limit (%d)\n", feed->muxSvr->programName, PATH_LENGTH);
		return;
	}

	slash = strrchr(feed->muxSvr->programName, '/');
	if (!slash)
	{
		pathname = av_mallocz(sizeof("ffmpeg"));
	}
	else
	{
		pathname = av_mallocz(slash - feed->muxSvr->programName + sizeof("ffmpeg"));
		if (pathname != NULL)
		{
			memcpy(pathname, feed->muxSvr->programName, slash - feed->muxSvr->programName);
		}
	}
	
	if (!pathname)
	{
		MUX_SVR_LOG("Could not allocate memory for children cmd line\n");
		return;
	}
	/* use "ffmpeg" in the path of current program. Ignore user provided path */

	strcat(pathname, "ffmpeg");

	for (; feed; feed = feed->next)
	{
		if (!feed->child_argv || feed->pid)
			continue;

		feed->pid_start = time(0);

		feed->pid = fork();
		if (feed->pid < 0)
		{
			MUX_SVR_LOG("Unable to create children: %s\n", strerror(errno));
			av_free (pathname);
			exit(EXIT_FAILURE);
		}

		if (feed->pid)
			continue;

		/* In child */

		MUX_SVR_LOG("Launch command line: ");
		MUX_SVR_LOG("%s ", pathname);

		for (i = 1; feed->child_argv[i] && feed->child_argv[i][0]; i++)
			MUX_SVR_LOG("%s ", feed->child_argv[i]);
		MUX_SVR_LOG("\n");

		for (i = 3; i < 256; i++)
			close(i);

		if (!feed->muxSvr->config.debug)
		{
			if (!freopen("/dev/null", "r", stdin))
				MUX_SVR_LOG("failed to redirect STDIN to /dev/null\n;");
			if (!freopen("/dev/null", "w", stdout))
				MUX_SVR_LOG("failed to redirect STDOUT to /dev/null\n;");
			if (!freopen("/dev/null", "w", stderr))
				MUX_SVR_LOG("failed to redirect STDERR to /dev/null\n;");
		}

		signal(SIGPIPE, SIG_DFL);
		execvp(pathname, feed->child_argv);
		av_free (pathname);
		_exit(1);
	}

	av_free (pathname);
}

/* open a listening socket */
static int _openServicePort(struct sockaddr_in *my_addr)
{
	int server_fd, tmp;

	server_fd = socket(AF_INET,SOCK_STREAM,0);
	if (server_fd < 0)
	{
		perror ("socket");
		return -1;
	}

	tmp = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &tmp, sizeof(tmp)))
		av_log(NULL, AV_LOG_WARNING, "setsockopt SO_REUSEADDR failed\n");

	my_addr->sin_family = AF_INET;
	if (bind (server_fd, (struct sockaddr *) my_addr, sizeof (*my_addr)) < 0)
	{
		char bindmsg[32];
		snprintf(bindmsg, sizeof(bindmsg), "bind(port %d)", ntohs(my_addr->sin_port));
		perror (bindmsg);
		goto fail;
	}

	if (listen (server_fd, 5) < 0)
	{
		perror ("listen");
		goto fail;
	}

	if (ff_socket_nonblock(server_fd, 1) < 0)
		av_log(NULL, AV_LOG_WARNING, "ff_socket_nonblock failed\n");

	return server_fd;

fail:
	closesocket(server_fd);
	return -1;
}

/* start all multicast streams */
static void _startMulticast(MUX_SVR *muxSvr)
{
	MuxStream *stream;
	char session_id[32];
	MuxContext *rtp_c;
	struct sockaddr_in dest_addr = {0};
	int default_port, stream_index;
	unsigned int random0, random1;

	default_port = 6000;
	for(stream = muxSvr->config.first_stream; stream; stream = stream->next)
	{

		if (!stream->is_multicast)
			continue;

		random0 = av_lfg_get(&muxSvr->random_state);
		random1 = av_lfg_get(&muxSvr->random_state);

		/* open the RTP connection */
		snprintf(session_id, sizeof(session_id), "%08x%08x", random0, random1);

		/* choose a port if none given */
		if (stream->multicast_port == 0)
		{
			stream->multicast_port = default_port;
			default_port += 100;
		}

		dest_addr.sin_family = AF_INET;
		dest_addr.sin_addr = stream->multicast_ip;
		dest_addr.sin_port = htons(stream->multicast_port);

		rtp_c = muxSvrConnNewRTP(muxSvr, &dest_addr, stream, session_id, RTSP_LOWER_TRANSPORT_UDP_MULTICAST);
		if (!rtp_c)
			continue;

		if (muxSvrConnOpenInputStream(rtp_c, "") < 0)
		{
			MUX_SVR_LOG("Could not open input stream for stream '%s'\n",
			stream->filename);
			continue;
		}

		/* open each RTP stream */
		for(stream_index = 0; stream_index < stream->nb_streams; stream_index++)
		{
			dest_addr.sin_port = htons(stream->multicast_port + 2 * stream_index);
			if (muxSvrRtpNewAvStream(rtp_c, stream_index, &dest_addr, NULL) >= 0)
				continue;

			MUX_SVR_LOG("Could not open output stream '%s/streamid=%d'\n", stream->filename, stream_index);
			exit(1);
		}

		rtp_c->state = HTTPSTATE_SEND_DATA;
	}
}

static void __httpSendTooBusyReply(MUX_SVR *muxSvr, int fd)
{
	char buffer[400];
	int len = snprintf(buffer, sizeof(buffer),
		"HTTP/1.0 503 Server too busy\r\n"
		"Content-type: text/html\r\n"
		"\r\n"
		"<!DOCTYPE html>\n"
		"<html><head><title>Too busy</title></head><body>\r\n"
		"<p>The server is too busy to serve your request at "
		"this time.</p>\r\n"
		"<p>The number of current connections is %u, and this "
		"exceeds the limit of %u.</p>\r\n"
		"</body></html>\r\n",
		muxSvr->nb_connections, muxSvr->config.nb_max_connections);

	av_assert0(len < sizeof(buffer));

	if (send(fd, buffer, len, 0) < len)
		av_log(NULL, AV_LOG_WARNING, "Could not send too-busy reply, send() failed\n");
}


/* start waiting for a new HTTP/RTSP request */
static void __startWaitRequest(MuxContext *c, int is_rtsp)
{
	c->buffer_ptr = c->buffer;
	c->buffer_end = c->buffer + c->buffer_size - 1; /* leave room for '\0' */

	c->state = is_rtsp ? RTSPSTATE_WAIT_REQUEST : HTTPSTATE_WAIT_REQUEST;
	c->timeout = c->muxSvr->currentTime + (is_rtsp ? RTSP_REQUEST_TIMEOUT : HTTP_REQUEST_TIMEOUT);
}


/* create new connection from service HTTP and RTSP */
static void _newConnection(MUX_SVR *muxSvr, int server_fd, int is_rtsp)
{
	struct sockaddr_in from_addr;
	socklen_t len;
	int fd;
	MuxContext *c = NULL;

	len = sizeof(from_addr);
	fd = accept(server_fd, (struct sockaddr *)&from_addr, &len);
	if (fd < 0)
	{
		MUX_SVR_LOG("error during accept %s\n", strerror(errno));
		return;
	}
	
	if (ff_socket_nonblock(fd, 1) < 0)
		av_log(NULL, AV_LOG_WARNING, "ff_socket_nonblock failed\n");

	if (muxSvr->nb_connections >= muxSvr->config.nb_max_connections)
	{
		__httpSendTooBusyReply(muxSvr, fd);
		goto fail;
	}

	/* add a new connection */
	c = av_mallocz(sizeof(MuxContext));
	if (!c)
		goto fail;

	c->fd = fd;
	c->poll_entry = NULL;
	c->from_addr = from_addr;
	c->buffer_size = IOBUFFER_INIT_SIZE;
	c->buffer = av_malloc(c->buffer_size);
	if (!c->buffer)
		goto fail;

VTRACE();

	SVR_ADD_NEW_CONN(muxSvr, c);
	
	__startWaitRequest(c, is_rtsp);

VTRACE();
	return;

fail:
	if (c)
	{
		av_freep(&c->buffer);
		av_free(c);
	}
	closesocket(fd);
}

static void __logConnection(MuxContext *c)
{
	if (c->suppress_log)
		return;

	MUX_SVR_LOG("%s - - [%s] \"%s %s\" %d %"PRId64"\n",
		inet_ntoa(c->from_addr.sin_addr), c->method, c->url,
		c->protocol, (c->http_error ? c->http_error : 200), c->data_count);
}

/* process of every connection */
static int _muxSvrConnectionHandle(MUX_SVR *muxSvr, MuxContext *c)
{
	int len, ret;
	uint8_t *ptr;

	switch(c->state)
	{
		case HTTPSTATE_WAIT_REQUEST:
		case RTSPSTATE_WAIT_REQUEST:

			MUX_SVR_LOG("handle connection in WAIT_REQUEST state\n");
			/* timeout ? */
			if ((c->timeout - muxSvr->currentTime) < 0)
				return S_EXIT_FAILURE;
			if (c->poll_entry->revents & (POLLERR | POLLHUP))
				return S_EXIT_FAILURE;

			/* no need to read if no events */
			if (!(c->poll_entry->revents & POLLIN))
				return S_EXIT_SUCCESS;
			/* read the data */
read_loop:
			if (!(len = recv(c->fd, c->buffer_ptr, 1, 0)))
				return S_EXIT_FAILURE;

			if (len < 0)
			{
				if (ff_neterrno() != AVERROR(EAGAIN) && ff_neterrno() != AVERROR(EINTR))
					return S_EXIT_FAILURE;
				break;
			}
			
			/* search for end of request. */
			c->buffer_ptr += len;
			ptr = c->buffer_ptr;
			if ((ptr >= c->buffer + 2 && !memcmp(ptr-2, "\n\n", 2)) ||
				(ptr >= c->buffer + 4 && !memcmp(ptr-4, "\r\n\r\n", 4)))
			{
				/* request found : parse it and reply */
				if (c->state == HTTPSTATE_WAIT_REQUEST)
				{
					MUX_SVR_LOG("parse request for new HTTP connection\n");
					ret = muxSvrHttpParseRequest(muxSvr, c);
				}
				else
				{
					MUX_SVR_LOG("parse request for new RTSP connection\n");
					ret = muxSvrRtspParseRequest(muxSvr,c);
				}

				if (ret < 0)
					return S_EXIT_FAILURE;
			}
			else if (ptr >= c->buffer_end)
			{/* request too long: cannot do anything */
				return S_EXIT_FAILURE;
			}
			else
				goto read_loop;

			break;

		case HTTPSTATE_SEND_HEADER:
			if (c->poll_entry->revents & (POLLERR | POLLHUP))
				return S_EXIT_FAILURE;

			/* no need to write if no events */
			if (!(c->poll_entry->revents & POLLOUT))
				return 0;
			
			len = send(c->fd, c->buffer_ptr, c->buffer_end - c->buffer_ptr, 0);
			if (len < 0)
			{
				if (ff_neterrno() != AVERROR(EAGAIN) && ff_neterrno() != AVERROR(EINTR))
				{
					goto close_connection;
				}
				break;
			}

			c->buffer_ptr += len;
			if (c->stream)
				c->stream->bytes_served += len;

			c->data_count += len;
			if (c->buffer_ptr >= c->buffer_end)
			{
				av_freep(&c->pb_buffer);
				/* if error, exit */
				if (c->http_error)
					return S_EXIT_FAILURE;
				
				/* all the buffer was sent : synchronize to the incoming stream */
				c->state = HTTPSTATE_SEND_DATA_HEADER;
				c->buffer_ptr = c->buffer_end = c->buffer;
			}
			break;

		case HTTPSTATE_SEND_DATA:
		case HTTPSTATE_SEND_DATA_HEADER:
		case HTTPSTATE_SEND_DATA_TRAILER:
			/* for packetized output, we consider we can always write (the
			* input streams set the speed). It may be better to verify
			* that we do not rely too much on the kernel queues */
			if (!c->is_packetized)
			{
				if (c->poll_entry->revents & (POLLERR | POLLHUP))
					return S_EXIT_FAILURE;

				/* no need to read if no events */
				if (!(c->poll_entry->revents & POLLOUT))
					return 0;
			}
			
			if (muxSvrSendData(c) < 0)
				return S_EXIT_FAILURE;
			
			/* close connection if trailer sent */
			if (c->state == HTTPSTATE_SEND_DATA_TRAILER)
				return S_EXIT_FAILURE;
			
			/* Check if it is a single jpeg frame 123 */
			if (c->stream->single_frame && c->data_count > c->cur_frame_bytes && c->cur_frame_bytes > 0)
			{
				muxSvrConnClose(c);
			}
			break;
			
		case HTTPSTATE_RECEIVE_DATA:
			/* no need to read if no events */
			if (c->poll_entry->revents & (POLLERR | POLLHUP))
				return S_EXIT_FAILURE;
			if (!(c->poll_entry->revents & POLLIN))
				return 0;
			if (muxSvrReceiveData(c) < 0)
				return S_EXIT_FAILURE;
			break;
		
		case HTTPSTATE_WAIT_FEED:
			/* no need to read if no events */
			if (c->poll_entry->revents & (POLLIN | POLLERR | POLLHUP))
				return S_EXIT_FAILURE;

			/* nothing to do, we'll be waken up by incoming feed packets */
			break;

		case RTSPSTATE_SEND_REPLY:
			if (c->poll_entry->revents & (POLLERR | POLLHUP))
				goto close_connection;
			/* no need to write if no events */
			if (!(c->poll_entry->revents & POLLOUT))
				return 0;
			
			len = send(c->fd, c->buffer_ptr, c->buffer_end - c->buffer_ptr, 0);
			if (len < 0)
			{
				if (ff_neterrno() != AVERROR(EAGAIN) && ff_neterrno() != AVERROR(EINTR))
				{
					goto close_connection;
				}
				break;
			}

			c->buffer_ptr += len;
			c->data_count += len;
			if (c->buffer_ptr >= c->buffer_end)
			{
				/* all the buffer was sent : wait for a new request */
				av_freep(&c->pb_buffer);
				__startWaitRequest(c, 1);
			}
			break;
			
		case RTSPSTATE_SEND_PACKET:
			if (c->poll_entry->revents & (POLLERR | POLLHUP))
			{
				av_freep(&c->packet_buffer);
				return S_EXIT_FAILURE;
			}
			
			/* no need to write if no events */
			if (!(c->poll_entry->revents & POLLOUT))
				return 0;
			
			len = send(c->fd, c->packet_buffer_ptr, c->packet_buffer_end - c->packet_buffer_ptr, 0);
			if (len < 0)
			{
				if (ff_neterrno() != AVERROR(EAGAIN) && ff_neterrno() != AVERROR(EINTR))
				{/* error : close connection */
					av_freep(&c->packet_buffer);
					return S_EXIT_FAILURE;
				}
				break;
			}
			
			c->packet_buffer_ptr += len;
			if (c->packet_buffer_ptr >= c->packet_buffer_end)
			{
				/* all the buffer was sent : wait for a new request */
				av_freep(&c->packet_buffer);
				c->state = RTSPSTATE_WAIT_REQUEST;
			}
			break;
		
		case HTTPSTATE_READY:
			/* nothing to do */
			break;
		
		default:
			return S_EXIT_FAILURE;
	}
	return 0;

close_connection:
	av_freep(&c->pb_buffer);
	return -1;
}

/* main loop of the server */
int muxSvrServer(MUX_SVR *muxSvr)
{
	int server_fd = 0, rtsp_server_fd = 0;
	int ret, delay;
	struct pollfd *poll_table, *poll_entry;
	MuxContext *c, *c_next;

	poll_table = av_mallocz_array(muxSvr->config.nb_max_http_connections + 2, sizeof(*poll_table));
	if(!poll_table)
	{
		MUX_SVR_LOG("Impossible to allocate a poll table handling %d connections.\n", muxSvr->config.nb_max_http_connections);
		return S_EXIT_FAILURE;
	}

	if (muxSvr->config.http_addr.sin_port)
	{
		server_fd = _openServicePort(&muxSvr->config.http_addr);
		if (server_fd < 0)
			goto quit;
	}

	if (muxSvr->config.rtsp_addr.sin_port)
	{
		rtsp_server_fd = _openServicePort(&muxSvr->config.rtsp_addr);
		if (rtsp_server_fd < 0)
		{
			closesocket(server_fd);
			goto quit;
		}
	}

	if (!rtsp_server_fd && !server_fd)
	{
		MUX_SVR_LOG("HTTP and RTSP disabled.\n");
		goto quit;
	}

	MUX_SVR_LOG("muxServer started.\n");

//	_startChildren(muxSvr->config.first_feed);

	_startMulticast(muxSvr);

	for(;;)
	{
		poll_entry = poll_table;
		if (server_fd)
		{
			poll_entry->fd = server_fd;
			poll_entry->events = POLLIN;
			poll_entry++;
		}
		
		if (rtsp_server_fd)
		{
			poll_entry->fd = rtsp_server_fd;
			poll_entry->events = POLLIN;
			poll_entry++;
		}

		/* wait for events on each HTTP handle */
		c = muxSvr->firstConnectCtx;
		delay = 1000;
		while (c)
		{
			int fd;
			fd = c->fd;
			
			switch(c->state)
			{
				case HTTPSTATE_SEND_HEADER:
				case RTSPSTATE_SEND_REPLY:
				case RTSPSTATE_SEND_PACKET:
					c->poll_entry = poll_entry;
					poll_entry->fd = fd;
					poll_entry->events = POLLOUT;
					poll_entry++;
					break;
				
				case HTTPSTATE_SEND_DATA_HEADER:
				case HTTPSTATE_SEND_DATA:
				case HTTPSTATE_SEND_DATA_TRAILER:
					if (!c->is_packetized)
					{
						/* for TCP, we output as much as we can
						* (may need to put a limit) */
						c->poll_entry = poll_entry;
						poll_entry->fd = fd;
						poll_entry->events = POLLOUT;
						poll_entry++;
					}
					else
					{
						/* when ffserver is doing the timing, we work by
						* looking at which packet needs to be sent every
						* 10 ms (one tick wait XXX: 10 ms assumed) */
						if (delay > 10)
							delay = 10;
					}
					break;

				case HTTPSTATE_WAIT_REQUEST:
				case HTTPSTATE_RECEIVE_DATA:
				case HTTPSTATE_WAIT_FEED:
				case RTSPSTATE_WAIT_REQUEST:
					/* need to catch errors */
					c->poll_entry = poll_entry;
					poll_entry->fd = fd;
					poll_entry->events = POLLIN;/* Maybe this will work */
					poll_entry++;
					break;

				default:
					c->poll_entry = NULL;
					break;
			}
			c = c->next;
		}

		/* wait for an event on one connection. We poll at least every second to handle timeouts */
		do
		{
			ret = poll(poll_table, poll_entry - poll_table, delay);
			if (ret < 0 && ff_neterrno() != AVERROR(EAGAIN) && ff_neterrno() != AVERROR(EINTR))
			{
				goto quit;
			}
		} while (ret < 0);

		muxSvr->currentTime = av_gettime() / 1000;

		if (muxSvr->need_to_start_children)
		{
			muxSvr->need_to_start_children = 0;
			_startChildren(muxSvr->config.first_feed);
		}

		/* now handle the events */
		for(c = muxSvr->firstConnectCtx; c; c = c_next)
		{
			c_next = c->next;
			if (_muxSvrConnectionHandle(muxSvr, c) < 0)
			{
				__logConnection(c);
				/* close and free the connection */
				muxSvrConnClose(c);
			}
		}

		poll_entry = poll_table;
		if (server_fd)
		{/* new HTTP connection request ? */
			if (poll_entry->revents & POLLIN)
				_newConnection(muxSvr, server_fd, 0);
			poll_entry++;
		}

		if (rtsp_server_fd)
		{/* new RTSP connection request ? */
			if (poll_entry->revents & POLLIN)
				_newConnection(muxSvr, rtsp_server_fd, 1);
		}
	}

quit:
	av_free(poll_table);
	return S_EXIT_FAILURE;
}

