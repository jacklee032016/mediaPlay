

static void http_send_too_busy_reply(int fd)
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
		nb_connections, config.nb_max_connections);

	av_assert0(len < sizeof(buffer));
	
	if (send(fd, buffer, len, 0) < len)
		av_log(NULL, AV_LOG_WARNING, "Could not send too-busy reply, send() failed\n");
}


static void new_connection(int server_fd, int is_rtsp)
{
	struct sockaddr_in from_addr;
	socklen_t len;
	int fd;
	MuxContext *c = NULL;

	len = sizeof(from_addr);
	fd = accept(server_fd, (struct sockaddr *)&from_addr, &len);
	if (fd < 0)
	{
		http_log("error during accept %s\n", strerror(errno));
		return;
	}
	
	if (ff_socket_nonblock(fd, 1) < 0)
		av_log(NULL, AV_LOG_WARNING, "ff_socket_nonblock failed\n");

	if (nb_connections >= config.nb_max_connections)
	{
		http_send_too_busy_reply(fd);
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

	c->next = first_http_ctx;
	first_http_ctx = c;
	nb_connections++;

	start_wait_request(c, is_rtsp);

	return;

fail:
	if (c)
	{
		av_freep(&c->buffer);
		av_free(c);
	}
	closesocket(fd);
}

static void close_connection(MuxContext *c)
{
	MuxContext **cp, *c1;
	int i, nb_streams;
	AVFormatContext *ctx;
	AVStream *st;

	/* remove connection from list */
	cp = &first_http_ctx;
	while (*cp)
	{
		c1 = *cp;
		if (c1 == c)
			*cp = c->next;
		else
			cp = &c1->next;
	}

	/* remove references, if any (XXX: do it faster) */
	for(c1 = first_http_ctx; c1; c1 = c1->next)
	{
		if (c1->rtsp_c == c)
			c1->rtsp_c = NULL;
	}

	/* remove connection associated resources */
	if (c->fd >= 0)
		closesocket(c->fd);

	if (c->fmt_in)
	{
		/* close each frame parser */
		for(i=0;i<c->fmt_in->nb_streams;i++)
		{
			st = c->fmt_in->streams[i];
			if (st->codec->codec)
				avcodec_close(st->codec);
		}
		avformat_close_input(&c->fmt_in);
	}

	/* free RTP output streams if any */
	nb_streams = 0;
	if (c->stream)
		nb_streams = c->stream->nb_streams;

	for(i=0;i<nb_streams;i++)
	{
		ctx = c->rtp_ctx[i];
		if (ctx)
		{
			av_write_trailer(ctx);
			av_dict_free(&ctx->metadata);
			av_freep(&ctx->streams[0]);
			av_freep(&ctx);
		}
		ffurl_close(c->rtp_handles[i]);
	}

	ctx = c->pfmt_ctx;

	if (ctx)
	{
		if (!c->last_packet_sent && c->state == HTTPSTATE_SEND_DATA_TRAILER)
		{
			/* prepare header */
			if (ctx->oformat && avio_open_dyn_buf(&ctx->pb) >= 0)
			{
				av_write_trailer(ctx);
				av_freep(&c->pb_buffer);
				avio_close_dyn_buf(ctx->pb, &c->pb_buffer);
			}
		}
		for(i=0; i<ctx->nb_streams; i++)
			av_freep(&ctx->streams[i]);
		
		av_freep(&ctx->streams);
		av_freep(&ctx->priv_data);
	}

	if (c->stream && !c->post && c->stream->stream_type == STREAM_TYPE_LIVE)
		current_bandwidth -= c->stream->bandwidth;

	/* signal that there is no feed if we are the feeder socket */
	if (c->state == HTTPSTATE_RECEIVE_DATA && c->stream)
	{
		c->stream->feed_opened = 0;
		close(c->feed_fd);
	}

	av_freep(&c->pb_buffer);
	av_freep(&c->packet_buffer);
	av_freep(&c->buffer);
	av_free(c);
	nb_connections--;
}

static int handle_connection(MuxContext *c)
{
	int len, ret;
	uint8_t *ptr;

	switch(c->state)
	{
		case HTTPSTATE_WAIT_REQUEST:
		case RTSPSTATE_WAIT_REQUEST:
			/* timeout ? */
			if ((c->timeout - cur_time) < 0)
				return -1;
			if (c->poll_entry->revents & (POLLERR | POLLHUP))
				return -1;

			/* no need to read if no events */
			if (!(c->poll_entry->revents & POLLIN))
				return 0;
			/* read the data */
read_loop:
			if (!(len = recv(c->fd, c->buffer_ptr, 1, 0)))
				return -1;

			if (len < 0)
			{
				if (ff_neterrno() != AVERROR(EAGAIN) && ff_neterrno() != AVERROR(EINTR))
					return -1;
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
					ret = http_parse_request(c);
				else
					ret = rtsp_parse_request(c);

				if (ret < 0)
				return -1;
			}
			else if (ptr >= c->buffer_end)
			{/* request too long: cannot do anything */
				return -1;
			}
			else
				goto read_loop;

			break;

		case HTTPSTATE_SEND_HEADER:
			if (c->poll_entry->revents & (POLLERR | POLLHUP))
				return -1;

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
					return -1;
				
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
					return -1;

				/* no need to read if no events */
				if (!(c->poll_entry->revents & POLLOUT))
					return 0;
			}
			
			if (http_send_data(c) < 0)
				return -1;
			
			/* close connection if trailer sent */
			if (c->state == HTTPSTATE_SEND_DATA_TRAILER)
				return -1;
			
			/* Check if it is a single jpeg frame 123 */
			if (c->stream->single_frame && c->data_count > c->cur_frame_bytes && c->cur_frame_bytes > 0)
			{
				close_connection(c);
			}
			break;
			
		case HTTPSTATE_RECEIVE_DATA:
			/* no need to read if no events */
			if (c->poll_entry->revents & (POLLERR | POLLHUP))
				return -1;
			if (!(c->poll_entry->revents & POLLIN))
				return 0;
			if (http_receive_data(c) < 0)
				return -1;
			break;
		
		case HTTPSTATE_WAIT_FEED:
			/* no need to read if no events */
			if (c->poll_entry->revents & (POLLIN | POLLERR | POLLHUP))
				return -1;

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
				start_wait_request(c, 1);
			}
			break;
			
		case RTSPSTATE_SEND_PACKET:
			if (c->poll_entry->revents & (POLLERR | POLLHUP))
			{
				av_freep(&c->packet_buffer);
				return -1;
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
					return -1;
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
			return -1;
	}
	return 0;

close_connection:
	av_freep(&c->pb_buffer);
	return -1;
}



