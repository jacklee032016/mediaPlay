

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"


/* subtitle can only be set for main window, so window parameter is provided in JSON command */
static int	_muxJSonSetSubtitle(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	
	int res = EXIT_SUCCESS;
	MUX_PLAY_T *player = muxPlayerFindByType(muxRx, RECT_TYPE_MAIN);
	
	char *media = cmnGetStrFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_MEDIA);
	if(media == NULL || !strlen(media))
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'media' is not defined");
		return res;
	}

	memset(&player->playerMedia, 0, sizeof(player->playerMedia));
	memcpy(player->playerMedia.aszExtSubUrl[0], media, strlen(media) );
	player->playerMedia.u32ExtSubNum = 1;

	MUX_PLAY_INFO( "### open external sub file is %s \n", player->playerMedia.aszExtSubUrl[0]);
	res = HI_SVR_PLAYER_SetMedia(player->playerHandler, HI_SVR_PLAYER_MEDIA_SUBTITLE, &player->playerMedia);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_WARN("Set sub file '%s' failed: 0x%x", player->playerMedia.aszExtSubUrl[0], res);
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Maybe subtitle has been set now");
	}
	else
	{
		dataConn->errCode =  IPCMD_ERR_NOERROR;
	}
	
	return res;
}


static int	_muxJSonMuteAll(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	int res = EXIT_SUCCESS;
	if(muxRx->isMute)
		muxRx->isMute = FALSE;
	else
		muxRx->isMute = TRUE;
		
	res = HI_UNF_SND_SetMute(HI_UNF_SND_0, HI_UNF_SND_OUTPUTPORT_ALL, muxRx->isMute);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_WARN( "Sound SetMute failed");
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Please re-try later");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return res;
}


static int	_muxJSonQuit(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
//	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	int res = EXIT_SUCCESS;

	dataConn->errCode = IPCMD_ERR_NOERROR;
//	SYSTEM_SIGNAL_EXIT();

	return res;
}

static int	_muxJSonAlertMsg(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	int res = EXIT_SUCCESS, ret;
	int fontColor; 
	char *message;	

	message = cmnGetStrFromJsonObject(jsonEvent->object, "message");
	if( IS_STRING_NULL( message) )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'message' is not defined");
		return res;
	}

	fontColor = cmnGetIntegerFromJsonObject(jsonEvent->object, "fontcolor");
	if(fontColor == -1)
	{
		fontColor = COLOR_GREEN;
	}
#if 0
	fontSize = cmnGetIntegerFromJsonObject(jsonEvent->object, "fontsize");
	if(fontSize == -1)
	{
		fontSize = 48;
	}
#endif

	ret= OSD_DESKTOP_LOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is locked for AlertMsg: %s", strerror(errno) );
		return HI_SUCCESS;
	}
	
	res = muxOutputAlert(muxRx, fontColor, message);
	
	ret = OSD_DESKTOP_UNLOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is unlocked for AlertMsg: %s", strerror(errno) );
		return HI_SUCCESS;
	}
	
	if(res != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Please re-try later");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}


 	return res;
}

static int	_muxJSonOsdPosition(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
#define	PARAMETER_CHECK(x, y , msg) \
	if(x == -1 ||y == -1 ) { \
		if(x != y ) { cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Position parameter wrong"); \
			return res; } \
		x = y = -1;	}

	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	int res = EXIT_SUCCESS, ret;
	MUX_OSD *osd = NULL;
	HI_RECT_S rect;
	
	int index = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_OSD);
	osd = muxOsdFind(&muxRx->higo, (MUX_OSD_TYPE)index);
	if(!osd)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'osd' parameter error");
		return res;
	}

	rect.s32X = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_LOCATION_X);
	rect.s32Y = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_LOCATION_Y);
	PARAMETER_CHECK(rect.s32X, rect.s32Y, "Position parameter of OSD wrong");
	
	rect.s32Width = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_LOCATION_W);
	rect.s32Height = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_LOCATION_H);
	PARAMETER_CHECK(rect.s32Width, rect.s32Height, "Size parameter of OSD wrong");

	ret= OSD_DESKTOP_LOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is locked for OsdPosition: %s", strerror(errno) );
		return HI_SUCCESS;
	}
	res = muxOsdPosition(osd, &rect, NULL);
	
	ret = OSD_DESKTOP_UNLOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is unlocked for OsdPosition: %s", strerror(errno) );
		return HI_SUCCESS;
	}

	if(res != EXIT_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "media for logo set failed, maybe media format is not support");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}


	return res;
}

