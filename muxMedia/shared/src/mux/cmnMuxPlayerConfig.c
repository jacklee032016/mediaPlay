/*
* $Id$
*/



#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


#if WITH_ADDRESS_ACL
static int __parseStreamACLConfig(RECT_CONFIG *window, const char **p)
{
	char arg[1024];
	int	ret = EXIT_SUCCESS;

	IPAddressACL  *acl, **aclPP;
	struct hostent *he;

	acl = (IPAddressACL *) cmn_malloc(sizeof(IPAddressACL));

	cmnParseGetArg(arg, sizeof(arg), p);

	he = gethostbyname(arg);
	if (!he)
	{
		fprintf(stderr, "ACL refers to invalid host or ip address '%s'\n", arg);
		return EXIT_FAILURE;
	}
	else
	{
		/* Only take the first */
		acl->first.s_addr = ntohl(((struct in_addr *) he->h_addr_list[0])->s_addr);
		acl->last = acl->first;
	}
	
	cmnParseGetArg(arg, sizeof(arg), p);

	if (arg[0])
	{
		he = gethostbyname(arg);
		if (!he)
		{
			fprintf(stderr, "ACL refers to invalid host or ip address '%s'\n", arg);
			//memcpy(&acl->last, &acl->first, sizeof(struct in_addr) );
			ret = EXIT_FAILURE;
		}
		else
		{
			/* Only take the first */
			acl->last.s_addr = ntohl(((struct in_addr *) he->h_addr_list[0])->s_addr);
		}
	}


	aclPP = &window->acl;

	if (aclPP)
	{
		while (*aclPP)
			aclPP = &(*aclPP)->next;

		*aclPP = acl;
	}

	return ret;
}
#endif


static int _parseRectConfig(RECT_CONFIG *window, const char **p, MuxPlayerConfig *cfg)
{
	char cmd[64];
	char arg[1024];
	int	ret = EXIT_SUCCESS;

	if(window==NULL)
		return EXIT_FAILURE;

	cmnParseGetArg(cmd, sizeof(cmd), p);

	if (IS_STRING_EQUAL(cmd, "Left"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		window->left = atof(arg);
	}
	else if (IS_STRING_EQUAL(cmd, "Top"))
	{
		cmnParseGetArg(arg, sizeof(arg), p);
		window->top = atof(arg);
	}
	else if (IS_STRING_EQUAL(cmd, "Width"))
	{
		window->width = cmnParseGetIntValue( p);
	}
	else if (IS_STRING_EQUAL(cmd, "Height"))
	{
		window->height =  cmnParseGetIntValue( p);
	}
	else if (IS_STRING_EQUAL(cmd, "FontSize"))
	{
		window->fontSize = cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "fontSize: %d\n", window->fontSize);
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "FontColor"))
	{
		sscanf(*p, "%x", &window->fontColor);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "FontColor: %d(0x%x)\n", window->fontColor, window->fontColor);
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "BackgroundColor"))
	{
		sscanf(*p, "%x", &window->backgroundColor);
	}
	else if (IS_STRING_EQUAL(cmd, "Alpha"))
	{
		window->alpha =  (char)cmnParseGetIntValue( p);
	}
	else if (IS_STRING_EQUAL(cmd, "Enable"))
	{
		window->enable = cmnParseGetBoolValue(p);
	}
	else if (IS_STRING_EQUAL(cmd, "AudioEnable"))
	{
		window->audioEnable = cmnParseGetBoolValue(p);
	}
	else if (IS_STRING_EQUAL(cmd, "DisplayMode"))
	{
		window->displayMode = cmnParseGetIntValue(p);
	}
	else if (IS_STRING_EQUAL(cmd, "ROTATE_MODE"))
	{
		window->rotateType = cmnParseGetIntValue(p);
		if(window->rotateType < ROTATE_TYPE_0|| window->rotateType > ROTATE_TYPE_270)
			window->rotateType = ROTATE_TYPE_0;
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "ROTATE_MODE: %d\n", window->rotateType);
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "URL"))
	{/*  */
		cmnParseGetArg(window->url, sizeof(window->url), p);
	}
	else if (!strcasecmp(cmd, "ACLGlob"))
	{/* for acl */
#if WITH_ADDRESS_ACL
		IPAddressACL  *acl;
		acl = (IPAddressACL *) cmn_malloc(sizeof(IPAddressACL));
	
		cmnParseGetArg(arg, sizeof(arg), p);
		if (IS_STRING_EQUAL(arg, "allow"))
		{
			acl->action = IP_ALLOW;
		}
		else if (IS_STRING_EQUAL(arg, "deny"))
		{
			acl->action = IP_DENY;
		}
		else if (IS_STRING_EQUAL(arg, "disable"))
		{
			acl->action = ACL_DISABLE;
		}
		else
		{
			fprintf(stderr, "ACLGlob is not ALLOW or DENY or DISABLE, Please check your config!\n" );
			ret = EXIT_FAILURE;
		}
		window->acl = acl;
#else
		//fprintf(stderr, "ACL is not support in this version\n" );
		ret = EXIT_FAILURE;
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "ACL"))
	{
#if WITH_ADDRESS_ACL
		if(window->acl->action != ACL_DISABLE)
		{
			ret = __parseStreamACLConfig( window, p );
		}
#endif
	}
	else
	{
		fprintf(stderr, "Invalidate configuration item: %s\n", cmd);
		ret = EXIT_FAILURE;
	}
	
	return ret;
}

