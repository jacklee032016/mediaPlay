
/*
* Controller: 
*   1. communicate with controller 811; 
*   2. finish the commands (FTP download commands) 
*   3. or send to commands to correspoding threads, such as Play/Stop
*/

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "_cmnMux.h"

static struct CTRL_CONN *_createCtrlConnection(CTRL_LINK_TYPE type, MuxMain *muxMain)
{
	int res = EXIT_SUCCESS;
	struct	CTRL_CONN *ctrlConn = NULL;
	char *socket_path = NULL; 
	int port = 0;
	
	int sockCtrl = 0;

	if(type == CTRL_LINK_UDP)
	{
		sockCtrl = socket(AF_INET, SOCK_DGRAM, 0);
	}
	else if(type == CTRL_LINK_TCP)
	{
		sockCtrl = socket(AF_INET, SOCK_STREAM, 0);
	}
	else if(type == CTRL_LINK_UNIX)
	{
		sockCtrl = socket(AF_UNIX, SOCK_STREAM, 0);
	}
	else
	{
		MUX_ERROR("Ctrl Connection type:'%d' is not support", type);
		exit(1);
	}
	
	if (sockCtrl < 0)
	{
		MUX_ERROR("Socket Error:%s", strerror(errno));
		return NULL;
	}


	if(type == CTRL_LINK_TCP || type == CTRL_LINK_UDP)
	{
		struct sockaddr_in addr;
		int yes;
		
		port = (type==CTRL_LINK_TCP)?muxMain->tcpCtrlPort:muxMain->udpCtrlPort;
		if (setsockopt(sockCtrl, SOL_SOCKET, SO_REUSEADDR, (char *) &yes, sizeof(yes)) < 0)
		{
			MUX_ERROR("Reusing ADDR failed");
			close(sockCtrl);
			return NULL;
		}

		bzero(&addr, sizeof(struct sockaddr_in));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(port);
		res = bind(sockCtrl, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
	}
	else
	{/* unix socket */
		struct sockaddr_un unaddr;
		memset(&unaddr, 0, sizeof(unaddr));
		
		socket_path = IS_STRING_NULL(muxMain->unixPort)? UNIX_SOCKET_PATH:muxMain->unixPort;
		
		unaddr.sun_family = AF_UNIX;

		if ( 0)//*socket_path == '\0')
		{
			*unaddr.sun_path = '\0';
			strncpy(unaddr.sun_path+1, socket_path+1, sizeof(unaddr.sun_path)-2);
		}
		else
		{
			strncpy(unaddr.sun_path, socket_path, sizeof(unaddr.sun_path)-1);
			unlink(socket_path);
		}

		res = bind(sockCtrl, (struct sockaddr*)&unaddr, sizeof(unaddr));
	}

	if(res < 0)
	{
		MUX_ERROR("Bind Error:%s", strerror(errno));
		close(sockCtrl);
		return NULL;
	}

#if 0
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(DEFAULT_MC_ADDRESS);         
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);         
	if (setsockopt(udpConn->sockCtrl, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
	{
		perror("setsockopt mreq");
		exit(1);
	}         
#endif

	if(type == CTRL_LINK_TCP || type == CTRL_LINK_UNIX)
	{
		if(listen(sockCtrl, 5) == -1)
		{
			MUX_ERROR("Bind Error:%s", strerror(errno));
			close(sockCtrl);
			return NULL;
		}
	}

	ctrlConn = cmn_malloc(sizeof(struct CTRL_CONN));
	ctrlConn->sockCtrl = sockCtrl;
	ctrlConn->type = type;
	ctrlConn->port = port;

	if(type == CTRL_LINK_UNIX)
	{
		MUX_DEBUG("Unix Controller in port %s", socket_path);
	}
	else
	{
		MUX_DEBUG("%s Controller in port %d", (type == CTRL_LINK_TCP)?"TCP":"UDP", port);
	}
	
	return ctrlConn;
}

/* reply error message for errors which happened before CMD cJSON objects has been parsed */
static int _cmnMuxJsonReplyErrorBeforeCmdObject(struct DATA_CONN *dataConn)
{
	char *errorData = cmnMuxCreateErrReply(dataConn->errCode, NULL);
	
	cmnMuxCtrlResponse(dataConn, errorData, strlen(errorData));

	cmn_free(errorData);
	dataConn->isFinished = TRUE;

	return EXIT_SUCCESS;
}

/* read command and parse cmdObj for every DATA_CONN */
static struct DATA_CONN *_readCommand(struct CTRL_CONN *ctrlConn)
{
	int dataSocket = 0;
	unsigned int crcDecoded, crcReceived;
	struct DATA_CONN *dataConn = NULL;
	cJSON				*cmdObjs;
	struct sockaddr_in		peerAddr;
//	struct sockaddr 		peerAddr;
	socklen_t				addrlen;
	int len = 0;
//	int errCode = IPCMD_ERR_NOERROR;
//	char *errorData;
	
	memset(&ctrlConn->cmdBuffer, 0, sizeof(CMN_IP_COMMAND));

	addrlen = sizeof(struct sockaddr);
	if(ctrlConn->type == CTRL_LINK_TCP || ctrlConn->type == CTRL_LINK_UNIX)
	{
		dataSocket = accept(ctrlConn->sockCtrl, (struct sockaddr *) &peerAddr, &addrlen);
		if(dataSocket < 0)
		{
			MUX_ERROR("accept Error:%s", strerror(errno));
			return NULL;
		}
		len = read(dataSocket, (uint8_t *)&ctrlConn->cmdBuffer, sizeof(CMN_IP_COMMAND));
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
		MUX_DEBUG("TCP/UNIX Received %d bytes packet from %s:%d", len, inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));
#endif
	}
	else
	{
		len = recvfrom(ctrlConn->sockCtrl, (uint8_t *)&ctrlConn->cmdBuffer, sizeof(CMN_IP_COMMAND), 0, (struct sockaddr *) &peerAddr, &addrlen);
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
		MUX_DEBUG("UDP Received %d bytes packet from %s:%d", len, inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));
#endif
	}

