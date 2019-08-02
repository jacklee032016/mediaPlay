/*
* Client of socket server, eg. UDP/TCP/Unix socket client, send JSON request and receive jSON response.. 
* used in webAdmin (CGI)
*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"


#include "_cmnMux.h"


void cmnMuxClientConnDestroy(struct CLIENT_CONN *clientConn)
{
	close(clientConn->socket);
	cmn_free(clientConn);
}

struct CLIENT_CONN *cmnMuxClientConnCreate(CTRL_LINK_TYPE type, int port, char *address, int timeoutSeconds)
{
	int res = EXIT_SUCCESS;
	struct	CLIENT_CONN *clientConn = NULL;
	struct sockaddr_in addr;
	int sock = 0;

	if(type == CTRL_LINK_UDP)
	{
		sock = socket(AF_INET, SOCK_DGRAM, 0);
	}
	else if(type == CTRL_LINK_TCP)
	{
		sock = socket(AF_INET, SOCK_STREAM, 0);
	}
	else if(type == CTRL_LINK_UNIX)
	{
		sock = socket(AF_UNIX, SOCK_STREAM, 0);
	}
	else
	{
		MUX_ERROR("Ctrl Connection type:'%d' is not support", type);
		exit(1);
	}
	
	if (sock < 0)
	{
		MUX_ERROR("Socket Error:%s", strerror(errno));
		return NULL;
	}

	struct timeval timeout;      
	timeout.tv_sec = timeoutSeconds;
	timeout.tv_usec = 0;

	if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
	{
		MUX_ERROR("Set socket REV timeout error:%s", strerror(errno));
	}

	if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
	{
		MUX_ERROR("Set socket SEND timeout error:%s", strerror(errno));
	}


	if(type == CTRL_LINK_TCP || type == CTRL_LINK_UDP)
	{
		struct hostent *he;

		bzero(&addr, sizeof(struct sockaddr_in));
		he = gethostbyname(address);
		if (!he)
		{
			MUX_ERROR("Invalid host or ip address '%s'", address);
			return NULL;
		}
		else
		{
			/* Only take the first */
//			addr.s_addr = ntohl(((struct in_addr *) he->h_addr_list[0])->s_addr);
//			bcopy((char *)he->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
			memcpy(&addr.sin_addr, he->h_addr_list[0], sizeof(struct in_addr));
		}

		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		res = connect(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
	}
	else
	{/* unix socket */
		struct sockaddr_un unaddr;
		memset(&unaddr, 0, sizeof(unaddr));
		
		char *socket_path = UNIX_SOCKET_PATH;
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

		res = connect(sock, (struct sockaddr*)&unaddr, sizeof(unaddr));
	}

	if(res < 0)
	{
		MUX_ERROR("Bind Error:%s", strerror(errno));
		close(sock);
		return NULL;
	}

	clientConn = cmn_malloc(sizeof(struct CLIENT_CONN));
	clientConn->socket = sock;
	clientConn->type = type;
	clientConn->port = port;
	clientConn->errCode = IPCMD_ERR_NOERROR;
	clientConn->timeout = timeoutSeconds;
	
	if(type == CTRL_LINK_UDP ||type == CTRL_LINK_TCP )
	{
		clientConn->addrlen = sizeof(struct sockaddr);
		memcpy(&clientConn->peerAddr, &addr, clientConn->addrlen);
	}

	return clientConn;
}


