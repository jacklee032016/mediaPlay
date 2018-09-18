/*
* Media IP command in json format are handled in this file
*/

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "_cmnMux.h"

#define	MEDIA_CTRL_PLAYLIST_REPEAT	"repeat"
#define	MEDIA_CTRL_PLAYLIST_STATUS	"playlistStatus"
#define	MEDIA_CTRL_PLAYLIST_HOUR		"hour"
#define	MEDIA_CTRL_PLAYLIST_MINUTE	"minute"
#define	MEDIA_CTRL_PLAYLIST_DATE		"date"


int _addOneLocalMediaFile(struct DATA_CONN *dataConn, cJSON *data, char *filename)
{
	int i;
	int found = 0;
	cmn_list_t *mediaFiles = cmnMediaScanGetFiles();

	cJSON *objectsArrary = cJSON_CreateArray();

	if(mediaFiles)
	{
		for(i=0; i< cmn_list_size(mediaFiles); i++)
		{
			char fullName[CMN_NAME_LENGTH];
			
			MEDIA_FILE_T *file = (MEDIA_FILE_T *)cmn_list_get(mediaFiles, i);

			snprintf(fullName, sizeof(fullName), "%s/%s", file->path, file->name);

			if(IS_STRING_EQUAL(fullName, filename) )
			{
				cJSON *obj = NULL;
				obj = cJSON_CreateObject();
				
				MUX_DEBUG("No. %d Media File :'%s' is found", i+1, fullName);
				cJSON_AddItemToObject(obj, "name", cJSON_CreateString(fullName) );
				cJSON_AddItemToObject(obj, "duration", cJSON_CreateNumber(file->duration) );
				cJSON_AddItemToObject(obj, "size", cJSON_CreateNumber(file->size) );
				cJSON_AddItemToObject(obj, "format", cJSON_CreateString(file->formatName) );
				cJSON_AddItemToObject(obj, "hasVideo", cJSON_CreateString(STR_BOOL_VALUE(file->hasVideo) ) );
				cJSON_AddItemToObject(obj, "hasAudio", cJSON_CreateString(STR_BOOL_VALUE(file->hasAudio) ) );

				cJSON_AddItemToArray(objectsArrary, obj);

				found = 1;
			}
		}
	}

	if(found == 0)
	{
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Media file '%s' is not found", filename);
	}
	else
	{
		cJSON_ReplaceItemInObjectCaseSensitive(data, MEDIA_CTRL_OBJECTS, objectsArrary);
	}	

	/* MEDIA_FILE_T objects are kept by ScanObject, so it can't be freed here */
	//cmn_list_ofchar_free(mediaFiles, TRUE);
	cmn_free(mediaFiles);
	
	return EXIT_SUCCESS;
}


int _addAllLocalMediaFiles(struct DATA_CONN *dataConn, cJSON *data)
{
	int i;
	cmn_list_t *mediaFiles = cmnMediaScanGetFiles();

	cJSON *objectsArrary = cJSON_CreateArray();

	if(mediaFiles)
	{
		for(i=0; i< cmn_list_size(mediaFiles); i++)
		{
			cJSON *obj = NULL;
			char fullName[CMN_NAME_LENGTH];
			
			MEDIA_FILE_T *file = (MEDIA_FILE_T *)cmn_list_get(mediaFiles, i);

			obj = cJSON_CreateObject();

			snprintf(fullName, sizeof(fullName), "%s/%s", file->path, file->name);
			
			MUX_DEBUG("No. %d Media File :'%s' is added", i+1, fullName);
			cJSON_AddItemToObject(obj, "name", cJSON_CreateString(fullName) );
			cJSON_AddItemToObject(obj, "duration", cJSON_CreateNumber(file->duration) );
			cJSON_AddItemToObject(obj, "size", cJSON_CreateNumber(file->size) );

			cJSON_AddItemToArray(objectsArrary, obj);
		}
	}

	dataConn->resultObject = objectsArrary;

	/* MEDIA_FILE_T objects are kept by ScanObject, so it can't be freed here */
	//cmn_list_ofchar_free(mediaFiles, TRUE);
	cmn_free(mediaFiles);
	
	return EXIT_SUCCESS;
}


