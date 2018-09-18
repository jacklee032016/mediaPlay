
#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>

#include "apiClient.h"

struct API_CLIENT_OPT_HANDLER;

typedef	struct API_CLIENT_OPT_HANDLER
{
	int		type;
	char		*name;

	char		*prompt;

	int	(*handler)(struct API_CLIENT_OPT_HANDLER *, char *,  struct API_PARAMETERS *);
}API_CLIENT_OPT_HANDLER;


/* for the command without any optional parameters*/
static int	_dumpParseSubOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	return EXIT_SUCCESS;
}

static int	_playSubOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		"index",
		"media",
		"repeat",
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret= EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[0], value);
					clientParams->index = atoi(value);
				}
				break;

			case 1:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[1]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[1], value);
					snprintf(clientParams->media, sizeof(clientParams->media), "%s",value);
				}
				break;

			case 2:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[2]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[2], value);
					clientParams->repeatNumber = atoi(value);
				}
				break;


			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}
	}

_ret:
	if(ret)
	{
//		fprintf(stderr, "option for %s : /%s=INDEX, %s=MEDIA, %s=REPEAT/\n", clientParams->cmd, token[0], token[1], token[2]);
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}


static int	_aspectOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		"index",
		"mode",
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret= EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[0], value);
					clientParams->index = atoi(value);
				}
				break;

			case 1:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[1]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[1], value);
					clientParams->repeatNumber = atoi(value);
					if (clientParams->repeatNumber < MUX_DISPLAY_MODE_DEFAULT || clientParams->repeatNumber >= MUX_DISPLAY_MODE_UNKNOWN)
					{
						fprintf(stderr, "'%s' value of '%d' is not validate\n", token[1], clientParams->repeatNumber);
						ret = EXIT_FAILURE;
						goto _ret;
					}

				}
				break;


			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}
	}

_ret:
	if(ret)
	{
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}

static int	_alertOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		"color",
		"media",
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret= EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					clientParams->color = cmnParseGetHexIntValue(value);
					fprintf(stderr, "'%s'='%s' (%d, 0x%x)\n", token[0], value, clientParams->color, clientParams->color );
				}
				break;

			case 1:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[1]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[1], value);
					snprintf(clientParams->media, sizeof(clientParams->media), "%s", value);
				}
				break;

			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}
	}

_ret:
	if(ret)
	{
//		fprintf(stderr, "option for %s : /%s=integer, %s=MEDIA/\n", clientParams->cmd, token[0], token[1]);
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}


static int	_osdTransparencyOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		"color",
		"index",
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret= EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					clientParams->color = atoi(value);
					if(clientParams->color<0 || clientParams->color > 255 )
					{
						fprintf(stderr, "transparency is 0~255, value is '%d'\n", clientParams->color );
						ret= EXIT_FAILURE;
					}
					fprintf(stderr, "'%s'='%s' (%d)\n", token[0], value, clientParams->color);
				}
				break;

			case 1:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[1]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[1], value);
					clientParams->index = atoi(value);
				}
				break;

			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}
	}

_ret:
	if(ret)
	{
//		fprintf(stderr, "option for %s : /%s=integer, %s=INDEX/\n", clientParams->cmd, token[0], token[1]);
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}


#if BACKGROUND_AS_STRING	
static int	_osdBackgroundOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		"color",
		"index",
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret= EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					if(!strcasestr(value, "0X") )
					{
						fprintf(stderr, "value of '%s' must be in format of '0xFFRRGGBB'\n", token[0]);
						ret = EXIT_FAILURE;
						goto _ret;
					}
					if( strlen(value) != 10 )
					{
						fprintf(stderr, "value of '%s' must be length of 10: %s\n", token[0], value);
						ret = EXIT_FAILURE;
						goto _ret;
					}
					
					snprintf(clientParams->backgroundColor, sizeof(clientParams->backgroundColor), "%s", value);
					fprintf(stderr, "'%s'='%s' (%s)\n", token[0], value, clientParams->backgroundColor );
				}
				break;

			case 1:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[1]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[1], value);
					clientParams->index = atoi(value);
				}
				break;

			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}
	}

_ret:
	if(ret)
	{
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}
#endif

