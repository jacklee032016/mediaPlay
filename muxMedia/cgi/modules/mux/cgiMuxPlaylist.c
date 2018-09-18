

#include "_cgiMux.h"

	char *days[7] =
	{
		"Mon","Tue","Wed","Thur", "Fri","Sat","Sun"
	};


PLAY_LIST *checkFieldsInPlaylist(MUX_SERVER_INFO *svrInfo)
{
	PLAY_LIST *playlist = NULL;
	int i;
	int res = EXIT_SUCCESS;

	char *name = GET_VALUE(&svrInfo->cgiMux.cgiVariables, "name");
	int hour = atoi(GET_VALUE(&svrInfo->cgiMux.cgiVariables, "hour"));
	int minute = atoi(GET_VALUE(&svrInfo->cgiMux.cgiVariables, "minute"));
	int day = atoi(GET_VALUE(&svrInfo->cgiMux.cgiVariables, "dayselect"));
	int repeat = atoi(GET_VALUE(&svrInfo->cgiMux.cgiVariables, "repeat"));
	int enable = atoi(GET_VALUE(&svrInfo->cgiMux.cgiVariables, "enable"));
	char *newFile = GET_VALUE(&svrInfo->cgiMux.cgiVariables, "newMediaFile");
	int count = atoi(GET_VALUE(&svrInfo->cgiMux.cgiVariables, "fileCount"));

	if(name==NULL || !strlen(name))
		res = EXIT_FAILURE;
	
	if(!strcasecmp(newFile, "None") && count == 0)
	{/* no media file is selected when add a new playlist */
		res = EXIT_FAILURE;
	}
	
	if(hour<-1 || hour>23)
		hour = -1;
	if(minute<-1 || minute > 59)
		minute = -1;

	if(res == EXIT_FAILURE)
	{
		return NULL;
	}

	playlist = cmn_malloc(sizeof(PLAY_LIST));
	cmn_list_init(&playlist->fileList);
	
	snprintf(playlist->name, CMN_NAME_LENGTH, "%s", name);
	playlist->hour = hour;
	playlist->minute = minute;
	playlist->dayOfWeek = day;
	playlist->enable = enable;
	playlist->repeat = repeat;

	for(i=0; i< count; i++)
	{
		char key[32];
		sprintf(key, "File.%d", i);
		char *filename = GET_VALUE(&svrInfo->cgiMux.cgiVariables, key);
		cmn_list_append(&playlist->fileList, filename);
	}
	
	if(strcasecmp(newFile, "None"))
	{
		cmn_list_append(&playlist->fileList, newFile);
	}

	return playlist;
}

