/*
 * $Id: cgi_navigate.c,v 1.38 2007/09/15 19:45:14 lizhijie Exp $
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "libCgi.h"
#include "cgi_version.h"

#include "libCgiSyscfg.h"

#define	NAVIGATE_NAME_LENGTH			128

#define	FOLDER_TREE_NAME					"foldersTree"

//#define	_(text)								(text)

typedef enum
{
	NAVIFATE_TARGET_NEW_WINDOW	= 0,
	NAVIGATE_TARGET_MAIN_FRAME		= 2
}NAVIGATE_TARGET_TYPE;


typedef enum
{
	NAVI_FOLDER_ROOT			=	1,

	/* folder for  */
	NAVI_FOLDER_RUNTIME,
	NAVI_FOLDER_PLAYER_CONFIG,
	NAVI_FOLDER_PLAYER,
	NAVI_FOLDER_MUX_SERVER,
	NAVI_FOLDER_RECORDER,
	NAVI_FOLDER_MEDIA_FILE,

	NAVI_FOLDER_NETWORK,
	NAVI_FOLDER_SYSTEM,
	NAVI_FOLDER_SOFT_HARDWARE,
	NAVI_FOLDER_UPDTAE,
	NAVI_FOLDER_BACKUP,
	NAVI_FOLDER_RESTORE,

	NAVI_FOLDER_SERVICES,
	NAVI_FOLDER_SERVICES_CONFIG,
	NAVI_FOLDER_SERVICES_DDNS,


	NAVI_FOLDER_WIRELESS,

	NAVI_FOLDER_SERVERS,
	NAVI_FOLDER_SERVER_WWW,
	
	NAVI_FOLDER_VIDEO,
	NAVI_FOLDER_VIDEO_CONFIG,
	NAVI_FOLDER_AUDIO_CONFIG,   //wyb
	NAVI_FOLDER_VIDEO_DYNAMIC,
	NAVI_FOLDER_VIDEO_ACTION,

	
	
	NAVI_FOLDER_NOT_EXIST
}FOLDER_ID;

struct _menu_folder
{
	FOLDER_ID					id;
	char							name[NAVIGATE_NAME_LENGTH];

	char							menuName[NAVIGATE_NAME_LENGTH];
	char							iconName[NAVIGATE_NAME_LENGTH];
	char							hReference[NAVIGATE_NAME_LENGTH];

	FOLDER_ID					parentId;

	int							where;
};

struct _menu_item
{
	NAVIGATE_TARGET_TYPE		target;
	
	char							name[NAVIGATE_NAME_LENGTH];
	char							alertMsg[NAVIGATE_NAME_LENGTH];
	char							iconName[NAVIGATE_NAME_LENGTH];

	char							hReference[NAVIGATE_NAME_LENGTH];

	FOLDER_ID					folderId;

	int							where;
};

typedef	struct _menu_folder		NAVIGATE_FOLDER;
typedef	struct _menu_item		NAVIGATE_MENU_ITEM;

#define	MENU_WHERE_BASE			0x00000001
#define	MENU_WHERE_SERVICES		0x00000002
#define	MENU_WHERE_SERVERS		0x00000004
#define	MENU_WHERE_VIDEO			0x00000008
#define	MENU_WHERE_ANYWHERE	0xFFFFFFFF

typedef		enum
{
	WEB_TYPE_BASE = 0,
	WEB_TYPE_SERVICES,
	WEB_TYPE_SERVERS,
	WEB_TYPE_VIDEO,
	WEB_TYPE_INVALIDATE
}WEB_TYPE_T;

/* for navigate.cgi and content.cgi */
typedef	struct	
{
	llist				cgiVariables;
	WEB_TYPE_T		type;

	int				where;
	
}BASE_INFO;

#define	CGI_NAVIGATE_KEYWORD_BASE			"base"
#define	CGI_NAVIGATE_KEYWORD_SERVICES		"services"
#define	CGI_NAVIGATE_KEYWORD_SERVERS		"servers"
#define	CGI_NAVIGATE_KEYWORD_VIDEO			"video"



