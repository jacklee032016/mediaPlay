
/*
* base window of HIGO, provide logo, subtitle, alert info, scroll test display, etc
*/


#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"


static int _muxCreateOSD(int index, void *element, void *pHigo)
{
	MUX_OSD	*osd = NULL;
	HIGO_WNDINFO_S winInfo;
	int res;
	
	MUXLAB_HIGO_T *higo = (MUXLAB_HIGO_T *)pHigo;
	RECT_CONFIG *cfg = (RECT_CONFIG *)element;
#if MUX_WITH_IMAGE_PLAY
#else
	if((cfg->type == RECT_TYPE_MAIN) || (cfg->type == RECT_TYPE_PIP) )
	{
		return HI_SUCCESS;
	}
#endif

	osd = cmn_malloc(sizeof(MUX_OSD));
	if( osd == NULL)
	{
		MUX_PLAY_ERROR("No memory is available for OSD");
		exit(1);
	}

	snprintf(osd->name, sizeof(osd->name), "%s", cfg->name );
	osd->higo = higo;
	osd->cfg = cfg;
	osd->type = MUX_OSD_TYPE_SUBTITLE;

	osd->desktop = higo->layerHandler;

	
	cfg->private = osd;

	MUX_PLAY_DEBUG("OSD '%s' is being created", osd->name);

	/** create the object of text display */    
#if 0
	res = muxOsdCreateFont(osd);
#else
	HIGO_TEXT_INFO_S fontInfo;

	fontInfo.pMbcFontFile = FONT_GB2312_TTFFILE;
	fontInfo.pSbcFontFile = FONT_GB2312_DOTFILE;
	fontInfo.u32Size = osd->cfg->fontSize;

	/** create the object of text display */    
	res = HI_GO_CreateTextEx(&fontInfo, &osd->fontHandle);
	if (HI_SUCCESS != res )
	{
		MUX_PLAY_ERROR("failed to create the font: '%s, size:%d': 0x%x", fontInfo.pMbcFontFile, fontInfo.u32Size, res);
		return res;
	}
	
	/** set the default color of text as white, and background color is black*/	
	res = HI_GO_SetTextBGColor( osd->fontHandle, 0xFFFF0000);
	res |= HI_GO_SetTextColor(osd->fontHandle, osd->cfg->fontColor);

#endif
	
	osd->rect.x = osd->cfg->left;
	osd->rect.y = osd->cfg->top;
	osd->rect.w = osd->cfg->width;
	osd->rect.h = osd->cfg->height;
	
	winInfo.hLayer = osd->desktop; /* graphic layer this window belong to */

	winInfo.rect.x = osd->rect.x;
	winInfo.rect.y = osd->rect.y;
	winInfo.rect.w = osd->rect.w;
	winInfo.rect.h = osd->rect.h;

	/* this rect is used to output text, relative correlation in OSD object */
#if 0
	/* Why? 06.05, 2019 */
	osd->rect.x = 50;
	osd->rect.y = 10;
	osd->rect.w = 1400;
	osd->rect.h = 180;
#endif

	winInfo.LayerNum = 0;
	winInfo.PixelFormat = HIGO_PF_8888;
	if((cfg->type == RECT_TYPE_MAIN) || (cfg->type == RECT_TYPE_PIP) )
	{
#if OSD_ENABLE_DOUBLE_BUFFER
		if(higo->muxPlayer->playerConfig.enableLowDelay )
		{
			winInfo.BufferType = HIGO_BUFFER_DOUBLE;
		}	
		else
		{
			winInfo.BufferType = HIGO_BUFFER_SINGLE;
		}
#else		
		winInfo.BufferType = HIGO_BUFFER_SINGLE;
#endif
	}
	else
	{
		winInfo.BufferType = HIGO_BUFFER_SINGLE;
	}

	/** create HIGO window*/
	res = HI_GO_CreateWindowEx(&winInfo, &osd->winHandle );
	if (HI_SUCCESS != res )
	{
		MUX_PLAY_WARN("Create %s windows failed : 0x%x", osd->cfg->name, res);
		return res;
	}

	osd->winSurface = HIGO_INVALID_HANDLE;

	/** get surface of the window */
	res = HI_GO_GetWindowSurface(osd->winHandle, &osd->winSurface);
	if (HI_SUCCESS != res )
	{
		CMN_ABORT("Get layer of subtitle window failed" );
		return res;
	}
//	MUX_PLAY_DEBUG("Layer hander of %s: 0x%x", osd->name, osd->winSurface);

	if((cfg->type == RECT_TYPE_MAIN) || (cfg->type == RECT_TYPE_PIP) )
	{
		if(cfg->type == RECT_TYPE_MAIN)
			osd->type = MUX_OSD_TYPE_MAIN_WIN;
		else
			osd->type = MUX_OSD_TYPE_PIP;
		
		res =  HI_GO_SetWindowOpacity( osd->winHandle, 0); 
	}
	else
	{
		osd->enable = osd->cfg->enable;
		res =  HI_GO_SetWindowOpacity( osd->winHandle, (osd->enable==HI_TRUE)?osd->cfg->alpha: 0); 
		if (HI_SUCCESS != res )
		{
			return res;
		}
	}

#if 0
	HI_RECT rect;
	HI_GO_GetWindowRect( osd->winHandle, &rect);
	MUX_PLAY_DEBUG("rect of window %s : [(%d, %d), (%d, %d)]", win->name, rect.x, rect.y, rect.w, rect.h);
#endif
	if(osd->cfg->type == RECT_TYPE_ALERT)
	{
		osd->type = MUX_OSD_TYPE_ALERT;
		higo->alert = osd;
	}
	else if(osd->cfg->type == RECT_TYPE_LOGO)
	{
		osd->type = MUX_OSD_TYPE_LOGO;
		higo->logo = osd;
	}
	else if(osd->cfg->type == RECT_TYPE_SUBTITLE)
	{
		osd->type = MUX_OSD_TYPE_SUBTITLE;
		higo->subtitle = osd;
	}
#if 0	
	else
	{
		higo->subtitle = osd;
	}
#endif

	cmn_list_append( &higo->osdList, osd);

	return res;
}

