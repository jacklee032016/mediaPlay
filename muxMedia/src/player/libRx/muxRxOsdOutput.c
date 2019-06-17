
/*
* base window of HIGO, provide logo, subtitle, alert info, scroll test display, etc
*/

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"

static int _muxHigoDecImage(HI_CHAR *pszFileName, HI_HANDLE *pSurface)
{
	HI_S32 ret         = HI_SUCCESS;
	HI_HANDLE hDecoder = 0;
	HIGO_DEC_ATTR_S stSrcDesc;
	HIGO_DEC_IMGINFO_S stImgInfo;
	HIGO_DEC_IMGATTR_S stAttr;

	stSrcDesc.SrcType = HIGO_DEC_SRCTYPE_FILE;
	stSrcDesc.SrcInfo.pFileName = pszFileName;

	ret = HI_GO_CreateDecoder(&stSrcDesc, &hDecoder);
	if(HI_SUCCESS != ret)
	{
		MUX_PLAY_ERROR("CreateDecoder failed: %s!", pszFileName );
		return EXIT_FAILURE;
	}

	ret = HI_GO_DecImgInfo(hDecoder, 0, &stImgInfo);
	if(HI_SUCCESS != ret)
	{
		MUX_PLAY_ERROR("HI_GO_DecImgInfo failed!" );
		HI_GO_DestroyDecoder(hDecoder);
		return EXIT_FAILURE;
	}

	if(stImgInfo.Width > 4095)
	{
		stAttr.Width = 4095;
	}
	else
	{
		stAttr.Width = stImgInfo.Width;
	}

	if(stImgInfo.Height > 4095)
	{
		stAttr.Height = 4095;
	}
	else
	{
		stAttr.Height = stImgInfo.Height;
	}

	stAttr.Format = HIGO_PF_8888;

	if(stImgInfo.Width > 4095 || stImgInfo.Height > 4095)
	{
		ret = HI_GO_DecImgData(hDecoder, 0, &stAttr, pSurface);
	}
	else
	{
		ret  = HI_GO_DecImgData(hDecoder, 0, NULL, pSurface);
	}
	if(HI_SUCCESS != ret)
	{
		MUX_PLAY_ERROR("HI_GO_DecImgData failed!" );
	}

	ret |= HI_GO_DestroyDecoder(hDecoder);

	return EXIT_SUCCESS;

}


int _muxOsdRefresh(MUX_OSD *osd)
{
	HI_S32 res = HI_SUCCESS;

#if WITH_HIGO_DEBUG
	MUX_PLAY_DEBUG("OSD '%s(%s)' Update Window", osd->name, osd->cfg->name );
#endif

	res = HI_GO_UpdateWindow( osd->winHandle, NULL);
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_WARN("Update OSD '%s' failed", osd->name);
		return EXIT_FAILURE;
	}

#if WITH_HIGO_DEBUG
	MUX_PLAY_DEBUG("OSD '%s(%s)' refresh layer.....", osd->name, osd->cfg->name );
#endif
	res = HI_GO_RefreshLayer(osd->desktop, NULL);
#if WITH_HIGO_DEBUG
	MUX_PLAY_DEBUG("OSD '%s(%s)' refresh layer ended!", osd->name, osd->cfg->name );
#endif
	if (HI_SUCCESS != res)
	{
//		MUX_ABORT("OSD: HIGO RefreshLayer OSD '%s' failed", osd->name);
		MUX_PLAY_WARN("OSD: HIGO RefreshLayer OSD '%s' failed: 0x%x", osd->name, res);
		return EXIT_FAILURE;
	}
#if WITH_HIGO_DEBUG
	MUX_PLAY_DEBUG("OSD '%s(%s)' return", osd->name, osd->cfg->name );
#endif

	return EXIT_SUCCESS;
}

