

#include "muxWeb.h"

#include "libMedia.h"
#include "libMux.h"


static int _muxWebJSonInfos(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jEvent)
{
	SVR_WEB  *webSvr = (SVR_WEB *)priv;
	cJSON *newObject = NULL;
	cJSON *array = NULL;
	
	if( webSvr == NULL )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Internal error: without data of WEB");
		goto endl;
	}

	array = cJSON_CreateArray();
	dataConn->resultObject = array;

	newObject = cJSON_CreateObject();
	cJSON_AddItemToArray(array, newObject);


//	JEVENT_ADD_STRING(newObject, "name", IS_STRING_NULL(webSvr->cfg.server_name)?"None":webSvr->cfg.server_name);
	JEVENT_ADD_INTEGER(newObject, "port", webSvr->cfg.server_port);
	
	JEVENT_ADD_STRING(newObject, "CGI", webSvr->cfg.cgi_path);
	JEVENT_ADD_STRING(newObject, "root", webSvr->cfg.document_root);
	JEVENT_ADD_STRING(newObject, "AccessLog", webSvr->cfg.access_log_name);
	
	dataConn->errCode = IPCMD_ERR_NOERROR;
	
endl:	
	
	return EXIT_SUCCESS;
}


PluginJSonHandler muxWebJsonHandlers[] =
{
	{
		.type	= CMD_TYPE_WEB_INFOS,
		.name 	= "infos",
		handler	: _muxWebJSonInfos
	},
	{
		.type	= CMD_TYPE_UNKNOWN,
		.name	= NULL,
		.handler	= NULL
	}
};


