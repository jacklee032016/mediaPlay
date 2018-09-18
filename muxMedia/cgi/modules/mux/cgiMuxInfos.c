/*
* pages for PlayerInfo, MediaInfo and MetaData
*/

#include "_cgiMux.h"

static char *_onePlayerInfo(MUX_SERVER_INFO *svrInfo, int winIndex )
{
	char 	buf[8192*5];
	int 		length = 0;
	int		statusCode = 0;

	svrInfo->cgiMux.response = muxApiPlayMediaPlayerInfo(winIndex);
	if((svrInfo->cgiMux.response == NULL) || (muxApiGetDataReply(svrInfo->cgiMux.response, 0)==NULL))
	{
//		return NULL;
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" colspan=2>Not played</TD></TR>\n" );
		return strdup(buf);
	}

	statusCode = muxApiGetStatus(svrInfo->cgiMux.response);
	if(statusCode != IPCMD_ERR_NOERROR) //IPCMD_ERR_SERVER_INTERNEL_ERROR
	{
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" colspan=2>Not played</TD></TR>\n" );
		return strdup(buf);
	}

	cJSON *obj = muxApiGetDataReply(svrInfo->cgiMux.response, 0);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d </TD>\n</TR>\n", 
		gettext("Progress"), cmnGetIntegerFromJsonObject(obj, "progress"));

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d (Second)</TD>\n</TR>\n", 
		gettext("Time"), cmnGetIntegerFromJsonObject(obj, "playedTime"));

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("Status"), cmnGetStrFromJsonObject(obj, "playStatus"));

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("URL"), cmnGetStrFromJsonObject(obj, _MUX_JSON_NAME_MEDIA) );

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("speed"), cmnGetIntegerFromJsonObject(obj, "speed") );


	return strdup(buf);
}


int	muxServerPlayerInfos(MUX_SERVER_INFO *svrInfo)
{
	char 	buf[8192*5];
	int 		length = 0;
	int 		i;
	char		*onePlayer;

	length += CGI_SPRINTF(buf,length, "<table width=\"95%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	
#if CGI_OPTIONS_WINDOW_LIST
	for(i=0; i<cmn_list_size(&svrInfo->cgiMux.cgiCfgPlay.windows); i++)
	{
		onePlayer = _onePlayerInfo(svrInfo, i);
		if(onePlayer==NULL && i == 0 )
		{
			return cgi_error_page(NULL, gettext("Send message to server failed!"), gettext( CGI_STR_SOME_ERROR ) );
		}
		
		length += CGI_SPRINTF(buf, length, "<TR><TD align=\"center\" bgcolor=\"#cccccc\" colspan=2><strong>Window No.%d</strong></TD>\n\t</TR>\n", i);
		length += CGI_SPRINTF(buf, length, "%s", onePlayer);
	}
#endif

	length += CGI_SPRINTF(buf, length, "</TABLE>\n");

//	return cgi_info_page(gettext("Player Status"), gettext("Status list as following:") , buf);
	return cgi_refresh_page(5, MUXSERVER_HLINK_PLAYER_INFO, gettext("Player Status"), buf);

}

char *_oneSubtitle(cJSON *obj)
{
	char 	buf[4096];
	int 		length = 0;
	
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Stream Index"), cmnGetIntegerFromJsonObject(obj, "streamIdx"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("Format"), cmnGetStrFromJsonObject(obj, "format"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("External"), (cmnGetIntegerFromJsonObject(obj, "extSub")==1)?"Yes":"No");
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Original Width"), cmnGetIntegerFromJsonObject(obj, "originalWidth"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Original Height"), cmnGetIntegerFromJsonObject(obj, "originalHeight"));
	
	return strdup(buf);
}