int muxOsdOutputText(MUX_OSD *osd, int align, const HI_CHAR* pszText)
{
#define	__TEXT_BORDER_WIDTH		10
	if(osd == NULL)
	{
		MUX_PLAY_WARN("Set OSD is not initialized" );
		return HI_FAILURE;
	}
	
	if (HIGO_INVALID_HANDLE != osd->fontHandle && HIGO_INVALID_HANDLE != osd->desktop)
	{
		int res  = 0;
		HI_RECT					rect;
		rect.x = __TEXT_BORDER_WIDTH;
		rect.y = __TEXT_BORDER_WIDTH;
		rect.w = osd->cfg->width -__TEXT_BORDER_WIDTH - __TEXT_BORDER_WIDTH;
		rect.h = osd->cfg->height -__TEXT_BORDER_WIDTH - __TEXT_BORDER_WIDTH ;

//		res = HI_GO_TextOutEx(higo->fontHandle, higo->layerSurfaceHandle, pszText, pstRect, HIGO_LAYOUT_WRAP | HIGO_LAYOUT_HCENTER | HIGO_LAYOUT_BOTTOM);

//		res = HI_GO_TextOutEx(osd->fontHandle, osd->winSurface, pszText, &osd->rect, HIGO_LAYOUT_WRAP | HIGO_LAYOUT_HCENTER | HIGO_LAYOUT_BOTTOM);
		/* 06.05, 2019, changed as following */
//		MUX_PLAY_DEBUG("OSD Aler box ['%d, %d],[%d, %d]'", osd->rect.x, osd->rect.y, osd->rect.w, osd->rect.h );
		res = HI_GO_TextOutEx(osd->fontHandle, osd->winSurface, pszText, &rect, align);
		if (HI_SUCCESS != res)
		{
			MUX_PLAY_ERROR("TextOutEx on OSD %s failed: 0x%x (0x%x)", osd->name, res, osd->winSurface);
		}

		if(osd->type == MUX_OSD_TYPE_ALERT)
		{
			res = HI_GO_ChangeWindowZOrder( osd->winHandle, HIGO_ZORDER_MOVETOP);
			if (HI_SUCCESS != res)
			{
				MUX_PLAY_ERROR("Set OSD %s ZOrder failed", osd->name);
			}
		}

		res = _muxOsdRefresh(osd);
	}
	else
	{
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


int muxOsdClear(MUX_OSD *osd)
{
	int res = 0;

//	MUX_PLAY_DEBUG("OSD clear: set OSD backgroundColor =%#x", osd->cfg->backgroundColor );
#if WITH_HIGO_DEBUG
	MUX_PLAY_DEBUG("OSD '%s' clear", osd->name );
#endif
	res = HI_GO_FillRect(osd->winSurface, NULL, osd->cfg->backgroundColor, HIGO_COMPOPT_NONE);
	if(res != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("OSD clear: set OSD background failed, ret=%#x", res);
		return res;
	}

#if 0
	/* comment on Dec.22nd, 2017 */
	HI_GO_SetLayerPos( osd->higo->layerHandler, 0, 0);
#endif

	/* comment on Dec.27, 2017, and must be commented for mutex access OsdClear and OsdOutputText */
//	res = muxOsdOutputText(osd, ALERT_DEFAULT_LAYOUT, "");

	return res;
}

static int _muxOsdPrint(MUX_OSD *osd, int color, int align, const char* buf)
{
	int res;
	
	res = muxOsdClear(osd);
	res |= HI_GO_SetTextColor( osd->fontHandle, color);
	res = muxOsdOutputText(osd, align, buf);
	
	return res;
}

#define	ALERT_WITH_TIMER		0

#if ALERT_WITH_TIMER
static int _alertwinTimeoutCallback(int interval, void *param)
{
	int res = 0;
	MUX_OSD *osd = (MUX_OSD *)param;

	OSD_DESKTOP_LOCK(osd->higo);
#if 1
	/* comment just for testing */
	res = muxOsdClear(osd);
#endif

	osd->higo->muxPlayer->muxRx->winTimer = NULL;
	
	OSD_DESKTOP_UNLOCK( osd->higo);

	return res;
}
#endif

int muxOutputAlert(MUX_RX_T *muxRx, int color, int align, const char* frmt,...)
{
	int res = 0;
	
//	MUX_OSD *osd = muxOsdFind( &muxRx->higo, MUX_OSD_TYPE_ALERT);
	if(muxRx->higo.alert == NULL)
	{
		MUX_PLAY_WARN("OSD for Alert is not initialized, please check your configuratoni");
		return HI_FAILURE;
	}

	char buf[1024] = {0};
	va_list ap;
	
	va_start(ap, frmt);
	vsnprintf(buf, 1024, frmt, ap);
	va_end(ap);
	

	res = _muxOsdPrint(muxRx->higo.alert, color, align, buf);

#if ALERT_WITH_TIMER
	/* comments for testing. Dec.7th, 2017 */	
	muxRx->higo.winTimer = cmn_add_timer(2500, _alertwinTimeoutCallback, muxRx->higo.alert, "AlertTimer" );
#else
#endif

	return res;
}

/* only load image into another buffer surface of double buffer */
int muxOsdImageLoad(MUX_OSD *osd, char *imageFile)
{
	HI_S32 ret = 0;
	HI_HANDLE logoSurface;
//	HI_HANDLE	winSurface;	/* surface(render layer) of this window*/
	HIGO_BLTOPT_S stBltOpt = {0};

	ret = _muxHigoDecImage(imageFile, &logoSurface);
	if(EXIT_SUCCESS != ret)
	{
		MUX_PLAY_WARN("File '%s' is not supported format(gif, png, jpg, bmp)", imageFile);
		return EXIT_FAILURE;
	}
	
//	muxOutputAlert(osd->higo->muxPlayer->muxRx, COLOR_GREEN, ALERT_DEFAULT_LAYOUT, "OSD Decoded!" );
#if OSD_ENABLE_DOUBLE_BUFFER
	if(osd->higo->muxPlayer->playerConfig.enableLowDelay)
	{
		ret = HI_GO_GetWindowSurface(osd->winHandle, &osd->winSurface );
		if(ret != HI_SUCCESS)
		{
			MUX_PLAY_WARN("OSD '%s' failed in GetWindowSurface: 0x%x", osd->name, ret);
			return EXIT_FAILURE;
		}
	}
#else
	ret = HI_GO_GetWindowSurface(osd->winHandle, &osd->winSurface );
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_WARN("OSD '%s' failed in GetWindowSurface: 0x%x", osd->name, ret);
		CMN_ABORT("Get Window Surface failed");
		return EXIT_FAILURE;
	}
#endif
	
	stBltOpt.EnableScale = HI_TRUE;
	/** Blit it to graphic layer Surface */
	ret = HI_GO_Blit(logoSurface, NULL, osd->winSurface, NULL, &stBltOpt);
	if(HI_SUCCESS != ret)
	{
		MUX_PLAY_WARN("Blit Operation failed %s: 0x%x", imageFile, ret );
		return EXIT_FAILURE;
	}

#if OSD_ENABLE_DOUBLE_BUFFER
	/* flip to next buffer after mapping image data to osd->winSurface */
	if(osd->higo->muxPlayer->playerConfig.enableLowDelay)
	{
		ret = HI_GO_FlipWindowSurface(osd->winHandle);
		if(ret != HI_SUCCESS)
		{
			MUX_PLAY_WARN("OSD '%s' failed in FlipWindow: 0x%x", osd->name, ret);
			return EXIT_FAILURE;
		}
	}
#endif
	
#if WITH_HIGO_DEBUG
	MUX_PLAY_DEBUG("OSD %s IMAGE decoding %s sucessed, free logoSurface :0x%x", osd->name, imageFile, logoSurface);
#endif
	HI_GO_FreeSurface(logoSurface);

	return EXIT_SUCCESS;
}


int muxOsdImageDisplay(MUX_OSD *osd, int isShow)
{
	int ret = 0;

	if(isShow)
	{
#if 0
		/* when OSD of logo is created, put it to top of ZOrder. J.L. 06.11, 2019 */
		MUX_OSD *logoOsd = muxOsdFind(osd->higo, MUX_OSD_TYPE_LOGO);
		ret = HI_GO_ChangeWindowZOrder(logoOsd->winHandle, HIGO_ZORDER_MOVETOP);
		if (HI_SUCCESS != ret )
		{
			MUX_PLAY_WARN("Set ZOrder of OSD %s failed:0x%x", logoOsd->name, ret );
		}
#endif		
		ret = HI_GO_SetWindowOpacity( osd->winHandle, (osd->type == MUX_OSD_TYPE_LOGO)? osd->cfg->alpha: 255);

	}
	else
	{
		ret = HI_GO_SetWindowOpacity( osd->winHandle, 0);
	}
	
	if (HI_SUCCESS != ret )
	{
		MUX_PLAY_WARN("Set opacity of OSD %s failed:0x%x", osd->name, ret );
//		return ret;
	}

#if WITH_HIGO_DEBUG
	MUX_PLAY_DEBUG("OSD '%s(%s)' refresh for %s .....", osd->name, osd->cfg->name, (isShow==0)?"clear": "display");
#endif
	ret = _muxOsdRefresh(osd);

#if WITH_HIGO_DEBUG
	MUX_PLAY_DEBUG("OSD '%s(%s)' refresh for %s ended!!!", osd->name, osd->cfg->name, (isShow==0)?"clear": "display");
#endif

	return ret;
}



int muxOsdImageShow(MUX_OSD *osd, char *imageFile)
{
	int ret = 0;

#if WITH_HIGO_DEBUG
	MUX_PLAY_DEBUG("OSD '%s(%s)' load image file %s....", osd->name, osd->cfg->name, imageFile );
#endif
	ret = muxOsdImageLoad(osd, imageFile);
	if(ret != EXIT_SUCCESS)
	{
		return ret;
	}
	
#if WITH_HIGO_DEBUG
	MUX_PLAY_DEBUG("OSD '%s(%s)' display image file %s....", osd->name, osd->cfg->name, imageFile );
#endif
	ret = muxOsdImageDisplay(osd, TRUE);

	return ret;
}

int muxOsdPosition(MUX_OSD *osd, HI_RECT_S *rect, MUX_PLAY_T *play)
{
	HI_S32 res = 0;

	if(rect->s32Width >= 0 && rect->s32Height >= 0 && 
		rect->s32X >= 0 && rect->s32Y >= 0 )
	{
		res = HI_GO_SetWindowPos(osd->winHandle, rect->s32X, rect->s32Y);
		if (HI_SUCCESS != res )
		{
			MUX_PLAY_WARN("Set Pos of OSD Window %s failed", osd->name );
			exit(1);
			return EXIT_FAILURE;
		}

		res = HI_GO_ResizeWindow(osd->winHandle, rect->s32Width, rect->s32Height);
		if (HI_SUCCESS != res )
		{
			MUX_PLAY_WARN("Resize OSD window %s failed", osd->name );
			exit(1);
			return EXIT_FAILURE;
		}


		if(osd->type == MUX_OSD_TYPE_LOGO)
		{/* double buffer ?? */
			if(cmnMediaCheckCheckImageFile(osd->cfg->url) )
			{
//				MUX_PLAY_WARN("OSD %s Load image '%s' for location operation....", osd->name, osd->cfg->url);
				muxOsdImageShow(osd, osd->cfg->url);
//				MUX_PLAY_WARN("OSD %s Load image '%s' for location operation ended!!!!", osd->name, osd->cfg->url);
			}
		}

		if(play!=NULL && PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE) )
		{
			if(cmnMediaCheckCheckImageFile(play->currentUrl) )
			{
				muxOsdImageShow(osd, play->currentUrl );
			}
			else
			{
				MUX_PLAY_WARN("%s in PLAY_IMAGE state, but current URL is not local media file: %s", play->muxFsm.name, play->currentUrl);
			}
		}

#if 1		
		/* dec.29, 2017 after reposition, the surface of this OSD must be retrieved, otherwise outputText, setBackgraound and other operations based on surface will fail */
		/* dec,21, 2017 after size of window has been changed, the window must be redraw : re-get the surface of this window */
		res = HI_GO_GetWindowSurface( osd->winHandle, &osd->winSurface);
		if (HI_SUCCESS != res )
		{
			MUX_PLAY_WARN("Get surface of OSD window %s failed", osd->name);
			exit(1);
			return EXIT_FAILURE;
		}
#endif

	}
	else
	{
		MUX_PLAY_WARN("coordination of OSD %s is invalidate: [(%d,%d),(%d, %d)]", osd->name, rect->s32X, rect->s32Y, rect->s32Width, rect->s32Height);
		return EXIT_FAILURE;
	}
	
	if (HI_SUCCESS != res)
	{
		MUX_PLAY_WARN("Set position of OSD %s failed", osd->name);
		return EXIT_FAILURE;
	}

	if(play == NULL)
	{
		res = _muxOsdRefresh(osd);
	}
	else if(PLAYER_CHECK_STATE(play, HISVR_PLAYER_STATE_IMAGE))
	{
		res = _muxOsdRefresh(osd);
	}
	
	osd->cfg->left = rect->s32X;
	osd->cfg->top = rect->s32Y;
	osd->cfg->width = rect->s32Width;
	osd->cfg->height = rect->s32Height;

	/* added for show test in new position. 06.05, 2019 */
	osd->rect.x = rect->s32X;
	osd->rect.y = rect->s32Y;
	osd->rect.w = rect->s32Width;
	osd->rect.h = rect->s32Height;

#if 1//WITH_HIGO_DEBUG
	MUX_PLAY_DEBUG("OSD %s position at [(%d, %d), (%d, %d)]", osd->name, rect->s32X, rect->s32Y, rect->s32Width, rect->s32Height);
#endif

	return res;
}