static int _addAllPlayLists(struct DATA_CONN *dataConn, cJSON *data)
{
	int i;

	cJSON *objectsArrary = cJSON_CreateArray();
	MuxMain *muxMain = SYS_MAIN(dataConn);
	cmn_list_t *_playlists = SYS_PLAYLISTS(muxMain);

	SYS_PLAYLIST_LOCK(muxMain);

//	cmn_list_t *_playlists = &dataConn->ctrlConn->controller->muxMain->playlists;

	for(i=0; i< cmn_list_size(_playlists); i++)
	{
		PLAY_LIST *playlist = (PLAY_LIST *)cmn_list_get(_playlists, i);

		MUX_DEBUG( "No. %d playlist :'%s' is added", i+1, playlist->name);
		cJSON_AddItemToArray(objectsArrary, cJSON_CreateString(playlist->name));
	}

	SYS_PLAYLIST_UNLOCK(muxMain);
	dataConn->resultObject = objectsArrary;

	return EXIT_SUCCESS;
}


static int _replyWithOnePlayList(struct DATA_CONN *dataConn, cJSON *data, char *playlistName)
{
	int i, j;

	MuxMain *muxMain = SYS_MAIN(dataConn);
	cmn_list_t *_playlists = SYS_PLAYLISTS(muxMain);

	SYS_PLAYLIST_LOCK(muxMain);

	for(i=0; i< cmn_list_size(_playlists); i++)
	{
		PLAY_LIST *playlist = (PLAY_LIST *)cmn_list_get(_playlists, i);
		if(!strcasecmp(playlist->name, playlistName))
		{
			cJSON_AddItemToObject(data, MEDIA_CTRL_PLAYLIST_REPEAT, cJSON_CreateNumber(playlist->repeat));
			cJSON_AddItemToObject(data, MEDIA_CTRL_PLAYLIST_STATUS, cJSON_CreateBool(playlist->enable));
			cJSON_AddItemToObject(data, MEDIA_CTRL_PLAYLIST_HOUR, cJSON_CreateNumber(playlist->hour));
			cJSON_AddItemToObject(data, MEDIA_CTRL_PLAYLIST_MINUTE, cJSON_CreateNumber(playlist->minute));
			cJSON_AddItemToObject(data, MEDIA_CTRL_PLAYLIST_DATE, cJSON_CreateNumber( playlist->dayOfWeek));

//			cJSON_AddItemToObject(data, MEDIA_CTRL_STATUS, cJSON_CreateNumber( IPCMD_ERR_NOERROR));

			cJSON *objectsArrary = cJSON_CreateArray();
			for(j=0;j< cmn_list_size(&playlist->fileList); j++)
			{
				PLAY_ITEM *playItem = (PLAY_ITEM *)cmn_list_get(&playlist->fileList, j);
				cJSON *playObj = cJSON_CreateObject();
				
				cJSON_AddStringToObject(playObj,  PLAYLIST_NAME_URL, playItem->filename);
				cJSON_AddNumberToObject(playObj,  PLAYLIST_NAME_DURATION, playItem->duration);
				
				cJSON_AddItemToArray(objectsArrary, playObj);
			}
			
			dataConn->resultObject = objectsArrary;

			SYS_PLAYLIST_UNLOCK(muxMain);
			return EXIT_SUCCESS;
		}
	}

	SYS_PLAYLIST_UNLOCK(muxMain);

	return EXIT_FAILURE;
}


