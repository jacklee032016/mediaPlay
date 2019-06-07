
#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"


HI_S32 muxSubtitleClear(MUX_PLAY_T *play, HI_UNF_SO_CLEAR_PARAM_S *param)
{
#if PLAYER_ENABLE_SUBTITLE
	int res = 0;

	MUX_PLAY_DEBUG( "CLEAR Subtitle!");
	res = OSD_DESKTOP_TRY_LOCK(&play->muxRx->higo);
	if(res != 0)
	{
		MUX_PLAY_WARN( "Window is locked for subtitle info, waiting next...");
		return HI_SUCCESS;
	}

	MUX_PLAY_DEBUG( "CLEAR Subtitle : %s", play->muxRx->higo.subtitle->name );
	muxOsdClear(play->muxRx->higo.subtitle);

	res = OSD_DESKTOP_UNLOCK(&play->muxRx->higo);
	if(res != 0)
	{
		MUX_PLAY_WARN( "Higo is unlocked for subtitle clear %s: %s", play->muxFsm.name, strerror(errno) );
		return HI_SUCCESS;
	}
#endif

	return HI_SUCCESS;
}

int	_muxOutTextSubtitle(MUX_PLAY_T *play, const HI_UNF_SO_SUBTITLE_INFO_S *pstInfo)
{
	int res =0;
	if (NULL == pstInfo->unSubtitleParam.stText.pu8Data)
	{
		return HI_FAILURE;
	}
	
	MUX_PLAY_DEBUG( "OUTPUT: %s \n", pstInfo->unSubtitleParam.stText.pu8Data);
	res = OSD_DESKTOP_TRY_LOCK(&play->muxRx->higo);
	if(res != 0)
	{
		MUX_PLAY_WARN("Window is locked for subtitle info, waiting next...");
		return HI_SUCCESS;
	}

//	HI_RECT rect = {50, 200, 620, 200};
//	(HI_VOID)_outputText(&play->muxRx->higo, (const HI_CHAR*)pstInfo->unSubtitleParam.stText.pu8Data, &rect );

	res = muxOsdOutputText(play->muxRx->higo.subtitle, ALERT_DEFAULT_LAYOUT, (const HI_CHAR*)pstInfo->unSubtitleParam.stText.pu8Data);

	res = OSD_DESKTOP_UNLOCK( &play->muxRx->higo);
	if(res != 0)
	{
		MUX_PLAY_WARN( "Higo is unlocked for subtitle text %s: %s", play->muxFsm.name, strerror(errno) );
		return HI_SUCCESS;
	}

	return HI_SUCCESS;
}

int	_muxOutAssSubtitle(MUX_PLAY_T *play, const HI_UNF_SO_SUBTITLE_INFO_S *pstInfo)
{
#if 0 /* commented, Dec.22nd, 2017 */
//	HI_RECT rc = {100, 730, 1020, 250};//{260, 550, 760, 100};
//	HI_RECT rc = {260, 550, 760, 200};
	HI_SO_SUBTITLE_ASS_DIALOG_S *pstDialog = NULL;
	HI_CHAR *pszOutHand = NULL;
	HI_S32 s32Ret = HI_FAILURE;
	HI_S32 s32AssLen = 0;
	
	HI_GO_SetLayerPos( play->muxRx->higo.layerHandler, 0, 0);
	if ( NULL == play->soParseHand )
	{
		s32Ret = HI_SO_SUBTITLE_ASS_InitParseHand(&play->soParseHand);

		if ( s32Ret != HI_SUCCESS )
		{
			return HI_FAILURE;
		}
	}

	s32Ret = HI_SO_SUBTITLE_ASS_StartParse(pstInfo, &pstDialog, play->soParseHand);
	if (s32Ret == HI_SUCCESS)
	{
		s32Ret = HI_SO_SUBTITLE_ASS_GetDialogText(pstDialog, &pszOutHand, &s32AssLen);

		if ( s32Ret != HI_SUCCESS || NULL == pszOutHand )
		{
			MUX_PLAY_WARN("##########can't parse this Dialog!#########\n");
		}
		else
		{
			s32Ret = muxOsdOutputText(play->muxRx->higo.subtitle, ALERT_DEFAULT_LAYOUT, pszOutHand);//, &rc);
			MUX_PLAY_DEBUG("OUTPUT: %s \n", pszOutHand);
		}
	}
	else
	{
		MUX_PLAY_WARN( "##########can't parse this ass file!#########\n");
	}

	free(pszOutHand);
	pszOutHand = NULL;
	
	HI_SO_SUBTITLE_ASS_FreeDialog(&pstDialog);
#endif

	return HI_SUCCESS;
}

