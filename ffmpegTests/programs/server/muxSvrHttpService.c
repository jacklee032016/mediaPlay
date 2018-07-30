
static void _start_children(FFServerStream *feed)
{
	char *pathname;
	char *slash;
	int i;
	size_t cmd_length;

	if (no_launch)
		return;

	cmd_length = strlen(my_program_name);

	/**
	* FIXME: WIP Safeguard. Remove after clearing all harcoded
	* '1024' path lengths
	*/
	if (cmd_length > PATH_LENGTH - 1)
	{
		http_log("Could not start children. Command line: '%s' exceeds path length limit (%d)\n", my_program_name, PATH_LENGTH);
		return;
	}

	slash = strrchr(my_program_name, '/');
	if (!slash)
	{
		pathname = av_mallocz(sizeof("ffmpeg"));
	}
	else
	{
		pathname = av_mallocz(slash - my_program_name + sizeof("ffmpeg"));
		if (pathname != NULL)
		{
			memcpy(pathname, my_program_name, slash - my_program_name);
		}
	}
	
	if (!pathname)
	{
		http_log("Could not allocate memory for children cmd line\n");
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
			http_log("Unable to create children: %s\n", strerror(errno));
			av_free (pathname);
			exit(EXIT_FAILURE);
		}

		if (feed->pid)
			continue;

		/* In child */

		http_log("Launch command line: ");
		http_log("%s ", pathname);

		for (i = 1; feed->child_argv[i] && feed->child_argv[i][0]; i++)
			http_log("%s ", feed->child_argv[i]);
		http_log("\n");

		for (i = 3; i < 256; i++)
			close(i);

		if (!config.debug)
		{
			if (!freopen("/dev/null", "r", stdin))
				http_log("failed to redirect STDIN to /dev/null\n;");
			if (!freopen("/dev/null", "w", stdout))
				http_log("failed to redirect STDOUT to /dev/null\n;");
			if (!freopen("/dev/null", "w", stderr))
				http_log("failed to redirect STDERR to /dev/null\n;");
		}

		signal(SIGPIPE, SIG_DFL);
		execvp(pathname, feed->child_argv);
		av_free (pathname);
		_exit(1);
	}

	av_free (pathname);
}

/* open a listening socket */
static int _socket_open_listen(struct sockaddr_in *my_addr)
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
static void _start_multicast(void)
{
	FFServerStream *stream;
	char session_id[32];
	MuxContext *rtp_c;
	struct sockaddr_in dest_addr = {0};
	int default_port, stream_index;
	unsigned int random0, random1;

	default_port = 6000;
	for(stream = config.first_stream; stream; stream = stream->next)
	{

		if (!stream->is_multicast)
			continue;

		random0 = av_lfg_get(&random_state);
		random1 = av_lfg_get(&random_state);

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

		rtp_c = rtp_new_connection(&dest_addr, stream, session_id, RTSP_LOWER_TRANSPORT_UDP_MULTICAST);
		if (!rtp_c)
			continue;

		if (open_input_stream(rtp_c, "") < 0)
		{
			http_log("Could not open input stream for stream '%s'\n",
			stream->filename);
			continue;
		}

		/* open each RTP stream */
		for(stream_index = 0; stream_index < stream->nb_streams; stream_index++)
		{
			dest_addr.sin_port = htons(stream->multicast_port + 2 * stream_index);
			if (rtp_new_av_stream(rtp_c, stream_index, &dest_addr, NULL) >= 0)
				continue;

			http_log("Could not open output stream '%s/streamid=%d'\n", stream->filename, stream_index);
			exit(1);
		}

		rtp_c->state = HTTPSTATE_SEND_DATA;
	}
}


/* main loop of the HTTP server */
int http_server(void)
{
	int server_fd = 0, rtsp_server_fd = 0;
	int ret, delay;
	struct pollfd *poll_table, *poll_entry;
	MuxContext *c, *c_next;

	poll_table = av_mallocz_array(config.nb_max_http_connections + 2, sizeof(*poll_table));
	if(!poll_table)
	{
		http_log("Impossible to allocate a poll table handling %d connections.\n", config.nb_max_http_connections);
		return -1;
	}

	if (config.http_addr.sin_port)
	{
		server_fd = _socket_open_listen(&config.http_addr);
		if (server_fd < 0)
			goto quit;
	}

	if (config.rtsp_addr.sin_port)
	{
		rtsp_server_fd = _socket_open_listen(&config.rtsp_addr);
		if (rtsp_server_fd < 0)
		{
			closesocket(server_fd);
			goto quit;
		}
	}

	if (!rtsp_server_fd && !server_fd)
	{
		http_log("HTTP and RTSP disabled.\n");
		goto quit;
	}

	http_log("FFserver started.\n");

	_start_children(config.first_feed);

	_start_multicast();

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
		c = first_http_ctx;
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

		cur_time = av_gettime() / 1000;

		if (need_to_start_children)
		{
			need_to_start_children = 0;
			_start_children(config.first_feed);
		}

		/* now handle the events */
		for(c = first_http_ctx; c; c = c_next)
		{
			c_next = c->next;
			if (handle_connection(c) < 0)
			{
				log_connection(c);
				/* close and free the connection */
				close_connection(c);
			}
		}

		poll_entry = poll_table;
		if (server_fd)
		{
			/* new HTTP connection request ? */
			if (poll_entry->revents & POLLIN)
				new_connection(server_fd, 0);
			poll_entry++;
		}

		if (rtsp_server_fd)
		{
			/* new RTSP connection request ? */
			if (poll_entry->revents & POLLIN)
			new_connection(rtsp_server_fd, 1);
		}
	}

quit:
	av_free(poll_table);
	return -1;
}


