
/**
 * multiple format streaming server based on the FFmpeg libraries
 */

#include "muxSvr.h"


/* compute the bandwidth used by each stream */
static void _computeBandwidth(MuxServerConfig *cfg)
{
	unsigned bandwidth;
	int i;
	MuxStream *stream;

	for(stream = cfg->first_stream; stream; stream = stream->next)
	{
		bandwidth = 0;
		for(i=0;i<stream->nb_streams;i++)
		{
			LayeredAVStream *st = stream->streams[i];
			switch(st->codec->codec_type)
			{
				case AVMEDIA_TYPE_AUDIO:
				case AVMEDIA_TYPE_VIDEO:
					bandwidth += st->codec->bit_rate;
					break;
				
				default:
					break;
			}
		}
		
		stream->bandwidth = (bandwidth + 999) / 1000;

		MUX_SVR_LOG("Stream '%s' bandwidth: %d\n", stream->filename, stream->bandwidth);

	}
}



int main(int argc, char **argv)
{
#if 0
	struct sigaction sigact = { { 0 } };
#endif
	int cfg_parsed;
	int ret = EXIT_FAILURE;
	MUX_SVR _muxSvr;
	MUX_SVR *muxSvr = &_muxSvr;
	memset(muxSvr, 0, sizeof(MUX_SVR));

	muxSvr->config.nb_max_http_connections = 2000,
	muxSvr->config.nb_max_connections = 5,
	muxSvr->config.max_bandwidth = 1000,
	muxSvr->config.use_defaults = 1,
	muxSvr->config.filename = av_strdup(MUX_SERVER_CONFIG_FILE);
	muxSvr->config.muxSvr = muxSvr;

//	muxSvr->config.debug = 1,

///	av_log_set_level(AV_LOG_DEBUG);
//	av_log_set_level(AV_LOG_WARNING);
	av_log_set_level(AV_LOG_TRACE);
	
	av_register_all();
	avformat_network_init();

	snprintf(muxSvr->programName, 128, "%s", MUX_SERVER_NAME );

	unsetenv("http_proxy");             /* Kill the http_proxy */

	av_lfg_init(&muxSvr->random_state, av_get_random_seed());

#if 0
	sigact.sa_handler = handle_child_exit;
	sigact.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction(SIGCHLD, &sigact, 0);
#endif

	if ((cfg_parsed = muxSvrParseConfig(muxSvr->config.filename, &muxSvr->config)) < 0)
	{
		fprintf(stderr, "Error reading configuration file '%s': %s\n", muxSvr->config.filename, av_err2str(cfg_parsed));
		goto bail;
	}
VTRACE();
	muxSvrInitLog( muxSvr);

VTRACE();
	muxSvrBuildFileStreams(muxSvr);

VTRACE();
	if (muxSvrBuildFeedStreams(muxSvr) < 0)
	{
		MUX_SVR_LOG("Could not setup feed streams\n");
		goto bail;
	}

	_computeBandwidth(&muxSvr->config);

	/* signal init */
	signal(SIGPIPE, SIG_IGN);

	if (muxSvrServer(muxSvr) < 0)
	{
		MUX_SVR_LOG("Could not start server\n");
		goto bail;
	}

	ret=EXIT_SUCCESS;

bail:
	av_freep (&muxSvr->config.filename);
	avformat_network_deinit();
	
	return ret;
}

