/*
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * multiple format streaming server based on the FFmpeg libraries
 */

#include "config.h"
#if !HAVE_CLOSESOCKET
#define closesocket close
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "libavformat/avformat.h"
/* FIXME: those are internal headers, ffserver _really_ shouldn't use them */
#include "libavformat/rtpproto.h"
#include "libavformat/rtsp.h"
#include "libavformat/avio_internal.h"
#include "libavformat/internal.h"

#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/lfg.h"
#include "libavutil/dict.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/mathematics.h"
#include "libavutil/random_seed.h"
#include "libavutil/rational.h"
#include "libavutil/parseutils.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"

#include <stdarg.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/ioctl.h>
#if HAVE_POLL_H
#include <poll.h>
#endif
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

#include "cmdutils.h"
#include "muxServer.h"


const char program_name[] = "ffserver";
const int program_birth_year = 2000;

static const OptionDef options[];


static const char * const http_state[] = {
    "HTTP_WAIT_REQUEST",
    "HTTP_SEND_HEADER",

    "SEND_DATA_HEADER",
    "SEND_DATA",
    "SEND_DATA_TRAILER",
    "RECEIVE_DATA",
    "WAIT_FEED",
    "READY",

    "RTSP_WAIT_REQUEST",
    "RTSP_SEND_REPLY",
    "RTSP_SEND_PACKET",
};

static MuxContext *first_http_ctx;

static FFServerConfig config =
{
	.nb_max_http_connections = 2000,
	.nb_max_connections = 5,
	.max_bandwidth = 1000,
	.use_defaults = 1,
};


static void new_connection(int server_fd, int is_rtsp);
static void close_connection(MuxContext *c);

/* HTTP handling */
static int handle_connection(MuxContext *c);
static inline void print_stream_params(AVIOContext *pb, FFServerStream *stream);
static void compute_status(MuxContext *c);
static int open_input_stream(MuxContext *c, const char *info);
static int http_parse_request(MuxContext *c);
static int http_send_data(MuxContext *c);
static int http_start_receive_data(MuxContext *c);
static int http_receive_data(MuxContext *c);

/* RTSP handling */
static int rtsp_parse_request(MuxContext *c);
static void rtsp_cmd_describe(MuxContext *c, const char *url);
static void rtsp_cmd_options(MuxContext *c, const char *url);
static void rtsp_cmd_setup(MuxContext *c, const char *url,
                           RTSPMessageHeader *h);
static void rtsp_cmd_play(MuxContext *c, const char *url,
                          RTSPMessageHeader *h);
static void rtsp_cmd_interrupt(MuxContext *c, const char *url,
                               RTSPMessageHeader *h, int pause_only);

/* SDP handling */
static int prepare_sdp_description(FFServerStream *stream, uint8_t **pbuffer,
                                   struct in_addr my_ip);

/* RTP handling */
static MuxContext *rtp_new_connection(struct sockaddr_in *from_addr,
                                       FFServerStream *stream,
                                       const char *session_id,
                                       enum RTSPLowerTransport rtp_protocol);
static int rtp_new_av_stream(MuxContext *c,
                             int stream_index, struct sockaddr_in *dest_addr,
                             MuxContext *rtsp_c);
/* utils */
static size_t htmlencode (const char *src, char **dest);
static inline void cp_html_entity (char *buffer, const char *entity);

static const char *my_program_name;

static int no_launch;
static int need_to_start_children;

/* maximum number of simultaneous HTTP connections */
static unsigned int nb_connections;

static uint64_t current_bandwidth;

/* Making this global saves on passing it around everywhere */
static int64_t cur_time;

static AVLFG random_state;

static FILE *logfile = NULL;

static void unlayer_stream(AVStream *st, LayeredAVStream *lst)
{
    avcodec_free_context(&st->codec);
    avcodec_parameters_free(&st->codecpar);
#define COPY(a) st->a = lst->a;
    COPY(index)
    COPY(id)
    COPY(codec)
    COPY(codecpar)
    COPY(time_base)
    COPY(pts_wrap_bits)
    COPY(sample_aspect_ratio)
    COPY(recommended_encoder_configuration)
}

