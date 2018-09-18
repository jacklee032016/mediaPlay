#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"

#include "hi_unf_hdmi.h"
#include "hi_unf_edid.h"
#include "hi_unf_disp.h"
#include "hi_adp.h"
#include "hi_adp_hdmi.h"

extern HI_S32 HI_UNF_HDMI_GetDeepColor(HI_UNF_HDMI_ID_E enHdmi, HI_UNF_HDMI_DEEP_COLOR_E *penDeepColor);


typedef struct hiHDMI_ARGS_S
{
	HI_UNF_HDMI_ID_E	enHdmi;
	HI_U32				u32HdcpVersion;
}HDMI_ARGS_S;

static HDMI_ARGS_S g_stHdmiArgs;

HI_U32 g_HDCPFlag         = HI_FALSE;
HI_U32 g_HDMIUserCallbackFlag = HI_FALSE;
HI_U32 g_enDefaultMode    = HI_UNF_HDMI_DEFAULT_ACTION_HDMI;//HI_UNF_HDMI_DEFAULT_ACTION_NULL;
 HI_UNF_HDMI_CALLBACK_FUNC_S g_stCallbackFunc;

User_HDMI_CallBack pfnHdmiUserCallback = NULL;

//#define HI_HDCP_SUPPORT
#ifdef HI_HDCP_SUPPORT
const HI_CHAR * pstencryptedHdcpKey = "EncryptedKey_332bytes.bin";
#endif


static HI_VOID HDMI_PrintAttr(HI_UNF_HDMI_ATTR_S *pstHDMIAttr, char *title)
{
	MUX_PLAY_DEBUG("\n=====HDMI Attributes : %s=====\n"
		"\tbEnableHdmi:%d\n"
		"\tbEnableVideo:%d\n"
		"\tenVidOutMode:%s\n"
		"\tenDeepColorMode:%s\n"
		"\tbxvYCCMode:%d\n\n"
		"\tbEnableAudio:%d\n"
		"\tbEnableAviInfoFrame:%d\n"
		"\tbEnableAudInfoFrame:%d\n"
		"\tbEnableSpdInfoFrame:%d\n"
		"\tbEnableMpegInfoFrame:%d\n\n"
		"==============================\n", title, 
		pstHDMIAttr->bEnableHdmi,
		pstHDMIAttr->bEnableVideo,
		muxHdmiVideoModeName(pstHDMIAttr->enVidOutMode), muxHdmiDeepColorName(pstHDMIAttr->enDeepColorMode),
		pstHDMIAttr->bxvYCCMode,
		
		pstHDMIAttr->bEnableAudio,
		pstHDMIAttr->bEnableAudInfoFrame,pstHDMIAttr->bEnableAudInfoFrame,
		pstHDMIAttr->bEnableSpdInfoFrame,pstHDMIAttr->bEnableMpegInfoFrame);
	return;
}

static void _hdmiStatus(HI_UNF_HDMI_STATUS_S *hdmiStatus)
{
	MUX_PLAY_DEBUG("\n=====HDMI Status=====\n"
		"\tbConnected:%d\n"
		"\tbSinkPowerOn:%d\n"
		"\tbAuthed:%d\n"
		"\tHDCP Version:%s\n"
		"\tBksv:%x:%x:%x:%x:%x\n"
		"==============================\n",
		hdmiStatus->bConnected,
		hdmiStatus->bSinkPowerOn,
		hdmiStatus->bAuthed, muxHdmiHdcpVersion(hdmiStatus->enHDCPVersion), 
		hdmiStatus->u8Bksv[0], hdmiStatus->u8Bksv[1], hdmiStatus->u8Bksv[2], hdmiStatus->u8Bksv[3], hdmiStatus->u8Bksv[4]);
	return;
}