static int _parseGlobalConfig(const char **p, MuxPlayerConfig *cfg)
{
	char cmd[64];
	int ret = EXIT_SUCCESS;
	
	cmnParseGetArg(cmd, sizeof(cmd), p);
	
	/***** configuration for URLs *****/
	if (IS_STRING_EQUAL(cmd, "PlayURL"))
	{
		cmnParseGetArg(cfg->playUrl, CMN_NAME_LENGTH, p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "PlayURL: %s\n", cfg->playUrl );
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "URL_HTTP"))
	{
		cmnParseGetArg(cfg->urlHttp, CMN_NAME_LENGTH, p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "URL_HTTP: %s\n", cfg->urlHttp );
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "URL_RTSP"))
	{
		cmnParseGetArg(cfg->urlRtsp, CMN_NAME_LENGTH, p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "URL_RTSP: %s\n", cfg->urlRtsp);
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "URL_WMSP"))
	{
		cmnParseGetArg(cfg->urlWmsp, CMN_NAME_LENGTH, p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "URL_WMSP: %s\n", cfg->urlWmsp);
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "URL_RTMP"))
	{
		cmnParseGetArg(cfg->urlRtmp, CMN_NAME_LENGTH, p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "URL_RTMP: %s\n", cfg->urlRtmp );
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "URL_RTP"))
	{
		cmnParseGetArg(cfg->urlRtp, CMN_NAME_LENGTH, p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "URL_RTP: %s\n", cfg->urlRtp );
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "URL_UDP"))
	{
		cmnParseGetArg(cfg->urlUdp, CMN_NAME_LENGTH, p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "URL_UDP: %s\n", cfg->urlUdp );
#endif
	}


	else if (IS_STRING_EQUAL(cmd, "VideoFormat"))
	{
		cmnParseGetArg(cfg->displayFormat,  CMN_NAME_LENGTH, p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "VideoFormat: %s\n", cfg->displayFormat);
#endif
	}

	else if (IS_STRING_EQUAL(cmd, "DeepColor"))
	{
		cfg->deepColor = cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "TimeoutLocal: %d\n", cfg->playTimeoutLocal);
#endif
	}

	else if (IS_STRING_EQUAL(cmd, "AudioVolume"))
	{
		cfg->audioVolume = cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "AudioVolume: %d\n", cfg->audioVolume);
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "TimeoutLocal"))
	{
		cfg->playTimeoutLocal = cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "TimeoutLocal: %d\n", cfg->playTimeoutLocal);
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "TimeoutNetwork"))
	{
		cfg->playTimeoutNetwork = cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "TimeoutNetwork: %d\n", cfg->playTimeoutNetwork);
