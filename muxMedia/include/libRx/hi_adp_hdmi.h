#ifndef __SAMPLE_HDMI_COMMON_H__
#define __SAMPLE_HDMI_COMMON_H__

#include "hi_unf_hdmi.h"
#include <string.h>

extern	HI_UNF_ENC_FMT_E favourite_format;

typedef void (*User_HDMI_CallBack)(HI_UNF_HDMI_EVENT_TYPE_E event, HI_VOID *pPrivateData);

HI_S32 HIADP_HDMI_Init(HI_UNF_HDMI_ID_E enHDMIId);
HI_S32 HIADP_HDMI_DeInit(HI_UNF_HDMI_ID_E enHDMIId);
//HI_S32 HIADP_HDMI_SetAdecAttr(HI_UNF_SND_INTERFACE_E enInterface, HI_UNF_SAMPLE_RATE_E enRate);
HI_VOID HDMI_PrintSinkCap(HI_UNF_EDID_BASE_INFO_S *pCapbility);

/* add 02.15,2018 */
HI_UNF_ENC_FMT_E stringToUnfFmt(HI_CHAR *pszFmt);
HI_UNF_ENC_FMT_E itemResolution(HI_CHAR * res, HI_U32 i);

#endif /* #if pub_HDMI_H_ */