#if 0
void HDMI_HotPlug_Proc(HI_VOID *pPrivateData)
{
	HI_S32          ret = HI_SUCCESS;
	HDMI_ARGS_S     *pArgs  = (HDMI_ARGS_S*)pPrivateData;
	HI_UNF_HDMI_ID_E       hHdmi   =  pArgs->enHdmi;
	HI_UNF_HDMI_ATTR_S             stHdmiAttr;
	//HI_UNF_HDMI_INFOFRAME_S        stInfoFrame;
	HI_UNF_EDID_BASE_INFO_S        stSinkCap;
	HI_UNF_HDMI_STATUS_S           stHdmiStatus;

#ifdef HI_HDCP_SUPPORT
	static HI_U8 u8FirstTimeSetting = HI_TRUE;
#endif

	MUX_PLAY_DEBUG("--- Get HDMI event: HOTPLUG. ---");

	ret = HI_UNF_HDMI_GetStatus(hHdmi, &stHdmiStatus);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get HDMI status failed: %#x", ret);
	}
	
	if (HI_FALSE == stHdmiStatus.bConnected)
	{
		MUX_PLAY_ERROR("HDMI No Connect");
		return;
	}

	_hdmiStatus(&stHdmiStatus);

	ret = HI_UNF_HDMI_GetAttr(hHdmi, &stHdmiAttr);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get HDMI Attribute failed: %#x", ret);
	}
	HDMI_PrintAttr(&stHdmiAttr, "After plugin");

	
	ret = HI_UNF_HDMI_GetSinkCapability(hHdmi, &stSinkCap);
	if(ret == HI_SUCCESS)
	{
		//stHdmiAttr.enVidOutMode = HI_UNF_HDMI_VIDEO_MODE_YCBCR444;
		if(HI_TRUE == stSinkCap.bSupportHdmi)
		{
			stHdmiAttr.bEnableHdmi = HI_TRUE;
			if(HI_TRUE != stSinkCap.stColorSpace.bYCbCr444)
			{
				stHdmiAttr.enVidOutMode = HI_UNF_HDMI_VIDEO_MODE_RGB444;
			}
			else
			{                
				stHdmiAttr.enVidOutMode = HI_UNF_HDMI_VIDEO_MODE_YCBCR444;  /* user can choicen RGB/YUV*/
			}
		}
		else
		{
			stHdmiAttr.enVidOutMode = HI_UNF_HDMI_VIDEO_MODE_RGB444;
			/* read real edid ok && sink not support hdmi,then we run in dvi mode */
			stHdmiAttr.bEnableHdmi = HI_FALSE;
		}
	}
	else
	{
		//when get capability fail,use default mode
		if(g_enDefaultMode != HI_UNF_HDMI_DEFAULT_ACTION_DVI)
			stHdmiAttr.bEnableHdmi = HI_TRUE;
		else
			stHdmiAttr.bEnableHdmi = HI_FALSE;
	}

	if(HI_TRUE == stHdmiAttr.bEnableHdmi)
	{
		stHdmiAttr.bEnableAudio = HI_TRUE;
		stHdmiAttr.bEnableVideo = HI_TRUE;
		stHdmiAttr.bEnableAudInfoFrame = HI_TRUE;
		stHdmiAttr.bEnableAviInfoFrame = HI_TRUE;
	}
	else
	{
		stHdmiAttr.bEnableAudio = HI_FALSE;
		stHdmiAttr.bEnableVideo = HI_TRUE;
		stHdmiAttr.bEnableAudInfoFrame = HI_FALSE;
		stHdmiAttr.bEnableAviInfoFrame = HI_FALSE;
		stHdmiAttr.enVidOutMode = HI_UNF_HDMI_VIDEO_MODE_RGB444;
	}

#ifdef HI_HDCP_SUPPORT
	if (u8FirstTimeSetting == HI_TRUE)
	{
		u8FirstTimeSetting = HI_FALSE;
		if (g_HDCPFlag == HI_TRUE)
		{
			stHdmiAttr.bHDCPEnable = HI_TRUE;//Enable HDCP
		}
		else
		{
			stHdmiAttr.bHDCPEnable= HI_FALSE;
		}
	}
	else
	{
		//HDCP Enable use default setting!!
	}
#endif

	ret = HI_UNF_HDMI_SetAttr(hHdmi, &stHdmiAttr);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("set HDMI Attribute failed: %#x", ret);
	}

	/* HI_UNF_HDMI_SetAttr must before HI_UNF_HDMI_Start! */
	ret = HI_UNF_HDMI_Start(hHdmi);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get HDMI start failed: %#x", ret);
	}

	HDMI_PrintAttr(&stHdmiAttr, "After SetAttr");

	muxHdmiPrintSinkCap();


	usleep(300 * 1000);

	ret = HI_UNF_HDMI_CEC_Enable(hHdmi);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("HDMI ECE enable failed: %#x", ret);
	}


	muxHdmiCECCommand(HI_UNF_CEC_LOGICALADD_TUNER_1, HI_UNF_CEC_LOGICALADD_TV, CEC_OPCODE_IMAGE_VIEW_ON, 0, NULL, 0);

	muxHdmiCECSetDeviceOsdName();

	muxHdmiCecCheckStatus();
	return;

}
#else

