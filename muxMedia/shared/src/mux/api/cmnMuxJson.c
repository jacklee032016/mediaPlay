

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "_cmnMux.h"


#define KEY_LEN	(20)
#define VALUE_LEN (1024)

typedef struct _HASH_MAP
{
	const char key[KEY_LEN];
	char value[VALUE_LEN];
}HASH_MAP;


typedef enum
{
	msg_targ,
	msg_cmd,
	msg_login_ack,
	msg_pwd_msg,
	msg_data,
	msg_login,
	msg_password,
	msg_pName,
	msg_model,
	msg_fwVer,
	msg_cName,
	msg_mac,
	msg_ip,
	msg_mask,
	msg_gateway,
	msg_isDhcp,
	msg_isDipOn,
	msg_isReset,
	msg_isReboot,
	msg_IRState,
	msg_IRFeedbackIp,
	msg_RS232Baudrate,
	msg_RS232Databits,
	msg_RS232Parity,
	msg_RS232Stopbits,
	msg_RS232FeedbackIp,
	msg_isTS_1,
	msg_TSurl_1,
	msg_isHLS_1,
	msg_HLSurl_1,
	msg_isFLV_1,
	msg_FLVurl_1,
	msg_HTTPport_1,
	msg_isRTSP_1,
	msg_RTSPurl_1,
	msg_RTSPport_1,
	msg_isRTSPph_1,
	msg_isRTMP_1,
	msg_isRTMPph_1,
	msg_RTMPurl_1,
	msg_RTMPport_1,
	msg_isMCAST_1,
	msg_MCASTip_1,
	msg_MCASTport_1,
	msg_MCASTurl_1,
	msg_SourceIp_1,
	msg_VidResI_1,
	msg_VidFpsI_1,
	msg_VidResO_1,
	msg_VidFpsO_1,
	msg_VidBitR_1,
	msg_VidCodec_1,
	msg_AudFrq_1,
	msg_AudBitR_1,
	msg_AudCodec_1,
	msg_num
} RSV_MSG;

#define VER	"1.1.4"

const HASH_MAP _defaultStates[msg_num] =
{
		{ "targ", "" },
		{ "cmd", "set_param" },
		{ "login_ack", "" },
		{ "pwd_msg", "" },
		{ IPCMD_NAME_KEYWORD_DATA, "" },
		{ "login", "" },
		{ "password", "" },
		{ "pName", "AV Over IP H.264 H.265" },
		{ "model", "RX-500762" },
		{ "fwVer", VER },
		{ "cName", "myRXname" },
		{ "mac", "" },
		{ "ip", "192.168.168.62" },
		{ "mask", "255.255.255.0" },
		{ "gateway", "192.168.168.1" },
		{ "isDhcp", "1" },
		{ "isDipOn", "0" },
		{ "isReset", "0" },
		{ "isReboot", "0" },
		{ "IRState", "0" },
		{ "IRFeedbackIp", "192.168.168.61" },
		{ "RS232Baudrate", "9600" },
		{ "RS232Databits", "8" },
		{ "RS232Parity", "none" },
		{ "RS232Stopbits", "1" },
		{ "RS232FeedbackIp", "192.168.168.61" },
		{ "isTS_1", "0" },
		{ "TSurl_1", "http://192.168.168.61:80/video1.ts" },
		{ "isHLS_1", "0" },
		{ "HLSurl_1", "http://192.168.168.61:80/video1.m3u8" },
		{ "isFLV_1", "0" },
		{ "FLVurl_1", "http://192.168.168.61:80/video1.flv" },
		{ "HTTPport_1", "80" },
		{ "isRTSP_1", "0" },
		{ "RTSPurl_1", "rtsp://192.168.168.61:554/video1.rtsp" },
		{ "RTSPport_1", "38001" },
		{ "isRTSPph_1", "0" },
		{ "isRTMP_1", "0" },
		{ "isRTMPph_1", "0" },
		{ "RTMPurl_1", "" },
		{ "RTMPport_1", "1935" },
		{ "isMCAST_1", "1" },
		{ "MCASTip_1", "239.100.0.1" },
		{ "MCASTport_1", "37000" },
		{ "MCASTurl_1", "udp://@239.100.0.1:37000" },
		{ "SourceIp_1", "" },
		{ "VidResI_1", "" },
		{ "VidFpsI_1", "" },
		{ "VidResO_1", "" },
		{ "VidFpsO_1", "" },
		{ "VidBitR_1", "" },
		{ "VidCodec_1", "" },
		{ "AudFrq_1", "" },
		{ "AudBitR_1", "" },
		{ "AudCodec_1", "" },
};