static inline void cp_html_entity (char *buffer, const char *entity)
{
	if (!buffer || !entity)
		return;
	while (*entity)
		*buffer++ = *entity++;
}

/**
 * Substitutes known conflicting chars on a text string with
 * their corresponding HTML entities.
 *
 * Returns the number of bytes in the 'encoded' representation
 * not including the terminating NUL.
 */
static size_t htmlencode (const char *src, char **dest)
{
    const char *amp = "&amp;";
    const char *lt  = "&lt;";
    const char *gt  = "&gt;";
    const char *start;
    char *tmp;
    size_t final_size = 0;

    if (!src)
        return 0;

    start = src;

    /* Compute needed dest size */
    while (*src != '\0') {
        switch(*src) {
            case 38: /* & */
                final_size += 5;
                break;
            case 60: /* < */
            case 62: /* > */
                final_size += 4;
                break;
            default:
                final_size++;
        }
        src++;
    }

    src = start;
    *dest = av_mallocz(final_size + 1);
    if (!*dest)
        return 0;

    /* Build dest */
    tmp = *dest;
    while (*src != '\0') {
        switch(*src) {
            case 38: /* & */
                cp_html_entity (tmp, amp);
                tmp += 5;
                break;
            case 60: /* < */
                cp_html_entity (tmp, lt);
                tmp += 4;
                break;
            case 62: /* > */
                cp_html_entity (tmp, gt);
                tmp += 4;
                break;
            default:
                *tmp = *src;
                tmp += 1;
        }
        src++;
    }
    *tmp = '\0';

    return final_size;
}

static int64_t ffm_read_write_index(int fd)
{
	uint8_t buf[8];

	if (lseek(fd, 8, SEEK_SET) < 0)
		return AVERROR(EIO);
	if (read(fd, buf, 8) != 8)
		return AVERROR(EIO);
	return AV_RB64(buf);
}

static int ffm_write_write_index(int fd, int64_t pos)
{
    uint8_t buf[8];
    int i;

    for(i=0;i<8;i++)
        buf[i] = (pos >> (56 - i * 8)) & 0xff;
    if (lseek(fd, 8, SEEK_SET) < 0)
        goto bail_eio;
    if (write(fd, buf, 8) != 8)
        goto bail_eio;

    return 8;

bail_eio:
    return AVERROR(EIO);
}

static void ffm_set_write_index(AVFormatContext *s, int64_t pos, int64_t file_size)
{
	av_opt_set_int(s, "server_attached", 1, AV_OPT_SEARCH_CHILDREN);
	av_opt_set_int(s, "ffm_write_index", pos, AV_OPT_SEARCH_CHILDREN);
	av_opt_set_int(s, "ffm_file_size", file_size, AV_OPT_SEARCH_CHILDREN);
}

static char *ctime1(char *buf2, size_t buf_size)
{
    time_t ti;
    char *p;

    ti = time(NULL);
    p = ctime(&ti);
    if (!p || !*p) {
        *buf2 = '\0';
        return buf2;
    }
    av_strlcpy(buf2, p, buf_size);
    p = buf2 + strlen(buf2) - 1;
    if (*p == '\n')
        *p = '\0';
    return buf2;
}

static void http_vlog(const char *fmt, va_list vargs)
{
	static int print_prefix = 1;
	char buf[32];

	if (!logfile)
		return;

	if (print_prefix)
	{
		ctime1(buf, sizeof(buf));
		fprintf(logfile, "%s ", buf);
	}

	print_prefix = strstr(fmt, "\n") != NULL;
	vfprintf(logfile, fmt, vargs);
	fflush(logfile);
}