char *_html4PlayList(PLAY_LIST *playlist)
{
	char 	buf[8192*5];
	int 		length = 0;
	int 		i, j;

	length += CGI_SPRINTF(buf,length, "<table width=\"85%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>"
		"<TD><input name=\"%s\"  type=\"text\" size=\"50\" value=\"%s\" maxlength=\"50\" /></TD>\n</TR>\n", 
		gettext("Name"), "Name", (playlist!=NULL)?playlist->name:"" );

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>", gettext("Playtime") );
	length += CGI_SPRINTF(buf,length, "<TD>%s:<input name=\"%s\" onKeyPress=\"\" onChange=\"checkIsNotNULL(hour,'%s')\"  type=\"text\" size=\"5\" value=\"%d\" maxlength=\"2\">", 
		gettext("Hour"), "hour",gettext("hour can not be NULL"), (playlist!=NULL)?playlist->hour:-1);
	length += CGI_SPRINTF(buf,length, "%s:<input name=\"%s\" onKeyPress=\"\" onChange=\"checkIsNotNULL(minutes,'%s')\"  type=\"text\" size=\"5\" value=\"%d\" maxlength=\"2\">", 
		gettext("Minute"), "minute",gettext("minutes can not be NULL"), (playlist!=NULL)?playlist->minute:-1);

	length += CGI_SPRINTF(buf,length, "Date:<select name=\"dayselect\">");

	for(i=0; i<7; i++)
	{
		length += CGI_SPRINTF(buf,length, "<option value=\"%d\" %s>%s</option>", i+1,  ((playlist!=NULL)&& playlist->dayOfWeek ==(i+1))?"selected":"", days[i]);
	}
	
	length += CGI_SPRINTF(buf,length, "</select>Hour:-1~23;Minute:-1~59<br>When Hour or Minute is -1, the playlist will not be played based on date and time</TD></TR>\r\n");	

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>"
		"<TD><input name=\"%s\"  type=\"text\" size=\"5\" value=\"%d\" maxlength=\"2\" />When repeat is 0(zero), unlimited replay.</TD>\n</TR>\n", 
		gettext("Repeat"), "repeat", (playlist!=NULL)?playlist->repeat:1 );

	length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>"
		"<TD><select name=\"%s\" ><option value=\"1\" %s>%s</option><option value=\"0\" %s>%s</option></select></TD>\n</TR>\n", 
		gettext(CGI_STR_ENABLE), "enable",  ((playlist!=NULL)&&(playlist->enable!=0))?"selected":"", gettext(CGI_STR_YES),  ((playlist!=NULL)&&(playlist->enable==0))?"selected":"", gettext(CGI_STR_NO));

	if(playlist)
	{
		for(j = 0; j< cmn_list_size(&playlist->fileList); j++)
		{
			char target[256];
			char *filename = (char *)cmn_list_get(&playlist->fileList, j);
			sprintf(target, MUXSERVER_HLINK_PLAYLIST_DELETE_FILE"&playlist=%s&file=%s", playlist->name, filename);
			length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>No.%d</strong></TD>\n\t<TD>"
				"<input name=\"File.%d\" type=\"text\" size=\"50\" value=\"%s\" maxlength=\"50\" />%s</TD>\n</TR>\n", 
				j+1, j,  filename, cgi_button(gettext(CGI_STR_DELETE), target) );
		}
		length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"fileCount\" id=\"fileCount\" value=\"%d\">\n", j);
	}
	else
	{
		length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"fileCount\" id=\"fileCount\" value=\"0\">\n" );
	}


	cmn_list_t *filelist = cmnMediaScanGetFiles();
	if(filelist==NULL || cmn_list_size(filelist)==0)
	{
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\" colspan=\"2\"><strong>No media file is found in device</strong></TD></TR>");
	}
	else
	{
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>New</strong></TD>\n\t<TD><select name=\"newMediaFile\" >");
		length += CGI_SPRINTF(buf,length, "<option value=\"None\">None</option>" );
		for(i=0; i< cmn_list_size(filelist); i++)
		{
			char filename[CMN_NAME_LENGTH];
			MEDIA_FILE_T *file = (MEDIA_FILE_T *)cmn_list_get(filelist, i);
			sprintf(filename, "%s/%s", file->path, file->name);
			length += CGI_SPRINTF(buf,length, "<option value=\"%s\">%s</option>", filename, filename);
		}
		length += CGI_SPRINTF(buf,length, "</select></TD></TR>\r\n");	

		cmn_list_ofchar_free(filelist, TRUE);
	}
	
	length += CGI_SPRINTF(buf, length, "</TABLE>\n");

	return strdup(buf);
}

static int	_muxServerPlaylistNew(MUX_SERVER_INFO *svrInfo)
{
	char 	buf[8192*5];
	int 		length = 0;

	length += CGI_SPRINTF(buf, length, "<form id=\"columnarForm\" method=\"post\" action=\"%s\">\n", WEB_URL_MUX_SERVER );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_KEYWORD_OPERATOR, CGI_KEYWORD_OPERATOR, CGI_MUX_SERVER_OP_PLAYLIST);
	
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_ACTION, CGI_ACTION, CGI_STR_ADD);
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"index\" id=\"index\" value=\"%d\">\n", -1);

	char *table = _html4PlayList(NULL);
	length += CGI_SPRINTF(buf, length, "%s\n", table);
	cmn_free(table);

	length += CGI_SPRINTF(buf, length, "<br><p align=\"center\">%s %s</p><br>", cgi_submit_button(gettext(CGI_STR_ADD)), cgi_reset_button(gettext(CGI_STR_RESET)) );
	
	cgi_info_page(gettext("New playList from local media files"), gettext("Add a new playlist with local media files"), buf );
	
	return 0;
}


