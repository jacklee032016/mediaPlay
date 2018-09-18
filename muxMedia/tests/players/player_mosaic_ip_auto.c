/******************************************************************************

  Copyright (C)

 ******************************************************************************
  File Name     :
  Version       :
  Author        :
  Created       :
  Description   :
  History       :
  1.Date        :
    Author      :
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <assert.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#include "hi_unf_common.h"
#include "hi_unf_avplay.h"
#include "hi_unf_sound.h"
#include "hi_unf_disp.h"
#include "hi_unf_vo.h"
#include "hi_unf_demux.h"
#include "hi_unf_hdmi.h"
#include "hi_unf_ecs.h"
#include "hi_adp_mpi.h"
#include "hi_adp_tuner.h"

#ifdef CONFIG_SUPPORT_CA_RELEASE
#define sample_printf
#else
#define sample_printf printf
#endif

#define AVPLAY_NUM  4
#define AVPLAY_NUM_MAX      16
#define AUD_MIX

#define UDP_RECV_MEM_MAX (1 * 1024 * 1024)

static pthread_t   g_SocketThd;
HI_CHAR            *g_pMultiAddr;
HI_U16             g_UdpPort;
HI_HANDLE          g_TsBuf;


PMT_COMPACT_TBL    *g_pProgTbl = HI_NULL;
static HI_BOOL g_StopSocketThread = HI_FALSE;

HI_VOID SocketThread(HI_VOID *args)
{
    HI_S32              SocketFd;
    struct sockaddr_in  ServerAddr;
    in_addr_t           IpAddr;
    struct ip_mreq      Mreq;
    HI_U32              AddrLen;

    HI_UNF_STREAM_BUF_S     StreamBuf;
    HI_U32              ReadLen;
    HI_U32              GetBufCount=0;
    HI_U32              ReceiveCount=0;
    HI_S32              Ret;
    HI_S32              optVal = UDP_RECV_MEM_MAX;
    socklen_t           optLen = sizeof(HI_S32);

    SocketFd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if (SocketFd < 0)
	{
		sample_printf("create socket error [%d].\n", errno);
		return;
	}

    ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	ServerAddr.sin_port = htons(g_UdpPort);

    if (bind(SocketFd,(struct sockaddr *)(&ServerAddr),sizeof(struct sockaddr_in)) < 0)
	{
		sample_printf("socket bind error [%d].\n", errno);
		close(SocketFd);
		return;
	}

    IpAddr = inet_addr(g_pMultiAddr);
	if (IpAddr)
	{
		Mreq.imr_multiaddr.s_addr = IpAddr;
		Mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(SocketFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &Mreq, sizeof(struct ip_mreq)))
		{
			sample_printf("Socket setsockopt ADD_MEMBERSHIP error [%d].\n", errno);
			close(SocketFd);
			return;
		}
	}
	Ret = setsockopt(SocketFd, SOL_SOCKET, SO_RCVBUF, (HI_CHAR*)&optVal, optLen);
	if (0 != Ret)
	{
		sample_printf("setsockopt error 0x%x", Ret);
	}

    AddrLen = sizeof(ServerAddr);

    while (!g_StopSocketThread)
    {
        Ret = HI_UNF_DMX_GetTSBuffer(g_TsBuf, 188*50, &StreamBuf, 0);
        if (Ret != HI_SUCCESS)
		{
            GetBufCount++;
            if(GetBufCount >= 10)
            {
                sample_printf("########## TS come too fast! #########, Ret=%d\n", Ret);
        	    GetBufCount=0;
            }

            usleep(5000) ;
			continue;
		}
		GetBufCount=0;

        ReadLen = recvfrom(SocketFd, StreamBuf.pu8Data, StreamBuf.u32Size, 0,
		                   (struct sockaddr *)&ServerAddr, &AddrLen);
		if (ReadLen <= 0)
		{
		    ReceiveCount++;
		    if (ReceiveCount >= 50)
		    {
			    sample_printf("########## TS come too slow or net error! #########\n");
			    ReceiveCount = 0;
		    }
		}
        else
		{
            ReceiveCount = 0;
			Ret = HI_UNF_DMX_PutTSBuffer(g_TsBuf, ReadLen);
		    if (Ret != HI_SUCCESS )
            {
                sample_printf("call HI_UNF_DMX_PutTSBuffer failed.\n");
            }
		}
    }

    close(SocketFd);
    return;
}

HI_S32 main(HI_S32 argc,HI_CHAR *argv[])
{
    HI_S32                  Ret,i;
    HI_HANDLE               hWin[AVPLAY_NUM_MAX];
	HI_U32                 	DmxId = 0;
	HI_UNF_DMX_PORT_E       RamPort = HI_UNF_DMX_PORT_RAM_0;
    HI_HANDLE               hAvplay[AVPLAY_NUM_MAX];
    HI_UNF_AVPLAY_ATTR_S        AvplayAttr;
    HI_UNF_SYNC_ATTR_S          SyncAttr;
    HI_UNF_AVPLAY_STOP_OPT_S    Stop;
    HI_CHAR                 InputCmd[32];
	 HI_UNF_ENC_FMT_E            VidFormat = HI_UNF_ENC_FMT_1080P_30;
    HI_U32                  ProgNum;
    HI_RECT_S               stWinRect;
    HI_UNF_AVPLAY_OPEN_OPT_S stVidOpenOpt;
    HI_BOOL                  bAudPlay[AVPLAY_NUM];

    HI_HANDLE                hTrack;
    HI_UNF_AUDIOTRACK_ATTR_S stTrackAttr;

    HI_U32                      winNum = 9;	//default num of mosaic
    HI_U32 						num_per_line = 3;	//default window number per line

    if (argc < 3)
    {
        sample_printf("Usage: %s MultiAddr UdpPort [Output_format:1080P_60...] [Num windows:1..16]\n",argv[0]);
        return HI_FAILURE;
    }

    g_pMultiAddr = argv[1];
	g_UdpPort = atoi(argv[2]);

    if (argc >= 4)
    {
        VidFormat = HIADP_Disp_StrToFmt(argv[3]);
    }

    if(argc >= 5)
    {
    	winNum = atoi(argv[4]);
    	//1..4:2	5..9:4	10..16:4 ...
    	if(winNum == 1)
    	{
    		num_per_line = 2;
    	}
    	else if(winNum>1 && winNum<=AVPLAY_NUM_MAX)
    	{
    		num_per_line = sqrt(winNum-1) + 1;
    	}
    	else
    	{
    		 sample_printf("Number of mosaic should be 1,2...%d \n", AVPLAY_NUM_MAX);
    		return -1;
    	}
    }


    for (i = 0; i < AVPLAY_NUM_MAX; i++)
    {
        hWin[i]     = HI_INVALID_HANDLE;
        hAvplay[i]  = HI_INVALID_HANDLE;
    }
    HI_SYS_Init();

    HIADP_MCE_Exit();
    
    Ret = HIADP_Snd_Init();
    if (HI_SUCCESS != Ret)
    {
        sample_printf("call HIADP_Snd_Init failed.\n");
        goto SYS_DEINIT;
    }

    Ret = HIADP_Disp_Init(VidFormat);
    if (HI_SUCCESS != Ret)
    {
        sample_printf("call HIADP_Disp_Init failed.\n");
        goto SND_DEINIT;
    }

    Ret = HIADP_VO_Init(HI_UNF_VO_DEV_MODE_MOSAIC);
    if (HI_SUCCESS != Ret)
    {
        sample_printf("call HIADP_VO_Init failed.\n");
        goto DISP_DEINIT;
    }

    for (i=0; i < winNum; i++)
    {
		stWinRect.s32Width    = 1920 / num_per_line;
        stWinRect.s32Height   = 1080 / num_per_line;
        stWinRect.s32X = i % num_per_line * stWinRect.s32Width;
        stWinRect.s32Y = i / num_per_line * stWinRect.s32Height;

        //Layout:A large window + some small windows
        if(i == 0)
        {
        	stWinRect.s32Width    = 1920*(num_per_line-1)/num_per_line;
        	stWinRect.s32Height   = 1080*(num_per_line-1)/num_per_line;
        	stWinRect.s32X = 0;
        	stWinRect.s32Y = 0;
        }

        sample_printf("x=%d, y=%d, w=%d, h=%d\n", stWinRect.s32X, stWinRect.s32Y, stWinRect.s32Width, stWinRect.s32Height);
		
        Ret = HIADP_VO_CreatWin(&stWinRect,&hWin[i]);
        if (Ret != HI_SUCCESS)
        {
            sample_printf("call WinInit failed.\n");
            goto VO_DEINIT;
        }

        //Layout: A large window + some small windows
       	HI_UNF_VO_SetWindowZorder(hWin[0], HI_LAYER_ZORDER_MOVETOP);

    }
    Ret = HI_UNF_DMX_Init();
    Ret |= HI_UNF_DMX_AttachTSPort(DmxId,RamPort);
    if (HI_SUCCESS != Ret)
    {
        sample_printf("call HIADP_Demux_Init failed.\n");
        goto VO_DEINIT;
    }
    
    Ret = HI_UNF_DMX_CreateTSBuffer(RamPort, 0x800000, &g_TsBuf);
    if (Ret != HI_SUCCESS)
    {
        sample_printf("call HI_UNF_DMX_CreateTSBuffer failed.\n");
        goto DMX_DEINIT;
    }

    g_StopSocketThread = HI_FALSE;    
    pthread_create(&g_SocketThd, HI_NULL, (HI_VOID *)SocketThread, HI_NULL);

    HIADP_Search_Init();
    Ret = HIADP_Search_GetAllPmt(DmxId,&g_pProgTbl);
    if (HI_SUCCESS != Ret)
    {
        sample_printf("call HIADP_Search_GetAllPmt failed.\n");
        goto PSISI_FREE;
    }
    
    Ret = HIADP_AVPlay_RegADecLib();
    if (HI_SUCCESS != Ret)
    {
        sample_printf("call HIADP_AVPlay_RegADecLib failed.\n");
        goto PSISI_FREE;
    }
    Ret = HI_UNF_AVPLAY_Init();
    if (Ret != HI_SUCCESS)
    {
        sample_printf("call HI_UNF_AVPLAY_Init failed.\n");
        goto PSISI_FREE;
    }

    for ( i = 0 ; i < winNum ; i++ )
    {
        sample_printf("===============avplay[%d]===============\n",i);
        HI_UNF_AVPLAY_GetDefaultConfig(&AvplayAttr, HI_UNF_AVPLAY_STREAM_TYPE_TS);
		AvplayAttr.u32DemuxId = DmxId;
        AvplayAttr.stStreamAttr.u32VidBufSize = 0x800000;
        Ret = HI_UNF_AVPLAY_Create(&AvplayAttr, &hAvplay[i]);
        if (Ret != HI_SUCCESS)
        {
            sample_printf("call HI_UNF_AVPLAY_Create failed.\n");
            goto AVPLAY_STOP;
        }

        stVidOpenOpt.enDecType = HI_UNF_VCODEC_DEC_TYPE_NORMAL;
		stVidOpenOpt.enCapLevel = HI_UNF_VCODEC_CAP_LEVEL_4096x2160;
        stVidOpenOpt.enProtocolLevel = HI_UNF_VCODEC_PRTCL_LEVEL_H264;
        Ret = HI_UNF_AVPLAY_ChnOpen(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID, &stVidOpenOpt);
        if (HI_SUCCESS != Ret)
        {
            sample_printf("call HI_UNF_AVPLAY_ChnOpen failed.\n");
            goto AVPLAY_STOP;
        }

        Ret = HI_UNF_VO_AttachWindow(hWin[i], hAvplay[i]);
        if (HI_SUCCESS != Ret)
        {
            sample_printf("call HI_UNF_VO_AttachWindow failed.\n");
            goto AVPLAY_STOP;
        }

        Ret = HI_UNF_VO_SetWindowEnable(hWin[i], HI_TRUE);
        if (HI_SUCCESS != Ret)
        {
            sample_printf("call HI_UNF_VO_SetWindowEnable failed.\n");
            goto AVPLAY_STOP;
        }
        
        bAudPlay[i] = HI_FALSE;
        if (0 == i)
        {
            bAudPlay[i] = HI_TRUE;
            Ret = HI_UNF_AVPLAY_ChnOpen(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_AUD, HI_NULL);
            stTrackAttr.enTrackType = HI_UNF_SND_TRACK_TYPE_MASTER;
            Ret = HI_UNF_SND_GetDefaultTrackAttr(HI_UNF_SND_TRACK_TYPE_MASTER, &stTrackAttr);
            if (Ret != HI_SUCCESS)
            {
                sample_printf("call HI_UNF_SND_GetDefaultTrackAttr failed.\n");
                goto AVPLAY_STOP;
            }
            Ret |= HI_UNF_SND_CreateTrack(HI_UNF_SND_0,&stTrackAttr,&hTrack);
            Ret |= HI_UNF_SND_Attach(hTrack, hAvplay[i]);
            if (Ret != HI_SUCCESS)
            {
                sample_printf("call HI_UNF_SND_Attach failed.\n");
                goto AVPLAY_STOP;
            }
        }

        Ret = HI_UNF_AVPLAY_GetAttr(hAvplay[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
        SyncAttr.enSyncRef = HI_UNF_SYNC_REF_NONE; /*HI_UNF_SYNC_REF_AUDIO*/
        Ret |= HI_UNF_AVPLAY_SetAttr(hAvplay[i], HI_UNF_AVPLAY_ATTR_ID_SYNC, &SyncAttr);
        if (Ret != HI_SUCCESS)
        {
            sample_printf("call HI_UNF_AVPLAY_SetAttr failed.\n");
            goto AVPLAY_STOP;
        }

        //ProgNum = 0;
        Ret = HIADP_AVPlay_PlayProg(hAvplay[i], g_pProgTbl, i%g_pProgTbl->prog_num,bAudPlay[i]);
        if (HI_SUCCESS != Ret)
        {
            sample_printf("call HIADP_AVPlay_SwitchProg failed\n");
 			goto AVPLAY_STOP;
        }
    }
    
    sample_printf("\n*********We have %d programs**********\n\n", g_pProgTbl->prog_num);

    while(1)
    {
        printf("please input 'q' to quit!\n");
        SAMPLE_GET_INPUTCMD(InputCmd);

        if ('q' == InputCmd[0])
        {
            printf("prepare to quit!\n");
            break;
        }

        ProgNum = atoi(InputCmd);

		if(ProgNum > 0)
		{
			for (i = 0; i < winNum; i++)
			{
				Ret = HIADP_AVPlay_PlayProg(hAvplay[i], g_pProgTbl, (ProgNum-1)%g_pProgTbl->prog_num, bAudPlay[i]);
				if (HI_SUCCESS != Ret)
				{
					sample_printf("call HIADP_AVPlay_PlayProg!\n");
					break;
				}
			}
		}
		else
		{
			for (i = 0; i < winNum; i++)
			{
				Ret = HIADP_AVPlay_PlayProg(hAvplay[i], g_pProgTbl, i%g_pProgTbl->prog_num, bAudPlay[i]);
				if (HI_SUCCESS != Ret)
				{
					sample_printf("call HIADP_AVPlay_PlayProg!\n");
					break;
				}
			}

		}

    }
    
