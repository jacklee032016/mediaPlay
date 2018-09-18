
#include "libCgi.h"

int cgiMuxInit(MUX_WEB_CMN *muxWeb)
{
	CGI_HTML_HEADE();

	memset(muxWeb, 0, sizeof(MUX_WEB_CMN));
	read_cgi_input(&muxWeb->cgiVariables, NULL, NULL);

	if( cmnMuxMainParse(MUX_MAIN_CONFIG_FILE, &muxWeb->cgiMain ) )
	{
		cgi_error_page(NULL, gettext(CGI_STR_ERROR), gettext(CGI_STR_WITHOUT_CONF_FILE) );
		return -1;
	}

	if( cmnMuxPlayerParseConfig(MUX_PLAYER_CONFIG_FILE, &muxWeb->cgiCfgPlay) )
	{
		cgi_error_page(NULL, gettext(CGI_STR_ERROR), gettext(CGI_STR_WITHOUT_CONF_FILE) );
		return -1;
	}

	cmnMuxConfigParseWeb(MUX_WEB_CONFIG_FILE, &muxWeb->cgiCfgWeb);

	if ( cmnMuxConfigParseRecord(MUX_RECORDER_CONFIG_FILE, &muxWeb->cgiCfgRecord ) < 0)
	{
		cgi_error_page(NULL, gettext(CGI_STR_ERROR), gettext(CGI_STR_WITHOUT_CONF_FILE) );
		return EXIT_FAILURE;
	}

#if 0
	/* web server redirect all stderr of CGI to its log file, so CGI needs no real log, just output to its stderr */
	sprintf(muxWeb->cfg.serverLog.name, "%s", "WebAdmin");
	sprintf(muxWeb->cfg.serverLog.logFileName, "%s", CMN_LOG_FILE_WEBADMIN);

	muxWeb->cfg.serverLog.isDaemonized = 0;	/* no matter what configuration of player is, webAdmin never runs as daemon */
	if(cmn_log_init(&muxWeb->cfg.serverLog)<0)
	{
		printf("%s Log Init Failed.\n", muxWeb->cfg.serverLog.name );
	}
#endif

	cmnMediaInit(&muxWeb->cgiMain.mediaCaptureConfig);

	cmnMuxClientInit(3600, CTRL_LINK_TCP, "127.0.0.1");//"192.168.168.101");

	return 0;	
}


int	cgiMuxPlayUrl(char *url, int winIndex, char *errRedirectPage, char *sucessRedirectPage)
{
	cJSON *res = NULL;
	char	buf[1024];

	if(!url|| !strlen(url))
	{
		return cgi_refresh_page(10, errRedirectPage, gettext("Retry"), gettext("Reselect media file, restart....") );
	}

	res = muxApiPlayMediaPlay(winIndex, url, 1);
	cgidebug("Window index : %d\n", winIndex);

	if(muxApiGetStatus(res) == IPCMD_ERR_NOERROR)
	{
		CGI_SPRINTF(buf, 0, "'%s' is playing now....", url);
		return cgi_refresh_page(5, sucessRedirectPage, gettext("Play seccessfully"), buf);
	}
	else
	{
		CGI_SPRINTF(buf, 0, "'%s' playing failed, try later....", url);
		return cgi_refresh_page(5, errRedirectPage, gettext("try later"), buf);
	}
}