static int	_osdColorOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		"color",
		"index",
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret= EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					clientParams->color = cmnParseGetHexIntValue(value);
					fprintf(stderr, "'%s'='%s' (%d, 0x%x)\n", token[0], value, clientParams->color, clientParams->color );
				}
				break;

			case 1:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[1]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[1], value);
					clientParams->index = atoi(value);
				}
				break;

			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}
	}

_ret:
	if(ret)
	{
//		fprintf(stderr, "option for %s : /%s=integer, %s=INDEX/\n", clientParams->cmd, token[0], token[1]);
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}

static int	_osdFontsizeOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		"size",
		"index",
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret= EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[0], value);
					clientParams->color = atoi(value);
				}
				break;

			case 1:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[1]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[1], value);
					clientParams->index = atoi(value);
				}
				break;

			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}
	}

_ret:
	if(ret)
	{
//		fprintf(stderr, "option for %s : /%s=integer, %s=INDEX/\n", clientParams->cmd, token[0], token[1]);
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}


static int	_positionOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		"index",
		"left",
		"top",
		"width",
		"height",
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret= EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[0], value);
					clientParams->index = atoi(value);
				}
				break;

			case 1:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[1]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[1], value);
					clientParams->left = atoi(value);
				}
				break;

			case 2:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[2]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[2], value);
					clientParams->top = atoi(value);
				}
				break;
			case 3:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[3]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[3], value);
					clientParams->width = atoi(value);
				}
				break;
			case 4:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[4]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[4], value);
					clientParams->height = atoi(value);
				}
				break;

			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}
	}

_ret:
	if(ret)
	{
//		fprintf(stderr, "option for %s : /%s=WIN_INDEX, %s=MEDIAFILE/\n", clientParams->cmd, token[0], token[1]);
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}


static int	_recordStartOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		"duration",
		"media",
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret= EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[0], value);
					clientParams->duration = atoi(value);
				}
				break;

			case 1:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[1]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[1], value);
					snprintf(clientParams->media, sizeof(clientParams->media), "%s", value);
				}
				break;

			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}
	}

_ret:
	if(ret)
	{
//		fprintf(stderr, "option for %s : /%s=DURATION, %s=MEDIA/\n", clientParams->cmd, token[0], token[1]);
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}


static int	_downloadOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		"media",
		"server",
		"user",
		"password",
		"path",
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[0], value);
					snprintf(clientParams->media, sizeof(clientParams->media), "%s", value);
				}
				break;
				
			case 1:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[1]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[1], value);
					snprintf(clientParams->ftpSvr, sizeof(clientParams->ftpSvr), "%s", value);
				}
				break;
			case 2:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[2]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[2], value);
					snprintf(clientParams->ftpUser, sizeof(clientParams->ftpUser), "%s", value);
				}
				break;
			case 3:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[3]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[3], value);
					snprintf(clientParams->ftpPasswd, sizeof(clientParams->ftpPasswd), "%s", value);
				}
				break;
			case 4:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[4]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[4], value);
					snprintf(clientParams->ftpPath, sizeof(clientParams->ftpPath), "%s", value);
				}
				break;

			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}
	}

_ret:
	if(ret)
	{
//		fprintf(stderr, "option for %s : /%s=MEDIA,%s=SERVER,%s=USER,%s=PASSWORD,%s=PATH/\n", clientParams->cmd, token[0], token[1], token[2], token[3], token[4]);
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}

static int	_fileAddOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		"media",
		"file",
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value for '%s'\n", token[0]);
					ret = EXIT_FAILURE;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[0], value);
					snprintf(clientParams->media, sizeof(clientParams->media), value);
				}
				break;
				
			case 1:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value for '%s'\n", token[1]);
					ret = EXIT_FAILURE;
				}
				else
				{
					char *file = NULL;
					
					fprintf(stderr, "'%s'='%s'\n", token[1], value);
					file = cmn_malloc(strlen(value)+1);
					snprintf(file, strlen(value)+1, value);
					
					cmn_list_append(&clientParams->files, file);
				}
				break;

			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				break;
		}
	}

	if(ret)
	{
//		fprintf(stderr, "option for %s : /%s=WIN_INDEX  %s=MEDIAFILE/\n", clientParams->cmd,  token[0], token[1]);
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}


