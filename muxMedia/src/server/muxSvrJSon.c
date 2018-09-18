

#include "muxSvr.h"

static int _muxSvrJSonConfig(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jEvent)
{
	MUX_SVR *muxSvr = (MUX_SVR *)priv;
	cJSON *newObject = NULL;
	cJSON *array = NULL;

	array = cJSON_CreateArray();
	dataConn->resultObject = array;

	newObject = cJSON_CreateObject();
	cJSON_AddItemToArray(array, newObject);

	JEVENT_ADD_STRING(newObject, "HttpAddress", muxSvr->svrConfig.httpAddress.address);
	JEVENT_ADD_INTEGER(newObject, "HttpPort", muxSvr->svrConfig.httpAddress.port);
	
	JEVENT_ADD_STRING(newObject, "RtspAddress", muxSvr->svrConfig.rtspAddress.address);
	JEVENT_ADD_INTEGER(newObject, "RtspPort", muxSvr->svrConfig.rtspAddress.port);
	
	JEVENT_ADD_INTEGER(newObject, "MaxClients", muxSvr->svrConfig.maxConnections);
	JEVENT_ADD_INTEGER(newObject, "MaxHttpConns", muxSvr->svrConfig.maxHttpConnections);

	dataConn->errCode = IPCMD_ERR_NOERROR;
	
	return EXIT_SUCCESS;
}


static int _muxSvrJSonConnsInfo(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jEvent)
{
	MUX_SVR *muxSvr = (MUX_SVR *)priv;
	cJSON *newObject = NULL, *newObject2 = NULL;
	cJSON *array = NULL, *array2 = NULL;
	int i, count;

	count =  cmn_list_size(&muxSvr->serverConns);
	if( count == 0 )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "No service connection is available!");
		goto endl;
	}

	array = cJSON_CreateArray();
	dataConn->resultObject = array;

	newObject = cJSON_CreateObject();
	cJSON_AddItemToArray(array, newObject);
	JEVENT_ADD_INTEGER(newObject, "conns", muxSvr->nbConnections);
	JEVENT_ADD_INTEGER(newObject, "bandwidth(KB)", muxSvr->currentBandwidth);
	
	array2 = cJSON_CreateArray();
	JEVENT_ADD_ARRAY(newObject, "services", array2);

	for(i=0; i< count; i++)
	{
		char		name[256];
		newObject2 = cJSON_CreateObject();
		SERVER_CONNECT *svrConn = (SERVER_CONNECT *)cmn_list_get(&muxSvr->serverConns, i);

		cJSON_AddItemToArray(array2, newObject2);

		if(IS_SERVICE_CONNECT(svrConn) )
		{/* RTSP and all HTTP conns */
			snprintf(name, sizeof(name), "%s/%s", svrConn->name, svrConn->stream->filename);
		}
		else
		{
			snprintf(name, sizeof(name), "%s", svrConn->name );
		}


		JEVENT_ADD_STRING(newObject2, "name", name );
		JEVENT_ADD_STRING(newObject2, "from", inet_ntoa( svrConn->peerAddress.sin_addr) );
		JEVENT_ADD_STRING(newObject2, "protocol", muxSvrConnTypeName(svrConn->type) );

		
		JEVENT_ADD_STRING(newObject2, "status", muxSvrConnStatusName(svrConn->state) );
	}

	dataConn->errCode = IPCMD_ERR_NOERROR;
	
endl:	
	
	return EXIT_SUCCESS;
}


static int _muxSvrJSonUrlsInfo(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jEvent)
{
	MUX_SVR *muxSvr = (MUX_SVR *)priv;
	cJSON *newObject = NULL;
	cJSON *array = NULL;
	cmn_list_t *streams = &muxSvr->svrConfig.streams;
	int i, count;

	count = cmn_list_size(streams);
	if( count == 0 )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "No URL exists!");
		goto endl;
	}

	array = cJSON_CreateArray();
	dataConn->resultObject = array;

	for(i=0; i< count; i++)
	{
		char *url = NULL, *format = NULL;
		MuxStream *st = (MuxStream *)cmn_list_get(streams, i);

		newObject = cJSON_CreateObject();
		cJSON_AddItemToArray(array, newObject);

		url = muxSvrStreamUrl(st);
		format = muxSvrStreamFormat(st);
		JEVENT_ADD_STRING(newObject, "URL",  url);
		JEVENT_ADD_STRING(newObject, "format", format );
		cmn_free(url);
		cmn_free(format);
		
		JEVENT_ADD_STRING(newObject, "media", (st->feed == NULL)?st->filename: st->feed->filename );
		JEVENT_ADD_INTEGER(newObject, "bandwidth", st->bandwidth);

		JEVENT_ADD_INTEGER(newObject, "servicedConns", st->conns_served);
		JEVENT_ADD_INTEGER(newObject, "servicedBytes", st->bytes_served);
	}

	dataConn->errCode = IPCMD_ERR_NOERROR;
