
#include "_muxWin.h"


MWIN_PLAY_T		_players;


int muxWinPlayerEventCallBack(HI_HANDLE hPlayer, HI_SVR_PLAYER_EVENT_S *event)
{
	MWIN_PLAY_T *players = (MWIN_PLAY_T *)&_players;
	
	if (0 == hPlayer || NULL == event)
	{
		return HI_SUCCESS;
	}

	HI_SVR_PLAYER_EVENT_S *newEvent = (HI_SVR_PLAYER_EVENT_S *)cmn_malloc(sizeof(HI_SVR_PLAYER_EVENT_S));
	if(newEvent==NULL)
	{
		MUX_ERROR( "No memory is available");
		exit(1);
	}

	newEvent->pu8Data = (HI_U8 *)cmn_malloc(event->u32Len);
	if(newEvent->pu8Data == NULL)
	{
		MUX_ERROR("No memory for data is available");
		exit(1);
	}
	newEvent->eEvent = event->eEvent;
	newEvent->u32Len = event->u32Len;
	memcpy(newEvent->pu8Data, event->pu8Data, newEvent->u32Len);

	if(newEvent->eEvent == HI_SVR_PLAYER_EVENT_PROGRESS )
	{
		return HI_SUCCESS;
	}

	MUX_INFO("Event : '%s'", muxPlayerEventName(newEvent->eEvent));
	if(newEvent->eEvent == HI_SVR_PLAYER_EVENT_STATE_CHANGED )
	{
		int eEventBk = (HI_SVR_PLAYER_STATE_E)*newEvent->pu8Data;
		MUX_INFO("STATE_CHANGED: Status change to %s ", muxHiPlayerStateName(eEventBk));
		return HI_SUCCESS;
	}

	if(newEvent->eEvent == HI_SVR_PLAYER_EVENT_EOF )
	{
		players->isRestart = 1;
		MUX_INFO("'EOF event!:%p: %d", players, players->isRestart );
		return HI_SUCCESS;
	}


#if 0
	HI_SVR_PLAYER_Invoke(muxPlay.playerHandler, HI_FORMAT_INVOKE_GET_BANDWIDTH, &muxPlay.downloadBandWidth);
	CMN_MSG_LOG(CMN_LOG_DEBUG, "DownloadProgress bandwidth:'%lld'", muxPlay.downloadBandWidth);
#endif

	return HI_SUCCESS;
}

int main(int argc, char **argv)
{
	MWIN_PLAY_T		*players = &_players;
	int res = 0;
	int count = 1;
#if 0	
	int	i;
	MWIN_PLAYER_T	*plays[2];
	MWIN_AVPLAY_T	*avPlays[2];
#endif

	printf("%s %s %d", __FILE__, __FUNCTION__, __LINE__);
	CMN_SHARED_INIT();

	memset(players, 0 , sizeof(MWIN_PLAY_T) );

	cmn_list_init( &players->avPlays);
	cmn_list_init( &players->players);
	players->count = count;
	players->masterIndex = 0;
	
	cmnThreadSetName("Main");
	res = muxWinInitMultipleAvPlay( players);
	if(res != HI_SUCCESS)
	{
		return -1;
	}

	res = muxWinInitMultipleHiplayer( players);
	if(res != HI_SUCCESS)
	{
		return -1;
	}

#if 0
	if(count != cmn_list_size(&players->players) || count != cmn_list_size( &players->avPlays) )
	{
		MUX_ERROR("Data wrong");
		return HI_FAILURE;
	}
#endif

	char c = 'c';
	int i= 0;

	cmnThreadSetName("Main");
	MUX_WARN("player is waiting (%p)....", players );
	while( 1 )
	{
//		printf("\n\t\tInput 'q' to continue:\n" );
//		c = getchar();
		i++;
		if( players->isRestart != 0)
		{
			printf("isRestart is '%d'\n\n", players->isRestart );
			
		}
		else
		{
//			printf("isRestart is '%d'", players->isRestart );
		}
		
		if(players->isRestart != 0)
		{
			MWIN_PLAYER_T *play = NULL;

			res |= HI_SVR_PLAYER_Invoke(play->hiPlayer, HI_FORMAT_INVOKE_PRE_CLOSE_FILE, NULL);
			if(res != HI_SUCCESS)
			{
				MUX_ERROR("Invoke Close File in HiPlayer failed :0x%x", res);
			}
			
			players->countOfPlaying++;
			MUX_WARN("No.%d playing....", players->countOfPlaying+1);
			play = (MWIN_PLAYER_T *)cmn_list_get(&players->players, 0);
#if 0
			res = HI_SVR_PLAYER_Stop(play->hiPlayer);
			if (HI_SUCCESS != res)
			{
				MUX_ERROR("stop fail, ret = 0x%x", res);
				return HI_FAILURE;
			}
#endif			

#if 0
			res |= HI_SVR_PLAYER_Invoke(play->hiPlayer, HI_FORMAT_INVOKE_PRE_CLOSE_FILE, NULL);
			if(res != HI_SUCCESS)
			{
				MUX_WARN("Close File in HiPlayer failed :0x%x", res);
			}
#endif			

			if(play == NULL)
			{
				MUX_ERROR(" Play in list is null" );
				return HI_FAILURE;
			}

			muxWinSetMedia( players, play, 0);

			players->isRestart = 0;
			
			res = HI_SVR_PLAYER_Stop(play->hiPlayer);
			if (HI_SUCCESS != res)
			{
				MUX_ERROR("stop fail, ret = 0x%x", res);
//				return HI_FAILURE;
			}
			else
			{
				MUX_DEBUG("stop command OK!");
			}
			players->isRestart = 1;
		}

		cmn_delay(3);

	}
	MUX_WARN("player ends!");

	return 0;
	
	while( c != 'q' )
	{
		printf("\n\t\tInput 'q' to continue; any other key to swap video window:\n" );
		c = getchar();
		i++;
		printf("\n\t\tyou input is '%c'\n", c);
		res = muxWinSwapVideo(players, i%2);
	}

	c = 'c';
	i = 0;

	while( c != 'q')
	{
		printf("\n\t\tInput 'q' to quit; any other key to rotate video window(%c):\n", c);
		c = getchar();
		i++;
		res = muxWinRotateVideo(players, i%HI_UNF_VO_ROTATION_BUTT);
	}

	return 0;
}