static int _muxRxDestroyOSD(int index, void *element, void *pHigo)
{
	MUX_OSD	*osd = (MUX_OSD *)element;

//	MUXLAB_HIGO_T *higo = (MUXLAB_HIGO_T *)pHigo;
	if (HIGO_INVALID_HANDLE != osd->winHandle )
	{
		(HI_VOID)HI_GO_DestroyWindow( osd->winHandle);
		osd->winHandle = HIGO_INVALID_HANDLE;
	}
	
	if (HIGO_INVALID_HANDLE != osd->fontHandle)
	{
		(HI_VOID)HI_GO_DestroyText( osd->fontHandle);
		osd->fontHandle = HIGO_INVALID_HANDLE;
	}
	return EXIT_SUCCESS;
}

int	muxOsdCreateFont(MUX_OSD	*osd)
{
	int res;
	HIGO_TEXT_INFO_S fontInfo;

	fontInfo.pMbcFontFile = FONT_GB2312_TTFFILE;
	fontInfo.pSbcFontFile = NULL;
	fontInfo.u32Size = osd->cfg->fontSize;

	/** create the object of text display */    
	res = HI_GO_CreateTextEx(&fontInfo, &osd->fontHandle);
	if (HI_SUCCESS != res )
	{
		MUX_PLAY_ERROR("failed to create the font: '%s'", fontInfo.pMbcFontFile);
		return res;
	}
	
	/** set the default color of text as white, and background color is black*/	
	res = HI_GO_SetTextBGColor( osd->fontHandle, 0xFFFF0000);
	res |= HI_GO_SetTextColor(osd->fontHandle, osd->cfg->fontColor);

	return res;
}

MUX_OSD *muxOsdFind(MUXLAB_HIGO_T *higo, MUX_OSD_TYPE type)
{
	MUX_OSD *osd;
	int i;

	for(i=0; i< cmn_list_size(&higo->osdList); i++)
	{
		osd = cmn_list_get( &higo->osdList, i);
		if(osd->type == type)
		{
			return osd;
		}
	}

	return NULL;
}

HI_S32 muxRxHigoInit(MUX_RX_T *muxRx)
{
	HI_S32 res = 0;
	HIGO_LAYER_INFO_S stLayerInfo = {0};
	MuxPlayer *muxPlayer = (MuxPlayer *)muxRx->muxPlayer;
	MUXLAB_HIGO_T	*higo = &muxRx->higo;

	memset(higo, 0, sizeof(MUXLAB_HIGO_T));
	higo->muxPlayer = muxPlayer;

	res = HI_GO_Init();
	if (HI_SUCCESS != res )
	{
		MUX_PLAY_ERROR("failed to init higo! ret = 0x%x !", res);
		return HI_FAILURE;
	}
#if 0
	(HI_VOID)HI_GO_GetLayerDefaultParam(HIGO_LAYER_HD_0, &stLayerInfo);
	stLayerInfo.LayerFlushType = HIGO_LAYER_FLUSH_DOUBBUFER;
	stLayerInfo.PixelFormat = HIGO_PF_8888;
#else
	stLayerInfo.CanvasWidth = MUX_CANVAS_WIDTH;
	stLayerInfo.CanvasHeight = MUX_CANVAS_HEIGHT;
	stLayerInfo.DisplayWidth = MUX_DISPLAY_BUFFER_WIDTH;
	stLayerInfo.DisplayHeight = MUX_DISPLAY_BUFFER_HEIGHT;
	stLayerInfo.ScreenWidth = MUX_VIRTUAL_SCREEN_WIDTH;
	stLayerInfo.ScreenHeight = MUX_VIRTUAL_SCREEN_HEIGHT;
	
	stLayerInfo.PixelFormat = HIGO_PF_8888;
	stLayerInfo.LayerFlushType = HIGO_LAYER_FLUSH_NORMAL;
	stLayerInfo.AntiLevel = HIGO_LAYER_DEFLICKER_AUTO;
	stLayerInfo.LayerID = HIGO_LAYER_HD_0;
#endif
	res = HI_GO_CreateLayer(&stLayerInfo, &higo->layerHandler);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR("failed to create the layer sd 0, ret = 0x%x ", res);
		goto RET;
	}

