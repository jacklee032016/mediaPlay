Update of IP Commands
###################################


Text banner size and alignment
=======================================

Modify size and position of banner box
::

    apiClient -c osdPosition -o index=1,left=260,top=50,width=1400,height=220


Show text message and assign the aligment of horizontal and vertical:

::

    apiClient -c alert -o color='COLOR(0xFFRRGGBB)',media='text message', halign=1(left)|2(center)|3(right), valign=1(up)|2(center)|3(bottom)


Lookup capacity of HDMI sink device
=======================================
Read capacity from HDMI sink device, and show current configuration of Resolution, Color Depth and Color Space

::

    apiClient -c cecInfo

Sample of output:

::
	{
		"comment":	"Play one local media file or playlist",
		"targ":	"11:11:11:11:11:11",
		"cmd":	"play_media",
		"login_ack":	"OK",
		"pwd_msg":	"OK",
		"data":	[{
				"comment":	"CEC Command: retrive TV Info",
				"action":	"cecInfo",
				"objects":	[{
						"Manufacture":	"HAI",
						"NativeFormat":	"PAL",
						"Formats":	["1080P_60", "1080P_50", "1080i_60", "1080i_50", "720P_60", "720P_50", "576P_50", "480P_60", "PAL", "NTSC", "VESA_800X600_60", "VESA_1024X768_60"],
						"ColorSpaces":	[{
								"RGB444":	true,
								"YCBCR422":	true,
								"YCBCR444":	true,
								"YCBCR420":	false
							}],
						"ColorDepthes":	[{
								"8":	false,
								"10":	false,
								"12":	false,
								"16":	false
							}],
						"Current":	{
							"Format":	"720P_60",
							"ColorSpace":	"RGB444",
							"ColorDepth":	"24Bit"
						}
					}],
				"status":	200
			}]
	}



Set and save Color Space
=======================================
Configure Color Space of HDMI Sink device; if success, then new configuration is saved;

::

   apiClient -c colorSpace –o o color='0|1|2|3|100' # for RGB444|YCBCR422|YCBCR444|YCBCR420|auto respectively
   
When 100(Auto) is selected, Color Space is used in following order:
* RGB444
* YCBCR444
* YCBCR422
* YCBCR420


Set and save Color Depth
=======================================
Configure Color Depth of HDMI Sink device; if success, then new configuration is saved;

::

   apiClient -c color –o o color='0|1|2|100' # for 8|10|12|auto bits color depth respectively'

When 100(Auto) is selected, depth is used in following order:
* 24BIT
* 30BIT
* 36BIT2



Set and save Resolution
=======================================
Configure resolution of HDMI Sink device; if success, then new configuration is saved;

::

   apiClient -c resolution –o resolution=$RESOLUTION

		"1080P_60",
		"1080P_50", /**<1080p 50 Hz*/
		"1080P_30",/**<1080p 30 Hz*/
		"1080P_25",
		"1080P_24",
		"1080i_60",
		"1080i_50",
		"720P_60",
		"720P_50",
		"576P_50",
		"480P_60",
		"PAL",
		"PAL_N",
		"PAL_Nc",
		"NTSC",
		"NTSC_J",
		"NTSC_PAL_M",
		"SECAM_SIN",
		"SECAM_COS",
		"1080P_24_FP",
		"720P_60_FP",
		"720P_50_FP",
		"861D_640X480_60",
		"VESA_800X600_60",
		"VESA_1024X768_60",
		"VESA_1280X720_60",
		"VESA_1280X800_60",
		"VESA_1280X1024_60",
		"VESA_1360X768_60",
		"VESA_1366X768_60",
		"VESA_1400X1050_60",
		.type = HI_UNF_ENC_FMT_VESA_1440X900_60,
		"VESA_1440X900_60_RB",
		"VESA_1600X900_60_RB",
		"VESA_1600X1200_60",
		"VESA_1680X1050_60",
		"VESA_1680X1050_60_RB",
		"VESA_1920X1080_60",
		"VESA_1920X1200_60",
		"VESA_1920X1440_60",
		"VESA_2048X1152_60",
		"VESA_2560X1440_60_RB",
		"VESA_2560X1600_60_RB",
		"3840X2160P_24",
		"3840X2160P_25",
		"3840X2160P_30",
		"3840X2160P_50",
		"3840X2160P_60",
		"4090X2160_24",
		"4090X2160_25",
		"4090X2160_30",
		"4090X2160_50",
		"4090X2160_60",
		"3840X2160_23_976",
		"3840X2160_29_97",
		"720P_59_94",
		"1080P_59_94",
		"1080P_29_97",
		"1080P_23_976",
		"1080i_59_94",
		"Auto"