static int	_playListAddOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;
	PLAY_ITEM *playItem = NULL;

	char *const token[] =
	{
		"media",
		PLAYLIST_NAME_URL,
		PLAYLIST_NAME_DURATION,
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[0], value);
					snprintf(clientParams->media, sizeof(clientParams->media), "%s", value);
				}
				break;
				
			case 1:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[1]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
//					char *file = NULL;
					
					fprintf(stderr, "'%s'='%s'\n", token[1], value);
					if(playItem != NULL)
					{
						fprintf(stderr, "Missing value of '%s'\n", token[2]);
						ret = EXIT_FAILURE;
						goto _ret;
					}
					
					playItem = cmn_malloc(sizeof(PLAY_ITEM));
					//file = cmn_malloc(strlen(value)+1);
					snprintf(playItem->filename, CMN_NAME_LENGTH, "%s",value);
					
//					cmn_list_append(&clientParams->files, file);
				}
				break;
				
			case 2:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[2]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
//					char *file = NULL;
					
					fprintf(stderr, "'%s'='%s'\n", token[2], value);
//					file = cmn_malloc(strlen(value)+1);
					if(playItem == NULL)
					{
						fprintf(stderr, "Missing value of '%s' before '%s'\n", token[1], token[2]);
						ret = EXIT_FAILURE;
						goto _ret;
					}
					
					playItem->duration = atoi(value);
					if(playItem->duration < 0 )
					{
						fprintf(stderr, "'%s' is invalidate value %d\n", token[1], playItem->duration);
						ret = EXIT_FAILURE;
						goto _ret;
					}
					
					cmn_list_append(&clientParams->files, playItem);
					playItem = NULL;
				}
				break;

			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}

	}
	
	if(playItem != NULL)
	{
		fprintf(stderr, "'%s' of '%s' missing value of '%s'\n", token[1], playItem->filename, token[2]);
		ret = EXIT_FAILURE;
	}

_ret:
	if(ret)
	{
//		fprintf(stderr, "option for %s : /%s=WIN_INDEX  %s=MEDIAFILE/\n", clientParams->cmd,  token[0], token[1]);
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}


static int	_edidDeepColorOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		IPCMD_EDID_DEEP_COLOR,
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret= EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[0], value);
					clientParams->color = atoi(value);
					if(clientParams->color< 0 || clientParams->color > 2)
					{
						ret = EXIT_FAILURE;
						goto _ret;
					}
				}
				break;

			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}
	}

_ret:
	if(ret)
	{
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}


static int	_edidResolutionOptions(struct API_CLIENT_OPT_HANDLER *handler, char *optarg, struct API_PARAMETERS *clientParams)
{
	char *subopts;
	char *value;
	int ret = EXIT_SUCCESS;

	char *const token[] =
	{
		IPCMD_EDID_RESOLUTION,
		NULL
	};

	
	subopts = optarg;
	
	while (*subopts != '\0' && ret != EXIT_FAILURE )
	{
		switch (getsubopt(&subopts, token, &value))
		{
			case 0:
				if (value == NULL)
				{
					fprintf(stderr, "Missing value of '%s'\n", token[0]);
					ret = EXIT_FAILURE;
					goto _ret;
				}
				else
				{
					fprintf(stderr, "'%s'='%s'\n", token[0], value);
					snprintf(clientParams->media, sizeof(clientParams->media), "%s",value);
				}
				break;

			default:
				fprintf(stderr, "No match found for token: /%s/\n", value);
				ret = EXIT_FAILURE;
				goto _ret;
				break;
		}
	}

_ret:
	if(ret)
	{
		fprintf(stderr, "option for %s : /%s/\n", clientParams->cmd, handler->prompt);
	}

	return ret;
}


