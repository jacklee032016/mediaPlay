/*
* $Id: cgi_nic.c,v 1.2 2007/06/14 11:23:50 lizhijie Exp $
*/
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <errno.h>
#include <string.h>

#include "libCgi.h"

//get network address by host and mask
void getNetaddr_by_host_mask(char *netaddr,char *host, char *mask)
{
	struct in_addr in_host;
	struct in_addr in_mask;
	struct in_addr in_netaddr;

	char * straddr;
	memset(&in_host,0,sizeof(struct in_addr) );
	memset(&in_mask,0,sizeof(struct in_addr) );
	memset(&in_netaddr,0,sizeof(struct in_addr) );
	
	if(! inet_aton(host,&in_host) )
	{
		cgidebug( "host address '%s' error", host );
		return ;
	}

	if(! inet_aton(mask,&in_mask) )
	{
		cgidebug( "netmask '%s' error", mask );
		return ;
	}

	in_netaddr.s_addr=in_host.s_addr & in_mask.s_addr;
	straddr= inet_ntoa(in_netaddr);
	sprintf(netaddr,"%s",straddr);
	return ;
}


int get_nic_info(interface_info  *intfInfo,char *nicName)
{
	struct ifreq req;
	struct sockaddr_in *sin;
	int fd;

	sprintf(req.ifr_name,"%s", nicName);//, IFNAMSIZ );

	intfInfo->state = 1;
	fd = socket( PF_INET, SOCK_DGRAM, 0 );
	if( fd < 0 )
	{
		cgidebug("error : %s\n", strerror(errno) );
		intfInfo->state = -1;
		return intfInfo->state;
	}

	sin = (struct sockaddr_in *)&req.ifr_addr;
	if( ioctl( fd, SIOCGIFADDR, &req) == 0 )
	{
//		sprintf(intfInfo->addr, "%s", inet_ntoa(sin->sin_addr.s_addr) );
		sprintf(intfInfo->addr, "%s", inet_ntoa(sin->sin_addr) );
		cgidebug("%s IP address : %s\n", nicName, intfInfo->addr );
	}
	else
	{
		cgidebug("ioctl error for IP address of %s : %s\n",nicName, strerror(errno) );
		intfInfo->state = -1;
		goto errorReturn;
	}

	sin = (struct sockaddr_in *)&req.ifr_broadaddr;
	if( ioctl( fd, SIOCGIFBRDADDR, &req) == 0 )
	{
//		sprintf(intfInfo->bcast, "%s", inet_ntoa(sin->sin_addr.s_addr) );
		sprintf(intfInfo->bcast, "%s", inet_ntoa(sin->sin_addr) );
		cgidebug("%s IP Broadcast address : %s\n", nicName, intfInfo->bcast );
	}
	else
	{
		cgidebug("ioctl error for IP broadcast address of %s : %s\n",nicName,  strerror(errno) );
		intfInfo->state = -1;
		goto errorReturn;
	}

#if 0
	sin = (struct sockaddr_in *)&req.ifr_dstaddr;
	if( ioctl( fd, SIOCGIFDSTADDR, &req) == 0 )
		printf("%s\n", inet_ntoa(sin->sin_addr.s_addr) );
	else
		perror("ioctl error: ");
#endif

	sin = (struct sockaddr_in *)&req.ifr_netmask;
	if( ioctl( fd, SIOCGIFNETMASK, &req) == 0 )
	{
//		sprintf(intfInfo->netmask, "%s", inet_ntoa(sin->sin_addr.s_addr) );
		sprintf(intfInfo->netmask, "%s", inet_ntoa(sin->sin_addr) );
		cgidebug("%s Netmask : %s\n", nicName, intfInfo->netmask );
	}
	else
	{
		cgidebug("ioctl error for IP Netmask of %s : %s\n",nicName,  strerror(errno) );
		intfInfo->state = -1;
		goto errorReturn;
	}
	
	strcpy(intfInfo->name, nicName);
	if(intfInfo->state != -1)
	{
		getNetaddr_by_host_mask(intfInfo->netAddress, intfInfo->addr, intfInfo->netmask);
		intfInfo->state = 1;
	}

errorReturn :

	close(fd);
	return intfInfo->state;
}


network_info	*cgi_get_network_info(int isCgi)
{
	network_info	*netInfo;
	interface_info	*nic;
	
	netInfo = (network_info *)malloc(sizeof(network_info) );
	if(!netInfo)
		return NULL;
	memset(netInfo, 0 ,sizeof(network_info) );

	nic = &netInfo->wanInfo;
	
	nic->type = NIC_TYPE_WAN;
	if(1!=get_nic_info(nic, PPP0) )
	{
		get_nic_info(nic, ETH0);
	}
/*
	nic = &netInfo->lanInfo;
	nic->type = NIC_TYPE_LAN;
	get_nic_info(nic, ETH1);
*/
	nic = &netInfo->wifiInfo;
	nic->type = NIC_TYPE_WIFI;
	get_nic_info(nic, WIFI0 );

	if( (netInfo->wanInfo.state==-1) )//|| (netInfo->lanInfo.state==-1) )
	{
		char msg[512];
		int length = 0;
		if(isCgi)
		{
			if(netInfo->wanInfo.state==-1)
				length += CGI_SPRINTF(msg, length, "'%s'", gettext(CGI_STR_WAN_PORT) );
/*
			if(netInfo->lanInfo.state==-1)
				length += CGI_SPRINTF(msg, length, "'%s'", gettext(CGI_STR_LAN_PORT) );
*/			
			length += CGI_SPRINTF(msg, length, "%s!%s, %s!",gettext(CGI_STR_FAIL), gettext("Please Check your network carefully"), gettext("Try Network configuration again") );

			cgi_error_page(NULL, gettext("Network start failed"), msg);
		}	
#if 0		
		else
		{
			if(netInfo->wanInfo.state==-1)
				length += CGI_SPRINTF(msg, length, "'%s'", "WAN Port" );
			if(netInfo->lanInfo.state==-1)
				length += CGI_SPRINTF(msg, length, "'%s'", "LAN Port" );
			length += CGI_SPRINTF(msg, length, "%s!%s, %s!","Failed", "Please Check your network carefully", "Try Network configuration again" );

			printf(msg);
		}	
#endif		
	}
	
	return netInfo;
}