int _muxOutBitmapSubtitle(MUX_PLAY_T *play, const HI_UNF_SO_SUBTITLE_INFO_S *pstInfo)
{
#if 0 /* commented, Dec.22nd, 2017 */
	HI_PIXELDATA pData;

	HI_U32 u32DisplayWidth = 0, u32DisplayHeight = 0;

	HI_U32 i = 0, j = 0;
	HI_U32 u32PaletteIdx = 0;
	HI_U8 *pu8Surface = NULL;
	HI_S32 x = 100 ,y= 100;
	HI_S32 s32GfxX, s32GfxY;
	HI_S32 s32SurfaceW = 0, s32SurfaceH = 0;
	HI_S32 s32SurfaceX, s32SurfaceY, s32SurfaceYOff = 0;
	HI_S32 s32DrawH, s32DrawW;
	HI_BOOL bScale = 0;

//	MUX_PLAY_DEBUG( "sub title bitmap! \n");
			
	if (0 == pstInfo->unSubtitleParam.stGfx.u32CanvasWidth || 0 == pstInfo->unSubtitleParam.stGfx.u32CanvasHeight)
	{
		return HI_SUCCESS;
	}

	MUX_PLAY_DEBUG( "display w/h is [%d][%d]\n", pstInfo->unSubtitleParam.stGfx.u32CanvasWidth, pstInfo->unSubtitleParam.stGfx.u32CanvasHeight);
	
#if WITH_HIGO_SURFACE_HANDLER	
	HI_GO_LockSurface( play->muxRx->higo.layerSurfaceHandle, pData, HI_TRUE);
	HI_GO_GetSurfaceSize( play->muxRx->higo.layerSurfaceHandle, &s32SurfaceW, &s32SurfaceH);
#endif

	pu8Surface = (HI_U8*)pData[0].pData;

	if (NULL == pu8Surface || NULL == pstInfo->unSubtitleParam.stGfx.stPalette)
	{
#if WITH_HIGO_SURFACE_HANDLER	
		HI_GO_UnlockSurface(play->muxRx->higo.layerSurfaceHandle);
#endif
		return HI_SUCCESS;
	}

	if (s32SurfaceW * s32SurfaceH * pData[0].Bpp < pstInfo->unSubtitleParam.stGfx.h * pstInfo->unSubtitleParam.stGfx.w * 4)
	{
#if WITH_HIGO_SURFACE_HANDLER	
		HI_GO_UnlockSurface(play->muxRx->higo.layerSurfaceHandle);
#endif
		return HI_SUCCESS;
	}

	/*l00192899,if the len==0, should not get u32PaletteIdx from pu8PixData, it will make a segment fault*/
	if( (pstInfo->unSubtitleParam.stGfx.w <=0) || (pstInfo->unSubtitleParam.stGfx.h <=0) || (pstInfo->unSubtitleParam.stGfx.pu8PixData ==NULL) || (pstInfo->unSubtitleParam.stGfx.u32Len <=0))
	{
		return HI_SUCCESS;
	}

	if (HI_SUCCESS == HI_UNF_DISP_GetVirtualScreen(HI_UNF_DISPLAY1, &u32DisplayWidth, &u32DisplayHeight))
	{
		if (pstInfo->unSubtitleParam.stGfx.w <= u32DisplayWidth && pstInfo->unSubtitleParam.stGfx.h <= u32DisplayHeight)
		{
			x = (u32DisplayWidth - pstInfo->unSubtitleParam.stGfx.w)/2;
			y = u32DisplayHeight - pstInfo->unSubtitleParam.stGfx.h - 100;
		}
		else
		{
			/*scale it*/
			bScale = 1;
			x = 0;
			y = 0;
		}
	}

	s32SurfaceX = x;
	s32SurfaceY = y;
	if (bScale)
	{
		s32DrawW = s32SurfaceW;
		s32DrawH = pstInfo->unSubtitleParam.stGfx.h * s32SurfaceW/pstInfo->unSubtitleParam.stGfx.w;
		s32SurfaceY = s32SurfaceH - s32DrawH - 50;
		if (s32SurfaceY <= 0)
		{
			s32SurfaceY = 20;
		}
		
		s32SurfaceYOff  = 0;
		for (i = 0; i < s32DrawH; i++)
		{
			s32GfxY = s32SurfaceYOff * pstInfo->unSubtitleParam.stGfx.w/s32SurfaceW;
			for (j = 0; j < s32DrawW; j++)
			{
				s32GfxX = s32SurfaceX * pstInfo->unSubtitleParam.stGfx.w/s32SurfaceW;
				if (s32GfxY * pstInfo->unSubtitleParam.stGfx.w + s32GfxX >= pstInfo->unSubtitleParam.stGfx.u32Len)
				{
					MUX_PLAY_ERROR( "error\n");
					break;
				}
				u32PaletteIdx = pstInfo->unSubtitleParam.stGfx.pu8PixData[s32GfxY * pstInfo->unSubtitleParam.stGfx.w + s32GfxX];

				if (u32PaletteIdx >= HI_UNF_SO_PALETTE_ENTRY)
				{
					break;
				}


				pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 2] 	= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Red;
				pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 1] 	= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Green;
				pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 0] 	= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Blue;
				pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 3] 	= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Alpha;
				s32SurfaceX++;
			}
			
			s32SurfaceX = x;
			s32SurfaceY ++;
			s32SurfaceYOff ++;
		}
	}
	else
	{
		for (i = 0; i < pstInfo->unSubtitleParam.stGfx.h; i++)
		{
			for (j = 0; j < pstInfo->unSubtitleParam.stGfx.w; j++)
			{
				u32PaletteIdx = pstInfo->unSubtitleParam.stGfx.pu8PixData[i * pstInfo->unSubtitleParam.stGfx.w + j];

				if (u32PaletteIdx >= HI_UNF_SO_PALETTE_ENTRY)
				{
					break;
				}

				if (i * pstInfo->unSubtitleParam.stGfx.w + j > pstInfo->unSubtitleParam.stGfx.u32Len)
				{
					break;
				}

				pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 2] = pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Red;
				pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 1]	= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Green;
				pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 0] 	= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Blue;
				pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 3] 	= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Alpha;

				s32SurfaceX++;
			}
			
			s32SurfaceX = x;
			s32SurfaceY++;
		}
	}