static API_CLIENT_OPT_HANDLER _optionHandlers[] =
{
	{
		CLIENT_CMD_PLAY,
		"play",
		"index='WINDOW',media='MEDIA(File or Playlist),repeat=n'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_STOP,
		"stop",
		"index='WINDOW'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_PAUSE,
		"pause",
		"index='WINDOW'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_RESUME,
		"resume",
		"index='WINDOW'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_FORWARD,
		"forward",
		"index='WINDOW'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_BACKWARD,
		"backforward",
		"index='WINDOW'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_SUBTITLE,
		"subtitle",
		"media='subtitle file'",
		_playSubOptions,
	},

	{
		CLIENT_CMD_PLAY_INFO,
		"playerInfo",
		"index='WINDOW'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_PLAY_MEDIA,
		"mediaInfo",
		"index='WINDOW'",
		_playSubOptions,
	},


		
	{
		CLIENT_CMD_RECORD_START,
		"recordStart",
		"duration='SECONDS',media='MEDIA(full name of record file)'",
		_recordStartOptions,
	},
	{
		CLIENT_CMD_RECORD_STOP,
		"recordStop",
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_RECORD_STATUS,
		"recordStatus",
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_ALERT,
		"alert",
		"color='COLOR(0xFFRRGGBB)',media='text message'",
		_alertOptions,
	},
	{
		CLIENT_CMD_LOGO,
		"logo",
		"media='Icon File'",
		_alertOptions,
	},
	{
		CLIENT_CMD_OSD_ENABLE,
		"osdEnable",
		"index='OSD Index(0:subtitle;1:alert; 2:logo, default is subtitle)'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_OSD_BACKGROUND,
		"osdBackground",
		"index='OSD Index(0:subtitle;1:alert; 2:logo, default is subtitle)',color='Color of background(0xFFRRGGBB)'",
#if BACKGROUND_AS_STRING	
		_osdBackgroundOptions,
#else
		_osdColorOptions,
#endif		
	},
	{
		CLIENT_CMD_OSD_TRANSPANRENCY,
		"osdTransparency",
		"index='OSD Index(0:subtitle;1:alert; 2:logo, default is subtitle)',color='transparency(0~255)'",
		_osdTransparencyOptions,
	},
	{
		CLIENT_CMD_OSD_FONTCOLOR,
		"osdFontColor",
		"index='OSD Index(0:subtitle;1:alert; 2:logo, default is subtitle)',color='Font Color(0xFFRRGGBB)'",
		_osdColorOptions,
	},
	{
		CLIENT_CMD_OSD_FONTSIZE,
		"osdFontSize",
		"index='OSD Index(0:subtitle;1:alert; 2:logo, default is subtitle)',size='Font Size'",
		_osdFontsizeOptions,
	},
	{
		CLIENT_CMD_OSD_POSITION,
		"osdPosition",
		"index='OSD index(0:subtitle;1:alert; 2:logo, default is subtitle)',left='LEFT',top='TOP',width='WIDTH',height='HEIGHT'",
		_positionOptions,
	},
	{
		CLIENT_CMD_OSD_INFO,
		IPCMD_NAME_OSD_INFO,
		"index='OSD Index(0:subtitle;1:alert; 2:logo, default is subtitle)'",
		_playSubOptions,
	},

	{
		CLIENT_CMD_SWAP_WINDOW,
		"sawpWindow",
		"index='WINDOW'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_ROTATE_WINDOW,
		"rotateWindow",
		"index='WINDOW'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_LOCATE_WINDOW,
		"locateWindow",
		"index='WINDOW',left='LEFT',top='TOP',width='WIDTH',height='HEIGHT'",
		_positionOptions,
	},

	{
		CLIENT_CMD_VOL_PLUS,
		"vol+",
		"index='WINDOW'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_VOL_MINUS,
		"vol-",
		"index='WINDOW'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_AUDIO,
		IPCMD_NAME_PLAYER_AUDIO,
		"index='WINDOW',media='YES|NO'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_MUTE,
		IPCMD_NAME_PLAYER_MUTE,
		"index='WINDOW'",
		_playSubOptions,
	},

	{
		CLIENT_CMD_ASPECT_WINDOW,
		IPCMD_NAME_ASPECT_WINDOW,
		"index='WINDOW',mode='MODE(0-3)'",
		_aspectOptions,
	},
	
	{
		CLIENT_CMD_MUTE_ALL,
		IPCMD_NAME_PLAYER_MUTE_ALL,
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_QUIT,
		"quit",
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_FILES,
		"files",
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_PLAYLISTS,
		"playlists",
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_FILE,
		"file",
		"media='Name of media file'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_PLAYLIST,
		"playlist",
		"media='Name of Playlist'",
		_playSubOptions,
	},
	{
		CLIENT_CMD_PLAYLIST_ADD,
		"playlistAdd",
		"media='Name of Playlist',"PLAYLIST_NAME_URL"='File01',"PLAYLIST_NAME_DURATION"='Time01(second)'...,"PLAYLIST_NAME_URL"='File0n',"PLAYLIST_NAME_DURATION"='Time0n(second)'",
		_playListAddOptions,
	},
	{
		CLIENT_CMD_FILE_DELETE,
		"fileDelete",
		"file='File01',...,file='File0n'",
		_fileAddOptions,
	},
	{
		CLIENT_CMD_PLAYLIST_DELETE,
		"playlistDelete",
		"file='playlist01',...,file='playlist0n'",
		_fileAddOptions,
	},
	{
		CLIENT_CMD_DOWNLOAD,
		"download",
		"media='FILE',server='SERVER',user='USER',password='PASSWORD',path='PATH'",
		_downloadOptions,
	},

	/* commands for CEC command */
	{
		CLIENT_CMD_CEC_STANDBY,
		IPCMD_CEC_STAND_BY,
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_CEC_IMAGE_ON,
		IPCMD_CEC_IMAGE_VIEW_ON,
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_CEC_VOLUME_UP,
		IPCMD_CEC_VOLUME_UP,
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_CEC_VOLUME_DOWN,
		IPCMD_CEC_VOLUME_DOWN,
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_CEC_MUTE,
		IPCMD_CEC_MUTE,
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_CEC_INFO,
		IPCMD_CEC_INFO,
		"No optional params",
		_dumpParseSubOptions,
	},

	/* commmand EDID */
	{
		CLIENT_CMD_EDID_RESOLUTION,
		IPCMD_EDID_RESOLUTION,
		"resolution='Format', such as '1080P_60', etc.",
		_edidResolutionOptions,
	},
	{
		CLIENT_CMD_EDID_DEEP_COLOR,
		IPCMD_EDID_DEEP_COLOR,
		"color='0|1|2', for 8|10|12 bits color respectively",
		_edidDeepColorOptions,
	},

	/* commands for SERVER */
	{
		CLIENT_CMD_SVR_CONFIG,
		CLIENT_CMD_NAME_SVR_CONFIG,
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_SVR_FEEDS,
		CLIENT_CMD_NAME_SVR_FEEDS,
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_SVR_CONNS,
		CLIENT_CMD_NAME_SVR_CONNS,
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_SVR_URLS,
		CLIENT_CMD_NAME_SVR_URLS,
		"No optional params",
		_dumpParseSubOptions,
	},


	/* commands for WEB */
	{
		CLIENT_CMD_WEB_INFOS,
		CLIENT_CMD_NAME_WEB_INFO,
		"No optional params",
		_dumpParseSubOptions,
	},


	/* commands for System Admin */
	{
		CLIENT_CMD_SYS_ADMIN_THREADS,
		IPCMD_SYS_ADMIN_THREADS,
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_SYS_ADMIN_VER_INFO,
		IPCMD_SYS_ADMIN_VER_INFO,
		"No optional params",
		_dumpParseSubOptions,
	},
	{
		CLIENT_CMD_UNKNOWN,
		NULL,
		NULL,
		NULL
	}
};


int	apiClientParseSubOptions(char *optarg, struct API_PARAMETERS *clientParams)
{
	API_CLIENT_OPT_HANDLER *handle = _optionHandlers;

	while(handle->handler != NULL)
	{
		if(!strcasecmp(handle->name, clientParams->cmd) )
		{
			clientParams->cmdType = handle->type;
			return handle->handler(handle, optarg, clientParams);
		}

		handle++;
	}

	return EXIT_FAILURE;
}



char	*apiClientOptionsPrompt(struct API_PARAMETERS *clientParams)
{
	API_CLIENT_OPT_HANDLER *handle = _optionHandlers;

	while(handle->handler != NULL)
	{
		if(!strcasecmp(handle->name, clientParams->cmd) )
		{
			return handle->prompt;
		}

		handle++;
	}

	return "";
}


