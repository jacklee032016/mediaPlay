
#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "_cmnMux.h"

/* media is name of playlist and local file, such as media in USB disk or SD card */
cJSON *muxApiPlayMediaPlay(int winIndex, char *media, int repeatNumber)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_WINDOW, cJSON_CreateNumber((double) winIndex));
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_REPEAT, cJSON_CreateNumber((double) repeatNumber));
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_MEDIA, cJSON_CreateString(media));


	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, "play", obj);
}

cJSON *muxApiPlayAspect(int winIndex, int mode)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_WINDOW, cJSON_CreateNumber((double) winIndex));
	cJSON_AddItemToObject(obj, MUX_JSON_NAME_ASPECT_MODE, cJSON_CreateNumber((double) mode));

	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, IPCMD_NAME_ASPECT_WINDOW, obj);
}


cJSON *_muxApiPlayMediaGenericCmd(int winIndex, char *action)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_WINDOW, cJSON_CreateNumber((double) winIndex));

	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, action, obj);
}

cJSON *_muxApiPlayMediaGenericCmdNoParams( char *action)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();

	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, action, obj);
}


cJSON *_muxApiMediaGenericCmdNoParams( char *ipcmdName, char *action)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();

	return cmnMuxClientRequest( ipcmdName, action, obj);
}

cJSON *muxApiPlayAudio(int winIndex, char *media)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_WINDOW, cJSON_CreateNumber((double) winIndex));
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_MEDIA, cJSON_CreateString(media));

	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, IPCMD_NAME_PLAYER_AUDIO , obj);
}


cJSON *muxApiPlayMediaLocateWindow(int winIndex, int left, int top, int width, int height)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_WINDOW, cJSON_CreateNumber((double) winIndex));

	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_LOCATION_X, cJSON_CreateNumber((double) left));
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_LOCATION_Y, cJSON_CreateNumber((double) top));
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_LOCATION_W, cJSON_CreateNumber((double) width));
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_LOCATION_H, cJSON_CreateNumber((double) height));

	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, "locateWindow", obj);
}


cJSON *muxApiPlayMediaOsdPosition(int osdIndex, int left, int top, int width, int height)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_OSD, cJSON_CreateNumber((double) osdIndex));

	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_LOCATION_X, cJSON_CreateNumber((double) left));
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_LOCATION_Y, cJSON_CreateNumber((double) top));
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_LOCATION_W, cJSON_CreateNumber((double) width));
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_LOCATION_H, cJSON_CreateNumber((double) height));

	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, "osdPosition", obj);
}

cJSON *muxApiPlayMediaOsdIndex(char *action, int osdIndex)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_OSD, cJSON_CreateNumber((double) osdIndex));

	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, action, obj);
}


cJSON *_muxApiPlayMediaGenericOsd(int osdIndex, char *action, char *key, int value)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_OSD, cJSON_CreateNumber((double) osdIndex));
	cJSON_AddItemToObject(obj, key, cJSON_CreateNumber((double) value));

	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, action, obj);
}

#if BACKGROUND_AS_STRING	
cJSON *muxApiPlayMediaOsdBackground(int osdIndex, char *backgroundStr)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_OSD, cJSON_CreateNumber((double) osdIndex));
	cJSON_AddItemToObject(obj, "background", cJSON_CreateString(backgroundStr));
 
	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY,  "osdBackground", obj);
}
#endif

cJSON *muxApiPlayMediaSubtitle( char *media)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_MEDIA, cJSON_CreateString(media));


	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, "subtitle", obj);
}

/* media for logo icon, can be bmp, gif, png and jpeg */
cJSON *muxApiPlayMediaLogo(char *media)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_MEDIA, cJSON_CreateString(media));

	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, "logo", obj);
}


cJSON *muxApiPlayMediaAlert(int fontcolor, char *message)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, MEDIA_CTRL_STATUS_MSG, cJSON_CreateString(message));

	cJSON_AddItemToObject(obj, "fontcolor", cJSON_CreateNumber((double) fontcolor));
	
	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, "alert", obj);
}