HI_UNF_ENC_FMT_E itemResolution(HI_CHAR * res, HI_U32 i)
{
	//printf("	res=%s, fps=%u\n", res, i);
	if (strcmp(res, "720x480") == 0)
	{
		switch (i)
		{
			case 60:
				return HI_UNF_ENC_FMT_480P_60;
		}
	}
	/* else if (strcmp(res, "720x576") == 0) {
	switch (i) {
	case 50:
	return HI_UNF_ENC_FMT_576P_50;
	}
	}*/
	else if (strcmp(res, "1280x720") == 0)
	{
		switch (i)
		{
			case 50:
				return HI_UNF_ENC_FMT_720P_50;
			case 60:
				return HI_UNF_ENC_FMT_720P_60;
		}
	}
	else if (strcmp(res, "1920x1080p") == 0)
	{
		switch (i)
		{
			case 24:
				return HI_UNF_ENC_FMT_1080P_24;
			case 25:
				return HI_UNF_ENC_FMT_1080P_25;
			case 30:
				return HI_UNF_ENC_FMT_1080P_30;
			case 50:
				return HI_UNF_ENC_FMT_1080P_50;
			case 60:
				return HI_UNF_ENC_FMT_1080P_60;
		}
	}
	else if (strcmp(res, "1920x1080i") == 0)
	{
		switch (i)
		{
			case 50:
				return HI_UNF_ENC_FMT_1080i_50;
			case 60:
				return HI_UNF_ENC_FMT_1080i_60;
		}
	}
	/* else if (strcmp(res, "4096x2160") == 0) {
		switch (i) {
		case 24:
		return HI_UNF_ENC_FMT_4096X2160_24;
		case 25:
		return HI_UNF_ENC_FMT_4096X2160_25;
		case 30:
		return HI_UNF_ENC_FMT_4096X2160_30;
		case 50:
		return HI_UNF_ENC_FMT_4096X2160_50;
		case 60:
		return HI_UNF_ENC_FMT_4096X2160_60;
	}
	}*/
	else if (strcmp(res, "3840x2160") == 0)
	{
		switch (i)
		{
			case 24:
				return HI_UNF_ENC_FMT_3840X2160_24;
			case 25:
				return HI_UNF_ENC_FMT_3840X2160_25;
			case 30:
				return HI_UNF_ENC_FMT_3840X2160_30;
			case 50:
				return HI_UNF_ENC_FMT_3840X2160_50;
			case 60:
				return HI_UNF_ENC_FMT_3840X2160_60;
		}
	}
	else if (i == 60)
	{
		/*if (strcmp(res, "640x480") == 0)
		 return HI_UNF_ENC_FMT_861D_640X480_60;
		 else if (strcmp(res, "800x600") == 0)
		 return HI_UNF_ENC_FMT_VESA_800X600_60;
		 else if (strcmp(res, "1024x768") == 0)
		 return HI_UNF_ENC_FMT_VESA_1024X768_60;
		 else if (strcmp(res, "1280x720") == 0)
		 return HI_UNF_ENC_FMT_VESA_1280X720_60;
		 else if (strcmp(res, "1280x800") == 0)
		 return HI_UNF_ENC_FMT_VESA_1280X800_60;
		 else if (strcmp(res, "1280x1024") == 0)
		 return HI_UNF_ENC_FMT_VESA_1280X1024_60;
		 else if (strcmp(res, "1360x768") == 0)
		 return HI_UNF_ENC_FMT_VESA_1360X768_60;
		 else if (strcmp(res, "1366x768") == 0)
		 return HI_UNF_ENC_FMT_VESA_1366X768_60;
		 else if (strcmp(res, "1400x1050") == 0)
		 return HI_UNF_ENC_FMT_VESA_1400X1050_60;
		 else if (strcmp(res, "1440x900") == 0)
		 return HI_UNF_ENC_FMT_VESA_1440X900_60;
		 else if (strcmp(res, "1600x1200") == 0)
		 return HI_UNF_ENC_FMT_VESA_1600X1200_60;
		 else if (strcmp(res, "1680x1050") == 0)
		 return HI_UNF_ENC_FMT_VESA_1680X1050_60;
		 else if (strcmp(res, "1920x1080") == 0)
		 return HI_UNF_ENC_FMT_VESA_1920X1080_60;
		 else if (strcmp(res, "1920x1200") == 0)
		 return HI_UNF_ENC_FMT_VESA_1920X1200_60;
		 else if (strcmp(res, "1920x1440") == 0)
		 return HI_UNF_ENC_FMT_VESA_1920X1440_60;
		 else if (strcmp(res, "2048x1152") == 0)
		 return HI_UNF_ENC_FMT_VESA_2048X1152_60;
		 else*/
		if (strcmp(res, "2560x1440") == 0)
			return HI_UNF_ENC_FMT_VESA_2560X1440_60_RB;
		else if (strcmp(res, "2560x1600") == 0)
			return HI_UNF_ENC_FMT_VESA_2560X1600_60_RB;
	}

	return HI_UNF_ENC_FMT_BUTT;
}


