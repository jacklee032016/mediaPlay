
#include "libRx.h"

static HI_S32 _outputText(MUXLAB_HIGO_T *higo, const HI_CHAR* pszText, HI_RECT *pstRect)
{
	if (HIGO_INVALID_HANDLE != higo->fontHandle && HIGO_INVALID_HANDLE != higo->layerHandler)
	{
		(HI_VOID)HI_GO_TextOutEx(higo->fontHandle, higo->layerSurfaceHandle, pszText, pstRect,
			HIGO_LAYOUT_WRAP | HIGO_LAYOUT_HCENTER | HIGO_LAYOUT_BOTTOM);
		(HI_VOID)HI_GO_RefreshLayer(higo->layerHandler, NULL);
	}
	else
	{
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}

HI_S32 muxSubtitleOnDrawCallback(HI_VOID * u32UserData, const HI_UNF_SO_SUBTITLE_INFO_S *pstInfo, HI_VOID *pArg)
{
	HI_RECT rc = {260, 450, 760, 200};//{260, 550, 760, 100};
	HI_S32 s32Ret = HI_FAILURE;
	HI_CHAR *pszOutHand = NULL;
	HI_SO_SUBTITLE_ASS_DIALOG_S *pstDialog = NULL;
	HI_PIXELDATA pData;
	HI_S32 s32AssLen = 0;
	HI_U32 u32DisplayWidth = 0, u32DisplayHeight = 0;
	MUXLAB_PLAY_T *mux = (MUXLAB_PLAY_T *)u32UserData;
	MUXLAB_HIGO_T *higo = &mux->higo;

	if (HI_UNF_SUBTITLE_ASS != pstInfo->eType)
	{
		if (NULL != mux->soParseHand)
		{
			HI_SO_SUBTITLE_ASS_DeinitParseHand(mux->soParseHand);
			mux->soParseHand = NULL;
		}
	}

	switch (pstInfo->eType)
	{
		case HI_UNF_SUBTITLE_BITMAP:
			muxSubtitleClear(mux);
			PRINTF("sub title bitmap! \n");
			{
				HI_U32 i = 0, j = 0;
				HI_U32 u32PaletteIdx = 0;
				HI_U8 *pu8Surface = NULL;
				HI_S32 x = 100 ,y= 100;
				HI_S32 s32GfxX, s32GfxY;
				HI_S32 s32SurfaceW = 0, s32SurfaceH = 0;
				HI_S32 s32SurfaceX, s32SurfaceY, s32SurfaceYOff = 0;
				HI_S32 s32DrawH, s32DrawW;
				HI_BOOL bScale = 0;

				HI_GO_LockSurface( higo->layerSurfaceHandle, pData, HI_TRUE);
				HI_GO_GetSurfaceSize( higo->layerSurfaceHandle, &s32SurfaceW, &s32SurfaceH);

				pu8Surface = (HI_U8*)pData[0].pData;

				if (NULL == pu8Surface || NULL == pstInfo->unSubtitleParam.stGfx.stPalette)
				{
					HI_GO_UnlockSurface(higo->layerSurfaceHandle);
					return HI_SUCCESS;
				}

				if (s32SurfaceW * s32SurfaceH * pData[0].Bpp < pstInfo->unSubtitleParam.stGfx.h * pstInfo->unSubtitleParam.stGfx.w * 4)
				{
					HI_GO_UnlockSurface(higo->layerSurfaceHandle);
					return HI_SUCCESS;
				}

				/*l00192899,if the len==0, should not get u32PaletteIdx from pu8PixData, it will make a segment fault*/
				if( (pstInfo->unSubtitleParam.stGfx.w <=0) || (pstInfo->unSubtitleParam.stGfx.h <=0) || (pstInfo->unSubtitleParam.stGfx.pu8PixData ==NULL) || (pstInfo->unSubtitleParam.stGfx.u32Len <=0))
				{
					break;
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
								PRINTF("error\n");
								break;
							}
							u32PaletteIdx = pstInfo->unSubtitleParam.stGfx.pu8PixData[s32GfxY * pstInfo->unSubtitleParam.stGfx.w + s32GfxX];

							if (u32PaletteIdx >= HI_UNF_SO_PALETTE_ENTRY)
							{
								break;
							}


							pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 2]
							= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Red;
							pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 1]
							= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Green;
							pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 0]
							= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Blue;
							pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 3]
							= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Alpha;
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

							pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 2]
								= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Red;
							pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 1]
								= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Green;
							pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 0]
								= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Blue;
							pu8Surface[s32SurfaceY * pData[0].Pitch + 4 * s32SurfaceX + 3]
								= pstInfo->unSubtitleParam.stGfx.stPalette[u32PaletteIdx].u8Alpha;

							s32SurfaceX++;
						}
						
						s32SurfaceX = x;
						s32SurfaceY++;
					}
				}

				HI_GO_UnlockSurface(higo->layerSurfaceHandle);
				// HI_GO_SetLayerPos(higo->layerHandler, x, y);
				HI_GO_SetLayerPos( higo->layerHandler, 0,0);
				(HI_VOID)HI_GO_RefreshLayer(higo->layerHandler, NULL);
				// update the current pgs clear time
				if (mux->s_bPgs)
				{
					mux->s_s64CurPgsClearTime = pstInfo->unSubtitleParam.stGfx.s64Pts + pstInfo->unSubtitleParam.stGfx.u32Duration;
				}
			}
			break;

		case HI_UNF_SUBTITLE_TEXT:
			muxSubtitleClear(mux);
			HI_GO_SetLayerPos( higo->layerHandler, 0, 0);
			if (NULL == pstInfo->unSubtitleParam.stText.pu8Data)
			{
				return HI_FAILURE;
			}

			PRINTF("OUTPUT: %s ", pstInfo->unSubtitleParam.stText.pu8Data);
			(HI_VOID)_outputText(&mux->higo, (const HI_CHAR*)pstInfo->unSubtitleParam.stText.pu8Data, &rc);

			break;

		case HI_UNF_SUBTITLE_ASS:
			muxSubtitleClear(mux);
			
			HI_GO_SetLayerPos( higo->layerHandler, 0, 0);
			if ( NULL == mux->soParseHand )
			{
				s32Ret = HI_SO_SUBTITLE_ASS_InitParseHand(&mux->soParseHand);

				if ( s32Ret != HI_SUCCESS )
				{
					return HI_FAILURE;
				}
			}

			s32Ret = HI_SO_SUBTITLE_ASS_StartParse(pstInfo, &pstDialog, mux->soParseHand);
			if (s32Ret == HI_SUCCESS)
			{
				s32Ret = HI_SO_SUBTITLE_ASS_GetDialogText(pstDialog, &pszOutHand, &s32AssLen);

				if ( s32Ret != HI_SUCCESS || NULL == pszOutHand )
				{
					PRINTF("##########can't parse this Dialog!#########\n");
				}
				else
				{
					(HI_VOID)_outputText(&mux->higo, pszOutHand, &rc);
					PRINTF("OUTPUT: %s \n", pszOutHand);
				}
			}
			else
			{
				PRINTF("##########can't parse this ass file!#########\n");
			}

			free(pszOutHand);
			pszOutHand = NULL;
			HI_SO_SUBTITLE_ASS_FreeDialog(&pstDialog);
			break;

		default:
			break;
	}

	return HI_SUCCESS;
}

