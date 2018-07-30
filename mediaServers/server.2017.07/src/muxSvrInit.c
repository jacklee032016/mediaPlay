
#include "muxSvr.h"

/********************************************************************/
/* ffserver initialization */

/* FIXME: This code should use avformat_new_stream() */
static LayeredAVStream *_add_av_stream1(MuxStream *stream, AVCodecContext *codec, int copy)
{
	LayeredAVStream *fst;

	if(stream->nb_streams >= MUX_ARRAY_ELEMS(stream->streams))
		return NULL;

	fst = av_mallocz(sizeof(*fst));
	if (!fst)
		return NULL;
	
	if (copy)
	{
		fst->codec = avcodec_alloc_context3(codec->codec);
		if (!fst->codec)
		{
			av_free(fst);
			return NULL;
		}
		avcodec_copy_context(fst->codec, codec);
	}
	else
		/* live streams must use the actual feed's codec since it may be
		* updated later to carry extradata needed by them.
		*/
		fst->codec = codec;

	//NOTE we previously allocated internal & internal->avctx, these seemed uneeded though
	fst->codecpar = avcodec_parameters_alloc();
	fst->index = stream->nb_streams;
	fst->time_base = codec->time_base;
	fst->pts_wrap_bits = 33;
	fst->sample_aspect_ratio = codec->sample_aspect_ratio;
	
	stream->streams[stream->nb_streams++] = fst;
	return fst;
}

static void _removeStream(MuxServerConfig *cfg, MuxStream *stream)
{
	MuxStream **ps;
	
	ps = &cfg->first_stream;
	while (*ps)
	{
		if (*ps == stream)
			*ps = (*ps)->next;
		else
			ps = &(*ps)->next;
	}
}

/* compute the needed AVStream for each file */
void muxSvrBuildFileStreams(MUX_SVR *muxSvr)
{
	MuxStream *stream;
	AVFormatContext *infile;
	int i, ret;

	/* gather all streams */
	for(stream = muxSvr->config.first_stream; stream; stream = stream->next)
	{
		infile = NULL;

		if (stream->stream_type != STREAM_TYPE_LIVE || stream->feed)
			continue;

		/* the stream comes from a file */
		/* try to open the file */
		/* open stream */

		/* specific case: if transport stream output to RTP,
		* we use a raw transport stream reader */
		if (stream->fmt && !strcmp(stream->fmt->name, "rtp"))
			av_dict_set(&stream->in_opts, "mpeg2ts_compute_pcr", "1", 0);

		if (!stream->feed_filename[0])
		{
			MUX_SVR_LOG("Unspecified feed file for stream '%s'\n", stream->filename);
			goto fail;
		}

		MUX_SVR_LOG("Opening feed file '%s' for stream '%s'\n", stream->feed_filename, stream->filename);

		ret = avformat_open_input(&infile, stream->feed_filename, stream->ifmt, &stream->in_opts);
		if (ret < 0)
		{
			MUX_SVR_LOG("Could not open '%s': %s\n", stream->feed_filename, av_err2str(ret));
			/* remove stream (no need to spend more time on it) */
fail:
			_removeStream(&muxSvr->config, stream);
		}
		else
		{
			/* find all the AVStreams inside and reference them in 'stream' */
			if (avformat_find_stream_info(infile, NULL) < 0)
			{
				MUX_SVR_LOG("Could not find codec parameters from '%s'\n", stream->feed_filename);
				avformat_close_input(&infile);
				goto fail;
			}

			for(i=0;i<infile->nb_streams;i++)
				_add_av_stream1(stream, infile->streams[i]->codec, 1);

			avformat_close_input(&infile);
		}
	}
}

static inline int _check_codec_match(LayeredAVStream *ccf, AVStream *ccs, int stream)
{
	int matches = 1;

	/* FIXME: Missed check on AVCodecContext.flags */
#define CHECK_CODEC(x)  (ccf->codecpar->x != ccs->codecpar->x)
	if (CHECK_CODEC(codec_id) || CHECK_CODEC(codec_type))
	{
		MUX_SVR_LOG("Codecs do not match for stream %d\n", stream);
		matches = 0;
	}
	else if (CHECK_CODEC(bit_rate))
	{
		MUX_SVR_LOG("Codec bitrates do not match for stream %d\n", stream);
		matches = 0;
	}
	else if (ccf->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		if (av_cmp_q(ccf->time_base, ccs->time_base) || CHECK_CODEC(width) || CHECK_CODEC(height))
		{
			MUX_SVR_LOG("Codec width, height or framerate do not match for stream %d\n", stream);
			matches = 0;
		}
	}
	else if (ccf->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		if (CHECK_CODEC(sample_rate) ||CHECK_CODEC(channels) ||CHECK_CODEC(frame_size))
		{
			MUX_SVR_LOG("Codec sample_rate, channels, frame_size do not match for stream %d\n", stream);
			matches = 0;
		}
	}
	else
	{
		MUX_SVR_LOG("Unknown codec type for stream %d\n", stream);
		matches = 0;
	}

	return matches;
}


