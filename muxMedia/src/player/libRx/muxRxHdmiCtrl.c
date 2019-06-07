/*
 * hdmi_cec.c
 *
 */

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"

typedef	struct
{
	int		type;
	char		*name;	
}_FMT_NAME;


static _FMT_NAME _dispFmtString[] =
{
	{/* 0*/
		.type = HI_UNF_ENC_FMT_1080P_60,
		.name = "1080P_60",
	},
	{
		.type = HI_UNF_ENC_FMT_1080P_50,
		.name = "1080P_50", /**<1080p 50 Hz*/
	},
	{
		.type = HI_UNF_ENC_FMT_1080P_30,
		.name = "1080P_30",/**<1080p 30 Hz*/
	},
	{
		.type = HI_UNF_ENC_FMT_1080P_25,
		.name = "1080P_25",
	},
	{
		.type = HI_UNF_ENC_FMT_1080P_24,
		.name = "1080P_24",
	},


	{/* 5*/
		.type = HI_UNF_ENC_FMT_1080i_60,
		.name = "1080i_60",
	},
	{
		.type = HI_UNF_ENC_FMT_1080i_50,
		.name = "1080i_50",
	},
	{
		.type = HI_UNF_ENC_FMT_720P_60,
		.name = "720P_60",
	},
	{
		.type = HI_UNF_ENC_FMT_720P_50,
		.name = "720P_50",
	},
	{
		.type = HI_UNF_ENC_FMT_576P_50,
		.name = "576P_50",
	},

	{/* 10 */
		.type = HI_UNF_ENC_FMT_480P_60,
		.name = "480P_60",
	},
	
	{
		.type = HI_UNF_ENC_FMT_PAL,
		.name = "PAL",
	},
	{
		.type = HI_UNF_ENC_FMT_PAL_N,
		.name = "PAL_N",
	},
	{
		.type = HI_UNF_ENC_FMT_PAL_Nc,
		.name = "PAL_Nc",
	},
	{
		.type = HI_UNF_ENC_FMT_NTSC,
		.name = "NTSC",
	},

	{/* 15 */
		.type = HI_UNF_ENC_FMT_NTSC_J,
		.name = "NTSC_J",
	},

	
	{
		.type = HI_UNF_ENC_FMT_NTSC_PAL_M,
		.name = "NTSC_PAL_M",
	},
	{
		.type = HI_UNF_ENC_FMT_SECAM_SIN,
		.name = "SECAM_SIN",
	},
	{
		.type = HI_UNF_ENC_FMT_SECAM_COS,
		.name = "SECAM_COS",
	},
	{
		.type = HI_UNF_ENC_FMT_1080P_24_FRAME_PACKING,
		.name = "1080P_24_FP",
	},
	{/* 20 */
		.type = HI_UNF_ENC_FMT_720P_60_FRAME_PACKING,
		.name = "720P_60_FP",
	},

		
	{
		.type = HI_UNF_ENC_FMT_720P_50_FRAME_PACKING,
		.name = "720P_50_FP",
	},
	{
		.type = HI_UNF_ENC_FMT_861D_640X480_60,
		.name = "861D_640X480_60",
	},
	{
		.type = HI_UNF_ENC_FMT_VESA_800X600_60,
		.name = "VESA_800X600_60",
	},
	{
		.type = HI_UNF_ENC_FMT_VESA_1024X768_60,
		.name = "VESA_1024X768_60",
	},
	{/* 25 */
		.type = HI_UNF_ENC_FMT_VESA_1280X720_60,
		.name = "VESA_1280X720_60",
	},

		
	{
		.type = HI_UNF_ENC_FMT_VESA_1280X800_60,
		.name = "VESA_1280X800_60",
	},
	{
		.type = HI_UNF_ENC_FMT_VESA_1280X1024_60,
		.name = "VESA_1280X1024_60",
	},
	{
		.type = HI_UNF_ENC_FMT_VESA_1360X768_60,
		.name = "VESA_1360X768_60",
	},
	{
		.type = HI_UNF_ENC_FMT_VESA_1366X768_60,
		.name = "VESA_1366X768_60",
	},
	{/* 30 */
		.type = HI_UNF_ENC_FMT_VESA_1400X1050_60,
		.name = "VESA_1400X1050_60",
	},

		
	{
		.type = HI_UNF_ENC_FMT_VESA_1440X900_60,
		.name = "VESA_1440X900_60",
	},
	{
		.type = HI_UNF_ENC_FMT_VESA_1440X900_60_RB,
		.name = "VESA_1440X900_60_RB",
	},
	{
		.type = HI_UNF_ENC_FMT_VESA_1600X900_60_RB,
		.name = "VESA_1600X900_60_RB",
	},
	{
		.type = HI_UNF_ENC_FMT_VESA_1600X1200_60,
		.name = "VESA_1600X1200_60",
	},

	{/* 35 */
		.type = HI_UNF_ENC_FMT_VESA_1680X1050_60,
		.name = "VESA_1680X1050_60",
	},

		
	{
		.type = HI_UNF_ENC_FMT_VESA_1680X1050_60_RB,
		.name = "VESA_1680X1050_60_RB",
	},
	{
		.type = HI_UNF_ENC_FMT_VESA_1920X1080_60,
		.name = "VESA_1920X1080_60",
	},
	{
		.type = HI_UNF_ENC_FMT_VESA_1920X1200_60,
		.name = "VESA_1920X1200_60",
	},
	{
		.type = HI_UNF_ENC_FMT_VESA_1920X1440_60,
		.name = "VESA_1920X1440_60",
	},
	{/* 40 */
		.type = HI_UNF_ENC_FMT_VESA_2048X1152_60,
		.name = "VESA_2048X1152_60",
	},


	{/* 41 */
		.type = HI_UNF_ENC_FMT_VESA_2560X1440_60_RB,
		.name = "VESA_2560X1440_60_RB",
	},
	{ /* 42 */
		.type = HI_UNF_ENC_FMT_VESA_2560X1600_60_RB,
		.name = "VESA_2560X1600_60_RB",
	},


	{ /*  0x40, 64 */
		.type = HI_UNF_ENC_FMT_3840X2160_24,
		.name = "3840X2160P_24",
	},

	{/* 65 */
		.type = HI_UNF_ENC_FMT_3840X2160_25,
		.name = "3840X2160P_25",
	},
	{
		.type = HI_UNF_ENC_FMT_3840X2160_30,
		.name = "3840X2160P_30",
	},
	{
		.type = HI_UNF_ENC_FMT_3840X2160_50,
		.name = "3840X2160P_50",
	},
	{
		.type = HI_UNF_ENC_FMT_3840X2160_60,
		.name = "3840X2160P_60",
	},
	{
		.type = HI_UNF_ENC_FMT_4096X2160_24,
		.name = "4090X2160_24",
	},


	{/* 70 */
		.type = HI_UNF_ENC_FMT_4096X2160_25,
		.name = "4090X2160_25",
	},

		
	{
		.type = HI_UNF_ENC_FMT_4096X2160_30,
		.name = "4090X2160_30",
	},
	{
		.type = HI_UNF_ENC_FMT_4096X2160_50,
		.name = "4090X2160_50",
	},
	{
		.type = HI_UNF_ENC_FMT_4096X2160_60,
		.name = "4090X2160_60",
	},
	{
		.type = HI_UNF_ENC_FMT_3840X2160_23_976,
		.name = "3840X2160_23_976",
	},

	{/* 75 */
		.type = HI_UNF_ENC_FMT_3840X2160_29_97,
		.name = "3840X2160_29_97",
	},
	{
		.type = HI_UNF_ENC_FMT_720P_59_94,
		.name = "720P_59_94",
	},
	{
		.type = HI_UNF_ENC_FMT_1080P_59_94,
		.name = "1080P_59_94",
	},
	{
		.type = HI_UNF_ENC_FMT_1080P_29_97,
		.name = "1080P_29_97",
	},
	{
		.type = HI_UNF_ENC_FMT_1080P_23_976,
		.name = "1080P_23_976",
	},

	{/* 80 */
		.type = HI_UNF_ENC_FMT_1080i_59_94,
		.name = "1080i_59_94",
	},
	{ /* 81 */
		.type = HI_UNF_ENC_FMT_BUTT,		/* automatically */
		.name = "Auto"
	},
	
	{ /* 82 */
		.type = HI_FMT_INVALDAITE,
		.name = NULL/**/
	}

};


