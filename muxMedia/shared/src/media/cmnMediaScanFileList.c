
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "_muxMedia.h"

const char *_directoryTypeString(int type)
{
	switch (type)
	{
		case AVIO_ENTRY_DIRECTORY:
			return "<DIR>";
		
		case AVIO_ENTRY_FILE:
			return "<FILE>";
			
		case AVIO_ENTRY_BLOCK_DEVICE:
			return "<BLOCK DEVICE>";
			
		case AVIO_ENTRY_CHARACTER_DEVICE:
			return "<CHARACTER DEVICE>";
			
		case AVIO_ENTRY_NAMED_PIPE:
			return "<PIPE>";
			
		case AVIO_ENTRY_SYMBOLIC_LINK:
			return "<LINK>";
			
		case AVIO_ENTRY_SOCKET:
			return "<SOCKET>";
			
		case AVIO_ENTRY_SERVER:
			return "<SERVER>";
			
		case AVIO_ENTRY_SHARE:
			return "<SHARE>";
			
		case AVIO_ENTRY_WORKGROUP:
			return "<WORKGROUP>";
			
		case AVIO_ENTRY_UNKNOWN:
		default:
			break;
	}
	
	return "<UNKNOWN>";
}

int muxMediaFileDebug(MEDIA_FILE_T *file, void *data)
{
	struct tm	ts;
	char			buf[128];
	int hours, mins, secs;
	
	printf("Media File: name:%s/%s, format: %s; size: %d; ", 
		file->path,file->name, file->formatName, file->size );

	if(file->duration > 0)
	{
		secs  = file->duration ;
		mins = secs /60;
		secs %= 60;
		hours = mins / 60;
		mins %= 60;
		printf("duration: %02d:%02d:%02d\n", hours, mins, secs );
	}
	else
	{
		printf("duration: N/A\n");
	}
	
	printf("\t Video:%s, Audio: %s; ",
		file->hasVideo==0?"No":"Yes", file->hasAudio==0?"No":"Yes" );
	printf(" Subtitle: %s; \n", file->hasSubtitle==0?"No":"Yes" );

	ts = *localtime((time_t *)&file->access_timestamp);
	strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
	printf("\t time:%s (%d)\n", buf,  file->access_timestamp );
#if 0
	printf("\t time: modify:%s; access:%s; status:%s\n", 
		asctime(gmtime((const time_t *)&file->modification_timestamp)), asctime(gmtime((const time_t *)&file->access_timestamp)), 
		asctime(gmtime((const time_t *)&file->status_change_timestamp)));
	printf("\t time: modify:%s; access:%s; status:%s\n", 
		ctime((const time_t *)&file->modification_timestamp), ctime((const time_t *)&file->access_timestamp), 
		ctime((const time_t *)&file->status_change_timestamp) );
	printf("\t time: modify:%d; access:%d; status:%d\n", 
		file->modification_timestamp, file->access_timestamp, file->status_change_timestamp );
#endif
	return 0;
}

static int _addOneFile(MEDIA_FILE_LIST_T *list, AVIODirEntry *file, char *path)
{
	AVFormatContext *inCtx = NULL;
	MEDIA_FILE_T *_file = NULL;
	char fullPath[CMN_NAME_LENGTH];
	int ret;
	int i;

	snprintf(fullPath, sizeof(fullPath), "%s/%s", path, file->name);

//	MUX_DEBUG(CMN_LOG_DEBUG, "Scan file '%s'...", fullPath);
	
	if ((ret = avformat_open_input(&inCtx, fullPath, 0, 0)) < 0)
	{
		MUX_ERROR("'%s' is not a supportted media file", fullPath);
		return -1;
	}

	if ((ret = avformat_find_stream_info(inCtx, 0)) < 0)
	{
		MUX_ERROR("Failed to retrieve input stream information in file '%s'", fullPath);
		avformat_close_input(&inCtx);
		return -1;
	}

	if(! strcmp(inCtx->iformat->name, "tty") )
	{
		avformat_close_input(&inCtx);
		return -1;
	}
	
	_file = (MEDIA_FILE_T *)cmn_malloc( sizeof(MEDIA_FILE_T));
	if( _file == NULL)
	{
		MUX_ERROR("No memory is available for media file list");
		exit(1);
	}

//	av_dump_format(inCtx, 0, fullPath, 0);
	
	_file->size = (int)KB(file->size);
	_file->duration = inCtx->duration;
	if(inCtx->duration != AV_NOPTS_VALUE)
		_file->duration = (int)((inCtx->duration + (inCtx->duration <= INT64_MAX - 5000 ? 5000 : 0))/AV_TIME_BASE);
	else
		_file->duration = 0;
	
	_file->modification_timestamp = file->modification_timestamp/INT64_C(1000000);
	_file->access_timestamp = file->access_timestamp/ INT64_C(1000000);
	_file->status_change_timestamp = file->status_change_timestamp/INT64_C(1000000);
	
	snprintf(_file->formatName, 128, "%s", inCtx->iformat->name);
	snprintf(_file->name, CMN_NAME_LENGTH, "%s", file->name);
	snprintf(_file->path, CMN_NAME_LENGTH, "%s", path);

	for (i = 0; i < inCtx->nb_streams; i++)
	{
		AVStream *st = inCtx->streams[i];

		switch (st->codecpar->codec_type)
		{
			case AVMEDIA_TYPE_VIDEO:
				_file->hasVideo = TRUE;
				break;
			case AVMEDIA_TYPE_AUDIO:
				_file->hasAudio = TRUE;
				break;
			case AVMEDIA_TYPE_SUBTITLE:
				_file->hasSubtitle= TRUE;
				break;
			default:
				break;
		}			

		//dump_stream_format(inCtx, i, i, 0);
	}

	cmn_list_append(list->list, _file);
	
//	MUX_DEBUG("Added file '%s'!!!", fullPath);
	avformat_close_input(&inCtx);

//	MUX_DEBUG("Add media file '%s' OK!", fullPath);
	return 0;
}