static int	_muxServerPlaylistModify(MUX_SERVER_INFO *svrInfo)
{
	char 	buf[8192*5];
	int 		length = 0;
	PLAY_LIST *playlist = NULL;
	int i;

	char *playlistName = GET_VALUE(&svrInfo->cgiMux.cgiVariables, "playlist");

	for(i=0; i< cmn_list_size(&svrInfo->cgiMux.cgiMain.playlists); i++)
	{
		PLAY_LIST *tmp = cmn_list_get(&svrInfo->cgiMux.cgiMain.playlists, i);
	 	if(!strcasecmp(tmp->name, playlistName))
		{
			playlist = tmp;
			break;
		}
	}

	if(playlistName==NULL || playlist==NULL)
	{
		char		buf[256];
		sprintf(buf, "Playlist with name of '%s' is not available", playlistName);
		return cgi_error_page(MUXSERVER_HLINK_PLAYLIST, gettext(CGI_STR_ERROR), buf);
	}

	length += CGI_SPRINTF(buf, length, "<form id=\"columnarForm\" method=\"post\" action=\"%s\">\n", WEB_URL_MUX_SERVER );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_KEYWORD_OPERATOR, CGI_KEYWORD_OPERATOR, CGI_MUX_SERVER_OP_PLAYLIST);
	
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_ACTION, CGI_ACTION, CGI_STR_MODIFY);
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"index\" id=\"index\" value=\"%d\">\n", i);

cgitrace();

	char *table = _html4PlayList(playlist);

	length += CGI_SPRINTF(buf, length, "%s\n", table);
	cmn_free(table);

	length += CGI_SPRINTF(buf, length, "<br><p align=\"center\">%s %s</p><br>", cgi_submit_button(gettext(CGI_STR_MODIFY)), cgi_reset_button(gettext(CGI_STR_RESET)) );
	
	cgi_info_page(gettext("Modify PlayList"), gettext("Configure and save one playlist"), buf );
	
	return 0;
}


