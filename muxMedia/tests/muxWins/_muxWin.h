
#ifndef	___MUX_WIN_H__
#define	___MUX_WIN_H__


#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/resource.h>
#include <signal.h>

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"
#include "ms_version.h"

#define	WITH_HI_AVPLAY	0	/* use AvPlay from HiPlayer */

typedef	struct	MWIN_AVPLAY
{

	HI_HANDLE			handleWin;
	HI_HANDLE			handleTrack;
	HI_HANDLE			handleAvPlay;

}MWIN_AVPLAY_T;


typedef	struct	MWIN_PLAYER
{
	HI_SVR_PLAYER_PARAM_S			playerParam;
	HI_SVR_PLAYER_MEDIA_S 			media;
	
	HI_HANDLE						hiPlayer;
}MWIN_PLAYER_T;

typedef	struct MWIN_PLAY
{
	int								count;			/* count of media */
	
	cmn_list_t						players;
	cmn_list_t						avPlays;


	int								masterIndex;	/* which media is the master track */
	HI_HANDLE						masterTrack;	/* only master track can be not mute at any time */

	char								url[256];
	int								isRestart;
	int								countOfPlaying;
}MWIN_PLAY_T;


int muxWinInitMultipleAvPlay( MWIN_PLAY_T *player);
int muxWinInitMultipleHiplayer(MWIN_PLAY_T *player);

int muxWinSwapVideo(MWIN_PLAY_T *player, int newMasterIndex);
int muxWinRotateVideo(MWIN_PLAY_T *player, HI_UNF_VO_ROTATION_E mode);
int muxWinConnectAvPlay(MWIN_AVPLAY_T *avPlay);


int muxWinSetMedia(MWIN_PLAY_T *players, MWIN_PLAYER_T *play, int index);

int muxWinPlayerEventCallBack(HI_HANDLE hPlayer, HI_SVR_PLAYER_EVENT_S *event);

#endif

