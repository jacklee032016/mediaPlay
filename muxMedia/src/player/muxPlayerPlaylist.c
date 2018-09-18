

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "muxPlayer.h"
#include "libMuxRx.h"

#define	PLAYER_ADD_PLAYITEM( muxPlaylist, playItem)	\
	{	PLAY_ITEM	*_newItem = (PLAY_ITEM *)cmn_malloc(sizeof(PLAY_ITEM));	\
		memcpy(_newItem, playItem, sizeof(PLAY_ITEM)); \
			cmn_list_append(&(muxPlaylist)->mediaList, _newItem); }


#define 	PLAYER_FRIOM_PLAYITEM(play, playItem)	\
		{	snprintf((play)->currentUrl, HI_FORMAT_MAX_URL_LEN, "%s", (playItem)->filename);	\
			(play)->currentDuration = (playItem)->duration;	}
#if 0 
 MUX_PLAY_INFO("set current duration as %d seconds of media URL %s of %s", (play)->currentDuration, (playItem)->filename, (play)->muxFsm.name);
#endif


/* when player startup or receive Open command from client, this function is called to initialize Playlist in player */
int	muxPlayerInitCurrentPlaylist(MUX_PLAY_T *player, char *fileOrPlaylist, int repeatNumber)
{
//	int i;
	int j;
	PLAY_LIST *cfgPlaylist = NULL;
	PLAY_ITEM	*playItem = NULL;
	struct MUX_PLAYLIST *currentPlaylist = &player->currentPlaylist;
	
	MuxMain *muxMain = (MuxMain *)player->muxRx->muxPlayer->priv;

	currentPlaylist->currentIndex = 0;
	currentPlaylist->currentRepeat = 0;


	cmn_list_ofchar_free(&currentPlaylist->mediaList, FALSE);
	cmn_list_init(&currentPlaylist->mediaList);

	SET_PLAYER_DEFAULT_SPEED(player);
	
	snprintf(currentPlaylist->name, CMN_NAME_LENGTH, "%s", fileOrPlaylist);

	/* if this is name of one playlist */
#if 1
	SYS_PLAYLIST_LOCK(muxMain);

	cfgPlaylist = cmnMuxPlaylistFind(SYS_PLAYLISTS(muxMain), fileOrPlaylist);
	if(cfgPlaylist)
	{
		if(repeatNumber < 0)
		{
//			currentPlaylist->repeat = cfgPlaylist->repeat;
			currentPlaylist->repeat = 0;
		}
		else
		{
			currentPlaylist->repeat = repeatNumber;
		}
		
		for(j=0; j< cmn_list_size(&cfgPlaylist->fileList); j++)
		{
			playItem = (PLAY_ITEM *)cmn_list_get(&cfgPlaylist->fileList, j);
//			cmn_list_append(&currentPlaylist->mediaList, strdup(media));
			PLAYER_ADD_PLAYITEM(currentPlaylist, playItem);
			if(j==0)
			{
				PLAYER_FRIOM_PLAYITEM(player, playItem);
			}
		}
		
		SYS_PLAYLIST_UNLOCK(muxMain);
		return EXIT_SUCCESS;
	}

#else
	MuxPlayerConfig *cfg = &player->muxRx->muxPlayer->playerConfig;
	for(i=0; i< cmn_list_size(&cfg->playlists); i++)
	{
		cfgPlaylist = (PLAY_LIST *)cmn_list_get(&cfg->playlists, i);
		if(!strcasecmp(cfgPlaylist->name, fileOrPlaylist) )
		{
			currentPlaylist->repeat = cfgPlaylist->repeat;
			snprintf(currentPlaylist->name, CMN_NAME_LENGTH, "%s", fileOrPlaylist);
			
			for(j=0; j< cmn_list_size(&cfgPlaylist->fileList); j++)
			{
				char	 *media = (char *)cmn_list_get(&cfgPlaylist->fileList, j);
				cmn_list_append(&currentPlaylist->mediaList, strdup(media));
				if(j==0)
				{
					snprintf(player->currentUrl, HI_FORMAT_MAX_URL_LEN, "%s", media);
				}
			}
			
			return EXIT_SUCCESS;
		}
	}
#endif
	SYS_PLAYLIST_UNLOCK(muxMain);

	/* this is a media file or URL */
	if(repeatNumber < 0)
	{
#if 1
		/* only play once  */
		currentPlaylist->repeat = 1;
#else
		/* repeat forever */
		currentPlaylist->repeat = 0;
#endif
	}
	else
	{
		currentPlaylist->repeat = repeatNumber; /* default repeat for local file or URL is only once */
	}

	playItem = cmn_malloc(sizeof(PLAY_ITEM));
	snprintf(playItem->filename, sizeof(playItem->filename), "%s", fileOrPlaylist);
	cmn_list_append(&currentPlaylist->mediaList, playItem);

	PLAYER_FRIOM_PLAYITEM(player, playItem);

	return EXIT_SUCCESS;
}