int	muxServerPlaylist(MUX_SERVER_INFO *svrInfo)
{
	char 	buf[8192*5];
	int 		length = 0;
	int 		i, j;
	PLAY_LIST *playList = NULL;
	int res = EXIT_SUCCESS;
	char 	*errMsg = NULL;

	char *action = GET_CGI_ACTION( &svrInfo->cgiMux.cgiVariables);
	int count = cmn_list_size(&svrInfo->cgiMux.cgiMain.playlists);

	if( action && strlen(action))
	{
	 	if(!strcasecmp(action,CGI_ACTION_MODIFY))
		{
			playList = checkFieldsInPlaylist(svrInfo);

			if(playList)
			{
				int index = atoi(GET_VALUE(&svrInfo->cgiMux.cgiVariables, "index"));
				cgi_refresh_page(2, MUXSERVER_HLINK_PLAYLIST, "Saving", "Playlist is modifying playlist, please wait for a momont.....");
				res = cmnMuxPlayListUpdate(&svrInfo->cgiMux.cgiMain.playlists, index, playList);
				if(res == EXIT_FAILURE)
				{
					errMsg = "Update playlist failed, maybe name of playlist is collided";
				}
				return res;
			}
			else
			{
				return _muxServerPlaylistModify(svrInfo);
			}
		}
	 	else if(!strcasecmp(action,CGI_ACTION_ADD))
		{
			playList = checkFieldsInPlaylist(svrInfo);

			if(playList)
			{
				cgi_refresh_page(2, MUXSERVER_HLINK_PLAYLIST, "Saving", "Playlist is saving new playlist, please wait for a momont.....");
				res = cmnMuxPlayListAdd(&svrInfo->cgiMux.cgiMain.playlists, playList);
				if(res == EXIT_FAILURE)
				{
					errMsg = "Save new playlist failed, maybe name of playlist is collided";
				}
				else
					return res;
			}
			else
			{
				return _muxServerPlaylistNew(svrInfo);
			}
		}
	 	else if(!strcasecmp(action,CGI_ACTION_DEL))
		{
			char *playlistName = GET_VALUE(&svrInfo->cgiMux.cgiVariables, "playlist");

			if(strlen(playlistName))
			{
				cgi_refresh_page(2, MUXSERVER_HLINK_PLAYLIST, "Deleting", "Playlist is being deleted, please wait for a momont.....");
				res = cmnMuxPlayListRemove(&svrInfo->cgiMux.cgiMain.playlists, playlistName);
				if(res == EXIT_FAILURE)
				{
					errMsg = "Delete playlist failed, maybe name of playlist is not existed";
				}
				else
					return res;
			}
		}
	 	else if(!strcasecmp(action, "deletefile"))
		{
			char *playlistName = GET_VALUE(&svrInfo->cgiMux.cgiVariables, "playlist");
			char *fileName = GET_VALUE(&svrInfo->cgiMux.cgiVariables, "file");

			if(strlen(playlistName) && strlen(fileName))
			{
				cgi_refresh_page(2, MUXSERVER_HLINK_PLAYLIST, "Deleting", "One Media file in Playlist is being deleted, please wait for a momont.....");
				res = cmnMuxPlayListFileRemove(&svrInfo->cgiMux.cgiMain.playlists, playlistName, fileName);
				if(res == EXIT_FAILURE)
				{
					errMsg = "Delete media file from playlist failed, maybe name of playlist or media file is not existed";
				}
				else
					return res;
			}

			return _muxServerPlaylistModify(svrInfo);
		}
	 	else if(!strcasecmp(action,CGI_ACTION_SELECT))
		{
			char *playlistName = GET_VALUE(&svrInfo->cgiMux.cgiVariables, "playlist");
			cJSON *result = muxApiPlayMediaPlay(0, playlistName, 1);
			if( (result==NULL) ||  (muxApiGetStatus(result) != IPCMD_ERR_NOERROR) )
			{
				errMsg = "Playlist playing failed, retry.....";
			}

		}
		else
		{
			return cgi_error_page(MUXSERVER_HLINK_PLAYLIST, gettext(CGI_STR_ERROR), "Invalidate command");
		}
	}
	
	length += CGI_SPRINTF(buf, length, "<form id=\"columnarForm\" method=\"post\" action=\"%s\">\n", WEB_URL_MUX_SERVER );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_KEYWORD_OPERATOR, CGI_KEYWORD_OPERATOR, CGI_MUX_SERVER_OP_PLAYLIST);

	if(errMsg)
	{
		length += CGI_SPRINTF(buf, length, "<br><font color=\"0xFF0000\">%s</font><br>\n", errMsg);
	}

	for(i=0; i< count; i++)
	{
		char targetModify[256], targetDelete[256], targetPlay[256];
		PLAY_LIST *playlist = cmn_list_get(&svrInfo->cgiMux.cgiMain.playlists, i);

		length += CGI_SPRINTF(buf,length, "<table width=\"85%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");

		snprintf(targetModify, sizeof(targetModify), MUXSERVER_HLINK_PLAYLIST_MODIFY"&playlist=%s", playlist->name );
		snprintf(targetDelete, sizeof(targetDelete), MUXSERVER_HLINK_PLAYLIST_DELETE"&playlist=%s", playlist->name );
		snprintf(targetPlay, sizeof(targetPlay), MUXSERVER_HLINK_PLAYLIST_SELECT"&playlist=%s", playlist->name );

		length += CGI_SPRINTF(buf, length, "<TR><TD align=\"center\" bgcolor=\"#cccccc\"><strong>PlayList No.%d</strong></TD>\n", i+1 );

		length += CGI_SPRINTF(buf, length, 	"<TD align=\"center\" ><strong>%s</strong></TD><TD align=\"center\" colspan=\"2\">%s</TD>\n"
			"<TD align=\"center\" >%s</TD><TD align=\"center\" >%s</TD>\n</TR>\n", 
			playlist->name, cgi_button(gettext(CGI_STR_DELETE), targetDelete), cgi_button(gettext(CGI_STR_MODIFY), targetModify), cgi_button(gettext(CGI_STR_PLAY), targetPlay) );

		length += CGI_SPRINTF(buf, length, "<TR>\n\t"
			"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD><TD align=\"center\" ><strong>hh:%d;mm:%d/%s</strong></TD>\n"
			"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD><TD align=\"center\" ><strong>%d</strong></TD>"
			"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD><TD align=\"center\" ><strong>%s</strong></TD>\n</TR>\n", 
			gettext("Play Time"), playlist->hour, playlist->minute, days[playlist->dayOfWeek-1], gettext("Repeat"), playlist->repeat, gettext(CGI_STR_ENABLE), (playlist->enable==0)?"No":"Yes" );

		for(j = 0; j< cmn_list_size(&playlist->fileList); j++)
		{
			char *filename = (char *)cmn_list_get(&playlist->fileList, j);
			length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>No.%d</strong></TD><TD align=\"center\" colspan=\"5\">%s</TD>\n\t\n</TR>\n", 
				j+1, filename );
		}

		length += CGI_SPRINTF(buf, length, "</TABLE>\n<p></p>");

	}


	length += CGI_SPRINTF(buf, length, "<br><p align=\"center\">%s</p><br>", cgi_button(gettext(CGI_STR_ADD), MUXSERVER_HLINK_PLAYLIST_ADD) );
	
	cgi_info_page(gettext("PlayList of local media files"), gettext("Display all playlist and configure the playlist"), buf );
	
	return 0;
}