int	muxMediaFileIterate(MEDIA_FILE_LIST_T *files, MEDIA_FILE_HANDLER *fileHandle, void *data)
{
	MEDIA_FILE_T *file = NULL;
	int i, ret;

	if (fileHandle == NULL || files == NULL)
	{
		return 0;
	}
	
	for(i=0; i< cmn_list_size(files->list); i++ ) 
	{
		file = (MEDIA_FILE_T *)cmn_list_get(files->list, i);
		if( file == NULL)
		{
			MUX_WARN( "No %d media file is NULL" , i);
			exit(1);
		}

		ret = (*fileHandle)(file, data);
		if(ret <0)
			return ret;
	}

	return 0;
}


static int _listDirectory(MEDIA_FILE_LIST_T *files,  char *dirName)
{
	AVIODirEntry *entry = NULL;
	AVIODirContext *ctx = NULL;
	char filemode[4];
//	char	uid_and_gid[20];
	int cnt, ret;

//	MUX_DEBUG("scan directory '%s' ...", dirName);
	
	if ((ret = avio_open_dir(&ctx, dirName, NULL)) < 0)
	{
		MUX_ERROR("Cannot open directory: %s.\n", av_err2str(ret));
		return -1;
	}

	cnt = 0;
	for (;;)
	{
		if ((ret = avio_read_dir(ctx, &entry)) < 0)
		{
			MUX_ERROR("Cannot list directory: %s.\n", av_err2str(ret));
			avio_close_dir(&ctx);
		}
		
		if (!entry)
		{/* end of this directory */
			break;
		}

		if (entry->filemode == -1)
		{
			snprintf(filemode, 4, "???");
		}
		else
		{
			snprintf(filemode, 4, "%3"PRIo64, entry->filemode);
		}

		if(entry->type == AVIO_ENTRY_FILE)
		{
			files->countSubFiles ++;
			ret = _addOneFile( files, entry, dirName);
		}
		else if(entry->type == AVIO_ENTRY_DIRECTORY)
		{
			char fullPath[CMN_NAME_LENGTH];
			snprintf(fullPath, sizeof(fullPath), "%s/%s", dirName, entry->name );

			files->countSubDirs++;
			ret = _listDirectory(files, fullPath);
		}

#if 0		
		snprintf(uid_and_gid, 20, "%"PRId64"(%"PRId64")", entry->user_id, entry->group_id);
		if (cnt == 0)
			av_log(NULL, AV_LOG_INFO, "%-9s %12s %30s %10s %s %16s %16s %16s\n",
				"TYPE", "SIZE", "NAME", "UID(GID)", "UGO", "MODIFIED",
				"ACCESSED", "STATUS_CHANGED");

		av_log(NULL, AV_LOG_INFO, "%-9s %12"PRId64" %30s %10s %s %16"PRId64" %16"PRId64" %16"PRId64"\n",
			_directoryTypeString(entry->type),
			entry->size,
			entry->name,
			uid_and_gid,
			filemode,
			entry->modification_timestamp,
			entry->access_timestamp,
			entry->status_change_timestamp);
#endif		

		avio_free_directory_entry(&entry);

		cnt++;
	};

	return 0;
}