NAVIGATE_FOLDER 	folders [] = 
{
	{
		.id			=	NAVI_FOLDER_ROOT,
		.name		=	"foldersTree",
		.menuName	=	"MuxLab",
		.iconName	=	"iWebServer",
		.hReference	=	"",
		.parentId		=	NAVI_FOLDER_NOT_EXIST,
		.where		=	MENU_WHERE_ANYWHERE,
	},

	/* folder and subfolders of global runtime */
	{
		.id			=	NAVI_FOLDER_RUNTIME,
		.name		=	"MuxGlobalConfiguration",
		.menuName	=	_("Global Configuration"),
		.iconName	=	"iJvm",
		.hReference	=	"",
		.parentId		=	NAVI_FOLDER_ROOT,
		.where		=	MENU_WHERE_BASE,
	},

	/* folder and subfolders of PLAYER */
	{
		.id			=	NAVI_FOLDER_PLAYER_CONFIG,
		.name		=	"MuxPlayerConfiguration",
		.menuName	=	_("Player Configuration"),
		.iconName	=	"iJvm",
		.hReference	=	"",
		.parentId		=	NAVI_FOLDER_ROOT,
		.where		=	MENU_WHERE_BASE,
	},

	{
		.id			=	NAVI_FOLDER_PLAYER,
		.name		=	"PlayerInformation",
		.menuName	=	_("Player Information"),
		.iconName	=	"iJvm",
		.hReference	=	"",
//		.parentId		=	NAVI_FOLDER_MUX_SERVER,
		.parentId		=	NAVI_FOLDER_ROOT,
		.where		=	MENU_WHERE_BASE,
	},

	/* folder and subfolders of SERVER */
	{
		.id			=	NAVI_FOLDER_MUX_SERVER,
		.name		=	"MuxServerConfiguration",
		.menuName	=	_("Sever Config and Runtime"),
		.iconName	=	"iJvm",
		.hReference	=	"",
		.parentId		=	NAVI_FOLDER_ROOT,
		.where		=	MENU_WHERE_BASE,
	},

	/* folder and subfolders of RECORDER */
	{
		.id			=	NAVI_FOLDER_RECORDER,
		.name		=	"RecorderConfiguration",
		.menuName	=	_("Recorder Configuration"),
		.iconName	=	"iJvm",
		.hReference	=	"",
//		.parentId		=	NAVI_FOLDER_MUX_SERVER,
		.parentId		=	NAVI_FOLDER_ROOT,
		.where		=	MENU_WHERE_BASE,
	},

	{
		.id			=	NAVI_FOLDER_MEDIA_FILE,
		.name		=	"MediaFiles",
		.menuName	=	_("Media Files Management"),
		.iconName	=	"iJvm",
		.hReference	=	"",
//		.parentId		=	NAVI_FOLDER_MUX_SERVER,
		.parentId		=	NAVI_FOLDER_ROOT,
		.where		=	MENU_WHERE_BASE,
	},

		
	{
		.id			=	NAVI_FOLDER_SYSTEM,
		.name		=	"SystemInfo",
		.menuName	=	_("System Configuration"),
		.iconName	=	"iJvm",
		.hReference	=	"",
		.parentId		=	NAVI_FOLDER_ROOT,
		.where		=	MENU_WHERE_BASE,
	},

#if 0
	{
		.id			=	NAVI_FOLDER_SERVICES,
		.name		=	"services",
		.menuName	=	_("System Services"),
		.iconName	=	"iWebServer",
		.hReference	=	"",
		.parentId		=	NAVI_FOLDER_ROOT,
		.where		=	MENU_WHERE_BASE,
	},
	{
		.id			=	NAVI_FOLDER_SERVICES_CONFIG,
		.name		=	"servicesCfg",
		.menuName	=	_("Services Configuration"),
		.iconName	=	"iJvm",
		.hReference	=	"",
		.parentId		=	NAVI_FOLDER_SERVICES,
		.where		=	MENU_WHERE_BASE,
	},


	{
		.id			=	NAVI_FOLDER_VIDEO,
		.name		=	"MediaConfiguration",
		.menuName	=	_("Media Configuration"),
		.iconName	=	"iJvm",
		.hReference	=	"",
		.parentId		=	NAVI_FOLDER_ROOT,
		.where		=	MENU_WHERE_BASE,
	},
	{
		.id			=	NAVI_FOLDER_AUDIO_CONFIG,     			//wyb
		.name		=	"AudioStatic",
		.menuName	=	_("Audio Config"),
		.iconName	=	"iJvm",
		.hReference	=	"",
		.parentId		=	NAVI_FOLDER_VIDEO,
		.where		=	MENU_WHERE_BASE,
	}
#endif

};

