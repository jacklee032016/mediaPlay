/*
* example the usage of memory surface 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hi_go.h"
#include "higo_displayInit.h"

HI_S32 main(HI_S32 argc, HI_CHAR* argv[])
{
	HI_S32 ret;
	HIGO_LAYER_INFO_S stLayerInfo;
	HI_HANDLE hLayer,hLayerSurface,hMemSurface,hMemSurface2;
	HIGO_BLTOPT_S stBlitOpt;
	HI_RECT stRect;
	HIGO_LAYER_E eLayerID = HIGO_LAYER_HD_0;
	
	HI_RECT rcTransparent = {50, 50, 200, 200};
	HI_COLOR key = 0xffff0000;
	HI_RECT src = {0, 0, 1920, 1080};
	HI_RECT dst = {0, 0, 1920, 1080};

	/** initial */
	ret = Display_Init();
	if (HI_SUCCESS != ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		return ret;
	}

	ret = HI_GO_Init();
	if (HI_SUCCESS != ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR1;
	}

	/** create graphic layer */
	ret = HI_GO_GetLayerDefaultParam(eLayerID, &stLayerInfo);
	if (HI_SUCCESS != ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR2;
	}

	stLayerInfo.CanvasWidth = 1920;
	stLayerInfo.CanvasHeight = 1080;
//	stLayerInfo.DisplayWidth = 1920;
//	stLayerInfo.DisplayHeight = 1080;
	ret = HI_GO_CreateLayer(&stLayerInfo,&hLayer);
	if (HI_SUCCESS != ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR2;
	}

	/** get the graphic layer Surface */
	ret = HI_GO_GetLayerSurface (hLayer,&hLayerSurface); 
	if (HI_SUCCESS != ret)  
	{
		goto ERR3;
	}

#if 0
	ret = HI_GO_FillRect(hLayerSurface, NULL, 0xffffffff, HIGO_COMPOPT_NONE);
	if (HI_SUCCESS != ret)  
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR3;
	}
#endif

	/** create memory Surface */
	ret = HI_GO_CreateSurface(640, 480, HIGO_PF_8888, &hMemSurface);
	if (HI_SUCCESS != ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR3;
	}


	/** draw a rectangle on the memory surface */
	ret = HI_GO_FillRect(hMemSurface, NULL, 0xFFFF0000,HIGO_COMPOPT_NONE);
	if (HI_SUCCESS != ret)
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4; 
	}

#if 0	
	ret = HI_GO_FillRect(hMemSurface, &rcTransparent, key, HIGO_COMPOPT_NONE);
	if (HI_SUCCESS != ret)  
	{
		Printf("failed in line %d\n", __LINE__ );
		goto ERR4;
	}
#endif

	int alpha = 1;
	while(1)
	{
		if(alpha == 256)
			break;
		
		ret = HI_GO_SetSurfaceAlpha(hMemSurface, alpha);
		if (HI_SUCCESS != ret)
		{
			Printf("failed in line %d\n", __LINE__ );
			goto ERR4; 
		}

#if 0	
		ret = HI_GO_SetSurfaceColorKey(hMemSurface, key);
		if (HI_SUCCESS != ret)      
		{
			Printf("failed in line %d\n", __LINE__ );
			goto ERR4;
		}
#endif

		HI_GO_SyncSurface(hMemSurface, HIGO_SYNC_MODE_CPU);

#if 0
		/** move it to graphic layer Surface */
		stRect.x = stRect.y = 50;
		stRect.w = 320;
		stRect.h = 240;
		memset(&stBlitOpt,0,sizeof(HIGO_BLTOPT_S));
		//stBlitOpt.EnablePixelAlpha = HI_TRUE;
		stBlitOpt.EnableGlobalAlpha = HI_TRUE;
		/** should set the pixel alpha mix mode when enable global alpha*/	
		stBlitOpt.PixelAlphaComp = HIGO_COMPOPT_SRCOVER;
		ret = HI_GO_Blit(hMemSurface,NULL,hLayerSurface,&stRect,&stBlitOpt);
		if (HI_SUCCESS != ret)
		{
			goto ERR4;   
		}
#else
		HIGO_BLTOPT_S opt = {0};
	//	opt.ColorKeyFrom = HIGO_CKEY_SRC;
		opt.EnableGlobalAlpha = HI_TRUE;
		opt.EnablePixelAlpha = HI_TRUE;
//		opt.PixelAlphaComp = HIGO_COMPOPT_SRCOVER;
		opt.PixelAlphaComp = HIGO_COMPOPT_ADD;

	//	ret = HI_GO_Blit(hMemSurface, &src, hLayerSurface, &dst, &opt);
		ret = HI_GO_Blit(hMemSurface, NULL, hLayerSurface, NULL, &opt);
		if (HI_SUCCESS != ret)
		{
			Printf("failed in line %d:0x%x\n", __LINE__, ret );
			goto ERR4;
		}
		HI_GO_SyncSurface(hMemSurface, HIGO_SYNC_MODE_CPU);
#endif

#if 0
		/** create memory Surface */
		ret = HI_GO_CreateSurface(320,240,HIGO_PF_1555,&hMemSurface2);
		if (HI_SUCCESS != ret)
		{
			goto ERR3;
		}
		ret = HI_GO_SetSurfaceAlpha(hMemSurface2, 128/2);
		if (HI_SUCCESS != ret)
		{
			goto ERR4; 
		}

		/** draw a rectangle on the memory surface */
		ret = HI_GO_FillRect(hMemSurface2,NULL,0xFF00FF00, HIGO_COMPOPT_SRCOVER);
		if (HI_SUCCESS != ret)
		{
			goto ERR4; 
		}

		/** move it to graphic layer Surface */
		stRect.x = stRect.w+20;
		stRect.y = stRect.h + 50;
		stRect.w = 320;
		stRect.h = 240;
		memset(&stBlitOpt,0,sizeof(HIGO_BLTOPT_S));
		//stBlitOpt.EnablePixelAlpha = HI_TRUE;
		stBlitOpt.EnableGlobalAlpha = HI_TRUE;
		/** should set the pixel alpha mix mode when enable global alpha*/	
		stBlitOpt.PixelAlphaComp = HIGO_COMPOPT_SRCOVER;
		ret = HI_GO_Blit(hMemSurface2,NULL, hLayerSurface, &stRect,&stBlitOpt);
		if (HI_SUCCESS != ret)
		{
			goto ERR4;   
		}
#endif


		/**fresh the graphic layer */ 
		ret = HI_GO_RefreshLayer(hLayer, NULL);
		if (HI_SUCCESS != ret)
		{
			goto ERR4; 
		}
		alpha++;
		
		Printf("Current Alpha is %d, please input anykey add ALPHA value\n", alpha);
		getchar();
	}

ERR4:
	HI_GO_FreeSurface(hMemSurface);
ERR3:
	HI_GO_DestroyLayer(hLayer);
ERR2:
	HI_GO_Deinit();
ERR1:
	Display_DeInit();

	return ret;
}