static int	_muxJSonOsdEnable(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	int res = EXIT_SUCCESS, ret;
	MUX_OSD *osd = NULL;
	
	int index = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_OSD);
	osd = muxOsdFind(&muxRx->higo, (MUX_OSD_TYPE)index);
	if(!osd)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'osd' parameter error");
		return res;
	}

	ret= OSD_DESKTOP_LOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is locked for OsdEnable: %s", strerror(errno) );
		return HI_SUCCESS;
	}

	res = muxOsdToggleEnable(osd);
	
	ret = OSD_DESKTOP_UNLOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is unlocked for OsdEnable: %s", strerror(errno) );
		return HI_SUCCESS;
	}
	
	if(res != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Toggle enable/disable failed");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}


	return res;
}


static int	_muxJSonOsdBackground(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	int res = EXIT_SUCCESS, ret;
	MUX_OSD *osd = NULL;
#if BACKGROUND_AS_STRING	
	char	*bgStr;
#endif
	int background;
	
	int index = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_OSD);
	osd = muxOsdFind(&muxRx->higo, (MUX_OSD_TYPE)index);
	if(!osd)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'osd' parameter error");
		return res;
	}
#if BACKGROUND_AS_STRING	
	bgStr = cmnGetStrFromJsonObject(jsonEvent->object, "background");
	background = cmnParseGetHexIntValue(bgStr);
	MUX_PLAY_DEBUG("background is :%s:%#x", bgStr, background);
#else
	background = cmnGetIntegerFromJsonObject(jsonEvent->object, "background");
	if(background == -1)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "No 'background' parameter");
		return res;
	}
#endif

	ret= OSD_DESKTOP_LOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is locked for OsdBackground: %s", strerror(errno) );
		return HI_SUCCESS;
	}

	res = muxOsdSetBackground(osd, background);
	
	ret = OSD_DESKTOP_UNLOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is unlocked for OsdBackground: %s", strerror(errno) );
		return HI_SUCCESS;
	}
	
	if(res != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Set background operation failed, try late");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return res;
}


static int	_muxJSonOsdTransparency(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	int res = EXIT_SUCCESS, ret;
	MUX_OSD *osd = NULL;
	char alpha;
	
	int index = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_OSD);
	osd = muxOsdFind(&muxRx->higo, (MUX_OSD_TYPE)index);
	if(!osd)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'osd' parameter error");
		return res;
	}

	alpha = (char )cmnGetIntegerFromJsonObject(jsonEvent->object, "transparency");
	if(alpha == -1)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "No 'transparency' parameter");
		return res;
	}

	ret= OSD_DESKTOP_LOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is locked for OsdTransparency: %s", strerror(errno) );
		return HI_SUCCESS;
	}

	res = muxOsdSetTransparency(osd, alpha);
	
	ret = OSD_DESKTOP_UNLOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is unlocked for OsdTransparency: %s", strerror(errno) );
		return HI_SUCCESS;
	}
	
	if(res != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Set transparency operation failed, try late");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return res;
}


static int	_muxJSonOsdFontColor(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	int res = EXIT_SUCCESS, ret;
	MUX_OSD *osd = NULL;
	int	fontcolor;
	
	int index = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_OSD);
	osd = muxOsdFind(&muxRx->higo, (MUX_OSD_TYPE)index);
	if(!osd)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'osd' parameter error");
		return res;
	}

	fontcolor = cmnGetIntegerFromJsonObject(jsonEvent->object, "fontcolor");
	if(fontcolor == -1)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "No 'fontcolor' parameter");
		return res;
	}

	ret= OSD_DESKTOP_LOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is locked for OsdFontColor: %s", strerror(errno) );
		return HI_SUCCESS;
	}

	res = muxOsdSetFontColor(osd, fontcolor);
	
	ret = OSD_DESKTOP_UNLOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is unlocked for OsdFontColor: %s", strerror(errno) );
		return HI_SUCCESS;
	}
	
	if(res != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Set FontColor operation failed, try late");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return res;
}

