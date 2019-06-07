
#ifndef	__API_CLIENT_H__
#define	__API_CLIENT_H__


#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "cmnMuxApi.h"

extern char *optarg;
extern int optind, opterr, optopt;

enum _PROTOCOL
{
	PROTOCOL_UDP		= 0,
	PROTOCOL_TCP,
	PROTOCOL_UNIX,
	PROTOCOL_UNKNOWN,
};

typedef	enum
{
	CLIENT_CMD_PLAY = 0,
	CLIENT_CMD_STOP,
	CLIENT_CMD_PAUSE,
	CLIENT_CMD_RESUME,
	CLIENT_CMD_FORWARD,
	CLIENT_CMD_BACKWARD,
	CLIENT_CMD_SUBTITLE,		/* 5 */

	CLIENT_CMD_PLAY_INFO,
	CLIENT_CMD_PLAY_MEDIA,
	
	CLIENT_CMD_RECORD_START,
	CLIENT_CMD_RECORD_STOP,
	CLIENT_CMD_RECORD_STATUS,
	CLIENT_CMD_ALERT,
	CLIENT_CMD_LOGO,
	CLIENT_CMD_OSD_ENABLE,			/* 10 */
	CLIENT_CMD_OSD_BACKGROUND,
	CLIENT_CMD_OSD_TRANSPANRENCY,
	CLIENT_CMD_OSD_FONTCOLOR,
	CLIENT_CMD_OSD_FONTSIZE,
	CLIENT_CMD_OSD_POSITION,
	CLIENT_CMD_OSD_INFO,

	CLIENT_CMD_SWAP_WINDOW,			/* 15 */
	CLIENT_CMD_ROTATE_WINDOW,
	CLIENT_CMD_LOCATE_WINDOW,

	CLIENT_CMD_ASPECT_WINDOW,

	
	CLIENT_CMD_VOL_PLUS,
	CLIENT_CMD_VOL_MINUS,
	CLIENT_CMD_AUDIO,
	CLIENT_CMD_MUTE,
	CLIENT_CMD_MUTE_ALL,
	
	CLIENT_CMD_QUIT,					/* 20 */
	CLIENT_CMD_FILES,
	CLIENT_CMD_PLAYLISTS,
	CLIENT_CMD_FILE,
	CLIENT_CMD_PLAYLIST,
	CLIENT_CMD_PLAYLIST_ADD,
	CLIENT_CMD_FILE_DELETE,			/* 25 */
	CLIENT_CMD_PLAYLIST_DELETE,
	CLIENT_CMD_DOWNLOAD,

	/* client API to HDMI CEC */
	CLIENT_CMD_CEC_STANDBY,
	CLIENT_CMD_CEC_IMAGE_ON,
	CLIENT_CMD_CEC_VOLUME_UP,
	CLIENT_CMD_CEC_VOLUME_DOWN,
	CLIENT_CMD_CEC_MUTE,
	CLIENT_CMD_CEC_INFO,

	CLIENT_CMD_EDID_RESOLUTION,
	CLIENT_CMD_EDID_DEEP_COLOR, /* added. 06.04, 2019 */
	CLIENT_CMD_EDID_COLOR_SPACE,

	/* client API to access SERVER */
	CLIENT_CMD_SVR_CONFIG,
	CLIENT_CMD_SVR_FEEDS,
	CLIENT_CMD_SVR_CONNS,
	CLIENT_CMD_SVR_URLS,


	/* client API to access WEB */
	CLIENT_CMD_WEB_INFOS,
	
	/* client API of system admin */
	CLIENT_CMD_SYS_ADMIN_THREADS,
	CLIENT_CMD_SYS_ADMIN_VER_INFO,

	CLIENT_CMD_UNKNOWN
}CLIENT_CMD_TYPE;


#define	CLIENT_CMD_NAME_SVR_CONFIG				"serviceConfig"
#define	CLIENT_CMD_NAME_SVR_FEEDS				"serviceFeeds"
#define	CLIENT_CMD_NAME_SVR_CONNS				"serviceConns"
#define	CLIENT_CMD_NAME_SVR_URLS				"serviceUrls"

#define	CLIENT_CMD_NAME_WEB_INFO				"webInfos"

struct API_PARAMETERS
{
	char 	address[CMN_NAME_LENGTH];
	enum _PROTOCOL		protocol;
	int		port;

	char		cmd[CMN_NAME_LENGTH];
	int		cmdType;
	
	char		media[CMN_NAME_LENGTH];
	int		repeatNumber;	/* repeat number for this media */

	cmn_list_t	files;	/* used in fileDelete and playlistAdd */

	int		index;
	
	int		left;
	int		top;
	int		width;
	int		height;

	int		duration;
	
	int		color;
#if BACKGROUND_AS_STRING	
	char		backgroundColor[CMN_NAME_LENGTH];
#endif

	char		ftpSvr[CMN_NAME_LENGTH];
	char		ftpUser[CMN_NAME_LENGTH];
	char		ftpPasswd[CMN_NAME_LENGTH];
	char		ftpPath[CMN_NAME_LENGTH];
	
	cJSON	*result;

};


int	apiClientParseSubOptions(char *optarg, struct API_PARAMETERS *clientParams);

char	*apiClientOptionsPrompt(struct API_PARAMETERS *clientParams);


#endif