void HDMI_HotPlug_Proc(HI_VOID *pPrivateData)
{
	HI_S32 ret = HI_SUCCESS;
	HDMI_ARGS_S *pArgs = (HDMI_ARGS_S*) pPrivateData;
	HI_UNF_HDMI_ID_E hHdmi = pArgs->enHdmi;
	HI_UNF_HDMI_ATTR_S stHdmiAttr;
	//HI_UNF_HDMI_INFOFRAME_S        stInfoFrame;
	HI_UNF_EDID_BASE_INFO_S stSinkCap;
	HI_UNF_HDMI_STATUS_S stHdmiStatus;

	HI_UNF_ENC_FMT_E	autoTvFormat = HI_UNF_ENC_FMT_BUTT;
#ifdef HI_HDCP_SUPPORT
	static HI_U8 u8FirstTimeSetting = HI_TRUE;
#endif

	MUX_PLAY_DEBUG("--- Get HDMI event: HOTPLUG. ---");

	ret = HI_UNF_HDMI_GetStatus(hHdmi, &stHdmiStatus);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get HDMI status failed: %#x", ret);
	}

	if (HI_FALSE == stHdmiStatus.bConnected)
	{
		MUX_PLAY_ERROR("No HDMI Connect");
		return;
	}

	HI_UNF_HDMI_GetAttr(hHdmi, &stHdmiAttr);
	ret = HI_UNF_HDMI_GetSinkCapability(hHdmi, &stSinkCap);
	if (ret == HI_SUCCESS)
	{
		HI_UNF_EDID_TIMING_S *pt = &(stSinkCap.stPerferTiming);

		HI_U32 hh = pt->u32HACT + pt->u32HBB + pt->u32HFB;
		HI_U32 vv = pt->u32VACT + pt->u32VBB + pt->u32VFB;
		HI_U32 fps = 0;
		if (hh && vv)
		{
			fps = (HI_U32) (pt->u32PixelClk * 1000.0 / hh / vv + 0.5);

			char res[20];
			char interlaced = pt->bInterlace ? 'i' : 'p';
			if (pt->u32HACT == 1920 && pt->u32VACT == 1080)
			{
				sprintf(res, "%dx%d%c", pt->u32HACT, pt->u32VACT, interlaced);
			}
			else
			{
				sprintf(res, "%dx%d", pt->u32HACT, pt->u32VACT);
			}

			MUX_PLAY_DEBUG("Check profile from EDID: fps:%d; resolution:%s; mode:%c", fps, res, interlaced);

			HI_UNF_ENC_FMT_E enFormat = itemResolution(res, fps);
			MUX_PLAY_DEBUG("calculated resolution is :%s!", muxHdmiFormatName(enFormat) );
			if (enFormat == HI_UNF_ENC_FMT_BUTT)
			{
				autoTvFormat = stSinkCap.enNativeFormat;
				MUX_PLAY_ERROR("Failed to get preferred resolution[%s] from PTB\n", res);
			}
			else
			{
				autoTvFormat = enFormat;
			}
		}
		else
		{
			autoTvFormat = stSinkCap.enNativeFormat;
			MUX_PLAY_WARN("No H/V params in EDDI, set as native format", muxHdmiFormatName(autoTvFormat) );
		}

		MUX_PLAY_DEBUG("set resolution is :%s!", muxHdmiFormatName(autoTvFormat) );
		muxHdmiReplugMonitor(autoTvFormat);

		if (HI_TRUE == stSinkCap.bSupportHdmi)
		{
			stHdmiAttr.bEnableHdmi = HI_TRUE;
			if (HI_TRUE == stSinkCap.stColorSpace.bRGB444)
			{
				stHdmiAttr.enVidOutMode = HI_UNF_HDMI_VIDEO_MODE_RGB444;
			}
			else if (HI_TRUE == stSinkCap.stColorSpace.bYCbCr444)
			{
				stHdmiAttr.enVidOutMode = HI_UNF_HDMI_VIDEO_MODE_YCBCR444;
			}
			else if (HI_TRUE == stSinkCap.stColorSpace.bYCbCr422)
			{
				stHdmiAttr.enVidOutMode = HI_UNF_HDMI_VIDEO_MODE_YCBCR422;
			}
			else
			{
				stHdmiAttr.enVidOutMode = HI_UNF_HDMI_VIDEO_MODE_YCBCR420; /* user can chooce RGB/YUV*/
			}

			//Dolby or HDR or SDR
			HI_S32 s32Ret;
			//if (s32Ret == HI_SUCCESS)
			{
				if (HI_TRUE == stSinkCap.bDolbySupport)
				{
					s32Ret = HI_UNF_DISP_SetHDRType(HI_UNF_DISPLAY1, HI_UNF_DISP_HDR_TYPE_DOLBY);
					if (HI_SUCCESS != s32Ret)
					{
						MUX_PLAY_ERROR("call HI_UNF_DISP_SetHDRType Dolby failed %#x.\n", s32Ret);
					}
					else
					{
						MUX_PLAY_DEBUG("call HI_UNF_DISP_SetHDRType Dolby succeed %#x.\n", s32Ret);
					}
				}
				else if (HI_TRUE == stSinkCap.bHdrSupport)
				{
					s32Ret = HI_UNF_DISP_SetHDRType(HI_UNF_DISPLAY1, HI_UNF_DISP_HDR_TYPE_HDR10);
					if (HI_SUCCESS != s32Ret)
					{
						MUX_PLAY_ERROR("call HI_UNF_DISP_SetHDRType HDR10 failed %#x.\n", s32Ret);
					}
					else
					{
						MUX_PLAY_DEBUG("call HI_UNF_DISP_SetHDRType HDR10 succeed %#x.\n", s32Ret);
					}
				}
				else
				{
					s32Ret = HI_UNF_DISP_SetHDRType(HI_UNF_DISPLAY1, HI_UNF_DISP_HDR_TYPE_NONE);
					if (HI_SUCCESS != s32Ret)
					{
						MUX_PLAY_ERROR("call HI_UNF_DISP_SetHDRType SDR failed %#x.\n", s32Ret);
					}
				}
			}

			//deep color : for phase 1, hardcode it to 8bit per channel
			/*if (stSinkCap.stDeepColor.bDeepColor30Bit)
				stHdmiAttr.enDeepColorMode = HI_UNF_HDMI_DEEP_COLOR_30BIT;
			else if (stSinkCap.stDeepColor.bDeepColorY444)
				stHdmiAttr.enDeepColorMode = HI_UNF_HDMI_DEEP_COLOR_24BIT;*/
		}
		else
		{
			stHdmiAttr.enVidOutMode = HI_UNF_HDMI_VIDEO_MODE_RGB444;
			//读取到了edid，并且不支持hdmi则进入dvi模式
			//read real edid ok && sink not support hdmi,then we run in dvi mode
			stHdmiAttr.bEnableHdmi = HI_FALSE;
		}
	}
	else
	{
		//when get capability fail,use default mode
		if (g_enDefaultMode != HI_UNF_HDMI_DEFAULT_ACTION_DVI)
			stHdmiAttr.bEnableHdmi = HI_TRUE;
		else
			stHdmiAttr.bEnableHdmi = HI_FALSE;
	}

	if (HI_TRUE == stHdmiAttr.bEnableHdmi)
	{
		stHdmiAttr.bEnableAudio = HI_TRUE;
		stHdmiAttr.bEnableVideo = HI_TRUE;
		stHdmiAttr.bEnableAudInfoFrame = HI_TRUE;
		stHdmiAttr.bEnableAviInfoFrame = HI_TRUE;
	}
	else
	{
		stHdmiAttr.bEnableAudio = HI_FALSE;
		stHdmiAttr.bEnableVideo = HI_TRUE;
		stHdmiAttr.bEnableAudInfoFrame = HI_FALSE;
		stHdmiAttr.bEnableAviInfoFrame = HI_FALSE;
		stHdmiAttr.enVidOutMode = HI_UNF_HDMI_VIDEO_MODE_RGB444;
	}