endl:	

	return EXIT_SUCCESS;
}


static int _muxSvrJSonFeedsInfo(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jEvent)
{
	MUX_SVR *muxSvr = (MUX_SVR *)priv;
	cJSON *newObject = NULL;
	cJSON *array = NULL;
	MuxFeed *feed = muxSvr->feeds;
	int i;
	
	if( feed == NULL )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_NOERROR, "No feed exist!");
		goto endl;
	}

	array = cJSON_CreateArray();
	dataConn->resultObject = array;

	while(feed)
	{
		cJSON *array2 = NULL;
		cJSON *newObject2 = NULL;
		char parameters[128];
	
		newObject = cJSON_CreateObject();
		cJSON_AddItemToArray(array, newObject);


		JEVENT_ADD_STRING(newObject, "name", feed->filename ); /* current recording file */
		JEVENT_ADD_STRING(newObject, "type", (feed->type==MUX_FEED_TYPE_FILE)?"File":"Capture" );
		JEVENT_ADD_STRING(newObject, "media", (feed->type==MUX_FEED_TYPE_FILE)?feed->feed_filename: feed->captureName);

		JEVENT_ADD_INTEGER(newObject, "duration(seond)", feed->durationSecond);
		JEVENT_ADD_INTEGER(newObject, "bandwidth(Kbps)", feed->bandwidth );

		array2 = cJSON_CreateArray();
		JEVENT_ADD_ARRAY(newObject, "streams", array2 );

		for (i = 0; i < feed->mediaCapture->nbStreams; i++)		
		{
			MuxMediaDescriber *desc = feed->mediaCapture->capturedMedias[i];

			AVCodecParameters *codecPar = (AVCodecParameters *)desc->codecPar;
			
			newObject2 = cJSON_CreateObject();
			cJSON_AddItemToArray(array2, newObject2);
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
					CMN_ABORT("Invalidate media type: '%s'", av_get_media_type_string(codecPar->codec_type) );
			}

			snprintf(parameters+strlen(parameters), sizeof(parameters)-strlen(parameters), ", BPS=%d",  desc->bandwidth );

			JEVENT_ADD_STRING(newObject2, "type", desc->type );
			JEVENT_ADD_STRING(newObject2, "codec", desc->codec);
			JEVENT_ADD_STRING(newObject2, "params", parameters );
		}


		feed = feed->next;
	}
	
	dataConn->errCode = IPCMD_ERR_NOERROR;
	
endl:	
	
	return EXIT_SUCCESS;
}


PluginJSonHandler muxSvrJsonHandlers[] =
{
	{
		.type	= CMD_TYPE_SVR_CONFIG,
		.name 	= IPCMD_NAME_SERVER_CONFIG,
		handler	: _muxSvrJSonConfig
	},
	{
		.type	= CMD_TYPE_SVR_FEEDS,
		.name 	= IPCMD_NAME_SERVER_FEEDS,
		handler	: _muxSvrJSonFeedsInfo
	},
	{
		.type	= CMD_TYPE_SVR_CONNS,
		.name 	= IPCMD_NAME_SERVER_CONNS,
		handler	: _muxSvrJSonConnsInfo
	},
	{
		.type	= CMD_TYPE_SVR_URLS,
		.name 	= IPCMD_NAME_SERVER_URLS,
		handler	: _muxSvrJSonUrlsInfo
	},
	{
		.type	= CMD_TYPE_UNKNOWN,
		.name	= NULL,
		.handler	= NULL
	}
};


