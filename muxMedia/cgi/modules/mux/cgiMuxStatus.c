

#include "_cgiMux.h"

#define	URL_DEFAULT_UDP		"udp://225.0.0.1:9090"
#define	URL_DEFAULT_HTTP		"http://192.168.168.100:8080/tests"
#define	URL_DEFAULT_RTSP		"rtsp://192.168.168.100:8554/tests"


#define JAVASCRIPTS		"<script> function submitForm(value){ "\
        "window.alert(value); document.getElementById('op').value = value;" \
        "document.getElementById('columnarForm').submit(); } </script>"


char *submitButton(char *name, char *actionName)
{
	char buf[1024];
	CGI_SPRINTF(buf, 0, 
		"<input type=\"button\" name=\"%s\" value=\"%s\" onclick=\"submitForm('%s')\" class=\"submit_button\" />",
		SUBMIT, name, actionName);
	return strdup(buf);
	
}

char *_selectionOptions(MUX_SERVER_INFO *svrInfo, int index)
{
	char buf[1024];
	int	length = 0;
	RECT_CONFIG *cfg;
	int	i = 0;
	
	length += CGI_SPRINTF(buf, length, "\n\t<select name=\"%s.%d\" style='width:160px;'>\n",  MUX_KEYWORD_WINDOW_INDEX, index );
#if CGI_OPTIONS_WINDOW_LIST
	for(i =0; i< cmn_list_size(&svrInfo->cgiMux.cgiCfgPlay.windows); i++)
	{
		cfg = (RECT_CONFIG *)cmn_list_get(&svrInfo->cgiMux.cgiCfgPlay.windows, i);
		
		length += CGI_SPRINTF(buf,length, "\t\t<option value=\"%d\" %s>%d [(%d,%d)(%d,%d)]</option>\n", 
			i, (cfg->type == RECT_TYPE_MAIN)?"selected":"", i, cfg->left, cfg->top, cfg->width, cfg->height);
	}
#endif

	length += CGI_SPRINTF(buf,length, "\t</select>\n\r\n");

	return strdup(buf);
}


int	muxServerUrlConfig(MUX_SERVER_INFO *svrInfo)
{
	char 	buf[8192*5];
	int 		length = 0;
	int		i = 0;

	length += CGI_SPRINTF(buf, length, "<form id=\"columnarForm\" method=\"post\" action=\"%s\">\n", WEB_URL_MUX_SERVER );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_KEYWORD_OPERATOR, CGI_KEYWORD_OPERATOR, CGI_MUX_SERVER_OP_CONFIG);

	length += CGI_SPRINTF(buf, length,  JAVASCRIPTS);

	length += CGI_SPRINTF(buf,length, "<table width=\"85%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD><TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n</TR>\n", 
		gettext("Protocol"), gettext("URL"), gettext("Window"), gettext("Action") );