int muxOsdToggleEnable(MUX_OSD *osd)
{
	HI_S32 res = 0;

	if(osd->enable == HI_TRUE)
		osd->enable = HI_FALSE;
	else
		osd->enable = HI_TRUE;
	
	res =  HI_GO_SetWindowOpacity( osd->winHandle, (osd->enable == TRUE)? osd->cfg->alpha: 0); 
	if (HI_SUCCESS != res )
	{
		MUX_PLAY_WARN("Toggle OSD Enabled failed, ret=%#x.\n", res);
		return res;
	}

	res = _muxOsdRefresh( osd);

	return res;
}

int muxOsdSetBackground(MUX_OSD *osd, int backgroundColor)
{
	HI_S32 res = 0;

	osd->cfg->backgroundColor = backgroundColor;
	
//	MUX_PLAY_DEBUG("set OSD backgroundColor =%#x", osd->cfg->backgroundColor );
	/* after repostiton it, HIGO_ERR_INVHANDLE(0xb0008004) happens */
	res = HI_GO_FillRect(osd->winSurface, NULL, osd->cfg->backgroundColor, HIGO_COMPOPT_NONE);
	if(res != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("set OSD background failed, ret=%#x", res);
		return res;
	}

	res = _muxOsdRefresh( osd);
	return res;
}

