
/**
 * multiple format streaming server based on the FFmpeg libraries
 */

#include "muxSvr.h"

volatile sig_atomic_t recvSigalTerminal = FALSE;

AVFrame				dummy_frame;


/*  */
static int _validateService(MUX_SVR *muxSvr)
{
	int j;
	MuxStream *stream;
	MuxFeed *feed = muxSvr->feeds;

	for(j=0; j< cmn_list_size(&muxSvr->svrConfig.streams); j++)	
	{
		stream = (MuxStream *)cmn_list_get(&muxSvr->svrConfig.streams, j);

		if(!stream->feed)
		{
			if( IS_STREAM_TYPE(stream, MUX_STREAM_TYPE_STATUS))
				continue;
			else
			{
				MUX_SVR_LOG("Configuration error: stream %s without feed", stream->filename);
				exit(1);
			}
		}
		
		stream->bandwidth = (unsigned )stream->feed->bandwidth;
	}

	while(feed)
	{
		if(feed->type == MUX_FEED_TYPE_FILE)
			return TRUE;
		feed = feed->next;
	}
	
	return FALSE;
}



int main(int argc, char **argv)
{
#if 0
	struct sigaction sigact = { { 0 } };
#endif
	int cfg_parsed;
	int ret = EXIT_FAILURE;
	int hasFileFeed = FALSE;
	MUX_SVR _muxSvr;
	MUX_SVR *muxSvr = &_muxSvr;
	memset(muxSvr, 0, sizeof(MUX_SVR));

	muxSvr->svrConfig.maxHttpConnections = HTTP_MAX_CONNECTIONS;
	muxSvr->svrConfig.maxConnections = 5;
	muxSvr->svrConfig.maxBandwidth = 1000;
	muxSvr->svrConfig.use_defaults = 1;
	snprintf(muxSvr->svrConfig.filename, sizeof(muxSvr->svrConfig.filename), "%s", MUX_SERVER_CONFIG_FILE);
	muxSvr->svrConfig.muxSvr = muxSvr;

	muxSvr->connsMutex = cmn_mutex_init();

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

	if ((cfg_parsed = muxSvrConfigParse(muxSvr->svrConfig.filename, &muxSvr->svrConfig)) < 0)
	{
		fprintf(stderr, "Error reading configuration file '%s': %s\n", muxSvr->svrConfig.filename, av_err2str(cfg_parsed));
		goto bail;
	}

	muxSvrInitLog( muxSvr);
	
	if(cmn_list_iterate(&muxSvr->svrConfig.feeds, FALSE, muxSvrFeedInit, muxSvr) != EXIT_SUCCESS)
	{
		fprintf(stderr, "Feed initialization failed\n" );
		goto bail;
	}

VTRACE();
	if(muxSvr->feeds == NULL)
	{
		fprintf(stderr, "No Feed is available\n" );
		goto bail;
	}

	hasFileFeed = _validateService(muxSvr);

VTRACE();
	/* signal init */
	signal(SIGPIPE, SIG_IGN);


	cmn_timer_init();
	
	/* feed thread must run first, so if feed error, startup will fail */
	if(hasFileFeed)
	{
		cmnThreadInfoInit( &threadFileFeed, muxSvr );
	}
	
	cmnThreadInfoInit( &webThread, muxSvr );

	/* CtrlThread must be started before MntrThread to receive INIT events and start monitoring */
	cmnThreadInfoInit( &threadController, muxSvr );
	
	/* check all feeds and their streams are validate: send event to CtrlThread?? */
	ret = muxSvrServiceConnectsInit(muxSvr);
	if(ret != EXIT_SUCCESS)
	{
		fprintf(stderr, "Service connection initialization failed\n" );
		goto bail;
	}
	

	if(hasFileFeed)
	{
		cmnThreadInfoJoin( &threadFileFeed);
	}

	cmnThreadInfoJoin( &threadController);


	cmnThreadInfoJoin( &webThread);

	ret=EXIT_SUCCESS;

bail:

	muxSvrServiceConnectsDestroy(muxSvr);
	/*
	remove resource allocated for Feed streams, feeds, etc. Then free Feed and Strean themselves
	*/
	muxSvrConfigFree(&muxSvr->svrConfig);
	
	avformat_network_deinit();

	cmn_mutex_destroy(muxSvr->connsMutex);

	return ret;
}