/* return the stream number in the feed */
static int _add_av_stream(MuxStream *feed, LayeredAVStream *st)
{
	LayeredAVStream *fst;
	AVCodecContext *av, *av1;
	int i;

	av = st->codec;
	for(i=0;i<feed->nb_streams;i++)
	{
		av1 = feed->streams[i]->codec;
		if (av1->codec_id == av->codec_id && av1->codec_type == av->codec_type && av1->bit_rate == av->bit_rate)
		{

			switch(av->codec_type)
			{
				case AVMEDIA_TYPE_AUDIO:
					if (av1->channels == av->channels &&
						av1->sample_rate == av->sample_rate)
						return i;
				break;
				
				case AVMEDIA_TYPE_VIDEO:
					if (av1->width == av->width &&
						av1->height == av->height &&
						av1->time_base.den == av->time_base.den &&
						av1->time_base.num == av->time_base.num &&
						av1->gop_size == av->gop_size)
						return i;
					break;
				
				default:
					abort();
			}
		}
	}

	fst = _add_av_stream1(feed, av, 0);
	if (!fst)
		return -1;
	if (st->recommended_encoder_configuration)
		fst->recommended_encoder_configuration = av_strdup(st->recommended_encoder_configuration);

	return feed->nb_streams - 1;
}

/* compute the needed AVStream for each feed */
int muxSvrBuildFeedStreams(MUX_SVR *muxSvr)
{
	MuxStream *stream, *feed;
	int i, fd;

	/* gather all streams */
	for(stream = muxSvr->config.first_stream; stream; stream = stream->next)
	{
		feed = stream->feed;
		if (!feed)
			continue;

		if (stream->is_feed)
		{
			for(i=0;i<stream->nb_streams;i++)
				stream->feed_streams[i] = i;
			continue;
		}
		
		/* we handle a stream coming from a feed */
		for(i=0;i<stream->nb_streams;i++)
			stream->feed_streams[i] = _add_av_stream(feed, stream->streams[i]);
	}

	/* create feed files if needed */
	for(feed = muxSvr->config.first_feed; feed; feed = feed->next_feed)
	{

		if (avio_check(feed->feed_filename, AVIO_FLAG_READ) > 0)
		{
			AVFormatContext *s = NULL;
			int matches = 0;

			/* See if it matches */
			if (avformat_open_input(&s, feed->feed_filename, NULL, NULL) < 0)
			{
				MUX_SVR_LOG("Deleting feed file '%s' as it appears to be corrupt\n", feed->feed_filename);
				goto drop;
			}

			/* set buffer size */
			if (ffio_set_buf_size(s->pb, MUX_PACKET_SIZE) < 0)
			{
				MUX_SVR_LOG("Failed to set buffer size\n");
				avformat_close_input(&s);
				goto bail;
			}

			/* Now see if it matches */
			if (s->nb_streams != feed->nb_streams)
			{
				MUX_SVR_LOG("Deleting feed file '%s' as stream counts differ (%d != %d)\n", feed->feed_filename, s->nb_streams, feed->nb_streams);
				goto drop;
			}

			matches = 1;
			for(i=0;i<s->nb_streams;i++)
			{
				AVStream *ss;
				LayeredAVStream *sf;

				sf = feed->streams[i];
				ss = s->streams[i];

				if (sf->index != ss->index || sf->id != ss->id)
				{
					MUX_SVR_LOG("Index & Id do not match for stream %d (%s)\n", i, feed->feed_filename);
					matches = 0;
					break;
				}

				matches = _check_codec_match (sf, ss, i);
				if (!matches)
					break;
			}

drop:
			if (s)
				avformat_close_input(&s);

			if (!matches)
			{
				if (feed->readonly)
				{
					MUX_SVR_LOG("Unable to delete read-only feed file '%s'\n", feed->feed_filename);
					goto bail;
				}
				unlink(feed->feed_filename);
			}
		}

		if (avio_check(feed->feed_filename, AVIO_FLAG_WRITE) <= 0)
		{
			AVFormatContext *s = avformat_alloc_context();

			if (!s)
			{
				MUX_SVR_LOG("Failed to allocate context\n");
				goto bail;
			}

			if (feed->readonly)
			{
				MUX_SVR_LOG("Unable to create feed file '%s' as it is marked readonly\n", feed->feed_filename);
				avformat_free_context(s);
				goto bail;
			}

			/* only write the header of the ffm file */
			if (avio_open(&s->pb, feed->feed_filename, AVIO_FLAG_WRITE) < 0)
			{
				MUX_SVR_LOG("Could not open output feed file '%s'\n", feed->feed_filename);
				avformat_free_context(s);
				goto bail;
			}
			s->oformat = feed->fmt;

			for (i = 0; i<feed->nb_streams; i++)
			{
				AVStream *st = avformat_new_stream(s, NULL); // FIXME free this
				if (!st)
				{
					MUX_SVR_LOG("Failed to allocate stream\n");
					goto bail;
				}
				unlayer_stream(st, feed->streams[i]);
			}
			
			if (avformat_write_header(s, NULL) < 0)
			{
				MUX_SVR_LOG("Container doesn't support the required parameters\n");
				avio_closep(&s->pb);
				s->streams = NULL;
				s->nb_streams = 0;
				avformat_free_context(s);
				goto bail;
			}
			
			/* XXX: need better API */
			av_freep(&s->priv_data);
			avio_closep(&s->pb);
			s->streams = NULL;
			s->nb_streams = 0;
			avformat_free_context(s);
		}

		/* get feed size and write index */
		fd = open(feed->feed_filename, O_RDONLY);
		if (fd < 0)
		{
			MUX_SVR_LOG("Could not open output feed file '%s'\n", feed->feed_filename);
			goto bail;
		}

		feed->feed_write_index = FFMAX(ffm_read_write_index(fd), MUX_PACKET_SIZE);
		feed->feed_size = lseek(fd, 0, SEEK_END);
		/* ensure that we do not wrap before the end of file */
		if (feed->feed_max_size && feed->feed_max_size < feed->feed_size)
			feed->feed_max_size = feed->feed_size;

		close(fd);
	}
	return 0;

bail:
	return -1;
}