int muxMediaListSearchDirectory(MEDIA_FILE_LIST_T *files)
{
	int ret;

	//cmn_list_init( &files->list);
//	MUX_DEBUG("Scanning '%s'", files->fullPath);
	if ( (files->list != NULL) )// && (cmn_list_size( &files->list) != 0) )
	{
		cmn_list_ofchar_free( files->list, TRUE );
		memcpy(&files->lastTime, &files->scanTime, sizeof(time_t));//struct tm) );
	}

	files->list = (cmn_list_t *)cmn_malloc(sizeof(cmn_list_t));
	if( files->list == NULL)
	{
		MUX_ERROR("No memory is available" );
		exit(1);
	}
	cmn_list_init( files->list);
	
	files->countSubDirs = 0;
	files->countSubFiles = 0;
	time(&files->scanTime);

	ret = _listDirectory(files, files->fullPath);
	
	if(ret < 0 || cmn_list_size( files->list) == 0)
	{
		MUX_ERROR("return %d; list size:%d\n", ret, cmn_list_size( files->list));
		return -1;
	}

	return 0;
}

int muxMediaScanAll(MEDIA_FILE_LIST_T *files)
{
	MEDIA_FILE_LIST_T *tmp = files;
	int ret;

	while(tmp)
	{
		ret = muxMediaListSearchDirectory(tmp);
		if( ret <0)
		{
			MUX_WARN("Failed in scanning directory '%s'\n", tmp->fullPath);
			//return EXIT_FAILURE;
		}
		tmp = tmp->next;
	}

	return EXIT_SUCCESS;
}


MEDIA_FILE_LIST_T filesInSdCard =
{
	.type = MEDIA_DEVICE_SDCARD,
#if 0
	.fullPath = MUX_PATH_OF_SDCARD,	
#endif
	.countSubDirs = 0,
	.countSubFiles = 0,
	.list = NULL,
	.next = NULL,
};

MEDIA_FILE_LIST_T filesInUsbDisk =
{
	.type = MEDIA_DEVICE_USBDISK,
#if 0
	.fullPath = MUX_PATH_OF_USBDISK,	
#endif
	.countSubDirs = 0,
	.countSubFiles = 0,
	.list = NULL,
	.next = NULL,
};

static MEDIA_FILE_LIST_T *_allStorage;

static int _scanFileAddIntoList( MEDIA_FILE_T *file, void *data)
{
	cmn_list_t *list = (cmn_list_t *)data;

	cmn_list_append(list, file);
	
	return EXIT_SUCCESS;
}

cmn_list_t	*cmnMediaScanGetFiles(void)
{
	int res = EXIT_SUCCESS;
	cmn_list_t	*mediaFiles = NULL;

	/* the list of all media files will be freed before next scan begin*/
	res = muxMediaScanAll(_allStorage );
	if(res == EXIT_FAILURE)
		return NULL;
	
	mediaFiles = (cmn_list_t *)cmn_malloc(sizeof(cmn_list_t));
	cmn_list_init(mediaFiles);
	
	MUX_MEDIA_ITERATE_ALL( _allStorage, _scanFileAddIntoList, mediaFiles);

	return mediaFiles;
}

/* TX762 only has internal SD card, RX762 has one USB disk and SD card */
int	cmnMediaScanInit(MuxMediaConfig *mediaCaptureConfig)
{
	int res = EXIT_SUCCESS;

	/* RX : has both SD Card and USB Disk */
	_allStorage = &filesInSdCard;
#if ARCH_X86
	snprintf(filesInSdCard.fullPath, CMN_NAME_LENGTH, "%s", MUX_PATH_OF_SDCARD);
#else
	snprintf(filesInSdCard.fullPath, CMN_NAME_LENGTH, "%s", mediaCaptureConfig->sdHome);
#endif

	if(mediaCaptureConfig->boardType == MUX_BOARD_TYPE_RX769 || mediaCaptureConfig->boardType == MUX_BOARD_TYPE_RX762 )
	{
		_allStorage->next = &filesInUsbDisk;
#if ARCH_X86
		snprintf(filesInUsbDisk.fullPath, CMN_NAME_LENGTH, "%s", MUX_PATH_OF_USBDISK);
#else
		snprintf(filesInUsbDisk.fullPath, CMN_NAME_LENGTH, "%s", mediaCaptureConfig->usbHome);
#endif
	}

	return res;
}