int muxHdmiFormatCode(HI_CHAR *pszFmt)
{
	int fmtReturn = HI_UNF_ENC_FMT_BUTT;
	_FMT_NAME *_fmt = _dispFmtString;

	if (NULL == pszFmt)
	{
		return HI_UNF_ENC_FMT_BUTT;
	}

	while(_fmt->name != NULL )
	{
		if (strcasestr(pszFmt, _fmt->name) )
		{
			fmtReturn = _fmt->type;
			break;
		}
		
		_fmt++;
	}

	if (fmtReturn >= HI_FMT_INVALDAITE)
	{
		fmtReturn = HI_UNF_ENC_FMT_720P_50;
		MUX_PLAY_WARN("Can NOT match TV format '%s', set format to '%s'", pszFmt, muxHdmiFormatName(fmtReturn) );
	}
	else
	{
//		MUX_PLAY_INFO("The format is '%s'/%d", pszFmt, fmtReturn);
	}
	
	return fmtReturn;
}

char *muxHdmiFormatName(HI_UNF_ENC_FMT_E code)
{
	_FMT_NAME *_fmt = _dispFmtString;
	
	if(code < HI_UNF_ENC_FMT_1080P_60 || code >= HI_FMT_INVALDAITE)
	{
		return "Unknown format";
	}

	while(_fmt->name != NULL )
	{
		if ( _fmt->type == code )
		{
			return _fmt->name;
			break;
		}
		
		_fmt++;
	}

	return "Unknown format";
}


char *muxHdmiHdcpVersion(int version)
{
	switch (version)
	{
		case HI_UNF_HDMI_HDCP_VERSION_NONE:
			return "None";		
			break;
		
		case HI_UNF_HDMI_HDCP_VERSION_HDCP14:
			return "HDCP1.4";
			break;
		
		case HI_UNF_HDMI_HDCP_VERSION_HDCP22:
			return "HDCP2.2";
			break;
		
		default:
			return "UN-KNOWN";
			break;
	}

	return "UN-KNOWN";
}

char *muxHdmiVideoModeName(int mode)
{
	switch (mode)
	{
		case HI_UNF_HDMI_VIDEO_MODE_RGB444:
			return "RGB444";		
			break;
		
		case HI_UNF_HDMI_VIDEO_MODE_YCBCR422:
			return "YCBCR422";
			break;
		
		case HI_UNF_HDMI_VIDEO_MODE_YCBCR444:
			return "YCBCR444";
			break;
		
		case HI_UNF_HDMI_VIDEO_MODE_YCBCR420:
			return "YCBCR420";
			break;
		
		default:
			return "UN-KNOWN";
			break;
	}

	return "UN-KNOWN";
}

