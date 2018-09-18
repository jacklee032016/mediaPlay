
#include "_cgiMux.h"

static char *_cgiSvrConfigPage(MUX_SERVER_INFO *svrInfo , cJSON *objs)
{
	char 	buf[8192*50];
	int 		length = 0;

	length += CGI_SPRINTF(buf,length, "<table width=\"35%%\" border=\"1\" align=\"left\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>HTTP Address</strong></TD>\n\t<TD align=\"center\" >%s</TD>\n"
		"</TR><TR><TD align=\"center\" bgcolor=\"#cccccc\"><strong>HTTP Port</strong></TD>\n\t<TD align=\"center\">%d</TD>\n"
		"</TR>\n", cmnGetStrFromJsonObject(objs, "HttpAddress"), cmnGetIntegerFromJsonObject(objs, "HttpPort") );
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>RTSP Address</strong></TD>\n\t<TD align=\"center\" >%s</TD>\n"
		"</TR><TR><TD align=\"center\" bgcolor=\"#cccccc\"><strong>RTSP Port</strong></TD>\n\t<TD align=\"center\">%d</TD>\n"
		"</TR>\n", cmnGetStrFromJsonObject(objs, "RtspAddress"), cmnGetIntegerFromJsonObject(objs, "RtspPort") );
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>Max Clients</strong></TD>\n\t<TD align=\"center\" >%d</TD>\n"
		"</TR><TR><TD align=\"center\" bgcolor=\"#cccccc\"><strong>Max HTTP Connections</strong></TD>\n\t<TD align=\"center\">%d</TD>\n"
		"</TR>\n", cmnGetIntegerFromJsonObject(objs, "MaxClients"), cmnGetIntegerFromJsonObject(objs, "MaxHttpConns") );
	length += CGI_SPRINTF(buf, length, "</TABLE><br>\n");

	return strdup(buf);
}



static char *_cgiSvrConnsInfoPage(MUX_SERVER_INFO *svrInfo , cJSON *objs)
{
	char 	buf[8192*50];
	int 		length = 0;
	int i, count =0;
	cJSON *conns;

	length += CGI_SPRINTF(buf,length, "<table width=\"35%%\" border=\"1\" align=\"left\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>Current Connnections</strong></TD>\n\t<TD align=\"center\" >%d</TD>\n"
		"</TR><TR><TD align=\"center\" bgcolor=\"#cccccc\"><strong>Total Bandwidth</strong></TD>\n\t<TD align=\"center\">%d</TD>\n"
		"</TR>\n", cmnGetIntegerFromJsonObject(objs, "conns"), cmnGetIntegerFromJsonObject(objs, "bandwidth(KB)") );
	length += CGI_SPRINTF(buf, length, "</TABLE><br>\n");


	conns = cJSON_GetObjectItem(objs, "services");
	count = cJSON_GetArraySize(conns);
	length += CGI_SPRINTF(buf,length, "<table width=\"65%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"</TR>\n", gettext("No."), gettext("URL"),  gettext("Protocol"), gettext("Status"));

	for(i=0; i<count; i++)	
	{
		cJSON *obj = cJSON_GetArrayItem(conns, i);
		
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%d</TD>\n\t<TD align=\"center\">%s</TD>\n", i+1, cmnGetStrFromJsonObject(obj, "name"));
		length += CGI_SPRINTF(buf, length, "<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n", cmnGetStrFromJsonObject(obj, "protocol"),
			cmnGetStrFromJsonObject(obj, "status"));
	}


	length += CGI_SPRINTF(buf, length, "</TABLE><br>\n");

	return strdup(buf);
}