char	*_checkDataValidate(cJSON *data)
{
	char *action = cmnGetStrFromJsonObject(data, MEDIA_CTRL_ACTION);
	cJSON *objects = cJSON_GetObjectItem(data, MEDIA_CTRL_OBJECTS);

	if( (action==NULL) || (strlen(action)==0) )
	{
		MUX_ERROR( "Data invalidate: 'action' can not be found");
		return NULL;
	}

	if( (objects == NULL) || (!cJSON_IsArray(objects)) )
	{
		MUX_ERROR( "Data invalidate: 'objects' is not an array");
		return NULL;
	}

	return action;
}


int	cmnMuxJsonHandle4GetMedia(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data) /* data is first item in data array */
{
	int res = EXIT_SUCCESS;

	char *action = _checkDataValidate(data);
	if(!action)
	{
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: action '%s' is not supportted in GET_MEDIA now", action);
	}

//	cJSON *mediaArray = cJSON_CreateArray();

	if(!strcasecmp(action, "files"))
	{
		_addAllLocalMediaFiles(dataConn, data);
	}
	else if(!strcasecmp(action, "playlists"))
	{
		_addAllPlayLists(dataConn, data);
	}
	else if(!strcasecmp(action, "playlist"))
	{
		cJSON *item = cJSON_GetArrayItem(cJSON_GetObjectItem(data, MEDIA_CTRL_OBJECTS), 0);

		if(!cJSON_IsString(item))
		{
			return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: 'objects' is not an string");
		}
		else
		{

			char *playlistName = item->valuestring;

			res = _replyWithOnePlayList(dataConn, data, playlistName);
			if(res == EXIT_FAILURE)
			{
				return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: playlist '%s' is not found", playlistName);
			}
		}
	}
	else if(!strcasecmp(action, "file"))
	{
		cJSON *item = cJSON_GetArrayItem(cJSON_GetObjectItem(data, MEDIA_CTRL_OBJECTS), 0);

		if(!cJSON_IsString(item))
		{
			return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: 'objects' is not an string");
		}
		else
		{

			char *file = item->valuestring;

			res = _addOneLocalMediaFile(dataConn, data, file);
			if(res == EXIT_FAILURE)
			{
				return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: file '%s' is not found", file);
			}
		}
	}
	else
	{
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: action '%s' is not validate", action);
	}

	return res;
}


int	cmnMuxJsonHandle4DownloadMedia(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data) /* data is first item in data array */
{
	int res = EXIT_SUCCESS;
	int i;
	cmn_list_t *filelist = NULL;

	char *server = cmnGetStrFromJsonObject(data, MEDIA_CTRL_FTP_SERVER);
	char *username = cmnGetStrFromJsonObject(data, MEDIA_CTRL_FTP_USERNAME);
	char *password = cmnGetStrFromJsonObject(data, MEDIA_CTRL_FTP_PASSWORD);
	char *path = cmnGetStrFromJsonObject(data, MEDIA_CTRL_FTP_PATH);
//	cJSON *files = cJSON_GetObjectItem(data, MEDIA_CTRL_FTP_FILES);
	cJSON *files = cJSON_GetObjectItem(data, MEDIA_CTRL_OBJECTS);

	MUX_DEBUG("server:'%s'; username:'%s'; password:'%s'; path:'%s'", server, username, password, path);

	if(!cJSON_IsArray(files))
	{
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: mediafiles is not an array");
	}

	if(!server || !username || !password ||!path)
	{
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: info is uncomplete");
	}

	filelist = (cmn_list_t *)cmn_malloc(sizeof(cmn_list_t));
	
	for(i=0; i < cJSON_GetArraySize(files); i++)
	{
		cJSON *item = cJSON_GetArrayItem(files, i);
		if(!cJSON_IsString(item))
		{
			cmn_list_ofchar_free(filelist, TRUE);
			return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: info is uncomplete");
		}
		
		int length =  strlen(path)+strlen(item->valuestring) + 2;
		length = (length< CMN_NAME_LENGTH)?CMN_NAME_LENGTH:length;
		
		char *filename = cmn_malloc(length);
		//snprintf(filename, length, "%s/%s", path, item->valuestring);
		snprintf(filename, length, "%s", item->valuestring);
		cmn_list_append( filelist, filename);
		
		MUX_DEBUG("No.%d Media File: '%s'", i, filename);
	}

	res = cmnMuxFtpAddEvent(server, username, password, path, filelist, dataConn);

	if(res  == EXIT_SUCCESS)
	{/* delay for thread/FSM process */
		dataConn->errCode = IPCMD_ERR_IN_PROCESSING;
	}
	else
	{/* jsonEvent not sent out, free it */
		dataConn->errCode = IPCMD_ERR_SERVER_INTERNEL_ERROR;
	}
	
	return res;
}


