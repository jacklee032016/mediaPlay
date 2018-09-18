/*$Id: lizhijie Exp $*/

#ifndef __LIB_CGI_SYSCFG_H__
#define	__LIB_CGI_SYSCFG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "libCgi.h"


#define	CONF_DH			CONFIG_DIR"/etc/udhcpd.conf"

#define	CONF_SYS			CONFIG_DIR"/etc/init.d/rcS"
#define	CONF_PROC			CONFIG_DIR"/etc/web/proclist.conf"
#define	CONF_DNS			CONFIG_DIR"/etc/resolv.conf"


#define	ETH0_CONFIG_FILE							CONFIG_DIR"/etc/sys/ifcfg-eth0"
#define	ETH1_CONFIG_FILE							CONFIG_DIR"/etc/sys/ifcfg-eth1"
#define	WIFI_CONF_NETWORK						CONFIG_DIR"/etc/sys/ifcfg-wifi"
#define	WIFI_CONFIG_FILE							CONFIG_DIR"/etc/sys/wifi.conf"

#define	WIFI_CONFIG_ACL_FILE						CONFIG_DIR"/etc/wifi/acl.conf"
#define	WIFI_CONFIG_AUTHEN_FILE					CONFIG_DIR"/etc/wifi/authen.conf"

#define	SERVER_CONFIG_FILE						CONFIG_DIR"/etc/mediaServer.conf"
#define	CAMERA_CONFIG_FILE						CONFIG_DIR"/etc/sys/camera.conf"
#define	RECORD_CONFIG_FILE						CONFIG_DIR"/etc/web/autorecord.conf"
#define	ACTIVE_CTRL_FILE							CONFIG_DIR"/etc/InitialSnrActive.cfg"
#define		UDHCP_CMD								"udhcpd"
#define		WDHCP_CMD								"wdhcpd"

#define 		UDHCP_CONF_FILE						CONFIG_DIR"/etc/udhcpd.conf"
#define 		WDHCP_CONF_FILE						CONFIG_DIR"/etc/wdhcpd.conf"


#define	PROC_IPFORWARD							CONFIG_DIR"/proc/sys/net/ipv4/ip_forward"
#define	CONF_IPFORWARD							CONFIG_DIR"/etc/sys/ip_forward"
#define	HOSTNAME_CONF							CONFIG_DIR"/etc/hosts"
#define	LOCALHOST									"localhost"
#define 	CAMERA_DECRIP_CONF						CONFIG_DIR"/etc/web/descrip.conf"

#define	WTD_NAME    								"wtd"

typedef struct
{
	char name[5];
	char addr[20];
	char mask[20];
	char mac[20];
	char protocol[10];
	char dns1[20];
	char dns2[20];
	char state[5];	
}eth_info;

#define	CGI_SYSCFG_OP_HOSTNAME					"hostname"
#define	CGI_SYSCFG_OP_WAN						"wan"
#define	CGI_SYSCFG_OP_LAN							"lan"
#define	CGI_SYSCFG_OP_LAN_DHCP					"lanDhcp"
#define	CGI_SYSCFG_OP_REBOOT						"reboot"
#define	CGI_SYSCFG_OP_USER						"user"
/*added by Bob,12-17,09:55,for ip camera description*/
#define	CGI_SYSCFG_OP_DESCRIPTION				"description"

#define 	NIC_KEYWORD_PROTOCOL						"IPPROTO"
#define	NIC_KEYWORD_PROTOCOL_PPPOE				"PPPOE"
#define	NIC_KEYWORD_PROTOCOL_DHCP				"DHCP"
#define	NIC_KEYWORD_PROTOCOL_STATIC				"static"

#define	NIC_KEYWORD_HWADDR						"HWADDR"

#define	NIC_KEYWORD_DEVICE						"DEVICE"
#define	NIC_KEYWORD_IP_ADDRESS					"IPADDR"
#define	NIC_KEYWORD_NETMASK						"NETMASK"
#define	NIC_KEYWORD_BROADCAST					"BROADCAST"
#define	NIC_KEYWORD_NET_ADDRESS					"NETWORK"
#define	NIC_KEYWORD_ON_BOOT						"ONBOOT"
#define	NIC_KEYWORD_DHCP_SERVICE				"DHCPSERVICE"

#define	NIC_KEYWORD_GATEWAY						"GATEWAY"


#define	PPPOE_KEYWORD_SERVER_NAME_ASTERIC		"*"	/* wild char for PPPOE Server Name */

#define	PPPOE_KEYWORD_SERVER_NAME				"SERVER_NAME"
#define	PPPOE_KEYWORD_USER						"USER"
#define	PPPOE_KEYWORD_PASSWORD					"PWD"
#define	PPPOE_KEYWORD_DNSTYPE					"DNSTYPE"
#define	PPPOE_KEYWORD_DNSTYPE_SERVER			"SERVER"
#define	PPPOE_KEYWORD_DNSTYPE_USER				"SPECIFY"

