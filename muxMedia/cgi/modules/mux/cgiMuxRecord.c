/*
* 
*/

#include "_cgiMux.h"

/* page for input filename and duration, and start button */
static int	_muxServerRecordFile(MUX_SERVER_INFO *svrInfo)
{
	char 	buf[8192*5];
	int 		length = 0;
	char 	*path = (svrInfo->cgiMux.cgiMain.mediaCaptureConfig.storeType== MEDIA_DEVICE_SDCARD)?svrInfo->cgiMux.cgiMain.mediaCaptureConfig.sdHome:svrInfo->cgiMux.cgiMain.mediaCaptureConfig.usbHome;

	length += CGI_SPRINTF(buf, length, "<form id=\"columnarForm\" method=\"post\" action=\"%s\">\n", WEB_URL_MUX_SERVER );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_KEYWORD_OPERATOR, CGI_KEYWORD_OPERATOR, CGI_MUX_SERVER_OP_RECORD);
	
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_ACTION, CGI_ACTION, CGI_ACTION_START);


	length += CGI_SPRINTF(buf,length, "<table width=\"95%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>Item</strong>"
		"</TD><TD align=\"center\" bgcolor=\"#cccccc\"><strong>Item</strong></TD></TR>\n");

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>File Name</strong></TD>"
		"<TD>%s/<input name=\"filename\"  type=\"text\" size=\"30\" value=\"\" maxlength=\"50\" />(Only '*.mkv' is supportted)</TD>\n</TR>\n", path);
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"path\" id=\"path\" value=\"%s\">\n", path);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>", gettext("Duration") );
	length += CGI_SPRINTF(buf,length, "<TD><input name=\"%s\" onKeyPress=\"\" onChange=\"checkIsNotNULL(%s,'%s')\"  type=\"text\" size=\"5\" value=\"\" maxlength=\"5\">Seconds", 
		"duration", "duration",gettext("duration can not be NULL") );

	length += CGI_SPRINTF(buf, length, "</TABLE>\n");

	length += CGI_SPRINTF(buf, length, "<br><p align=\"center\">%s %s</p><br>", cgi_submit_button(gettext("Start Record")), cgi_reset_button(gettext(CGI_STR_RESET)) );
	
	cgi_info_page(gettext("Recording"), gettext("Input recording parameters and start recording"), buf );
	
	return 0;
}


/* page display recording status and stop button */
static int __recorderStatusPageHtml(MUX_SERVER_INFO *svrInfo )
{
	char 	buf[8192*5];
	int 		length = 0;

/* table 1 */	
	length += CGI_SPRINTF(buf,length, "<table width=\"85%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"</TR>\n", gettext("Item"), gettext("Content") );

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("Last File"), svrInfo->recordStatus.lastFile);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("Current File"), svrInfo->recordStatus.currentFile);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("Status"), svrInfo->recordStatus.status);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d(second)</TD>\n</TR>\n", 
		gettext("Total Time"), svrInfo->recordStatus.totalTime);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d(MB)</TD>\n</TR>\n", 
		gettext("Total Bytes"), svrInfo->recordStatus.totalBytes/1024);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Packets"), svrInfo->recordStatus.packets);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" colspan=\"2\">%s</TD></TR>\n", cgi_button(gettext("Stop Recording"), MUXSERVER_HLINK_RECORD_STOP));

	length += CGI_SPRINTF(buf, length, "</TABLE>\n");

	if(!strncasecmp(svrInfo->recordStatus.status, "stop", 4))
	{
		return cgi_info_page(gettext("Recording Status"), gettext("Status list as following:") , buf );
	}

	return cgi_refresh_page( 5, MUXSERVER_HLINK_RECORD_STATUS, gettext("Recording Status"), buf);
}


static int _parseRecorderStatus(MUX_SERVER_INFO *svrInfo, cJSON *resObj)
{
	char	*temp = NULL;
	snprintf(svrInfo->recordStatus.currentFile, 2048, "%s", cmnGetStrFromJsonObject(resObj, "filename"));

	temp =  cmnGetStrFromJsonObject(resObj, "recordStatus");
	snprintf(svrInfo->recordStatus.status, 128, "%s", IS_STRING_NULL(temp)?"STOP":temp);
	cgidebug( "<br>status : '%s' <br>", svrInfo->recordStatus.status);

	svrInfo->recordStatus.totalTime=  cmnGetIntegerFromJsonObject(resObj, "recordTime");	/* ms */
	cgidebug( "<br>total Time : '%d' second<br>", svrInfo->recordStatus.totalTime);

	svrInfo->recordStatus.totalBytes = cmnGetIntegerFromJsonObject(resObj, "size"); /*KB */
	cgidebug("<br>total : '%d' KB<br>", svrInfo->recordStatus.totalBytes);
		
	svrInfo->recordStatus.packets = cmnGetIntegerFromJsonObject(resObj, "packets"); /*KB */
	cgidebug("<br>total : '%d' packets<br>", svrInfo->recordStatus.packets);
	return 0;
}


