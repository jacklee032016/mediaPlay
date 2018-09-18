/* 
* All for service socket operations
*/

#include "muxWeb.h"

#ifndef	WIN32
#include <arpa/inet.h>          /* inet_ntoa */
#endif

/* Binds to the existing server_s, based on the configuration string
   in server_ip.  IPv6 version doesn't pay attention to server_ip yet.  */
int bind_server(int sock, char *ip, unsigned int port)
{
#ifdef INET6
	struct sockaddr_in6 server_sockaddr;
	server_sockaddr.sin6_family = PF_INET6;
	memcpy(&server_sockaddr.sin6_addr, &in6addr_any, sizeof (in6addr_any));
	server_sockaddr.sin6_port = htons(port);
#else
	struct sockaddr_in server_sockaddr;
	memset(&server_sockaddr, 0, sizeof server_sockaddr);
#ifdef HAVE_SIN_LEN             /* uncomment for BSDs */
	server_sockaddr.sin_len = sizeof server_sockaddr;
#endif
	server_sockaddr.sin_family = PF_INET;
	if (ip != NULL)
	{
#ifdef HAVE_INET_ATON
		inet_aton(ip, &server_sockaddr.sin_addr);
#elif defined HAVE_INET_ADDR
		server_sockaddr.sin_addr.s_addr = inet_addr(ip);
#else
		server_sockaddr.sin_addr.s_addr = inet_addr(ip);
//		#error "Neither inet_aton nor inet_addr exist!"
#endif
	}
	else
	{
		server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
		server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sockaddr.sin_port = htons(port);
#endif

	return bind(sock, (struct sockaddr *) &server_sockaddr, sizeof (server_sockaddr));
}

char *ascii_sockaddr(struct SOCKADDR *s, char *dest, unsigned int len)
{
#ifdef INET6
	if (getnameinfo((struct sockaddr *) s, sizeof (struct SOCKADDR), dest, len, NULL, 0, NI_NUMERICHOST))
	{
		fprintf(stderr, "[IPv6] getnameinfo failed\n");
		*dest = '\0';
	}
#ifdef WHEN_DOES_THIS_APPLY
#error Dont use memmove
	if ((s->__ss_family == PF_INET6) && IN6_IS_ADDR_V4MAPPED(&(((struct sockaddr_in6 *) s)->sin6_addr)))
	{
#error The following two lines are broken
		memmove(dest, dest + 7, BOA_NI_MAXHOST);
		dest[BOA_NI_MAXHOST] = '\0';
	}
#endif /* ifdef WHEN_DOES_THIS_APPLY */
#else  /* ifdef INET6 */

	unsigned int newlen;
	char *buf;

	/*    memmove(dest, inet_ntoa(s->sin_addr), len); */
	buf = inet_ntoa(s->sin_addr);
	newlen = strlen(buf);
	/* we need newlen + 1 byte to be <= len, thus
	* newlen <= len - 1 is good
	* and newlen > len -1 is bad thus
	*     newlen + 1 > len ==== newlen >= len
	*/
	if (newlen + 1 > len)
	{ /* too many bytes incl. the NUL */
		return NULL;
	}

	memcpy(dest, buf, newlen);
	dest[newlen] = '\0';
#endif /* ifdef INET6 */

	return dest;
}

int net_port(struct SOCKADDR *s)
{
	int p = -1;
#ifdef INET6
	char serv[NI_MAXSERV];

	if (getnameinfo((struct sockaddr *) s, sizeof (struct SOCKADDR),	NULL, 0, serv, sizeof (serv), NI_NUMERICSERV))
	{
		fprintf(stderr, "[IPv6] getnameinfo failed\n");
	}
	else
	{
		p = atoi(serv);
	}
#else
	p = ntohs(s->sin_port);
#endif
	return p;
}


int svcWebConnRead(WEB_CLIENT_CONN *conn, char *buffer, int size)
{
	int bytes = 0;
	
	bytes = RECV(conn->socket, buffer , size);
	if (bytes < 0)
	{
		if (errno == EINTR)
		{
			ECPWS_ERR_INFO("Socket EINTR error\n" );
			return 0;
		}
		else if (errno == EAGAIN || errno == EWOULDBLOCK) /* request blocked */
		{
			ECPWS_ERR_INFO( "Socket blocked\n" );
			return -1;
		}
		
		ECPWS_ERR_INFO( "header read"); /* don't need to save errno because ecpwsLogErrorReq does */
		return 0;
	}
	else if (bytes == 0)
	{
		ECPWS_LOG_INFO("client unexpectedly closed connection.\n" );
		return 0;
	}

	return bytes;
}


int svcWebConnWrite(WEB_CLIENT_CONN *conn, char *buffer, int size)
{
#define	_WITH_MULTIPLE_SEND	0

	int bytes_written;

#if _WITH_MULTIPLE_SEND
	int left = size;
	int start = 0;
	
	while( left > 0)
	{
		bytes_written = SEND(conn->socket, buffer+start, left);
#else	
		bytes_written = SEND(conn->socket, buffer, size);
#endif
		if (bytes_written == -1)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN)
			{/* EWOULDBLOCK, Operation would block */
#if 0				
				cmn_usleep(2000*20);
				continue;
#else				
				/* return 0, so this conn is keeping in current state to send left data again. Jack Lee, Sep.,8, 2017 */
				return 0; /* request blocked at the pipe level because of large data buffer, but keep going */
#endif
			}
			else if (errno == EINTR)
			{
				return 0;
			}
			else
			{
				ECPWS_LOG_REQ_ERR(conn, "ioshuffle write");
				WEB_CONN_RES_ERR(conn, WEB_RES_ERROR);
			}
		}
		else if (bytes_written == 0)
		{
			return 0;
		}

#if _WITH_MULTIPLE_SEND
		left -= bytes_written;
		start += bytes_written;
		ECPWS_DEBUG_INFO("CONN%d: %d bytes send, total %d bytes in %d bytes have been send, %d bytes left\n",conn->index,  bytes_written, start, size, left );
		cmn_usleep(2000);
		
	}
	return start;