static int	_muxJSonOsdFontSize(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	int res = EXIT_SUCCESS, ret;
	MUX_OSD *osd = NULL;
	int fontSize;
	
	int index = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_OSD);
	osd = muxOsdFind(&muxRx->higo, (MUX_OSD_TYPE)index);
	if(!osd)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'osd' parameter error");
		return res;
	}

	fontSize = cmnGetIntegerFromJsonObject(jsonEvent->object, "fontsize");
	if(fontSize == -1)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "No 'fontsize' parameter");
		return res;
	}

	ret= OSD_DESKTOP_LOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is locked for OsdFontSize: %s", strerror(errno) );
		return HI_SUCCESS;
	}

	res = muxOsdSetFontSize(osd, fontSize);
	
	ret = OSD_DESKTOP_UNLOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is unlocked for OsdFontSize: %s", strerror(errno) );
		return HI_SUCCESS;
	}
	
	if(res != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "Set FontSize operation failed, try late");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return res;
}

static int	_muxJSonOsdLogo(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	int res = EXIT_SUCCESS, ret;
	
	char *media = cmnGetStrFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_MEDIA);
	if(media == NULL || !strlen(media))
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'media' is not defined for LOGO OSD");
		return res;
	}

	MUX_OSD *logoOsd = muxOsdFind(&muxRx->higo, MUX_OSD_TYPE_LOGO);

	snprintf(logoOsd->cfg->url, 1024, "%s", media);
	
	ret= OSD_DESKTOP_LOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is locked for OsdLogo: %s", strerror(errno) );
		return HI_SUCCESS;
	}

	res = muxOsdImageShow(logoOsd, logoOsd->cfg->url);
	
	ret = OSD_DESKTOP_UNLOCK(&muxRx->higo);
	if(ret != 0)
	{
		MUX_PLAY_WARN( "Higo is unlocked for OsdLogo: %s", strerror(errno) );
		return HI_SUCCESS;
	}
	
	if(res != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "media for logo set failed, maybe media format is not support");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}


	return res;
}



static int	_muxJSonOsdInfo(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	int res = EXIT_SUCCESS;
	MUX_OSD *osd = NULL;
	char		buf[128];
	
	int index = cmnGetIntegerFromJsonObject(jsonEvent->object, _MUX_JSON_NAME_OSD);
	osd = muxOsdFind(&muxRx->higo, (MUX_OSD_TYPE)index);
	if(!osd)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'osd' index parameter '%d' error", index);
		return res;
	}

	JEVENT_ADD_STRING( jsonEvent->object, "name", osd->name);
//	JEVENT_ADD_INTEGER( jsonEvent->object, "index", osd->type);
	JEVENT_ADD_STRING( jsonEvent->object, "Enable", STR_BOOL_VALUE(osd->enable));

	JEVENT_ADD_INTEGER(jsonEvent->object, _MUX_JSON_NAME_LOCATION_X, osd->cfg->left);
	JEVENT_ADD_INTEGER(jsonEvent->object, _MUX_JSON_NAME_LOCATION_Y, osd->cfg->top);
	JEVENT_ADD_INTEGER(jsonEvent->object, _MUX_JSON_NAME_LOCATION_W, osd->cfg->width);
	JEVENT_ADD_INTEGER(jsonEvent->object, _MUX_JSON_NAME_LOCATION_H, osd->cfg->height);

	JEVENT_ADD_INTEGER(jsonEvent->object, "fontsize", osd->cfg->fontSize );

	snprintf(buf, sizeof(buf), "%#.8X", osd->cfg->fontColor);
	JEVENT_ADD_STRING(jsonEvent->object, "fontcolor", buf );

	snprintf(buf, sizeof(buf), "%#.8X", osd->cfg->backgroundColor);
	JEVENT_ADD_STRING(jsonEvent->object, "background", buf );

	snprintf(buf, sizeof(buf), "%#.2X", osd->cfg->alpha);
	JEVENT_ADD_STRING(jsonEvent->object, "alpha", buf );

	dataConn->errCode = IPCMD_ERR_NOERROR;

	return res;
}


