
/********************************************************************/
/* RTSP handling */

static void rtsp_reply_header(MuxContext *c, enum RTSPStatusCode error_number)
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

static void rtsp_reply_error(MuxContext *c, enum RTSPStatusCode error_number)
{
    rtsp_reply_header(c, error_number);
    avio_printf(c->pb, "\r\n");
}

static int rtsp_parse_request(MuxContext *c)
{
    const char *p, *p1, *p2;
    char cmd[32];
    char url[1024];
    char protocol[32];
    char line[1024];
    int len;
    RTSPMessageHeader header1 = { 0 }, *header = &header1;

    c->buffer_ptr[0] = '\0';
    p = c->buffer;

    get_word(cmd, sizeof(cmd), &p);
    get_word(url, sizeof(url), &p);
    get_word(protocol, sizeof(protocol), &p);

    av_strlcpy(c->method, cmd, sizeof(c->method));
    av_strlcpy(c->url, url, sizeof(c->url));
    av_strlcpy(c->protocol, protocol, sizeof(c->protocol));

    if (avio_open_dyn_buf(&c->pb) < 0) {
        /* XXX: cannot do more */
        c->pb = NULL; /* safety */
        return -1;
    }

    /* check version name */
    if (strcmp(protocol, "RTSP/1.0")) {
        rtsp_reply_error(c, RTSP_STATUS_VERSION);
        goto the_end;
    }

    /* parse each header line */
    /* skip to next line */
    while (*p != '\n' && *p != '\0')
        p++;
    if (*p == '\n')
        p++;
    while (*p != '\0') {
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
        rtsp_cmd_describe(c, url);
    else if (!strcmp(cmd, "OPTIONS"))
        rtsp_cmd_options(c, url);
    else if (!strcmp(cmd, "SETUP"))
        rtsp_cmd_setup(c, url, header);
    else if (!strcmp(cmd, "PLAY"))
        rtsp_cmd_play(c, url, header);
    else if (!strcmp(cmd, "PAUSE"))
        rtsp_cmd_interrupt(c, url, header, 1);
    else if (!strcmp(cmd, "TEARDOWN"))
        rtsp_cmd_interrupt(c, url, header, 0);
    else
        rtsp_reply_error(c, RTSP_STATUS_METHOD);

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

static int prepare_sdp_description(FFServerStream *stream, uint8_t **pbuffer,
                                   struct in_addr my_ip)
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
    av_dict_set(&avc->metadata, "title",
                entry ? entry->value : "No Title", 0);
    if (stream->is_multicast) {
        snprintf(avc->filename, 1024, "rtp://%s:%d?multicast=1?ttl=%d",
                 inet_ntoa(stream->multicast_ip),
                 stream->multicast_port, stream->multicast_ttl);
    } else
        snprintf(avc->filename, 1024, "rtp://0.0.0.0");

    for(i = 0; i < stream->nb_streams; i++) {
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
    av_sdp_create(&avc, 1, *pbuffer, PBUFFER_SIZE);

 sdp_done:
    av_freep(&avc->streams);
    av_dict_free(&avc->metadata);
    av_free(avc);

    return *pbuffer ? strlen(*pbuffer) : AVERROR(ENOMEM);
}

static void rtsp_cmd_options(MuxContext *c, const char *url)
{
    /* rtsp_reply_header(c, RTSP_STATUS_OK); */
    avio_printf(c->pb, "RTSP/1.0 %d %s\r\n", RTSP_STATUS_OK, "OK");
    avio_printf(c->pb, "CSeq: %d\r\n", c->seq);
    avio_printf(c->pb, "Public: %s\r\n",
                "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE");
    avio_printf(c->pb, "\r\n");
}

static void rtsp_cmd_describe(MuxContext *c, const char *url)
{
    FFServerStream *stream;
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

    for(stream = config.first_stream; stream; stream = stream->next) {
        if (!stream->is_feed &&
            stream->fmt && !strcmp(stream->fmt->name, "rtp") &&
            !strcmp(path, stream->filename)) {
            goto found;
        }
    }
    /* no stream found */
    rtsp_reply_error(c, RTSP_STATUS_NOT_FOUND);
    return;

 found:
    /* prepare the media description in SDP format */

    /* get the host IP */
    len = sizeof(my_addr);
    getsockname(c->fd, (struct sockaddr *)&my_addr, &len);
    content_length = prepare_sdp_description(stream, &content,
                                             my_addr.sin_addr);
    if (content_length < 0) {
        rtsp_reply_error(c, RTSP_STATUS_INTERNAL);
        return;
    }
    rtsp_reply_header(c, RTSP_STATUS_OK);
    avio_printf(c->pb, "Content-Base: %s/\r\n", url);
    avio_printf(c->pb, "Content-Type: application/sdp\r\n");
    avio_printf(c->pb, "Content-Length: %d\r\n", content_length);
    avio_printf(c->pb, "\r\n");
    avio_write(c->pb, content, content_length);
    av_free(content);
}

static MuxContext *find_rtp_session(const char *session_id)
{
    MuxContext *c;

    if (session_id[0] == '\0')
        return NULL;

    for(c = first_http_ctx; c; c = c->next) {
        if (!strcmp(c->session_id, session_id))
            return c;
    }
    return NULL;
}

static RTSPTransportField *find_transport(RTSPMessageHeader *h, enum RTSPLowerTransport lower_transport)
{
    RTSPTransportField *th;
    int i;

    for(i=0;i<h->nb_transports;i++) {
        th = &h->transports[i];
        if (th->lower_transport == lower_transport)
            return th;
    }
    return NULL;
}

static void rtsp_cmd_setup(MuxContext *c, const char *url,
                           RTSPMessageHeader *h)
{
    FFServerStream *stream;
    int stream_index, rtp_port, rtcp_port;
    char buf[1024];
    char path1[1024];
    const char *path;
    MuxContext *rtp_c;
    RTSPTransportField *th;
    struct sockaddr_in dest_addr;
    RTSPActionServerSetup setup;

    /* find which URL is asked */
    av_url_split(NULL, 0, NULL, 0, NULL, 0, NULL, path1, sizeof(path1), url);
    path = path1;
    if (*path == '/')
        path++;

    /* now check each stream */
    for(stream = config.first_stream; stream; stream = stream->next) {
        if (stream->is_feed || !stream->fmt ||
            strcmp(stream->fmt->name, "rtp")) {
            continue;
        }
        /* accept aggregate filenames only if single stream */
        if (!strcmp(path, stream->filename)) {
            if (stream->nb_streams != 1) {
                rtsp_reply_error(c, RTSP_STATUS_AGGREGATE);
                return;
            }
            stream_index = 0;
            goto found;
        }

        for(stream_index = 0; stream_index < stream->nb_streams;
            stream_index++) {
            snprintf(buf, sizeof(buf), "%s/streamid=%d",
                     stream->filename, stream_index);
            if (!strcmp(path, buf))
                goto found;
        }
    }
    /* no stream found */
    rtsp_reply_error(c, RTSP_STATUS_SERVICE); /* XXX: right error ? */
    return;
 found:

    /* generate session id if needed */
    if (h->session_id[0] == '\0') {
        unsigned random0 = av_lfg_get(&random_state);
        unsigned random1 = av_lfg_get(&random_state);
        snprintf(h->session_id, sizeof(h->session_id), "%08x%08x",
                 random0, random1);
    }

    /* find RTP session, and create it if none found */
    rtp_c = find_rtp_session(h->session_id);
    if (!rtp_c) {
        /* always prefer UDP */
        th = find_transport(h, RTSP_LOWER_TRANSPORT_UDP);
        if (!th) {
            th = find_transport(h, RTSP_LOWER_TRANSPORT_TCP);
            if (!th) {
                rtsp_reply_error(c, RTSP_STATUS_TRANSPORT);
                return;
            }
        }

        rtp_c = rtp_new_connection(&c->from_addr, stream, h->session_id,
                                   th->lower_transport);
        if (!rtp_c) {
            rtsp_reply_error(c, RTSP_STATUS_BANDWIDTH);
            return;
        }

        /* open input stream */
        if (open_input_stream(rtp_c, "") < 0) {
            rtsp_reply_error(c, RTSP_STATUS_INTERNAL);
            return;
        }
    }

    /* test if stream is OK (test needed because several SETUP needs
     * to be done for a given file) */
    if (rtp_c->stream != stream) {
        rtsp_reply_error(c, RTSP_STATUS_SERVICE);
        return;
    }

    /* test if stream is already set up */
    if (rtp_c->rtp_ctx[stream_index]) {
        rtsp_reply_error(c, RTSP_STATUS_STATE);
        return;
    }

    /* check transport */
    th = find_transport(h, rtp_c->rtp_protocol);
    if (!th || (th->lower_transport == RTSP_LOWER_TRANSPORT_UDP &&
                th->client_port_min <= 0)) {
        rtsp_reply_error(c, RTSP_STATUS_TRANSPORT);
        return;
    }

    /* setup default options */
    setup.transport_option[0] = '\0';
    dest_addr = rtp_c->from_addr;
    dest_addr.sin_port = htons(th->client_port_min);

    /* setup stream */
    if (rtp_new_av_stream(rtp_c, stream_index, &dest_addr, c) < 0) {
        rtsp_reply_error(c, RTSP_STATUS_TRANSPORT);
        return;
    }

    /* now everything is OK, so we can send the connection parameters */
    rtsp_reply_header(c, RTSP_STATUS_OK);
    /* session ID */
    avio_printf(c->pb, "Session: %s\r\n", rtp_c->session_id);

    switch(rtp_c->rtp_protocol) {
    case RTSP_LOWER_TRANSPORT_UDP:
        rtp_port = ff_rtp_get_local_rtp_port(rtp_c->rtp_handles[stream_index]);
        rtcp_port = ff_rtp_get_local_rtcp_port(rtp_c->rtp_handles[stream_index]);
        avio_printf(c->pb, "Transport: RTP/AVP/UDP;unicast;"
                    "client_port=%d-%d;server_port=%d-%d",
                    th->client_port_min, th->client_port_max,
                    rtp_port, rtcp_port);
        break;
    case RTSP_LOWER_TRANSPORT_TCP:
        avio_printf(c->pb, "Transport: RTP/AVP/TCP;interleaved=%d-%d",
                    stream_index * 2, stream_index * 2 + 1);
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
 * find an RTP connection by using the session ID. Check consistency
 * with filename
 */
static MuxContext *find_rtp_session_with_url(const char *url,
                                              const char *session_id)
{
    MuxContext *rtp_c;
    char path1[1024];
    const char *path;
    char buf[1024];
    int s, len;

    rtp_c = find_rtp_session(session_id);
    if (!rtp_c)
        return NULL;

    /* find which URL is asked */
    av_url_split(NULL, 0, NULL, 0, NULL, 0, NULL, path1, sizeof(path1), url);
    path = path1;
    if (*path == '/')
        path++;
    if(!strcmp(path, rtp_c->stream->filename)) return rtp_c;
    for(s=0; s<rtp_c->stream->nb_streams; ++s) {
      snprintf(buf, sizeof(buf), "%s/streamid=%d",
        rtp_c->stream->filename, s);
      if(!strncmp(path, buf, sizeof(buf)))
        /* XXX: Should we reply with RTSP_STATUS_ONLY_AGGREGATE
         * if nb_streams>1? */
        return rtp_c;
    }
    len = strlen(path);
    if (len > 0 && path[len - 1] == '/' &&
        !strncmp(path, rtp_c->stream->filename, len - 1))
        return rtp_c;
    return NULL;
}

static void rtsp_cmd_play(MuxContext *c, const char *url, RTSPMessageHeader *h)
{
    MuxContext *rtp_c;

    rtp_c = find_rtp_session_with_url(url, h->session_id);
    if (!rtp_c) {
        rtsp_reply_error(c, RTSP_STATUS_SESSION);
        return;
    }

    if (rtp_c->state != HTTPSTATE_SEND_DATA &&
        rtp_c->state != HTTPSTATE_WAIT_FEED &&
        rtp_c->state != HTTPSTATE_READY) {
        rtsp_reply_error(c, RTSP_STATUS_STATE);
        return;
    }

    rtp_c->state = HTTPSTATE_SEND_DATA;

    /* now everything is OK, so we can send the connection parameters */
    rtsp_reply_header(c, RTSP_STATUS_OK);
    /* session ID */
    avio_printf(c->pb, "Session: %s\r\n", rtp_c->session_id);
    avio_printf(c->pb, "\r\n");
}

static void rtsp_cmd_interrupt(MuxContext *c, const char *url,
                               RTSPMessageHeader *h, int pause_only)
{
    MuxContext *rtp_c;

    rtp_c = find_rtp_session_with_url(url, h->session_id);
    if (!rtp_c) {
        rtsp_reply_error(c, RTSP_STATUS_SESSION);
        return;
    }

    if (pause_only) {
        if (rtp_c->state != HTTPSTATE_SEND_DATA &&
            rtp_c->state != HTTPSTATE_WAIT_FEED) {
            rtsp_reply_error(c, RTSP_STATUS_STATE);
            return;
        }
        rtp_c->state = HTTPSTATE_READY;
        rtp_c->first_pts = AV_NOPTS_VALUE;
    }

    /* now everything is OK, so we can send the connection parameters */
    rtsp_reply_header(c, RTSP_STATUS_OK);
    /* session ID */
    avio_printf(c->pb, "Session: %s\r\n", rtp_c->session_id);
    avio_printf(c->pb, "\r\n");

    if (!pause_only)
        close_connection(rtp_c);
}

/********************************************************************/
/* RTP handling */

static MuxContext *rtp_new_connection(struct sockaddr_in *from_addr,
                                       FFServerStream *stream,
                                       const char *session_id,
                                       enum RTSPLowerTransport rtp_protocol)
{
    MuxContext *c = NULL;
    const char *proto_str;

    /* XXX: should output a warning page when coming
     * close to the connection limit */
    if (nb_connections >= config.nb_max_connections)
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
    nb_connections++;
    c->stream = stream;
    av_strlcpy(c->session_id, session_id, sizeof(c->session_id));
    c->state = HTTPSTATE_READY;
    c->is_packetized = 1;
    c->rtp_protocol = rtp_protocol;

    /* protocol is shown in statistics */
    switch(c->rtp_protocol) {
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

    current_bandwidth += stream->bandwidth;

    c->next = first_http_ctx;
    first_http_ctx = c;
    return c;

 fail:
    if (c) {
        av_freep(&c->buffer);
        av_free(c);
    }
    return NULL;
}

/**
 * add a new RTP stream in an RTP connection (used in RTSP SETUP
 * command). If RTP/TCP protocol is used, TCP connection 'rtsp_c' is
 * used.
 */
static int rtp_new_av_stream(MuxContext *c,
                             int stream_index, struct sockaddr_in *dest_addr,
                             MuxContext *rtsp_c)
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

    if (!c->stream->feed ||
        c->stream->feed == c->stream)
        unlayer_stream(st, c->stream->streams[stream_index]);
    else
        unlayer_stream(st,
               c->stream->feed->streams[c->stream->feed_streams[stream_index]]);
    av_assert0(st->priv_data == NULL);
    av_assert0(st->internal == st_internal);

    /* build destination RTP address */
    ipaddr = inet_ntoa(dest_addr->sin_addr);

    switch(c->rtp_protocol) {
    case RTSP_LOWER_TRANSPORT_UDP:
    case RTSP_LOWER_TRANSPORT_UDP_MULTICAST:
        /* RTP/UDP case */

        /* XXX: also pass as parameter to function ? */
        if (c->stream->is_multicast) {
            int ttl;
            ttl = c->stream->multicast_ttl;
            if (!ttl)
                ttl = 16;
            snprintf(ctx->filename, sizeof(ctx->filename),
                     "rtp://%s:%d?multicast=1&ttl=%d",
                     ipaddr, ntohs(dest_addr->sin_port), ttl);
        } else {
            snprintf(ctx->filename, sizeof(ctx->filename),
                     "rtp://%s:%d", ipaddr, ntohs(dest_addr->sin_port));
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

    http_log("%s:%d - - \"PLAY %s/streamid=%d %s\"\n",
             ipaddr, ntohs(dest_addr->sin_port),
             c->stream->filename, stream_index, c->protocol);

    /* normally, no packets should be output here, but the packet size may
     * be checked */
    if (ffio_open_dyn_packet_buf(&ctx->pb, max_packet_size) < 0)
        /* XXX: close stream */
        goto fail;

    if (avformat_write_header(ctx, NULL) < 0) {
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

