/*
* This is a command line API client, mainly used in crond task to play media based on time
*/


#include "apiClient.h"

/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
*/

/*
send JSON request from json file and parse JSON response, used in CGI and other client, such as controller 811
*/


static void usage(char* base, struct API_PARAMETERS *params)
{
#if 0
	printf("%s: Command line interface for JSON API.\n"\
		"\t\t%s -c command [-i windowIndex/osdIndex -m media -l left -t top -w width -h height -a ipaddress/fqdn -d duratoin " \
		"-C background|fontColor|fontsize] -s ftpServer -u ftpUser -f ftpPassword -P ftpPath -p 0(UDP)|1(TCP)|2(UnixSys) -b port(3600)\n"\
		"\t\t Current command: \n\t\t\tplay, stop, pause, resume, forward, backforwar, subtitle, " \
		"\n\t\t\trecordStart, recordStop, recordStatus, "\
		"\n\t\t\talert, logo, osdEnable, osdBackground, osdFontColor, osdFontSize, osdPosition, " \
		"\n\t\t\tsawpWindow, rotateWindow, vol+, vol-, mute, quit, " \
		"\n\t\t\tfiles, playlists, playlist, fileDelete, playlistDelete, download\n" \
		"\t\t windowIndex: default is 0; media : URL/Local file/Playlist; \n" \
		"\t\t ipaddress/fqdn: default localhost; -d duration: duration for recording; \n", 
		  base, base);
#else
	printf("%s: \n\tCommand line interface for JSON API.\n"\
		"\t%s-a ipaddress/fqdn -p 0(UDP)|1(TCP, default)|2(UnixSys) -b port(3600)  -c command -o options\n"\
		"\t\t Current command:  " \
		"\n\t\t\tplay, stop, pause, resume, forward, backforwar, subtitle, playerInfo, mediaInfo, " \
		"\n\t\t\tsawpWindow, rotateWindow, locateWindow, " IPCMD_NAME_ASPECT_WINDOW", vol+, vol-, "IPCMD_NAME_PLAYER_MUTE", "IPCMD_NAME_PLAYER_MUTE_ALL", "IPCMD_NAME_PLAYER_AUDIO", " \
		"\n\t\t\t"_MUX_ALERT_COMMAND", logo, osdEnable, osdBackground, osdTransparency, osdFontColor, osdFontSize, osdPosition, " IPCMD_NAME_OSD_INFO ", "\
		"\n\t\t\trecordStart, recordStop, recordStatus, recordPause, recordResume, "\
		"\n\t\t\tfiles, playlists, playlist, fileDelete, playlistDelete, playlistAdd, download," \
		"\n\t\t\t"CLIENT_CMD_NAME_SVR_CONFIG", "CLIENT_CMD_NAME_SVR_FEEDS", "CLIENT_CMD_NAME_SVR_CONNS", "CLIENT_CMD_NAME_SVR_URLS", " \
		"\n\t\t\t"CLIENT_CMD_NAME_WEB_INFO ", " \
		"\n\t\t\t"IPCMD_CEC_STAND_BY ", " IPCMD_CEC_IMAGE_VIEW_ON", " IPCMD_CEC_VOLUME_UP ", " IPCMD_CEC_VOLUME_DOWN ", " IPCMD_CEC_MUTE ", " IPCMD_CEC_INFO ", " \
		"\n\t\t\t"IPCMD_EDID_RESOLUTION ", " IPCMD_EDID_DEEP_COLOR", "  IPCMD_EDID_COLOR_SPACE", " \
		"\n\t\t\t"IPCMD_SYS_ADMIN_THREADS ", " IPCMD_SYS_ADMIN_VER_INFO", quit \n" \
		"\n\t\t windowIndex: default is 0; media : URL/Local file/Playlist; \n" \
		"\t\t ipaddress/fqdn: default localhost; -d duration: duration for recording; \n", 
		  base, base);

	if(!IS_STRING_NULL(apiClientOptionsPrompt(params)))
	{
		printf("\n\tOptions parameters for command '%s': '-o %s'\n\n", params->cmd, apiClientOptionsPrompt(params));
	}
#endif
	exit(-1);
}

struct API_CLIENT_CMD_HANDLER;

