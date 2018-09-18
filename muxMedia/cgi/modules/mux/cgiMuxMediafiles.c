/*
* $Id: servicesMiscMain.c,v 1.7 2007/09/06 09:19:59 lizhijie Exp $
*/

#include "_cgiMux.h"

#include <unistd.h>


#define JAVASCRIPTS_MEDIA_FILE		"<script> function submitForm2(value){ "\
        "window.alert(value); document.getElementById('mediaName').value = value;" \
        "document.getElementById('fileSelectForm').submit(); } </script>"

char *submitButton2(char *name, char *actionName)
{
	char buf[1024];
	CGI_SPRINTF(buf, 0, 
		"<input type=\"button\" name=\"%s\" value=\"%s\" onclick=\"submitForm2('%s')\" class=\"submit_button\" />",
		SUBMIT, name, actionName);
	return strdup(buf);
	
}



static char *_selectionWindowOptions(MUX_SERVER_INFO *svrInfo, char *filename)
{
	char buf[1024];
	int	length = 0;
	RECT_CONFIG *cfg;
	int	i = 0;
	
	length += CGI_SPRINTF(buf, length, "\n\t<select name=\"%s.%s\" style='width:160px;'>\n",  MUX_KEYWORD_WINDOW_INDEX, filename );

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



char *_mediafile_list_html(MUX_SERVER_INFO *svrInfo )
{
	char 	buf[8192*5];
	int 		length = 0;
	char		target[1024];
	char		targetDelete[1024];
	int		i;

//	cmnMediaScanInit(TRUE, &svrInfo->cgiMux.cgiCfgPlay);
	cmn_list_t *dest = cmnMediaScanGetFiles();
	

	length += CGI_SPRINTF(buf, length, "<form id=\"fileSelectForm\" method=\"post\" action=\"%s\">\n", WEB_URL_MUX_SERVER );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_KEYWORD_OPERATOR, CGI_KEYWORD_OPERATOR, CGI_MUX_SERVER_OP_MEDIAFILES);
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_ACTION, CGI_ACTION, CGI_ACTION_SELECT);

	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_MEDIA_KEYWORK_MDIA_NAME, CGI_MEDIA_KEYWORK_MDIA_NAME, "");
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", "windowIndex", "windowIndex", "");
	
	length += CGI_SPRINTF(buf, length,  JAVASCRIPTS_MEDIA_FILE);

/* table 1 */	
	length += CGI_SPRINTF(buf,length, "<table width=\"95%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD><TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD><TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"</TR>\n", 
		gettext("Name"), gettext("Path"), gettext("Format"), gettext("Duration"), gettext("Video"), gettext("Audio"), gettext("Subtitle"), gettext(CGI_STR_PLAY) , gettext(CGI_STR_DELETE));

	for(i=0; i< cmn_list_size(dest); i++)
	{
		MEDIA_FILE_T *file = (MEDIA_FILE_T *)cmn_list_get(dest, i);
		char filename[2048];
		snprintf(filename, sizeof(filename), "%s/%s", file->path, file->name);
		
		snprintf(target, sizeof(target), MUXSERVER_HLINK_MEDIAFILE_PLAY"=%s", filename );
		snprintf(targetDelete, sizeof(targetDelete), MUXSERVER_HLINK_MEDIAFILE_DELETE"=%s", filename );

		length += CGI_SPRINTF(buf, length, "<TR>\n\t"
			"<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n"
			"<TD align=\"center\">%d</TD>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n"
			"<TD align=\"center\">%s</TD><TD align=\"center\">%s%s</TD><TD align=\"center\">%s</TD>\n\t"
			"</TR>\n", 
			file->name, file->path, file->formatName, file->duration, file->hasVideo==0?"No":"Yes", file->hasAudio==0?"No":"Yes", file->hasSubtitle==0?"No":"Yes",
			_selectionWindowOptions(svrInfo, filename),  submitButton2(gettext(CGI_STR_PLAY), filename),
			cgi_button(gettext(CGI_STR_DELETE), targetDelete));
//			cgi_button(gettext(CGI_STR_PLAY), target), cgi_button(gettext(CGI_STR_DELETE), targetDelete));

	}
	
//	length += CGI_SPRINTF(buf,length, "<TR><TD align=\"center\" colspan=3>%s\n</TD></TR>\r\n",cgi_help_button(CGI_HELP_MSG_SYSTEM_PROC) );

	length += CGI_SPRINTF(buf, length, "</TABLE>\n");

	
//	MUX_DEBUG("%s\n", buf);
	
	return strdup(buf);
}



static int	muxServerFilePlay(MUX_SERVER_INFO *svrInfo)
{
	int windowIndex = 0;
	char	winKeyword[256];

	char		*filename;

	filename = GET_VALUE(&svrInfo->cgiMux.cgiVariables, CGI_MEDIA_KEYWORK_MDIA_NAME) ;
	
	if(filename== NULL || strlen(filename)==0)
	{
		return cgi_error_page(NULL, gettext("URL is null, please try again!"), gettext( CGI_STR_SOME_ERROR ) );
	}
	cgidebug("select window filename '%s' \n", filename);

	cgitrace();
	CGI_SPRINTF(winKeyword, 0, "%s.%s",MUX_KEYWORD_WINDOW_INDEX, filename);
	windowIndex = atoi(GET_VALUE(&svrInfo->cgiMux.cgiVariables, winKeyword) );

	cgitrace();
	return cgiMuxPlayUrl(filename, windowIndex, MUXSERVER_HLINK_MEDIAFILE, MUXSERVER_HLINK_PLAYER_INFO);
}


int	muxServerMediaFiles(MUX_SERVER_INFO *svrInfo)
{

	char *action = GET_CGI_ACTION( &svrInfo->cgiMux.cgiVariables);

	if( !action || !strlen(action) )
	{
		return cgi_info_page(gettext("Media Files"), gettext("Media files list as following:") , _mediafile_list_html(svrInfo ) );
	}
	else if( !strcasecmp(action, CGI_ACTION_SELECT) )
	{
		return muxServerFilePlay(svrInfo);
	}
	else	 if(!strcasecmp(action, CGI_ACTION_DEL) )
	{
		char *file = GET_VALUE(&svrInfo->cgiMux.cgiVariables, CGI_MEDIA_KEYWORK_MDIA_NAME );
		cgi_refresh_page(2, MUXSERVER_HLINK_MEDIAFILE, "Deleting", "Media file is deleting now, please wait for a momont.....");
		if(unlink(file))
		{
			fprintf(stdout, "Delete file '%s' failed: %s\n",file, strerror(errno));
		}
	}
	else
	{
		cgi_invalidate_page();
		return 0;
	}
	
	return 0;	
}