#endif
	}
	else if (IS_STRING_EQUAL(cmd, "TimeoutErrorMsg"))
	{
		cfg->timeoutErrorMsg = cmnParseGetIntValue(p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "TimeoutErrorMsg: %d\n", cfg->timeoutErrorMsg);
#endif
	}

	else if (IS_STRING_EQUAL(cmd, "EnableScreenDebug"))
	{
		cfg->enableScreenDebug = cmnParseGetBoolValue( p);
	}
	else if (IS_STRING_EQUAL(cmd, "WindowKeepLastFrame"))
	{
		cfg->keepLastFrame = cmnParseGetBoolValue( p);
	}
	else if (IS_STRING_EQUAL(cmd, "EnableLowDelay"))
	{
		cfg->enableLowDelay = cmnParseGetBoolValue( p);
	}
	
#if WITH_SERVER_WATCHDOG
	else if (IS_STRING_EQUAL(cmd, "EnableWd"))
	{
		cfg->enableWd = cmnParseGetBoolValue( p);
	}
	else if (IS_STRING_EQUAL(cmd, "WdTimeout"))
	{
		cfg->wdTimeout = cmnParseGetIntValue(p);
	}
#endif

	else if (IS_STRING_EQUAL(cmd, "CaptureName"))
	{
		cmnParseGetArg(cfg->captureName, CMN_NAME_LENGTH, p);
#if DEBUG_CONFIG_FILE
		fprintf(stderr, "CaptureName: %s\n", cfg->captureName);
#endif
	}

	return ret;
}