int _cmnMuxJsonAddPlaylist(struct DATA_CONN *dataConn, cJSON *data)
{
	int ret = EXIT_SUCCESS;
	int i;
	
	MuxMain *muxMain = SYS_MAIN(dataConn);
	cmn_list_t *_playlists = SYS_PLAYLISTS(muxMain);
	PLAY_LIST *_plist = NULL;
	char *playlistName = cmnGetStrFromJsonObject(data, _MUX_JSON_NAME_MEDIA);

	if(IS_STRING_NULL(playlistName))
	{
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "No name is defined for the new playlist");
	}

	cJSON *playItems = cJSON_GetObjectItem(data, MEDIA_CTRL_OBJECTS);
	if(!cJSON_IsArray(playItems))
	{
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: mediafiles is not an array");
	}

	SYS_PLAYLIST_LOCK(muxMain);

	_plist = cmnMuxPlaylistFind(_playlists, playlistName);
	if(_plist != NULL)
	{
#if 0	
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Playlist '%s' existed now", playlistName);
#else
		ret = cmnMuxPlayListRemove(_playlists, playlistName);
		if(ret == EXIT_FAILURE)
		{
			SYS_PLAYLIST_UNLOCK(muxMain);
			return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Removing existing playlist '%s' failed", playlistName);
		}
#endif
	}


	_plist = cmnMuxCreateOnePlaylist(playlistName, NULL);
	
	for(i=0; i < cJSON_GetArraySize(playItems); i++)
	{
		cJSON *item = cJSON_GetArrayItem(playItems, i);
		PLAY_ITEM *_playItem = NULL;
		
		char *file = cmnGetStrFromJsonObject(item, PLAYLIST_NAME_URL);
		int duration = cmnGetIntegerFromJsonObject(item, PLAYLIST_NAME_DURATION);

#if 0
		if(!cJSON_IsString(item))
		{
			cmn_list_ofchar_free( &_plist->fileList, TRUE);
			
			return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: file is uncomplete");
		}
		int length =  strlen(item->valuestring) + 2;
		length = (length< CMN_NAME_LENGTH)?CMN_NAME_LENGTH:length;
		
		char *filename = cmn_malloc(length);
		//snprintf(filename, length, "%s/%s", path, item->valuestring);
		snprintf(filename, length, "%s", item->valuestring);
		cmn_list_append( &_plist->fileList, filename);
#endif

		if(IS_STRING_NULL(file) || duration< 0)
		{
			cmn_list_ofchar_free( &_plist->fileList, TRUE);
			SYS_PLAYLIST_UNLOCK(muxMain);
			return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Invalidate value: %s: '%s', %s: %d", PLAYLIST_NAME_URL, file, PLAYLIST_NAME_DURATION, duration );
		}

		_playItem = cmn_malloc(sizeof(PLAY_ITEM));
		snprintf(_playItem->filename, sizeof(_playItem->filename), "%s", file);
		_playItem->duration = duration;
		
		cmn_list_append( &_plist->fileList, _playItem);
		
		MUX_DEBUG("No.%d %s: '%s', %s: %d", i, PLAYLIST_NAME_URL, file, PLAYLIST_NAME_DURATION, duration );
	}
	
	ret = cmnMuxPlayListAdd(_playlists, _plist);