AVPLAY_STOP:
	//Move HI_UNF_VO_SetWindowEnable here to avoid refreshing delay
	//but error occurs: Play: queue frame to master win failed 0x80110041
	/*for (i = 0; i < winNum; i++)
	{
		HI_UNF_VO_SetWindowEnable(hWin[i],HI_FALSE);
	}*/
	
    for(i = 0 ; i < winNum;i++)
    {
        Stop.enMode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
        Stop.u32TimeoutMs = 0;
        HI_UNF_AVPLAY_Stop(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD, &Stop);
        if (0 == i)
        {
            HI_UNF_SND_Detach(hTrack, hAvplay[i]);
            HI_UNF_SND_DestroyTrack(hTrack);
        }        
        HI_UNF_VO_SetWindowEnable(hWin[i],HI_FALSE);
        HI_UNF_VO_DetachWindow(hWin[i], hAvplay[i]);
        HI_UNF_AVPLAY_ChnClose(hAvplay[i], HI_UNF_AVPLAY_MEDIA_CHAN_VID | HI_UNF_AVPLAY_MEDIA_CHAN_AUD);
        HI_UNF_AVPLAY_Destroy(hAvplay[i]);         
    }
    HI_UNF_AVPLAY_DeInit();

PSISI_FREE:
    HIADP_Search_FreeAllPmt(g_pProgTbl);
    HIADP_Search_DeInit();

    g_StopSocketThread = HI_TRUE;
    pthread_join(g_SocketThd, HI_NULL);

    HI_UNF_DMX_DestroyTSBuffer(g_TsBuf);
    
DMX_DEINIT:
    HI_UNF_DMX_DetachTSPort(DmxId);
    HI_UNF_DMX_DeInit();

VO_DEINIT:
    for ( i = 0 ; i < winNum ; i++ )
    {
        HI_UNF_VO_DestroyWindow(hWin[i]);        
    }
    HIADP_VO_DeInit();

DISP_DEINIT:
	HIADP_Disp_DeInit();

SND_DEINIT:
    HIADP_Snd_DeInit();

SYS_DEINIT:
    HI_SYS_DeInit();

    return Ret;
}


