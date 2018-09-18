#include <stdio.h>
#include <stdlib.h>
#include "hi_go.h"
#include "higo_displayInit.h"

HI_S32 main(HI_S32 argc, HI_CHAR* argv[])
{
#define	_WIDTH		600
#define	_HEIGHT	130

	HI_S32 s32Ret = 0;
	HI_HANDLE hFont = HIGO_INVALID_HANDLE;
	HIGO_LAYER_INFO_S stLayerInfo = {0};
	HI_HANDLE hLayer = HIGO_INVALID_HANDLE;
	HI_HANDLE hLayerSurface;
	HI_RECT rc = {0};
	//HI_CHAR szText[] = {0x77,0xe2,0x91,0xcf,0x5b,0x4f,0x53,0x66,0x34,0x79, 0x3a, 0x0}; // vector character display
	HI_CHAR szText[] = "矢量AabC中英文混合";
#if 1
	HIGO_LAYER_E eLayerID = HIGO_LAYER_HD_0;
#else
	HIGO_LAYER_E eLayerID = HIGO_LAYER_SD_0;
#endif
 	HI_S32 s32layerWidth = 0, s32layerHeight = 0;
 	HI_S32 s32textWidth = 0, s32textHeight = 0;

	s32Ret = Display_Init();
	if (HI_SUCCESS != s32Ret)
	{
		return s32Ret;
	}

	s32Ret = HI_GO_Init();
	if (HI_SUCCESS != s32Ret)
	{
		goto ERR1;
	}

#if  0
	s32Ret = HI_GO_GetLayerDefaultParam(eLayerID, &stLayerInfo); 
	if (HI_SUCCESS != s32Ret)
	{
		goto ERR2;
	}

	Printf("Default params:Canvas:%dx%d;Display:%dx%d;Screen:%dx%d; layerID:%d\n", 
		stLayerInfo.CanvasWidth, stLayerInfo.CanvasHeight, stLayerInfo.DisplayWidth, stLayerInfo.DisplayHeight, stLayerInfo.ScreenWidth, stLayerInfo.ScreenHeight, stLayerInfo.LayerID);

	stLayerInfo.PixelFormat = HIGO_PF_8888;
	stLayerInfo.LayerFlushType = HIGO_LAYER_FLUSH_NORMAL;
#else
	stLayerInfo.CanvasWidth = 1920;
	stLayerInfo.CanvasHeight = 1080;
	stLayerInfo.DisplayWidth = 1920;
	stLayerInfo.DisplayHeight = 1080;
	stLayerInfo.ScreenWidth = 1920;
	stLayerInfo.ScreenHeight = 1080;
	stLayerInfo.PixelFormat = HIGO_PF_8888;
	stLayerInfo.LayerFlushType = HIGO_LAYER_FLUSH_NORMAL;
	stLayerInfo.AntiLevel = HIGO_LAYER_DEFLICKER_AUTO;
	stLayerInfo.LayerID = eLayerID;
#endif
	s32Ret = HI_GO_CreateLayer(&stLayerInfo, &hLayer);
	if (HI_SUCCESS != s32Ret)
	{
		goto ERR2;
	}

	s32Ret =  HI_GO_GetLayerSurface(hLayer, &hLayerSurface);
	if  (HI_SUCCESS != s32Ret)
	{
		goto ERR3;       
	}

	HI_GO_FillRect(hLayerSurface, NULL, 0xFFFFFFFF, HIGO_COMPOPT_NONE);
	if  (HI_SUCCESS != s32Ret)
	{
		goto ERR3;       
	}

#if 0
	s32Ret = HI_GO_CreateText(FONT_GB2312_DOTFILE, NULL,  &hFont);
	if (HI_SUCCESS != s32Ret) 
	{
		goto ERR3;       
	}
	
	s32Ret = HI_GO_SetTextColor(hFont, 0xffff0000);
	if (HI_SUCCESS != s32Ret) 
	{
		goto ERR4;       
	}
#else
	HIGO_TEXT_INFO_S info;
	info.pMbcFontFile = FONT_GB2312_TTFFILE;
	info.pSbcFontFile = NULL;
	info.u32Size = 48;

	/** create the object of text display */    
	s32Ret = HI_GO_CreateTextEx(&info, &hFont);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed to create the font:line%d,%s\n", __LINE__,info.pMbcFontFile);
		HI_GO_DestroyLayer(hLayer);
		return HI_FAILURE; 
	}