#if MUX_SAVE_PLAYLIST
	ret = cmnMuxSaveAllConfig(muxMain);
#endif

	SYS_PLAYLIST_UNLOCK(muxMain);
	return ret;		
}


int	cmnMuxJsonHandle4SetMedia(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data) /* data is first item in data array */
{
	int res = EXIT_SUCCESS;
	int i;

	cJSON *objects = cJSON_GetObjectItem(data, MEDIA_CTRL_OBJECTS);
	MuxMain *muxMain = SYS_MAIN(dataConn);
	cmn_list_t *_playlists = SYS_PLAYLISTS(muxMain);

	char *action = _checkDataValidate(data);
	if(!action)
	{
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: no 'action' is defined in this SET_MEDIA command", action);
	}

	if( strcasecmp(action, "fileDelete") && 
		strcasecmp(action, "download")  &&
		strcasecmp(action, "playlistDelete")  &&
		strcasecmp(action, "playlistAdd") )
	{
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: action '%s' is not supportted in SET_MEDIA now", action);
	}

	if(!strcasecmp(action, "download"))
	{
		return cmnMuxJsonHandle4DownloadMedia(MUX_PLUGIN_TYPE_MAIN, dataConn, data);
	}

	if(!strcasecmp(action, "playlistAdd"))
	{
		return _cmnMuxJsonAddPlaylist(dataConn, data);
	}
			
	for (i = 0; i < cJSON_GetArraySize(objects); i++)
	{
		cJSON *subitem = cJSON_GetArrayItem(objects, i);
		if(!subitem || !cJSON_IsString(subitem))
		{
			return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "No '%d' item in 'objects' is not validate", i+1);
		}

		if(!strcasecmp(action, "fileDelete"))
		{
			if(unlink(subitem->valuestring) )
			{
				res = EXIT_FAILURE;
			}
			else
			{
				res = EXIT_SUCCESS;
			}
		}
		else if(!strcasecmp(action, "playlistDelete"))
		{
			SYS_PLAYLIST_LOCK(muxMain);
			res = cmnMuxPlayListRemove(_playlists, subitem->valuestring);
			SYS_PLAYLIST_UNLOCK(muxMain);
		}

		if(res == EXIT_FAILURE)
		{
			return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Removing No '%d' of item '%s' failed", i+1,subitem->valuestring);
		}
		else
		{
			dataConn->resultObject = cJSON_CreateArray();
			cJSON_AddItemToArray(dataConn->resultObject, cJSON_CreateString(subitem->valuestring));
		}
	}

	return res;
}

static int _cmnMuxAddJSonEvent(MUX_PLUGIN_TYPE pluginType, char *action, cJSON *data, void *_dataConn)
{
	CMN_PLAY_JSON_EVENT *jsonEvent =(CMN_PLAY_JSON_EVENT *)cmn_malloc(sizeof(CMN_PLAY_JSON_EVENT));
	int res = EXIT_SUCCESS;

	struct DATA_CONN *dataConn = (struct DATA_CONN *)_dataConn;
	MuxMain *muxMain = SYS_MAIN(dataConn);
	
	if(muxMain == NULL )
	{
		MUX_ERROR("No MuxMain is found in DataConn");
		exit(1);
		return EXIT_FAILURE;
	}
	/* event->event set as 0, that is OK  */
	snprintf(jsonEvent->action, JSON_ACTION_LENGTH, "%s", action);
	jsonEvent->object = data;
	jsonEvent->priv = _dataConn;
	
	res = muxMain->reportEvent(muxMain, pluginType, jsonEvent);
	if(res  == EXIT_FAILURE)
	{/* jsonEvent not sent out, free it */
		cmn_free(jsonEvent);
		dataConn->errCode = IPCMD_ERR_PLUGIN_NOT_EXISTS;
	}
	else
	{/* delay for thread/FSM process */
		dataConn->errCode = IPCMD_ERR_IN_PROCESSING;
	}
	
	return EXIT_SUCCESS;
}