char *muxHdmiDeepColorName(int color)
{
	switch (color)
	{
		case HI_UNF_HDMI_DEEP_COLOR_24BIT:
			return "24Bit";		
			break;
		
		case HI_UNF_HDMI_DEEP_COLOR_30BIT:
			return "30Bit";
			break;
		
		case HI_UNF_HDMI_DEEP_COLOR_36BIT:
			return "36Bit";
			break;
		
		case HI_UNF_HDMI_DEEP_COLOR_OFF:
			return "0XFF";
			break;
			
		default:
			return "UN-KNOWN";
			break;
	}

	return "UN-KNOWN";
}



void muxHdmiPrintSinkCap(void)
{
	char		buf[1024*20];
	int 		index = 0;
	int		size = sizeof(buf);
	int		i;
	int ret;
	
	HI_UNF_EDID_BASE_INFO_S sinkAttr;
	memset(buf, 0, size);

	ret = HI_UNF_HDMI_GetSinkCapability(HI_UNF_HDMI_ID_0, &sinkAttr);
	if( ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get HDMI Sink Capability failed: %#x", ret);
		return;
	}

	index +=snprintf(buf+index, size-index, "\t%s Device\n", (sinkAttr.bSupportHdmi)?"HDMI":"AVI");
	index +=snprintf(buf+index, size-index, "\tNative Format: %s\n", muxHdmiFormatName(sinkAttr.enNativeFormat));
	for(i=0; i< HI_UNF_ENC_FMT_BUTT; i++ )
	{
		if(sinkAttr.bSupportFormat[i])
		{
			index +=snprintf(buf+index, size-index, "\t\tFormat No.%d(%d): %s\n", i, HI_UNF_ENC_FMT_BUTT, muxHdmiFormatName((HI_UNF_ENC_FMT_E)i) );
		}
	}
	
	index +=snprintf(buf+index, size-index, "\t3D : %s\n", (sinkAttr.st3DInfo.bSupport3D)?"YES":"NO");
//	index +=snprintf(buf+index, size-index, "\t3D : %s\n", sinkAttr.stDeepColor;            /**<YCBCR/RGB deep color Info*//**<CNcomment:YCBCR/RGB deep color ������*/
//	index +=snprintf(buf+index, size-index, "\t3D : %s\n", sinkAttr.stColorMetry;           /**<colorimetry Info*//**<CNcomment:ɫ�������� */
//	index +=snprintf(buf+index, size-index, "\t3D : %s\n", sinkAttr.stColorSpace;           /**<color space Info*//**<CNcomment:��ɫ�ռ������� */            

	index +=snprintf(buf+index, size-index, "\tNumber of Audio Info : %d\n", sinkAttr.u32AudioInfoNum);
	for(i=0; i< HI_UNF_EDID_MAX_AUDIO_CAP_COUNT; i++)
	{
//		sinkAttr.stAudioInfo[HI_UNF_EDID_MAX_AUDIO_CAP_COUNT];   /**<audio Info*//**<CNcomment:��Ƶ������ */
	}
	
//	sinkAttr.bSupportAudioSpeaker[HI_UNF_EDID_AUDIO_SPEAKER_BUTT];/**<speaker Info*//**<CNcomment:speaker ������ */

	index +=snprintf(buf+index, size-index, "\tedid extend block num Info : %d\n", sinkAttr.u8ExtBlockNum);

	index +=snprintf(buf+index, size-index, "\tManufacture: %s, Code:%d, serial number:%d, week:%d, year:%d, SinkName:%s\n", 
		sinkAttr.stMfrsInfo.u8MfrsName, sinkAttr.stMfrsInfo.u32ProductCode, sinkAttr.stMfrsInfo.u32SerialNumber, sinkAttr.stMfrsInfo.u32Week, sinkAttr.stMfrsInfo.u32Year, sinkAttr.stMfrsInfo.u8pSinkName);
	index +=snprintf(buf+index, size-index, "\tVersion: %d.%d\n", sinkAttr.u8Version, sinkAttr.u8Revision);

	index +=snprintf(buf+index, size-index, "\tCEC Addess: PhyAddValid: %s, %x:%x:%x:%x\n", 
		(sinkAttr.stCECAddr.bPhyAddrValid)?"YES":"NO", sinkAttr.stCECAddr.u8PhyAddrA, sinkAttr.stCECAddr.u8PhyAddrB, sinkAttr.stCECAddr.u8PhyAddrC, sinkAttr.stCECAddr.u8PhyAddrD);

	index +=snprintf(buf+index, size-index, "\tDVI dual-link: %s\n", (sinkAttr.bSupportDVIDual)?"YES":"NO");
	index +=snprintf(buf+index, size-index, "\tSupport AI: %s\n", (sinkAttr.bSupportsAI)?"YES":"NO");
//	index +=snprintf(buf+index, size-index, "\tfirst detailed timing Info: %d\n", sinkAttr.stPerferTiming);         /**<first detailed timing Info*//**<CNcomment:VESA�����ϸ��ʽ��Ϣ */          
	index +=snprintf(buf+index, size-index, "\tMax TMDS clock in MHz: %d\n", sinkAttr.u32MaxTMDSClock);

	for(i=0; i< HI_UNF_ENC_FMT_BUTT; i++ )
	{
		if(sinkAttr.bSupportY420Format[i])
		{
			index +=snprintf(buf+index, size-index, "\t\tFormat: %s, support YCBCR420: YES\n", muxHdmiFormatName((HI_UNF_ENC_FMT_E)i) );
		}
	}

	index +=snprintf(buf+index, size-index, "\tYCBCR deep color Info: 30BIT:%d; 36BIT:%d; 48BIT:%d\n", 
		sinkAttr.stY420DeepColor.bY420DeepColor30Bit, sinkAttr.stY420DeepColor.bY420DeepColor36Bit, sinkAttr.stY420DeepColor.bY420DeepColor48Bit);

	index +=snprintf(buf+index, size-index, "\tDolby: %s;\n", (sinkAttr.bDolbySupport)?"YES":"NO");
//sinkAttr.stDolby;                /**<Dolby capability*//**<CNcomment:Dolby������ */

	index +=snprintf(buf+index, size-index, "\tHDR: %s;\n", (sinkAttr.bHdrSupport)?"YES":"NO");
	
	//nkAttr.stHdr;                  /**<HDR capability*//**<CNcomment:HDR������ */
	
	index +=snprintf(buf+index, size-index, "\tDHCP 1.4: %s;2.2: %s\n", (sinkAttr.stHDCPSupport.bHdcp14Support)?"YES":"NO", (sinkAttr.stHDCPSupport.bHdcp22Support)?"YES":"NO");
	index +=snprintf(buf+index, size-index, "\tScreen : width: %d; height:%d\n", sinkAttr.stBaseDispPara.u8MaxImageWidth, sinkAttr.stBaseDispPara.u8MaxImageHeight);

	printf("\nSinkCapability================\n%s==============================\n", buf);

}

