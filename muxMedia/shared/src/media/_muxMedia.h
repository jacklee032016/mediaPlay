/*
* local declarations in libMedia
*/

#ifndef	__MUX_MEDIA_H__
#define	__MUX_MEDIA_H__


#define	MUX_MEDIA_ITERATE_ALL( _files, _handler, _data) \
	{ MEDIA_FILE_LIST_T *tmp = (_files); \
		while(tmp){muxMediaFileIterate(tmp, (_handler), (_data)); tmp = tmp->next;} }


int muxMediaFileDebug(MEDIA_FILE_T *file, void *data);
int	muxMediaFileIterate(MEDIA_FILE_LIST_T *files, MEDIA_FILE_HANDLER *fileHandle, void *data);

int	cmnMediaScanInit(MuxMediaConfig *mediaCaptureConfig);


#endif

