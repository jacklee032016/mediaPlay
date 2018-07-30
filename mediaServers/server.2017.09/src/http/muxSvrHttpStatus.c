/*
* $Id$
*/


#include "muxSvr.h"

#include "__services.h"

#if WITH_SERVER_HTTP_STATS

#define	BG_COLOR			" bgcolor=\"#cccccc\" "

static void fmt_bytecount(AVIOContext *pb, int64_t count)
{
	static const char *suffix = " kMGTP";
	const char *s;

	for (s = suffix; count >= 100000 && s[1]; count /= 1000, s++)
	{
	}

	avio_printf(pb, "%lld%c", count, *s);
}


/* output URL and format. svrConn is HTTP conn for status info */
static int _streamInfo(SERVER_CONNECT *svrConn, MuxStream *stream)
{
	char sfilename[1024];
	char *eosf;
	char format[256];

	switch(stream->type)
	{
		case MUX_STREAM_TYPE_LIVE:
		VTRACE();
		//	av_strlcpy(sfilename, stream->filename, sizeof(sfilename) - 10);
			snprintf(sfilename, sizeof(sfilename)-10, "%s://%s:%d/%s", IS_STREAM_FORMAT(stream->fmt,  "rtp" )?"rtsp":"http", inet_ntoa(stream->muxSvr->myAddr.sin_addr),
				IS_STREAM_FORMAT(stream->fmt,  "rtp" )?stream->muxSvr->svrConfig.rtspAddress.port:stream->muxSvr->svrConfig.httpAddress.port, stream->filename);
		VTRACE();
			eosf = sfilename + strlen(sfilename);
			
			if (eosf - sfilename >= 4)
			{
				if (strcmp(eosf - 4, ".asf") == 0)
				{
					strcpy(eosf - 4, ".asx");
				}
				else if (strcmp(eosf - 3, ".rm") == 0)
				{
					strcpy(eosf - 3, ".ram");
				}
				else if ( IS_STREAM_FORMAT(stream->fmt,  "rtp") )
				{
					/* generate a sample RTSP redirector if unicast. Generate an SDP redirector if multicast */
					eosf = strrchr(sfilename, '.');
					if (!eosf)
						eosf = sfilename + strlen(sfilename);
					if (stream->is_multicast)
						strcpy(eosf, ".sdp");
					else
						strcpy(eosf, ".rtsp");
				}
			}
			
			if(IS_STREAM_FORMAT(stream->fmt,  "rtp" ) )
				snprintf(format, sizeof(format), "RTSP");
			else if(IS_STREAM_FORMAT(stream->fmt,  "mpegts" ) )
				snprintf(format, sizeof(format), "TS");
			else if(IS_STREAM_FORMAT(stream->fmt,  "flv" ) )
				snprintf(format, sizeof(format), "FLV");
			else if(IS_STREAM_FORMAT(stream->fmt,  "matroska" ) )
				snprintf(format, sizeof(format), "MKV");
			else if(IS_STREAM_FORMAT(stream->fmt,  "avi" ) )
				snprintf(format, sizeof(format), "AVI");
			else
				snprintf(format, sizeof(format), "Unknown");
			
			break;

		case MUX_STREAM_TYPE_UMC:
			snprintf(sfilename, sizeof(sfilename), "udp://%s:%d", stream->multicastAddress.address,stream->multicastAddress.port);
			snprintf(format, sizeof(format), "UDP Multicast");
			break;

		case MUX_STREAM_TYPE_RTMP:
			snprintf(sfilename, sizeof(sfilename), "%s", stream->rtmpUrl);
			snprintf(format, sizeof(format), "RTMP");
			break;

		case MUX_STREAM_TYPE_HLS:
			snprintf(sfilename, sizeof(sfilename), "http://%s:%d/%s", inet_ntoa(stream->muxSvr->myAddr.sin_addr), svrWebGetPort(), stream->filename);
			snprintf(format, sizeof(format), "HLS");
			break;

		case MUX_STREAM_TYPE_STATUS:
			snprintf(sfilename, sizeof(sfilename), "http://%s:%d/%s", inet_ntoa(stream->muxSvr->myAddr.sin_addr), stream->muxSvr->svrConfig.httpAddress.port, stream->filename);
			snprintf(format, sizeof(format), "HTML");
			break;

		default:
			snprintf(sfilename, sizeof(sfilename), "%s", "Unknown");
			snprintf(format, sizeof(format), "Unknown");
			break;
	}
	

	muxSvrConnectPrintf(svrConn, "<TR><TD><A HREF=\"%s\">%s</A> <TD align=center> %s ", 
		sfilename, sfilename,//stream->filename); /* URL */
		format );

	return EXIT_SUCCESS;
}