typedef	struct API_CLIENT_CMD_HANDLER
{
	char		*name;

	/* validate parameters of this command. return 0: success; others: fail */
	int	(*validate)(struct API_CLIENT_CMD_HANDLER *, struct API_PARAMETERS *, char *);

	/* execute this command */
	int	(*execute)(struct API_CLIENT_CMD_HANDLER *,  struct API_PARAMETERS *);
}API_CLIENT_CMD_HANDLER;


/**************** for play commands ******************/
static int _playStartValidate(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if(IS_STRING_NULL(params->media) )
	{
		printf("No 'media' for '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	if(params->index == -1)
	{
		printf("WARNS: No 'index' of window for the '%s' command; default is main window\n", params->cmd);
		params->index = 0;
	}
	if(params->repeatNumber == -1)
	{
		printf("WARNS: No 'repeat' of window for the '%s' command\n", params->cmd);
		params->repeatNumber = 1;
	}

	return EXIT_SUCCESS;
}

static int _playStartExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaPlay(params->index, params->media, params->repeatNumber );
	return EXIT_SUCCESS;
}

static int _validateIndex(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *Program)
{
	if(params->index == -1)
	{
		printf("WARNS: No 'index' of window for the '%s' command; default is main window\n", params->cmd);
		params->index = 0;
	}
	return EXIT_SUCCESS;
}
	
static int _playStopExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaStop(params->index);
	return EXIT_SUCCESS;
}

static int _playPauseExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaPause(params->index);
	return EXIT_SUCCESS;
}

static int _playResumeExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaResume(params->index);
	return EXIT_SUCCESS;
}


static int _playForwardExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaForward(params->index);
	return EXIT_SUCCESS;
}

static int _playBackforwardExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaBackward(params->index);
	return EXIT_SUCCESS;
}

static int _validateMedia(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if( IS_STRING_NULL(params->media) )
	{
		printf("No media for '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
		
static int _playSubtitleExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaSubtitle(params->media);
	return EXIT_SUCCESS;
}


/**************** for RECORD commands ******************/
static int _validateRecordStart(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if( IS_STRING_NULL(params->media) )
	{
		printf("No media for '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	
	if(params->duration == -1)
	{
		printf("WARNS: duration is not defined for '%s' command\n",params->cmd);
	}
	return EXIT_SUCCESS;
}		

static int _recordStartExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaRecordStart(params->media, params->duration);
	return EXIT_SUCCESS;
}

static int _recordStopExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaRecordStop();
	return EXIT_SUCCESS;
}

static int _recordStatusExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaRecordStatus();
	return EXIT_SUCCESS;
}

static int _recordPauseExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaRecordPause();
	return EXIT_SUCCESS;
}

static int _recordResumeExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaRecordResume();
	return EXIT_SUCCESS;
}