static char *_cgiSvrStreamsInfoPage(MUX_SERVER_INFO *svrInfo , cJSON *objsArray)
{
	char 	buf[8192*50];
	int 		length = 0;
	int i, count;

	count = cJSON_GetArraySize(objsArray);

	length += CGI_SPRINTF(buf,length, "<table width=\"95%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	length += CGI_SPRINTF(buf, length, "<TR>\n\t"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
		"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t\n"
		"</TR>\n", gettext("No."), gettext("URL"),  gettext("Format"), gettext("Media"), gettext("KBPS"), gettext("Serviced Conns"), gettext("Serviced Bytes") );
	
	for(i=0; i<count; i++)	
	{
		cJSON *obj = cJSON_GetArrayItem(objsArray, i);
		
		length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%d</TD>\n\t<TD align=\"center\">%s</TD>\n", i+1, cmnGetStrFromJsonObject(obj, "URL"));
		length += CGI_SPRINTF(buf, length, "<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n", cmnGetStrFromJsonObject(obj, "format"),
			cmnGetStrFromJsonObject(obj, "media"));
		length += CGI_SPRINTF(buf, length, "<TD align=\"center\">%d</TD>\n\t<TD align=\"center\">%d</TD><TD align=\"center\">%d</TD>\n\n", 
			cmnGetIntegerFromJsonObject(obj, "bandwidth"), cmnGetIntegerFromJsonObject(obj, "servicedConns"), cmnGetIntegerFromJsonObject(obj, "servicedBytes") );
	}


	length += CGI_SPRINTF(buf, length, "</TABLE><br>\n");

	return strdup(buf);
}


