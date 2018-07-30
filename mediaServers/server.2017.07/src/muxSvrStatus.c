
#include "muxSvr.h"

static void _fmtBytecount(AVIOContext *pb, int64_t count)
{
	static const char suffix[] = " kMGTP";
	const char *s;

	for (s = suffix; count >= 100000 && s[1]; count /= 1000, s++);

	avio_printf(pb, "%"PRId64"%c", count, *s);
}



static void _cleanHtml(char *clean, int clean_len, char *dirty)
{
	int i, o;

	for (o = i = 0; o+10 < clean_len && dirty[i];)
	{
		int len = strspn(dirty+i, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ$-_.+!*(),?/ :;%");
		if (len)
		{
			if (o + len >= clean_len)
				break;
			memcpy(clean + o, dirty + i, len);
			i += len;
			o += len;
		}
		else
		{
			int c = dirty[i++];
			switch (c)
			{
				case  '&':
					av_strlcat(clean+o, "&amp;"  , clean_len - o);
					break;
				case  '<':
					av_strlcat(clean+o, "&lt;"   , clean_len - o);
					break;
				case  '>':
					av_strlcat(clean+o, "&gt;"   , clean_len - o);
					break;
				case '\'':
					av_strlcat(clean+o, "&apos;" , clean_len - o);
					break;
				case '\"':
					av_strlcat(clean+o, "&quot;" , clean_len - o);
					break;
				default:
					av_strlcat(clean+o, "&#9785;", clean_len - o);
					break;
			}
			o += strlen(clean+o);
		}
	}
	
	clean[o] = 0;
}


static inline void _printStreamParams(AVIOContext *pb, MuxStream *stream)
{
	int i, stream_no;
	const char *type = "unknown";
	char parameters[64];
	LayeredAVStream *st;
	AVCodec *codec;

	stream_no = stream->nb_streams;

	avio_printf(pb, "<table><tr><th>Stream<th>type<th>kbit/s<th>codec<th>Parameters\n");

	for (i = 0; i < stream_no; i++)
	{
		st = stream->streams[i];
		codec = avcodec_find_encoder(st->codecpar->codec_id);

		parameters[0] = 0;

		switch(st->codecpar->codec_type)
		{
			case AVMEDIA_TYPE_AUDIO:
				type = "audio";
				snprintf(parameters, sizeof(parameters), "%d channel(s), %d Hz", st->codecpar->channels, st->codecpar->sample_rate);
				break;
			
			case AVMEDIA_TYPE_VIDEO:
				type = "video";
				snprintf(parameters, sizeof(parameters), "%dx%d, q=%d-%d, fps=%d", st->codecpar->width,
					st->codecpar->height, st->codec->qmin, st->codec->qmax, st->time_base.den / st->time_base.num);
				break;
				
			default:
				abort();
		}

		avio_printf(pb, "<tr><td>%d<td>%s<td>%"PRId64 "<td>%s<td>%s\n",
			i, type, (int64_t)st->codecpar->bit_rate/1000, codec ? codec->name : "", parameters);
	}

	avio_printf(pb, "</table>\n");
}

void muxSvrHttpStatusPage(MUX_SVR *muxSvr, MuxContext *c)
{
	MuxContext *c1;
	MuxStream *stream;
	char *p;
	time_t ti;
	int i, len;
	AVIOContext *pb;

	int index = 0;

VTRACE();
	if (avio_open_dyn_buf(&pb) < 0)
	{/* XXX: return an error ? */
		c->buffer_ptr = c->buffer;
		c->buffer_end = c->buffer;
		return;
	}

	avio_printf(pb, "HTTP/1.0 200 OK\r\n");
	avio_printf(pb, "Content-type: text/html\r\n");
	avio_printf(pb, "Pragma: no-cache\r\n");
	avio_printf(pb, "\r\n");

	avio_printf(pb, "<!DOCTYPE html>\n");
	avio_printf(pb, "<html><head><title>%s Status</title>\n", muxSvr->programName);
	if (c->stream->feed_filename[0])
		avio_printf(pb, "<link rel=\"shortcut icon\" href=\"%s\">\n", c->stream->feed_filename);

	avio_printf(pb, "</head>\n<body>");
	avio_printf(pb, "<h1>%s Status</h1>\n", muxSvr->programName);
	/* format status */
	avio_printf(pb, "<h2>Available Streams</h2>\n");
	avio_printf(pb, "<table  border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	avio_printf(pb, "<tr><th>Path<th>Served<br>Conns<th><br>bytes<th>Format<th>Bit rate<br>kbit/s<th>Video<br>kbit/s<th><br>Codec<th>Audio<br>kbit/s<th><br>Codec<th>Feed\n");

	stream = muxSvr->config.first_stream;
	while (stream)
	{
		char sfilename[1024];
		char *eosf;

		if (stream->feed == stream)
		{
			stream = stream->next;
			continue;
		}

		av_strlcpy(sfilename, stream->filename, sizeof(sfilename) - 10);
		eosf = sfilename + strlen(sfilename);
		if (eosf - sfilename >= 4)
		{
			if (strcmp(eosf - 4, ".asf") == 0)
				strcpy(eosf - 4, ".asx");
			else if (strcmp(eosf - 3, ".rm") == 0)
				strcpy(eosf - 3, ".ram");
			else if (stream->fmt && !strcmp(stream->fmt->name, "rtp"))
			{
				/* generate a sample RTSP director if unicast. Generate an SDP redirector if multicast */
				eosf = strrchr(sfilename, '.');
				if (!eosf)
					eosf = sfilename + strlen(sfilename);
				
				if (stream->is_multicast)
					strcpy(eosf, ".sdp");
				else
					strcpy(eosf, ".rtsp");
			}
		}

VTRACE();
		avio_printf(pb, "<tr><td><a href=\"/%s\">%s</a> ", sfilename, stream->filename);
		avio_printf(pb, "<td> %d <td> ", stream->conns_served);
		/* TODO: Investigate if we can make http bitexact so it always produces the same count of bytes */
		if (!muxSvr->config.bitexact)
			_fmtBytecount(pb, stream->bytes_served);

		MUX_SVR_LOG("No. %d stream is '%s(%s)'\n", ++index, sfilename, stream->filename);
VTRACE();
		switch(stream->stream_type)
		{
			case STREAM_TYPE_LIVE:
			{
				int audio_bit_rate = 0;
				int video_bit_rate = 0;
				const char *audio_codec_name = "";
				const char *video_codec_name = "";
				const char *audio_codec_name_extra = "";
				const char *video_codec_name_extra = "";

				for(i=0; i<stream->nb_streams;i++)
				{
					LayeredAVStream *st = stream->streams[i];
					AVCodec *codec = avcodec_find_encoder(st->codecpar->codec_id);

					MUX_SVR_LOG("\tNo.%d sub-stream\n", i);
VTRACE();
					switch(st->codecpar->codec_type)
					{
						case AVMEDIA_TYPE_AUDIO:
							MUX_SVR_LOG("\tNo.%d sub-stream is AUDIO Stream\n", i);
							audio_bit_rate += st->codecpar->bit_rate;
							if (codec)
							{
								if (*audio_codec_name)
									audio_codec_name_extra = "...";
								audio_codec_name = codec->name;
							}
							break;
						
						case AVMEDIA_TYPE_VIDEO:
							MUX_SVR_LOG("\tNo.%d substream is VIDEO Stream\n", i);
							video_bit_rate += st->codecpar->bit_rate;
							if (codec)
							{
								if (*video_codec_name)
									video_codec_name_extra = "...";
								video_codec_name = codec->name;
							}
							break;
						
						case AVMEDIA_TYPE_DATA:
							MUX_SVR_LOG("\tNo.%d substream is DATA Stream\n", i);
							video_bit_rate += st->codecpar->bit_rate;
							break;
							
						default:
							MUX_SVR_LOG("\tNo.%d substream is invalidate type=%d, abort\n", i, st->codecpar->codec_type);
							break;
							abort();
					}
				}

				avio_printf(pb, "<td> %s <td> %d <td> %d <td> %s %s <td> %d <td> %s %s",
					stream->fmt->name, stream->bandwidth, video_bit_rate / 1000, video_codec_name,
					video_codec_name_extra, audio_bit_rate / 1000, audio_codec_name, audio_codec_name_extra);

				if (stream->feed)
				{
					avio_printf(pb, "<td>%s", stream->feed->filename);
					MUX_SVR_LOG("No.%d :%s\n", i, stream->feed->filename);
				}
				else
				{
					avio_printf(pb, "<td>%s", stream->feed_filename);
					MUX_SVR_LOG("No.%d :%s\n", i, stream->feed_filename);
				}

				avio_printf(pb, "\n");
			}
			break;

			default:
				avio_printf(pb, "<td> - <td> - <td> - <td><td> - <td>\n");
				break;
		}
VTRACE();
		stream = stream->next;
	}
	avio_printf(pb, "</table>\n");

VTRACE();
	stream = muxSvr->config.first_stream;
	while (stream)
	{
		if (stream->feed != stream)
		{
			stream = stream->next;
			continue;
		}

		avio_printf(pb, "<h2>Feed %s</h2>", stream->filename);
		if (stream->pid)
		{
			avio_printf(pb, "Running as pid %"PRId64".\n", (int64_t) stream->pid);

#if defined(linux)
			{
				FILE *pid_stat;
				char ps_cmd[64];

				/* This is somewhat linux specific I guess */
				snprintf(ps_cmd, sizeof(ps_cmd), "ps -o \"%%cpu,cputime\" --no-headers %"PRId64"", (int64_t) stream->pid);

				pid_stat = popen(ps_cmd, "r");
				if (pid_stat)
				{
					char cpuperc[10];
					char cpuused[64];

					if (fscanf(pid_stat, "%9s %63s", cpuperc, cpuused) == 2)
					{
						avio_printf(pb, "Currently using %s%% of the cpu. Total time used %s.\n", cpuperc, cpuused);
					}
					fclose(pid_stat);
				}
			}
#endif

			avio_printf(pb, "<p>");
		}

		_printStreamParams(pb, stream);
		stream = stream->next;
	}

	/* connection status */
	avio_printf(pb, "<h2>Connection Status</h2>\n");

	avio_printf(pb, "Number of connections: %d / %d<br>\n", muxSvr->nb_connections, muxSvr->config.nb_max_connections);

	avio_printf(pb, "Bandwidth in use: %"PRIu64"k / %"PRIu64"k<br>\n",  muxSvr->current_bandwidth, muxSvr->config.max_bandwidth);

	avio_printf(pb, "<table>\n");
	avio_printf(pb, "<tr><th>#<th>File<th>IP<th>URL<th>Proto<th>State<th>Target bit/s<th>Actual bit/s<th>Bytes transferred\n");

	c1 = muxSvr->firstConnectCtx;
	i = 0;
	while (c1)
	{
		int bitrate;
		int j;

		bitrate = 0;
		if (c1->stream)
		{
			for (j = 0; j < c1->stream->nb_streams; j++)
			{
				if (!c1->stream->feed)
					bitrate += c1->stream->streams[j]->codecpar->bit_rate;
				else if (c1->feed_streams[j] >= 0)
					bitrate += c1->stream->feed->streams[c1->feed_streams[j]]->codecpar->bit_rate;
			}
		}

		i++;
		p = inet_ntoa(c1->from_addr.sin_addr);
		_cleanHtml(c1->clean_url, sizeof(c1->clean_url), c1->url);
		avio_printf(pb, "<tr><td><b>%d</b><td>%s%s<td>%s<td>%s<td>%s<td>%s<td>",
			i, c1->stream ? c1->stream->filename : "", c1->state == HTTPSTATE_RECEIVE_DATA ? "(input)" : "",
			p, c1->clean_url, c1->protocol, muxSvrStatusName(c1->state) );

		_fmtBytecount(pb, bitrate);
		avio_printf(pb, "<td>");
		_fmtBytecount(pb, muxSvrComputeDatarate(muxSvr, &c1->datarate, c1->data_count) * 8);
		avio_printf(pb, "<td>");
		_fmtBytecount(pb, c1->data_count);
		avio_printf(pb, "\n");
		c1 = c1->next;
	}
	avio_printf(pb, "</table>\n");

	if (!muxSvr->config.bitexact)
	{
		/* date */
		ti = time(NULL);
		p = ctime(&ti);
		avio_printf(pb, "<hr>Generated at %s", p);
	}
	avio_printf(pb, "</body>\n</html>\n");

VTRACE();
	len = avio_close_dyn_buf(pb, &c->pb_buffer);
	c->buffer_ptr = c->pb_buffer;
	c->buffer_end = c->pb_buffer + len;
}

