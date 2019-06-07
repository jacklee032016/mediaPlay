
#ifndef	__CMN_MUX_API_H__
#define	__CMN_MUX_API_H__

#ifdef __cplusplus
	extern "C"
	{
#endif

int cmnMuxClientInit(int port, CTRL_LINK_TYPE type, char *serverAddress);
void cmnMuxClientDestroy(void);

/* PlayMedia command and its actions */

cJSON *_muxApiPlayMediaGenericCmd(int winIndex, char *action);
cJSON *_muxApiPlayMediaGenericCmdNoParams( char *action);

cJSON *muxApiPlayAudio(int winIndex, char *media);


cJSON *muxApiPlayAspect(int winIndex, int mode);

cJSON *muxApiPlayMediaPlay(int winIndex, char *media, int repeatNumber);

#define	muxApiPlayMediaStop(winIndex) \
	_muxApiPlayMediaGenericCmd((winIndex), "stop")

#define	muxApiPlayMediaPause(winIndex) \
	_muxApiPlayMediaGenericCmd((winIndex), "pause")

#define	muxApiPlayMediaResume(winIndex) \
	_muxApiPlayMediaGenericCmd((winIndex), "resume")

#define	muxApiPlayMediaForward(winIndex) \
	_muxApiPlayMediaGenericCmd((winIndex), "forward")

#define	muxApiPlayMediaBackward(winIndex) \
	_muxApiPlayMediaGenericCmd((winIndex), "backward")

#define	muxApiPlayMediaPlayerInfo(winIndex) \
	_muxApiPlayMediaGenericCmd((winIndex), "playerInfo")

#define	muxApiPlayMediaMediaInfo(winIndex) \
	_muxApiPlayMediaGenericCmd((winIndex), "mediaInfo")

#define	muxApiPlayMediaMetaData(winIndex) \
	_muxApiPlayMediaGenericCmd((winIndex), "metaData")

#define	muxApiPlayMediaSwapWindow(winIndex) \
	_muxApiPlayMediaGenericCmd((winIndex), "swapWindow")

#define	muxApiPlayMediaRotateWindow(winIndex) \
	_muxApiPlayMediaGenericCmd((winIndex), "rotateWindow")

#define	muxApiPlayMediaVolumePlus(winIndex)	\
	_muxApiPlayMediaGenericCmd((winIndex), "vol+")

#define	muxApiPlayMediaVolumeMinus(winIndex)	\
	_muxApiPlayMediaGenericCmd((winIndex), "vol-")

#define	muxApiPlayMediaMute(winIndex)	\
	_muxApiPlayMediaGenericCmd((winIndex), IPCMD_NAME_PLAYER_MUTE)


#define	muxApiPlayMediaMuteAll()	\
	_muxApiPlayMediaGenericCmdNoParams(IPCMD_NAME_PLAYER_MUTE_ALL)

cJSON *muxApiPlayMediaRecordStart(char *recordFileName, int duration/* in second*/);


cJSON *_muxApiMediaGenericCmdNoParams( char *ipcmdName, char *action);

#define	muxApiPlayMediaRecordStop()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_RECORDER, "stop")

#define	muxApiPlayMediaRecordStatus()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_RECORDER, "status")

#define	muxApiPlayMediaRecordPause()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_RECORDER, "pause")

#define	muxApiPlayMediaRecordResume()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_RECORDER, "resume")

/* APIs for CEC */
#define	muxApiCecStandby()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_PLAY, IPCMD_CEC_STAND_BY)

#define	muxApiCecImageOn()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_PLAY, IPCMD_CEC_IMAGE_VIEW_ON)

#define	muxApiCecVolumeUp()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_PLAY, IPCMD_CEC_VOLUME_UP)

#define	muxApiCecVolumeDown()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_PLAY, IPCMD_CEC_VOLUME_DOWN)

#define	muxApiCecMute()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_PLAY, IPCMD_CEC_MUTE)

#define	muxApiCecInfo()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_PLAY, IPCMD_CEC_INFO)


/* APIs for SERVER */
#define	muxApiServerConfig()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_SERVER, IPCMD_NAME_SERVER_CONFIG)

#define	muxApiServerFeeds()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_SERVER, IPCMD_NAME_SERVER_FEEDS)

#define	muxApiServerConns()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_SERVER, IPCMD_NAME_SERVER_CONNS)

#define	muxApiServerUrls()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_SERVER, IPCMD_NAME_SERVER_URLS)


/* APIs for WEB */
#define	muxApiWebInfos()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_MEDIA_WEB, "infos")
	

/* APIs for Sys Admin */
#define	muxApiSysAdminThreads()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_SYS_ADMIN, IPCMD_SYS_ADMIN_THREADS)
	
#define	muxApiSysAdminVerInfo()	\
	_muxApiMediaGenericCmdNoParams(IPCMD_NAME_SYS_ADMIN, IPCMD_SYS_ADMIN_VER_INFO)



#define	muxApiPlayMediaQuit()	\
	_muxApiPlayMediaGenericCmdNoParams("quit")




cJSON *muxApiPlayMediaLocateWindow(int winIndex, int left, int top, int width, int height);
cJSON *muxApiPlayMediaOsdPosition(int osdIndex, int left, int top, int width, int height);
cJSON *muxApiPlayMediaOsdIndex(char *action, int osdIndex);
cJSON *muxApiPlayMediaLogo(char *media);

cJSON *muxApiPlayMediaSubtitle( char *media);
cJSON *muxApiPlayMediaAlert(int fontcolor, char *message, int hAlign, int vAlign);


cJSON *_muxApiPlayMediaGenericOsd(int osdIndex, char *action, char *key, int value);


#if BACKGROUND_AS_STRING	
cJSON *muxApiPlayMediaOsdBackground(int osdIndex, char *backgroundStr);
#else
#define	muxApiPlayMediaOsdBackground(osdIndex, background)		\
	_muxApiPlayMediaGenericOsd((osdIndex), "osdBackground", "background", (background))
#endif

#define	muxApiPlayMediaOsdTranspanrency(osdIndex, alpha)		\
	_muxApiPlayMediaGenericOsd((osdIndex), "osdTransparency", "transparency", (alpha))

#define	muxApiPlayMediaOsdFontSize(osdIndex, fontSize)		\
	_muxApiPlayMediaGenericOsd((osdIndex), "osdFontSize", "fontsize", (fontSize))

#define	muxApiPlayMediaOsdFontColor(osdIndex, fontColor)		\
	_muxApiPlayMediaGenericOsd((osdIndex), "osdFontColor", "fontcolor", (fontColor))


/************* other IP commands **************/
/* GetMedia command */
cJSON *_muxApiGetMedia(char *cmd, char *playlistName);

#define	muxApiGetMediaGetAllFile() \
	_muxApiGetMedia("files", NULL)

#define	muxApiGetMediaGetAllPlaylists() \
	_muxApiGetMedia("playlists", NULL)

#define	muxApiGetMediaGetOneFile(mediaFile) \
	_muxApiGetMedia("file", (mediaFile))

#define	muxApiGetMediaGetOnePlaylist(playlist) \
	_muxApiGetMedia("playlist", (playlist))

/* SetMedia command */
cJSON *_muxApiSetMedia(char *cmd, char *objectName, cmn_list_t *medias);
cJSON *_muxApiSetMediaAddPlaylist(char *action, char *objectName, cmn_list_t *medias);


#define	muxApiSetMediaDeleteFile( mediaList) \
		_muxApiSetMedia("fileDelete", NULL, (mediaList) )

#define	muxApiSetMediaDeletePlaylist(mediaList) \
		_muxApiSetMedia("playlistDelete", NULL, (mediaList) )

#define	muxApiSetMediaAddPlaylist(playlistName, mediaList) \
		_muxApiSetMediaAddPlaylist("playlistAdd", (playlistName), (mediaList) )



/* DownloadMedia command */
cJSON *muxApiDownloadMedia(char *server, char *user, char *password, char *path, char *filename);


cJSON *muxApiGetDataReply(cJSON *ipCmdReply, int returnArray);
int	muxApiGetStatus(cJSON *ipCmdReply);

cJSON *muxApiEdidResolution( char *resolution);
cJSON *muxApiEdidColorDepth( int colorDepth);
cJSON *muxApiEdidColorSpace( int colorSapce);


#ifdef __cplusplus
	};
#endif

#endif