#ifdef HI_HDCP_SUPPORT
	if (u8FirstTimeSetting == HI_TRUE)
	{
		u8FirstTimeSetting = HI_FALSE;
		if (g_HDCPFlag > 0)
		{
			stHdmiAttr.bHDCPEnable = HI_TRUE;            //Enable HDCP
			if(g_HDCPFlag == 3)
				stHdmiAttr.enHDCPMode = HI_UNF_HDMI_HDCP_MODE_AUTO;
			else if(g_HDCPFlag == 1)
				stHdmiAttr.enHDCPMode = HI_UNF_HDMI_HDCP_MODE_1_4;
			else if(g_HDCPFlag == 2)
				stHdmiAttr.enHDCPMode = HI_UNF_HDMI_HDCP_MODE_2_2;
			else
				stHdmiAttr.bHDCPEnable= HI_FALSE;
		}
		else
		{
			stHdmiAttr.bHDCPEnable= HI_FALSE;
		}
	}
	else
	{
		//HDCP Enable use default setting!!
	}
#endif

	ret = HI_UNF_HDMI_SetAttr(hHdmi, &stHdmiAttr);

	/* HI_UNF_HDMI_SetAttr must before HI_UNF_HDMI_Start! */
	ret = HI_UNF_HDMI_Start(hHdmi);