#endif

	return bytes_written;
}



unsigned int system_bufsize = 0; /* Default size of SNDBUF given by system */

static int sock_opt = 1;

static unsigned int sockbufsize = SOCKETBUF_SIZE;

#ifdef	WIN32
char *ecpwsNetReadIp4Interface(char *ifName)
{
	DWORD dwError;

	struct hostent *remoteHost;
	struct in_addr addr;
	
	// If the user input is an alpha name for the host, use gethostbyname()
	// If not, get host by addr (assume IPv4)
	if(!ifName || !isalpha(ifName[0]) )
	{
		addr.s_addr = (ifName==NULL)?INADDR_ANY: inet_addr(ifName);
//		ECPWS_LOG_INFO("Calling gethostbyaddr with '%s'\n", ifName);
		if (addr.s_addr == INADDR_NONE)
		{
			ECPWS_FATAL("%s is not a legal IP address\n", ifName);
		}
		TRACE();
#if 1		
		remoteHost = gethostbyaddr((char *) &addr, sizeof(struct in_addr), AF_INET);
#else
		/* it has been an IP address */
		return STRDUP(ifName);
#endif
	}
	 else
	{/* host address is a name */
		ECPWS_LOG_INFO("Calling gethostbyname with '%s'\n", ifName);
		remoteHost = gethostbyname(ifName);
	}

	if (remoteHost == NULL)
	{
		dwError = WSAGetLastError();
		if (dwError != 0)
		{
			if (dwError == WSAHOST_NOT_FOUND)
			{
				ECPWS_FATAL("Host not found\n");
			} 
			else if (dwError == WSANO_DATA)
			{
				ECPWS_FATAL("No data record found\n");
			}
			else
			{
				ECPWS_FATAL("Function failed with error: %ld\n", dwError);
			}
		}
		ECPWS_FATAL("other windows socket error");
	}

	ECPWS_LOG_INFO("\tOfficial name: %s\n", remoteHost->h_name);
	ECPWS_LOG_INFO("\tAlternate names: %s\n", remoteHost->h_aliases);
	ECPWS_LOG_INFO("\tAddress type: ");
	switch (remoteHost->h_addrtype)
	{
		case AF_INET:
			ECPWS_LOG_INFO("AF_INET\n");
			break;
			
		case AF_INET6:
			ECPWS_LOG_INFO("AF_INET\n");
			break;
			
		case AF_NETBIOS:
			ECPWS_LOG_INFO("AF_NETBIOS\n");
			break;
			
		default:
			ECPWS_LOG_INFO(" %d\n", remoteHost->h_addrtype);
			break;
	}
	
	ECPWS_LOG_INFO("\tAddress length: %d\n", remoteHost->h_length);
	addr.s_addr = *(u_long *) remoteHost->h_addr_list[0];
	ECPWS_LOG_INFO("\tFirst IP Address: %s\n", inet_ntoa(addr));

	return STRDUP(inet_ntoa(addr));
}
#else