const char DIP_SWITCH[16][15] = {
		"239.100.0.1",
		"239.100.0.2",
		"239.100.0.3",
		"239.100.0.4",
		"239.100.0.5",
		"239.100.0.6",
		"239.100.0.7",
		"239.100.0.8",
		"239.100.0.9",
		"239.100.0.10",
		"239.100.0.11",
		"239.100.0.12",
		"239.100.0.13",
		"239.100.0.14",
		"239.100.0.15",
		"239.100.0.16"
};



cJSON *cmnMuxCreateDefaultJsonObject(void)
{
	cJSON *json_root = cJSON_CreateObject();
	cJSON *item_array = cJSON_CreateArray();
	cJSON *parameter_pool = cJSON_CreateObject();

//	cJSON *value = cJSON_CreateObject();
	int i;
	for (i = 0; i < msg_data; i++)
	{
		cJSON_AddItemToObject(json_root, _defaultStates[i].key, cJSON_CreateString(_defaultStates[i].value));
	}

	cJSON_AddItemToObject(json_root, _defaultStates[msg_data].key, item_array);
	cJSON_AddItemToObject(item_array, _defaultStates[msg_data].key, parameter_pool);

	for (i = msg_login; i < msg_num; i++)
	{
		cJSON_AddItemToObject(parameter_pool, _defaultStates[i].key, cJSON_CreateString(_defaultStates[i].value));
		//printf("Add to cJSON: %s --- %s\n", _defaultStates[i].key, _defaultStates[i].value);
	}

	return json_root;
}

char* cmnGetStrFromJsonObject(cJSON* json, const char * key)
{
	cJSON * obj = cJSON_GetObjectItem(json, key);

	if(cJSON_IsString(obj) )
		return obj->valuestring;
	else
		return "";
}

int cmnGetIntegerFromJsonObject(cJSON* json, const char * key)
{
	cJSON * obj = cJSON_GetObjectItem(json, key);
	
	if(cJSON_IsNumber(obj) )
		return obj->valueint;
	else
		return -1;
}



/* cmd is command cJSON from client */
char *cmnMuxCreateErrReply(int errCode, cJSON *cmd)
{
	cJSON *errReply = cJSON_CreateObject();
	cJSON *arrayItem = cJSON_CreateArray();
	cJSON *params = cJSON_CreateObject();
	char *errStr = MUX_JSON_ERROR_STR(errCode);
	char* reply = NULL;

	if(cmd == NULL)
	{
		cJSON_AddItemToObject(errReply, _defaultStates[msg_targ].key, cJSON_CreateString("FF:FF:FF:FF:FF:FF"));
		cJSON_AddItemToObject(errReply, _defaultStates[msg_cmd].key, cJSON_CreateString("Unknown"));
	}
	else
	{
		cJSON_AddItemToObject(errReply, _defaultStates[msg_targ].key, cJSON_CreateString(cmnGetStrFromJsonObject(cmd, _defaultStates[msg_targ].key)));
		cJSON_AddItemToObject(errReply, _defaultStates[msg_cmd].key, cJSON_CreateString(cmnGetStrFromJsonObject(cmd, _defaultStates[msg_cmd].key)));
	}
	cJSON_AddItemToObject(errReply, _defaultStates[msg_login_ack].key, cJSON_CreateString("NOK"));
	cJSON_AddItemToObject(errReply, _defaultStates[msg_pwd_msg].key, cJSON_CreateString( (errStr!=NULL)?errStr:"Unknown Error" ));
	cJSON_AddItemToObject(errReply, _defaultStates[msg_data].key, arrayItem);
	
	cJSON_AddItemToArray(arrayItem, params);
	cJSON_AddItemToObject(params, MEDIA_CTRL_STATUS, cJSON_CreateNumber((double) errCode));

	//reply = cJSON_Print(errReply);
	reply = cJSON_PrintUnformatted(errReply);

	cJSON_Delete(errReply);

	return reply;
}


struct json_handler
{
	char		*name;
	
	/* which plugins or main should process this JSON object */
	MUX_PLUGIN_TYPE		dest;

	/* handler only operate errorStats, msg, resultObjs of dataConn object, never execute send operation.
	* return value has no meaning 
	*/
	int		(*handler)(MUX_PLUGIN_TYPE dest, struct DATA_CONN *, cJSON *data);
};

#if 0
static int	_jsonHandle4GetParam(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data) /* data is first item in data array */
{
	return -EXIT_SUCCESS;
}