#if 0
	{
		HDMI_PrintAttr(&stHdmiAttr, "After SetAttr");
		HI_UNF_HDMI_DEEP_COLOR_E enDeepColor = HI_UNF_HDMI_DEEP_COLOR_BUTT;
		HI_UNF_HDMI_GetDeepColor(hHdmi, &enDeepColor);

		/*HI_BOOL pbEnable = HI_TRUE;
		HI_UNF_DISP_SetBT2020Enable(HI_UNF_DISPLAY1, pbEnable);
		pbEnable = HI_FALSE;
		HI_UNF_DISP_GetBT2020Enable(HI_UNF_DISPLAY1, &pbEnable);*/

		HI_UNF_DISP_HDR_TYPE_E hdrType = HI_UNF_DISP_HDR_TYPE_BUTT;
		HI_UNF_DISP_GetHDRType(HI_UNF_DISPLAY1, &hdrType);

//	MUX_PLAY_INFO("----->HDRType = %d  enDeepColor=%d", hdrType, enDeepColor);
		HI_UNF_ENC_FMT_E fmtTmp = HI_UNF_ENC_FMT_BUTT;
		HI_UNF_DISP_GetFormat(HI_UNF_DISPLAY1, &fmtTmp);
		
		MUX_PLAY_INFO("Resolution:%s; deepColor=%d",muxHdmiFormatName(fmtTmp), enDeepColor) ;
	}
#endif
	
	return;

}

#endif

HI_VOID HDMI_UnPlug_Proc(HI_VOID *pPrivateData)
{
	HDMI_ARGS_S     *pArgs  = (HDMI_ARGS_S*)pPrivateData;
	HI_UNF_HDMI_ID_E       hHdmi   =  pArgs->enHdmi;
	int ret;

	MUX_PLAY_DEBUG("--- Get HDMI event: UnPlug. --- ");
	HI_UNF_HDMI_Stop(hHdmi);

	ret = HI_UNF_HDMI_CEC_Disable(hHdmi);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("HDMI ECE enable failed: %#x", ret);
	}

	return;
}