#if WITH_HIGO_SURFACE_HANDLER	
	HI_GO_UnlockSurface(play->muxRx->higo.layerSurfaceHandle);
#endif
	// HI_GO_SetLayerPos(play->muxRx->higo.layerHandler, x, y);
	HI_GO_SetLayerPos( play->muxRx->higo.layerHandler, 0,0);
	(HI_VOID)HI_GO_RefreshLayer( play->muxRx->higo.layerHandler, NULL);
	// update the current pgs clear time
	if ( play->muxRx->s_bPgs)
	{
		play->muxRx->s_s64CurPgsClearTime = pstInfo->unSubtitleParam.stGfx.s64Pts + pstInfo->unSubtitleParam.stGfx.u32Duration;
	}
	
#endif

	return HI_SUCCESS;
}


/* after media has been set (when HiPlayer init or set media), subtitle callback must be reset*/
HI_S32 muxSubtitleOnDrawCallback(HI_VOID * u32UserData, const HI_UNF_SO_SUBTITLE_INFO_S *pstInfo, HI_VOID *pArg)
{
	HI_S32 s32Ret = HI_FAILURE;
	MUX_PLAY_T *play = (MUX_PLAY_T *)u32UserData;
#if 0	
	HI_PIXELDATA pData;
	HI_S32 s32DisplayWidth = 0, s32DisplayHeight = 0;
#endif

	if (HI_UNF_SUBTITLE_ASS != pstInfo->eType)
	{
		if (NULL != play->soParseHand)
		{
			HI_SO_SUBTITLE_ASS_DeinitParseHand(play->soParseHand);
			play->soParseHand = NULL;
		}
	}

#if 0	
	if (HIGO_INVALID_HANDLE != higo->memSurfaceHandle)
	{
		(HI_VOID)HI_GO_LockSurface( higo->memSurfaceHandle, pData, HI_TRUE);
		(HI_VOID)HI_GO_GetSurfaceSize(higo->memSurfaceHandle, &s32DisplayWidth, &s32DisplayHeight);
		(HI_VOID)HI_GO_UnlockSurface(higo->memSurfaceHandle);
	}
#endif

#if 0
	if (s32DisplayWidth != pstInfo->unSubtitleParam.stGfx.u32CanvasWidth ||
		s32DisplayHeight != pstInfo->unSubtitleParam.stGfx.u32CanvasHeight)
	{
		if (HIGO_INVALID_HANDLE != higo->memSurfaceHandle)
		{
			(HI_VOID)HI_GO_FreeSurface(higo->memSurfaceHandle);
			higo->memSurfaceHandle = HIGO_INVALID_HANDLE;
		}
		
		s32Ret = HI_GO_CreateSurface(pstInfo->unSubtitleParam.stGfx.u32CanvasWidth, pstInfo->unSubtitleParam.stGfx.u32CanvasHeight,
			HIGO_PF_8888, &higo->memSurfaceHandle);
		if (s32Ret == HI_FAILURE)
		{
			return HI_FAILURE;
		}
	}
#endif

	switch (pstInfo->eType)
	{
		case HI_UNF_SUBTITLE_BITMAP:
			MUX_SUBTITLE_CLEAR_ALL(play);
			s32Ret = _muxOutBitmapSubtitle(play, pstInfo);
			
			break;

		case HI_UNF_SUBTITLE_TEXT:
			MUX_SUBTITLE_CLEAR_ALL(play);
			s32Ret = _muxOutTextSubtitle(play, pstInfo);
			break;

		case HI_UNF_SUBTITLE_ASS:
			MUX_SUBTITLE_CLEAR_ALL(play);
			s32Ret = _muxOutAssSubtitle(play, pstInfo);
			
			break;

		default:
			break;
	}

	return s32Ret;
}

