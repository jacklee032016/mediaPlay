/*
*/

#ifndef __CGI_MUX_H__
#define __CGI_MUX_H__

#include "libCgi.h"
#include "libCmn.h"

#define	CGI_OPTIONS_WINDOW_LIST		1


typedef	struct 
{
	char		lastFile[2048];
	char		currentFile[2048];
	
	char		status[128];
	
	int		totalTime;

	int		totalBytes;
	int		packets;
}_RECORDER_STATUS;

typedef	struct
{
	int		startTime;
	int		duration; /* ms */
	int		videoFormat;	
	int		capLevel;
	int		frameRate;

	int		audioFormat;
}RECORDER_CONFIG;

typedef	struct
{
	MUX_WEB_CMN		cgiMux;

	llist					cfgList;/* for the URLs which are only accessed by web */

	RECORDER_CONFIG		recordCfg;
	_RECORDER_STATUS 	recordStatus;

}MUX_SERVER_INFO;


#define 	MUX_SERVER_CONFIG_FILE	"/etc/muxMedia.conf"

#define	MUX_URL_KEYWORD_HTTP		"URL_HTTP"
#define	MUX_URL_KEYWORD_RTSP		"URL_RTSP"
#define	MUX_URL_KEYWORD_RTP			"URL_RTP"
#define	MUX_URL_KEYWORD_UDP			"URL_UDP"
#define	MUX_URL_KEYWORD_WMSP		"URL_WMSP"
#define	MUX_URL_KEYWORD_RTMP		"URL_RTMP"
#define	MUX_URL_KEYWORD_INPUT		"URL_INPUT"

#define	MUX_WINDOW_KEYWORD_SWAP		"swap"


#define	MUX_KEYWORD_WINDOW_INDEX		"WindowIndex"


int	muxServerUrlConfig(MUX_SERVER_INFO *svrInfo);
int	muxServerUrlPlay(MUX_SERVER_INFO *svrInfo);
int	muxServerUrlSave(MUX_SERVER_INFO *svrInfo);
int	muxServerWindow(MUX_SERVER_INFO *svrInfo);

int	muxServerPlaylist(MUX_SERVER_INFO *svrInfo);

int	muxServerMediaFiles(MUX_SERVER_INFO *svrInfo);

char *submitButton(char *name, char *actionName);

int	muxServerRecord(MUX_SERVER_INFO *svrInfo);

int	muxServerPlayerInfos(MUX_SERVER_INFO *svrInfo);
int	muxServerMediaInfos(MUX_SERVER_INFO *svrInfo);
int	muxServerMetadataInfo(MUX_SERVER_INFO *svrInfo);


int	muxCgiGlobalConfig(MUX_SERVER_INFO *svrInfo);
int	muxCgiSvrRuntime(MUX_SERVER_INFO *svrInfo);

#endif