NAVIGATE_MENU_ITEM	menus[] = 
{

/* globl Menus
*/
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Modules"),
		.alertMsg		=	_("Function modules of MuxMedia"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_GLOBAL_PLUGINS,
		.folderId		=	NAVI_FOLDER_RUNTIME,
		.where		=	MENU_WHERE_BASE,
	},
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Configuration of Controller"),
		.alertMsg		=	_("Info about controller"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_GLOBAL_CONTROLLER,
		.folderId		=	NAVI_FOLDER_RUNTIME,
		.where		=	MENU_WHERE_BASE,
	},
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Configuration of Captuere"),
		.alertMsg		=	_("Info about captuering media from player"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_GLOBAL_CAPTURER,
		.folderId		=	NAVI_FOLDER_RUNTIME,
		.where		=	MENU_WHERE_BASE,
	},


/* Player Menu in PLAYER folder
*/
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Player Information"),
		.alertMsg		=	_("Status of Player and its configuration"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_PLAYER_INFO,
		.folderId		=	NAVI_FOLDER_PLAYER,
		.where		=	MENU_WHERE_BASE,
	},
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Media Information"),
		.alertMsg		=	_("Status of playing media, stream, etc"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_MEDIA_INFO,
		.folderId		=	NAVI_FOLDER_PLAYER,
		.where		=	MENU_WHERE_BASE,
	},
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
//		.name		=	_("Player Runtime"),
		.name		=	_("MedaDatas"),
		.alertMsg		=	_("Running Information of Player"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_METADATA_INFO,
		.folderId		=	NAVI_FOLDER_PLAYER,
		.where		=	MENU_WHERE_BASE,
	},

/* Record menu in Mux server folder
*/
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Recorder Configation"),
		.alertMsg		=	_("Detailed configuration of recorder"),
		.iconName	=	"iServlets.gif",
		.hReference	=	MUXSERVER_HLINK_RECORD_CONFIG,
		.folderId		=	NAVI_FOLDER_RECORDER,
		.where		=	MENU_WHERE_BASE,
	},
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Recorder Status"),
		.alertMsg		=	_("Status and Statistics of Recorder"),
		.iconName	=	"iServlets.gif",
		.hReference	=	MUXSERVER_HLINK_RECORD_STATUS,
		.folderId		=	NAVI_FOLDER_RECORDER,
		.where		=	MENU_WHERE_BASE,
	},

/* Media Files menu items  in Mux server folder*/
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Local Media Files"),
		.alertMsg		=	_("List all media files in USB disk or SD card"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_MEDIAFILE,
		.folderId		=	NAVI_FOLDER_MEDIA_FILE,
		.where		=	MENU_WHERE_BASE,
	},

/* Reboot mux server menu items  in Mux server folder*/
#if 0
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Status of Player"),
		.alertMsg		=	_("Lookup status of Player or reboot Player"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	WEB_URL_MUX_SERVER"?"CGI_KEYWORD_OPERATOR"="CGI_MUX_SERVER_OP_LIST,
		.folderId		=	NAVI_FOLDER_MUX_SERVER,
		.where		=	MENU_WHERE_BASE,
	},