int	cmnMediaCheckLocalFile(char *localImageFile)
{
	char *resolvePath = NULL;
	MEDIA_FILE_LIST_T *dirHeader =  _allStorage;
	int isLocal = 0;

	if(IS_STRING_NULL(localImageFile))
	{
		return EXIT_SUCCESS;
	}

	resolvePath = realpath(localImageFile, NULL);
	if(!resolvePath)
	{
		MUX_WARN("Can't get resolved path of '%s': %s", localImageFile, strerror(errno));
		return EXIT_SUCCESS;
	}

	while(dirHeader)
	{
//		MUX_DEBUG("resolved path '%s': local path '%s'", localImageFile, dirHeader->fullPath );
		if(!strncasecmp(dirHeader->fullPath, resolvePath, (strlen(dirHeader->fullPath)>strlen(resolvePath))?strlen(resolvePath):strlen(dirHeader->fullPath)))
		{
//			MUX_DEBUG("resolved path '%s': is local path ", localImageFile );
			isLocal = 1;
		}

		dirHeader = dirHeader->next;
	}

	free(resolvePath);

	return isLocal;

}


/* 0: not image file; others : image file */
int	cmnMediaCheckCheckImageFile(char *imageFile)
{
	AVFormatContext *inCtx = NULL;
	AVStream *st = NULL;
	int ret;
	int isImage = 0;


//	MUX_DEBUG("check image file '%s'...", imageFile);
	if ((ret = avformat_open_input(&inCtx, imageFile, 0, 0)) < 0)
	{
		MUX_WARN("'%s' is not a supportted media file", imageFile);
		goto fail1;
	}

	if ((ret = avformat_find_stream_info(inCtx, 0)) < 0)
	{
		MUX_WARN("Failed to retrieve input stream information in file '%s'", imageFile);
		goto fail2;
	}

	av_dump_format(inCtx, 0, imageFile, 0);
	
	if( inCtx->nb_streams != 1)
	{
//		MUX_WARN("More than one streams (%d) in '%s'", inCtx->nb_streams, imageFile);
		goto fail2;		
	}
	
	st = inCtx->streams[0];
	if(st->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		switch (st->codecpar->codec_id)
		{
			case	AV_CODEC_ID_GIF:
			case	AV_CODEC_ID_BMP:
			case	AV_CODEC_ID_JPEG2000:
			case	AV_CODEC_ID_JPEGLS:
			case	AV_CODEC_ID_MJPEG:
			case	AV_CODEC_ID_PNG:
				isImage = 1;
//				MUX_DEBUG("supported codec format is '%s'", avcodec_get_name(st->codecpar->codec_id));
				break;
			default:
//				MUX_WARN("not supported codec format for image is '%s'", avcodec_get_name(st->codecpar->codec_id));
				break;
		}		
	}
	else
	{
		MUX_WARN("Media format of '%s' is %s", imageFile, av_get_media_type_string(st->codecpar->codec_type));
	}

//	MUX_DEBUG("Added file '%s'!!!", imageFile);

fail2:
	avformat_close_input(&inCtx);

fail1:
	return isImage;
}


/* check whether file is in SD card ot not */
int	cmnMediaCheckSDCardFile(char *fileName)
{
	char *resolvePath = NULL;
	MEDIA_FILE_LIST_T *dirHeader =  _allStorage;

	if(IS_STRING_NULL(fileName))
	{
		return FALSE;
	}

	resolvePath = realpath(fileName, NULL);
	if(!resolvePath)
	{
		MUX_WARN("Can't get resolved path of '%s': %s", fileName, strerror(errno));
		return FALSE;
	}

	while(dirHeader)
	{
//		MUX_DEBUG("resolved path '%s': local path '%s'", localImageFile, dirHeader->fullPath );
		if(dirHeader->type == MEDIA_DEVICE_SDCARD )
		{
			if(!strncasecmp(dirHeader->fullPath, resolvePath, (strlen(dirHeader->fullPath)>strlen(resolvePath))?strlen(resolvePath):strlen(dirHeader->fullPath)))
			{
//				MUX_DEBUG("resolved path '%s': is local path ", localImageFile );
				free(resolvePath);
				return TRUE;
			}
		}	

		dirHeader = dirHeader->next;
	}

	free(resolvePath);

	return FALSE;
	

}


/* remount SD card as rw or ro */
int	cmnMediaRemountSDCard(int isWrite)
{
	MEDIA_FILE_LIST_T *dirHeader =  _allStorage;
	char		command[256];
//	int ret;

	while(dirHeader)
	{
		if(dirHeader->type == MEDIA_DEVICE_SDCARD )
		{
			snprintf(command, sizeof(command), "mount -o remount -o %s %s", (isWrite==0)?"ro":"rw", dirHeader->fullPath);
			//ret = 
			system(command);

			return EXIT_SUCCESS;
		}	

		dirHeader = dirHeader->next;
	}


	return EXIT_FAILURE;
}


