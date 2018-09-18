
#include <ctype.h>


#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

static int _parseGlobalConfig(const char **p, MuxMain *muxMain, int linenum)
{
	char cmd[64];
	char arg[1024];
	int val;
	int ret = EXIT_SUCCESS;
	
	cmnParseGetArg(cmd, sizeof(cmd), p);
	
	/***** configuration for log *****/
	if (!strcasecmp(cmd, "MuxBoard"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);

		if (!strcasecmp(arg, "RX769"))
		{
			muxMain->boardType = MUX_BOARD_TYPE_RX769;
		}
		else
		{
			muxMain->boardType = MUX_BOARD_TYPE_UNKNOWN;
		}	
	}
	else if (!strcasecmp(cmd, "AvSyncType"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
#if 1
		muxMain->mediaCaptureConfig.avSyncType = CMN_MUX_FIND_SYNC_TYPE(arg);
#else
		if (!strcasecmp(arg, "None"))
		{
			muxMain->mediaCaptureConfig.avSyncType = MUX_SYNC_MASTER_NONE;
		}
		else if (!strcasecmp(arg, "Audio"))
		{
			muxMain->mediaCaptureConfig.avSyncType = MUX_SYNC_MASTER_AUDIO;
		}
		else if (!strcasecmp(arg, "Video"))
		{
			muxMain->mediaCaptureConfig.avSyncType = MUX_SYNC_MASTER_VIDEO;
		}
		else if (!strcasecmp(arg, "System"))
		{
			muxMain->mediaCaptureConfig.avSyncType = MUX_SYNC_MASTER_SYSTEM;
		}
		else
		{
			muxMain->mediaCaptureConfig.avSyncType = MUX_SYNC_MASTER_NONE;
		}	
#endif		
	}
	else if (!strcasecmp(cmd, "Daemon"))
	{
		muxMain->muxLog.isDaemonized = cmnParseGetBoolValue(p);
#if DEBUG_CONFIG_FILE
		if(muxMain->muxLog.isDaemonized )
			fprintf(stderr, "running as daemon\n");
		else
			fprintf(stderr, "running as front-end\n");
#endif
	}
	else if (!strcasecmp(cmd, "CustomLog"))
	{
		cmnParseGetArg(muxMain->muxLog.logFileName, 256, p);
		if (!strcmp(muxMain->muxLog.logFileName, "-"))
		{
			muxMain->muxLog.lstyle = USE_CONSOLE;
		}
	}
	else if (!strcasecmp(cmd, "MaxLogSize"))
	{
		muxMain->muxLog.maxSize = 2048*UNIT_OF_KILO;//cmnParseGetIntValue(p);
	}
	else if (!strcasecmp(cmd, "DebugLevel"))
	{
		val = cmnParseGetIntValue(p);
		if(val < CMN_LOG_EMERG || val > CMN_LOG_DEBUG)
		{
			val = CMN_LOG_NOTICE;
		}
		muxMain->muxLog.llevel = val;
	}

	else if (!strcasecmp(cmd, "UDPCtrlPort"))
	{
		muxMain->udpCtrlPort = cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "Controller UDP Port: %d\n", muxMain->udpCtrlPort );
#endif
	}
	else if (!strcasecmp(cmd, "TCPCtrlPort"))
	{
		muxMain->tcpCtrlPort = cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "Controller TCP Port: %d\n", muxMain->tcpCtrlPort );
#endif
	}
	else if (!strcasecmp(cmd, "UNIXPort"))
	{
		cmnParseGetArg(muxMain->unixPort, CMN_NAME_LENGTH, p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "Controller UNIX Port: %s\n", muxMain->unixPort);
#endif
	}
#if 0	
	else if (!strcasecmp(cmd, "CtrlProtocol"))
	{
		muxMain->ctrlProtocol = CTRL_LINK_UDP;

		cmnParseGetArg(arg, sizeof(arg), p);
		if (strcasecmp(arg, "UDP") == 0)
		{
			muxMain->ctrlProtocol = CTRL_LINK_UDP;
		}
		else if (strcasecmp(arg, "TCP") == 0)
		{
			muxMain->ctrlProtocol = CTRL_LINK_TCP;
		}
		else if (strcasecmp(arg, "UNIX") == 0)
		{
			muxMain->ctrlProtocol = CTRL_LINK_UNIX;
		}
		else if (strcasecmp(arg, "MULTI") == 0)
		{
			muxMain->ctrlProtocol = CTRL_LINK_MULTICAST;
		}
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "Controller Protocol: %s(%d)\n", arg, muxMain->ctrlProtocol );
#endif
	}