static int	_muxJSonCecStandby(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	int ret = EXIT_SUCCESS;
	unsigned char oprand[10];

	/* see spec1.4, P.292*/
	ret = muxHdmiCECCommand(HI_UNF_CEC_LOGICALADD_TUNER_1, HI_UNF_CEC_LOGICALADD_TV, CEC_OPCODE_STANDBY, 0, oprand, 0);
	if(ret != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "CEC Command failed");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return ret;
}

static int	_muxJSonCecImageOn(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	int ret = EXIT_SUCCESS;
	unsigned char oprand[10];

	ret = muxHdmiCECCommand(HI_UNF_CEC_LOGICALADD_TUNER_1, HI_UNF_CEC_LOGICALADD_TV, CEC_OPCODE_IMAGE_VIEW_ON, 0, oprand, 0);
/*
else if (strcmp(value_of_key_recieved, "IMAGE_VIEW_ON") == 0)
{
	ret = hdmi_cec_setcmd(0, 0x04, oprand, 0);
}
*/
	if(ret != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "CEC Command failed");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return ret;
}


static int	_muxJSonCecVolumeUp(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	int ret = EXIT_SUCCESS;
	unsigned char oprand[10];

	oprand[0] = HI_UNF_CEC_UICMD_VOLUME_UP/* 0x41*/;
	ret = muxHdmiCECCommand(HI_UNF_CEC_LOGICALADD_PLAYDEV_1, HI_UNF_CEC_LOGICALADD_TV, CEC_OPCODE_USER_CONTROL_PRESSED, HI_UNF_CEC_UICMD_VOLUME_UP, oprand, 0);
/*
else if (strcmp(value_of_key_recieved, "VOLUME_UP") == 0)
{
	oprand[0] = 0x41;
	ret = hdmi_cec_setcmd(0, 0x36, oprand, 1);
}
*/
	if(ret != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "CEC Command failed");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return ret;
}


static int	_muxJSonCecVolumeDown(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	int ret = EXIT_SUCCESS;
	unsigned char oprand[10];

	oprand[0] = HI_UNF_CEC_UICMD_VOLUME_DOWN/*0x42*/;
//	ret = muxHdmiCECCommand(HI_UNF_CEC_LOGICALADD_PLAYDEV_1, HI_UNF_CEC_LOGICALADD_TV, CEC_OPCODE_USER_CONTROL_PRESSED, HI_UNF_CEC_UICMD_POWER_OFF_FUNCTION, oprand, 0);
	ret = muxHdmiCECCommand(HI_UNF_CEC_LOGICALADD_TUNER_1, HI_UNF_CEC_LOGICALADD_TV, CEC_OPCODE_USER_CONTROL_PRESSED, HI_UNF_CEC_UICMD_VOLUME_DOWN, oprand, 0);
/*
else if (strcmp(value_of_key_recieved, "VOLUME_DOWN") == 0)
{
	oprand[0] = 0x42;
	ret = hdmi_cec_setcmd(0, 0x36, oprand, 1);
}
*/
	if(ret != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "CEC Command failed");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return ret;
}


static int	_muxJSonCecMute(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	int ret = EXIT_SUCCESS;
	unsigned char oprand[10];

/*40 44 43*/
//	oprand[0] = 0x80;
	oprand[0] = 0xE3;
//	ret = muxHdmiCECCommand(HI_UNF_CEC_LOGICALADD_PLAYDEV_1, HI_UNF_CEC_LOGICALADD_TV, CEC_OPCODE_USER_CONTROL_PRESSED, HI_UNF_CEC_UICMD_MUTE, oprand, 0);
	ret = muxHdmiCECCommand(HI_UNF_CEC_LOGICALADD_TUNER_1, HI_UNF_CEC_LOGICALADD_TV, CEC_OPCODE_USER_CONTROL_PRESSED, HI_UNF_CEC_UICMD_MUTE_FUNCTION, oprand, 0);
/*
else if (strcmp(value_of_key_recieved, "MUTE") == 0)
{
	oprand[0] = 0x43;
	ret = hdmi_cec_setcmd(0, 0x36, oprand, 1);
}
*/
	if(ret != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "CEC Command failed");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return ret;
}