int cmnMuxClientConnSendRequest(struct CLIENT_CONN *clientConn, void *buf, int size)
{
	int len = 0;
	CMN_IP_COMMAND cmd;
	unsigned int crc32 =0;
	
	cmd.tag = CMD_TAG_A;
	cmd.length = htons(size + 4);

	memcpy( cmd.data, buf, size);
	crc32 =  htonl(cmnMuxCRC32b(buf, size));
	*((unsigned int *)(cmd.data+size)) = crc32;

#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	MUX_DEBUG( "CtrlClient send %d bytes packet to %s:%d, CRC=0x%x", size+8, inet_ntoa(clientConn->peerAddr.sin_addr), ntohs(clientConn->peerAddr.sin_port), crc32);
	cmnHexDump((const uint8_t *)&cmd, size+8);
#endif
	if(clientConn->type == CTRL_LINK_TCP || clientConn->type == CTRL_LINK_UNIX)
	{
		len = write(clientConn->socket, &cmd, size+8 );
		if (len < 0)
		{
			MUX_ERROR("ERROR writing to socket: '%s'", strerror(errno));
		}
		//close(clientConn->socket);
	}
	else if(clientConn->type == CTRL_LINK_UDP)
	{
		len = sendto(clientConn->socket, &cmd, size+8, 0, (struct sockaddr *)&clientConn->peerAddr, clientConn->addrlen); 
	}

	return len;
}


cJSON *cmnMuxClientConnReadReponse(struct CLIENT_CONN *clientConn)
{
	unsigned int crcDecoded, crcReceived;
	cJSON	*responseObjs = NULL;
	int len = 0;

	CMN_IP_COMMAND responseBuf;
	
	memset(&responseBuf, 0, sizeof(CMN_IP_COMMAND));

	if(clientConn->type == CTRL_LINK_TCP || clientConn->type == CTRL_LINK_UNIX)
	{
#if 1	
		len = read(clientConn->socket, (uint8_t *)&responseBuf, sizeof(CMN_IP_COMMAND));
#else
		len = recv(clientConn->socket, (uint8_t *)&responseBuf, sizeof(CMN_IP_COMMAND), 0);
#endif

#if MUX_OPTIONS_DEBUG_IP_COMMAND			
		MUX_DEBUG("TCP/UNIX Received %d bytes packet from %s:%d", len, inet_ntoa(clientConn->peerAddr.sin_addr), ntohs(clientConn->peerAddr.sin_port));
#endif
	}
	else
	{
		len = recvfrom(clientConn->socket, (uint8_t *)&responseBuf, sizeof(CMN_IP_COMMAND), 0, (struct sockaddr *) &clientConn->peerAddr, &clientConn->addrlen);
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
		MUX_DEBUG("UDP Received %d bytes packet from %s:%d", len, inet_ntoa(clientConn->peerAddr.sin_addr), ntohs(clientConn->peerAddr.sin_port));
#endif
	}
	
	if(len == -1 && (errno == EAGAIN || errno== ESPIPE ||errno== EINPROGRESS||errno== EWOULDBLOCK))
	{ // timed out before any data read : EINTR
		MUX_WARN("Timeout after %d seconds for this command from %s:%d", clientConn->timeout, inet_ntoa(clientConn->peerAddr.sin_addr), ntohs(clientConn->peerAddr.sin_port));
		return NULL;
	}
	else if(len == 0)
	{
		MUX_WARN("Connection is shutdown by peer" );
//		errCode = IPCMD_ERR_COMMUNICATION;
		return NULL;
	}
	else if(len < 0)
	{
		MUX_ERROR("Receive data error: %s(%d)", strerror(errno), errno);
		return NULL;
	}
	
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	MUX_DEBUG("Received %d bytes packet from %s:%d", len, inet_ntoa(clientConn->peerAddr.sin_addr), ntohs(clientConn->peerAddr.sin_port));
	cmnHexDump( (uint8_t *)&responseBuf, len);
#endif

	responseBuf.length = ntohs(responseBuf.length);

	if (CMD_TAG_B != responseBuf.tag)
	{
		MUX_ERROR("Tag of command is wrong:0x%x", responseBuf.tag);
		clientConn->errCode = IPCMD_ERR_WRONG_PROTOCOL;
		return NULL;
	}

	if(responseBuf.length+4 != len )
	{
		MUX_ERROR("length field:%d, received length:%d", responseBuf.length, len);
		clientConn->errCode = IPCMD_ERR_CRC_FAIL;
		return NULL;
	}
	
	crcReceived = *((unsigned int *)(responseBuf.data+responseBuf.length-4));	/* the first 4 bytes are header of command, so here is the position of CRC */
	crcDecoded = htonl(cmnMuxCRC32b(responseBuf.data, responseBuf.length-4));
	if (crcReceived != crcDecoded)
	{//Wrong CRC
		//sendto(sockCtrl, msg_reply, n /*+ 8*/, 0, (struct sockaddr *) &addr, addrlen); //  if suceeded the updated data is send back
		MUX_ERROR("CRC of command is wrong:received CRC: 0x%x, Decoded CRC:0x%x", crcReceived, crcDecoded);
		clientConn->errCode = IPCMD_ERR_CRC_FAIL;
		return NULL;
	}
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	MUX_DEBUG("received CRC: 0x%x, Decoded CRC:0x%x", crcReceived, crcDecoded);
#endif

	responseObjs = cJSON_Parse((char *)responseBuf.data);
	if( responseObjs == NULL)
	{/*Wrong JSON format */
		MUX_ERROR("JSON of command is wrong:received JSON: '%s'", (char *)responseBuf.data);
		clientConn->errCode = IPCMD_ERR_JSON_CORRUPTED;
	}

	return responseObjs;
}