/*added by lizhijie,2006.11.20 */
char *ecpwsNetReadIp4Interface(char *interfaceName)
{
#define		RETRY_COUNT	10
#define		RETRY_STEP		10	/* 10 seconds */

	int 		sock;
	struct	ifreq ifr;
	struct sockaddr_in *our_ip;
	int		count = 0;

	memset(&ifr, 0, sizeof(struct ifreq));
	if((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0)
	{
		ifr.ifr_addr.sa_family = AF_INET;
retry:		
		STRCPY(ifr.ifr_name, sizeof(ifr.ifr_name), interfaceName);

		if (ioctl(sock, SIOCGIFADDR, &ifr) == 0)
		{
			our_ip = (struct sockaddr_in *) &ifr.ifr_addr;
//			*addr = our_ip->sin_addr.s_addr;
			CloseSocket(sock);
			ECPWS_LOG_INFO("%s (our ip) = %s\n", ifr.ifr_name, inet_ntoa(our_ip->sin_addr));
			
			return STRDUP(inet_ntoa(our_ip->sin_addr));
		}
		
		if(!strcasecmp(interfaceName, "eth0" ) )
		{
			sprintf(ifr.ifr_name, "%s", "ppp0");
			if (ioctl(sock, SIOCGIFADDR, &ifr) == 0)
			{
				our_ip = (struct sockaddr_in *) &ifr.ifr_addr;
				ECPWS_LOG_INFO( "%s (our ip) = %s", ifr.ifr_name, inet_ntoa(our_ip->sin_addr));
				CloseSocket(sock);
				
				return STRDUP(inet_ntoa(our_ip->sin_addr));
			}
		}
		
		sleep(RETRY_STEP);
		count ++;
/*	wait forever, Li Zhijie, 2006.12.14		
		if(count < RETRY_COUNT)
*/		
		{
			ECPWS_FATAL( "No IP Address is allocated to %s, so delay for %d seconds\n", ifr.ifr_name, count*RETRY_STEP );
			goto retry;
		}
		
		CloseSocket(sock);
/*		
		else
		{
			fprintf(stderr, "SIOCGIFADDR failed, is the interface(%s) up and configured?: %m", interfaceName);
			return -1;
		}
		*/
	}
/*	
	else
	{
		DIE( "socket failed when read IP address of device!");
		return -1;
	}
	CloseSocket(sock);
*/	
	ECPWS_FATAL("socket failed when read IP address of device '%s'!socket failed when read IP address of device!", interfaceName );
	return NULL;
}
#endif


#ifndef	WIN32
void build_addr(SOCKADDR_IN_T	*addr, const char* peer, int port, int udp)
{
	int useLookup = 0;
	(void)useLookup;
	(void)udp;

	memset(addr, 0, sizeof(SOCKADDR_IN_T));

#ifndef INET6
	/* peer could be in human readable form */
	if ( (peer != INADDR_ANY) && isalpha((int)peer[0]))
	{
		struct hostent	*_hostent;
#ifdef WOLFSSL_TIRTOS
		_hostent = DNSGetHostByName(peer);
#else
		_hostent = gethostbyname(peer);
#endif

		if (_hostent)
		{
			memcpy(&addr->sin_addr.s_addr,  &/*_hostent->h_addr*/_hostent->h_addr_list[0], _hostent->h_length);
			printf("using ip %s\n",inet_ntoa(addr->sin_addr));
			useLookup = 1;
		}
		else
			perror("no entry for host");
	}
#endif


#ifndef INET6
#ifdef	WIN32
	addr->sin_family = AF_INET_V;
#else
	addr->sin_family = PF_INET;
#endif
	addr->sin_port = htons(port);
	if (peer == INADDR_ANY)
		addr->sin_addr.s_addr = INADDR_ANY;
	else
	{
		if (!useLookup)
			addr->sin_addr.s_addr = inet_addr(peer);
	}
#else
	addr->sin6_family = AF_INET_V;
	addr->sin6_port = htons(port);
	if (peer == INADDR_ANY)
	{
		memcpy(&addr->sin6_addr, &in6addr_any, sizeof (in6addr_any));
//		addr->sin6_addr = in6addr_any;
	}
	else
	{
#ifdef HAVE_GETADDRINFO
		struct addrinfo  hints;
		struct addrinfo* answer = NULL;
		int    ret;
		char   strPort[80];

		memset(&hints, 0, sizeof(hints));

		hints.ai_family   = AF_INET_V;
		hints.ai_socktype = udp ? SOCK_DGRAM : SOCK_STREAM;
		hints.ai_protocol = udp ? IPPROTO_UDP : IPPROTO_TCP;

		SNPRINTF(strPort, sizeof(strPort), "%d", port);
		strPort[79] = '\0';

		ret = getaddrinfo(peer, strPort, &hints, &answer);
		if (ret < 0 || answer == NULL)
			perror("getaddrinfo failed");

		memcpy(addr, answer->ai_addr, answer->ai_addrlen);
		freeaddrinfo(answer);
#else
		printf("no ipv6 getaddrinfo, loopback only tests/examples\n");
		addr->sin6_addr = in6addr_loopback;
#endif
	}
#endif
}
#endif

int webSvrServiceSocketInit(SVR_WEB *webSvr)
{
#define	HW_ADDRESS_ASSIGNED		1
	char	*_serverIp;


#if HW_ADDRESS_ASSIGNED	
	_serverIp = ecpwsNetReadIp4Interface(webSvr->cfg.server_ip );
#endif	

	webSvr->serverSocket = socket(SERVER_PF, SOCK_STREAM, IPPROTO_TCP);
	if (webSvr->serverSocket  == INVALID_SOCKET)
	{
		ECPWS_FATAL("unable to create socket");
	}

#ifndef	WIN32
	/* server socket is nonblocking */
	if (set_nonblock_fd(webSvr->serverSocket ) == -1)
	{
		ECPWS_FATAL("fcntl: unable to set server socket to nonblocking");
	}

	/* close server socket on exec so cgi's can't write to it */
	if (fcntl(webSvr->serverSocket , F_SETFD, 1) == -1)
	{
		ECPWS_FATAL("can't set close-on-exec on server socket!");
	}

	/* reuse socket addr */
	if ((setsockopt(webSvr->serverSocket , SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt, sizeof (sock_opt))) == -1)
	{
		ECPWS_FATAL("setsockopt");
	}
#endif

	/* Internet family-specific code encapsulated in bind_server()  */
#if HW_ADDRESS_ASSIGNED	
	if (bind_server(webSvr->serverSocket , _serverIp, webSvr->cfg.server_port) == SOCKET_ERROR)
#else		
	if (bind_server(webSvr->serverSocket , webSvr->cfg.server_ip, webSvr->cfg.server_port) == SOCKET_ERROR)
#endif
	{
		ECPWS_FATAL("unable to bind");
	}

#if HW_ADDRESS_ASSIGNED
	free(_serverIp);
#endif

	/* listen: large number just in case your kernel is nicely tweaked */
	if (listen(webSvr->serverSocket , webSvr->cfg.backlog) == SOCKET_ERROR)
	{
		ECPWS_FATAL("unable to listen");
	}
	return FAIL;
}

/*
 *Polls the server socket for a request.  If one exists,
 * does some basic initialization and adds it to the ready queue;.
 */
void webSvrNewConnection(SVR_WEB *webSvr)
{
	int socket;/* socket */
	struct SOCKADDR remote_addr; /* address */
	struct SOCKADDR salocal;
	unsigned int remote_addrlen = sizeof (struct SOCKADDR);
	WEB_CLIENT_CONN *conn;  /* connection */
	socklen_t len;

#ifndef INET6
	remote_addr.S_FAMILY = (sa_family_t) 0xdead;
#endif
	socket = accept(webSvr->serverSocket, (struct sockaddr *) &remote_addr, &remote_addrlen);
	if (socket == INVALID_SOCKET)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{/* abnormal error */
			WARN("accept");
		}
		else
		/* no requests */
			;
		
		return;
	}

#ifndef WIN32	
	if (socket >= FD_SETSIZE)
	{
		ECPWS_LOG_INFO("Got socket >= FD_SETSIZE.");
		CloseSocket(socket);
		return;
	}
#endif
	
#ifdef DEBUGNONINET
	/* This shows up due to race conditions in some Linux kernels
	when the client closes the socket sometime between
	the select() and accept() syscalls.
	Code and description by Larry Doolittle <ldoolitt@boa.org>
	*/
#define HEX(x) (((x)>9)?(('a'-10)+(x)):('0'+(x)))

	if (remote_addr.sin_family != PF_INET)
	{
		struct sockaddr *bogus = (struct sockaddr *) &remote_addr;
		char *ap, ablock[44];
		int i;
		CloseSocket(socket);
		for (ap = ablock, i = 0; i < remote_addrlen && i < 14; i++)
		{
			*ap++ = ' ';
			*ap++ = HEX((bogus->sa_data[i] >> 4) & 0x0f);
			*ap++ = HEX(bogus->sa_data[i] & 0x0f);
		}
		
		*ap = '\0';
		ECPWS_LOG_INFO("non-INET connection attempt: socket %d, sa_family = %hu, sa_data[%d] = %s\n",
			socket, bogus->sa_family, remote_addrlen, ablock);
		return;
	}
	else
	{
//		struct	in_addr addr;
//		addr.s_addr = 
		ECPWS_LOG_INFO("Peer Address :%s", inet_ntoa(remote_addr.sin_addr.s_addr));	
	}
#endif

	/* XXX Either delete this, or document why it's needed */
	/* Pointed out 3-Oct-1999 by Paul Saab <paul@mu.org> */
#ifdef REUSE_EACH_CLIENT_CONNECTION_SOCKET
	if ((setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,	sizeof (sock_opt))) == -1)
	{
		ECPWS_FATAL("setsockopt: unable to set SO_REUSEADDR");
	}