#if  0 //def __GNUC__
__attribute__ ((format (printf, 1, 2)))
#endif
static void http_log(const char *fmt, ...)
{
	va_list vargs;
	va_start(vargs, fmt);
	http_vlog(fmt, vargs);
	va_end(vargs);
}

static void http_av_log(void *ptr, int level, const char *fmt, va_list vargs)
{
	static int print_prefix = 1;
	AVClass *avc = ptr ? *(AVClass**)ptr : NULL;
	if (level > av_log_get_level())
		return;
	if (print_prefix && avc)
		http_log("[%s @ %p]", avc->item_name(ptr), ptr);
	print_prefix = strstr(fmt, "\n") != NULL;
	http_vlog(fmt, vargs);
}

static void log_connection(MuxContext *c)
{
	if (c->suppress_log)
		return;

	http_log("%s - - [%s] \"%s %s\" %d %"PRId64"\n",
		inet_ntoa(c->from_addr.sin_addr), c->method, c->url,
		c->protocol, (c->http_error ? c->http_error : 200), c->data_count);
}

static void update_datarate(DataRateData *drd, int64_t count)
{
	if (!drd->time1 && !drd->count1)
	{
		drd->time1 = drd->time2 = cur_time;
		drd->count1 = drd->count2 = count;
	}
	else if (cur_time - drd->time2 > 5000)
	{
		drd->time1 = drd->time2;
		drd->count1 = drd->count2;
		drd->time2 = cur_time;
		drd->count2 = count;
	}
}

/* In bytes per second */
static int compute_datarate(DataRateData *drd, int64_t count)
{
	if (cur_time == drd->time1)
		return 0;

	return ((count - drd->count1) * 1000) / (cur_time - drd->time1);
}



/* start waiting for a new HTTP/RTSP request */
static void start_wait_request(MuxContext *c, int is_rtsp)
{
    c->buffer_ptr = c->buffer;
    c->buffer_end = c->buffer + c->buffer_size - 1; /* leave room for '\0' */

    c->state = is_rtsp ? RTSPSTATE_WAIT_REQUEST : HTTPSTATE_WAIT_REQUEST;
    c->timeout = cur_time +
                 (is_rtsp ? RTSP_REQUEST_TIMEOUT : HTTP_REQUEST_TIMEOUT);
}