#endif
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("URLs of Player"),
		.alertMsg		=	_("Config URLs of Player"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_URLS,
		.folderId		=	NAVI_FOLDER_PLAYER_CONFIG,
		.where		=	MENU_WHERE_BASE,
	},

	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Windows of Player"),
		.alertMsg		=	_("Window, OSD and URLs of Player"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_WINDOWS,
		.folderId		=	NAVI_FOLDER_PLAYER_CONFIG,
		.where		=	MENU_WHERE_BASE,
	},
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Playlist"),
		.alertMsg		=	_("Playlist for local media file"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_PLAYLIST,
		.folderId		=	NAVI_FOLDER_PLAYER_CONFIG,
		.where		=	MENU_WHERE_BASE,
	},

	/* SERVER configuration and runtime info */
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Server Configuration"),
		.alertMsg		=	_("Configuration for all services"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_SVR_CONFIG,
		.folderId		=	NAVI_FOLDER_MUX_SERVER,
		.where		=	MENU_WHERE_BASE,
	},
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Current Connections"),
		.alertMsg		=	_("List all servicing connections"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_SVR_CONNECTIONS,
		.folderId		=	NAVI_FOLDER_MUX_SERVER,
		.where		=	MENU_WHERE_BASE,
	},
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Media Sources"),
		.alertMsg		=	_("List all servicing media source"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_SVR_SOURCE,
		.folderId		=	NAVI_FOLDER_MUX_SERVER,
		.where		=	MENU_WHERE_BASE,
	},
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Protocols and URLs"),
		.alertMsg		=	_("List all servicing Protocols and URLs"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	MUXSERVER_HLINK_SVR_URLS,
		.folderId		=	NAVI_FOLDER_MUX_SERVER,
		.where		=	MENU_WHERE_BASE,
	},


	/* System Info Menu Items */
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("System Version"),
		.alertMsg		=	_("Version information of main system processes"),
		.iconName	=	"iServer.gif",
		.hReference	=	WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_VERSION,
		.folderId		=	NAVI_FOLDER_SYSTEM,
		.where		=	MENU_WHERE_BASE,
	},
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("System Processes"),
		.alertMsg		=	_("Information of main system processes"),
		.iconName	=	"iServer.gif",
		.hReference	=	WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_SYS_PROC,
		.folderId		=	NAVI_FOLDER_SYSTEM,
		.where		=	MENU_WHERE_BASE,
	},

	/* Services menu items */	
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("Services Management"),
		.alertMsg		=	_("System Version and releases Information"),
		.iconName	=	"iServer.gif",
		.hReference	=	WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_SERVICES,
		.folderId		=	NAVI_FOLDER_SYSTEM,
		.where		=	MENU_WHERE_BASE,
	},
#if 0
	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("System Language"),
		.alertMsg		=	_("Lanugage Setup"),
		.iconName	=	"iApplicationAttribute.gif",
		.hReference	=	WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_LOCALE,
		.folderId		=	NAVI_FOLDER_SYSTEM,
		.where		=	MENU_WHERE_BASE,
	},
#endif

	{
		.target		=	NAVIGATE_TARGET_MAIN_FRAME,
		.name		=	_("System Time"),
		.alertMsg		=	_("Mode setup of system time"),
		.iconName	=	"iServer.gif",
		.hReference	=	WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_TIME,
		.folderId		=	NAVI_FOLDER_SYSTEM,
		.where		=	MENU_WHERE_BASE,
	},



};

NAVIGATE_MENU_ITEM rebootMenu =
{
	.target		=	NAVIGATE_TARGET_MAIN_FRAME,
	.name		=	CGI_STR_REBOOT,
	.alertMsg		=	_("In order to activate configuration, please reboot"),
	.iconName	=	"iLocalMachine.gif",
	.hReference	=	WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_REBOOT,
	.folderId		=	NAVI_FOLDER_ROOT,
	.where		=	MENU_WHERE_ANYWHERE,
};


#define	FOLDER_OUTPUT(folder, parentFolder )		\
	do{ if( (parentFolder)==NULL) 					\
		printf("%s = gFld(\"%s|%s\", \"\") \n", (folder)->name, gettext((folder)->menuName), (folder)->iconName); \
		else												\
			printf("%s = insFld(%s, gFld(\"%s|%s\", \"\"))\n", (folder)->name, (parentFolder)->name, gettext((folder)->menuName), (folder)->iconName ); \
	}while(0)

