
 #define _GNU_SOURCE  
 #include <string.h>

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

TYPE_NAME_T outVideoCaplevels[] =
{
	{
		.type = OUT_VIDEO_SIZE_CIF,
		.name = "CIF(352x288)"	,
		.value = NULL,
	},
	{
		.type = OUT_VIDEO_SIZE_D1,
		.name = "D1(720x576)"	,
		.value = NULL,
	},
	{
		.type = OUT_VIDEO_SIZE_720P,
		.name = "720P(1280x720)"	,
		.value = NULL,
	},
	{
		.type = OUT_VIDEO_SIZE_FULLHD,
		.name = "FULLHD(1920x1080)"	,
		.value = NULL,
	},
	{
		.type = OUT_VIDEO_SIZE_4K,
		.name = "4K(3840x2160)"	,
		.value = NULL,
	},
	{
		.type = -1,
		.name = NULL,
		.value = NULL,
	},
};


TYPE_NAME_T outVideoFormats[] =
{
	{
		.type = OUT_VIDEO_FORMAT_H264_BASELINE,
		.name = "H264_BASE",
		.value = NULL,
	},
	{
		.type = OUT_VIDEO_FORMAT_H264_MAIN,
		.name = "H264_MAIN",
		.value = NULL,
	},
	{
		.type = OUT_VIDEO_FORMAT_H264_EXTENDED,
		.name = "H264_EXTENDED",
		.value = NULL,
	},
	{
		.type = OUT_VIDEO_FORMAT_H264_HIGH,
		.name = "H264_HIGH",
		.value = NULL,
	},
	{
		.type = OUT_VIDEO_FORMAT_HEVC,
		.name = "HEVC",
		.value = NULL,
	},
	{
		.type = -1,
		.name = NULL,
		.value = NULL,
	},
};



TYPE_NAME_T rotateTypes[] =
{
	{
		.type = ROTATE_TYPE_0,
		.name = "0'",
		.value = NULL,
	},
	{
		.type = ROTATE_TYPE_90,
		.name = "90'",
		.value = NULL,
	},
	{
		.type = ROTATE_TYPE_180,
		.name = "180",
		.value = NULL,
	},
	{
		.type = ROTATE_TYPE_270,
		.name = "270",
		.value = NULL,
	},
	{
		.type = -1,
		.name = NULL,
		.value = NULL,
	},
};



TYPE_NAME_T jsonErrors[] =
{
	{
		.type = IPCMD_ERR_IN_PROCESSING,
		.name = "InProcess",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_NOERROR,
		.name = "NoError",
		.value = NULL,
	},
	
	{
		.type = IPCMD_ERR_COMMUNICATION,
		.name = "Error communication",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_JSON_CORRUPTED,
		.name = "CORRUPTED-JSON-DATA-RECEIVED",
		.value = NULL,
	},
	
	{
		.type = IPCMD_ERR_MAC_MISMATCH,
		.name = "MAC-MISMATCH",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_CRC_FAIL,
		.name = "CRC-FAILED",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_WRONG_PROTOCOL,
		.name = "WRONG_PROTOCOL",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_DATA_ERROR,
		.name = "Data Item Error",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_NOT_SUPPORT_COMMND,
		.name = "Unknown Command",
		.value = NULL,
	},
	
	{
		.type = IPCMD_ERR_IS_BUSY_NOW,
		.name = "Unknown Command",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_SERVER_INTERNEL_ERROR,
		.name = "Server internal error",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_PLUGIN_NOT_EXISTS,
		.name = "Module not exist or not load",
		.value = NULL,
	},

	{
		.type = IPCMD_ERR_PLUGIN_PLAYER_TIMEOUT,
		.name = "Timeout when try to play",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_PLUGIN_PLAYER_FAILED,
		.name = "Play fails, media URL is wrong or invalidate format",
		.value = NULL,
	},
	
	{
		.type = IPCMD_ERR_FTP_SERVER_ERROR,
		.name = "can't connect to server",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_FTP_USER_PASSWORD,
		.name = "username or password is wrong",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_FTP_NO_FREESPACE,
		.name = "no space in RX, can't make directory for the file",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_FTP_NAME_COLLISION,
		.name = "name collision, filename is a local file or local directory",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_FTP_DATA_ERROR,
		.name = "failed when get data from remote file or save data as local file",
		.value = NULL,
	},
	{
		.type = IPCMD_ERR_FTP_PARTLY_FAILED,
		.name = "some files failed, some successed in file list",
		.value = NULL,
	},

		
	{
		.type = IPCMD_ERR_UNKNOWN,
		.name = "Unknown Command",
		.value = NULL,
	},
	
	{
		.type = -1,
		.name = NULL,
		.value = NULL,
	},
};


