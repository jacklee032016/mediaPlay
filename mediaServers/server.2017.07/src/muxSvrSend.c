
#include "muxSvr.h"

static int _httpPrepareData(MuxContext *c)
{
	int i, len, ret;
	AVFormatContext *ctx;

	av_freep(&c->pb_buffer);
	switch(c->state)
	{
		case HTTPSTATE_SEND_DATA_HEADER:
#if  1
			avformat_alloc_output_context2(&ctx, NULL, NULL, c->stream->filename);//"Example_Out.m3u8");
			//ofmt = ofmt_ctx->oformat;
#else
			ctx = avformat_alloc_context();
#endif
			if (!ctx)
				return AVERROR(ENOMEM);
			c->pfmt_ctx = ctx;
			av_dict_copy(&(c->pfmt_ctx->metadata), c->stream->metadata, 0);

			for(i=0;i<c->stream->nb_streams;i++)
			{
				LayeredAVStream *src;
				AVStream *st = avformat_new_stream(c->pfmt_ctx, NULL);
				if (!st)
					return AVERROR(ENOMEM);

				/* if file or feed, then just take streams from MuxStream
				* struct */
				if (!c->stream->feed ||c->stream->feed == c->stream)
					src = c->stream->streams[i];
				else
					src = c->stream->feed->streams[c->stream->feed_streams[i]];

				unlayer_stream(c->pfmt_ctx->streams[i], src); //TODO we no longer copy st->internal, does this matter?
				av_assert0(!c->pfmt_ctx->streams[i]->priv_data);

				if (src->codec->flags & AV_CODEC_FLAG_BITEXACT)
					c->pfmt_ctx->flags |= AVFMT_FLAG_BITEXACT;
			}
			/* set output format parameters */
//			c->pfmt_ctx->oformat = c->stream->fmt;
			av_assert0(c->pfmt_ctx->nb_streams == c->stream->nb_streams);

			AVDictionary	*headerOptions = NULL;
			
			if (!strcmp(c->stream->fmt->name, "hls"))
			{
			VTRACE();
#if 1	
				av_dict_set_int(&headerOptions, "hls_time", 10, 0);
				av_dict_set_int(&headerOptions, "hls_list_size", 6, 0);
				av_dict_set(&headerOptions, "hls_flags", "delete_segments", 0);
				av_dict_set(&headerOptions, "hls_segment_filename", "file%03d.ts", 0);
//				av_dict_set(&headerOptions, "hls_flags", "single_file", 0);//AV_OPT_SEARCH_CHILDREN);
			VTRACE();
#else
 				av_opt_set_int(c->pfmt_ctx->priv_data, "hls_time", 10, AV_OPT_SEARCH_CHILDREN);
				av_opt_set_int(c->pfmt_ctx->priv_data, "hls_list_size", 6, AV_OPT_SEARCH_CHILDREN);
#if 1
		av_opt_set(c->pfmt_ctx->priv_data, "hls_flags", "delete_segments", AV_OPT_SEARCH_CHILDREN);
//av_opt_set(c->pfmt_ctx->priv_data, "hls_segment_filename", "segment_%Y%m%d%H%M%S_%%04d_%%08s_%%013t.ts", AV_OPT_SEARCH_CHILDREN);
		av_opt_set(c->pfmt_ctx->priv_data, "hls_segment_filename", "file%03d.ts", AV_OPT_SEARCH_CHILDREN);
#else
		av_opt_set(c->pfmt_ctx->priv_data, "hls_flags", "single_file", 0);//AV_OPT_SEARCH_CHILDREN);
#endif
#endif
 			VTRACE();
			}

			c->got_key_frame = 0;

			/* prepare header and save header data in a stream */
			if (avio_open_dyn_buf(&c->pfmt_ctx->pb) < 0)
			{
				/* XXX: potential leak */
				return -1;
			}
			c->pfmt_ctx->pb->seekable = 0;

			/*
			* HACK to avoid MPEG-PS muxer to spit many underflow errors
			* Default value from FFmpeg
			* Try to set it using configuration option
			*/
			c->pfmt_ctx->max_delay = (int)(0.7*AV_TIME_BASE);

//			if ((ret = avformat_write_header(c->pfmt_ctx, NULL)) < 0)
			if ((ret = avformat_write_header(c->pfmt_ctx, &headerOptions)) < 0)
			{
				MUX_SVR_LOG("Error writing output header for stream '%s': %s\n", c->stream->filename, av_err2str(ret));
				return ret;
			}
			av_dict_free(&c->pfmt_ctx->metadata);

			len = avio_close_dyn_buf(c->pfmt_ctx->pb, &c->pb_buffer);
			c->buffer_ptr = c->pb_buffer;
			c->buffer_end = c->pb_buffer + len;

			c->state = HTTPSTATE_SEND_DATA;
			c->last_packet_sent = 0;
			break;

		
		case HTTPSTATE_SEND_DATA:
		/* find a new packet */
			/* read a packet from the input stream */
			if (c->stream->feed)
				ffm_set_write_index(c->fmt_in,  c->stream->feed->feed_write_index, c->stream->feed->feed_size);

			if (c->stream->max_time && c->stream->max_time + c->start_time - c->muxSvr->currentTime < 0)
				/* We have timed out */
				c->state = HTTPSTATE_SEND_DATA_TRAILER;
			else
			{
				AVPacket pkt;
redo:
				ret = av_read_frame(c->fmt_in, &pkt);
				if (ret < 0)
				{
					if (c->stream->feed)
					{
						/* if coming from feed, it means we reached the end of the
						* ffm file, so must wait for more data */
						c->state = HTTPSTATE_WAIT_FEED;
						return 1; /* state changed */
					}
					
					if (ret == AVERROR(EAGAIN))
					{/* input not ready, come back later */
						return 0;
					}
					
					if (c->stream->loop)
					{
						avformat_close_input(&c->fmt_in);
						if( ( muxSvrConnOpenInputStream(c, "") < 0))
							goto no_loop;
						goto redo;
					}
					else
					{
no_loop:
						/* must send trailer now because EOF or error */
						c->state = HTTPSTATE_SEND_DATA_TRAILER;
					}
				} 
				else
				{
					int source_index = pkt.stream_index;
					/* update first pts if needed */
					if (c->first_pts == AV_NOPTS_VALUE && pkt.dts != AV_NOPTS_VALUE)
					{
						c->first_pts = av_rescale_q(pkt.dts, c->fmt_in->streams[pkt.stream_index]->time_base, AV_TIME_BASE_Q);
						c->start_time = c->muxSvr->currentTime;
					}
					
					/* send it to the appropriate stream */
					if (c->stream->feed)
					{
						/* if coming from a feed, select the right stream */
						if (c->switch_pending)
						{
							c->switch_pending = 0;
							for(i=0;i<c->stream->nb_streams;i++)
							{
								if (c->switch_feed_streams[i] == pkt.stream_index)
									if (pkt.flags & AV_PKT_FLAG_KEY)
										c->switch_feed_streams[i] = -1;
								if (c->switch_feed_streams[i] >= 0)
									c->switch_pending = 1;
							}
						}
						
						for(i=0;i<c->stream->nb_streams;i++)
						{
							if (c->stream->feed_streams[i] == pkt.stream_index)
							{
								AVStream *st = c->fmt_in->streams[source_index];
								pkt.stream_index = i;
								if (pkt.flags & AV_PKT_FLAG_KEY && (st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO ||	c->stream->nb_streams == 1))
									c->got_key_frame = 1;
								if (!c->stream->send_on_key || c->got_key_frame)
									goto send_it;
							}
						}
					}
					else
					{
						AVStream *ist, *ost;
send_it:
						ist = c->fmt_in->streams[source_index];
						/* specific handling for RTP: we use several
						* output streams (one for each RTP connection).
						* XXX: need more abstract handling */
						if (c->is_packetized)
						{
							/* compute send time and duration */
							if (pkt.dts != AV_NOPTS_VALUE)
							{
								c->cur_pts = av_rescale_q(pkt.dts, ist->time_base, AV_TIME_BASE_Q);
								c->cur_pts -= c->first_pts;
							}
							
							c->cur_frame_duration = av_rescale_q(pkt.duration, ist->time_base, AV_TIME_BASE_Q);
							/* find RTP context */
							c->packet_stream_index = pkt.stream_index;
							ctx = c->rtp_ctx[c->packet_stream_index];
							if(!ctx)
							{
								av_packet_unref(&pkt);
								break;
							}
							/* only one stream per RTP connection */
							pkt.stream_index = 0;
						}
						else
						{
							ctx = c->pfmt_ctx;
							/* Fudge here */
						}

						if (c->is_packetized)
						{
							int max_packet_size;
							if (c->rtp_protocol == RTSP_LOWER_TRANSPORT_TCP)
								max_packet_size = RTSP_TCP_MAX_PACKET_SIZE;
							else
								max_packet_size = c->rtp_handles[c->packet_stream_index]->max_packet_size;

							ret = ffio_open_dyn_packet_buf(&ctx->pb, max_packet_size);
						}
						else
							ret = avio_open_dyn_buf(&ctx->pb);

						if (ret < 0)
						{/* XXX: potential leak */
							return -1;
						}
						ost = ctx->streams[pkt.stream_index];

						ctx->pb->seekable = 0;
						if (pkt.dts != AV_NOPTS_VALUE)
						{
							pkt.dts = av_rescale_q(pkt.dts, ist->time_base, ost->time_base);
//							MUX_SVR_LOG("rescale dts : %ld\n", pkt.dts);
						}
						else
						{
//							MUX_SVR_LOG("dts is AV_NOPTS_VALUE\n" );
						}
						
						if (pkt.pts != AV_NOPTS_VALUE)
						{
							pkt.pts = av_rescale_q(pkt.pts, ist->time_base, ost->time_base);
//							MUX_SVR_LOG("rescale pts : %ld\n", pkt.pts);
						}
						else
						{
//							MUX_SVR_LOG("pts is AV_NOPTS_VALUE\n");
						}


						pkt.duration = av_rescale_q(pkt.duration, ist->time_base, ost->time_base);
//VTRACE();
						if ((ret = av_write_frame(ctx, &pkt)) < 0)
						{
							MUX_SVR_LOG("Error writing frame to output for stream '%s': %s\n", c->stream->filename, av_err2str(ret));
							c->state = HTTPSTATE_SEND_DATA_TRAILER;
						}

						av_freep(&c->pb_buffer);
						len = avio_close_dyn_buf(ctx->pb, &c->pb_buffer);
						ctx->pb = NULL;
						c->cur_frame_bytes = len;
						c->buffer_ptr = c->pb_buffer;
						c->buffer_end = c->pb_buffer + len;

						/* len returned by HLS muxer is always 0, so HLS muxer always output all media as quick as program runs. Jack Lee */
						MUX_SVR_LOG("buffer length is %d after av_write_frame(): pkt->size:%d\n", len, pkt.size);
						
						if (len == 0)
						{
							av_packet_unref(&pkt);
							goto redo;
						}
					}
					av_packet_unref(&pkt);
				}
			}
		break;

		
		default:
		case HTTPSTATE_SEND_DATA_TRAILER:
			/* last packet test ? */
			if (c->last_packet_sent || c->is_packetized)
				return -1;
			ctx = c->pfmt_ctx;
			/* prepare header */
			if (avio_open_dyn_buf(&ctx->pb) < 0)
			{
				/* XXX: potential leak */
				return -1;
			}

			c->pfmt_ctx->pb->seekable = 0;
			av_write_trailer(ctx);
			len = avio_close_dyn_buf(ctx->pb, &c->pb_buffer);
			c->buffer_ptr = c->pb_buffer;
			c->buffer_end = c->pb_buffer + len;

			c->last_packet_sent = 1;
			break;
	}
	return 0;
}