static int	_jsonHandle4SetParam(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data) /* data is first item in data array */
{
	return -EXIT_SUCCESS;
}

static int	_jsonHandle4SendDataRs232(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data) /* data is first item in data array */
{
	return -EXIT_SUCCESS;
}

static int	_jsonHandle4SendDataIr(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data) /* data is first item in data array */
{
	return -EXIT_SUCCESS;
}
#endif


struct json_handler jsonActionHandlers[] =
{
#if 0
	{
		.name 	= IPCMD_NAME_GET_PARAM,
		.dest	= MUX_PLUGIN_TYPE_UNKNOWN,
		.handler	= _jsonHandle4GetParam
	},
	{
		.name 	= IPCMD_NAME_SET_PARAM,
		.dest	= MUX_PLUGIN_TYPE_UNKNOWN,
		.handler	= _jsonHandle4SetParam
	},
	{
		.name 	= IPCMD_NAME_SEND_RS232,
		.dest	= MUX_PLUGIN_TYPE_UNKNOWN,
		.handler	= _jsonHandle4SendDataRs232
	},
	{
		.name 	= IPCMD_NAME_SEND_IR,
		.dest	= MUX_PLUGIN_TYPE_UNKNOWN,
		.handler	= _jsonHandle4SendDataIr
	},
#endif	
	{
		.name 	= IPCMD_NAME_MEDIA_GET,
		.dest	= MUX_PLUGIN_TYPE_MAIN,
		.handler	= cmnMuxJsonHandle4GetMedia
	},
	{
		.name 	= IPCMD_NAME_MEDIA_SET,
		.dest	= MUX_PLUGIN_TYPE_MAIN,
		.handler	= cmnMuxJsonHandle4SetMedia
	},
	{
		.name 	= IPCMD_NAME_MEDIA_PLAY,
		.dest	= MUX_PLUGIN_TYPE_PLAYER,
		.handler	= cmnMuxJsonHandle4Plugin
	},
	{
		.name 	= IPCMD_NAME_MEDIA_RECORDER,
		.dest	= MUX_PLUGIN_TYPE_RECORDER,
		.handler	= cmnMuxJsonHandle4Plugin
	},
	{
		.name 	= IPCMD_NAME_MEDIA_SERVER,
		.dest	= MUX_PLUGIN_TYPE_SERVER,
		.handler	= cmnMuxJsonHandle4Plugin
	},
	{
		.name 	= IPCMD_NAME_MEDIA_WEB,
		.dest	= MUX_PLUGIN_TYPE_WEB,
		.handler	= cmnMuxJsonHandle4Plugin
	},

	{
		.name 	= IPCMD_NAME_SYS_ADMIN,
		.dest	= MUX_PLUGIN_TYPE_MAIN,
		.handler	= cmnMuxJsonHandle4SysAdmin
	},

	{
		.name 	= NULL,
		.dest	= MUX_PLUGIN_TYPE_UNKNOWN,
		.handler	= NULL
	}
};


#if 0
/* get configuration for current command */
cJSON *cmnMuxGetCurrentCfg(struct DATA_CONN *dataConn)
{
	int i;
	
	cJSON *cmdObj = cJSON_GetObjectItem(dataConn->cmdObjs, _defaultStates[msg_cmd].key);
	if(cmdObj == NULL)
	{
		MUX_ERROR("No cmd child object is found");
		return NULL;
	}

	MUX_DEBUG("cmd is '%s'", cmdObj->valuestring);

	for (i = 0; i < cJSON_GetArraySize(dataConn->ctrlConn->controller->cfgHandlers); i++)
	{
		cJSON *subitem = cJSON_GetArrayItem(dataConn->ctrlConn->controller->cfgHandlers, i);
		if(!subitem)
		{
			MUX_ERROR("IP Command configuration file '%s' initialization error", IP_COMMAND_CONFIG_FILE);
			return NULL;
		}
		
		cJSON *temp = cJSON_GetObjectItem(subitem, _defaultStates[msg_cmd].key);
		if( !strcasecmp(cmdObj->valuestring, temp->valuestring) )
		{
			return temp;
		}
	}

	MUX_ERROR("No command configuration for command of '%s'", cmdObj->valuestring);
	return NULL;
}
#endif