static HI_U32 HDCPFailCount = 0;
HI_VOID HDMI_HdcpFail_Proc(HI_VOID *pPrivateData)
{
	HI_UNF_HDMI_STATUS_S        stHdmiStatus;
	HDMI_ARGS_S                 *pArgs  = (HDMI_ARGS_S*)pPrivateData;
	HI_UNF_HDMI_ID_E            hHdmi   =  pArgs->enHdmi;
	HI_U32                      u32Ret  = HI_FAILURE;

	memset(&stHdmiStatus, 0, sizeof(HI_UNF_HDMI_STATUS_S));
	u32Ret = HI_UNF_HDMI_GetStatus(hHdmi, &stHdmiStatus);
	if(u32Ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get hdmi status failed: %#x", u32Ret);
	}

	MUX_PLAY_ERROR(" --- Get HDMI event: HDCP_FAIL, version(%d). --- ", stHdmiStatus.enHDCPVersion);
	
	HDCPFailCount ++ ;
	if(HDCPFailCount >= 50)
	{
		HDCPFailCount = 0;
		MUX_PLAY_WARN("Warrning:Customer need to deal with HDCP Fail!!!!!!");
	}
	
#if 0
	HI_UNF_HDMI_GetAttr(0, &stHdmiAttr);
	stHdmiAttr.bHDCPEnable = HI_FALSE;
	HI_UNF_HDMI_SetAttr(0, &stHdmiAttr);
#endif

	return;
}

HI_VOID HDMI_HdcpSuccess_Proc(HI_VOID *pPrivateData)
{
	HI_UNF_HDMI_STATUS_S        stHdmiStatus;
	HDMI_ARGS_S                 *pArgs  = (HDMI_ARGS_S*)pPrivateData;
	HI_UNF_HDMI_ID_E            hHdmi   =  pArgs->enHdmi;
	HI_U32                      u32Ret  = HI_FAILURE;

	memset(&stHdmiStatus, 0, sizeof(HI_UNF_HDMI_STATUS_S));
	u32Ret = HI_UNF_HDMI_GetStatus(hHdmi, &stHdmiStatus);
	if(u32Ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get hdmi status failed! ");
	}

	MUX_PLAY_DEBUG(" --- Get HDMI event: HDCP_SUCCESS, version(%d) ---", stHdmiStatus.enHDCPVersion);
	return;
}

HI_VOID HDMI_Event_Proc(HI_UNF_HDMI_EVENT_TYPE_E event, HI_VOID *pPrivateData)
{

	switch ( event )
	{
		case HI_UNF_HDMI_EVENT_HOTPLUG:
			HDMI_HotPlug_Proc(pPrivateData);
			break;
			
		case HI_UNF_HDMI_EVENT_NO_PLUG:
			HDMI_UnPlug_Proc(pPrivateData);
			break;
			
		case HI_UNF_HDMI_EVENT_EDID_FAIL:
			MUX_PLAY_DEBUG("HI_UNF_HDMI_EVENT_EDID_FAIL**********");
			break;
			
		case HI_UNF_HDMI_EVENT_HDCP_FAIL:
			HDMI_HdcpFail_Proc(pPrivateData);
			break;
		
		case HI_UNF_HDMI_EVENT_HDCP_SUCCESS:
			HDMI_HdcpSuccess_Proc(pPrivateData);
			break;
			
		case HI_UNF_HDMI_EVENT_RSEN_CONNECT:
			MUX_PLAY_DEBUG("HI_UNF_HDMI_EVENT_RSEN_CONNECT**********");
			break;

		case HI_UNF_HDMI_EVENT_RSEN_DISCONNECT:
			MUX_PLAY_DEBUG("HI_UNF_HDMI_EVENT_RSEN_DISCONNECT**********");
			break;
			
		case HI_UNF_HDMI_EVENT_HDCP_USERSETTING:
			MUX_PLAY_INFO("HI_UNF_HDMI_EVENT_HDCP_USERSETTING**********");
			break;
			
		default:
			MUX_PLAY_INFO("HI_UNF_HDMI_EVENT_BUTT**********");
			break;
	}
	
	/* Private Usage */
	if ((g_HDMIUserCallbackFlag == HI_TRUE) && (pfnHdmiUserCallback != NULL))
	{
		pfnHdmiUserCallback(event, NULL);
	}

	return;
}