/* IP commands handled by plugin which external to main component */
int	cmnMuxJsonHandle4Plugin(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data) /* data is first item in data array */
{
	int res = EXIT_SUCCESS;

	char *action = _checkDataValidate(data);
	if(!action)
	{
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: action '%s' is not supportted in PLAY_MEDIA now", action);
	}
	
	cJSON *objects = cJSON_GetObjectItem(data, MEDIA_CTRL_OBJECTS);
	cJSON *content = cJSON_GetArrayItem(objects, 0);

	res = _cmnMuxAddJSonEvent(dest, action, content, dataConn);

	return res;
}


/* system administration of this program */
int	cmnMuxJsonHandle4SysAdmin(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data) /* data is first item in data array */
{
	int res = EXIT_SUCCESS;

//	cJSON *objects = cJSON_GetObjectItem(data, MEDIA_CTRL_OBJECTS);
	MuxMain *muxMain = SYS_MAIN(dataConn);
//	cmn_list_t *_playlists = SYS_PLAYLISTS(muxMain);
	int i = 0;

	char *action = _checkDataValidate(data);
	if(!action)
	{
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: no 'action' is defined in this sys_admin command", action);
	}

	if( strcasecmp(action, IPCMD_SYS_ADMIN_THREADS) && 
		strcasecmp(action, IPCMD_SYS_ADMIN_VER_INFO) )
	{
		return CMN_CONTROLLER_REPLY_DATA_ERR(dataConn, "Data invalidate: action '%s' is not supportted in sys_admin now", action);
	}

	if(!strcasecmp(action, IPCMD_SYS_ADMIN_THREADS))
	{
		cmn_list_t  *threads = cmnThreadLockList();//&_threadList;
		cJSON *objectsArrary = cJSON_CreateArray();

		for(i=0; i< cmn_list_size(threads); i++)
		{
			CmnThread *thread = (CmnThread *)cmn_list_get(threads, i);
			cJSON *obj = NULL;

#if MUX_OPTIONS_DEBUG_IP_COMMAND			
			MUX_DEBUG( "No. %d thread :'%s' is added", i+1, thread->name);
#endif

			obj = cJSON_CreateObject();
				
			cJSON_AddItemToObject(obj, "name", cJSON_CreateString(thread->name) );
			cJSON_AddItemToObject(obj, "PID", cJSON_CreateNumber((int)thread->pId) );

			cJSON_AddItemToArray(objectsArrary, obj);

		}

		dataConn->resultObject = objectsArrary;

		cmnThreadReleaseList();
		return EXIT_SUCCESS;
	}

	if(!strcasecmp(action, IPCMD_SYS_ADMIN_VER_INFO))
	{
		MuxPlugIn	*plugin = NULL;
		cJSON *obj = NULL;

		cJSON *objectsArrary = cJSON_CreateArray();
		
		obj = cJSON_CreateObject();
		cJSON_AddItemToObject(obj, "name", cJSON_CreateString(CMN_MODULE_MAIN_NAME) );
		cJSON_AddItemToObject(obj, "version", cJSON_CreateString(CMN_VERSION_INFO(CMN_MODULE_MAIN_NAME)) );
		cJSON_AddItemToArray(objectsArrary, obj);

		plugin = muxMain->plugins;
		while(plugin)
		{
			if(plugin->enable )
			{
				obj = cJSON_CreateObject();
				cJSON_AddItemToObject(obj, "name", cJSON_CreateString(plugin->name) );
				cJSON_AddItemToObject(obj, "version", cJSON_CreateString(plugin->version) );
				cJSON_AddItemToArray(objectsArrary, obj);
			}

 			plugin = plugin->next;
		}

		dataConn->resultObject = objectsArrary;

		cmnThreadReleaseList();

		return EXIT_SUCCESS;
	}

	
	return res;
}