#endif


#ifndef	WIN32
	/* nonblocking socket */
	if (set_nonblock_fd(socket) == -1)
	{
		WARN("fcntl: unable to set new socket to non-block");
		CloseSocket(socket);
		return;
	}

	/* set close on exec to true */
	if (fcntl(socket, F_SETFD, 1) == -1)
	{
		WARN("fctnl: unable to set close-on-exec for new socket");
		CloseSocket(socket);
		return;
	}
#endif


#ifdef TUNE_SNDBUF
	/* Increase buffer size if we have to.
	* Only ask the system the buffer size on the first request,
	* and assume all subsequent sockets have the same size.
	*/
	if (system_bufsize == 0)
	{
		len = (socklen_t)sizeof (system_bufsize);
		if (getsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *)&system_bufsize, &len) == 0 && len == sizeof (system_bufsize))
		{
			ECPWS_DEBUG_INFO("Send Buffer Size is %d\n", system_bufsize);
			if(system_bufsize==0)
			{
				ECPWS_ERR_INFO("Send Buffer Size is %d, is so small\n", system_bufsize);
			}
		}
		else
		{
			WARN("getsockopt(SNDBUF)");
			system_bufsize = 1;
		}
	}
	
	if (system_bufsize < sockbufsize)
	{
		if (setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (void *) &sockbufsize,sizeof (sockbufsize)) == -1)
		{
			WARN("setsockopt: unable to set socket buffer size");
#ifdef DIE_ON_ERROR_TUNING_SNDBUF
			exit(errno);
#endif /* DIE_ON_ERROR_TUNING_SNDBUF */
		}
	}