static int	_muxJSonCecInfo(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
//	MUX_RX_T *muxRx = (MUX_RX_T *)priv;
	int ret = EXIT_SUCCESS;
#if 0
	unsigned char oprand[10];
	oprand[0] = 0x00;
	oprand[1] = 0x48; 
	oprand[2] = 0x65;
//	ret = muxHdmiCECCommand(HI_UNF_CEC_LOGICALADD_BROADCAST, HI_UNF_CEC_LOGICALADD_TV, CEC_OPCODE_SET_OSD_STRING, 0, oprand, 3);
	ret = muxHdmiCECCommand(HI_UNF_CEC_LOGICALADD_TUNER_1, HI_UNF_CEC_LOGICALADD_TV, CEC_OPCODE_USER_CONTROL_PRESSED, HI_UNF_CEC_UICMD_POWER_OFF_FUNCTION, oprand, 0);

	/*
F0:64:00:48:65:6C:6C:6F:20:77:6F:72:6C:64	
00 is the "display control" parameter set to "display for default time", and 48:65:6C:6C:6F:20:77:6F:72:6C:64 is the "OSD String" parameter set to "Hello world". 
	*/
#endif
	int i;
	HI_UNF_EDID_BASE_INFO_S sinkAttr;
	cJSON *formats = NULL;

	ret = HI_UNF_HDMI_GetSinkCapability(HI_UNF_HDMI_ID_0, &sinkAttr);
	if( ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get HDMI Sink Capability failed: %#x", ret);
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "CEC Command failed, maybe HDMI is not working now!");
		return ret;
	}

	JEVENT_ADD_STRING( jsonEvent->object, "Manufacture", (char *)sinkAttr.stMfrsInfo.u8MfrsName );
	JEVENT_ADD_STRING( jsonEvent->object, "NativeFormat", muxHdmiFormatName(sinkAttr.enNativeFormat) );

	JEVENT_ADD_ARRAY(jsonEvent->object, "Formats", formats);
	for(i=0; i< HI_UNF_ENC_FMT_BUTT; i++ )
	{
		if(sinkAttr.bSupportFormat[i])
		{
			cJSON_AddItemToArray(formats, cJSON_CreateString(muxHdmiFormatName((HI_UNF_ENC_FMT_E)i)));
		}
	}

	if(ret != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "CEC Command failed");
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return ret;
}


static int	_muxJSonEdidResolution(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	int ret = EXIT_SUCCESS;
	HI_UNF_ENC_FMT_E enForm;

	char *formatStr;
	formatStr = cmnGetStrFromJsonObject(jsonEvent->object, IPCMD_EDID_RESOLUTION);
	if( IS_STRING_NULL( formatStr) )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'%s' is not defined", IPCMD_EDID_RESOLUTION);
		return ret;
	}

	enForm = (HI_UNF_ENC_FMT_E ) muxHdmiFormatCode(formatStr);
	if( (enForm < HI_UNF_ENC_FMT_1080P_60) || (enForm >= HI_UNF_ENC_FMT_BUTT) )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'%s' parameter '%d' error", IPCMD_EDID_RESOLUTION, enForm);
		return ret;
	}

	ret = muxHdmiConfigFormat(enForm );
	if(ret != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "'%s' Command failed", IPCMD_EDID_RESOLUTION);
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return ret;
}

static int	_muxJSonEdidDeepColor(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jsonEvent)
{
	int ret = EXIT_SUCCESS;
	int deepColor;

	deepColor = cmnGetIntegerFromJsonObject(jsonEvent->object, IPCMD_EDID_DEEP_COLOR);
	if( (deepColor < HI_UNF_HDMI_DEEP_COLOR_24BIT) || (deepColor> HI_UNF_HDMI_DEEP_COLOR_36BIT) )
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_DATA_ERROR, "'%s' parameter '%d' error", IPCMD_EDID_DEEP_COLOR, deepColor);
		return ret;
	}

	ret = muxHdmiConfigDeepColor((HI_UNF_HDMI_DEEP_COLOR_E) deepColor);
	if(ret != HI_SUCCESS)
	{
		cmnMuxJsonControllerReply(dataConn, IPCMD_ERR_SERVER_INTERNEL_ERROR, "'%s' Command failed", IPCMD_EDID_DEEP_COLOR);
	}
	else
	{
		dataConn->errCode = IPCMD_ERR_NOERROR;
	}

	return ret;
}