HI_S32 muxSubtitleOnClearCallback(HI_VOID * u32UserData, HI_VOID *pArg)
{
	HI_RECT rc = {160, 550, 760, 100};
	HI_CHAR TextClear[] = "";
	MUXLAB_PLAY_T *mux = (MUXLAB_PLAY_T *)u32UserData;

#if 1
	HI_UNF_SO_CLEAR_PARAM_S *pstClearParam = NULL;
	if(pArg)
	{
		pstClearParam = (HI_UNF_SO_CLEAR_PARAM_S *)pArg;
	}

	// l00192899 if clear operation time is earlier than s_s64CurPgsClearTime,current clear operation is out of date */
	// the clear operation must come from previous pgs subtitle.
	if(-1 != mux->s_s64CurPgsClearTime && pstClearParam)
	{
		if (pstClearParam->s64ClearTime < mux->s_s64CurPgsClearTime)
		{
			PRINTF("s64Cleartime=%lld s_s64CurPgsClearTime=%lld is not time!\n", pstClearParam->s64ClearTime, mux->s_s64CurPgsClearTime);
			return HI_SUCCESS;
		}
	}
#endif

	PRINTF("CLEAR Subtitle! \n");
	(HI_VOID)HI_GO_FillRect( mux->higo.layerSurfaceHandle, NULL, 0x00000000, HIGO_COMPOPT_NONE);

	if (HIGO_INVALID_HANDLE != mux->higo.fontHandle&& HIGO_INVALID_HANDLE != mux->higo.layerHandler)
	{
		(HI_VOID)HI_GO_TextOutEx(mux->higo.fontHandle, mux->higo.layerSurfaceHandle, TextClear, &rc,
			HIGO_LAYOUT_WRAP | HIGO_LAYOUT_HCENTER | HIGO_LAYOUT_BOTTOM);
	}

	if (HIGO_INVALID_HANDLE != mux->higo.layerHandler)
		(HI_VOID)HI_GO_RefreshLayer(mux->higo.layerHandler, NULL);

	return HI_SUCCESS;
}

HI_S32 muxSubtitleClear(MUXLAB_PLAY_T *mux)
{
	HI_RECT rc = {160, 550, 760, 100};
	HI_CHAR TextClear[] = "";

	PRINTF("CLEAR Subtitle! \n");
	(HI_VOID)HI_GO_FillRect(mux->higo.layerSurfaceHandle, NULL, 0x00000000, HIGO_COMPOPT_NONE);

	if (HIGO_INVALID_HANDLE != mux->higo.fontHandle && HIGO_INVALID_HANDLE != mux->higo.layerHandler)
	{
		(HI_VOID)HI_GO_TextOutEx( mux->higo.fontHandle, mux->higo.layerSurfaceHandle, TextClear, &rc, HIGO_LAYOUT_WRAP | HIGO_LAYOUT_HCENTER | HIGO_LAYOUT_BOTTOM);
	}

	if (HIGO_INVALID_HANDLE != mux->higo.layerHandler)
		(HI_VOID)HI_GO_RefreshLayer( mux->higo.layerHandler, NULL);

	return HI_SUCCESS;
}