/**************** for OSD commands ******************/
static int _validateOsdBanner(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if( IS_STRING_NULL(params->media) )
	{
		printf("No 'message' (-m) for '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}

	if(params->color == -1)
	{
		printf("WARNS: No 'fontcolor' for the '%s' command, default green is used;\n", params->cmd);
		params->color = 0x0000FF00;
	}

	if(params->left != _BANNER_H_ALIGN_LEFT && params->left != _BANNER_H_ALIGN_CENTER && params->left != _BANNER_H_ALIGN_RIGHT )
	{
		printf("WARNS: '-"_MUX_ALERT_H_ALIGN"' for the '%s' command is wrong:%d, default Central Align is used;\n", params->cmd, params->left);
		params->left = _BANNER_H_ALIGN_CENTER;
	}

	if(params->width != _BANNER_V_ALIGN_TOP && params->width != _BANNER_V_ALIGN_CENTER && params->width != _BANNER_V_ALIGN_BOTTOM  )
	{
		printf("WARNS: '-"_MUX_ALERT_V_ALIGN"' for the '%s' command is wrong:%d, default Central Align is used;\n", params->cmd, params->width);
		params->width = _BANNER_V_ALIGN_CENTER;
	}

	return EXIT_SUCCESS;
}

static int _osdBannerExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaAlert(params->color, params->media, params->left, params->width );
	return EXIT_SUCCESS;
}

static int _validateOsdLogo(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if( IS_STRING_NULL(params->media) )
	{
		printf("No 'media' (icon) for '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static int _osdLogoExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaLogo(params->media);
	return EXIT_SUCCESS;
}

static int _validateOsdEnable(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if(params->index == -1)
	{
		printf("WARNS: No 'index' of OSD for the '%s' command;0:subtitle;1:alert; 2:logo, default is subtitle\n", params->cmd);
		params->index = 0;
	}
	return EXIT_SUCCESS;
}

static int _osdEnableExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaOsdIndex("osdEnable", params->index);
	return EXIT_SUCCESS;
}

static int _validateOsdBackground(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if(params->index == -1)
	{
		printf("WARNS: No 'index' of OSD for the '%s' command;0:subtitle;1:alert; 2:logo, default is subtitle\n", params->cmd);
		params->index = 0;
	}

#if BACKGROUND_AS_STRING	
	if( IS_STRING_NULL(params->backgroundColor))
	{
		printf("WARNS: No 'color' of OSD for the '%s' command;\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
#else
	if(params->color == -1)
	{
		printf("WARNS: No 'color' of OSD for the '%s' command;\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
#endif

	return EXIT_SUCCESS;
}
		

static int _validateOsdTransparency(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if(params->index == -1)
	{
		printf("WARNS: No 'index' of OSD for the '%s' command;0:subtitle;1:alert; 2:logo, default is subtitle\n", params->cmd);
		params->index = 0;
	}

	if(params->color == -1)
	{
		printf("WARNS: No 'color' of OSD for the '%s' command;\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
		
static int _osdBackgroundExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
#if BACKGROUND_AS_STRING	
	params->result = muxApiPlayMediaOsdBackground(params->index, params->backgroundColor );
#else
	params->result = muxApiPlayMediaOsdBackground(params->index, params->color);
#endif
	return EXIT_SUCCESS;
}

static int _osdTransparencyExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaOsdTranspanrency(params->index, params->color);
	return EXIT_SUCCESS;
}


static int _validateOsdFontcolor(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if(params->index != 0)
	{
		printf("WARNS: 'osdFontColor' command can only be used on subtitle\n");
		params->index = 0;
	}

	if(params->color == -1)
	{
		printf("WARNS: No 'color' of OSD for the '%s' command;\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
	
static int _osdFontColorExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaOsdFontColor(params->index, params->color);
	return EXIT_SUCCESS;
}

static int _validateOsdFontsize(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if(params->index == -1)
	{
		printf("WARNS: No 'index' of OSD for the '%s' command;0:subtitle;1:alert; 2:logo, default is subtitle\n", params->cmd);
		params->index = 0;
	}

	if(params->color == -1)
	{
		printf("WARNS: No 'fontsize' of OSD for the '%s' command;\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
	
static int _osdFontSizeExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaOsdFontSize(params->index, params->color);
	return EXIT_SUCCESS;
}

static int _validatePosition(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if(params->index == -1)
	{
		printf("WARNS: No 'index' of OSD for the '%s' command;0:subtitle;1:alert; 2:logo, default is subtitle\n", params->cmd);
		params->index = 0;
	}

	if(params->left == -1 || params->top==-1 || params->width == -1 || params->height == -1)
	{
		printf("WARNS: 'left', 'top', 'width' or 'height' of OSD for the '%s' command can not be -1, [(%d,%d),(%d, %d)];\n", 
			params->cmd, params->left, params->top, params->width, params->height);
		usage(program, params);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static int _osdPositionExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaOsdPosition(params->index, params->left, params->top, params->width, params->height);
	return EXIT_SUCCESS;
}

static int _osdInfoExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaOsdIndex(IPCMD_NAME_OSD_INFO, params->index);
	return EXIT_SUCCESS;
}


/**************** for WINDOW commands ******************/
static int _windowSwapExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaSwapWindow(params->index);
	return EXIT_SUCCESS;
}

static int _windowRotateExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaRotateWindow(params->index);
	return EXIT_SUCCESS;
}

static int _validateWindowPosition(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if(params->index == -1)
	{
		printf("WARNS: No 'index' of window for the 'locateWindow' command;default is main window(0)\n");
		params->index = 0;
	}

	if(params->left == -1 || params->top==-1 || params->width == -1 || params->height == -1)
	{
		printf("WARNS: 'left', 'top', 'width' or 'height' of window for the '%s' command can not be -1, [(%d,%d),(%d, %d)];\n", params->cmd, 
			params->left, params->top, params->width, params->height);
		usage(program, params);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
	
static int _windowPositionExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaLocateWindow(params->index, params->left, params->top, params->width, params->height);
	return EXIT_SUCCESS;
}

static int _validateWindowAspect(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if(params->index == -1)
	{
		printf("WARNS: No 'index' of window for the '%s' command;default is main window(0)\n", params->cmd );
		params->index = 0;
	}

	if(params->repeatNumber < 0 /*|| params->repeatNumber >=-1 */)
	{
		printf("WARNS: 'mode' of window for the '%s' command can not be %d\n", params->cmd, 	params->repeatNumber );
		usage(program, params);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}



static int _windowAspectExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayAspect(params->index, params->repeatNumber );
	return EXIT_SUCCESS;
}

	

/* other commands for PLAYER */
static int _miscPlayVolPlusExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaVolumeMinus(params->index);
	return EXIT_SUCCESS;
}
	
static int _miscPlayVolMinuxExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaVolumeMinus(params->index);
	return EXIT_SUCCESS;
}

static int _validateAudio(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if(params->index == -1)
	{
		printf("WARNS: No 'index' of window for the '%s' command; default is main window\n", params->cmd);
		params->index = 0;
	}

	if( IS_STRING_NULL(params->media) )
	{
		printf("No 'media' (YES|NO) for '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}

	if( !IS_STRING_EQUAL(params->media, "YES") 
		&&  !IS_STRING_EQUAL(params->media, "NO")  )
	{
		printf("'media' is not 'YES' or 'NO' for '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}
	

static int _miscPlayAudioExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayAudio(params->index, params->media);
	return EXIT_SUCCESS;
}


static int _miscPlayMuteExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaMute(params->index);
	return EXIT_SUCCESS;
}

static int _miscPlayMuteAllExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaMuteAll();
	return EXIT_SUCCESS;
}
	
	
static int _miscPlayInfoExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaPlayerInfo(params->index);
	return EXIT_SUCCESS;
}

static int _miscPlayMediaInfoExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaMediaInfo(params->index);
	return EXIT_SUCCESS;
}


/* MEDIA commands */
static int _mediaFilesExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiGetMediaGetAllFile();
	return EXIT_SUCCESS;
}

static int _mediaPlaylistsExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiGetMediaGetAllPlaylists();
	return EXIT_SUCCESS;
}

static int _mediaFileExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiGetMediaGetOneFile(params->media);
	return EXIT_SUCCESS;
}

static int _mediaPlaylistExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiGetMediaGetOnePlaylist(params->media);
	return EXIT_SUCCESS;
}

static int _validateMediaFiles(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if( cmn_list_size(&params->files) == 0 )
	{
		printf("No local file for '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
static int _mediaFileDeleteExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiSetMediaDeleteFile(&params->files);
	return EXIT_SUCCESS;
}

static int _validateMediaPlaylistDelete(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if( cmn_list_size(&params->files) == 0 )
	{
		printf("No playlist for '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static int _mediaPlaylistDeleteExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiSetMediaDeletePlaylist(&params->files);
	return EXIT_SUCCESS;
}

static int _validateMediaPlaylistAdd(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if( IS_STRING_NULL(params->media) )
	{
		printf("No name of playlist for '%s' command\n", params->cmd );
		usage(program, params);
		return EXIT_FAILURE;
	}
	if( cmn_list_size(&params->files) == 0 )
	{
		printf("No list of 'media' and 'duration' for '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
	
static int _mediaPlaylistAddExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiSetMediaAddPlaylist(params->media, &params->files);
	return EXIT_SUCCESS;
}
	


static int _validateMediaDownload(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *program)
{
	if(IS_STRING_NULL(params->ftpSvr) )
	{
		printf("WARNS: No 'FtpServer' is defined for the '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	if(IS_STRING_NULL(params->ftpUser) )
	{
		printf("WARNS: No 'FtpUser' is defined for the '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	if(IS_STRING_NULL(params->ftpPasswd) )
	{
		printf("WARNS: No 'FtpPassword' is defined for the '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	if(IS_STRING_NULL(params->ftpPath) )
	{
		printf("WARNS: No 'FtpPath' is defined for the '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}
	if(IS_STRING_NULL(params->media) )
	{
		printf("WARNS: No 'mediafile' is defined for the '%s' command\n", params->cmd);
		usage(program, params);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static int _mediaDownloadExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiDownloadMedia(params->ftpSvr, params->ftpUser, params->ftpPasswd, params->ftpPath, params->media);
	return EXIT_SUCCESS;
}

/* commands for CEC */
static int _clientCecStandbyExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiCecStandby();
	return EXIT_SUCCESS;
}

static int _clientCecImageOnExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiCecImageOn();
	return EXIT_SUCCESS;
}

static int _clientCecVolumeUpExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiCecVolumeUp();
	return EXIT_SUCCESS;
}

static int _clientCecVolumeDownExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiCecVolumeDown();
	return EXIT_SUCCESS;
}

static int _clientCecMuteExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiCecMute();
	return EXIT_SUCCESS;
}

static int _clientCecInfoExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiCecInfo();
	return EXIT_SUCCESS;
}

static int _edidResolutionExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiEdidResolution(params->media);
	return EXIT_SUCCESS;
}
	
static int _edidDeepColorExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiEdidColorDepth(params->color);
	return EXIT_SUCCESS;
}

static int _validateEdidColor(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *Program)
{
	if(params->color == -1)
	{
		printf("WARNS: No 'color' of deep color for the '%s' command\n", params->cmd);
		params->color = 0;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
	

static int _edidColorSpaceExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiEdidColorSpace(params->color);
	return EXIT_SUCCESS;
}
	
	
static int _validateEdidColorSpace(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params, char *Program)
{
	if(params->color == -1)
	{
		printf("WARNS: No 'color' of color space for the '%s' command\n", params->cmd);
		params->color = 0;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
	


/* commands for SERVER */
static int _clientSvrConfigExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiServerConfig();
	return EXIT_SUCCESS;
}

static int _clientSvrFeedsExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiServerFeeds();
	return EXIT_SUCCESS;
}
	
static int _clientSvrConnsExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiServerConns();
	return EXIT_SUCCESS;
}
	
static int _clientSvrUrlsExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiServerUrls();
	return EXIT_SUCCESS;
}


/* commands for WEB */	
static int _clientWebInfosExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiWebInfos();
	return EXIT_SUCCESS;
}
	
static int _clientSysAdminThreadsExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiSysAdminThreads();
	return EXIT_SUCCESS;
}
	
static int _clientSysAdminVerInfoExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiSysAdminVerInfo();
	return EXIT_SUCCESS;
}

static int _quitExec(struct API_CLIENT_CMD_HANDLER *handle, struct API_PARAMETERS *params)
{
	params->result = muxApiPlayMediaQuit();
	return EXIT_SUCCESS;
}


API_CLIENT_CMD_HANDLER apiClientCmdHandlers[]=
{
	/* PLAY commands */
	{
		.name = "play",
		.validate = _playStartValidate,
		.execute = _playStartExec
	},
	{
		.name = "stop",
		.validate = _validateIndex,
		.execute = _playStopExec
	},
	{
		.name = "pause",
		.validate = _validateIndex,
		.execute = _playPauseExec
	},
	{
		.name = "resume",
		.validate = _validateIndex,
		.execute = _playResumeExec
	},
	{
		.name = "forward",
		.validate = _validateIndex,
		.execute = _playForwardExec
	},
	{
		.name = "backward",
		.validate = _validateIndex,
		.execute = _playBackforwardExec
	},
	{/* 7 */
		.name = "subtitle",
		.validate = _validateMedia,
		.execute = _playSubtitleExec
	},

	/* RECORD commands */
	{
		.name = "recordStart",
		.validate = _validateRecordStart,
		.execute = _recordStartExec
	},
	{
		.name = "recordStop",
		.validate = NULL,
		.execute = _recordStopExec
	},
	{
		.name = "recordStatus",
		.validate = NULL,
		.execute = _recordStatusExec
	},
	{
		.name = "recordPause",
		.validate = NULL,
		.execute = _recordPauseExec
	},
	{/* 5 */
		.name = "recordResume",
		.validate = NULL,
		.execute = _recordResumeExec
	},

	/* OSD commands */
	{
		.name = _MUX_ALERT_COMMAND,
		.validate = _validateOsdBanner,
		.execute = _osdBannerExec
	},
	{
		.name = "logo",
		.validate = _validateOsdLogo,
		.execute = _osdLogoExec
	},
	{
		.name = "osdEnable",
		.validate = _validateOsdEnable,
		.execute = _osdEnableExec
	},
	
	{
		.name = "osdBackground",
		.validate = _validateOsdBackground,
		.execute = _osdBackgroundExec
	},
	{
		.name = "osdTransparency",
		.validate = _validateOsdTransparency,
		.execute = _osdTransparencyExec
	},
	{
		.name = "osdFontColor",
		.validate = _validateOsdFontcolor,
		.execute = _osdFontColorExec
	},	
	{
		.name = "osdFontSize",
		.validate = _validateOsdFontsize,
		.execute = _osdFontSizeExec
	},	
	{/* 7 */
		.name = "osdPosition",
		.validate = _validatePosition,
		.execute = _osdPositionExec
	},
	{/* */
		.name = IPCMD_NAME_OSD_INFO,
		.validate = _validateOsdEnable,
		.execute = _osdInfoExec
	},

	/* WINDOW commands */	
	{
		.name = "swapWindow",
		.validate = _validateIndex,
		.execute = _windowSwapExec
	},
	{
		.name = "rotateWindow",
		.validate = _validateIndex,
		.execute = _windowRotateExec
	},
	{/* 3 */
		.name = "locateWindow",
		.validate = _validateWindowPosition,
		.execute = _windowPositionExec
	},

	{
		.name = IPCMD_NAME_ASPECT_WINDOW,
		.validate = _validateWindowAspect,
		.execute = _windowAspectExec
	},

	/* MISC PLAY commands */	
	{
		.name = "vol+",
		.validate = _validateIndex,
		.execute = _miscPlayVolPlusExec
	},
	{
		.name = "vol-",
		.validate = _validateIndex,
		.execute = _miscPlayVolMinuxExec
	},
	{
		.name = IPCMD_NAME_PLAYER_AUDIO,
		.validate = _validateAudio,
		.execute = _miscPlayAudioExec
	},
	{
		.name = IPCMD_NAME_PLAYER_MUTE,
		.validate = _validateIndex,
		.execute = _miscPlayMuteExec
	},
	{
		.name = IPCMD_NAME_PLAYER_MUTE_ALL,
		.validate = NULL,
		.execute = _miscPlayMuteAllExec
	},

	{
		.name = IPCMD_EDID_RESOLUTION,
		.validate = _validateMedia,
		.execute = _edidResolutionExec
	},

	{
		.name = IPCMD_EDID_DEEP_COLOR,
		.validate = _validateEdidColor,
		.execute = _edidDeepColorExec
	},

	{
		.name = IPCMD_EDID_COLOR_SPACE,
		.validate = _validateEdidColorSpace,
		.execute = _edidColorSpaceExec
	},
	
	{
		.name = "playerInfo",
		.validate = _validateIndex,
		.execute = _miscPlayInfoExec
	},
	{/* 5 */
		.name = "mediaInfo",
		.validate = _validateIndex,
		.execute = _miscPlayMediaInfoExec
	},
	
	/* MEDIA commands */
	{
		.name = "files",
		.validate = NULL,
		.execute = _mediaFilesExec
	},
	{
		.name = "playlists",
		.validate = NULL,
		.execute = _mediaPlaylistsExec
	},
	{
		.name = "file",
		.validate = _validateMedia,
		.execute = _mediaFileExec
	},
	{
		.name = "playlist",
		.validate = _validateMedia,
		.execute = _mediaPlaylistExec
	},
	{
		.name = "fileDelete",
		.validate = _validateMediaFiles,
		.execute = _mediaFileDeleteExec
	},
	{
		.name = "playlistDelete",
		.validate = _validateMediaPlaylistDelete,
		.execute = _mediaPlaylistDeleteExec
	},
	{
		.name = "playlistAdd",
		.validate = _validateMediaPlaylistAdd,
		.execute = _mediaPlaylistAddExec
	},

		
	{/* 7 */
		.name = "download",
		.validate = _validateMediaDownload,
		.execute = _mediaDownloadExec
	},

	/* commands for CEC */
	{
		.name = IPCMD_CEC_STAND_BY,
		.validate = NULL,
		.execute = _clientCecStandbyExec
	},
	{
		.name = IPCMD_CEC_IMAGE_VIEW_ON,
		.validate = NULL,
		.execute = _clientCecImageOnExec
	},
	{
		.name = IPCMD_CEC_VOLUME_UP,
		.validate = NULL,
		.execute = _clientCecVolumeUpExec
	},
	{
		.name = IPCMD_CEC_VOLUME_DOWN,
		.validate = NULL,
		.execute = _clientCecVolumeDownExec
	},
	{
		.name = IPCMD_CEC_MUTE,
		.validate = NULL,
		.execute = _clientCecMuteExec
	},
	{
		.name = IPCMD_CEC_INFO,
		.validate = NULL,
		.execute = _clientCecInfoExec
	},


	/* commands for SERVER */
	{
		.name = CLIENT_CMD_NAME_SVR_CONFIG,
		.validate = NULL,
		.execute = _clientSvrConfigExec
	},
	{
		.name = CLIENT_CMD_NAME_SVR_FEEDS,
		.validate = NULL,
		.execute = _clientSvrFeedsExec
	},
	{
		.name = CLIENT_CMD_NAME_SVR_CONNS,
		.validate = NULL,
		.execute = _clientSvrConnsExec
	},
	{
		.name = CLIENT_CMD_NAME_SVR_URLS,
		.validate = NULL,
		.execute = _clientSvrUrlsExec
	},

	
	/* commands for WEB */
	{
		.name = CLIENT_CMD_NAME_WEB_INFO,
		.validate = NULL,
		.execute = _clientWebInfosExec
	},


	/* commands for Sys Admin */
	{
		.name = IPCMD_SYS_ADMIN_THREADS,
		.validate = NULL,
		.execute = _clientSysAdminThreadsExec
	},
	
	{
		.name = IPCMD_SYS_ADMIN_VER_INFO,
		.validate = NULL,
		.execute = _clientSysAdminVerInfoExec
	},

	{
		.name = "quit",
		.validate = NULL,
		.execute = _quitExec
	},

	{
		.name = NULL,
		.validate = NULL,
		.execute = NULL
	}
};

static int	_apiFindCmd(struct API_PARAMETERS *params, char *programName)
{
	API_CLIENT_CMD_HANDLER *handle = apiClientCmdHandlers;

	while(handle->execute )
	{
		if(IS_STRING_EQUAL(handle->name, params->cmd) )
		{
			return EXIT_SUCCESS;
		}
		
		handle++;
	}
	return EXIT_FAILURE;
}



static int	_apiHandleCmd(struct API_PARAMETERS *params, char *programName)
{
	int ret = EXIT_SUCCESS;

	API_CLIENT_CMD_HANDLER *handle = apiClientCmdHandlers;

	while(handle->execute )
	{
		if(IS_STRING_EQUAL(handle->name, params->cmd) )
		{
			if( handle->validate==NULL ||  (handle->validate)(handle, params, programName ) == EXIT_SUCCESS)
			{
					CTRL_LINK_TYPE linkType = CTRL_LINK_UDP;
					
					if(params->protocol == PROTOCOL_TCP)
						linkType = CTRL_LINK_TCP;
					else if(params->protocol == PROTOCOL_UNIX)
						linkType = CTRL_LINK_UNIX;
					
					fprintf(stderr, "Client connectting to %s:%d on %s protocol.....\n", params->address, params->port, (linkType == CTRL_LINK_TCP)?"TCP":(linkType == CTRL_LINK_UDP)?"UDP":"Unix");
					ret = cmnMuxClientInit(params->port, linkType, params->address);
					if(ret == EXIT_FAILURE)
					{
						printf("JSON API initialization failed, please check your configuration and server address\n");
						exit(1);
						return EXIT_FAILURE;
					}

					fprintf(stderr, "Client execute command of '%s'.....\n", params->cmd);
					return (handle->execute)(handle, params);
			}
			else
			{
				return EXIT_FAILURE;
			}
			
		}
		
		handle++;
	}

	printf("WARNS: '%s' command is not found\n\n", params->cmd);
	usage(programName, params);
	return ret;
}


int main(int argc, char *argv[])
{
	int res = EXIT_SUCCESS;
	int opt;

	struct	API_PARAMETERS params;

	memset(&params, 0, sizeof(struct API_PARAMETERS));

	cmn_list_init(&params.files);
	snprintf(params.address, sizeof(params.address), "%s", "127.0.0.1");
	params.port = 3600;
	params.protocol = PROTOCOL_TCP;
	params.index = -1;
	
	params.left = -1;
	params.width = -1;
	params.height = -1;
	params.duration = -1;
	params.color = -1;

//	while ((opt = getopt (argc, argv, "a:p:b:c:i:m:l:t:w:h:a:d:C:s:u:f:P:p:b:")) != -1)
	while ((opt = getopt (argc, argv, "a:p:b:c:o:")) != -1)
	{
		switch (opt)
		{
			case 'a':
				snprintf(params.address, sizeof(params.address), "%s",optarg);
				break;

			case 'p':
				params.protocol = atoi(optarg);
				if(params.protocol >= PROTOCOL_UNKNOWN)
					params.protocol = PROTOCOL_UDP;
				break;

			case 'b':
				params.port = atoi(optarg);
				break;

			case 'c':
				snprintf(params.cmd, sizeof(params.cmd), "%s", optarg);
				break;

			case 'o':
				if(IS_STRING_NULL(params.cmd))
				{
					fprintf(stderr, "No command defined before options for the command, use -c CMD before -o OPTIONS\n");
					exit(1);
				}

				res = _apiFindCmd(&params, argv[0]);
				if(res ==  EXIT_FAILURE)
				{
					printf("Unknow command '%s' \n", params.cmd);
					usage(argv[0], &params);
				}

				res = apiClientParseSubOptions(optarg, &params);
				if(res )
				{
					exit(1);
				}
				break;


#if 0				
			case 'C':
				params.color = atoi(optarg);
				break;

			case 'i':
				params.index = atoi(optarg);
				break;

			case 'm':
				snprintf(params.media, sizeof(params.media), "%s", optarg);
				break;

			case 'l':
				params.left = atoi(optarg);
				break;

			case 't':
				params.top = atoi(optarg);
				break;

			case 'w':
				params.width = atoi(optarg);
				break;

			case 'h':
				params.height = atoi(optarg);
				break;

			case 'd':
				params.duration = atoi(optarg);
				break;
				

			case 's':
				snprintf(params.ftpSvr, sizeof(params.ftpSvr), "%s",optarg);
				break;
			case 'u':
				snprintf(params.ftpUser, sizeof(params.ftpUser), "%s",optarg);
				break;
			case 'f':
				snprintf(params.ftpPasswd, sizeof(params.ftpPasswd), "%s",optarg);
				break;
			case 'P':
				snprintf(params.ftpPath, sizeof(params.ftpPath), "%s",optarg);
				break;
#endif

/*
			case 'p':
				address = atoi(optarg);
				break;
*/
			default:
				usage(argv[0], &params);
		}

	}

//	res = cmnMuxPlayerParseConfig(MUX_PLAYER_CONFIG_FILE, &_cfg);

	printf(CMN_VERSION_INFO(CMN_MODULE_APICLIENT_NAME));

	if( IS_STRING_NULL(params.cmd) )
	{
		printf("Command is not defined\n");
		usage(argv[0], &params);
	}
	

	/* res is EXIT_SUCESS when command is handled by this function; params->result is NULL when command failed */
	res = _apiHandleCmd(&params, argv[0]);
	if(res ==  EXIT_FAILURE)
	{
		printf("Unknow command '%s' \n", params.cmd);
		usage(argv[0], &params);
	}

	if(! params.result )
	{
		return 1;
	}
	
	res = muxApiGetStatus(params.result);
	if( res == 200)
	{
		printf("Command '%s' success\n", params.cmd);
	}
	else
	{
		char *printed_json = cJSON_Print(params.result);
		printf("Command '%s' failed: '%s'\n", params.cmd, printed_json);
		free(printed_json);
		/* bash script will check it to determine whether go on or not */
		return 1;
	}
	
	return 0;
}