PluginJSonHandler jsonMuxPlayActionHandlers[] =
{
	/* global control, without window parameter, synchronous command*/	
	{
		type	: CMD_TYPE_SUBTITLE,
		name 	: "subtitle",
		handler	: _muxJSonSetSubtitle
	},
	{
		type	: CMD_TYPE_MUTE_ALL,
		name 	: IPCMD_NAME_PLAYER_MUTE_ALL,
		handler	: _muxJSonMuteAll
	},


	{
		type	: CMD_TYPE_QUIT,
		name 	: "quit",
		handler	: _muxJSonQuit
	},

	/* OSD controllers */
	{
		type	: CMD_TYPE_ALERT_MESSAGE,
		name 	: "alert",
		handler	: _muxJSonAlertMsg
	},
	{
		type	: CMD_TYPE_OSD_ENABLE,
		name 	: "osdEnable",
		handler	: _muxJSonOsdEnable
	},
	{
		type	: CMD_TYPE_OSD_POSITION,
		name 	: "osdPosition",
		handler	: _muxJSonOsdPosition
	},

	{
		type	: CMD_TYPE_OSD_BACKGROUND,
		name 	: "osdBackground",
		handler	: _muxJSonOsdBackground
	},
	{
		type	: CMD_TYPE_OSD_TRANSPANRENCY,
		name 	: "osdTransparency",
		handler	: _muxJSonOsdTransparency
	},
	{
		type	: CMD_TYPE_OSD_FONT_SIZE,
		name 	: "osdFontSize",
		handler	: _muxJSonOsdFontSize
	},
	{
		type	: CMD_TYPE_OSD_FONT_COLOR,
		name 	: "osdFontColor",
		handler	: _muxJSonOsdFontColor
	},

	{
		type	: CMD_TYPE_OSD_FONT_INFO,
		name 	: IPCMD_NAME_OSD_INFO,
		handler	: _muxJSonOsdInfo
	},

	{
		type	: CMD_TYPE_OSD_LOGO,
		name 	: "logo",
		handler	: _muxJSonOsdLogo
	},

	/* CEC commands */
	{
		type	: CMD_TYPE_CEC_STAND_BY,
		name 	: IPCMD_CEC_STAND_BY,
		handler	: _muxJSonCecStandby
	},
	{
		type	: CMD_TYPE_CEC_IMAGE_ON,
		name 	: IPCMD_CEC_IMAGE_VIEW_ON,
		handler	: _muxJSonCecImageOn
	},
	{
		type	: CMD_TYPE_CEC_VOLUME_UP,
		name 	: IPCMD_CEC_VOLUME_UP,
		handler	: _muxJSonCecVolumeUp
	},
	{
		type	: CMD_TYPE_CEC_VOLUME_DOWN,
		name 	: IPCMD_CEC_VOLUME_DOWN,
		handler	: _muxJSonCecVolumeDown
	},
	{
		type	: CMD_TYPE_CEC_MUTE,
		name 	: IPCMD_CEC_MUTE,
		handler	: _muxJSonCecMute
	},
	{
		type	: CMD_TYPE_CEC_INFO,
		name 	: IPCMD_CEC_INFO,
		handler	: _muxJSonCecInfo
	},

	{
		type	: CMD_TYPE_EDID_RESOLUTION,
		name 	: IPCMD_EDID_RESOLUTION,
		handler	: _muxJSonEdidResolution
	},

	{
		type	: CMD_TYPE_EDID_DEEP_COLOR,
		name 	: IPCMD_EDID_DEEP_COLOR,
		handler	: _muxJSonEdidDeepColor
	},


	{
		type	: CMD_TYPE_UNKNOWN,
		name	: NULL,
		handler	: NULL
	}
};