cJSON *muxApiPlayMediaRecordStart(char *recordFileName, int duration/* in second*/)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, _MUX_JSON_NAME_MEDIA, cJSON_CreateString(recordFileName));
	cJSON_AddItemToObject(obj, "duration", cJSON_CreateNumber((double) duration));

	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_RECORDER, "start", obj);
}



/* other IP commands*/

/* GetMedia command */
cJSON *_muxApiGetMedia(char *action, char *playlistName)
{
	cJSON *objsArray = NULL, *actionArray;//, *obj = NULL;
	cJSON *obj = NULL;//cJSON_CreateObject();

	if( (playlistName !=NULL) && (strlen(playlistName)) )
	{/* add name of playlist or media file of 'file' and 'playlst' command */
		objsArray = cJSON_CreateArray();
		cJSON_AddItemToArray(objsArray, cJSON_CreateString(playlistName) );
		
		obj= cJSON_CreateObject();
		cJSON_AddItemToObject(obj, MEDIA_CTRL_OBJECTS, objsArray);
		cJSON_AddItemToObject(obj, MEDIA_CTRL_ACTION, cJSON_CreateString(action) );
		
		actionArray = cJSON_CreateArray();
		cJSON_AddItemToArray(actionArray, obj );
		return cmnMuxClientSendout(IPCMD_NAME_MEDIA_GET, actionArray);
	}
	else
	{/* for 'files' and 'playlists' commands without parameter */
		obj = cJSON_CreateObject();
		return cmnMuxClientRequest( IPCMD_NAME_MEDIA_GET, action, obj);
	}
	
//	cJSON_AddItemToObject(obj, MEDIA_CTRL_OBJECTS, array);
//	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_GET, cmd, obj);

}


/* SetMedia command: 'fileDelete', 'playlistDelete' and 'playlistAdd' */
cJSON *_muxApiSetMediaAddPlaylist(char *action, char *objectName, cmn_list_t *medias)
{
	cJSON *objsArray = NULL, *obj = NULL;
	cJSON	*actionArray = NULL;
	int i=0;

	obj = cJSON_CreateObject();

	/* file list for fileDelete and playlistAdd command */
	objsArray = cJSON_CreateArray();
	
	for(i=0; i< cmn_list_size(medias); i++)
	{
		cJSON *playObj = NULL;
		PLAY_ITEM *playItem =  (PLAY_ITEM *)cmn_list_get(medias, i);
		playObj = cJSON_CreateObject();
		cJSON_AddItemToObject(playObj, PLAYLIST_NAME_URL, cJSON_CreateString(playItem->filename) );
		cJSON_AddItemToObject(playObj, PLAYLIST_NAME_DURATION, cJSON_CreateNumber(playItem->duration) );

		cJSON_AddItemToArray(objsArray, playObj);
	}
	
	cJSON_AddItemToObject(obj, MEDIA_CTRL_OBJECTS, objsArray);

	if( (objectName !=NULL) && (strlen(objectName)) )
	{
		cJSON_AddItemToObject(obj, _MUX_JSON_NAME_MEDIA, cJSON_CreateString(objectName));
	}
	cJSON_AddItemToObject(obj, MEDIA_CTRL_ACTION, cJSON_CreateString(action) );

	actionArray = cJSON_CreateArray();
	cJSON_AddItemToArray(actionArray, obj );
	return cmnMuxClientSendout( IPCMD_NAME_MEDIA_SET, actionArray);
}

/* SetMedia command: 'fileDelete', 'playlistDelete' and 'playlistAdd' */
cJSON *_muxApiSetMedia(char *action, char *objectName, cmn_list_t *medias)
{
	cJSON *objsArray = NULL, *obj = NULL;
	cJSON	*actionArray = NULL;
	int i=0;

	obj = cJSON_CreateObject();

	/* file list for fileDelete and playlistAdd command */
	objsArray = cJSON_CreateArray();
	
	for(i=0; i< cmn_list_size(medias); i++)
	{
		char	*mediaName = (char *)cmn_list_get(medias, i);
		cJSON_AddItemToArray(objsArray, cJSON_CreateString(mediaName) );
	}
	
	cJSON_AddItemToObject(obj, MEDIA_CTRL_OBJECTS, objsArray);

	if( (objectName !=NULL) && (strlen(objectName)) )
	{
		cJSON_AddItemToObject(obj, _MUX_JSON_NAME_MEDIA, cJSON_CreateString(objectName));
	}
	cJSON_AddItemToObject(obj, MEDIA_CTRL_ACTION, cJSON_CreateString(action) );

	actionArray = cJSON_CreateArray();
	cJSON_AddItemToArray(actionArray, obj );
	return cmnMuxClientSendout( IPCMD_NAME_MEDIA_SET, actionArray);
}


