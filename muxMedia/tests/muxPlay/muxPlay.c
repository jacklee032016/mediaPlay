
#include "libRx.h"

MUXLAB_PLAY_T  		muxPlay;

static HI_SVR_PLAYER_PARAM_S _param = { 0, 3, 0, 0, 0, 0, 100, 0, 0, 0,
#ifndef HI_PLAYER_HBBTV_SUPPORT
		0,
#endif
		HI_UNF_DISPLAY1, 100, 0 };

static void _initDefault(MUXLAB_PLAY_T *mux)
{
	memset(mux, 0, sizeof(MUXLAB_PLAY_T));

	muxRecorderInitDefault( &mux->outCfg);
	
#if (USE_EXTERNAL_AVPLAY == 1)
	mux->trackHandle = (HI_HANDLE)INVALID_TRACK_HDL;
	mux->windowHandle = (HI_HANDLE)HI_SVR_PLAYER_INVALID_HDL;
#endif

	memset(&mux->playerStreamIDs, 0, sizeof(HI_SVR_PLAYER_STREAMID_S));
#if 0
	mux->playerParam = { 0, 3, 0, 0, 0, 0, 100, 0, 0, 0,
#ifndef HI_PLAYER_HBBTV_SUPPORT
		0,
#endif
		HI_UNF_DISPLAY1, 100, 0 };
#else
	memcpy( &mux->playerParam, &_param, sizeof(HI_SVR_PLAYER_PARAM_S));
#endif
	mux->videoFormat = HI_UNF_ENC_FMT_720P_50;

	mux->playerHandler = HI_SVR_PLAYER_INVALID_HDL;
	mux->playerState = HI_SVR_PLAYER_STATE_BUTT;
	mux->pausedCtrl = 0;
	
	mux->soHandler  = HI_SVR_PLAYER_INVALID_HDL;
	mux->avPlayHandler = HI_SVR_PLAYER_INVALID_HDL;
	
	mux->s_s64CurPgsClearTime = -1;

	mux->hlsStartMode = 0;

	mux->isStopSaveThread = HI_TRUE;
	mux->isSaveStream = HI_FALSE;

	mux->noPrintInHdmiATC = HI_TRUE;
}


HI_S32 getfiledir(HI_CHAR *dirName,HI_CHAR *mediaUrl)
{
	DIR *dir = NULL;
	struct dirent *dir_env = NULL;
	//  struct stat stat_file;

	HI_CHAR dir_temp[1024];
	HI_CHAR vid_name[1024];

	FILE *m3u8 = NULL;

	if (NULL == m3u8)
	{
		m3u8 = fopen(mediaUrl,"w+");
	}

	fwrite("#EXTM3U\n",8,1, m3u8);
	fwrite("#EXT-X-TARGETDURATION:\n",23,1, m3u8);
	fwrite("#EXT-X-MEDIA-SEQUENCE:\n",23,1, m3u8);
	fflush(m3u8);

	if ((dir = opendir(dirName)) == NULL)
	{
		return HI_FAILURE;
	}

	while ((dir_env = readdir(dir)) != NULL)
	{
		if (!strcasecmp(dir_env->d_name,"bdmv"))
		{
			strcpy(dir_temp,dirName);
			strcat(dir_temp,"bdmv/stream/");
			break;
		}
		else if (!strcasecmp(dir_env->d_name,"video_ts"))
		{
			strcpy(dir_temp,dirName);
			strcat(dir_temp,"video_ts/");
			break;
		}
	}

	closedir(dir);

	if ((dir = opendir(dir_temp)) == NULL)
	{
		return HI_FAILURE;
	}

	if (strstr(dir_temp,"bdmv/stream/"))  /*for bule ray*/
	{
		while ((dir_env = readdir(dir)) != NULL)
		{
			strcpy(vid_name,dir_temp);
			strcat(vid_name,dir_env->d_name);
			fwrite("#EXTINF:\n",9,1, m3u8);
			fwrite(vid_name,strlen(vid_name),1, m3u8);
			fwrite("\n",1,1, m3u8);
			fflush(m3u8);
		}
	}
	else if (strstr(dir_temp,"video_ts/"))  /*for DVD*/
	{
		while ((dir_env = readdir(dir)) != NULL)
		{
			if (strstr(dir_env->d_name,".vob"))
			{
				if (!strcasecmp(dir_env->d_name,"VIDEO_TS.VOB"))
				{
					continue;
				}
				else
				{
					strcpy(vid_name,dir_temp);
					strcat(vid_name,dir_env->d_name);
					fwrite("#EXTINF:\n",9,1, m3u8);
					fwrite(vid_name,strlen(vid_name),1, m3u8);
					fwrite("\n",1,1, m3u8);
					fflush(m3u8);
				}
			}
		}
	}

	fwrite("#EXT-X-ENDLIST\n",15,1, m3u8);
	fflush(m3u8);
	fclose(m3u8);
	closedir(dir);

	return HI_SUCCESS;
}


HI_S32 main(HI_S32 argc, HI_CHAR **argv)
{
	MUXLAB_PLAY_T *mux = &muxPlay;
	HI_S32 ret;

	if (argc < 2)
	{
		PRINTF("Usage: sample_localplay file                               \n"
		"                                                           \n"
		"          ------- select operation -------                 \n"
		"                                                           \n"
		"  -s subtitlefile1,subtitlefile2,... : input subtitle file \n"
		"  --drm trans=1 mimetype=xx DrmPath=./pdrd,LocalStore=data.localstore\n"
		"  --drm InstallDrmEngine file.so \n"
		"  -f videoFormat \n"
		"  --hls_fast_start"
		"\n");
		return HI_FAILURE;
	}

	_initDefault( mux);
	ret = muxInit( mux, argc, argv);
	if( ret == HI_FAILURE)
		goto LEAVE;

	av_register_all();


	while (1)
	{
		if (muxPlayCmdsHandle(mux) == MUX_QUIT)
			break;
	}

LEAVE:
	muxDeinit(mux);
	
	return HI_SUCCESS;
}