int muxHdmiCecCheckStatus(void )
{
	char		buf[1024*20];
	int 		index = 0;
	int		size = sizeof(buf);
	int		i;
	int		ret;
	HI_UNF_HDMI_CEC_STATUS_S  cecStatus, *status;
	status = &cecStatus;

	ret = HI_UNF_HDMI_CECStatus(HI_UNF_HDMI_ID_0, status);
	if( ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get HDMI CEC status failed: %#x", ret);
		return EXIT_FAILURE;
	}

	index +=snprintf(buf+index, size-index, "\tStatus: %s\n", (status->bEnable)?"Enable":"Disable");
	index +=snprintf(buf+index, size-index, "\tPhy Address: %x:%x:%x:%x\n", status->u8PhysicalAddr[0], status->u8PhysicalAddr[1], status->u8PhysicalAddr[2], status->u8PhysicalAddr[3]);
	index +=snprintf(buf+index, size-index, "\tLogic Address: %d\n", status->u8LogicalAddr );
	index +=snprintf(buf+index, size-index, "\tNetwork: \n");
	for(i=0; i< HI_UNF_CEC_LOGICALADD_BUTT; i++ )
	{
		index +=snprintf(buf+index, size-index, "\t\tNo.%d(%d): %s\n", i, HI_UNF_CEC_LOGICALADD_BUTT, (status->u8Network[i])?"YES":"NO" );
	}

	MUX_PLAY_DEBUG("\nCEC Status==============================\n%s==============================\n", buf);

	return EXIT_SUCCESS;
}




HI_U32 muxHdmiCECCommand(HI_U8 srcAddr, HI_U8 destAddr, HI_U8 u8Opcode, HI_U8 userCode, HI_U8 *data, HI_U8 datalength)
{
	HI_UNF_HDMI_CEC_CMD_S CECCmd;

	//CEC function needs to be enabled in decoder's init procedure
	//HI_UNF_HDMI_CEC_Enable(mux_Hdmi_ID);

	MUX_PLAY_DEBUG("HDMI CEC Command, destAddr:0x%x, Opcode:0x%x, Datalength:0x%x", destAddr, u8Opcode, datalength);
	if(data)
	{
		MUX_PLAY_DEBUG("Data:0x%x:0x%x:0x%x:0x%x", data[0], data[1], data[2], data[3]);
	}

//	usleep(300 * 1000);
	memset(&CECCmd, 0, sizeof(HI_UNF_HDMI_CEC_CMD_S));
	CECCmd.enSrcAdd = srcAddr;//HI_UNF_CEC_LOGICALADD_TUNER_1;
	CECCmd.enDstAdd = destAddr;    //HI_UNF_CEC_LOGICALADD_TV;
	CECCmd.u8Opcode = u8Opcode;
	CECCmd.unOperand.stUIOpcode = userCode;
	CECCmd.unOperand.stRawData.u8Length = datalength;

	if(datalength>0)
	{
		memcpy(&(CECCmd.unOperand.stRawData.u8Data), data, (datalength>15)?15:datalength);
	}

	int ret = HI_UNF_HDMI_SetCECCommand(HI_UNF_HDMI_ID_0, &CECCmd);
	if( ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("set HDMI CEC Command failed: %#x", ret);
		return EXIT_FAILURE;
	}

	muxHdmiCecCheckStatus();
	
	return EXIT_SUCCESS;
	//HI_UNF_HDMI_CEC_Disable(mux_Hdmi_ID);
}

