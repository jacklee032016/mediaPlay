
#include "muxSvr.h"


void muxSvrConnClose(MuxContext *c)
{
	MuxContext **cp, *c1;
	int i, nb_streams;
	AVFormatContext *ctx;
	AVStream *st;

	/* remove connection from list */
	cp = &c->muxSvr->firstConnectCtx;
	while (*cp)
	{
		c1 = *cp;
		if (c1 == c)
			*cp = c->next;
		else
			cp = &c1->next;
	}

	/* remove references, if any (XXX: do it faster) */
	for(c1 = c->muxSvr->firstConnectCtx; c1; c1 = c1->next)
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
		c->muxSvr->current_bandwidth -= c->stream->bandwidth;

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
	c->muxSvr->nb_connections--;
}


MuxContext *muxSvrConnNewRTP(MUX_SVR *muxSvr, struct sockaddr_in *from_addr, MuxStream *stream, const char *session_id, enum RTSPLowerTransport rtp_protocol)
{
	MuxContext *c = NULL;
	const char *proto_str;

	/* XXX: should output a warning page when coming
	* close to the connection limit */
	if (muxSvr->nb_connections >= muxSvr->config.nb_max_connections)
		goto fail;

	/* add a new connection */
	c = av_mallocz(sizeof(MuxContext));
	if (!c)
		goto fail;

	c->fd = -1;
	c->poll_entry = NULL;
	c->from_addr = *from_addr;
	c->buffer_size = IOBUFFER_INIT_SIZE;
	c->buffer = av_malloc(c->buffer_size);
	if (!c->buffer)
		goto fail;

	c->stream = stream;
	av_strlcpy(c->session_id, session_id, sizeof(c->session_id));
	c->state = HTTPSTATE_READY;
	c->is_packetized = 1;
	c->rtp_protocol = rtp_protocol;

	/* protocol is shown in statistics */
	switch(c->rtp_protocol)
	{
		case RTSP_LOWER_TRANSPORT_UDP_MULTICAST:
			proto_str = "MCAST";
			break;
		
		case RTSP_LOWER_TRANSPORT_UDP:
			proto_str = "UDP";
			break;
		
		case RTSP_LOWER_TRANSPORT_TCP:
			proto_str = "TCP";
			break;
		
		default:
			proto_str = "???";
			break;
	}
	
	av_strlcpy(c->protocol, "RTP/", sizeof(c->protocol));
	av_strlcat(c->protocol, proto_str, sizeof(c->protocol));

	muxSvr->current_bandwidth += stream->bandwidth;

	SVR_ADD_NEW_CONN(muxSvr, c);

	return c;

fail:
	if (c)
	{
		av_freep(&c->buffer);
		av_free(c);
	}
	
	return NULL;
}

int muxSvrConnOpenInputStream(MuxContext *c, const char *info)
{
	char buf[128];
	char input_filename[1024];
	AVFormatContext *s = NULL;
	int buf_size, i, ret;
	int64_t stream_pos;

	/* find file name */
	if (c->stream->feed)
	{
		strcpy(input_filename, c->stream->feed->feed_filename);
		buf_size = MUX_PACKET_SIZE;
		/* compute position (absolute time) */
		if (av_find_info_tag(buf, sizeof(buf), "date", info))
		{
			if ((ret = av_parse_time(&stream_pos, buf, 0)) < 0)
			{
				MUX_SVR_LOG("Invalid date specification '%s' for stream\n", buf);
				return ret;
			}
		}
		else if (av_find_info_tag(buf, sizeof(buf), "buffer", info))
		{
			int prebuffer = strtol(buf, 0, 10);
			stream_pos = av_gettime() - prebuffer * (int64_t)1000000;
		}
		else
			stream_pos = av_gettime() - c->stream->prebuffer * (int64_t)1000;
	}
	else
	{
		strcpy(input_filename, c->stream->feed_filename);
		buf_size = 0;
		/* compute position (relative time) */
		if (av_find_info_tag(buf, sizeof(buf), "date", info))
		{
			if ((ret = av_parse_time(&stream_pos, buf, 1)) < 0)
			{
				MUX_SVR_LOG("Invalid date specification '%s' for stream\n", buf);
				return ret;
			}
		}
		else
			stream_pos = 0;
	}
	
	if (!input_filename[0])
	{
		MUX_SVR_LOG("No filename was specified for stream\n");
		return AVERROR(EINVAL);
	}

	/* open stream */
	ret = avformat_open_input(&s, input_filename, c->stream->ifmt, &c->stream->in_opts);
	if (ret < 0)
	{
		MUX_SVR_LOG("Could not open input '%s': %s\n", input_filename, av_err2str(ret));
		return ret;
	}

	/* set buffer size */
	if (buf_size > 0)
	{
		ret = ffio_set_buf_size(s->pb, buf_size);
		if (ret < 0)
		{
			MUX_SVR_LOG("Failed to set buffer size\n");
			return ret;
		}
	}

	s->flags |= AVFMT_FLAG_GENPTS;
	c->fmt_in = s;
	if (strcmp(s->iformat->name, "ffm") && (ret = avformat_find_stream_info(c->fmt_in, NULL)) < 0)
	{
		MUX_SVR_LOG("Could not find stream info for input '%s'\n", input_filename);
		avformat_close_input(&s);
		return ret;
	}

	/* choose stream as clock source (we favor the video stream if
	* present) for packet sending */
	c->pts_stream_index = 0;
	for(i=0;i<c->stream->nb_streams;i++)
	{
		if (c->pts_stream_index == 0 && c->stream->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			c->pts_stream_index = i;
		}
	}

	if (c->fmt_in->iformat->read_seek)
		av_seek_frame(c->fmt_in, -1, stream_pos, 0);
	
	/* set the start time (needed for maxtime and RTP packet timing) */
	c->start_time = c->muxSvr->currentTime;
	c->first_pts = AV_NOPTS_VALUE;
	return 0;
}