cJSON *muxApiDownloadMedia(char *server, char *user, char *password, char *path, char *filename)
{
	cJSON *array = NULL, *obj = NULL;
	cJSON	*actionArray = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, MEDIA_CTRL_FTP_SERVER, cJSON_CreateString(server));
	cJSON_AddItemToObject(obj, MEDIA_CTRL_FTP_USERNAME, cJSON_CreateString(user));
	cJSON_AddItemToObject(obj, MEDIA_CTRL_FTP_PASSWORD, cJSON_CreateString(password));
	cJSON_AddItemToObject(obj, MEDIA_CTRL_FTP_PATH, cJSON_CreateString(path));

	array = cJSON_CreateArray();
	cJSON_AddItemToArray(array, cJSON_CreateString(filename));
	
//	cJSON_AddItemToObject(obj, MEDIA_CTRL_FTP_FILES, array);
	cJSON_AddItemToObject(obj, MEDIA_CTRL_OBJECTS, array);

	cJSON_AddItemToObject(obj, MEDIA_CTRL_ACTION, cJSON_CreateString("download") );

	actionArray = cJSON_CreateArray();
	cJSON_AddItemToArray(actionArray, obj );
	return cmnMuxClientSendout( IPCMD_NAME_MEDIA_SET, actionArray);

//	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_DOWNLOAD, NULL, obj);
}

/* get first entry in 'data' array in reply of IP command */
static cJSON *_muxApiGetDataReply(cJSON *ipCmdReply)
{
	cJSON *data = cJSON_GetObjectItem(ipCmdReply, IPCMD_NAME_KEYWORD_DATA);
	if( (data==NULL) ||( !cJSON_IsArray(data)) )
	{
		MUX_ERROR("IP command '%s' wrong", IPCMD_NAME_KEYWORD_DATA);
		return NULL;
	}

	return cJSON_GetArrayItem(data, 0);
}

/* returnArray =0: return first item in array; otherwise return this array */
cJSON *muxApiGetDataReply(cJSON *ipCmdReply, int returnArray)
{
	cJSON *data = _muxApiGetDataReply(ipCmdReply);
	if( (data==NULL) )
	{
		MUX_ERROR("No 'data' in JSON object");
		return NULL;
	}


	cJSON *objs = cJSON_GetObjectItem(data, MEDIA_CTRL_OBJECTS);
	if( (objs ==NULL) ||( !cJSON_IsArray(objs)) )
	{
		MUX_ERROR("No 'objects' in 'data' JSON object");
		return NULL;
	}

	if(returnArray )
	{
		return objs;
	}
	
	return cJSON_GetArrayItem(objs, 0);
}


int	muxApiGetStatus(cJSON *ipCmdReply)
{
	cJSON *data = _muxApiGetDataReply(ipCmdReply);
	if(data == NULL)
	{
		return 600;
	}

	return cmnGetIntegerFromJsonObject(data, MEDIA_CTRL_STATUS);
}

cJSON *muxApiEdidResolution( char *resolution)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, IPCMD_EDID_RESOLUTION, cJSON_CreateString(resolution));

	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, IPCMD_EDID_RESOLUTION, obj);
}


cJSON *muxApiEdidColorDepth( int colorDepth)
{
	cJSON *obj = NULL;

	obj = cJSON_CreateObject();
	cJSON_AddItemToObject(obj, IPCMD_EDID_DEEP_COLOR, cJSON_CreateNumber((double) colorDepth));

	return cmnMuxClientRequest( IPCMD_NAME_MEDIA_PLAY, IPCMD_EDID_DEEP_COLOR, obj);
}