static int  _streamStatus(int index, void *ele, void *priv)
{
	MuxStream *stream = (MuxStream *)ele;
	SERVER_CONNECT *svrConn = (SERVER_CONNECT *)priv;

	_streamInfo( svrConn, stream);
	muxSvrConnectPrintf(svrConn, "<td align=right> %d <td align=right> ", stream->conns_served); /* Served Conns */
	
VTRACE();
	fmt_bytecount(svrConn->dynaBuffer, stream->bytes_served); /* bytes */

	if(stream->feed == NULL)
	{
		muxSvrConnectPrintf(svrConn, "<TD align=right> - <TD> -\n");
	}
	else
	{
		muxSvrConnectPrintf(svrConn, "<TD align=right> %d <TD>%s\n", stream->bandwidth, (stream->feed)?stream->feed->filename: stream->filename );
	}
	
VTRACE();

	return EXIT_SUCCESS;
}


static int _printFeedInfo(MuxFeed  *feed, SERVER_CONNECT *svrConn)
{
	int i;
	char parameters[128];

	muxSvrConnectPrintf(svrConn, "<tr><th colspan=2 "BG_COLOR">%s<th  "BG_COLOR">%d Kbps<th colspan=2  "BG_COLOR"> %s \n", feed->filename, feed->bandwidth, feed->feed_filename);
	muxSvrConnectPrintf(svrConn, "<tr><th "BG_COLOR">Stream<th "BG_COLOR">Type<th "BG_COLOR">Codec<th "BG_COLOR">kbit/s<th "BG_COLOR">Parameters\n");

	for (i = 0; i < feed->nbInStreams; i++)		
	{
		MuxMediaDescriber *desc = feed->feedStreams[i];
		AVCodecParameters *codecPar = (AVCodecParameters *)desc->codecPar;
		
		parameters[0] = 0;

		switch(codecPar->codec_type)
		{
			case AVMEDIA_TYPE_AUDIO:
				snprintf(parameters, sizeof(parameters), "%d channel(s), %d Hz", codecPar->channels, codecPar->sample_rate);
				break;
			
			case AVMEDIA_TYPE_VIDEO:
				snprintf(parameters, sizeof(parameters), "%dx%d", codecPar->width, codecPar->height );
				snprintf(parameters+strlen(parameters), sizeof(parameters)-strlen(parameters), ", fps=%3.2f", desc->fps );
				break;

			case AVMEDIA_TYPE_SUBTITLE:
				break;
				
			default:
				abort();
		}

		snprintf(parameters+strlen(parameters), sizeof(parameters)-strlen(parameters), ", BPS=%d",  desc->bandwidth );
		muxSvrConnectPrintf(svrConn, "<tr><td>%d<td>%s<td>%s<td>%"PRId64 "<td>%s\n", i, desc->type, desc->codec, codecPar->bit_rate/1000, parameters);
	}
VTRACE();

	return EXIT_SUCCESS;
}