int cmnMuxPlayerParseConfig(const char *filename, MuxPlayerConfig *cfg)
{
#define	PARSE_STATE_GLOBAL	0
#define	PARSE_STATE_WINDOW	1
#define	PARSE_STATE_PLAYLIST	2

	FILE *f;
	char line[1024];
	char cmd[64];
	const char *p;
	int line_num = 0;
	int parseState = PARSE_STATE_GLOBAL;
	int ret = EXIT_SUCCESS;
	RECT_CONFIG *window = NULL;
	static int pipIndex = 0;
	
	f = fopen(filename, "r");
	if (!f)
	{
		perror(filename);
		return EXIT_FAILURE;
	}

	/** it must be initialized as all 0s before enter into this function */
//	memset(cfg, 0, sizeof(MuxPlayerConfig));	

	cfg->audioVolume = 100;
	cfg->deepColor = 100;
	
	cmn_list_init(&cfg->windows);
	cmn_list_init(&cfg->osds);

	snprintf(cfg->configFileName, CMN_NAME_LENGTH, "%s", filename );

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


		if ( strstr(line, "<Window") )
		{
			char *q;

			cmnParseGetArg(cmd, sizeof(cmd), &p);
			if (window )
			{
				fprintf(stderr, "%s:%d: Already in a tag for <Window>\n",filename, line_num);
				ret = EXIT_FAILURE;
				break;
			}
			else
			{
				window = cmn_malloc(sizeof(RECT_CONFIG));
				window->mainCfg = cfg;

				
				cmnParseGetArg(window->name, sizeof(window->name), &p);
				q = strrchr(window->name, '>');
				if (*q)
					*q = '\0';
				 if (IS_STRING_EQUAL(window->name, "MAIN"))
				 {
				 	window->type = RECT_TYPE_MAIN;
					cmn_list_append( &cfg->windows, window);
					cmn_list_append( &cfg->osds, window);
				 }
				 else if (IS_STRING_EQUAL(window->name, "PIP"))
				 {
					snprintf(window->name, sizeof(window->name), "PIP%d", pipIndex++);
				 	window->type = RECT_TYPE_PIP;
					/* add in stream list */
					cmn_list_append( &cfg->windows, window);
					cmn_list_append( &cfg->osds, window);
				 }
				 else if (IS_STRING_EQUAL(window->name, "SUBTITLE"))
				 {
				 	window->type = RECT_TYPE_SUBTITLE;
					cmn_list_append( &cfg->osds, window);
				 }
				 else if (IS_STRING_EQUAL(window->name, "ALERT"))
				 {
				 	window->type = RECT_TYPE_ALERT;
					cmn_list_append( &cfg->osds, window);
				 }
				 else if (IS_STRING_EQUAL(window->name, "LOGO"))
				 {
				 	window->type = RECT_TYPE_LOGO;
					cmn_list_append( &cfg->osds, window);
				 }
				 else
				 {
				 	window->type = RECT_TYPE_UNKNOWN;
					fprintf(stderr, "Unknow window type '%s'\n", window->name);
				 }
			}

			parseState = PARSE_STATE_WINDOW;

			continue;
		}
		else if ( strstr(line, "</Window>"))
		{
			if (!window)
			{
				fprintf(stderr, "%s:%d: No corresponding <Window> for </Window>\n", filename, line_num);
				ret = EXIT_FAILURE;
				break;
			}

			window = NULL;
			parseState = PARSE_STATE_GLOBAL;
			continue;
		}

		if(parseState == PARSE_STATE_GLOBAL)
		{
			ret = _parseGlobalConfig( &p, cfg);
		}
		else if(parseState == PARSE_STATE_WINDOW)
		{
			ret = _parseRectConfig(window, &p, cfg);
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


int cmnMuxConfigParseRecord(const char *filename, CmnMuxRecordConfig *recordConfig)
{
	FILE *f;
	char line[1024];
	char cmd[64];
	const char *p;
	int ret = 0;

	if(!recordConfig)
	{
		CMN_ABORT("RECORDER configuration is NULL");
	}

	f = fopen(filename, "r");
	if (!f)
	{
		MUX_ERROR("Could not open the configuration file '%s' :%s", filename, strerror(errno) );
		return -EXIT_FAILURE;
	}

	snprintf(recordConfig->configFile, sizeof(recordConfig->configFile), "%s", filename );

	recordConfig->lineNum = 0;
	while (fgets(line, sizeof(line), f) != NULL)
	{
		recordConfig->lineNum++;
		p = line;
		while (isspace(*p))
			p++;
		
		if (*p == '\0' || *p == '#')
			continue;

		cmnParseGetArg(cmd, sizeof(cmd), &p);

		if (IS_STRING_EQUAL(cmd, "CaptureName") )
		{
			cmnParseGetArg(recordConfig->captureName, sizeof(recordConfig->captureName), &p);
#if DEBUG_CONFIG_FILE
			fprintf(stderr, "\tRecorder CaptureName: %s\n", recordConfig->captureName );
#endif
		}
		else if (IS_STRING_EQUAL(cmd, "Format"))
		{
			cmnParseGetArg(recordConfig->format, sizeof(recordConfig->format), &p);
#if DEBUG_CONFIG_FILE
			fprintf(stderr, "\tRecorder output format: %s\n", recordConfig->format );
#endif
		}
		else if (IS_STRING_EQUAL(cmd, "NoVideo"))
		{
			recordConfig->noVideo = TRUE;
#if DEBUG_CONFIG_FILE
			fprintf(stderr, "\tRecorder NoVideo: %s\n", STR_BOOL_VALUE(recordConfig->noVideo) );
#endif
		}
		else if (IS_STRING_EQUAL(cmd, "NoAudio"))
		{
			recordConfig->noAudio = TRUE;
#if DEBUG_CONFIG_FILE
			fprintf(stderr, "\tRecorder NoAudio: %s\n", STR_BOOL_VALUE(recordConfig->noAudio) );
#endif
		}
		else if (IS_STRING_EQUAL(cmd, "NoSubtitle"))
		{
			recordConfig->noSubtitle = TRUE;
#if DEBUG_CONFIG_FILE
			fprintf(stderr, "\tRecorder NoSubtitle: %s\n", STR_BOOL_VALUE(recordConfig->noSubtitle) );
#endif
		}
		else
		{
			MUX_ERROR("Incorrect keyword: '%s' in line %d"ERROR_TEXT_END, cmd, recordConfig->lineNum);
		}
	}
	
	fclose(f);
	
	if (ret < 0)
		return ret;

	return EXIT_SUCCESS;
}