static char *_recorderConfigPageHtml(MUX_SERVER_INFO *svrInfo )
{
	char 	buf[8192*5];
	int 		length = 0;

/* table 1 */	
	length += CGI_SPRINTF(buf,length, "<table width=\"85%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"</TR>\n", gettext("Item"), gettext("Content") );

#if 0
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Start Time"), svrInfo->recordCfg.startTime);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s(Second)</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Duration"), svrInfo->recordCfg.duration );

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Video Format"),svrInfo->recordCfg.videoFormat);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("CapLevel"), svrInfo->recordCfg.capLevel);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Frame Rate"),svrInfo->recordCfg.frameRate);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Audio Format"), svrInfo->recordCfg.audioFormat);
#endif

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("Capture Source"), svrInfo->cgiMux.cgiCfgRecord.captureName);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("Save Format"), svrInfo->cgiMux.cgiCfgRecord.format);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("No Video"), STR_BOOL_VALUE(svrInfo->cgiMux.cgiCfgRecord.noVideo) );

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("No Audio"), STR_BOOL_VALUE(svrInfo->cgiMux.cgiCfgRecord.noAudio) );

//	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
//		gettext("No Subtitle"), STR_BOOL_VALUE(svrInfo->cgiMux.cgiCfgRecord.noSubtitle) );

	length += CGI_SPRINTF(buf, length, "</TABLE>\n");

	return strdup(buf);
}



int	muxServerRecord(MUX_SERVER_INFO *svrInfo)
{
	int res = 0;
	char *action = GET_CGI_ACTION( &svrInfo->cgiMux.cgiVariables);
	cJSON *resObj = NULL;

	if( !action || !strlen(action) || !strcasecmp(action, CGI_ACTION_STATUS) )
	{
		svrInfo->cgiMux.response = muxApiPlayMediaRecordStatus();
		if((svrInfo->cgiMux.response == NULL) || (muxApiGetDataReply(svrInfo->cgiMux.response, 0)==NULL))
		{
			cgi_error_page(NULL, gettext("Response from server is invalidate!"), gettext( CGI_STR_SOME_ERROR ) );
			return 1;
		}
		
		resObj = muxApiGetDataReply(svrInfo->cgiMux.response, 0);
		if(resObj == NULL)
		{
			sprintf(svrInfo->recordStatus.status,"%s", "stop");
		}
		else
		{
			res = _parseRecorderStatus( svrInfo, resObj);
			if(res < 0)
			{
				cgi_error_page(NULL, gettext("Response from server is invalidate!"), gettext( CGI_STR_SOME_ERROR ) );
				return 1;
			}
		}

		if(!strncasecmp(svrInfo->recordStatus.status, "stop", 4))
		{
			_muxServerRecordFile(svrInfo);
		}
		else
		{
			return __recorderStatusPageHtml(svrInfo);
		}
	}
	else if( !strcasecmp(action, CGI_ACTION_STOP ) )
	{
		svrInfo->cgiMux.response = muxApiPlayMediaRecordStop();
		return cgi_refresh_page(1, MUXSERVER_HLINK_RECORD_STATUS, "Stop recording", "Please waiting a moment.....");	
	}
	else if( !strcasecmp(action, CGI_ACTION_START ) )
	{
		char buf[1024];
		char recordFile[CMN_NAME_LENGTH];
		
		int duration = atoi(GET_VALUE(&svrInfo->cgiMux.cgiVariables, "duration"));
		char *filename = GET_VALUE(&svrInfo->cgiMux.cgiVariables, "filename");
		char *path = GET_VALUE(&svrInfo->cgiMux.cgiVariables, "path");

		snprintf(recordFile, sizeof(recordFile), "%s/%s", path, filename);
		svrInfo->cgiMux.response = muxApiPlayMediaRecordStart( recordFile, duration*1000);

		snprintf(buf, sizeof(buf), "Recording %d second content into file of '%s'. Please waiting.....", duration, recordFile);
		return cgi_refresh_page(1, MUXSERVER_HLINK_RECORD_STATUS, "Begin recording", buf);	
	}
	else if( !strcasecmp(action, CGI_ACTION_CONFIG ) )
	{
		cgi_info_page(gettext("Configuration of Recorder"), gettext("Current configuration of recorder"), _recorderConfigPageHtml(svrInfo)  );
		return 0;	
	}
	else
	{
		cgi_invalidate_page();
		return 0;
	}
	
	return 0;	
}