/* after media has been set (when HiPlayer init or set media), subtitle callback must be reset*/
HI_S32 muxSubtitleOnClearCallback(HI_VOID * u32UserData, HI_VOID *pArg)
{
	MUX_PLAY_T *play = (MUX_PLAY_T *)u32UserData;

	MUX_PLAY_DEBUG( "%s.%s (0x%x) Subtitle OnClear Callback entry...", play->muxFsm.name, play->osd->name, play );
	HI_UNF_SO_CLEAR_PARAM_S *pstClearParam = NULL;
	if(pArg)
	{
		pstClearParam = (HI_UNF_SO_CLEAR_PARAM_S *)pArg;
	}

	// l00192899 if clear operation time is earlier than s_s64CurPgsClearTime,current clear operation is out of date */
	// the clear operation must come from previous pgs subtitle.
	if(-1 != play->s_s64CurPgsClearTime && pstClearParam)
	{
		if (pstClearParam->s64ClearTime < play->s_s64CurPgsClearTime)
		{
			MUX_PLAY_INFO("s64Cleartime=%lld s_s64CurPgsClearTime=%lld is not time!\n", pstClearParam->s64ClearTime, play->s_s64CurPgsClearTime);
			return HI_SUCCESS;
		}
	}

//	MUX_PLAY_DEBUG( "[(%d, %d),(%d, %d)]", pstClearParam->x, pstClearParam->y, pstClearParam->w, pstClearParam->h);

	MUX_PLAY_DEBUG( "Subtitle OnClear on OSD...");
	muxSubtitleClear(play, pstClearParam);

	return HI_SUCCESS;
}


