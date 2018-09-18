
#include "_cgiMux.h"

int	muxServerWindow(MUX_SERVER_INFO *svrInfo)
{
	char 	buf[8192*5];
	int 		length = 0;
	int 		i;

#if CGI_OPTIONS_WINDOW_LIST
	char *action = GET_CGI_ACTION( &svrInfo->cgiMux.cgiVariables);
	int count = cmn_list_size(&svrInfo->cgiMux.cgiCfgPlay.windows);

	if( action )
	{
	 	if(!strcasecmp(action,MUX_WINDOW_KEYWORD_SWAP))
		{
			int index = atoi(GET_VALUE(&svrInfo->cgiMux.cgiVariables,"Index" ) );

			if(index <= 0 || index >= count)
			{
				length += CGI_SPRINTF(buf, length, "Error: Index %d is out of range of window", index );
			}
			else
			{
				cJSON *resObj = muxApiPlayMediaSwapWindow(index);
				if( (resObj==NULL) ||  (muxApiGetStatus(resObj) != IPCMD_ERR_NOERROR) )
					return 1;
			}
		}
	}
	length += CGI_SPRINTF(buf, length, "<form id=\"columnarForm\" method=\"post\" action=\"%s\">\n", WEB_URL_MUX_SERVER );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" id=\"%s\" value=\"%s\">\n", CGI_KEYWORD_OPERATOR, CGI_KEYWORD_OPERATOR, CGI_MUX_SERVER_OP_CONFIG);

cgitrace();

//	length += CGI_SPRINTF(buf, length,  JAVASCRIPTS);

/* table 1 : windows list */	
	length += CGI_SPRINTF(buf,length, "<table width=\"85%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\" colspan=\"5\"><strong>%s</strong></TD>\n\t</TD>\n</TR>\n", 	gettext("Window List") );
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD><TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD><TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n</TR>\n", 
		gettext("Window"), gettext("URL"), gettext("Position"), gettext("Rotate"), gettext("Action") );


	for(i = 0; i< count; i++)
	{
		char		target[1024];
		RECT_CONFIG *rect = (RECT_CONFIG *)cmn_list_get(&svrInfo->cgiMux.cgiCfgPlay.windows, i);
		
		snprintf(target, sizeof(target), "%s?%s=%s&%s=%s&Index=%d", WEB_URL_MUX_SERVER, CGI_KEYWORD_OPERATOR, CGI_MUX_SERVER_OP_WINDOW, CGI_ACTION, MUX_WINDOW_KEYWORD_SWAP, i);

		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n\t"
			"<TD align=\"center\">(%d,%d),(%d,%d)</TD><TD align=\"center\">%s</TD><TD align=\"center\">%s</TD>\n</TR>\n", rect->name, rect->url, 
			rect->left, rect->top, rect->width, rect->height, CMN_MUX_FIND_RORATE_NAME(rect->rotateType), 
			(rect->type != RECT_TYPE_MAIN)? cgi_button(gettext("swap"), target):"" );
	}

	length += CGI_SPRINTF(buf, length, "</TABLE><br><br>\n");


	count = cmn_list_size(&svrInfo->cgiMux.cgiCfgPlay.osds);
/* table 1 : OSD list */	
	length += CGI_SPRINTF(buf,length, "<table width=\"85%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\" colspan=\"8\"><strong>%s</strong></TD>\n\t</TD>\n</TR>\n", 	gettext("OSD List") );
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD><TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD><TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD><TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD><TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n</TR>\n", 
		gettext("OSD"), gettext("URL"), gettext("Position"), gettext("Font Size"), gettext("Font Color"), gettext("Background"), gettext("Alpha"), gettext("Enabled") );

	for(i = 0; i< count; i++)
	{
		RECT_CONFIG *rect = (RECT_CONFIG *)cmn_list_get(&svrInfo->cgiMux.cgiCfgPlay.osds, i);
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD><TD align=\"center\">(%d,%d),(%d,%d)</TD>\n\t<TD align=\"center\">%d</TD>\n\t"
			"<TD align=\"center\">0X%x</TD><TD align=\"center\">0X%x</TD><TD align=\"center\">%d</TD><TD align=\"center\">%s</TD>\n</TR>\n", 
			rect->name,  (rect->type == RECT_TYPE_LOGO)? rect->url:"", rect->left, rect->top, rect->width, rect->height, (rect->type != RECT_TYPE_LOGO)?rect->fontSize:0, 
			 (rect->type != RECT_TYPE_LOGO)?rect->fontColor:0,  (rect->type != RECT_TYPE_LOGO)?rect->backgroundColor:0, rect->alpha, (rect->enable==0)?"No":"Yes"  );
	}

#endif

	length += CGI_SPRINTF(buf, length, "</TABLE>\n");

	cgi_info_page(gettext("Window/OSD List"), gettext("Windows/OSDs and their attributes"), buf );
	
	return 0;
}