static char *_cgiSvrFeedsInfoPage(MUX_SERVER_INFO *svrInfo , cJSON *objs)
{
#define	WITH_MULTIPLE_TABLE		0
	char 	buf[8192*50];
	int 		length = 0;
	int i, j, count, streamCount;
	cJSON *streams=NULL, *feed = NULL;

	count = cJSON_GetArraySize(objs);
	
#if WITH_MULTIPLE_TABLE
#else
	length += CGI_SPRINTF(buf,length, "<table width=\"95%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
#endif

	for(i =0 ; i< count; i++)
	{
		feed = cJSON_GetArrayItem(objs, i);
		
#if WITH_MULTIPLE_TABLE
		length += CGI_SPRINTF(buf,length, "<table width=\"95%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
#endif
		length += CGI_SPRINTF(buf, length, "<TR>\n\t"
			"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>Name</strong></TD><TD align=\"center\">%s</TD><TD align=\"center\" bgcolor=\"#cccccc\"><strong>Type</strong></TD><TD align=\"center\">%s</TD>\n"
			"</TR>\n", cmnGetStrFromJsonObject(feed, "name"), cmnGetStrFromJsonObject(feed, "type") );
		length += CGI_SPRINTF(buf, length, "<TR>\n\t"
			"<TD align=\"center\" bgcolor=\"#cccccc\" ><strong>Media</strong></TD><TD colspan=3>%s</TD>\n\t\n""</TR>\n", cmnGetStrFromJsonObject(feed, "media") );

		length += CGI_SPRINTF(buf, length, "<TR>\n\t"
			"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>Duration(second)</strong></TD><TD align=\"center\">%d</TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>Bandwidth(kbps)</strong></TD><TD align=\"center\">%d</TD>\n"
			"</TR>\n", cmnGetIntegerFromJsonObject(feed, "duration(seond)"), cmnGetIntegerFromJsonObject(feed, "bandwidth(Kbps)") );
#if WITH_MULTIPLE_TABLE
		length += CGI_SPRINTF(buf, length, "</TABLE>\n");
#else
		length += CGI_SPRINTF(buf, length, "<TR></TR><br>\n");
#endif

		streams = cJSON_GetObjectItem(feed, "streams");

		if( streams && cJSON_IsArray(streams) )
		{
			streamCount = cJSON_GetArraySize(streams);

#if WITH_MULTIPLE_TABLE
			length += CGI_SPRINTF(buf,length, "<table width=\"95%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
#endif
			length += CGI_SPRINTF(buf, length, "<TR>\n\t"
				"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
				"<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n"
				"</TR>\n", gettext("No."), gettext("Type"), gettext("Codec"), gettext("Parameters") );
			
			for(j=0; j<streamCount; j++)	
			{
				cJSON *st = cJSON_GetArrayItem(streams, j);
				
				length += CGI_SPRINTF(buf, length, "<TR>\n\t<TD align=\"center\">%d</TD>\n\t<TD align=\"center\">%s</TD>\n", j+1, cmnGetStrFromJsonObject(st, "type"));
				length += CGI_SPRINTF(buf, length, "<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n", cmnGetStrFromJsonObject(st, "codec"), 	cmnGetStrFromJsonObject(st, "params"));
			}
			
#if WITH_MULTIPLE_TABLE
			length += CGI_SPRINTF(buf, length, "</TABLE><br><br>\n");
#endif

		}
	}
	
#if WITH_MULTIPLE_TABLE
#else
	length += CGI_SPRINTF(buf, length, "</TABLE><br><br>\n");
#endif

	return strdup(buf);
}



/*
int	muxCgiSvrRuntime(MUX_SERVER_INFO *svrInfo)
{
	return cgiRedirect("192.168.168.102:9090/index.html");
}
*/


int	muxCgiSvrRuntime(MUX_SERVER_INFO *svrInfo)
{
	char *action = GET_CGI_ACTION( &svrInfo->cgiMux.cgiVariables);
	cJSON *resObj = NULL;

	if( !action || !strlen(action) || !strcasecmp(action, IPCMD_NAME_SERVER_FEEDS) )
	{
		svrInfo->cgiMux.response = muxApiServerFeeds();
		if((svrInfo->cgiMux.response == NULL) || (muxApiGetDataReply(svrInfo->cgiMux.response, 1)==NULL))
		{
			cgi_error_page(NULL, gettext("Response from server is invalidate!"), gettext( CGI_STR_SOME_ERROR ) );
			return 1;
		}
		
		resObj = muxApiGetDataReply(svrInfo->cgiMux.response, 1);
		return cgi_info_page(gettext("Configuration of Media Sources"), gettext("Current configuration of media sources"), _cgiSvrFeedsInfoPage(svrInfo, resObj)  );
	}
	else if( !strcasecmp(action, IPCMD_NAME_SERVER_CONNS ) )
	{
		svrInfo->cgiMux.response = muxApiServerConns();
		if((svrInfo->cgiMux.response == NULL) || (muxApiGetDataReply(svrInfo->cgiMux.response, 1)==NULL))
		{
			cgi_error_page(NULL, gettext("Response from server is invalidate!"), gettext( CGI_STR_SOME_ERROR ) );
			return 1;
		}
		
		resObj = muxApiGetDataReply(svrInfo->cgiMux.response, 0);
		return cgi_info_page(gettext("Current Connections"), gettext("Current connections in servicing"), _cgiSvrConnsInfoPage(svrInfo, resObj)  );
	}
	else if( !strcasecmp(action, IPCMD_NAME_SERVER_CONFIG ) )
	{
		svrInfo->cgiMux.response = muxApiServerConfig();
		if((svrInfo->cgiMux.response == NULL) || (muxApiGetDataReply(svrInfo->cgiMux.response, 1)==NULL))
		{
			cgi_error_page(NULL, gettext("Response from server is invalidate!"), gettext( CGI_STR_SOME_ERROR ) );
			return 1;
		}
		
		resObj = muxApiGetDataReply(svrInfo->cgiMux.response, 0);
		return cgi_info_page(gettext("Service Configuration"), gettext("Current service configuration"), _cgiSvrConfigPage(svrInfo, resObj)  );
	}
	else if( !strcasecmp(action, IPCMD_NAME_SERVER_URLS ) )
	{
		svrInfo->cgiMux.response = muxApiServerUrls();
		if((svrInfo->cgiMux.response == NULL) || (muxApiGetDataReply(svrInfo->cgiMux.response, 1)==NULL))
		{
			cgi_error_page(NULL, gettext("Response from server is invalidate!"), gettext( CGI_STR_SOME_ERROR ) );
			return 1;
		}
		
		resObj = muxApiGetDataReply(svrInfo->cgiMux.response, 1);
		cgi_info_page(gettext("Configuration of URLs of Streams"), gettext("Current configuration of streams"), _cgiSvrStreamsInfoPage(svrInfo, resObj)  );
		return 0;	
	}
	else
	{
		cgi_invalidate_page();
		return 0;
	}
	
	return 0;	
}