#endif                          /* TUNE_SNDBUF */

	len =  (socklen_t)sizeof (salocal);
	if (getsockname(socket, (struct sockaddr *) &salocal, &len) != 0)
	{
		WARN("getsockname");
		CloseSocket(socket);
		return;
	}

	conn = new_request(webSvr);
	if (!conn)
	{
		CloseSocket(socket);
		return;
	}

	if(webSvr->cfg.baseAuthen && !STRCASECMP(webSvr->cfg.baseAuthen, "No") )
		conn->authen_status = REQUEST_AUTHENED;
	else	
		conn->authen_status = REQUEST_NOT_AUTHENED;

	conn->socket = socket;
	conn->status = WEB_CLIENT_S_READ_HEADER;
	conn->header_line = conn->client_stream;
	conn->time_last = webSvr->current_time;
	conn->kacount = webSvr->cfg.ka_max;
	if (ascii_sockaddr(&salocal, conn->localIpAddr, sizeof (conn->localIpAddr)) == NULL)
	{
		WARN("ascii_sockaddr failed");
		CloseSocket(socket);
		QUEUE_EN_FREE(webSvr, conn);
		return;
	}

	if (ascii_sockaddr(&remote_addr, conn->remoteIpAddr,sizeof (conn->remoteIpAddr)) == NULL)
	{
		WARN("ascii_sockaddr failed");
		CloseSocket(socket);
		QUEUE_EN_FREE(webSvr, conn);
		return;
	}

	/* for possible use by CGI programs */
	conn->remotePort = net_port(&remote_addr);