#endif

	/***** configuration for Player *****/
	else if (!strcasecmp(cmd, "SDHomeDir"))
	{
		cmnParseGetArg(muxMain->mediaCaptureConfig.sdHome, CMN_NAME_LENGTH, p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "SDHomeDir: %s\n", muxMain->mediaCaptureConfig.sdHome );
#endif
	}
	else if (!strcasecmp(cmd, "USBHomeDir"))
	{
		cmnParseGetArg(muxMain->mediaCaptureConfig.usbHome, CMN_NAME_LENGTH, p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "USBHomeDir: %s\n", muxMain->mediaCaptureConfig.usbHome );
#endif
	}
	else if (!strcasecmp(cmd, "AspectRatioWindow"))
	{
		muxMain->mediaCaptureConfig.aspectRatioWindow = cmnParseGetBoolValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "AspectRatioWindow: %d\n", muxMain->mediaCaptureConfig.aspectRatioWindow);
#endif
	}

	/***** configuration items for recorder *****/
	else if (!strcasecmp(cmd, "RecordDevice"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);

		if (!strcasecmp(arg, "SDCard"))
		{
			muxMain->mediaCaptureConfig.storeType = MEDIA_DEVICE_SDCARD;
		}
		else
		{
			muxMain->mediaCaptureConfig.storeType = MEDIA_DEVICE_USBDISK;
		}	
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "RecordDevice: %d('%s')\n", muxMain->mediaCaptureConfig.storeType, arg );
#endif
	}

	/***** configuration for record video *****/
	else if (!strcasecmp(cmd, "CaptureVideoFormat"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		muxMain->mediaCaptureConfig.videoType = CMN_MUX_FIND_VIDEO_OUT_FORMAT(arg);
		if(muxMain->mediaCaptureConfig.videoType == -1)
		{
			muxMain->mediaCaptureConfig.videoType = OUT_VIDEO_FORMAT_H264_HIGH;
			fprintf(stderr, "Capture video format error: '%s' on line %d\n", arg, linenum);
		}
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "Capture video format: %d\n", muxMain->mediaCaptureConfig.videoType);
#endif
	}
	else if (!strcasecmp(cmd, "CaptureVideoCapLevel"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		muxMain->mediaCaptureConfig.videoCapLevel= CMN_MUX_FIND_VIDEO_OUT_CAPLEVEL(arg);
		if(muxMain->mediaCaptureConfig.videoCapLevel == -1)
		{
			muxMain->mediaCaptureConfig.videoCapLevel = OUT_VIDEO_SIZE_720P;
			fprintf(stderr, "Capture video caplevel param error: '%s' on line %d\n", arg, linenum);
		}

#if DEBUG_CONFIG_FILE
		fprintf(stderr, "Capture Caplevel: %d\n", muxMain->mediaCaptureConfig.videoCapLevel);
#endif
	}
	else if (!strcasecmp(cmd, "CaptureVideoBitRate"))
	{
		muxMain->mediaCaptureConfig.videoBitrate = cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "CaptureVideoBitRate: %d\n", muxMain->mediaCaptureConfig.videoBitrate);
#endif
	}
	else if (!strcasecmp(cmd, "CaptureVideoGOP"))
	{
		muxMain->mediaCaptureConfig.gop= cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "CaptureVideoGOP: %d\n", muxMain->mediaCaptureConfig.gop);
#endif
	}
	else if (!strcasecmp(cmd, "CaptureVideoFrameRate"))
	{
		muxMain->mediaCaptureConfig.outputFrameRate= cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "CaptureFrameRate: %d\n", muxMain->mediaCaptureConfig.outputFrameRate);