static int _printFileFeedInfo(MuxFeed  *feed, SERVER_CONNECT *svrConn)
{
	int i;
	char parameters[128];
	AVStream *st;

	muxSvrConnectPrintf(svrConn, "<tr><th colspan=2 "BG_COLOR">%s<th  "BG_COLOR">%" PRId64 " Kbps<th colspan=2  "BG_COLOR"> %s \n", feed->filename, MUX_KILO(feed->inputCtx->bit_rate), feed->feed_filename);
	muxSvrConnectPrintf(svrConn, "<tr><th "BG_COLOR">Stream<th "BG_COLOR">Type<th "BG_COLOR">Codec<th "BG_COLOR">kbit/s<th "BG_COLOR">Parameters\n");

VTRACE();
	for (i = 0; i < feed->inputCtx->nb_streams; i++)
	{
		AVDictionary *metaData;
		AVDictionaryEntry *tag = NULL;
		
		st = feed->inputCtx->streams[i];
VTRACE();
		metaData = st->metadata;

		parameters[0] = 0;

		switch(st->codecpar->codec_type)
		{
			case AVMEDIA_TYPE_AUDIO:
				snprintf(parameters, sizeof(parameters), "%d channel(s), %d Hz", st->codecpar->channels, st->codecpar->sample_rate);
				tag = av_dict_get(metaData, "BPS", NULL, AV_DICT_IGNORE_SUFFIX);
				if(tag )
				{
					snprintf(parameters+strlen(parameters), sizeof(parameters)-strlen(parameters), ", BPS=%s", (tag==NULL)?"...":tag->value);
				}
				break;
			
			case AVMEDIA_TYPE_VIDEO:
#if MUX_WITH_CODEC_FIELD	
				snprintf(parameters, sizeof(parameters), "%dx%d, q=%d-%d, fps=%d", st->codecpar->width,
					st->codecpar->height, st->codec->qmin, st->codec->qmax, st->time_base.den / st->time_base.num);
#else
VTRACE();
				snprintf(parameters, sizeof(parameters), "%dx%d", st->codecpar->width, st->codecpar->height );

//				AVRational *framerate = &st->avg_frame_rate;
				AVRational *framerate = &st->r_frame_rate;
				int fps = framerate->den && framerate->num;
				if(fps && st->codecpar->codec_id != AV_CODEC_ID_MJPEG )
				{
					snprintf(parameters+strlen(parameters), sizeof(parameters)-strlen(parameters), ", fps=%3.2f", (double)framerate->num/(double)framerate->den );
				}

				tag = av_dict_get(metaData, "BPS", NULL, AV_DICT_IGNORE_SUFFIX);
				if(tag )
				{
					snprintf(parameters+strlen(parameters), sizeof(parameters)-strlen(parameters), ", BPS=%s", (tag==NULL)?"...":tag->value);
				}
				else
				{
					tag = av_dict_get(metaData, "filename", NULL, AV_DICT_IGNORE_SUFFIX);
					snprintf(parameters+strlen(parameters), sizeof(parameters)-strlen(parameters), ", %s", (tag==NULL)?"...":tag->value);
				}
#endif
				break;

			case AVMEDIA_TYPE_DATA:
				MUX_SVR_DEBUG("Stream Data");
//				snprintf(parameters, sizeof(parameters), "%d channel(s), %d Hz", st->codecpar->channels, st->codecpar->sample_rate);
				break;
				
			case AVMEDIA_TYPE_SUBTITLE:
				tag = av_dict_get(metaData, "NUMBER_OF_FRAMES", NULL, AV_DICT_IGNORE_SUFFIX);
				if(tag )
				{
					snprintf(parameters+strlen(parameters), sizeof(parameters)-strlen(parameters), "frames=%s, ", (tag==NULL)?"...":tag->value);
				}
				tag = av_dict_get(metaData, "NUMBER_OF_BYTES", NULL, AV_DICT_IGNORE_SUFFIX);
				if(tag )
				{
					snprintf(parameters+strlen(parameters), sizeof(parameters)-strlen(parameters), "bytes=%s", (tag==NULL)?"...":tag->value);
				}

				break;
				
			case AVMEDIA_TYPE_ATTACHMENT:
				tag = av_dict_get(metaData, "filename", NULL, AV_DICT_IGNORE_SUFFIX);
				if(tag )
				{
					snprintf(parameters+strlen(parameters), sizeof(parameters)-strlen(parameters), "filename=%s, ", (tag==NULL)?"...":tag->value);
				}
				tag = av_dict_get(metaData, "mimetype", NULL, AV_DICT_IGNORE_SUFFIX);
				if(tag)
					snprintf(parameters+strlen(parameters), sizeof(parameters)-strlen(parameters), "mimetype=%s", (tag==NULL)?"...":tag->value);

				break;
				
			default:
				abort();
		}

		muxSvrConnectPrintf(svrConn, "<tr><td>%d<td>%s<td>%s<td>%"PRId64 "<td>%s\n",
			i, av_get_media_type_string(st->codecpar->codec_type), avcodec_get_name(st->codecpar->codec_id), (int64_t)st->codecpar->bit_rate/1000, parameters);
	}
VTRACE();

	return EXIT_SUCCESS;
}


static char *_svrConnTypeName(conn_type_t	type)
{
	switch (type)
	{
		case CONN_TYPE_HTTP:
			return "HTTP";
			break;
		case CONN_TYPE_RTSP:
			return "RTSP";
			break;
		case CONN_TYPE_MULTICAST:
			return "UDP Multicast";
			break;
		case CONN_TYPE_RTMP:
			return "RTMP";
			break;
		case CONN_TYPE_HLS:
			return "HLS";
			break;
			
		case CONN_TYPE_RTP_UDP:
			return "RTP/UDP";
			break;
		case CONN_TYPE_RTP_TCP:
			return "RTP/TCP";
			break;
		case CONN_TYPE_RTP_MULTICAST:
			return "RTP Multicast";
			break;
		default:	
			return "Unknown";
			break;
	}

	return "Unknown";
}