static int extract_rates(char *rates, int ratelen, const char *request)
{
    const char *p;

    for (p = request; *p && *p != '\r' && *p != '\n'; ) {
        if (av_strncasecmp(p, "Pragma:", 7) == 0) {
            const char *q = p + 7;

            while (*q && *q != '\n' && av_isspace(*q))
                q++;

            if (av_strncasecmp(q, "stream-switch-entry=", 20) == 0) {
                int stream_no;
                int rate_no;

                q += 20;

                memset(rates, 0xff, ratelen);

                while (1) {
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

static int find_stream_in_feed(FFServerStream *feed, AVCodecParameters *codec,
                               int bit_rate)
{
    int i;
    int best_bitrate = 100000000;
    int best = -1;

    for (i = 0; i < feed->nb_streams; i++) {
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

        if (feed_codec->bit_rate <= bit_rate) {
            if (best_bitrate > bit_rate ||
                feed_codec->bit_rate > best_bitrate) {
                best_bitrate = feed_codec->bit_rate;
                best = i;
            }
            continue;
        }
        if (feed_codec->bit_rate < best_bitrate) {
            best_bitrate = feed_codec->bit_rate;
            best = i;
        }
    }
    return best;
}

static int modify_current_stream(MuxContext *c, char *rates)
{
    int i;
    FFServerStream *req = c->stream;
    int action_required = 0;

    /* Not much we can do for a feed */
    if (!req->feed)
        return 0;

    for (i = 0; i < req->nb_streams; i++) {
        AVCodecParameters *codec = req->streams[i]->codecpar;

        switch(rates[i]) {
            case 0:
                c->switch_feed_streams[i] = req->feed_streams[i];
                break;
            case 1:
                c->switch_feed_streams[i] = find_stream_in_feed(req->feed, codec, codec->bit_rate / 2);
                break;
            case 2:
                /* Wants off or slow */
                c->switch_feed_streams[i] = find_stream_in_feed(req->feed, codec, codec->bit_rate / 4);
#ifdef WANTS_OFF
                /* This doesn't work well when it turns off the only stream! */
                c->switch_feed_streams[i] = -2;
                c->feed_streams[i] = -2;
#endif
                break;
        }

        if (c->switch_feed_streams[i] >= 0 &&
            c->switch_feed_streams[i] != c->feed_streams[i]) {
            action_required = 1;
        }
    }

    return action_required;
}

static void get_word(char *buf, int buf_size, const char **pp)
{
    const char *p;
    char *q;

#define SPACE_CHARS " \t\r\n"

    p = *pp;
    p += strspn(p, SPACE_CHARS);
    q = buf;
    while (!av_isspace(*p) && *p != '\0') {
        if ((q - buf) < buf_size - 1)
            *q++ = *p;
        p++;
    }
    if (buf_size > 0)
        *q = '\0';
    *pp = p;
}

/**
 * compute the real filename of a file by matching it without its
 * extensions to all the stream's filenames
 */
static void compute_real_filename(char *filename, int max_size)
{
    char file1[1024];
    char file2[1024];
    char *p;
    FFServerStream *stream;

    av_strlcpy(file1, filename, sizeof(file1));
    p = strrchr(file1, '.');
    if (p)
        *p = '\0';
    for(stream = config.first_stream; stream; stream = stream->next) {
        av_strlcpy(file2, stream->filename, sizeof(file2));
        p = strrchr(file2, '.');
        if (p)
            *p = '\0';
        if (!strcmp(file1, file2)) {
            av_strlcpy(filename, stream->filename, max_size);
            break;
        }
    }
}


static inline void print_stream_params(AVIOContext *pb, FFServerStream *stream)
{
    int i, stream_no;
    const char *type = "unknown";
    char parameters[64];
    LayeredAVStream *st;
    AVCodec *codec;

    stream_no = stream->nb_streams;

    avio_printf(pb, "<table><tr><th>Stream<th>"
                    "type<th>kbit/s<th>codec<th>"
                    "Parameters\n");

    for (i = 0; i < stream_no; i++) {
        st = stream->streams[i];
        codec = avcodec_find_encoder(st->codecpar->codec_id);

        parameters[0] = 0;

        switch(st->codecpar->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            type = "audio";
            snprintf(parameters, sizeof(parameters), "%d channel(s), %d Hz",
                     st->codecpar->channels, st->codecpar->sample_rate);
            break;
        case AVMEDIA_TYPE_VIDEO:
            type = "video";
            snprintf(parameters, sizeof(parameters),
                     "%dx%d, q=%d-%d, fps=%d", st->codecpar->width,
                     st->codecpar->height, st->codec->qmin, st->codec->qmax,
                     st->time_base.den / st->time_base.num);
            break;
        default:
            abort();
        }

        avio_printf(pb, "<tr><td>%d<td>%s<td>%"PRId64
                        "<td>%s<td>%s\n",
                    i, type, (int64_t)st->codecpar->bit_rate/1000,
                    codec ? codec->name : "", parameters);
     }

     avio_printf(pb, "</table>\n");
}

static void clean_html(char *clean, int clean_len, char *dirty)
{
    int i, o;

    for (o = i = 0; o+10 < clean_len && dirty[i];) {
        int len = strspn(dirty+i, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ$-_.+!*(),?/ :;%");
        if (len) {
            if (o + len >= clean_len)
                break;
            memcpy(clean + o, dirty + i, len);
            i += len;
            o += len;
        } else {
            int c = dirty[i++];
            switch (c) {
            case  '&': av_strlcat(clean+o, "&amp;"  , clean_len - o); break;
            case  '<': av_strlcat(clean+o, "&lt;"   , clean_len - o); break;
            case  '>': av_strlcat(clean+o, "&gt;"   , clean_len - o); break;
            case '\'': av_strlcat(clean+o, "&apos;" , clean_len - o); break;
            case '\"': av_strlcat(clean+o, "&quot;" , clean_len - o); break;
            default:   av_strlcat(clean+o, "&#9785;", clean_len - o); break;
            }
            o += strlen(clean+o);
        }
    }
    clean[o] = 0;
}

static int open_input_stream(MuxContext *c, const char *info)
{
    char buf[128];
    char input_filename[1024];
    AVFormatContext *s = NULL;
    int buf_size, i, ret;
    int64_t stream_pos;

    /* find file name */
    if (c->stream->feed) {
        strcpy(input_filename, c->stream->feed->feed_filename);
        buf_size = FFM_PACKET_SIZE;
        /* compute position (absolute time) */
        if (av_find_info_tag(buf, sizeof(buf), "date", info)) {
            if ((ret = av_parse_time(&stream_pos, buf, 0)) < 0) {
                http_log("Invalid date specification '%s' for stream\n", buf);
                return ret;
            }
        } else if (av_find_info_tag(buf, sizeof(buf), "buffer", info)) {
            int prebuffer = strtol(buf, 0, 10);
            stream_pos = av_gettime() - prebuffer * (int64_t)1000000;
        } else
            stream_pos = av_gettime() - c->stream->prebuffer * (int64_t)1000;
    } else {
        strcpy(input_filename, c->stream->feed_filename);
        buf_size = 0;
        /* compute position (relative time) */
        if (av_find_info_tag(buf, sizeof(buf), "date", info)) {
            if ((ret = av_parse_time(&stream_pos, buf, 1)) < 0) {
                http_log("Invalid date specification '%s' for stream\n", buf);
                return ret;
            }
        } else
            stream_pos = 0;
    }
    if (!input_filename[0]) {
        http_log("No filename was specified for stream\n");
        return AVERROR(EINVAL);
    }

    /* open stream */
    ret = avformat_open_input(&s, input_filename, c->stream->ifmt,
                              &c->stream->in_opts);
    if (ret < 0) {
        http_log("Could not open input '%s': %s\n",
                 input_filename, av_err2str(ret));
        return ret;
    }

    /* set buffer size */
    if (buf_size > 0) {
        ret = ffio_set_buf_size(s->pb, buf_size);
        if (ret < 0) {
            http_log("Failed to set buffer size\n");
            return ret;
        }
    }

    s->flags |= AVFMT_FLAG_GENPTS;
    c->fmt_in = s;
    if (strcmp(s->iformat->name, "ffm") &&
        (ret = avformat_find_stream_info(c->fmt_in, NULL)) < 0) {
        http_log("Could not find stream info for input '%s'\n", input_filename);
        avformat_close_input(&s);
        return ret;
    }

    /* choose stream as clock source (we favor the video stream if
     * present) for packet sending */
    c->pts_stream_index = 0;
    for(i=0;i<c->stream->nb_streams;i++) {
        if (c->pts_stream_index == 0 &&
            c->stream->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            c->pts_stream_index = i;
        }
    }

    if (c->fmt_in->iformat->read_seek)
        av_seek_frame(c->fmt_in, -1, stream_pos, 0);
    /* set the start time (needed for maxtime and RTP packet timing) */
    c->start_time = cur_time;
    c->first_pts = AV_NOPTS_VALUE;
    return 0;
}

/* return the server clock (in us) */
static int64_t get_server_clock(MuxContext *c)
{
    /* compute current pts value from system time */
    return (cur_time - c->start_time) * 1000;
}

/* return the estimated time (in us) at which the current packet must be sent */
static int64_t get_packet_send_clock(MuxContext *c)
{
    int bytes_left, bytes_sent, frame_bytes;

    frame_bytes = c->cur_frame_bytes;
    if (frame_bytes <= 0)
        return c->cur_pts;

    bytes_left = c->buffer_end - c->buffer_ptr;
    bytes_sent = frame_bytes - bytes_left;
    return c->cur_pts + (c->cur_frame_duration * bytes_sent) / frame_bytes;
}


/* compute the bandwidth used by each stream */
static void compute_bandwidth(void)
{
    unsigned bandwidth;
    int i;
    FFServerStream *stream;

    for(stream = config.first_stream; stream; stream = stream->next) {
        bandwidth = 0;
        for(i=0;i<stream->nb_streams;i++) {
            LayeredAVStream *st = stream->streams[i];
            switch(st->codec->codec_type) {
            case AVMEDIA_TYPE_AUDIO:
            case AVMEDIA_TYPE_VIDEO:
                bandwidth += st->codec->bit_rate;
                break;
            default:
                break;
            }
        }
        stream->bandwidth = (bandwidth + 999) / 1000;
    }
}

static void handle_child_exit(int sig)
{
    pid_t pid;
    int status;
    time_t uptime;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        FFServerStream *feed;

        for (feed = config.first_feed; feed; feed = feed->next) {
            if (feed->pid != pid)
                continue;

            uptime = time(0) - feed->pid_start;
            feed->pid = 0;
            fprintf(stderr,
                    "%s: Pid %"PRId64" exited with status %d after %"PRId64" "
                        "seconds\n",
                    feed->filename, (int64_t) pid, status, (int64_t)uptime);

            if (uptime < 30)
                /* Turn off any more restarts */
                ffserver_free_child_args(&feed->child_argv);
        }
    }

    need_to_start_children = 1;
}

static void opt_debug(void)
{
    config.debug = 1;
    snprintf(config.logfilename, sizeof(config.logfilename), "-");
}

void show_help_default(const char *opt, const char *arg)
{
    show_help_options(options, "Main options:", 0, 0, 0);
}

static const OptionDef options[] = {
#include "cmdutils_common_opts.h"
    { "n", OPT_BOOL, {(void *)&no_launch }, "enable no-launch mode" },
    { "d", 0, {(void*)opt_debug}, "enable debug mode" },
    { "f", HAS_ARG | OPT_STRING, {(void*)&config.filename }, "use configfile instead of /etc/ffserver.conf", "configfile" },
    { NULL },
};

int main(int argc, char **argv)
{
	struct sigaction sigact = { { 0 } };
	int cfg_parsed;
	int ret = EXIT_FAILURE;

	init_dynload();

	config.filename = av_strdup("/etc/ffserver.conf");

	parse_loglevel(argc, argv, options);
	av_register_all();
	avformat_network_init();

	show_banner(argc, argv, options);

	my_program_name = argv[0];

	parse_options(NULL, argc, argv, options, NULL);

	unsetenv("http_proxy");             /* Kill the http_proxy */

	av_lfg_init(&random_state, av_get_random_seed());

	sigact.sa_handler = handle_child_exit;
	sigact.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction(SIGCHLD, &sigact, 0);

	if ((cfg_parsed = ffserver_parse_ffconfig(config.filename, &config)) < 0)
	{
		fprintf(stderr, "Error reading configuration file '%s': %s\n", config.filename, av_err2str(cfg_parsed));
		goto bail;
	}

	/* open log file if needed */
	if (config.logfilename[0] != '\0')
	{
		if (!strcmp(config.logfilename, "-"))
			logfile = stdout;
		else
			logfile = fopen(config.logfilename, "a");
		av_log_set_callback(http_av_log);
	}

	build_file_streams();

	if (build_feed_streams() < 0)
	{
		http_log("Could not setup feed streams\n");
		goto bail;
	}

	compute_bandwidth();

	/* signal init */
	signal(SIGPIPE, SIG_IGN);

	if (http_server() < 0)
	{
		http_log("Could not start server\n");
		goto bail;
	}

	ret=EXIT_SUCCESS;

bail:
	av_freep (&config.filename);
	avformat_network_deinit();
	
	return ret;
}