static int _setCurrentIndexInPlaylist(MUX_PLAY_T *player, struct MUX_PLAYLIST *currentPlaylist )
{
	PLAY_ITEM *playItem = NULL;
	
	playItem = (PLAY_ITEM *)cmn_list_get(&currentPlaylist->mediaList, currentPlaylist->currentIndex);
	if(!playItem)
	{
		CMN_ABORT("PLAY_ITEM is null in position %d of medialist", currentPlaylist->currentIndex);
		return EXIT_FAILURE;
	}

	PLAYER_FRIOM_PLAYITEM(player, playItem);

	return EXIT_SUCCESS;
}

/* when one media playing ends, this function is called to determine how to process in next step */
int	muxPlayerUpdateCurrentPlaylist(MUX_PLAY_T *play)
{
	struct MUX_PLAYLIST *currentPlaylist = &play->currentPlaylist;

	if(currentPlaylist->currentIndex < (cmn_list_size(&currentPlaylist->mediaList) -1) )
	{/* play next media in this playlist */
		currentPlaylist->currentIndex++;
		return _setCurrentIndexInPlaylist(play, currentPlaylist);
	}

	/* next iteration for playlist */
	if(PLAYLIST_UNLIMITED(currentPlaylist->repeat) )
	{
		/* copy it to another memory, and then use as argument; otherwise currentPlaylist->name will copy data to itself */
		char name[CMN_NAME_LENGTH];
		snprintf(name, CMN_NAME_LENGTH, "%s", currentPlaylist->name);
		muxPlayerInitCurrentPlaylist(play, name, currentPlaylist->repeat);
		return EXIT_SUCCESS;
	}

	if(currentPlaylist->currentRepeat < currentPlaylist->repeat -1)
	{
		currentPlaylist->currentRepeat++;
		currentPlaylist->currentIndex = 0;
		
		return _setCurrentIndexInPlaylist(play, currentPlaylist);
	}
	else
	{/* stop all */
		return EXIT_FAILURE;
	}
}


int	muxPlayPreloadImage(MUX_PLAY_T *play)
{
	struct MUX_PLAYLIST *currentPlaylist = &play->currentPlaylist;
	int	nextIndex = currentPlaylist->currentIndex+1;
	int	countOfMedia = cmn_list_size(&currentPlaylist->mediaList);
	char	*mediaName = NULL;
	PLAY_ITEM *playItem = NULL;

	if(nextIndex >= countOfMedia )//&& countOfMedia != 1)
	{
		nextIndex = 0;
	}

	playItem = (PLAY_ITEM *)cmn_list_get(&currentPlaylist->mediaList, nextIndex);
	if(playItem == NULL)
	{
		MUX_PLAY_WARN("No.%d itme in current playlist is null", nextIndex );
		return EXIT_SUCCESS;
	}

	mediaName = playItem->filename;
	if(IS_LOCAL_IMAGE_FILE(mediaName) )
	{
		MUX_PLAY_DEBUG("precoding image file : %s", mediaName);
		PLAY_ALERT_MSG(play, COLOR_GREEN, "Preload image file %s", mediaName);
		muxOsdImageLoad(play->osd, mediaName);
	}

	return EXIT_SUCCESS;
}