cJSON *cmnMuxJsonLoadConfiguration(char *cfgFileName)
{
	cJSON *cfgHandlers = NULL;

	char *jsonStr = cmn_read_file(cfgFileName);
	if ((jsonStr == NULL) )
	{
		MUX_ERROR("IP Command configuration file '%s' reading failed", cfgFileName);
		return NULL;
	}
	
	cfgHandlers = cJSON_Parse(jsonStr);
	if (cfgHandlers== NULL)
	{
		cmn_free(jsonStr);
		MUX_ERROR("IP Command configuration file '%s' Parsing failed", cfgFileName);
		return NULL;
	}

	cmn_free(jsonStr);

	return cfgHandlers;
}



/* Reply JSON object in any threads (context of controller and other threads) and 
* all cases: OK or error after DATA_CONN->cmdObjs has been parsed in _read_command 
*/
static int	_cmnMuxJsonReply4All(struct DATA_CONN *dataConn)
{
	int res = EXIT_SUCCESS;
	cJSON *dataArray, *dataItem;

	if(dataConn->errCode == IPCMD_ERR_NOERROR)
	{
		cJSON_ReplaceItemInObject(dataConn->cmdObjs, _defaultStates[msg_login_ack].key, cJSON_CreateString("OK"));
		cJSON_ReplaceItemInObject(dataConn->cmdObjs, _defaultStates[msg_pwd_msg].key, cJSON_CreateString("OK"));
	}
	else
	{
		char *errMsg = MUX_JSON_ERROR_STR(dataConn->errCode);
		cJSON_ReplaceItemInObject(dataConn->cmdObjs, _defaultStates[msg_login_ack].key, cJSON_CreateString("NOK"));
		cJSON_ReplaceItemInObject(dataConn->cmdObjs, _defaultStates[msg_pwd_msg].key, cJSON_CreateString(errMsg));
	}
	
	dataArray = cJSON_GetObjectItem(dataConn->cmdObjs, _defaultStates[msg_data].key);
	if(!cJSON_IsArray(dataArray))
	{
	}
	
	dataItem = cJSON_GetArrayItem(dataArray, 0);

	if( (dataConn->errCode == IPCMD_ERR_NOERROR ||dataConn->errCode == IPCMD_ERR_FTP_PARTLY_FAILED) && dataConn->resultObject != NULL )
	{
		cJSON_ReplaceItemInObjectCaseSensitive(dataItem, MEDIA_CTRL_OBJECTS, dataConn->resultObject);
	}
	
	cJSON_AddItemToObject(dataItem, MEDIA_CTRL_STATUS, cJSON_CreateNumber((double) dataConn->errCode));
	if(!IS_STRING_NULL(dataConn->detailedMsg) )
	{
		cJSON_AddItemToObject(dataItem, MEDIA_CTRL_STATUS_MSG, cJSON_CreateString( dataConn->detailedMsg) );
	}

	char *msg = cJSON_PrintUnformatted(dataConn->cmdObjs);

	res = cmnMuxCtrlResponse(dataConn, msg, strlen(msg));
	cmn_free(msg);

	dataConn->isFinished = TRUE;
	return res;
}

/* reply error or OK message in context of controller and other threads */
int cmnMuxJsonControllerReply(struct DATA_CONN *dataConn, int status, const char *fmt, ... )
{
	va_list vargs;
	va_start(vargs, fmt);

	dataConn->errCode = status;

	vsnprintf(dataConn->detailedMsg, CMN_NAME_LENGTH,  fmt, vargs);
	
	MUX_ERROR("JSON handler error: %s", dataConn->detailedMsg);

	va_end(vargs);

	return EXIT_SUCCESS;
}

/* reply with JEVENT in context of FSM and plugin */
int cmnMuxJEventReply(CMN_PLAY_JSON_EVENT *jsonEvent, int errCode, const char *fmt, ...)
{
	va_list vargs;

	struct DATA_CONN *dataConn = (struct DATA_CONN *) jsonEvent->priv;
	if(!dataConn)
	{
		CMN_ABORT("DATA_CONN is null");
	}
	dataConn->errCode = errCode;

	va_start(vargs, fmt);
	vsnprintf(dataConn->detailedMsg, CMN_NAME_LENGTH,  fmt, vargs);
	MUX_ERROR("JEVENT handler error: %s", dataConn->detailedMsg);
	va_end(vargs);

	return cmnMuxJsonPluginReplay(dataConn, jsonEvent);
}