typedef	struct	_CfgTypes
{
	int		type;
	char		*name;
}CfgType;


TYPE_NAME_T cmnMuxSyncTypes[] =
{
	{
		.type = MUX_SYNC_MASTER_NONE,
		.name = "None",	
		.value = NULL,
	},
	{
		.type = MUX_SYNC_MASTER_AUDIO,
		.name = "Audio",	
		.value = NULL,
	},
	{
		.type = MUX_SYNC_MASTER_VIDEO,
		.name = "Video",	
		.value = NULL,
	},
	{
		.type = MUX_SYNC_MASTER_SYSTEM,
		.name = "System",	
		.value = NULL,
	},
	{
		.type = MUX_SYNC_MASTER_UNKNOWN,
		.name = NULL,	
		.value = NULL,
	}
};



TYPE_NAME_T cmnMuxAudioCaptureTypes[] =
{
	{
		.type = MUX_SYNC_MASTER_NONE,
		.name = "None",	
		.value = NULL,
	},
	{
		.type = MUX_SYNC_MASTER_AUDIO,
		.name = "Audio",	
		.value = NULL,
	},
	{
		.type = MUX_SYNC_MASTER_VIDEO,
		.name = "Video",	
		.value = NULL,
	},
	{
		.type = MUX_SYNC_MASTER_SYSTEM,
		.name = "System",	
		.value = NULL,
	},
	{
		.type = MUX_SYNC_MASTER_UNKNOWN,
		.name = NULL,	
		.value = NULL,
	}
};


TYPE_NAME_T cmnMuxPluginTypes[] =
{
	{
		.type = MUX_PLUGIN_TYPE_MAIN,
		.name = "Main",	
		.value = NULL,
	},
	{
		.type = MUX_PLUGIN_TYPE_PLAYER,
		.name = "PLAYER",	
		.value = NULL,
	},
	{
		.type = MUX_PLUGIN_TYPE_SERVER,
		.name = "SERVER",	
		.value = NULL,
	},
	{
		.type = MUX_PLUGIN_TYPE_WEB,
		.name = "WEB",	
		.value = NULL,
	},
	{
		.type = MUX_PLUGIN_TYPE_RECORDER,
		.name = "RECORDER",	
		.value = NULL,
	},
	{
		.type = MUX_PLUGIN_TYPE_UNKNOWN,
		.name = NULL,	
		.value = NULL,
	}
};



int	cmnMuxTypeFindType(TYPE_NAME_T *types, char *name)
{
	TYPE_NAME_T *tmp = types;

	while(tmp->type != -1 || tmp->name != NULL ) 
	{
		//if( !strcasecmp(tmp->name, name) )
		if( strcasestr(tmp->name, name) )
		{		
			return tmp->type;
		}
		tmp++;
	}

	{
		CMN_ABORT("Type of name %s is not found, the first item is :%s", name, types->name);
	}
	return -1;
}


char *cmnMuxTypeFindName(TYPE_NAME_T *types, int type)
{
	TYPE_NAME_T *tmp = types;

	while(tmp->type != -1 || tmp->name != NULL ) 
	{
		if( tmp->type == type )
		{		
			return tmp->name;
		}
		tmp++;
	}

#ifndef   __CMN_RELEASE__
	{
		CMN_ABORT("Type %d is not found, the first item is :%s", type, types->name);
	}
#endif
	return NULL;
}

unsigned int cmnMuxCRC32b(void *message, int len)
{
	int i, j;
	unsigned int byte, crc, mask;
	char * tmp = (char*) message;
	i = 0;
	crc = 0xFFFFFFFF;
	
	//while (tmp[i] != 0)
	for (i=0; i< len; i++)	
	{
		byte = tmp[i];            // Get next byte.
		crc = crc ^ byte;
		for (j = 7; j >= 0; j--)
		{    // Do eight times.
			mask = -(crc & 1);
			crc = (crc >> 1) ^ (0xEDB88320 & mask);
		}
		
//		i = i + 1;
	}
	
	return ~crc;
}

MuxPlugIn *cmnMuxPluginFind(MuxMain *muxMain, MUX_PLUGIN_TYPE type)
{
	MuxPlugIn *plugin = muxMain->plugins;

	while(plugin)
	{
		if(plugin->type == type)
		{
			return plugin;
		}
		
		plugin = plugin->next;
	}

	return NULL;
}