#endif
	/** set the default color of text as white, and background color is black*/	
	s32Ret = HI_GO_SetTextBGColor(hFont,0xFFFF0000);
	s32Ret |= HI_GO_SetTextColor(hFont,0xFF00FF00);

	s32Ret = HI_GO_GetTextExtent(hFont, szText, &s32textWidth, &s32textHeight);
	if (HI_SUCCESS != s32Ret) 
	{
		//if(HIGO_ERR_UNSUPPORT_CHARSET == s32Ret)
		goto ERR4;       
	}
	Printf( "size of extend '%s' : WxH=%dx%d\n", szText, s32textWidth, s32textHeight );
	
 	(HI_VOID)HI_GO_GetSurfaceSize(hLayerSurface, &s32layerWidth, &s32layerHeight);
	Printf( "WxH=%dx%d, ", s32layerWidth, s32layerHeight );

#if 0
	s32layerWidth = 1920;
	s32layerHeight = 1080;
#endif

	/* 1: most left and up */
	rc.x = MARGIN_HORIZON;
	rc.y = MARGIN_VERTIC;
	rc.w = s32textWidth + MARGIN_HORIZON;
	rc.h = s32textHeight + MARGIN_VERTIC;
	s32Ret = HI_GO_DrawRect(hLayerSurface, &rc, 0xffff0000);
	if (HI_SUCCESS != s32Ret) 
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4;       
	}
	s32Ret = HI_GO_FillRect(hLayerSurface,&rc,0xFF000000,HIGO_COMPOPT_NONE);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR3;
	}

	s32Ret = HI_GO_TextOutEx(hFont, hLayerSurface, szText, &rc, HIGO_LAYOUT_LEFT | HIGO_LAYOUT_TOP);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	s32Ret = HI_GO_SetTextColor(hFont, 0xff00ff00);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	/* 2 : most right and up */
	rc.y = MARGIN_VERTIC;
	rc.w = s32textWidth + MARGIN_HORIZON;
	rc.h = s32textHeight + MARGIN_VERTIC;
	rc.x = s32layerWidth -rc.w - MARGIN_HORIZON;
	s32Ret = HI_GO_SetTextStyle(hFont, HIGO_TEXT_STYLE_ITALIC);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	s32Ret = HI_GO_DrawRect(hLayerSurface, &rc, 0xff7f7f7f); 
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	s32Ret = HI_GO_TextOutEx(hFont, hLayerSurface, szText, &rc, HIGO_LAYOUT_RIGHT);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	s32Ret = HI_GO_SetTextColor(hFont, 0xff0000ff);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	/* 3 : most left and bottom */
	rc.x = MARGIN_HORIZON;
	rc.w = s32textWidth + MARGIN_HORIZON;
	rc.h = s32textHeight + MARGIN_VERTIC;
	rc.y = s32layerHeight - rc.h - MARGIN_VERTIC;
	s32Ret = HI_GO_DrawRect(hLayerSurface, &rc, 0xff00ff00);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	s32Ret = HI_GO_TextOutEx(hFont, hLayerSurface, szText, &rc, HIGO_LAYOUT_LEFT|HIGO_LAYOUT_BOTTOM);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	/* 4 : most right and bottom */
	rc.w = s32textWidth + MARGIN_HORIZON;
	rc.h = s32textHeight + MARGIN_VERTIC;
	rc.x = s32layerWidth - rc.w - MARGIN_HORIZON;
	rc.y = s32layerHeight - rc.h - MARGIN_VERTIC;
	s32Ret = HI_GO_DrawRect(hLayerSurface, &rc, 0xff0000ff);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	s32Ret = HI_GO_TextOutEx(hFont, hLayerSurface, szText, &rc, HIGO_LAYOUT_RIGHT|HIGO_LAYOUT_BOTTOM);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	s32Ret = HI_GO_RefreshLayer(hLayer, NULL);
	if (HI_SUCCESS != s32Ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	getchar();

ERR4:
	HI_GO_DestroyText(hFont);
ERR3:
	HI_GO_DestroyLayer(hLayer);
ERR2:
	HI_GO_Deinit();
ERR1:
	Display_DeInit();
	return s32Ret;
}