#endif
	}

	/***** configuration for capture audio *****/
	else if (!strcasecmp(cmd, "CaptureAudioType"))
	{
		muxMain->mediaCaptureConfig.audioType= 0;// cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "CaptureAudioType: %d\n", muxMain->mediaCaptureConfig.audioType);
#endif
	}
	else if (!strcasecmp(cmd, "CaptureAudioSampleRate"))
	{
		muxMain->mediaCaptureConfig.audioSampleRate = cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "CaptureAudioSampleRate: %d\n", muxMain->mediaCaptureConfig.audioSampleRate);
#endif
	}
	else if (!strcasecmp(cmd, "CaptureAudioFormat"))
	{
		muxMain->mediaCaptureConfig.audioFormat= cmnParseGetIntValue(p); /* 32 or 16 */
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "CaptureAudioFormat: %d\n", muxMain->mediaCaptureConfig.audioFormat);
#endif
	}
	else if (!strcasecmp(cmd, "CaptureAudioChannels"))
	{
		muxMain->mediaCaptureConfig.audioChannels= cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "CaptureAudioChannels: %d\n", muxMain->mediaCaptureConfig.audioChannels);
#endif
	}

	/* stream description */
	else if (!strcasecmp(cmd, "Author"))
	{
		cmnParseGetArg(muxMain->mediaDescription.author, sizeof(muxMain->mediaDescription.author),  p);
	}
	else if (!strcasecmp(cmd, "Comment"))
	{
		cmnParseGetArg(muxMain->mediaDescription.comment, sizeof(muxMain->mediaDescription.comment), p);
	}
	else if (!strcasecmp(cmd, "Copyright"))
	{
		cmnParseGetArg(muxMain->mediaDescription.copyright, sizeof(muxMain->mediaDescription.copyright), p);
	}
	else if (!strcasecmp(cmd, "Title"))
	{
		cmnParseGetArg(muxMain->mediaDescription.title, sizeof(muxMain->mediaDescription.title), p);
	}
	else
	{
		fprintf(stderr, "Invalidate configuration item: %s on line %d\n", cmd, linenum);
		ret = EXIT_FAILURE;
	}


	return ret;
}


static PLAY_ITEM *playItem = NULL;

static int _parsePlaylist(PLAY_LIST *playlist, const char **p, int linenum)
{
	char cmd[64];
	char arg[1024];
	int	ret = EXIT_SUCCESS;

	if(playlist==NULL)
		return EXIT_FAILURE;

	cmnParseGetArg(cmd, sizeof(cmd), p);

	if (!strcasecmp(cmd, "Repeat"))
	{
		playlist->repeat = cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "Repeat: %d\n", playlist->repeat);
#endif
	}
	else if (!strcasecmp(cmd, "PlayTime"))
	{
		unsigned h, m, d;

		cmnParseGetArg(arg, sizeof(arg), p);
		int cnt = sscanf(arg, "%2d:%2d/%2d", &h, &m, &d);
		if (cnt != 3 )
		{
			fprintf(stderr, "Playlist invalidate format: %s(cnt=%d,h:%d, m:%d, d:%d) on line %d\n", arg, cnt, h, m, d, linenum);
			playlist->hour = -1;
			playlist->minute = -1;
			playlist->dayOfWeek = -1;
		}
		else if ((h >= 24 || m >= 60 || d > 7) )
		{
			fprintf(stderr, "Playlist invalidate data: %2d:%2d %d on line %d\n", h, m, d, linenum);
			playlist->hour = -1;
			playlist->minute = -1;
			playlist->dayOfWeek = -1;
		}
		else
		{
			playlist->hour = h;
			playlist->minute = m;
			playlist->dayOfWeek = d;
		}
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "PlayTime: %2d:%2d %d\n", playlist->hour, playlist->minute, playlist->dayOfWeek);
#endif
	}
	else if (!strcasecmp(cmd, "Enable"))
	{
		playlist->enable = cmnParseGetBoolValue(p);
	}
	else if (!strcasecmp(cmd, PLAYLIST_NAME_URL))
	{/*  */
		if(playItem != NULL)
		{
			fprintf(stderr, "Missing value of '%s' for '%s' on line %d\n", PLAYLIST_NAME_DURATION, playItem->filename, linenum );
			return EXIT_FAILURE;
		}
		cmnParseGetArg(arg, sizeof(arg), p);
		
		playItem = cmn_malloc(sizeof(PLAY_ITEM));
		snprintf(playItem->filename, CMN_NAME_LENGTH, "%s", arg);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "%s: %s\n", PLAYLIST_NAME_URL, playItem->filename);