#ifdef HI_HDCP_SUPPORT
HI_S32 HIADP_HDMI_SetHDCPKey(HI_UNF_HDMI_ID_E enHDMIId)
{
	HI_UNF_HDMI_LOAD_KEY_S stLoadKey;
	FILE *pBinFile;
	HI_U32 u32Len;
	HI_U32 u32Ret;

	pBinFile = fopen(pstencryptedHdcpKey, "rb");
	if(HI_NULL == pBinFile)
	{
		MUX_PLAY_ERROR("can't find key file: %s", pstencryptedHdcpKey);
		return HI_FAILURE;
	}
	fseek(pBinFile, 0, SEEK_END);
	u32Len = ftell(pBinFile);
	fseek(pBinFile, 0, SEEK_SET);

	stLoadKey.u32KeyLength = u32Len; //332
	stLoadKey.pu8InputEncryptedKey  = (HI_U8*)malloc(u32Len);
	if(HI_NULL == stLoadKey.pu8InputEncryptedKey)
	{
		MUX_PLAY_ERROR("malloc for HDCP Key erro!\n");
		fclose(pBinFile);
		return HI_FAILURE;
	}
	if (u32Len != fread(stLoadKey.pu8InputEncryptedKey, 1, u32Len, pBinFile))
	{
		MUX_PLAY_ERROR("read file %d!\n", __LINE__);
		fclose(pBinFile);
		free(stLoadKey.pu8InputEncryptedKey);
		return HI_FAILURE;
	}

	u32Ret = HI_UNF_HDMI_LoadHDCPKey(enHDMIId,&stLoadKey);
	free(stLoadKey.pu8InputEncryptedKey);
	fclose(pBinFile);
	MUX_PLAY_ERROR("Load HDCP Key:%s!!!",u32Ret?"FAILURE":"SUCCESS");

	return u32Ret;
}

#endif

HI_S32 HIADP_HDMI_Init(HI_UNF_HDMI_ID_E enHDMIId)
{
	HI_S32 Ret = HI_FAILURE;
	HI_UNF_HDMI_OPEN_PARA_S stOpenParam;
	HI_UNF_HDMI_DELAY_S  stDelay;

	g_stHdmiArgs.enHdmi       = enHDMIId;

	Ret = HI_UNF_HDMI_Init();
	if (HI_SUCCESS != Ret)
	{
		MUX_PLAY_ERROR("HI_UNF_HDMI_Init failed:%#x",Ret);
		return HI_FAILURE;
	}
	
#ifdef HI_HDCP_SUPPORT
	Ret = HIADP_HDMI_SetHDCPKey(enHDMIId);
	if (HI_SUCCESS != Ret)
	{
		MUX_PLAY_ERROR("Set hdcp erro:%#x",Ret);
		//return HI_FAILURE;
	}
#endif

	HI_UNF_HDMI_GetDelay(HI_UNF_HDMI_ID_0, &stDelay);
	stDelay.bForceFmtDelay = HI_TRUE;
	stDelay.bForceMuteDelay = HI_TRUE;

#if defined(CHIP_TYPE_hi3798mv200_a) || defined(CHIP_TYPE_hi3798mv200)
	stDelay.u32FmtDelay  = 1;
	stDelay.u32MuteDelay = 50;
#else
	stDelay.u32FmtDelay = 500;
	stDelay.u32MuteDelay = 120;
#endif
	HI_UNF_HDMI_SetDelay(0,&stDelay);


	g_stCallbackFunc.pfnHdmiEventCallback = HDMI_Event_Proc;
	g_stCallbackFunc.pPrivateData = &g_stHdmiArgs;

	Ret = HI_UNF_HDMI_RegCallbackFunc(enHDMIId, &g_stCallbackFunc);
	if (Ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("hdmi reg failed:%#x",Ret);
		HI_UNF_HDMI_DeInit();
		return HI_FAILURE;
	}

	stOpenParam.enDefaultMode = g_enDefaultMode;//HI_UNF_HDMI_FORCE_NULL;
	Ret = HI_UNF_HDMI_Open(enHDMIId, &stOpenParam);
	if (Ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("HI_UNF_HDMI_Open failed:%#x",Ret);
		HI_UNF_HDMI_DeInit();
		return HI_FAILURE;
	}

	return HI_SUCCESS;
}


HI_S32 HIADP_HDMI_DeInit(HI_UNF_HDMI_ID_E enHDMIId)
{

//    HI_UNF_HDMI_Stop(enHDMIId);

	HI_UNF_HDMI_Close(enHDMIId);

	HI_UNF_HDMI_UnRegCallbackFunc(enHDMIId, &g_stCallbackFunc);

	HI_UNF_HDMI_DeInit();

	return HI_SUCCESS;
}


