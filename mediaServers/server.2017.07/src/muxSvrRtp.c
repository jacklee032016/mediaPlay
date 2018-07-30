
#include "muxSvr.h"

/* RTP handling */

/**
 * add a new RTP stream in an RTP connection (used in RTSP SETUP command or multicast RTP). 
 If RTP/TCP protocol is used, TCP connection 'rtsp_c' is used.
 */
int muxSvrRtpNewAvStream(MuxContext *c, int stream_index, struct sockaddr_in *dest_addr, MuxContext *rtsp_c)
{
	AVFormatContext *ctx;
	AVStream *st;
	char *ipaddr;
	URLContext *h = NULL;
	uint8_t *dummy_buf;
	int max_packet_size;
	void *st_internal;

	/* now we can open the relevant output stream */
	ctx = avformat_alloc_context();
	if (!ctx)
		return -1;
	ctx->oformat = av_guess_format("rtp", NULL, NULL);

	st = avformat_new_stream(ctx, NULL);
	if (!st)
		goto fail;

	st_internal = st->internal;

	if (!c->stream->feed ||c->stream->feed == c->stream)
		unlayer_stream(st, c->stream->streams[stream_index]);
	else
		unlayer_stream(st, c->stream->feed->streams[c->stream->feed_streams[stream_index]]);
	av_assert0(st->priv_data == NULL);
	av_assert0(st->internal == st_internal);

	/* build destination RTP address */
	ipaddr = inet_ntoa(dest_addr->sin_addr);

	switch(c->rtp_protocol)
	{
		case RTSP_LOWER_TRANSPORT_UDP:
		case RTSP_LOWER_TRANSPORT_UDP_MULTICAST:
			/* RTP/UDP case */

			/* XXX: also pass as parameter to function ? */
			if (c->stream->is_multicast)
			{
				int ttl;
				ttl = c->stream->multicast_ttl;
				if (!ttl)
					ttl = 16;
				snprintf(ctx->filename, sizeof(ctx->filename), 	"rtp://%s:%d?multicast=1&ttl=%d", ipaddr, ntohs(dest_addr->sin_port), ttl);
			}
			else
			{
				snprintf(ctx->filename, sizeof(ctx->filename), 	"rtp://%s:%d", ipaddr, ntohs(dest_addr->sin_port));
			}

			if (ffurl_open(&h, ctx->filename, AVIO_FLAG_WRITE, NULL, NULL) < 0)
				goto fail;
			c->rtp_handles[stream_index] = h;
			max_packet_size = h->max_packet_size;
			break;
			
		case RTSP_LOWER_TRANSPORT_TCP:
			/* RTP/TCP case */
			c->rtsp_c = rtsp_c;
			max_packet_size = RTSP_TCP_MAX_PACKET_SIZE;
			break;
			
		default:
			goto fail;
	}

	MUX_SVR_LOG("%s:%d - - \"PLAY %s/streamid=%d %s\"\n", ipaddr, ntohs(dest_addr->sin_port), c->stream->filename, stream_index, c->protocol);

	/* normally, no packets should be output here, but the packet size may be checked */
	if (ffio_open_dyn_packet_buf(&ctx->pb, max_packet_size) < 0)
	/* XXX: close stream */
		goto fail;

	if (avformat_write_header(ctx, NULL) < 0)
	{
fail:
		if (h)
			ffurl_close(h);
		av_free(st);
		av_free(ctx);
		return -1;
	}
	
	avio_close_dyn_buf(ctx->pb, &dummy_buf);
	ctx->pb = NULL;
	av_free(dummy_buf);

	c->rtp_ctx[stream_index] = ctx;
	return 0;
}


int muxSvrPrepareSdpDescription(MuxStream *stream, uint8_t **pbuffer, struct in_addr my_ip)
{
	AVFormatContext *avc;
	AVOutputFormat *rtp_format = av_guess_format("rtp", NULL, NULL);
	AVDictionaryEntry *entry = av_dict_get(stream->metadata, "title", NULL, 0);
	int i;

	*pbuffer = NULL;

	avc =  avformat_alloc_context();
	if (!avc || !rtp_format)
		return -1;

	avc->oformat = rtp_format;
	av_dict_set(&avc->metadata, "title", entry ? entry->value : "No Title", 0);
	if (stream->is_multicast)
	{
		snprintf(avc->filename, 1024, "rtp://%s:%d?multicast=1?ttl=%d", inet_ntoa(stream->multicast_ip), stream->multicast_port, stream->multicast_ttl);
	}
	else
		snprintf(avc->filename, 1024, "rtp://0.0.0.0");

	for(i = 0; i < stream->nb_streams; i++)
	{
		AVStream *st = avformat_new_stream(avc, NULL);
		if (!st)
			goto sdp_done;
		avcodec_parameters_from_context(stream->streams[i]->codecpar, stream->streams[i]->codec);
		unlayer_stream(st, stream->streams[i]);
	}
#define PBUFFER_SIZE 2048

	*pbuffer = av_mallocz(PBUFFER_SIZE);
	if (!*pbuffer)
		goto sdp_done;
	av_sdp_create(&avc, 1, (char *)*pbuffer, PBUFFER_SIZE);

sdp_done:
	av_freep(&avc->streams);
	av_dict_free(&avc->metadata);
	av_free(avc);

	return *pbuffer ? strlen((char *)*pbuffer) : AVERROR(ENOMEM);
}