/*
 * reply the JSON message to peer for sync plugin or async FSM, eg. in context of threads other than controller  
*/
int	cmnMuxJsonPluginReplay(struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	cmn_mutex_lock(dataConn->mutexLock);

	if(dataConn->errCode != IPCMD_ERR_IN_PROCESSING)
	{/* otherwise, wait the end of processing in FSM */
		if(CONTROLLER_IS_DEBUG(dataConn) )
		{
			MUX_DEBUG("API engine is replying '%s' action", jsonEvent->action );
		}
		_cmnMuxJsonReply4All(dataConn);
		
		cmn_mutex_unlock(dataConn->mutexLock);

		cmnMuxDataConnClose(dataConn);

#if MUX_OPTIONS_DEBUG_IP_COMMAND			
		MUX_DEBUG("free jsonEvent:%p", jsonEvent);	
#endif
		cmn_free(jsonEvent);
	}
	else
	{
		cmn_mutex_unlock(dataConn->mutexLock);
	}

	return EXIT_SUCCESS;
}




/* 
* Loop of json handler for plugins (other threads except main component), for synchronized commands which is not delayed for processing 
*/
int cmnMuxJSonPluginHandle(void *priv, PluginJSonHandler *firstHandler, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	int res = EXIT_SUCCESS;
	PluginJSonHandler *handler = firstHandler;
	struct DATA_CONN *dataConn =(struct DATA_CONN *)jsonEvent->priv;
	if(!dataConn)
	{
		MUX_ERROR("DataConn is null for JSON reply");
		exit(1);
	}

	cmn_mutex_lock(dataConn->mutexLock);
	dataConn->errCode = IPCMD_ERR_NOT_SUPPORT_COMMND;

	while( handler->handler)
	{
		if(!strcasecmp( handler->name, jsonEvent->action))
		{
			MUX_DEBUG("API engine is processing '%s' action", handler->name );
			jsonEvent->event = handler->type;
			
			res = handler->handler(priv, dataConn, jsonEvent);

			break;
		}
		
		handler++;
	}
	cmn_mutex_unlock(dataConn->mutexLock);

	if(CONTROLLER_IS_DEBUG(dataConn) )
	{
		MUX_DEBUG("API engine is replying '%s' action", handler->name );
	}

	cmnMuxJsonPluginReplay(dataConn, jsonEvent);

	return res;
}


/*
 * Loop of json handler in context of controller
*/
int cmnMuxCtrlDataHandle( struct DATA_CONN *dataConn )
{
	int res = EXIT_SUCCESS;
	struct json_handler *_handle = jsonActionHandlers;

	cJSON *cmdObj = cJSON_GetObjectItem(dataConn->cmdObjs, _defaultStates[msg_cmd].key);
	if(cmdObj == NULL)
	{
//		MUX_ERROR("No field of '%s' is found in packet", _defaultStates[msg_cmd].key);
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "No field of '%s' is found in packet", _defaultStates[msg_cmd].key );
	}

	while(_handle->handler )
	{
		if(!strcasecmp(_handle->name, cmdObj->valuestring))
		{

			if(_handle->dest != MUX_PLUGIN_TYPE_MAIN)
			{
				MuxPlugIn *plugin;
				plugin = cmnMuxPluginFind( SYS_MAIN(dataConn), _handle->dest);
				if(!plugin)
				{
					CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Function '%s' is not loaded", CMN_MUX_FIND_PLUGIN_NAME(_handle->dest) );
					goto _ret;
				}
			}
			
			if(CONTROLLER_IS_DEBUG(dataConn))
			{
				MUX_DEBUG("IP Command '%s' is processing.....", cmdObj->valuestring);
			}
			cJSON *dataArray = cJSON_GetObjectItem(dataConn->cmdObjs, _defaultStates[msg_data].key);

			if(!dataArray || !cJSON_IsArray(dataArray))
			{
//				MUX_ERROR( "Data invalidate: Data Item is not an array JSON object");
				CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: Data Item is not an array JSON object");
				goto _ret;
			}

			cJSON *data = cJSON_GetArrayItem( dataArray, 0);
			if(!data )
			{
//				MUX_ERROR("Data invalidate: No data Item is not an data array object");
				CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: No data Item is not an data array object");
				goto _ret;
			}

			dataConn->dataObj = data;
			res = _handle->handler(_handle->dest, dataConn, data);
			if(res == EXIT_FAILURE)
			{
//				MUX_ERROR("Internal error ");
				cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Internal error ");
			}

			
			if(dataConn->errCode != IPCMD_ERR_IN_PROCESSING)
			{
				_cmnMuxJsonReply4All(dataConn);
			}

			goto _ret;
		}

		_handle++;
	}

	/* make controller reply 'command is not defined' in controller's receiving */
	dataConn->errCode = IPCMD_ERR_NOT_SUPPORT_COMMND;

_ret:

	return res;
}