#define	PPPOE_KEYWORD_DNS1						"DNS1"
#define	PPPOE_KEYWORD_DNS2						"DNS2"
#define	PPPOE_KEYWORD_ACNAME					"ACNAME"
#define	PPPOE_KEYWORD_SERVICENAME				"SERVICENAME"

#define	NIC_VALUE_YES		CGI_STR_YES_CASE
#define	NIC_VALUE_NO		CGI_STR_NO_CASE

#define	WAN_OP_SELECT_PROTOCOL			1
#define	WAN_OP_PROTOCOL_REDIRECT		2
#define	WAN_OP_INPUT_STATIC				3
#define	WAN_OP_INPUT_PPPOE				4
#define	WAN_OP_CONFIG_STATIC				5
#define	WAN_OP_CONFIG_DHCP				6
#define	WAN_OP_CONFIG_PPPOE				7
#define	WAN_OP_PPPOE_CHECK				8

#define	QUICK_WIZARD_OP_FINISHED					10

#define	WAN_OP_REBOOT_ALERT				1
#define	WAN_OP_REBOOT_NOW				2
#define	WAN_OP_REBOOT_OK					3

#define	CGI_QUICK_OP_SELECT		"qucikSelect"	/* select the protocol of WAN */
#define	CGI_QUICK_OP_WAN			"quickWan"
#define	CGI_QUICK_OP_LAN			"quickLan"
#define	CGI_QUICK_OP_DEFAULT		"quickDefault"		/* restore default config from Server TFTP which is implemented in get_config_data.c */
#define	CGI_QUICK_OP_FINISH		"quickFinish"

typedef struct
{
	char client[ITEM_LENGTH];
	char server[ITEM_LENGTH];
	char secret[ITEM_LENGTH];
	char addr[ITEM_LENGTH];
}pppoe;


typedef enum
{
	SYSCFG_TYPE_NORMAL	= 0,
	SYSCFG_TYPE_WIZARD
}syscfg_type_t;

//#define   BACK_CONF_PPPOE   				"/etc/back/pppoe.conf"
#define	CONF_PPPOE						"/etc/ppp/pppoe.conf"
#define 	CONF_PPP_PAP					"/etc/ppp/pap-secrets"
#define 	CONF_PPP_CHAP					"/etc/ppp/chap-secrets"

#define PPPOE_MSG_ERR_FAIL				"authentication fail"
#define PPPOE_MSG_ERR_REFUSE			"refused to authenticate"
#define PPPOE_MSG_ERR_NONET			"Timeout waiting for PADO packets"
#define PPPOE_MSG_OK					"authentication succeeded"
#define AUTH_SUB_STR					"authenticat"

#define 	PPPOE_LOG		       			"/tmp/PPPoE.log"

#define	VPN_LOG_FILE					"/tmp/log/vpnInfo"

typedef	enum
{
	CGI_LAN_EHT1	=	0,
	CGI_LAN_WIFI,	
	CGI_LAN_MESH	
}cgi_lan_type_t;

typedef enum
{
	CGI_LAN_WHERE_WIZARD,			/* used in QucikWizard */
	CGI_LAN_WHERE_SYSCFG,			/* used in SysConfig */
	CGI_LAN_WHERE_WIFI,				/* used in WifiConfig */
	CGI_LAN_WHERE_MESH
}cgi_lan_where_t;

typedef	struct
{
	char		name[64];

	char		devName[64];
	char		nicConfigFile[64];

	char		url[64];
	char		urlNic[64];
	char		urlDhcp[64];
	
	char		dhcpConfigFile[64];

	char		dhcpCmd[64];		/* detect status of DHCP Service */
}lan_cfg_t;

typedef struct
{
	llist 			*cgiVariables;
	llist			cfgs;
	
	lan_cfg_t		*info;		
}LAN_INFO;

typedef struct
{
	char name[32];
	char value1[32];
	char value2[64];
}DHCP_line;


int read_dhcp_conf(char *filename,char *startname,char *dhcpmask,int u,llist l);

int cgi_syscfg_write_nic_config(llist *l, syscfg_type_t type);

void cgi_syscfg_wan_activate_pppoe(llist *cgiVariables );
void cgi_syscfg_wan_activate_dhcp(llist *cgiVariables , syscfg_type_t type );
void cgi_syscfg_update_host_conf();


void cgi_syscfg_lan_auto_iptables( );
int cgi_syscfg_lan_update_dhcp_server();



void pppoe_wirte_conf(char *fname, llist *cgiVariables);

#endif