	if(len == 0)
	{
		MUX_INFO("Connection is broken by client");
		return NULL;
	}
	if(len<0)
	{
		MUX_ERROR("Receive data error: %s", strerror(errno));
//		errCode = IPCMD_ERR_COMMUNICATION;
		return NULL;
	}
	
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	MUX_DEBUG("Received %d bytes(%d length) packet from %s:%d", len, ntohs(ctrlConn->cmdBuffer.length), inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));
	cmnHexDump( (uint8_t *)&ctrlConn->cmdBuffer, len);
#endif
	ctrlConn->cmdBuffer.length = ntohs(ctrlConn->cmdBuffer.length);

#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	MUX_DEBUG("received length: %d, length field: %d", len, ctrlConn->cmdBuffer.length);
#endif

	if(ctrlConn->cmdBuffer.length+4 != len )
	{
		MUX_ERROR("length field:%d, received length:%d", ctrlConn->cmdBuffer.length, len);
		dataConn->errCode = IPCMD_ERR_WRONG_PROTOCOL;
		return NULL;
	}
	
	dataConn = cmn_malloc(sizeof(struct DATA_CONN));

	dataConn->errCode = IPCMD_ERR_NOERROR;
	dataConn->sock = dataSocket;
	dataConn->port =  ntohs(peerAddr.sin_port);
	memcpy(&dataConn->peerAddr, &peerAddr, addrlen);
	dataConn->addrlen = addrlen;
	dataConn->ctrlConn = ctrlConn;
	dataConn->mutexLock = cmn_mutex_init();


	if (CMD_TAG_A != ctrlConn->cmdBuffer.tag)
	{
		MUX_ERROR("Tag of command is wrong:0x%x", ctrlConn->cmdBuffer.tag);
		dataConn->errCode = IPCMD_ERR_WRONG_PROTOCOL;
		goto Failed;
	}

	crcReceived = *(unsigned int *)(ctrlConn->cmdBuffer.data+ctrlConn->cmdBuffer.length-4);	/* the first 4 bytes are header of command, so here is the position of CRC */
	crcDecoded = htonl(cmnMuxCRC32b(ctrlConn->cmdBuffer.data, ctrlConn->cmdBuffer.length-4));
	if (crcReceived != crcDecoded)
	{//Wrong CRC
		//sendto(sockCtrl, msg_reply, n /*+ 8*/, 0, (struct sockaddr *) &addr, addrlen); //  if suceeded the updated data is send back
		MUX_ERROR("CRC of command is wrong:received CRC: 0x%x, Decoded CRC:0x%x", crcReceived, crcDecoded);
		dataConn->errCode = IPCMD_ERR_CRC_FAIL;
		goto Failed;
	}
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	MUX_DEBUG( "received CRC: 0x%x, Decoded CRC:0x%x", crcReceived, crcDecoded);
#endif

	cmdObjs = cJSON_Parse((char *)ctrlConn->cmdBuffer.data);
	if( cmdObjs == NULL)
	{/*Wrong JSON format */
		MUX_ERROR("JSON of command is wrong:received JSON: '%s'", (char *)ctrlConn->cmdBuffer.data);
		dataConn->errCode = IPCMD_ERR_JSON_CORRUPTED;
		goto Failed;
	}

	dataConn->cmdObjs = cmdObjs;
Failed:
	return dataConn;
}