#endif
	}
	else if (!strcasecmp(cmd, PLAYLIST_NAME_DURATION ))
	{/*  */
		if(playItem == NULL)
		{
			fprintf(stderr, "Missing value of '%s' before '%s' on line %d\n", PLAYLIST_NAME_URL, PLAYLIST_NAME_DURATION, linenum);
			return EXIT_FAILURE;
		}
		
		playItem->duration = cmnParseGetIntValue(p);
		if(playItem->duration < 0 )
		{
			fprintf(stderr, "'%s' is invalidate value %d on line %d\n", PLAYLIST_NAME_DURATION, playItem->duration, linenum );
			return EXIT_FAILURE;
		}
		
		cmn_list_append(&playlist->fileList, playItem);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "%s: %s; %s: %d seconds\n", PLAYLIST_NAME_URL, playItem->filename, PLAYLIST_NAME_DURATION, playItem->duration);
#endif
		playItem = NULL;
		
	}
	else
	{
		fprintf(stderr, "Invalidate configuration item: %s on line %d\n", cmd, linenum);
		ret = EXIT_FAILURE;
	}
	
	return ret;
}

static int _parsePlugin(MuxPlugIn *plugin, const char **p, int linenum)
{
	char cmd[64];
	int	ret = EXIT_SUCCESS;

	if(plugin==NULL)
		return EXIT_FAILURE;

	cmnParseGetArg(cmd, sizeof(cmd), p);

	if (!strcasecmp(cmd, "Enable"))
	{
		plugin->enable = cmnParseGetBoolValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "\tPlugin Enable: %s\n", (plugin->enable==FALSE)?"NO":"YES");
#endif
	}
	else if (!strcasecmp(cmd, "Lib"))
	{
		cmnParseGetArg(plugin->path, sizeof(plugin->path),  p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "\tPlugin Library: %s\n", plugin->path);
#endif
	}
	else
	{
		fprintf(stderr, "Invalidate configuration item: %s on line %d\n", cmd, linenum);
		ret = EXIT_FAILURE;
	}
	
	return ret;
}


