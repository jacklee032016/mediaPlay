
#include "_cgiMux.h"

int	muxCgiGlobalConfig(MUX_SERVER_INFO *svrInfo)
{
	char 	buf[8192*5];
	int 		length = 0;

	char *action = GET_CGI_ACTION( &svrInfo->cgiMux.cgiVariables);

	MuxPlugIn *plugin = svrInfo->cgiMux.cgiMain.plugins;
	MuxMediaConfig *mc = &svrInfo->cgiMux.cgiMain.mediaCaptureConfig;

//	length += CGI_SPRINTF(buf, length, "<form id=\"columnarForm\" method=\"post\" action=\"%s\">\n", WEB_URL_MUX_SERVER );
//	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_KEYWORD_OPERATOR, CGI_KEYWORD_OPERATOR, CGI_MUX_SERVER_OP_CONFIG);
//	length += CGI_SPRINTF(buf, length,  JAVASCRIPTS);

	if( !action || !strlen(action) || !strcasecmp(action, CGI_MUX_SERVER_ACTION_GLOBAL_PLUGIN) )
	{
		length += CGI_SPRINTF(buf,length, "<table width=\"85%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\" colspan=3><strong>Function Plugins Configuration</strong></TD>\n" );
		length += CGI_SPRINTF(buf, length, "<TR>\n\t"
			"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
			"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n", 
			gettext("Plugin"), gettext("Binary"), gettext("Status") );
		
		while(plugin)
		{
			length += CGI_SPRINTF(buf, length, "<TR>\n\t"
				"<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD><TD align=\"center\">%s</TD>\n</TR>\n", plugin->name, plugin->path, STR_BOOL_VALUE(plugin->enable) );
			plugin = plugin->next;
		}
		length += CGI_SPRINTF(buf, length, "</TABLE><br>\n");

		return cgi_info_page(gettext("Global Configuration"), gettext("Function Modules"), buf );

	}
	else if( !strcasecmp(action, CGI_MUX_SERVER_ACTION_GLOBAL_CONTROLLER) )
	{
		length += CGI_SPRINTF(buf,length, "<table width=\"85%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");

		length += CGI_SPRINTF(buf, length, "<TR>\n\t"
			"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" >%d</TD></TR>\n"
			"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" >%d</TD></TR>\n"
			"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" >%s</TD></TR>\n", 
			gettext("UDP Port"), svrInfo->cgiMux.cgiMain.udpCtrlPort, 
			gettext("TCP Port"), svrInfo->cgiMux.cgiMain.tcpCtrlPort, 
			gettext("Unix Socket"), svrInfo->cgiMux.cgiMain.unixPort);

		length += CGI_SPRINTF(buf, length, "</TABLE><br>\n");

		return cgi_info_page(gettext("Global Configuration"), gettext("IP Command Controller"), buf );
	}
	else if( !strcasecmp(action, CGI_MUX_SERVER_ACTION_GLOBAL_CAPTURER ) )
	{
		length += CGI_SPRINTF(buf,length, "<table width=\"85%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");

		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\" colspan=3><strong>Media Capturing Configuration</strong></TD>\n" );
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n",
			gettext("Item"), gettext("Configuration") );
		
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">Video Type</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
			CMN_MUX_FIND_VIDEO_OUT_FORMAT_TYPE(mc->videoType) );
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">Video CapLevel</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
			CMN_MUX_FIND_VIDEO_OUT_CAPLEVEL_TYPE(mc->videoCapLevel) );
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">Video Bitrate</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", mc->videoBitrate );
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">Video GOP</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", mc->gop );

		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">Video Framerate</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", mc->outputFrameRate );
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">Video AspectRatioWindow</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", STR_BOOL_VALUE(mc->aspectRatioWindow) );


		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">Audio Type</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", "AAC" );
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">Audio Samplerate</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", mc->audioSampleRate );
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">Audio Format</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", mc->audioFormat);
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">Audio Channels</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", mc->audioChannels );

		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">A-V Sync Type</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", CMN_MUX_FIND_SYNC_NAME(mc->avSyncType) );
		
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">Saving Location</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
			(mc->storeType==MEDIA_DEVICE_SDCARD)?mc->sdHome:mc->usbHome);

		
		length += CGI_SPRINTF(buf, length, "</TABLE>\n");

		return cgi_info_page(gettext("Global Configuration"), gettext("Configuration for captuering media from player"), buf );
	}
	else
	{
		cgi_invalidate_page();
		return 0;
	}
	return 0;
}