#ifdef USE_TCPNODELAY
	/* Thanks to Jef Poskanzer <jef@acme.com> for this tweak */
	{
		int one = 1;
		if (setsockopt(conn->socket, IPPROTO_TCP, TCP_NODELAY, (void *) &one, sizeof (one)) == -1)
		{
			DIE("setsockopt: unable to set TCP_NODELAY");
		}
	}
#endif

#if 1
	/* */
#ifndef NO_RATE_LIMIT
	if (webSvr->totalConnections + 1 > webSvr->cfg.maxConnections)
	{
		conn->response_status = WEB_RES_SERVICE_UNAV;
		WEB_EMIT_EVENT(conn, WEB_EVENT_ERROR);
	}
#endif                          /* NO_RATE_LIMIT */

//	ECPWS_LOG_INFO("Peer Address :%s; local Address :%s\n", inet_ntoa(remote_addr.sin_addr), inet_ntoa(salocal.sin_addr));	
	ECPWS_LOG_INFO("CONN %d(%p): PeerAddress:%s, Port:%d; LocalAddress:%s\n", conn->index, conn, conn->remoteIpAddr, conn->remotePort, conn->localIpAddr);	
	webSvr->statistics.requests++;
	webSvr->totalConnections++;
	
#endif
	
	QUEUE_EN_READY(webSvr, conn);
}