int cmnMuxMainParse(const char *filename, MuxMain *muxMain)
{
#define	PARSE_GLOBAL		0
#define	PARSE_PLUGIN		1
#define	PARSE_PLAYLIST		2

	FILE *f;
	char line[1024];
	char cmd[64];
	const char *p;
	int line_num = 0;
	int parseState = PARSE_GLOBAL;
	int ret = EXIT_SUCCESS;

	MuxPlugIn	*plugin = NULL;
	PLAY_LIST	*playlist = NULL;
	
	f = fopen(filename, "r");
	if (!f)
	{
		perror(filename);
		return EXIT_FAILURE;
	}

	/** it must be initialized as all 0s before enter into this function */
//	memset(cfg, 0, sizeof(MuxPlayerConfig));	

	muxMain->muxLog.lfacility = CMN_LOG_7;
	muxMain->muxLog.lstyle = USE_FILE;
	muxMain->muxLog.llevel  = CMN_LOG_DEBUG;
	muxMain->muxLog.isDaemonized = TRUE;


	muxMain->mediaCaptureConfig.inputFrameRate  = 50;
	muxMain->mediaCaptureConfig.outputFrameRate = 30;

	muxMain->mediaCaptureConfig.audioType = 0; /* output audio type, only AAC is supported by HW, HA_CODEC_ID_E */
	muxMain->mediaCaptureConfig.audioSampleRate = 48000;
	muxMain->mediaCaptureConfig.audioFormat = 32;  /*32 bit or 16 bit per sample */
	muxMain->mediaCaptureConfig.audioChannels = 2;	/* 2 or 1 */

	cmn_list_init(SYS_PLAYLISTS(muxMain) );
	muxMain->playlistLock = cmn_mutex_init();

	snprintf(muxMain->configFileName, CMN_NAME_LENGTH, "%s", filename );

	for(;;)
	{
		if (fgets(line, sizeof(line), f) == NULL)
			break;
		
		line_num++;
		p = line;
		while (isspace(*p)) 
			p++;
		
		if (*p == '\0' || *p == '#')
			continue;


		if ( strstr(line, "<PlugIn") )
		{
			char *q;

			cmnParseGetArg(cmd, sizeof(cmd), &p);
			if (plugin )
			{
				fprintf(stderr, "%s:%d: Already in a tag for <plugin>\n",filename, line_num);
				ret = EXIT_FAILURE;
				break;
			}
			else
			{
				plugin = cmn_malloc(sizeof(MuxPlugIn));
				plugin->muxMain = muxMain;

				
				cmnParseGetArg(plugin->name, sizeof(plugin->name), &p);
				q = strrchr(plugin->name, '>');
				if (*q)
					*q = '\0';
				
				if (!strcasecmp(plugin->name, "PLAYER"))
				{
					plugin->type = MUX_PLUGIN_TYPE_PLAYER;
				}
				else if (!strcasecmp(plugin->name, "SERVER"))
				{
					plugin->type = MUX_PLUGIN_TYPE_SERVER;
				}
				else if (!strcasecmp(plugin->name, "WEB"))
				{
					plugin->type = MUX_PLUGIN_TYPE_WEB;
				}
				else if (!strcasecmp(plugin->name, "RECORDER"))
				{
					plugin->type = MUX_PLUGIN_TYPE_RECORDER;
				}
				else
				{
					plugin->type = MUX_PLUGIN_TYPE_UNKNOWN;
					fprintf(stderr, "Unknow plugin type '%s'\n", plugin->name);
				}
#if DEBUG_CONFIG_FILE
				fprintf(stderr, "PlugIn: %s\n", plugin->name );
#endif
			}

			parseState = PARSE_PLUGIN;

			continue;
		}
		else if ( strstr(line, "</PlugIn>"))
		{
			if (!plugin)
			{
				fprintf(stderr, "%s:%d: No corresponding <PlugIn> for </PlugIn>\n", filename, line_num);
				ret = EXIT_FAILURE;
				break;
			}

			ADD_ELEMENT(muxMain->plugins, plugin);

			plugin = NULL;
			parseState = PARSE_GLOBAL;
			continue;
		}

		if ( strstr(line, "<PlayList") )
		{
			char *q;

			cmnParseGetArg(cmd, sizeof(cmd), &p);
			if (playlist )
			{
				fprintf(stderr, "%s:%d: Already in a tag for <Window>\n", filename, line_num);
				ret = -EXIT_FAILURE;
				break;
			}
			else
			{
				playlist = cmn_malloc(sizeof(PLAY_LIST));
				playlist->muxMain = muxMain;
				cmn_list_init( &playlist->fileList);

				cmnParseGetArg(playlist->name, sizeof(playlist->name), &p);
				q = strrchr(playlist->name, '>');
				if (*q)
					*q = '\0';
				
				cmn_list_append( SYS_PLAYLISTS(muxMain), playlist);
			}

			parseState = PARSE_PLAYLIST;

			continue;
		}
		else if ( strstr(line, "</PlayList>"))
		{
			if (!playlist)
			{
				fprintf(stderr, "%s:%d: No corresponding <PlayList> for </PlayList>\n", filename, line_num);
				ret = EXIT_FAILURE;
				break;
			}

			playlist = NULL;
			parseState = PARSE_GLOBAL;
			continue;
		}

		
		if(parseState == PARSE_GLOBAL)
		{
			ret = _parseGlobalConfig( &p, muxMain, line_num);
		}
		else if(parseState == PARSE_PLAYLIST)
		{
			ret = _parsePlaylist(playlist, &p, line_num);
		}
		else
		{
			ret = _parsePlugin(plugin, &p, line_num);
		}
		
		if(ret == -EXIT_FAILURE )
		{
			fprintf(stderr, "%s:%d: Incorrect keyword: '%s'\n", filename, line_num, cmd);
			break;
		}
		
	}

	fclose(f);

	return ret;
}


