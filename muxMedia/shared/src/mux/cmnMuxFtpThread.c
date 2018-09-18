
#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include <sys/stat.h>

#include "_cmnMux.h"

struct	CMN_FTP_CONTROLLER
{
	char			localPath[CMN_NAME_LENGTH];
	int			freeSpace;	/* used to extended */

	CMN_FTP_CLIENT	*currentReq;

	int				downloadSize;	/* how many bytes have been downloaded for all requests */
	int				totalReqs;
	int				totalSuccess;
	int				totalFailed;

	void				*priv;	/*save MuxPlayerConfig **/
};

int _checkAndMakeDirectory(char *localPath)
{
	int res = EXIT_SUCCESS;
	struct	stat info;
	
	res = stat(localPath, &info);
	if( res == -1)
	{
        	if(1)//errno == ENOTDIR )
        	{
	        	res = mkdir(localPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
 			res = stat(localPath, &info);
       	}
			
		if(res == -1)	
		{
			MUX_ERROR("local path '%s' failed: %s", localPath, strerror(errno));
			return EXIT_FAILURE;
		}
	}
		
	if(!S_ISDIR(info.st_mode))
	{
		MUX_ERROR("local path '%s' is not a directory", localPath );
		return EXIT_FAILURE;
	}

	return res;
}

void cmnFtpClientInitDefault(CMN_FTP_CLIENT *client)
{
	if (client == NULL)
		return;
	memset(client, 0, sizeof(CMN_FTP_CLIENT));

	snprintf(client->username, CMN_NAME_LENGTH, "%s", "root");
//	snprintf(client->username, CMN_NAME_LENGTH, "%s", "anonymous");

	client->port = 21;

#if ARCH_X86
	snprintf(client->host, CMN_NAME_LENGTH, "%s", "192.168.168.102");
//	snprintf(client->localPath, CMN_NAME_LENGTH, "%s", "/media/sf_mux");
#else
//	snprintf(client->host, CMN_NAME_LENGTH, "%s", "192.168.168.50");
	snprintf(client->host, CMN_NAME_LENGTH, "%s", "192.168.168.102");
//	snprintf(client->localPath, CMN_NAME_LENGTH, "%s", "/media/usb");
#endif

}


static int _cmnFtpClientConnect(CMN_FTP_CLIENT *client, struct CMN_FTP_CONTROLLER *ftpAgent)
{
	if (!FtpConnect(client->host,&client->ctrlConn))
	{
		MUX_ERROR("Unable to connect to node %s", client->host);
		return IPCMD_ERR_FTP_SERVER_ERROR;
	}
	
	if (!FtpLogin(client->username,client->password, client->ctrlConn))
	{
		MUX_ERROR("Login failure: %s", FtpLastResponse(client->ctrlConn));
		return IPCMD_ERR_FTP_USER_PASSWORD;
	}
	
	FtpClearCallback(client->ctrlConn);

	return EXIT_SUCCESS;
}

static int _cmnFtpClientGetFiles(CMN_FTP_CLIENT *client, struct CMN_FTP_CONTROLLER *ftpAgent)
{
	char		localFile[CMN_NAME_LENGTH];
	char		remoteFile[CMN_NAME_LENGTH];
	fsz_t		fsz;
	char		mode = 'I';
	fsz_t		returnSz = 0;
	int i;
	
	for(i=0; i< cmn_list_size(client->downloadFileList); i++)
	{
		char *file = cmn_list_get(client->downloadFileList, i);
		
		snprintf(remoteFile,CMN_NAME_LENGTH, "%s/%s", client->remotePath, file);
		snprintf(localFile, CMN_NAME_LENGTH, "%s/%s", ftpAgent->localPath, file);

		if(cmnMediaCheckSDCardFile(localFile) )
		{
			cmnMediaRemountSDCard(TRUE);
		}

		
#if defined(__UINT64_MAX)
		if (!FtpSizeLong(remoteFile, &fsz, mode, client->ctrlConn))
#else
		if (!FtpSize(remoteFile, &fsz, mode, client->ctrlConn) )
#endif
		{	
			MUX_ERROR("Retrieving length of remote file '%s' failed", remoteFile);
			fsz = 0;	
		}

		client->fsz = fsz;
#if 0		
		fsz /= 100;
		if (fsz > 100000)
			fsz = 100000;
		if ( fsz == 0 )
			fsz = 32768;

		if (ftplib_debug)
		{
			FtpCallbackOptions opt;
			opt.cbFunc = log_progress;
			opt.cbArg = f;
			opt.idleTime = 1000;
			opt.bytesXferred = fsz;
			FtpSetCallback(&opt,conn);
		}
#endif

		returnSz = FtpGet(localFile, remoteFile, mode, client->ctrlConn);
		if(!returnSz)
		{
			MUX_ERROR("Get No. %d remote file '%s', local file '%s' failed", i, remoteFile, localFile);
			return IPCMD_ERR_FTP_DATA_ERROR;
		}
		
		ftpAgent->totalSuccess++;
		ftpAgent->downloadSize += fsz;

		if(cmnMediaCheckSDCardFile(localFile) )
		{
			cmnMediaRemountSDCard(FALSE);
		}

		cmn_list_append(&client->downloadResults, strdup(localFile));
		MUX_DEBUG("%d bytes have been transferred sucessfully for '%s'", fsz, localFile);
	}
	
	return EXIT_SUCCESS;
}

static void _cmnFtpClientClose(CMN_FTP_CLIENT *client)
{
	cmn_list_ofchar_free(&client->downloadResults, FALSE);
	cmn_list_ofchar_free(client->downloadFileList, TRUE);
	
	if (client->ctrlConn)
	{
		FtpClose(client->ctrlConn);
		client->ctrlConn = NULL;
	}
	
	cmn_free(client);
}


static int _cmnFtpHandlerDownload(void *priv, struct DATA_CONN *dataConn, CMN_PLAY_JSON_EVENT *jEvent)
{
	struct	CMN_FTP_CONTROLLER *ftpAgent = (struct	CMN_FTP_CONTROLLER *)priv;
	int res = EXIT_SUCCESS;
	CMN_FTP_CLIENT *client = (CMN_FTP_CLIENT *)jEvent->object;
	cJSON *fileObjs = NULL;
	int	i;

#if 0//ARCH_ARM
	if (SYSTEM_IS_EXITING())
	{
		MUX_WARN(CMN_LOG_WARNING, "Thread '%s' recv EXIT signal\n", th->name);
		return -EXIT_FAILURE;
	}
#endif

	dataConn->errCode = IPCMD_ERR_NOERROR;
	ftpAgent->currentReq = client;
	res = _cmnFtpClientConnect(client, ftpAgent);
	if( res == EXIT_SUCCESS)
	{
		res = _cmnFtpClientGetFiles(client, ftpAgent);
	}

	if(res != EXIT_SUCCESS)
	{
		dataConn->errCode = res;
		return EXIT_SUCCESS;
	}

	if( (cmn_list_size(&client->downloadResults) /*!= cmn_list_size(&client->downloadFileList)*/ )
		&& dataConn->errCode != IPCMD_ERR_NOERROR)
	{
		MUX_DEBUG( "DATA_CONN error code: '%d' ", dataConn->errCode);
		dataConn->errCode = IPCMD_ERR_FTP_PARTLY_FAILED;
	}
	
	fileObjs = cJSON_CreateArray();
	for(i=0; i< cmn_list_size(&client->downloadResults); i++)
	{
		char	*filename = (char *)cmn_list_get(&client->downloadResults, i);
		MUX_DEBUG( "No.%d: '%s' have been saved sucessfully", i+1, filename);
		cJSON_AddItemToArray(fileObjs, cJSON_CreateString(filename));
	}
	dataConn->resultObject = fileObjs;
	
	_cmnFtpClientClose(client);

	return EXIT_SUCCESS;
}


static PluginJSonHandler _jsonFtpHandlers[] =
{
	{
		.type	= CMD_TYPE_SET_MEDIA,
		.name 	= "download",
		handler	: _cmnFtpHandlerDownload
	},
	{
		.type	= CMD_TYPE_UNKNOWN,
		.name	= NULL,
		.handler	= NULL
	}
};

static int	_ftpEventHandler(struct _CmnThread *th, void *_event)
{
	struct	CMN_FTP_CONTROLLER *ftpAgent = (struct	CMN_FTP_CONTROLLER *)th->data;

	CMN_PLAY_JSON_EVENT *jsonEvent = (CMN_PLAY_JSON_EVENT *)_event;
	int res = EXIT_SUCCESS;

	res = cmnMuxJSonPluginHandle(ftpAgent, _jsonFtpHandlers, jsonEvent);

#if 0	
	if (SYSTEM_IS_EXITING())
	{
		MUX_PLAY_WARN( "'%s' Thread recv EXIT signal\n", th->name );
		return -EXIT_FAILURE;
	}
#endif
	return res;
}


static int _initFtpThread(CmnThread *th, void *data)
{
	int res = EXIT_SUCCESS;
//	MuxPlayerConfig *cfg = (MuxPlayerConfig *)data;
	char *localPath = (char *)data ;

	struct CMN_FTP_CONTROLLER *ftpAgent = (struct CMN_FTP_CONTROLLER *)cmn_malloc(sizeof(struct CMN_FTP_CONTROLLER));

//	localPath = cfg->usbHome;
	res = _checkAndMakeDirectory(localPath);
	if(res != EXIT_SUCCESS)
		return res;
	
	FtpInit();

	snprintf(ftpAgent->localPath, CMN_NAME_LENGTH, "%s", localPath);
	ftpAgent->priv = data;
	cmnThreadMask(th->name);
	th->data = ftpAgent;
	
	return EXIT_SUCCESS;
}

static void _destoryFtpThread(struct _CmnThread *th)
{
	struct	CMN_FTP_CONTROLLER *ftpAgent = (struct	CMN_FTP_CONTROLLER *)th->data;

	cmn_free(ftpAgent);
}



CmnThread  threadCmnFtp =
{
	name		:	"FTP",
	flags		:	SET_BIT(1, CMN_THREAD_FLAG_WAIT),
	
	init			:	_initFtpThread,
	mainLoop		:	NULL,
	eventHandler	:	_ftpEventHandler,
	destory		:	_destoryFtpThread,
	data			:	NULL,
};


int cmnMuxFtpAddEvent(char *server, 	char *username, char *password, char *path, cmn_list_t *list, void *dataConn)
{
	CMN_FTP_CLIENT *client =(CMN_FTP_CLIENT *)cmn_malloc(sizeof(CMN_FTP_CLIENT));

	CMN_PLAY_JSON_EVENT *jsonEvent =(CMN_PLAY_JSON_EVENT *)cmn_malloc(sizeof(CMN_PLAY_JSON_EVENT));

	cmnFtpClientInitDefault(client);

	snprintf(client->username, CMN_NAME_LENGTH, "%s", username);
	snprintf(client->password, CMN_NAME_LENGTH, "%s", password);
	
	snprintf(client->host, CMN_NAME_LENGTH, "%s", server);
	
	snprintf(client->remotePath, CMN_NAME_LENGTH, "%s", path);
//	snprintf(client->filename, CMN_NAME_LENGTH, "%s", file);

	client->downloadFileList = list;
	client->priv = dataConn;
	cmn_list_init( &client->downloadResults);

	snprintf(jsonEvent->action, JSON_ACTION_LENGTH, "%s", "download");
	jsonEvent->object = (cJSON *)client;
	jsonEvent->priv = dataConn;

	return cmnThreadAddEvent(&threadCmnFtp, jsonEvent);
}

