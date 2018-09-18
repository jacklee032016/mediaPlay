/*
* 
*/

#include "_cgiMux.h"

static int __status_init(MUX_SERVER_INFO *svrInfo)
{
	memset(svrInfo, 0, sizeof(MUX_SERVER_INFO));
	
	cgiMuxInit( &svrInfo->cgiMux);

	if (readconfig(MUX_PLAYER_CONFIG_FILE, ITEM_DBL, NULL, &svrInfo->cfgList) != 0)
	{
		cgi_error_page(NULL, gettext(CGI_STR_ERROR), gettext(CGI_STR_WITHOUT_CONF_FILE) );
cgitrace();
		return -1;
	}
	
	return 0;
}


int main(int argc, char *argv[])
{
	MUX_SERVER_INFO	_svrInfo;
	MUX_SERVER_INFO	*svrInfo = &_svrInfo;
	char 	*cmd=NULL;
//	int res = 0;

	if(__status_init(svrInfo) )
	{
		cgi_error_page(NULL, gettext("Player Status Operation Failed"), gettext( CGI_STR_SOME_ERROR ) );
		return 1;
	}

	cmd = GET_CGI_OP(&svrInfo->cgiMux.cgiVariables);
	if( !cmd || !strlen(cmd) || !strcasecmp(cmd, CGI_MUX_SERVER_OP_CONFIG ) )
	{
		return	muxServerUrlConfig(svrInfo);
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_LIST ) )
	{
		return muxServerPlayerInfos(svrInfo);
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_MEDIA_INFO ) )
	{
		return muxServerMediaInfos(svrInfo);
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_METADATA_INFO) )
	{
		return muxServerMetadataInfo(svrInfo);
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_HTTP ) || 
		!strcasecmp(cmd, CGI_MUX_SERVER_OP_RTSP ) ||
		!strcasecmp(cmd, CGI_MUX_SERVER_OP_RTP ) ||
		!strcasecmp(cmd, CGI_MUX_SERVER_OP_UDP ) ||
		!strcasecmp(cmd, CGI_MUX_SERVER_OP_WMSP ) ||
		!strcasecmp(cmd, CGI_MUX_SERVER_OP_RTMP ) ||
		!strcasecmp(cmd, CGI_MUX_SERVER_OP_INPUT) )
	{
		return muxServerUrlPlay(svrInfo);
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_SAVE ) )
	{
		return muxServerUrlSave(svrInfo);
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_WINDOW ) )
	{
		return muxServerWindow(svrInfo);
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_PLAYLIST ) )
	{
		return muxServerPlaylist(svrInfo);
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_MEDIAFILES) )
	{
		return muxServerMediaFiles(svrInfo);
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_RECORD) )
	{
		return muxServerRecord(svrInfo);
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_GLOBAL) )
	{
		return muxCgiGlobalConfig(svrInfo);
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_SERVER) )
	{
		return muxCgiSvrRuntime(svrInfo);
	}
	else
	{
		cgi_invalidate_page();
		return 0;
	}
	
	return 0;	
}