/* table 1 */	
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\">HTTP</TD>\n\t<TD align=\"center\"><input name=\"%s\"  type=\"text\" size=\"50\" value=\"%s\" maxlength=\"50\" />"
		"</TD><TD align=\"center\">%s</TD><TD align=\"center\">%s</TD>\n</TR>\n", MUX_URL_KEYWORD_HTTP,  
		GET_VALUE( &svrInfo->cfgList, MUX_URL_KEYWORD_HTTP), _selectionOptions(svrInfo, i++), submitButton(gettext(CGI_STR_PLAY), CGI_MUX_SERVER_OP_HTTP) );
	
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\">RTSP</TD>\n\t<TD align=\"center\"><input name=\"%s\"  type=\"text\" size=\"50\" value=\"%s\" maxlength=\"50\" />"
		"</TD><TD align=\"center\">%s</TD><TD align=\"center\">%s</TD>\n</TR>\n", MUX_URL_KEYWORD_RTSP, 
		GET_VALUE( &svrInfo->cfgList, MUX_URL_KEYWORD_RTSP), _selectionOptions(svrInfo, i++), submitButton(gettext(CGI_STR_PLAY), CGI_MUX_SERVER_OP_RTSP) );

	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\">RTP</TD>\n\t<TD align=\"center\"><input name=\"%s\"  type=\"text\" size=\"50\" value=\"%s\" maxlength=\"50\" />"
		"</TD><TD align=\"center\">%s</TD><TD align=\"center\">%s</TD>\n</TR>\n",MUX_URL_KEYWORD_RTP,  
		GET_VALUE( &svrInfo->cfgList, MUX_URL_KEYWORD_RTP), _selectionOptions(svrInfo, i++), submitButton(gettext(CGI_STR_PLAY), CGI_MUX_SERVER_OP_RTP) );

	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\">UDP</TD>\n\t<TD align=\"center\"><input name=\"%s\"  type=\"text\" size=\"50\" value=\"%s\" maxlength=\"50\" />"
		"</TD><TD align=\"center\">%s</TD><TD align=\"center\">%s</TD>\n</TR>\n",MUX_URL_KEYWORD_UDP, 
		GET_VALUE( &svrInfo->cfgList, MUX_URL_KEYWORD_UDP), _selectionOptions(svrInfo, i++), submitButton(gettext(CGI_STR_PLAY), CGI_MUX_SERVER_OP_UDP) );

	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\">WMSP</TD>\n\t<TD align=\"center\"><input name=\"%s\"  type=\"text\" size=\"50\" value=\"%s\" maxlength=\"50\" />"
		"</TD><TD align=\"center\">%s</TD><TD align=\"center\">%s</TD>\n</TR>\n",MUX_URL_KEYWORD_WMSP, 
		GET_VALUE( &svrInfo->cfgList, MUX_URL_KEYWORD_WMSP), _selectionOptions(svrInfo, i++), submitButton(gettext(CGI_STR_PLAY), CGI_MUX_SERVER_OP_WMSP) );

	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\">RTMP</TD>\n\t<TD align=\"center\"><input name=\"%s\"  type=\"text\" size=\"50\" value=\"%s\" maxlength=\"50\" />"
		"</TD><TD align=\"center\">%s</TD><TD align=\"center\">%s</TD>\n</TR>\n",MUX_URL_KEYWORD_RTMP, 
		GET_VALUE( &svrInfo->cfgList, MUX_URL_KEYWORD_RTMP), _selectionOptions(svrInfo, i++), submitButton(gettext(CGI_STR_PLAY), CGI_MUX_SERVER_OP_RTMP) );

	/* */
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\">Input</TD>\n\t<TD align=\"center\"><input name=\"%s\"  type=\"text\" size=\"50\" value=\"%s/\" maxlength=\"50\" />"
		"</TD><TD align=\"center\">%s</TD><TD align=\"center\">%s</TD>\n</TR>\n", MUX_URL_KEYWORD_INPUT, svrInfo->cgiMux.cgiMain.mediaCaptureConfig.usbHome,
		_selectionOptions(svrInfo, i), submitButton(gettext(CGI_STR_PLAY), CGI_MUX_SERVER_OP_INPUT) );


	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"left\" colspan=\"4\">Notes:<br>Directory of USD Disk : %s<br>Directory of SD Card : %s</TD>\n</TR>\n", 
		svrInfo->cgiMux.cgiMain.mediaCaptureConfig.usbHome, svrInfo->cgiMux.cgiMain.mediaCaptureConfig.sdHome);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" colspan=\"4\">%s</TD>\n"
		"</TR>\n", submitButton(gettext(CGI_STR_SAVE), CGI_MUX_SERVER_OP_SAVE) );
	
	length += CGI_SPRINTF(buf, length, "</TABLE>\n");
	length += CGI_SPRINTF(buf, length, "</form>\n");

	cgi_info_page(gettext("URL List"), gettext("Select and play the optional URL"), buf );
	
	return 0;
}

