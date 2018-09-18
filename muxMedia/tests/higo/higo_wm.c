/*
* exmaple of GIF decoding and display
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "hi_go.h"
#include "higo_displayInit.h"

HI_S32 main(HI_S32 argc, HI_CHAR* argv[])
{
	HI_S32 ret = HI_SUCCESS;
	HI_HANDLE hLayer;
	HIGO_WNDINFO_S WinInfo;
	HI_HANDLE hWindow;
	HI_HANDLE hWindow2;
	HI_S32 sWidth = 500, sHeight=400;
	HI_HANDLE hWinSurface;
	HI_HANDLE hWinSurface2;
	HIGO_LAYER_INFO_S LayerInfo;
	HIGO_LAYER_E eLayerID = HIGO_LAYER_HD_0;

	/** initial*/
	ret = Display_Init();
	if (HI_SUCCESS != ret)
	{
		return ret;
	}

	ret = HI_GO_Init();
	if (HI_SUCCESS != ret)
	{
		goto ERR1;
	}

	/** get the HD layer's default parameters*/
	ret = HI_GO_GetLayerDefaultParam(eLayerID, &LayerInfo);
	if (HI_SUCCESS != ret)
	{
		goto ERR2;
	}

	/** create graphic layer */
	ret = HI_GO_CreateLayer(&LayerInfo, &hLayer);
	if (HI_SUCCESS != ret)
	{
		goto ERR2;
	}

	/** set the background color of graphic layer*/    
	ret = HI_GO_SetLayerBGColor(hLayer,0xffffffff);
	if (HI_SUCCESS != ret)
	{
		goto ERR3;
	}


	HIGO_TEXT_INFO_S info;
	info.pMbcFontFile = FONT_GB2312_TTFFILE;
	info.pSbcFontFile = NULL;
	info.u32Size = 48;
	HI_HANDLE hFont = HIGO_INVALID_HANDLE;

	/** create the object of text display */    
	ret = HI_GO_CreateTextEx(&info, &hFont);
	if (HI_SUCCESS != ret)
	{
		Printf("failed to create the font:line%d,%s\n", __LINE__,info.pMbcFontFile);
		HI_GO_DestroyLayer(hLayer);
		return HI_FAILURE; 
	}
	ret |= HI_GO_SetTextColor(hFont,0xFF00FF00);


	WinInfo.hLayer = hLayer;
	WinInfo.rect.x = 50;
	WinInfo.rect.y = 50;
	WinInfo.rect.w = sWidth;
	WinInfo.rect.h = sHeight;
	WinInfo.LayerNum = 0;
	WinInfo.PixelFormat = HIGO_PF_8888;
	WinInfo.BufferType = HIGO_BUFFER_SINGLE;

	/** create window*/
	ret = HI_GO_CreateWindowEx(&WinInfo, &hWindow);
	if (HI_SUCCESS != ret)
	{
		goto ERR3;
	}

	/** get surface of the window */
	ret = HI_GO_GetWindowSurface(hWindow, &hWinSurface);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

	Printf("windows surface: 0x%x\n", hWinSurface);
	/** fill window*/
	ret = HI_GO_FillRect(hWinSurface,NULL,0xFFFF0000,HIGO_COMPOPT_NONE);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

	HI_RECT rc;
	rc.x = 50;
	rc.y = 50;
	rc.w = 400;
	rc.h = 200;
	ret = HI_GO_TextOutEx(hFont, hWinSurface, "Text String", &rc, HIGO_LAYOUT_LEFT | HIGO_LAYOUT_TOP);
	if (HI_SUCCESS != ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	/** set the opacity of the window */ 
	ret =  HI_GO_SetWindowOpacity(hWindow,128); 
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

	/** should fresh the window if any changed to the window*/	
	ret = HI_GO_UpdateWindow(hWindow,NULL);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

/* 2 window */
	WinInfo.hLayer = hLayer;
	WinInfo.rect.x = sWidth- 50;
	WinInfo.rect.y = sHeight - 50;
	WinInfo.rect.w = sWidth;
	WinInfo.rect.h = sHeight;
	WinInfo.LayerNum = 0;
	WinInfo.PixelFormat = HIGO_PF_8888;
	WinInfo.BufferType = HIGO_BUFFER_SINGLE;

	/** create window*/
	ret = HI_GO_CreateWindowEx(&WinInfo, &hWindow2);
	if (HI_SUCCESS != ret)
	{
		goto ERR3;
	}


	/** get surface of the window */
	ret = HI_GO_GetWindowSurface(hWindow2, &hWinSurface2);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

	/** fill window*/
	ret = HI_GO_FillRect(hWinSurface2,NULL,0xFF0000FF,HIGO_COMPOPT_NONE);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

	/** set the opacity of the window */ 
	ret =  HI_GO_SetWindowOpacity(hWindow2, 64); 
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

	/** should fresh the window if any changed to the window*/	
	ret = HI_GO_UpdateWindow(hWindow2,NULL);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

	//fresh the window to the graphic layer
	ret = HI_GO_RefreshLayer(hLayer, NULL);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}  

	Printf("press any key to change pos ans zorder\n");
	getchar();

	ret = HI_GO_TextOutEx(hFont, hWinSurface, "Another Text String", &rc, HIGO_LAYOUT_LEFT | HIGO_LAYOUT_TOP);
	if (HI_SUCCESS != ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

	/** change the position of the window*/
	ret = HI_GO_SetWindowPos(hWindow,300,200);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

	ret = HI_GO_ChangeWindowZOrder(hWindow, HIGO_ZORDER_MOVETOP);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}
	ret =  HI_GO_SetWindowOpacity(hWindow,255); 
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

	/** fresh the window */
	ret = HI_GO_UpdateWindow(hWindow, NULL);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

	//fresh the window to the graphic layer
	ret = HI_GO_RefreshLayer(hLayer, NULL);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}  
	Printf("press any key to change pos ans zorder\n");
	getchar();
	ret =  HI_GO_SetWindowOpacity(hWindow,0); 
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

	/** fresh the window */
	ret = HI_GO_UpdateWindow(hWindow, NULL);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}

	//fresh the window to the graphic layer
	ret = HI_GO_RefreshLayer(hLayer, NULL);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}  

	printf("press any key to exit\n");
	getchar();


	//fresh the window to the graphic layer
	ret = HI_GO_RefreshLayer(hLayer, NULL);
	if (HI_SUCCESS != ret)
	{
		goto ERR4;
	}  



ERR4:
	HI_GO_DestroyWindow(hWindow);
ERR3:
	HI_GO_DestroyLayer(hLayer);
ERR2:
	HI_GO_Deinit();
ERR1:
	Display_DeInit();
	return ret;    
}