#if 0	
	res = HI_GO_SetLayerBGColor(higo->layerHandler,0xffffffff);
	if (HI_SUCCESS != res)
	{
		exit(1);
	}
#endif

#if WITH_HIGO_SURFACE_HANDLER	
	res =  HI_GO_GetLayerSurface(higo->layerHandler, &higo->layerSurfaceHandle);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_ERROR(" failed to get layer surface! s32Ret = 0x%x", res);
		goto RET;
	}

	(HI_VOID)HI_GO_FillRect(higo->layerSurfaceHandle, NULL, 0x00000000, HIGO_COMPOPT_NONE);
#endif

//	res |= HI_GO_SetTextColor(higo->fontHandle, 0xFF000000);

	RECT_CONFIG *cfg = muxGetRectConfig(muxPlayer, RECT_TYPE_SUBTITLE);
	if(cfg != NULL)
	{
		higo->rectSubtitle.x = cfg->left;
		higo->rectSubtitle.y = cfg->top;
		higo->rectSubtitle.w = cfg->width;
		higo->rectSubtitle.h = cfg->height;
	}
	else
	{
	 	higo->rectSubtitle.x = 260;
		higo->rectSubtitle.y = 250;
		higo->rectSubtitle.w = 760;
		higo->rectSubtitle.h = 200;
	}
//	MUX_PLAY_DEBUG( "subtitle rect : [(%d, %d), (%d, %d)]", higo->rectSubtitle.x, higo->rectSubtitle.y, higo->rectSubtitle.w, higo->rectSubtitle.h);

	cmn_list_init(&higo->osdList);
	res |= cmn_list_iterate(&muxPlayer->playerConfig.osds, HI_TRUE, _muxCreateOSD, higo);

	MUX_OSD *logoOsd = muxOsdFind(higo, MUX_OSD_TYPE_LOGO);
	muxOsdImageShow(logoOsd, logoOsd->cfg->url);

	pthread_mutex_init(&higo->winMutex, NULL);



#if 0

	rect.x = 10;
	rect.y = 10;
	rect.w = 200;
	rect.h = 100;


		s32Ret = HI_GO_FillRect(higo->subtitleWinSurface, NULL,0xFF00FFFF,HIGO_COMPOPT_NONE);
		if (HI_SUCCESS != s32Ret)
		{
			MUX_PLAY_WARN("TextOutEx on window failed: 0x%x (0x%x)", s32Ret, higo->subtitleWinSurface);
		}
		s32Ret = HI_GO_TextOutEx(higo->fontHandle, higo->subtitleWinSurface, "Testing...", &rect, HIGO_LAYOUT_WRAP | HIGO_LAYOUT_HCENTER | HIGO_LAYOUT_BOTTOM);

#if 0		
		s32Ret = HI_GO_ChangeWindowZOrder(higo->subtitleWinHandle, HIGO_ZORDER_MOVETOP);
		if (HI_SUCCESS != s32Ret)
		{
			MUX_PLAY_WARN("Update window failed");
		}
#endif	
		s32Ret = HI_GO_UpdateWindow(higo->subtitleWinHandle, NULL);
		if (HI_SUCCESS != s32Ret)
		{
			MUX_PLAY_WARN("Update window failed");
		}

		if (HIGO_INVALID_HANDLE != higo->layerHandler)
			(HI_VOID)HI_GO_RefreshLayer(higo->layerHandler, NULL);

sleep(10000);
#endif

	return HI_SUCCESS;

RET:
	exit(1);
	return HI_FAILURE;
}

HI_VOID muxRxHigoDeinit(MUX_RX_T *muxRx)
{
	HI_S32 res = 0;

#if 0
	if (HIGO_INVALID_HANDLE != muxRx->higo.memSurfaceHandle)
	{
		(HI_VOID)HI_GO_FreeSurface(muxRx->higo.memSurfaceHandle);
		muxRx->higo.memSurfaceHandle = HIGO_INVALID_HANDLE;
	}
#endif

	pthread_mutex_destroy(&muxRx->higo.winMutex);


	res |= cmn_list_iterate(&muxRx->higo.osdList, HI_TRUE, _muxRxDestroyOSD, &muxRx->higo);

	cmn_list_ofchar_free(&muxRx->higo.osdList, FALSE);


	if (HIGO_INVALID_HANDLE != muxRx->higo.layerHandler)
	{
		(HI_VOID)HI_GO_DestroyLayer(muxRx->higo.layerHandler);
		muxRx->higo.layerHandler = HIGO_INVALID_HANDLE;
	}


#if WITH_HIGO_SURFACE_HANDLER	
	muxRx->higo.layerSurfaceHandle = HIGO_INVALID_HANDLE;
#endif

	(HI_VOID)HI_GO_Deinit();
}