int muxHdmiCECSetDeviceOsdName(void)
{
	int ret;
	unsigned char *name = (unsigned char *)"MuxPlayer";
	
	ret = muxHdmiCECCommand(HI_UNF_CEC_LOGICALADD_TUNER_1, HI_UNF_CEC_LOGICALADD_TV, CEC_OPCODE_SET_OSD_NAME, 0, name, (unsigned char )strlen((char *)name) );
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_WARN("Set Device OSD name failed: 0x%x", ret);
	}

	return ret;
}


/* it is called when TV is plugined */
void muxHdmiReplugMonitor(HI_UNF_ENC_FMT_E enForm)
{
	HI_S32 s32Ret;
	MUX_RX_T  	*muxRx = &_muxRx;

//	if( FMT_AUTO(muxRx->videoFormat) )
	{
		s32Ret = muxHdmiConfigFormat(enForm);
		if (s32Ret != EXIT_SUCCESS)
		{
			MUX_PLAY_ERROR("call HI_UNF_HDMI_CEC_Enable failed.\n");
			return;
		}
	}	

	HI_UNF_EDID_BASE_INFO_S stSinkCap;
	HI_UNF_HDMI_DEEP_COLOR_E deepColor = HI_UNF_HDMI_DEEP_COLOR_BUTT;
	
	if (muxRx->muxPlayer->playerConfig.deepColor >= HI_UNF_HDMI_DEEP_COLOR_24BIT && muxRx->muxPlayer->playerConfig.deepColor <= HI_UNF_HDMI_DEEP_COLOR_36BIT)
	{
		deepColor = muxRx->muxPlayer->playerConfig.deepColor;
		MUX_PLAY_DEBUG("DeepColor configured as %d", deepColor);
	}
	else// if (decoder_state.deep_color == DEEP_COLOR_AUTO)
	{ //AUTO
		s32Ret = HI_UNF_HDMI_GetSinkCapability(HI_UNF_HDMI_ID_0, &stSinkCap);
		if (s32Ret == HI_SUCCESS)
		{
			if (stSinkCap.stDeepColor.bDeepColor36Bit)
			{
				MUX_PLAY_DEBUG("DeepColor supports 36bits");
				deepColor = HI_UNF_HDMI_DEEP_COLOR_36BIT;
			}
			else if (stSinkCap.stDeepColor.bDeepColor30Bit)
			{
				MUX_PLAY_DEBUG("DeepColor supports 30bits");
				deepColor = HI_UNF_HDMI_DEEP_COLOR_30BIT;
			}
			else
			{
				MUX_PLAY_DEBUG("DeepColor supports 24bits");
				deepColor = HI_UNF_HDMI_DEEP_COLOR_24BIT;
			}
		}
		else
		{
			deepColor = HI_UNF_HDMI_DEEP_COLOR_24BIT;
			MUX_PLAY_ERROR("Failed to get EDID information, set deep color as 24bits");
		}
	}

	s32Ret = muxHdmiConfigDeepColor(deepColor);

}


/* called by IP command and when TX is plugin. isConfig: called from IP command */
int muxHdmiConfigDeepColor(HI_UNF_HDMI_DEEP_COLOR_E deepColor)
{
	HI_S32 ret = 0;
	HI_UNF_EDID_BASE_INFO_S stSinkCap;
	HI_UNF_HDMI_DEEP_COLOR_E colorDepth;

	ret = HI_UNF_HDMI_GetSinkCapability(HI_UNF_HDMI_ID_0, &stSinkCap);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get HDMI SinkCapabilty failed: %#x", ret);
		return EXIT_FAILURE;
	}

	if (HI_FALSE == stSinkCap.bSupportHdmi)
	{
		MUX_PLAY_ERROR("HDMI is not support, instead it is DIV");
		return EXIT_FAILURE;
	}
	

	colorDepth = muxHdmiFindNewColorDepth(deepColor, &stSinkCap.stDeepColor);
	
	ret = HI_UNF_HDMI_SetDeepColor(HI_UNF_HDMI_ID_0, colorDepth);
	if( ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("Unable to set deep color %s", muxHdmiDeepColorName(colorDepth) );
		return EXIT_FAILURE;
	}
	else
	{
		HI_UNF_HDMI_DEEP_COLOR_E dc = HI_UNF_HDMI_DEEP_COLOR_BUTT;

		ret = HI_UNF_HDMI_Start(HI_UNF_HDMI_ID_0);
		if (ret != HI_SUCCESS)
		{
			MUX_PLAY_ERROR("HDMI start failed:0x%x.\n", ret);
			return EXIT_FAILURE;
		}

		HI_UNF_HDMI_GetDeepColor(HI_UNF_HDMI_ID_0, &dc);

		MUX_PLAY_DEBUG("After Set DeepColor %s(%s), GetDeepColor:%s", 
			muxHdmiDeepColorName(deepColor), muxHdmiDeepColorName(colorDepth), muxHdmiDeepColorName(dc));
		if(dc != deepColor)
		{
			MUX_PLAY_ERROR("DeepColor %s(%s) has been set, but it is not working now, DeepColor %s is still used", 
				muxHdmiDeepColorName(deepColor), muxHdmiDeepColorName(colorDepth), muxHdmiDeepColorName(dc));
			return EXIT_FAILURE;
		}

		/*
		//for "AUTO", we still save the setting even if setup failed, since in case of 4K60 RGB444, it can only support
		//8 bit, however even if we set it up with 10 bit, the HI_UNF_HDMI_SetDeepColor will still return HI_TRUE
		*/
#if 0		
		if (decoder_state.deep_color != DEEP_COLOR_AUTO && dc != HI_UNF_HDMI_DEEP_COLOR_BUTT)
			decoder_state.deep_color = dc;
#endif		
	}


	return EXIT_SUCCESS;
}

