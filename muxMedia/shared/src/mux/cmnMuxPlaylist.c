
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"


#include "_cmnMux.h"


int	cmnMuxPlayListUpdate(cmn_list_t *playlists, int playlistIndex, PLAY_LIST *_playlist)
{
	int res = EXIT_SUCCESS;
	PLAY_LIST *tmp = NULL;

	if(playlistIndex >= cmn_list_size( playlists) )
	{
		fprintf(stdout, "Index %d of PlayList '%s' is out of range (<%d)\n", playlistIndex, _playlist->name, cmn_list_size(playlists) );
		return EXIT_FAILURE;
	}
	
	tmp = (PLAY_LIST *)cmn_list_get(playlists, playlistIndex);
	memcpy(tmp, _playlist, sizeof(PLAY_LIST));
	
	return res;
}

int	cmnMuxPlayListAdd(cmn_list_t *playlists, PLAY_LIST *_playlist)
{
	int res = EXIT_SUCCESS;
	int i;
	PLAY_LIST *tmp;

	for(i=0; i< cmn_list_size(playlists);i++)		
	{
		tmp = (PLAY_LIST *)cmn_list_get(playlists, i);
		if(!strcasecmp(tmp->name, _playlist->name))
		{
			fprintf(stdout, "PlayList with same name of '%s' has existed when adding new playlist\n", _playlist->name);
			return EXIT_FAILURE;
		}
	}

	cmn_list_append(playlists, _playlist);

	return res;
}

int	cmnMuxPlayListRemove(cmn_list_t *playlists, char *playlistName)
{
	int res = EXIT_SUCCESS;
	int i;
	PLAY_LIST *tmp = NULL;

	for(i=0; i< cmn_list_size(playlists); i++)
	{
		tmp = (PLAY_LIST *)cmn_list_get(playlists, i);
	 	if(!strcasecmp(tmp->name, playlistName))
		{
			cmn_list_remove(playlists, i);
			return res;
		}
	}
		
	return EXIT_FAILURE;
}

int	cmnMuxPlayListFileRemove(cmn_list_t *playlists, char *playlistName, char *mediaFileName)
{
	int res = EXIT_SUCCESS;
	int i, j;
	PLAY_LIST *tmp = NULL;

	for(i=0; i< cmn_list_size(playlists); i++)
	{
		tmp = (PLAY_LIST *)cmn_list_get(playlists, i);
	 	if(!strcasecmp(tmp->name, playlistName))
		{
			for(j=0; j< cmn_list_size(&tmp->fileList); j++)
			{
				char *file = (char *)cmn_list_get(&tmp->fileList, j);
				if( !strcasecmp(file, mediaFileName))
				{
					cmn_list_remove(&tmp->fileList, j);
					return res;
				}
			}
		}
	}
		
	return EXIT_FAILURE;
}

int  cmnMuxPlaylistSaveCrontable(cmn_list_t *playlists)
{
	FILE 	*fp;
	char 	buf[128];
	int i;

	if((fp=fopen(CRON_TASK_FILE,"w"))==NULL)
	{	
		fprintf(stderr, "crond file %s open failed\n", CRON_TASK_FILE);
		return EXIT_FAILURE;
	}

	fprintf(fp, "## Configuration file created by program, don't modify it manually##\n");

	{
		struct timeval tv;
		struct tm *ptm;

		char date[32] = {0};
		gettimeofday(&tv, NULL);
		ptm = localtime(&tv.tv_sec);
		strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", ptm);

		fprintf(fp, "## Datetime created : %s\n##\n\n", date);
	}
	
	for(i=0; i< cmn_list_size(playlists); i++)
	{
		PLAY_LIST *playlist = (PLAY_LIST *)cmn_list_get(playlists, i);

		if(playlist->enable && playlist->hour != -1 )
		{
			memset(buf, 0, sizeof(buf) );
			sprintf(buf, "%d %d * * %d apiClient -c play -m %s 1>&2 > /var/apiClient.log\n", playlist->minute, playlist->hour,playlist->dayOfWeek, playlist->name);
			fputs(buf, fp);
		}
	}
	
	fclose(fp);

	cmnKillProcess("crond");
	cmnForkCommand("/usr/sbin/crond");
	
	return EXIT_SUCCESS;
}


/* find Playlist by name */
PLAY_LIST *cmnMuxPlaylistFind(cmn_list_t *playlists, char *playlistName)
{
	int i;
	PLAY_LIST *tmp = NULL;

	for(i=0; i< cmn_list_size(playlists); i++)
	{
		tmp = (PLAY_LIST *)cmn_list_get(playlists, i);
		if(tmp==NULL)
		{
			CMN_ABORT("No. %d playlist is null", i);
		}
		
	 	if(!strcasecmp(tmp->name, playlistName))
		{
			return tmp;
		}
	}
		
	return NULL;
}

PLAY_LIST *cmnMuxCreateOnePlaylist(char *playlistName, cmn_list_t *files)
{
	int i;
	char *filename = NULL;
	PLAY_LIST *playlist = cmn_malloc(sizeof(PLAY_LIST));
	if(playlist==NULL)
	{
		fprintf(stdout, "Without memory forplaylist\n");
		return NULL;
	}
	snprintf(playlist->name, sizeof(playlist->name), playlistName);
	cmn_list_init(&playlist->fileList);
	playlist->enable = TRUE;

	if(files==NULL)
		return playlist;
	
	for(i=0; i< cmn_list_size(files); i++)
	{
		filename = (char *)cmn_list_get(files, i);
		cmn_list_append(&playlist->fileList, filename);
	}

	return playlist;
}