#define	ITEM_OUTPUT(item, parentFolder )		\
	do{ 						 					\
		printf("insDoc(%s, gLnk(%d, \"%s^%s^%s\", \"%s\") )\n", \
		(parentFolder)->name, (item)->target, gettext((item)->name), gettext((item)->alertMsg), (item)->iconName, (item)->hReference); \
	}while(0)

static void __navigate_begin()
{
	printf("<html>\n<head>\n\n");
	printf("<SCRIPT SRC=\"/include/ftree.js\"></SCRIPT>\n");
	printf("<SCRIPT SRC=\"/include/help.js\"></SCRIPT>\n\n");
	printf("<SCRIPT>\n");
}

static void __navigate_end()
{
	printf("</SCRIPT>\n\n");
	printf("<SCRIPT>\ninitializeDocument()\n</SCRIPT>\n");
	printf("</HEAD>\n");
	printf("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n");
	printf("<title>MuxLab Inc.</title>\n");
	printf("<LINK href=\"/include/style.css\" rel=stylesheet type=text/css>\n");
	printf("</HTML>\n");
}

static NAVIGATE_FOLDER *__navigate_find_parent(FOLDER_ID parentId, int where)
{
	int	i= 0;
	int	count = sizeof(folders)/sizeof(folders[0]);
	NAVIGATE_FOLDER	*iter = folders;

	for(i = 0; i< count; i++)
	{
		if( iter->id == parentId && (iter->where&where) )
		{
			return iter;
		}
		
		iter++;
	}

	return NULL;
}

static void __navigate_iterate(BASE_INFO *info)
{
	int	i= 0, j;
	int	count = sizeof(folders)/sizeof(folders[0]);
	int	menuCount = sizeof(menus)/sizeof(menus[0]);
	NAVIGATE_FOLDER	*iter = folders;
	NAVIGATE_MENU_ITEM	*item = NULL;

	for(i = 0; i< count; i++)
	{
		if(( iter->where&info->where) )
		{
			FOLDER_OUTPUT(iter, __navigate_find_parent(iter->parentId, info->where) );

			item = menus;
			for(j=0; j< menuCount;j++)
			{
				if(item->folderId == iter->id && (item->where&info->where) )
				{
					ITEM_OUTPUT(item, iter);
				}
				item++;
			}
			printf("\n");
		}
		
		iter++;
	}

	ITEM_OUTPUT( &rebootMenu, __navigate_find_parent(NAVI_FOLDER_ROOT, MENU_WHERE_ANYWHERE));
}


static int	__navigate_init(BASE_INFO *info)
{
	char		*cmd;

	read_cgi_input(&info->cgiVariables, NULL, NULL);

	info->type = WEB_TYPE_BASE;
	info->where = MENU_WHERE_BASE;
	cmd = GET_VALUE( &info->cgiVariables, CGI_KEYWORD_OPERATOR);

	if( !strcasecmp(cmd, CGI_NAVIGATE_KEYWORD_SERVERS) )
	{
		info->type = WEB_TYPE_SERVERS;
		info->where = MENU_WHERE_SERVERS;
	}
	else if( !strcasecmp(cmd, CGI_NAVIGATE_KEYWORD_SERVICES) )
	{
		info->type = WEB_TYPE_SERVICES;
		info->where = MENU_WHERE_SERVICES;
	}
	else if( !strcasecmp(cmd, CGI_NAVIGATE_KEYWORD_VIDEO) )
	{
		info->type = WEB_TYPE_VIDEO;
		info->where = MENU_WHERE_VIDEO;
	}
	
	CGI_HTML_HEADE();

	return 0;
}

int main(int argc, char *argv[])
{
	BASE_INFO	info;
	
	__navigate_init( &info);
	__navigate_begin( );

	__navigate_iterate( &info);
	
	__navigate_end();
	
	return 0;
}