void cmnMuxDataConnClose(struct DATA_CONN *dataConn)
{
	if( (dataConn)->cmdObjs)
	{
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
		MUX_DEBUG("cJSON_Delete cmdObjs:%p", dataConn->cmdObjs);	
#endif
		cJSON_Delete( (dataConn)->cmdObjs);
	}
	
	if( (dataConn)->resultObject)
	{/* when is it been freed?? */
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
		MUX_DEBUG("cJSON_Delete cmdObjs:%p", dataConn->resultObject);	
#endif
//		cJSON_Delete( (dataConn)->resultObject);
	}

	close((dataConn)->sock);
	cmn_mutex_destroy(dataConn->mutexLock);
	
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	MUX_DEBUG("free dataConn:%p", dataConn);	
#endif
	cmn_free((dataConn));
}

/* controller interface of sending back all JSON reply which is in format of buf. Only interface from controller called by other modules  */
int cmnMuxCtrlResponse(struct DATA_CONN *dataConn, void *buf, int size)
{
	int len = 0;
	CMN_IP_COMMAND cmd;
	unsigned int crc32 =0;
	
	cmd.tag = CMD_TAG_B;
	cmd.length =  htons(size + 4);

	memcpy( cmd.data, buf, size);
	crc32 =  htonl(cmnMuxCRC32b(buf, size));
	*(unsigned int *)(cmd.data+size) = crc32;


#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	MUX_DEBUG("Reply %d bytes packet to %s:%d", size+8, inet_ntoa(dataConn->peerAddr.sin_addr), ntohs(dataConn->peerAddr.sin_port));
	cmnHexDump((const uint8_t *)&cmd, size+8);
#endif
	if(dataConn->ctrlConn->type == CTRL_LINK_TCP || dataConn->ctrlConn->type == CTRL_LINK_UNIX)
	{
		len = write(dataConn->sock, &cmd, size+8 );
		if (len < 0)
		{
			MUX_ERROR("ERROR writing to socket: '%s'", strerror(errno));
		}
		close(dataConn->sock);
	}
	else if(dataConn->ctrlConn->type == CTRL_LINK_UDP)
	{
		len = sendto(dataConn->ctrlConn->sockCtrl, &cmd, size+8, 0, (struct sockaddr *)&dataConn->peerAddr, dataConn->addrlen); //  if suceeded the updated data is send back
	}

	return len;
}


#define	CONTROLLER_ADD_CTRL_CONN(controller, ctrlConn) \
	{ if((ctrlConn)==NULL){exit(1);} (ctrlConn)->controller =(controller); if((controller)->ctrlConns==NULL) {(controller)->ctrlConns=(ctrlConn);} \
	else{(ctrlConn)->next=(controller)->ctrlConns; (controller)->ctrlConns = (ctrlConn);} }
	

static CMN_MUX_CONTROLLER *_cmnMuxControllerInit(MuxMain *muxMain)
{
	struct CTRL_CONN *ctrlConn = NULL;
	CMN_MUX_CONTROLLER *controller = NULL;
	cJSON				*cfgHandlers = NULL;

	cfgHandlers = cmnMuxJsonLoadConfiguration(IP_COMMAND_CONFIG_FILE);
	if (cfgHandlers== NULL)
	{
		MUX_ERROR("IP Command configuration file '%s' Parsing failed", IP_COMMAND_CONFIG_FILE);
		return NULL;
	}

	controller = cmn_malloc(sizeof(CMN_MUX_CONTROLLER));

	if(muxMain->udpCtrlPort != 0)
	{
		ctrlConn = _createCtrlConnection(CTRL_LINK_UDP, muxMain);
		CONTROLLER_ADD_CTRL_CONN(controller, ctrlConn);
	}

	if(muxMain->tcpCtrlPort != 0)
	{
		ctrlConn = _createCtrlConnection(CTRL_LINK_TCP, muxMain);
		CONTROLLER_ADD_CTRL_CONN(controller, ctrlConn);
	}

	ctrlConn = _createCtrlConnection(CTRL_LINK_UNIX, muxMain);
	CONTROLLER_ADD_CTRL_CONN(controller, ctrlConn);

	controller->muxMain = muxMain;
//	controller->cfg = muxMain-> cfg;
	controller->cfgHandlers = cfgHandlers;
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	{
		char *printed_json = cJSON_Print(controller->cfgHandlers);
		MUX_DEBUG("FormatPrint:\n'%s'\n", printed_json);
		fprintf(stderr,"FormatPrint:\n'%s'\n", printed_json);
		cmn_free(printed_json);
	}
#endif

	return controller;
}


static void _cmnMuxControllerDestroy(CMN_MUX_CONTROLLER *controller)
{
	struct CTRL_CONN *ctrlConn = controller->ctrlConns;

	while(ctrlConn)
	{
		struct CTRL_CONN *tmp = ctrlConn;
		ctrlConn = tmp->next;
		
		close(tmp->sockCtrl);
		cmn_free(tmp);
	}

	cJSON_Delete(controller->cfgHandlers);
	controller->cfgHandlers = NULL;

	cmn_free(controller);
}