/* alpha : 0~255, 0: invisible for rect; 255: full visible for rect */
int muxOsdSetTransparency(MUX_OSD *osd, char alpha)
{
	HI_S32 res = 0;

	osd->cfg->alpha = alpha;
	
//	MUX_PLAY_DEBUG("set OSD transparency =%d", osd->cfg->alpha);
	res = HI_GO_SetWindowOpacity(osd->winHandle, osd->cfg->alpha);
	if(res != HI_SUCCESS)
	{
		MUX_PLAY_WARN("set OSD transparency failed, ret=%#x.\n", res);
		return res;
	}

	res = _muxOsdRefresh( osd);
	return res;
}

/* add mutex protect from subtitle callback */
int muxOsdSetFontSize(MUX_OSD *osd, int fontsize)
{
	HI_S32 res = 0;

	if(osd->cfg->fontSize != fontsize)
	{
		osd->cfg->fontSize = fontsize;
		
		HI_GO_DestroyText(osd->fontHandle);

		res = muxOsdCreateFont( osd);
	}

	return res;
}

/* only effective to subtitle OSD; for alert, every command defines its color */
int muxOsdSetFontColor(MUX_OSD *osd, int fontcolor)
{
	HI_S32 res = 0;

	osd->cfg->fontColor = fontcolor;
	res = HI_GO_SetTextColor( osd->fontHandle, fontcolor);

	return res;
}


