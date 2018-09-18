
/**
 * multiple format streaming server based on the FFmpeg libraries
 */

#include "muxSvr.h"

#include "libMedia.h"
#include "libMux.h"


#include "__services.h"

AVFrame				dummy_frame;

static MUX_SVR _muxSvr;


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
				MUX_SVR_ERROR( "Configuration error: stream %s without feed", stream->filename);
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

static void _muxSvrDestory(MuxPlugIn *plugin)
{
	MUX_SVR *muxSvr = (MUX_SVR *)plugin->priv;

	muxSvrServiceConnectsDestroy(muxSvr);
	/*
	remove resource allocated for Feed streams, feeds, etc. Then free Feed and Strean themselves
	*/
	muxSvrConfigFree(&muxSvr->svrConfig);
	
	avformat_network_deinit();

	cmn_mutex_destroy(muxSvr->connsMutex);
}

static int _muxSvrReportEvent(struct _MuxPlugin *plugin, CMN_PLAY_JSON_EVENT *jEvent)
{
	return cmnThreadAddEvent(&threadServer, jEvent);
}

int init( MuxPlugIn *plugin)
{
#if 0
	struct sigaction sigact = { { 0 } };
#endif
	int cfg_parsed;
	int ret = EXIT_FAILURE;
	int hasFileFeed = FALSE;
	MUX_SVR *muxSvr = &_muxSvr;
	memset(muxSvr, 0, sizeof(MUX_SVR));

	plugin->priv = muxSvr;
	plugin->type = MUX_PLUGIN_TYPE_SERVER;
	plugin->signalExit = NULL;//_muxPlayerSignalExit;
	plugin->reportEvent = _muxSvrReportEvent;
	plugin->destory = _muxSvrDestory;
	snprintf(plugin->name, sizeof(plugin->name), "%s", CMN_MODULE_SERVER_NAME );
	snprintf(plugin->version, sizeof(plugin->version), "%s", CMN_VERSION_INFO(CMN_MODULE_SERVER_NAME) );
	muxSvr->priv = plugin->muxMain;

	muxSvr->svrConfig.maxHttpConnections = HTTP_MAX_CONNECTIONS;
	muxSvr->svrConfig.maxConnections = 5;
	muxSvr->svrConfig.maxBandwidth = 1000;
	muxSvr->svrConfig.use_defaults = 1;
	snprintf(muxSvr->svrConfig.filename, sizeof(muxSvr->svrConfig.filename), "%s", MUX_SERVER_CONFIG_FILE);
	muxSvr->svrConfig.muxSvr = muxSvr;

	muxSvr->connsMutex = cmn_mutex_init();
#if 0//def	PLUGIN_SUPPORT
#else
	/* must restart in this plugin */
//	av_register_all();
//	avformat_network_init();

	MUX_INIT_FFMPEG(plugin->muxMain->muxLog.llevel);

#endif

	snprintf(muxSvr->programName, 128, "%s", MUX_SERVER_NAME );

	av_lfg_init(&muxSvr->random_state, av_get_random_seed());

#if 0
	sigact.sa_handler = handle_child_exit;
	sigact.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigaction(SIGCHLD, &sigact, 0);
#endif

	if ((cfg_parsed = muxSvrConfigParse(muxSvr->svrConfig.filename, &muxSvr->svrConfig)) < 0)
	{
		fprintf(stderr, "Error reading configuration file '%s': %s\n", muxSvr->svrConfig.filename, av_err2str(cfg_parsed));
		_muxSvrDestory(plugin);
		return EXIT_FAILURE;
	}

	muxSvrInitLog( muxSvr);
	
	if(cmn_list_iterate(&muxSvr->svrConfig.feeds, FALSE, muxSvrFeedInit, muxSvr) != EXIT_SUCCESS)
	{
		fprintf(stderr, "Feed initialization failed\n" );
		_muxSvrDestory(plugin);
		return EXIT_FAILURE;
	}

VTRACE();
	if(muxSvr->feeds == NULL)
	{
		fprintf(stderr, "No Feed is available\n" );
		_muxSvrDestory(plugin);
		return EXIT_FAILURE;
	}

	hasFileFeed = _validateService(muxSvr);


	/* feed thread must run first, so if feed error, startup will fail */
	if(hasFileFeed)
	{
		fprintf(stderr, "Start File Feed.....\n" );
#if 0	
#ifdef	PLUGIN_SUPPORT
		plugin->muxMain->initThread(plugin->muxMain,&threadFileFeed, muxSvr);
#else
		cmnThreadInit( &threadFileFeed, muxSvr );
#endif
#endif
	}

	/* CtrlThread must be started before MntrThread to receive INIT events and start monitoring */
#ifdef	PLUGIN_SUPPORT
	ret = plugin->muxMain->initThread(plugin->muxMain, &threadServer, muxSvr);
#else
	cmnThreadInit( &threadServer, muxSvr );
#endif

	/* check all feeds and their streams are validate: send event to CtrlThread?? */
	ret = muxSvrServiceConnectsInit(muxSvr);
	if(ret != EXIT_SUCCESS)
	{
		fprintf(stderr, "Service connection initialization failed\n" );
		_muxSvrDestory(plugin);
		return ret;
	}
	

#ifdef	PLUGIN_SUPPORT
#else
	if(hasFileFeed)
	{
		cmnThreadJoin( &threadFileFeed);
	}

	cmnThreadJoin( &threadServer);
#endif

	return EXIT_SUCCESS;
}