static cJSON *findJSonObject(cJSON *array, char *key, char *valuestring)
{
	int i;

	if(!cJSON_IsArray(array))
	{
		MUX_ERROR("cJSON is not an array object");
		return NULL;
	}

	for (i = 0; i < cJSON_GetArraySize(array); i++)
	{

		cJSON *subitem = cJSON_GetArrayItem(array, i);
		if(!subitem)
		{
			MUX_ERROR("IP Command configuration file '%s' initialization error", IP_COMMAND_CONFIG_FILE);
			return NULL;
		}
		
		cJSON *temp = cJSON_GetObjectItem(subitem, key);
		if( !strcasecmp(valuestring, temp->valuestring) )
		{
			return subitem;
		}
	}

	return NULL;
}


CLIENT_CTRL _clientCtrl;

static int _getJSonHandlerFromFile(char *jsonFilename, cJSON **handler)
{
	*handler = cmnMuxJsonLoadConfiguration(jsonFilename);
	if (*handler== NULL)
	{
		MUX_ERROR("IP Command configuration file '%s' Parsing failed", jsonFilename);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int cmnMuxClientInit(int port, CTRL_LINK_TYPE type, char *serverAddress, int timeoutSeconds)
{
	CLIENT_CTRL	*clientCtrl = &_clientCtrl;
	struct CLIENT_CONN *clientConn;

	memset(clientCtrl, 0, sizeof(CLIENT_CTRL));

	clientConn = cmnMuxClientConnCreate(type, port, serverAddress, timeoutSeconds);
	if(clientConn == NULL)
	{
		MUX_ERROR("Connect to '%s:%d' failed", serverAddress, port);
		return EXIT_FAILURE;
	}
	
	if(_getJSonHandlerFromFile(IP_COMMAND_CONFIG_FILE, &clientCtrl->ipCmds) )
	{
		goto failed;
	}

	if(_getJSonHandlerFromFile(MEDIA_ACTION_CONFIG_FILE, &clientCtrl->mediaActions))
	{
		goto failed;
	}

	if(_getJSonHandlerFromFile(PLAY_ACTION_CONFIG_FILE, &clientCtrl->playActions))
	{
		goto failed;
	}

	if(_getJSonHandlerFromFile(RECORD_ACTION_CONFIG_FILE, &clientCtrl->recordActions))
	{
		goto failed;
	}


	if(_getJSonHandlerFromFile(SEVER_ACTION_CONFIG_FILE, &clientCtrl->serviceActions))
	{
		goto failed;
	}

	if(_getJSonHandlerFromFile(WEB_ACTION_CONFIG_FILE, &clientCtrl->webActions) )
	{
		goto failed;
	}

	if(_getJSonHandlerFromFile(SYS_ADMIN_CONFIG_FILE, &clientCtrl->sysAdminActions))
	{
		goto failed;
	}
	
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	{
		char *printed_json = cJSON_Print(clientCtrl->playActions);
		fprintf(stderr,"FormatPrint PlayActions:\n'%s'\n", printed_json);
		cmn_free(printed_json);
	}
#endif

	clientCtrl->conn = clientConn;
	clientCtrl->inited = TRUE;
	
	return EXIT_SUCCESS;

failed:

	if(clientCtrl->ipCmds)
		cJSON_Delete(clientCtrl->ipCmds);
	if(clientCtrl->mediaActions)
		cJSON_Delete(clientCtrl->mediaActions);
	if(clientCtrl->playActions)
		cJSON_Delete(clientCtrl->playActions);
	if(clientCtrl->recordActions)
		cJSON_Delete(clientCtrl->recordActions);
	if(clientCtrl->serviceActions)
		cJSON_Delete(clientCtrl->serviceActions);
	if(clientCtrl->webActions)
		cJSON_Delete(clientCtrl->webActions);

	cmnMuxClientConnDestroy(clientConn);
	return EXIT_FAILURE;
}


void cmnMuxClientDestroy(void)
{
	CLIENT_CTRL	*clientCtrl = &_clientCtrl;
	cJSON_Delete(clientCtrl->ipCmds);
	cJSON_Delete(clientCtrl->mediaActions);
	cJSON_Delete(clientCtrl->playActions);
	cJSON_Delete(clientCtrl->recordActions);
	cJSON_Delete(clientCtrl->serviceActions);
	cJSON_Delete(clientCtrl->webActions);

	cmnMuxClientConnDestroy(clientCtrl->conn);
}


/* replace 'data' object in IP command object and send it to API controller */
cJSON *cmnMuxClientSendout(char *ipcmdName, cJSON *dataArray)
{
	CLIENT_CTRL *clientCtrl = &_clientCtrl;
	int res = EXIT_SUCCESS;
	cJSON *response = NULL;

	cJSON *ipCmd = findJSonObject(clientCtrl->ipCmds, "cmd", ipcmdName);
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	{
		char *printed_json = cJSON_Print(ipCmd);
		fprintf(stderr,"FormatPrint ipCmd:\n'%s'\n", printed_json);
		cmn_free(printed_json);
	}
#endif

	if(ipCmd == NULL)
	{
		MUX_ERROR("'%s' is not validate IP Command", ipcmdName);
		return NULL;
	}
	
	cJSON_ReplaceItemInObject(ipCmd, IPCMD_NAME_KEYWORD_DATA, dataArray);


	char *msg = cJSON_PrintUnformatted(ipCmd);

#if 1//MUX_OPTIONS_DEBUG_IP_COMMAND			
	{
		char *printed_json = cJSON_Print(ipCmd);
		fprintf(stderr,"Sendout ipCmd:\n'%s'\n", printed_json);
		cmn_free(printed_json);
	}
#endif
	res = cmnMuxClientConnSendRequest(clientCtrl->conn, msg, strlen(msg));
	cmn_free(msg);

	if(res == -1 && errno == EINTR)
	{ // timed out before any data read
		MUX_ERROR("Timeout when send out command to %s:%d. Please check your parameters", inet_ntoa(clientCtrl->conn->peerAddr.sin_addr), ntohs(clientCtrl->conn->peerAddr.sin_port));
		return NULL;
	}
	if(res <= 0)
	{
		MUX_ERROR("IP command '%s' send failed, maybe the API server is not running!", ipcmdName);
		return NULL;
	}

	response = cmnMuxClientConnReadReponse(clientCtrl->conn);

	{
		if(response )
		{
			char *printed_json = cJSON_Print(response);
			fprintf(stderr, "reply:\n'%s'\n", printed_json);
			cmn_free(printed_json);
		}
	}


	return response;
}

/*
* replace the 'action' object in xxxxAction.json, and send to next step
*/
cJSON *cmnMuxClientRequest( char *ipcmdName, char *actionName, cJSON *obj)
{
//	int res = EXIT_SUCCESS;
	CLIENT_CTRL *clientCtrl = &_clientCtrl;
	cJSON *actionObj = NULL;
	cJSON *objsArray=NULL, *dataArray = NULL;

	if(actionName == NULL)
	{
		MUX_ERROR("IP command '%s' has no action", ipcmdName);
		return NULL;
	}
		
	if(clientCtrl->inited == 0)
	{
		MUX_ERROR("Ctrl Client is not inited now, please call cmnMuxClientInit() first!" );
		return NULL;
	}

	if(!strcasecmp( ipcmdName, IPCMD_NAME_SYS_ADMIN) )
	{
		actionObj = findJSonObject(clientCtrl->sysAdminActions, MEDIA_CTRL_ACTION, actionName);
	}	
	else if(!strcasecmp( ipcmdName, IPCMD_NAME_MEDIA_RECORDER) )
	{
		actionObj = findJSonObject(clientCtrl->recordActions, MEDIA_CTRL_ACTION, actionName);
	}	
	else if(!strcasecmp( ipcmdName, IPCMD_NAME_MEDIA_SERVER) )
	{
		actionObj = findJSonObject(clientCtrl->serviceActions, MEDIA_CTRL_ACTION, actionName);
	}
	else if(!strcasecmp( ipcmdName, IPCMD_NAME_MEDIA_WEB) )
	{
		actionObj = findJSonObject(clientCtrl->webActions, MEDIA_CTRL_ACTION, actionName);
	}
	else if(!strcasecmp( ipcmdName, IPCMD_NAME_MEDIA_PLAY) )
	{
		actionObj = findJSonObject(clientCtrl->playActions, MEDIA_CTRL_ACTION, actionName);
	}	
	else //if(!strcasecmp( ipcmdName, IPCMD_NAME_MEDIA_PLAY) )
	{
		actionObj = findJSonObject(clientCtrl->mediaActions, MEDIA_CTRL_ACTION, actionName);
	}	

	if(actionObj == NULL)
	{
		MUX_ERROR("IP command '%s' action '%s' is not validate", ipcmdName, actionName);
		return NULL;
	}

#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	{
		char *printed_json = cJSON_Print(actionObj);
		fprintf(stderr,"FormatPrint actionObj before replaced:\n'%s'\n", printed_json);
		cmn_free(printed_json);
	}
#endif

	/* add object in one array */
	objsArray = cJSON_CreateArray();
	cJSON_AddItemToArray(objsArray, obj);

	/* 'objects' must be a array */
	cJSON_ReplaceItemInObject(actionObj, MEDIA_CTRL_OBJECTS, objsArray);
	
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	{
		char *printed_json = cJSON_Print(actionObj);
		fprintf(stderr,"FormatPrint actionObj:\n'%s'\n", printed_json);
		cmn_free(printed_json);
	}
#endif

	/* must copy it from serial CJSON objects without recurse */
	cJSON *newAct = cJSON_Duplicate(actionObj, 1);

	dataArray = cJSON_CreateArray();
	cJSON_AddItemToArray(dataArray, newAct);
#if MUX_OPTIONS_DEBUG_IP_COMMAND			
	{
		char *printed_json = cJSON_Print(dataArray);
		fprintf(stderr,"FormatPrint dataArray:\n'%s'\n", printed_json);
		cmn_free(printed_json);
	}
#endif


	return cmnMuxClientSendout( ipcmdName, dataArray);
}