static int _cmnMuxControllerReceive(CMN_MUX_CONTROLLER *controller)
{
	struct CTRL_CONN *ctrlConn = controller->ctrlConns;
	fd_set				readFdSet;
	fd_set				writeFdSet;
	int					maxFd = 0;
	int	res = EXIT_SUCCESS;
	struct timeval req_timeout; /* timeval for select */

	FD_ZERO(&readFdSet);
	FD_ZERO(&writeFdSet);
	
	while(ctrlConn)
	{
		FD_SET(ctrlConn->sockCtrl, &readFdSet);
		if(ctrlConn->sockCtrl > maxFd)
			maxFd = ctrlConn->sockCtrl;
		
		ctrlConn = ctrlConn->next;
	}

	req_timeout.tv_sec = 5;
	req_timeout.tv_usec = 0l; /* reset timeout */

	res = select(maxFd + 1, &readFdSet, &writeFdSet, NULL, &req_timeout);
	if (res == -1 )
	{
		/* what is the appropriate thing to do here on EBADF */
		if (errno == EINTR)
			return EXIT_SUCCESS;
		else if (errno != EBADF)
		{
			MUX_ERROR("select system call failed: '%s'", strerror(errno));
			return EXIT_FAILURE;
		}
	}
	else if(res == 0)/* timeout */
	{
//		MUX_DEBUG("Timeout of socket");
		return EXIT_SUCCESS;
	}

	
	ctrlConn = controller->ctrlConns;
	while(ctrlConn)
	{
		struct DATA_CONN *dataConn = NULL;
		if ( FD_ISSET(ctrlConn->sockCtrl, &readFdSet) )
		{
			dataConn = _readCommand(ctrlConn);
			if(dataConn == NULL)
			{
				return EXIT_SUCCESS;
			}
			
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
			MUX_DEBUG( "Read data from peer port %d", dataConn->port );
#endif

			cmn_mutex_lock(dataConn->mutexLock);
			if(dataConn->errCode != IPCMD_ERR_NOERROR )
			{
				_cmnMuxJsonReplyErrorBeforeCmdObject(dataConn);
			}
			else
			{
				cmnMuxCtrlDataHandle(dataConn);
				if(dataConn->errCode == IPCMD_ERR_NOT_SUPPORT_COMMND)
				{
					dataConn->errCode = IPCMD_ERR_NOT_SUPPORT_COMMND;
					_cmnMuxJsonReplyErrorBeforeCmdObject(dataConn);
				}
			}

			//if(dataConn->isFinished)
			if(dataConn->errCode != IPCMD_ERR_IN_PROCESSING)
			{/* not finished, ie. this dataConn is end to other process */
				cmn_mutex_unlock(dataConn->mutexLock);
				cmnMuxDataConnClose(dataConn);
			}
			else 
			{/* otherwise, send data to other thread to process it */
				cmn_mutex_unlock(dataConn->mutexLock);
			}

#if MUX_OPTIONS_DEBUG_IP_COMMAND			
			MUX_DEBUG( "controller finish this DATA_CONN %p", dataConn);
#endif
			
		}
		
		ctrlConn = ctrlConn->next;
	}

	return EXIT_SUCCESS;
}

static int _muxCtrlMainLoop(CmnThread *th)
{
	int res = EXIT_SUCCESS;
	CMN_MUX_CONTROLLER *controller =(CMN_MUX_CONTROLLER *)th->data;

#if 0//ARCH_ARM
	if (SYSTEM_IS_EXITING())
	{
		MUX_WARN("Thread '%s' recv EXIT signal\n", th->name);
		return -EXIT_FAILURE;
	}
#endif

	res = _cmnMuxControllerReceive(controller);
	
	return res;
}

static int _initCtrlThread(CmnThread *th, void *data)
{
	int res = EXIT_SUCCESS;
	MuxMain *muxMain = (MuxMain *)data;

	CMN_MUX_CONTROLLER *controller = _cmnMuxControllerInit(muxMain);
	if(controller == NULL)
		return -EXIT_FAILURE;

	cmnThreadMask(th->name);
	th->data = controller;

	return res;
}

static void _destoryCtrlThread(struct _CmnThread *th)
{
	CMN_MUX_CONTROLLER *controller = (CMN_MUX_CONTROLLER *)th->data;

	_cmnMuxControllerDestroy(controller);
}


CmnThread  threadController =
{
	name		:	"Controller",
	init			:	_initCtrlThread,
	mainLoop		:	_muxCtrlMainLoop,
	eventHandler	:	NULL,
	destory		:	_destoryCtrlThread,
	
	data			:	NULL,
};