/* return the estimated time (in us) at which the current packet must be sent */
static int64_t _getPacketSendClock(MuxContext *c)
{
	int bytes_left, bytes_sent, frame_bytes;

	frame_bytes = c->cur_frame_bytes;
	if (frame_bytes <= 0)
		return c->cur_pts;

	bytes_left = c->buffer_end - c->buffer_ptr;
	bytes_sent = frame_bytes - bytes_left;
	return c->cur_pts + (c->cur_frame_duration * bytes_sent) / frame_bytes;
}


/* return the server clock (in us) */
static int64_t _getServerClock(MuxContext *c)
{
	/* compute current pts value from system time */
	return (c->muxSvr->currentTime - c->start_time) * 1000;
}

/* should convert the format at the same time */
/* send data starting at c->buffer_ptr to the output connection
 * (either UDP or TCP)
 */
int muxSvrSendData(MuxContext *c)
{
	int len, ret;

	for(;;)
	{
		if (c->buffer_ptr >= c->buffer_end)
		{/* no free space to read data */
			ret = _httpPrepareData(c);
			if (ret < 0)
				return -1;
			else if (ret)
				/* state change requested */
				break;
		}
		else
		{
			if (c->is_packetized)
			{
				/* RTP data output */
				len = c->buffer_end - c->buffer_ptr;
				if (len < 4)
				{
					/* fail safe - should never happen */
fail1:
					c->buffer_ptr = c->buffer_end;
					return 0;
				}
				
				len = (c->buffer_ptr[0] << 24) |(c->buffer_ptr[1] << 16) |(c->buffer_ptr[2] << 8) |(c->buffer_ptr[3]);
				MUX_SVR_LOG("sub-packet length : %d\n", len);
				if (len > (c->buffer_end - c->buffer_ptr))
					goto fail1;
				
				if ((_getPacketSendClock(c) - _getServerClock(c)) > 0)
				{
					/* nothing to send yet: we can wait */
					return 0;
				}

				c->data_count += len;
				muxSvrUpdateDatarate(c->muxSvr, &c->datarate, c->data_count);
				if (c->stream)
					c->stream->bytes_served += len;

				if (c->rtp_protocol == RTSP_LOWER_TRANSPORT_TCP)
				{
					/* RTP packets are sent inside the RTSP TCP connection */
					AVIOContext *pb;
					int interleaved_index, size;
					uint8_t header[4];
					MuxContext *rtsp_c;

					rtsp_c = c->rtsp_c;
					/* if no RTSP connection left, error */
					if (!rtsp_c)
						return -1;
					/* if already sending something, then wait. */
					if (rtsp_c->state != RTSPSTATE_WAIT_REQUEST)
						break;
					if (avio_open_dyn_buf(&pb) < 0)
						goto fail1;
					
					interleaved_index = c->packet_stream_index * 2;
					/* RTCP packets are sent at odd indexes */
					if (c->buffer_ptr[1] == 200)
						interleaved_index++;
					
					/* write RTSP TCP header */
					header[0] = '$';
					header[1] = interleaved_index;
					header[2] = len >> 8;
					header[3] = len;
					avio_write(pb, header, 4);
					/* write RTP packet data */
					c->buffer_ptr += 4;
					avio_write(pb, c->buffer_ptr, len);
					size = avio_close_dyn_buf(pb, &c->packet_buffer);

					MUX_SVR_LOG("len %d, size : %d\n", len, size);
					/* prepare asynchronous TCP sending */
					rtsp_c->packet_buffer_ptr = c->packet_buffer;
					rtsp_c->packet_buffer_end = c->packet_buffer + size;
					c->buffer_ptr += len;

					/* send everything we can NOW */
					len = send(rtsp_c->fd, rtsp_c->packet_buffer_ptr, rtsp_c->packet_buffer_end - rtsp_c->packet_buffer_ptr, 0);
					if (len > 0)
						rtsp_c->packet_buffer_ptr += len;
					if (rtsp_c->packet_buffer_ptr < rtsp_c->packet_buffer_end)
					{
						/* if we could not send all the data, we will
						* send it later, so a new state is needed to
						* "lock" the RTSP TCP connection */
						rtsp_c->state = RTSPSTATE_SEND_PACKET;
						break;
					}
					else
						/* all data has been sent */
						av_freep(&c->packet_buffer);
				} 
				else
				{
					/* send RTP packet directly in UDP */
					c->buffer_ptr += 4;
					ffurl_write(c->rtp_handles[c->packet_stream_index], c->buffer_ptr, len);
					c->buffer_ptr += len;
					/* here we continue as we can send several packets per 10 ms slot */
				}
			} 
			else
			{
				/* TCP data output */
				len = send(c->fd, c->buffer_ptr, c->buffer_end - c->buffer_ptr, 0);
				if (len < 0)
				{
					if (ff_neterrno() != AVERROR(EAGAIN) && ff_neterrno() != AVERROR(EINTR))
					/* error : close connection */
						return -1;
					else
						return 0;
				}
				c->buffer_ptr += len;

				c->data_count += len;
				muxSvrUpdateDatarate(c->muxSvr, &c->datarate, c->data_count);
				if (c->stream)
					c->stream->bytes_served += len;
				break;
			}
		}
	} /* for(;;) */
	return 0;
}