char *_oneAudio(cJSON *obj)
{
	char 	buf[4096];
	int 		length = 0;
	
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Stream Index"), cmnGetIntegerFromJsonObject(obj, "streamIdx"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("Format"), cmnGetStrFromJsonObject(obj, "format"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Profile"), cmnGetIntegerFromJsonObject(obj, "profile"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("SampleRate"), cmnGetIntegerFromJsonObject(obj, "samplerate"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("BitPerSample"), cmnGetIntegerFromJsonObject(obj, "bitpersample"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Channels"), cmnGetIntegerFromJsonObject(obj, "channels"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("BPS"), cmnGetIntegerFromJsonObject(obj, "bps"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("Languge"), cmnGetStrFromJsonObject(obj, "language"));
	
	return strdup(buf);
}



char *_oneVideo(cJSON *obj)
{
	char 	buf[4096];
	int 		length = 0;
	
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Stream Index"), cmnGetIntegerFromJsonObject(obj, "streamIdx"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("Format"), cmnGetStrFromJsonObject(obj, "format"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Width"), cmnGetIntegerFromJsonObject(obj, "width"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Height"), cmnGetIntegerFromJsonObject(obj, "height"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("FPS"), cmnGetIntegerFromJsonObject(obj, "fps"));
	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("BPS"), cmnGetIntegerFromJsonObject(obj, "bps"));
	
	return strdup(buf);
}

static char *_oneArray(cJSON *obj, char *name,  char *(*iterateFunc)(cJSON *ele) )
{
	char 	buf[4096];
	int 		length = 0;
	
	int i;
	cJSON *array = cJSON_GetObjectItem(obj, name);

	if(array == NULL)
	{
		return "";//NULL;
	}

	if(!cJSON_IsArray(array))
	{
		return "";//NULL;
	}

	length += CGI_SPRINTF(buf, length, "<TR><TD align=\"center\" bgcolor=\"#cccccc\" colspan=2><strong>%s</strong></TD>\n\t</TR>\n", name);
	for(i=0; i< cJSON_GetArraySize(array); i++)
	{
		cJSON *item = cJSON_GetArrayItem( array, i);
		length += CGI_SPRINTF(buf, length, "%s",  (iterateFunc)(item));
	}

	return strdup(buf);
}

static char *_oneMediaInfo(MUX_SERVER_INFO *svrInfo, int winIndex )
{
	char 	buf[8192*5];
	int 		length = 0;
	int		statusCode =0;

	svrInfo->cgiMux.response = muxApiPlayMediaMediaInfo(winIndex);
	if((svrInfo->cgiMux.response == NULL) || (muxApiGetDataReply(svrInfo->cgiMux.response, 0)==NULL))
	{
//		return NULL;
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" colspan=2>Not played</TD></TR>\n" );
		return strdup(buf);
	}

	statusCode = muxApiGetStatus(svrInfo->cgiMux.response);
	if(statusCode != IPCMD_ERR_NOERROR) //IPCMD_ERR_SERVER_INTERNEL_ERROR
	{
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" colspan=2>Not played</TD></TR>\n" );
		return strdup(buf);
	}

	cJSON *obj = muxApiGetDataReply(svrInfo->cgiMux.response, 0);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("Type"), cmnGetStrFromJsonObject(obj, "streamType"));

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
		gettext("Source Type"), cmnGetStrFromJsonObject(obj, "sourceType"));

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s </TD>\n\t<TD align=\"center\">%d(KB)</TD>\n</TR>\n", 
		gettext("Size"), cmnGetIntegerFromJsonObject(obj, "fileSize")/1024);

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Start Time"), cmnGetIntegerFromJsonObject(obj, "startTime") );

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("Duration"), cmnGetIntegerFromJsonObject(obj, "duration") );

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%d</TD>\n</TR>\n", 
		gettext("BPS"), cmnGetIntegerFromJsonObject(obj, "bps") );

	length += CGI_SPRINTF(buf, length, "%s",  _oneArray(obj, "videos", _oneVideo) );
	length += CGI_SPRINTF(buf, length, "%s",  _oneArray(obj, "audios", _oneAudio) );
	length += CGI_SPRINTF(buf, length, "%s",  _oneArray(obj, "subtitles", _oneSubtitle) );
	

	return strdup(buf);
}



int	muxServerMediaInfos(MUX_SERVER_INFO *svrInfo)
{
	char 	buf[8192*5];
	int 		length = 0;
	int 		i;
	char		*onePlayer;

	length += CGI_SPRINTF(buf,length, "<table width=\"95%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");

#if CGI_OPTIONS_WINDOW_LIST
	for(i=0; i<cmn_list_size(&svrInfo->cgiMux.cgiCfgPlay.windows); i++)
	{
		onePlayer = _oneMediaInfo(svrInfo, i);
		if(onePlayer==NULL && i == 0 )
		{
			return cgi_error_page(NULL, gettext("Send message to server failed!"), gettext( CGI_STR_SOME_ERROR ) );
		}
		
		length += CGI_SPRINTF(buf, length, "<TR><TD align=\"center\" bgcolor=\"#cccccc\" colspan=2><strong>Window No.%d</strong></TD>\n\t</TR>\n", i);
		length += CGI_SPRINTF(buf, length, "%s", onePlayer);
	}
#endif

	length += CGI_SPRINTF(buf, length, "</TABLE>\n");

//	return cgi_info_page(gettext("Media Status"), gettext("Status list as following:") , buf);
	
	return cgi_refresh_page(5, MUXSERVER_HLINK_MEDIA_INFO, gettext("Media Status"), buf);
}


int	muxServerMetadataInfo(MUX_SERVER_INFO *svrInfo)
{
	char 	buf[8192*5];
	int 		length = 0;

#if CGI_OPTIONS_WINDOW_LIST
	int 		i;
	char		*onePlayer;
	length += CGI_SPRINTF(buf,length, "<table width=\"95%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	for(i=0; i<cmn_list_size(&svrInfo->cgiMux.cgiCfgPlay.windows); i++)
	{
		onePlayer = _oneMediaInfo(svrInfo, i);
		if(onePlayer==NULL && i == 0 )
		{
			return cgi_error_page(NULL, gettext("Send message to server failed!"), gettext( CGI_STR_SOME_ERROR ) );
		}
		
		length += CGI_SPRINTF(buf, length, "<TR><TD align=\"center\" bgcolor=\"#cccccc\" colspan=2><strong>Window No.%d</strong></TD>\n\t</TR>\n", i);
		length += CGI_SPRINTF(buf, length, "%s", onePlayer);
	}
#endif

	length += CGI_SPRINTF(buf, length, "</TABLE>\n");

	return cgi_info_page(gettext("Metadata Information"), gettext("Status list as following:"), buf);

}