int	muxServerUrlPlay(MUX_SERVER_INFO *svrInfo)
{
	int windowIndex = 0;
	char	winKeyword[256];
	char		*keyword = NULL;
	char *cmd = GET_VALUE( &svrInfo->cgiMux.cgiVariables, CGI_KEYWORD_OPERATOR);

#if 0
	char *urlHttp = GET_VALUE( &svrInfo->cgiVariables, MUX_URL_KEYWORD_HTTP);
	char *urlRtsp = GET_VALUE( &svrInfo->cgiVariables, MUX_URL_KEYWORD_RTSP);
	char *urlRtp = GET_VALUE( &svrInfo->cgiVariables, MUX_URL_KEYWORD_RTP);
	char *urlUdp = GET_VALUE( &svrInfo->cgiVariables, MUX_URL_KEYWORD_UDP);
	char *urlWmsp = GET_VALUE( &svrInfo->cgiVariables, MUX_URL_KEYWORD_WMSP);
	char *urlRtmp = GET_VALUE( &svrInfo->cgiVariables, MUX_URL_KEYWORD_RTMP);
#endif

	if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_HTTP ) )
	{
		windowIndex = 0;
		keyword = MUX_URL_KEYWORD_HTTP;
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_RTSP ) )
	{
		windowIndex = 1;
		keyword = MUX_URL_KEYWORD_RTSP;
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_RTP ) )
	{
		windowIndex = 2;
		keyword = MUX_URL_KEYWORD_RTP;
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_UDP ) )
	{
		windowIndex = 3;
		keyword = MUX_URL_KEYWORD_UDP;
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_WMSP ) )
	{
		windowIndex = 4;
		keyword = MUX_URL_KEYWORD_WMSP;
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_RTMP ) )
	{
		windowIndex = 5;
		keyword = MUX_URL_KEYWORD_RTMP;
	}
	else if( !strcasecmp(cmd, CGI_MUX_SERVER_OP_INPUT ) )
	{
		windowIndex = 6;
		keyword = MUX_URL_KEYWORD_INPUT;
	}

	char *url = GET_VALUE( &svrInfo->cgiMux.cgiVariables, keyword);

#if 1	
	cgidebug("select window index from protocols No.%d \n", windowIndex);
	{
		char		*value;
		int 	i =0;
		for(i=0;i<6; i++)
		{
			CGI_SPRINTF(winKeyword, 0, "%s.%d",MUX_KEYWORD_WINDOW_INDEX, i);
			value = GET_VALUE(&svrInfo->cgiMux.cgiVariables, winKeyword) ;
			cgidebug("No.%d keyword '%s', value:'%s'\n", i, winKeyword, value);
		}
	}
#endif

	CGI_SPRINTF(winKeyword, 0, "%s.%d",MUX_KEYWORD_WINDOW_INDEX, windowIndex);
	windowIndex = atoi(GET_VALUE(&svrInfo->cgiMux.cgiVariables, winKeyword) );
	cgidebug("No.%d windows is selected, cmd: '%s', keyword:'%s'\n", windowIndex, cmd, winKeyword);

	if(url== NULL || strlen(url)==0)
	{
		return cgi_error_page(NULL, gettext("URL is null, please try again!"), gettext( CGI_STR_SOME_ERROR ) );
	}
	
	return cgiMuxPlayUrl(url, windowIndex, MUXSERVER_HLINK_URLS, MUXSERVER_HLINK_PLAYER_INFO);
}


int	muxServerUrlSave(MUX_SERVER_INFO *svrInfo)
{
//	char *cmd = GET_VALUE( &svrInfo->cgiMux.cgiVariables, CGI_KEYWORD_OPERATOR);
	
	char *urlHttp = GET_VALUE( &svrInfo->cgiMux.cgiVariables, MUX_URL_KEYWORD_HTTP);
	char *urlRtsp = GET_VALUE( &svrInfo->cgiMux.cgiVariables, MUX_URL_KEYWORD_RTSP);
	char *urlRtp = GET_VALUE( &svrInfo->cgiMux.cgiVariables, MUX_URL_KEYWORD_RTP);
	char *urlUdp = GET_VALUE( &svrInfo->cgiMux.cgiVariables, MUX_URL_KEYWORD_UDP);
	char *urlWmsp = GET_VALUE( &svrInfo->cgiMux.cgiVariables, MUX_URL_KEYWORD_WMSP);
	char *urlRtmp = GET_VALUE( &svrInfo->cgiMux.cgiVariables, MUX_URL_KEYWORD_RTMP);

	printf("HTTP URL: %s<br>", urlHttp);
	printf("RTSP URL: %s<br>", urlRtsp);
	printf("RTP URL: %s<br>", urlRtp);
	printf("UDP URL: %s<br>", urlUdp);
	printf("WMSP URL: %s<br>", urlWmsp);
	printf("RTMP URL: %s<br>", urlRtmp);

	return muxServerUrlConfig( svrInfo);

}