static char *_svrConnStatusName(int	status)
{
	switch (status)
	{
		case MUX_SVR_STATE_CLOSING:
			return "CLOSING";
			break;
		case MUX_SVR_STATE_CONTINUE:
			return "CONTINUE";
			break;
		case HTTP_STATE_INIT:
			return "INIT";
			break;
		case HTTP_STATE_DATA:
			return "DATA";
			break;
		case HTTP_STATE_ERROR:
			return "ERROR";
			break;
			
		case RTSP_STATE_INIT:
			return "RTSP_INIT";
			break;
		case RTSP_STATE_READY:
			return "RTSP_READY";
			break;
		case RTSP_STATE_PLAY:
			return "RTSP_PLAY";
			break;
		case RTSP_STATE_RECORD:
			return "RTSP_RECORD";
			break;
		case RTSP_STATE_ERROR:
			return "RTSP_ERROR";
			break;
		default:	
			return "Unknown";
			break;
	}

	return "Unknown";
}



/* output status of one servicing connection */
static  int _serviceConStatus(int index, void *ele, void *priv)
{
	SERVER_CONNECT *svrConn = (SERVER_CONNECT *)ele;
	SERVER_CONNECT *httpConn = (SERVER_CONNECT *)priv;
	char ip[128];
	char url[1024];
	
VTRACE();
#if 0
	if(svrConn== NULL || svrConn->stream == NULL)
	{
		MUX_SVR_LOG(ERROR_TEXT_BEGIN"%s has no stream "ERROR_TEXT_END, svrConn->name );
		abort();
	}
#endif

	if(svrConn->type == CONN_TYPE_HTTP || svrConn->type == CONN_TYPE_RTSP )
	{
VTRACE();
		if(IS_SERVICE_CONNECT(svrConn) )
		{/* RTSP and all HTTP conns */
			snprintf(url, sizeof(url), "%s/%s", svrConn->name, svrConn->stream->filename);
			snprintf(ip, sizeof(ip), "%s", inet_ntoa( svrConn->peerAddress.sin_addr) );
		}
		else
		{/* monitor connection of HTTP/RTSP */
			snprintf(url, sizeof(url), "%s", svrConn->name );
			snprintf(ip, sizeof(ip), "%s:%d", inet_ntoa(httpConn->stream->muxSvr->myAddr.sin_addr), 
				(svrConn->type == CONN_TYPE_RTSP )?httpConn->stream->muxSvr->svrConfig.rtspAddress.port:httpConn->stream->muxSvr->svrConfig.httpAddress.port );
			//return EXIT_SUCCESS;
		}
	}
	else
	{
		snprintf(url, sizeof(url), "%s", svrConn->name);
		if(svrConn->stream->type == MUX_STREAM_TYPE_UMC )
		{
			snprintf(ip, sizeof(ip), "udp://%s:%d", svrConn->stream->multicastAddress.address, svrConn->stream->multicastAddress.port);
		}
		else if(svrConn->stream->type == MUX_STREAM_TYPE_RTMP )
		{
			snprintf(ip, sizeof(ip), "%s", svrConn->stream->rtmpUrl);
		}
		else if(svrConn->stream->type == MUX_STREAM_TYPE_HLS )
		{
			snprintf(ip, sizeof(ip), "http://%s:%d/%s", inet_ntoa(svrConn->stream->muxSvr->myAddr.sin_addr), svrWebGetPort(), svrConn->stream->filename);
		}
		else
			ip[0] = 0;

	}
	
VTRACE();
	muxSvrConnectPrintf(httpConn, "<TR><TD><B>%d</B><TD>%s<TD>%s<TD>%s<TD>%s<td align=right>", 
			index, url, ip, _svrConnTypeName(svrConn->type), _svrConnStatusName(svrConn->state) ); 
		
//		fmt_bytecount(_svrConn->dynaBuffer,  bitrate); // Target bits/sec
	muxSvrConnectPrintf(httpConn, "<td align=right>");
		//fmt_bytecount(svrConn->dynaBuffer, compute_datarate(&svrConn->datarate, serviceConn->dataCount, server) * 8);
//		fmt_bytecount(_svrConn->dynaBuffer, 8); // Actual bits/sec
	muxSvrConnectPrintf(httpConn,  "<td align=right>");
//		fmt_bytecount(_svrConn->dynaBuffer, _svrConn->dataCount); // Bytes transferred
	muxSvrConnectPrintf(httpConn,  "\n");

VTRACE();
	return EXIT_SUCCESS;
}