/* called by IP command and when TX is plugin. isConfig: called from IP command. enForm is validate value */
int muxHdmiConfigFormat(HI_UNF_ENC_FMT_E enForm)
{
	HI_S32 s32Ret;
//	char *fmtStr = NULL;


	HI_UNF_ENC_FMT_E fmtTmp = HI_UNF_ENC_FMT_BUTT;
	HI_UNF_DISP_GetFormat(HI_UNF_DISPLAY1, &fmtTmp);

	if(enForm == fmtTmp)
	{
		MUX_PLAY_WARN("HDMI existed resolution has been %s!", muxHdmiFormatName(fmtTmp) );
		return EXIT_SUCCESS;
	}

	
	if( enForm == HI_UNF_ENC_FMT_BUTT)
	{
		MUX_PLAY_ERROR("Format param is invalidate %s", muxHdmiFormatName(enForm));
		//continue;
		return EXIT_FAILURE;
	}

	s32Ret = HI_UNF_DISP_SetFormat(HI_UNF_DISPLAY1, enForm);
	if (s32Ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("SetFormat %s on DISPLAY failed 0x%x", muxHdmiFormatName(enForm), s32Ret);
		//continue;
		return EXIT_FAILURE;
	}
	else
	{

		s32Ret = HI_UNF_HDMI_Start(HI_UNF_HDMI_ID_0);
		if (s32Ret != HI_SUCCESS)
		{
			MUX_PLAY_ERROR("HDMI start failed:0x%x.\n", s32Ret);
			return EXIT_FAILURE;
		}

		MUX_PLAY_DEBUG("SetFormat %s on DISPLAY success", muxHdmiFormatName(enForm) );
		HI_UNF_DISP_GetFormat(HI_UNF_DISPLAY1, &fmtTmp);
		if(enForm != fmtTmp)
		{
			MUX_PLAY_ERROR("Format %s has been set, but it is not working, format %s is still used", 
				muxHdmiFormatName(enForm), muxHdmiFormatName(fmtTmp) );
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}


/* Added. 06.03, 2019 */
/* called by IP command and when TX is plugin. isConfig: called from IP command */
int muxHdmiConfigColorSpace(HI_UNF_HDMI_VIDEO_MODE_E colorSpace)
{
	HI_S32 s32Ret;

	HI_UNF_HDMI_ATTR_S stHdmiAttr;
	HI_UNF_EDID_BASE_INFO_S stSinkCap;
	HI_UNF_HDMI_STATUS_S stHdmiStatus;
	HI_UNF_HDMI_VIDEO_MODE_E tmpColorSpace;

	/* check HDMI status */
	s32Ret = HI_UNF_HDMI_GetStatus(HI_UNF_HDMI_ID_0, &stHdmiStatus);
	if(s32Ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get HDMI status failed: %#x", s32Ret);
	}

	if (HI_FALSE == stHdmiStatus.bConnected)
	{
		MUX_PLAY_ERROR("No HDMI Connect");
		return EXIT_FAILURE;
	}

	s32Ret= HI_UNF_HDMI_GetAttr(HI_UNF_HDMI_ID_0, &stHdmiAttr);
	if(s32Ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get HDMI Attr failed: %#x", s32Ret);
		return EXIT_FAILURE;
	}
	
	s32Ret = HI_UNF_HDMI_GetSinkCapability(HI_UNF_HDMI_ID_0, &stSinkCap);
	if(s32Ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get HDMI SinkCapabilty failed: %#x", s32Ret);
		return EXIT_FAILURE;
	}

	if (HI_FALSE == stSinkCap.bSupportHdmi)
	{
		MUX_PLAY_ERROR("HDMI is not support, instead it is DIV");
		return EXIT_FAILURE;
	}
	
	stHdmiAttr.bEnableHdmi = HI_TRUE;

	tmpColorSpace = muxHdmiFindNewColorSpace(colorSpace, &stSinkCap.stColorSpace);
	stHdmiAttr.enVidOutMode = tmpColorSpace;
	

	s32Ret = HI_UNF_HDMI_SetAttr(HI_UNF_HDMI_ID_0, &stHdmiAttr);
	if (s32Ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("HDMI SetAttr failed 0x%x", s32Ret);
		//continue;
		return EXIT_FAILURE;
	}
	else
	{
		MUX_PLAY_DEBUG("HDMI Set Color Space into %s(%s)" , 
			muxHdmiVideoModeName(tmpColorSpace), muxHdmiVideoModeName(colorSpace));
	}

#if 1
	/* HI_UNF_HDMI_SetAttr must before HI_UNF_HDMI_Start! */
	s32Ret = HI_UNF_HDMI_Start(HI_UNF_HDMI_ID_0);
	if (s32Ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("HDMI Start failed 0x%x", s32Ret);
		//continue;
		return EXIT_FAILURE;
	}
	else
	{
		MUX_PLAY_DEBUG("HDMI Start success");
	}
#endif

	HI_UNF_HDMI_GetAttr(HI_UNF_HDMI_ID_0, &stHdmiAttr);
	MUX_PLAY_DEBUG("Color Space is %s", muxHdmiVideoModeName(stHdmiAttr.enVidOutMode));
	if(stHdmiAttr.enVidOutMode != tmpColorSpace)
	{
		MUX_PLAY_ERROR("Color Space %s has been set, but it is not working, Color Space %sis still used" , 
			muxHdmiVideoModeName(colorSpace), muxHdmiVideoModeName(stHdmiAttr.enVidOutMode) );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}


HI_UNF_HDMI_DEEP_COLOR_E muxHdmiFindNewColorDepth(HI_UNF_HDMI_DEEP_COLOR_E colorDepthCfg, HI_UNF_EDID_DEEP_COLOR_S *sinkCapColorDepthes)
{
	HI_UNF_HDMI_DEEP_COLOR_E outMode = HI_UNF_HDMI_DEEP_COLOR_24BIT; /* default */
	
	if(colorDepthCfg == HI_UNF_HDMI_DEEP_COLOR_BUTT)
	{/* automatic */
		if (HI_TRUE == sinkCapColorDepthes->bDeepColorY444)
		{
			outMode = HI_UNF_HDMI_DEEP_COLOR_24BIT;
		}
		else if (HI_TRUE == sinkCapColorDepthes->bDeepColor30Bit)
		{
			outMode = HI_UNF_HDMI_DEEP_COLOR_30BIT;
		}
		else if (HI_TRUE == sinkCapColorDepthes->bDeepColor36Bit)
		{
			outMode = HI_UNF_HDMI_DEEP_COLOR_36BIT;
		}
		else if (HI_TRUE == sinkCapColorDepthes->bDeepColor48Bit)
		{/* 48 bit is not support */
			outMode = HI_UNF_HDMI_DEEP_COLOR_24BIT;
		}
		else
		{
			outMode = HI_UNF_HDMI_DEEP_COLOR_24BIT;
			MUX_PLAY_WARN("No ColorDepth is read from TV, so default %s is selected", muxHdmiDeepColorName(outMode) );
		}
	}
	else
	{
		if ( (HI_TRUE == sinkCapColorDepthes->bDeepColorY444) && (colorDepthCfg == HI_UNF_HDMI_DEEP_COLOR_24BIT) )
		{
			outMode = HI_UNF_HDMI_DEEP_COLOR_24BIT;
		}
		else if ((HI_TRUE == sinkCapColorDepthes->bDeepColor30Bit) && (colorDepthCfg == HI_UNF_HDMI_DEEP_COLOR_30BIT) )
		{
			outMode = HI_UNF_HDMI_DEEP_COLOR_30BIT;
		}
		else if (( HI_TRUE == sinkCapColorDepthes->bDeepColor36Bit) && (colorDepthCfg == HI_UNF_HDMI_DEEP_COLOR_36BIT) )
		{
			outMode = HI_UNF_HDMI_DEEP_COLOR_36BIT;
		}
		else if (( HI_TRUE == sinkCapColorDepthes->bDeepColor48Bit) ) //&& (colorDepthCfg == HI_UNF_HDMI_VIDEO_MODE_YCBCR420) )
		{
			outMode = HI_UNF_HDMI_DEEP_COLOR_24BIT;
		}
		else
		{
			outMode = HI_UNF_HDMI_DEEP_COLOR_24BIT;
			MUX_PLAY_WARN("ColorDepth %s is not support by TV, so default %s is selected",muxHdmiDeepColorName(colorDepthCfg), muxHdmiDeepColorName(outMode) );
		}
	}

	return outMode;
}


HI_UNF_HDMI_VIDEO_MODE_E muxHdmiFindNewColorSpace(HI_UNF_HDMI_VIDEO_MODE_E colorSpaceCfg, HI_UNF_EDID_COLOR_SPACE_S *sinkCapColorSpace)
{
	HI_UNF_HDMI_VIDEO_MODE_E outMode = HI_UNF_HDMI_VIDEO_MODE_RGB444; /* default */
	
	if(colorSpaceCfg == HI_UNF_HDMI_VIDEO_MODE_BUTT)
	{/* automatic */
		if (HI_FALSE != sinkCapColorSpace->bRGB444)
		{
			outMode = HI_UNF_HDMI_VIDEO_MODE_RGB444;
		}
		else if (HI_FALSE != sinkCapColorSpace->bYCbCr444)
		{
			outMode = HI_UNF_HDMI_VIDEO_MODE_YCBCR444;
		}
		else if (HI_FALSE !=  sinkCapColorSpace->bYCbCr422)
		{
			outMode = HI_UNF_HDMI_VIDEO_MODE_YCBCR422;
		}
		else if (HI_FALSE !=  sinkCapColorSpace->bYCbCr420)
		{
			outMode = HI_UNF_HDMI_VIDEO_MODE_YCBCR420;
		}
		else
		{
			outMode = HI_UNF_HDMI_VIDEO_MODE_RGB444;
			MUX_PLAY_WARN("No ColorSpace is read from TV, so default %s is selected", muxHdmiVideoModeName(outMode) );
		}
	}
	else
	{
		if ( (HI_FALSE !=  sinkCapColorSpace->bRGB444) && (colorSpaceCfg == HI_UNF_HDMI_VIDEO_MODE_RGB444) )
		{
			outMode = HI_UNF_HDMI_VIDEO_MODE_RGB444;
		}
		else if ((HI_FALSE !=  sinkCapColorSpace->bYCbCr444) && (colorSpaceCfg == HI_UNF_HDMI_VIDEO_MODE_YCBCR444) )
		{
			outMode = HI_UNF_HDMI_VIDEO_MODE_YCBCR444;
		}
		else if (( HI_FALSE !=  sinkCapColorSpace->bYCbCr422) && (colorSpaceCfg == HI_UNF_HDMI_VIDEO_MODE_YCBCR422) )
		{
			outMode = HI_UNF_HDMI_VIDEO_MODE_YCBCR422;
		}
		else if (( HI_FALSE !=  sinkCapColorSpace->bYCbCr420) && (colorSpaceCfg == HI_UNF_HDMI_VIDEO_MODE_YCBCR420) )
		{
			outMode = HI_UNF_HDMI_VIDEO_MODE_YCBCR420;
		}
		else
		{
			outMode = HI_UNF_HDMI_VIDEO_MODE_RGB444;
			MUX_PLAY_WARN("ColorSpace %s is not support by TV, so default %s is selected", muxHdmiVideoModeName(colorSpaceCfg), muxHdmiVideoModeName(outMode) );
		}
	}

	return outMode;
}


HI_UNF_ENC_FMT_E _itemResolution(HI_CHAR * res, HI_U32 i)
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


HI_UNF_ENC_FMT_E	_getHdmiResolution(HI_UNF_EDID_BASE_INFO_S *hdmiSinkCap)
{
	HI_UNF_ENC_FMT_E resultFormat = HI_UNF_ENC_FMT_BUTT;
	HI_UNF_EDID_TIMING_S *pt = &(hdmiSinkCap->stPerferTiming);

	HI_U32 hh = pt->u32HACT + pt->u32HBB + pt->u32HFB;
	HI_U32 vv = pt->u32VACT + pt->u32VBB + pt->u32VFB;
	HI_U32 fps = 0;
	if (hh && vv)
	{
		char res[20];
		char interlaced = pt->bInterlace ? 'i' : 'p';

		fps = (HI_U32) (pt->u32PixelClk * 1000.0 / hh / vv + 0.5);

		if (pt->u32HACT == 1920 && pt->u32VACT == 1080)
		{
			sprintf(res, "%dx%d%c", pt->u32HACT, pt->u32VACT, interlaced);
		}
		else
		{
			sprintf(res, "%dx%d", pt->u32HACT, pt->u32VACT);
		}

		MUX_PLAY_DEBUG("Check profile from EDID: fps:%d; resolution:%s; mode:%c", fps, res, interlaced);

		HI_UNF_ENC_FMT_E enFormat = _itemResolution(res, fps);
		MUX_PLAY_DEBUG("calculated resolution is :%s!", muxHdmiFormatName(enFormat) );
		if (enFormat == HI_UNF_ENC_FMT_BUTT)
		{
			resultFormat = hdmiSinkCap->enNativeFormat;
			MUX_PLAY_ERROR("Failed to get preferred resolution[%s] from PTB\n", res);
		}
		else
		{
			resultFormat = enFormat;
		}
	}
	else
	{
		resultFormat = hdmiSinkCap->enNativeFormat;
		MUX_PLAY_WARN("No H/V params in EDDI, set as native format", muxHdmiFormatName(resultFormat) );
	}

	MUX_PLAY_DEBUG("select resolution is :%s!", muxHdmiFormatName(resultFormat) );

	return resultFormat;
}

int muxHdmiGetAutoConfig(MUX_HDMI_CFG_T *autoCfg)
{
	HI_S32 ret;
	HI_UNF_HDMI_STATUS_S hdmiStatus;
	HI_UNF_EDID_BASE_INFO_S hdmiSinkCap;
	
	ret = HI_UNF_HDMI_GetStatus(HI_UNF_HDMI_ID_0, &hdmiStatus);
	if(ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("get HDMI status failed: %#x", ret);
		return EXIT_FAILURE;
	}

	if (HI_FALSE == hdmiStatus.bConnected)
	{
		MUX_PLAY_ERROR("No HDMI Connect");
		return EXIT_FAILURE;
	}

	ret = HI_UNF_HDMI_GetSinkCapability(HI_UNF_HDMI_ID_0, &hdmiSinkCap );
	if (ret != HI_SUCCESS)
	{
		MUX_PLAY_ERROR("HDMI Sink Capacity failed: 0x%x", ret);
		return EXIT_FAILURE;
	}

	if (HI_TRUE != hdmiSinkCap.bSupportHdmi)
	{
		MUX_PLAY_ERROR("Sink Capacity not support HDMI, only DVI supported");
		return EXIT_FAILURE;
	}


	autoCfg->format = _getHdmiResolution(&hdmiSinkCap);
	autoCfg->colorSpace = muxHdmiFindNewColorSpace(HI_UNF_HDMI_DEEP_COLOR_BUTT, &hdmiSinkCap.stColorSpace);

	autoCfg->colorDepth = muxHdmiFindNewColorDepth(HI_UNF_HDMI_DEEP_COLOR_BUTT, &hdmiSinkCap.stDeepColor);;

	if (HI_TRUE == hdmiSinkCap.bDolbySupport)
	{
		autoCfg->hdrType = HI_UNF_DISP_HDR_TYPE_DOLBY;
	}
	else if (HI_TRUE == hdmiSinkCap.bHdrSupport)
	{
		autoCfg->hdrType = HI_UNF_DISP_HDR_TYPE_HDR10;
	}
	else
	{
		autoCfg->hdrType = HI_UNF_DISP_HDR_TYPE_NONE;
	}

	return EXIT_SUCCESS;
}