int httpEventStatusPage(SERVER_CONNECT *svrConn, MUX_SVR_EVENT *event )
{
	MUX_SVR		*muxSvr = svrConn->muxSvr;
	MuxFeed 		*feed = muxSvr->feeds;
	char *p;
	time_t ti;

	muxSvrGetHostIP(svrConn);
	muxSvrConnectPrintf(svrConn, "HTTP/1.0 200 OK\r\nContent-type: text/html\r\nPragma: no-cache\r\n\r\n" );

	muxSvrConnectPrintf(svrConn, "<HEAD><TITLE>MuxLab Service Status</TITLE>\n");
	muxSvrConnectPrintf(svrConn, "</HEAD>\n<BODY><H1>%s</H1>\n", muxSvr->programName);


	/******** table : Connections Status */
	muxSvrConnectPrintf(svrConn,  "<H2 align=center>Status of Servicing Connections</H2>\nNumber of connections: %d / %d<BR>\n", 
		muxSvr->nbConnections,  muxSvr->svrConfig.maxConnections);
	muxSvrConnectPrintf(svrConn, "Bandwidth in use: %" PRId64 "kB / %" PRId64 "k<BR>\n", 
		muxSvr->currentBandwidth, muxSvr->svrConfig.maxBandwidth);
	muxSvrConnectPrintf(svrConn, "<TABLE border=1 cellspacing=2 cellpadding=2>\n<TR><th "BG_COLOR">#<th "BG_COLOR">Name<th "BG_COLOR">IP<th "BG_COLOR">Proto<th "BG_COLOR">"
		"State<th "BG_COLOR">Target bits/sec<th "BG_COLOR">Actual bits/sec<th "BG_COLOR">Bytes transferred\n"); 
	cmn_list_iterate(&muxSvr->serverConns, TRUE, _serviceConStatus, svrConn);
	muxSvrConnectPrintf(svrConn, "</TABLE>\n");


VTRACE();

	/******** table : Available Streams */
	/* format status */
	muxSvrConnectPrintf(svrConn, "<H2  align=center>Available Media Streams</H2>\n<TABLE border=1 cellspacing=2 cellpadding=2>\n");
	muxSvrConnectPrintf(svrConn, "<TR><Th "BG_COLOR">URL<Th "BG_COLOR">Format<th align=center "BG_COLOR">Served Conns<Th "BG_COLOR">"
		"Bytes<Th "BG_COLOR">BitRate(kbps)<Th "BG_COLOR">Media\n");
	cmn_list_iterate(&muxSvr->svrConfig.streams, TRUE, _streamStatus, svrConn);
	muxSvrConnectPrintf(svrConn, "</TABLE>\n");


VTRACE();
	/******** table : Feed Media */
	muxSvrConnectPrintf(svrConn, "<H2 align=center>Feed Information</H2>\n<table border=1 cellspacing=1 cellpadding=1>\n");
	while(feed)
	{
		_printFeedInfo(feed, svrConn);
		feed = feed->next;
	}
	muxSvrConnectPrintf(svrConn, "</table>\n");
	
VTRACE();
	/******** table : Media File */
	muxSvrConnectPrintf(svrConn, "<H2 align=center>Media File Information</H2>\n<table border=1 cellspacing=1 cellpadding=1>\n");
	feed = muxSvr->feeds;
	while(feed)
	{
		if(feed->type == MUX_FEED_TYPE_FILE)
		{
			_printFileFeedInfo(feed, svrConn);
		}
		feed = feed->next;
	}
	muxSvrConnectPrintf(svrConn, "</table>\n");
	

	/*******  date */
	ti = time(NULL);
	p = ctime(&ti);
	muxSvrConnectPrintf(svrConn, "<HR size=1 noshade>Generated at %s</BODY>\n</HTML>\n", p);

	muxSvrConnectFlushOut( svrConn);

	/* after service, this HTTP connection will be closed at once */
#if 0	
	return MUX_SVR_STATE_CONTINUE;
#else
	return MUX_SVR_STATE_CLOSING;
#endif
}
#endif

