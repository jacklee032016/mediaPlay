/*
* libCgi.h - header file for cgi-lib.c
* $Author: lizhijie $
* $Revision: 1.37 $
*/

#ifndef __LIB_CGI_H__
#define __LIB_CGI_H__

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#include "libCgiList.h"
#include "libCmn.h"


#if ARCH_X86
#define	CONFIG_DIR   ROOT_DIR"/releases"
#else
#define	CONFIG_DIR			
#endif

/* MACRO for conditional compiling */
#define 		DEBUG						1
#define		WITH_MISC_HTML			0		/* not used html tag functions */

// for i18n
#define	PACKAGE						"webadmin"
#define	TEXT_NOT_FOUND				"locale text not found"

#define _(x)								(x)

#define LINE_MAX_LENGTH		1024
#define ITEM_LENGTH 			256//64

#define ITEM_ONE 					1
#define ITEM_DBL 				2
#define ITEM_TRI 				3
#define ITEM_TRI_DHCP			10
#define ITEM_FOR 				4
#define ITEM_BLCK_LIST			5
#define ITEM_IP_FORWARD  		6
#define ITEM_EQU				99

//for raw read or write cofigure
#define	LINE4RAW				128

#define	REBOOT_WAIT_TIME			20	/* seconds */

//following are names from html but should not write in conf files
#define	SUBMIT					"WriteToConf"
#define	RESET					"ReadFromConf"


#define	PPP0 					"ppp0"
#define	ETH0					"eth0"
#define	ETH1					"eth1"
#define	WIFI0					"wifi0"

#define	SYS_CFG_PATH			CONFIG_DIR"/etc/sys"

#define  CGI_DOMAIN_FILE		CONFIG_DIR"/etc/web/domain.conf"
#define  CGI_HOST_NAME_FILE	CONFIG_DIR"/etc/web/hostname.conf"

#define	SYSUSER_CONF_FILE		"/etc/web/userList.conf"


#define	HDDISK_PART_MOUNT_POINT_1_ROOT			"/"
#define	HDDISK_PART_MOUNT_POINT_2_SPOOL		"/var/spool"
#define	HDDISK_PART_MOUNT_POINT_3_FILESERVER	"/var/data/share"

//#define	HDDISK_PART_MOUNT_POINT_2_SPOOL		"/var/data"

/* change this if you are using HTTP upload */
#ifndef UPLOADDIR
#define UPLOADDIR							"/tmp"
#endif


#define	cgiLog(fmt, args...) 			fprintf(stderr, fmt, ##args)

#if 		DEBUG
#define 	cgidebug(fmt, args...) 	fprintf(stderr, fmt, ##args)
#define	cgitrace()					fprintf(stderr, "%s.%s()-%d lines\n", __FILE__, __FUNCTION__, __LINE__)
#else
#define	cgidebug(fmt, args...) 	do{}while(0)
#define 	cgitrace()					do{}while(0)
#endif


#define	SYSTEM_CALL(cmd)	\
	system(cmd); cgidebug("system command : %s\n", cmd);

#define	DIR_DEVICE						"/etc/deviceID"
#define	PARSE_FILE_MAX_LENGTH		1024
#define	KEY_MAX_LENGTH				96
#define	VALUE_MAX_LENGTH				PARSE_FILE_MAX_LENGTH-KEY_MAX_LENGTH-32


/* CGI Environment Variables */
#define SERVER_SOFTWARE 			getenv("SERVER_SOFTWARE")
#define SERVER_NAME 				getenv("SERVER_NAME")
#define GATEWAY_INTERFACE 			getenv("GATEWAY_INTERFACE")

#define SERVER_PROTOCOL 			getenv("SERVER_PROTOCOL")
#define SERVER_PORT 				getenv("SERVER_PORT")
#define REQUEST_METHOD 			getenv("REQUEST_METHOD")
#define PATH_INFO 					getenv("PATH_INFO")
#define PATH_TRANSLATED 			getenv("PATH_TRANSLATED")
#define SCRIPT_NAME 				getenv("SCRIPT_NAME")
#define QUERY_STRING 				getenv("QUERY_STRING")
#define REMOTE_HOST 				getenv("REMOTE_HOST")
#define REMOTE_ADDR 				getenv("REMOTE_ADDR")
#define AUTH_TYPE 					getenv("AUTH_TYPE")
#define REMOTE_USER 				getenv("REMOTE_USER")
#define REMOTE_IDENT 				getenv("REMOTE_IDENT")
#define CONTENT_TYPE 				getenv("CONTENT_TYPE")
#define CONTENT_LENGTH 			getenv("CONTENT_LENGTH")

#define HTTP_USER_AGENT 			getenv("HTTP_USER_AGENT")
#define LOCALE						getenv("LC_ALL")
#define HTTP_ACCEPT_LANGUAGE 		getenv("HTTP_ACCEPT_LANGUAGE")

typedef enum
{
	WEB_TYPE_WWW = 0,		/* upload all WWW pages for Internet Web Server */
	WEB_TYPE_ADMIN,			/* upload admin package */	
	WEB_TYPE_PBX_FIRMWARE
}web_type_t;

typedef struct 
{
	char 	name[5];
	char 	addr[20];
	char 	MAC[20];
	char 	netmask[20];
	char 	bcast[20];
	char		netAddress[24];
	int 		state;
	web_type_t	type;
}interface_info;


typedef	struct
{
	interface_info		wanInfo;
//	interface_info		lanInfo;

	interface_info		wifiInfo;
}network_info;

typedef struct
{
	char		*key;
	char		*value;
}conf_item_t;

typedef struct 
{
	char		year[32];
	char		month[32];
	char		day[32];
	char		current_time[64];
}current_time_value_t;


#define		ROOT_PATH_WWW								CONFIG_DIR"/var/www"
#define		ROOT_PATH_ADMIN								CONFIG_DIR"/var/admin"


#define		WEB_CFG_WWW									CONFIG_DIR"/etc/whttpd/www.conf"
#define		WEB_CFG_ADMIN								CONFIG_DIR"/etc/whttpd/webAdmin.conf"

#define		SYS_CFG_LOCALE								CONFIG_DIR"/etc/whttpd/admin.locale"

#define		BOA_KEYWORD_LISTEN							"Listen"


typedef enum
{
	CGI_HELP_MSG_UPLOAD_ADMIN = 0,
	CGI_HELP_MSG_UPLOAD_WWW ,
	CGI_HELP_MSG_SERVICES ,
	CGI_HELP_MSG_SYSTEM_USER ,
	CGI_HELP_MSG_HOST_NAME,			/* host name amd domain name */
	CGI_HELP_MSG_WAN_CONFIG,		/* LAN config */
	CGI_HELP_MSG_STATIC_WAN_CONFIG,               /* WAN config */
	CGI_HELP_MSG_PPPOE_CONFIG,		/* LAN config */
	CGI_HELP_MSG_LOCALE,				/* language setup */
	CGI_HELP_MSG_SYSTIME,				/* system time, such as NTP or HW  */
	CGI_HELP_MSG_SERVICES_DHCP,
	CGI_HELP_MSG_SERVICES_SYSLOG,
	CGI_HELP_MSG_SERVICES_FTP,
	CGI_HELP_MSG_SERVICES_PPTP,
	CGI_HELP_MSG_SERVICES_QOS,
	CGI_HELP_MSG_SERVICES_UPNP,
	CGI_HELP_MSG_ROUTE,				/* system Route  */
	CGI_HELP_MSG_SNAT,				/* source NAT  */
	CGI_HELP_MSG_VPN,					/* VPN based on PPTP */
	CGI_HELP_MSG_QOS,
	CGI_HELP_MSG_BACKUP_SERVER,
	CGI_HELP_MSG_SYSTEM_PROC,
	CGI_HELP_MSG_MEDIA_CONFIG,
	CGI_HELP_MSG_MOTION_DETECT,
	CGI_HELP_MSG_INVALIDATE			/* this must be the last item */
}cgi_help_type_t;


typedef enum
{
	NIC_TYPE_WAN = 0,
	NIC_TYPE_LAN,
	NIC_TYPE_WIFI
}nic_type_t;


/* CGI URL definations */
#define	WEB_CGI_ROOT						"/cgi-bin/"

#define	WEB_URL_SYSCONFIG				WEB_CGI_ROOT"sysconf.cgi"


#define	WEB_URL_QUICK_INSTALL			WEB_CGI_ROOT"quick.cgi"

/* network URL */
#define	WEB_URL_SYSTEM_ROUTE			WEB_CGI_ROOT"network/route.cgi"

/* system URL */
#define	WEB_URL_DATA						WEB_CGI_ROOT"system/sysData.cgi"
#define	WEB_URL_UPLOAD_ADMIN			WEB_CGI_ROOT"system/uploadAdmin.cgi"

/* services URL */
#define	WEB_URL_SERVICES					WEB_CGI_ROOT"servicesmisc.cgi"
#define	WEB_URL_SERVICES_DDNSD			WEB_CGI_ROOT"services/ddns.cgi"
#define	WEB_URL_SERVICES_PPTP			WEB_CGI_ROOT"pptp.cgi"

/* firewall URL */
#define	WEB_URL_FIREWALL					WEB_CGI_ROOT"network/firewall.cgi"

/* SIP UA URL */
#define	WEB_URL_SIP_UA					WEB_CGI_ROOT"sip/sipUa.cgi"


#define	WEB_URL_WLAN						WEB_CGI_ROOT"network/wifi.cgi"


#define	WEB_URL_UPLOAD_WWW				WEB_CGI_ROOT"servers/uploadWww.cgi"
#define	WEB_URL_WWW_DETECT				WEB_CGI_ROOT"servers/www.cgi"

#define	WEB_URL_VIDEO						WEB_CGI_ROOT"video/video.cgi"

#define	WEB_URL_INDEX						WEB_CGI_ROOT"index.cgi"
#define	WEB_URL_NAVIGATE					WEB_CGI_ROOT"navigate.cgi"
#define	WEB_URL_CONTENT					WEB_CGI_ROOT"content.cgi"

#define	WEB_URL_WIZARD					WEB_CGI_ROOT"wizard.cgi"

/*VPN URL */
#define	WEB_URL_VPN						WEB_CGI_ROOT"video/vpn.cgi"


/* Media Files URL */
#define	WEB_URL_SYSTEM_INFO				WEB_CGI_ROOT"sysInfo.cgi"




#define	CGI_MEDIA_KEYWORK_MDIA_NAME		"mediaName"




#define	CGI_MEDIA_FILE_OP_LIST						"list"
#define	CGI_MEDIA_FILE_OP_SCAN						"scan"

#define	CGI_PLAYER_STATUS_OP_MEDIA					"media"
#define	CGI_PLAYER_STATUS_OP_PLAYER					"player"
#define	CGI_PLAYER_STATUS_OP_RUNINFO				"runInfo"

#define	CGI_PLAYER_STATUS_OP_SEEK					"seek"



#define		CGI_STR_ERROR					_("Error")
#define		CGI_STR_SUCCESS				_("Success")
#define		CGI_STR_SAVE					_("Save")
#define		CGI_STR_ADD					_("Add")
#define		CGI_STR_DELETE					_("Delete")
#define		CGI_STR_ACTIVATE				_("Activate")
#define		CGI_STR_DEACTIVATE			_("Deactivate")
#define		CGI_STR_MODIFY				_("Modify")
#define		CGI_STR_DEFAULT				_("Default")
#define		CGI_STR_RESET					_("Reset")
#define		CGI_STR_SUBMIT				_("Submit")
#define		CGI_STR_MANAGEMENT			_("Management")

#define		CGI_STR_STOP					_("Stop")
#define		CGI_STR_RUNNING				_("Running")

#define		CGI_STR_NOTE					_("Note")
#define		CGI_STR_DEVICE					_("Device")
#define		CGI_STR_TYPE					_("Type")

#define		CGI_STR_YES					_("Yes")
#define		CGI_STR_NO						_("No")
#define		CGI_STR_YES_CASE				_("YES")
#define		CGI_STR_NO_CASE				_("NO")
#define		CGI_STR_ON						_("On")
#define		CGI_STR_OFF					_("Off")
#define		CGI_STR_ENABLE				_("Enable")
#define		CGI_STR_DISABLE				_("Disable")
#define		CGI_STR_BEGIN					_("Begin")
#define		CGI_STR_END					_("End")
#define		CGI_STR_EXIT					_("Exit")
#define		CGI_STR_PLAY					_("Play")
#define		CGI_STR_RECORD				_("Record")
#define		CGI_STR_THANKS				_("Thanks")

#define		CGI_STR_RESTORE_DEFAULT		_("Restore Default")

#define		CGI_STR_NAME					_("Name")
#define		CGI_STR_SYSTEM_USER			_("System User")
#define		CGI_STR_USER_NAME			_("User Name")
#define		CGI_STR_PASSWORD				_("Password")

#define		CGI_STR_NEXT					_("Next")
#define		CGI_STR_STATUS				_("Status")
#define		CGI_STR_ALL					_("All")
#define		CGI_STR_INDEX					_("Index")

#define		CGI_STR_DHCP_CONFIG			_("DHCP Configuration")

#define		CGI_STR_LAN_CONFIG			_("LAN Configuration")
#define		CGI_STR_ETH_CONFIG			_("Ethernet Configuration")
#define		CGI_STR_PROTOCOL				_("Protocol")
#define		CGI_STR_IP_ADDRESS			_("IP Address")
#define		CGI_STR_NETMASK				_("Netmask")
#define		CGI_STR_GATEWAY				_("Gateway")
#define		CGI_STR_PHONE_NUMBER			_("Phone Number")

#define		CGI_STR_NETWORK_ADDRESS		_("Network Address")

#define		CGI_STR_PARTITION				_("Partition")
#define		CGI_STR_AUTHORITY				_("Authority")
#define		CGI_STR_AUTHENTICATION		_("Authentication")

#define		CGI_STR_HAS_EXIST				_("has exist")
#define		CGI_STR_IS_NOT_EXIST			_("is not exist")
#define		CGI_STR_SOME_ERROR			_("<font color=\"red\">Some Errors in your releases, maybe service is not running</font><br>")
#define		CGI_STR_PLEASE_WAIT			_("Please Waiting.....")
#define		CGI_STR_WAIT					_("Waiting")

#define		CGI_STR_PARAMETER				_("parameter")

#define		CGI_STR_ARE_YOU_SURE			_("ARE YOU SURE")
#define		CGI_STR_SORRY					_("Sorry")
#define		CGI_STR_WARN					_("Warning")
#define		CGI_STR_FAIL					_("Failed")

#define		CGI_STR_REBOOT				_("Reboot")
#define		CGI_STR_UPLOAD				_("Upload")
#define		CGI_STR_SET_ON_PAGAE			_("are set on this page")

#define		CGI_STR_SAVE_CONFIG			_("Save Configuration")

#define		CGI_STR_SPEED_DIAL			_("SPEED_DIAL")
#define		CGI_STR_INTERNAL_DIAL			_("INTERNAL_DIAL")
#define		CGI_STR_NORMAL_DIAL			_("NORMAL_DIAL")
#define		CGI_STR_LOCAL_DIAL			_("LOCAL_DIAL")
#define		CGI_STR_CALL_RETURN			_("CALL_RETURN")
#define		CGI_STR_URL_DIAL				_("URL_DIAL")
#define		CGI_STR_PSTN_DIAL				_("PSTN_DIAL")
#define		CGI_STR_DIRECT_DIAL			_("DIRECT_DIAL")


#define		CGI_STR_WAN_PORT				_("WAN Port")
#define		CGI_STR_LAN_PORT				_("LAN Port")
#define		CGI_STR_WIFI_PORT				_("WIFI Port")
#define		CGI_STR_MESH_PORT				_("MESH Port")


#define		CGI_STR_PREV					_("Prev")
#define		CGI_STR_SKIP					_("Skip")
#define		CGI_STR_STEP					_("Step")

#define		CGI_STR_MODE					_("Mode")
#define		CGI_STR_MAC_ADDRESS			_("MAC Address")
#define		CGI_STR_SOURCE_ADDRESS		_("Source Address")
#define		CGI_STR_DESTIN_ADDRESS		_("Destination Address")

#define		CGI_STR_QUICK_INSTALL			_("Quick Install")
#define		CGI_STR_FUTURE_VERSION		_("will be provided in future version")

#define		CGI_STR_START_WHEN_BOOT		_("Startup when boot")
#define		CGI_STR_FW_MIDDLE_CONFIG	_("Firewall Middle Configuration")
#define		CGI_STR_NO_OPERATION			_("No This Operation")
#define		CGI_STR_WITHOUT_CONF_FILE	_("Without Configuration File")

#define		CGI_STR_DNS_SERVER			_("DNS Server")
#define		CGI_STR_FIRST_DNS_SERVER		_("First DNS Server")
#define		CGI_STR_SECOND_DNS_SERVER	_("Second DNS Server")

#define		CGI_STR_PARTITION_INFO		_("Partition Info")

#define		CGI_STR_MESH_DEVICE			_("MESH Device")
#define		CGI_STR_WORK_MODE			_("Work Mode")
#define		CGI_STR_AD_HOC				_("Ad Hoc")
#define		CGI_STR_PHY_MODE				_("PHY Mode")
#define		CGI_STR_START_AODV			_("Start AODV")
#define		CGI_STR_ESSID					_("ESSID(Extended Service ID)")
#define		CGI_STR_NICKNAME				_("Nick Name")
#define		CGI_STR_RATE					_("Rate")
#define		CGI_STR_CHANNEL_FREQ			_("Channel(Fequency)")
#define		CGI_STR_CHANNEL				_("Channel")
#define		CGI_STR_AUTO					_("Auto")

#define		CGI_STR_SECONDS				_("Seconds")
#define		CGI_STR_MSECONDS				_("ms")
#define		CGI_STR_SIP_URL				_("SIP URL")
#define		CGI_STR_BASIC_CONFIG			_("Basic Configuration")

#define		CGI_STR_MUX_SERVER			_("MuxLab Media Player")
#define		CGI_STR_SIP_MY_UA				_("SIP UA Daemon")
#define		CGI_STR_CGIPBX					_("PBX MGR Task")
#define		CGI_STR_UARTD					_("PBX Signal Task")

#define		CGI_STR_ALERT_VOICE			_("Alert Voice")

#define		CGI_STR_ID						_("ID")
#define		CGI_STR_START_CYLINDER		_("Start Cylinder")
#define		CGI_STR_END_CYLINDER			_("End Cylinder")
#define		CGI_STR_MOUNT_POINT			_("Mount Point")

#define		CGI_STR_CHINESE				_("Chinese")
#define		CGI_STR_JAPANESE				_("Japanese")
#define		CGI_STR_ENGLISH				_("English")

#define		CGI_WIFI_ACL_OPEN_STR		_("NO")
#define		CGI_WIFI_ACL_ALLOW_STR		_("ALLOW")
#define		CGI_WIFI_ACL_DENY_STR		_("DENY")
#define		CGI_WIFI_ACL_FLUSH_STR		_("FLUSH")
#define		CGI_WIFI_ACL_DEATTACH_STR	_("DEATTACH")

#define		CGI_STR_TEST_RESULT			_("Test Result")
#define		CGI_STR_DAEMON				_("Daemon")

#define		CGI_STR_LOG_CONFIGURATION	_("Log Configuration")
#define		CGI_STR_LOG_FILE				_("Log File")
#define		CGI_STR_LOG_LEVEL				_("Log Level")
#define		CGI_STR_CONFIGURATION		_("Configuration")
#define		CGI_STR_PORT					_("Port")

#define		CGI_STR_HELP_NOT_DEFINED		_("Help has not defined for this")
#define		CGI_STR_HELP_IN_THIS_PAGE	_("In this page you can config")

#define	SYSTIME_STR_CONFIGURATION			_("System Time Configuration")
#define	SYSTIME_STR_STEP						CGI_STR_STEP
#define	SYSTIME_STR_CURRENT_TIME				_("Current System Time")
#define	SYSTIME_STR_SYSTEM_ERROR				_("System Internal Error")
#define	SYSTIME_STR_TIME_ZONE				_("Time Zone")
#define	SYSTIME_STR_TIME_MODE				_("Time Mode")
#define	SYSTIME_STR_CURRENT					_("Current")
#define	SYSTIME_STR_NTP_SERVER				_("NTP Server")

#define	CGI_STR_SERVER							_("Server")

#define	CGI_STR_LOCAL							_("Local")
#define	CGI_STR_REMOTE						_("Remote")

#define	SYS_LOCALE_CONF					CONFIG_DIR"/etc/sys/locale.conf"
#define	SYS_TIMEZONE						"TZ"

#define		CGI_KEYWORD_OPERATOR					"op"

#define	CGI_STR_DDNS_NAME				_("DDNS Client")

#define	CGI_ACTION							"action"
#define	CGI_ACTION_ADD					"Add"
#define	CGI_ACTION_ACTIVATE				CGI_STR_ACTIVATE
#define	CGI_ACTION_DEACTIVATE			CGI_STR_DEACTIVATE
#define	CGI_ACTION_DEL						"Delete"
#define	CGI_ACTION_MODIFY					"Modify"
#define	CGI_ACTION_SELECT					"Select"
#define	CGI_ACTION_DEFAULT				CGI_STR_DEFAULT
#define	CGI_ACTION_SAVE					CGI_STR_SAVE
#define	CGI_ACTION_RESET					CGI_STR_RESET /* reset PBX */


#define	GET_CGI_OP(cgiVars)					GET_VALUE((cgiVars), CGI_KEYWORD_OPERATOR)
#define	GET_CGI_ACTION(cgiVars)			GET_VALUE((cgiVars), CGI_ACTION)

#define	CGI_COLOR_RED						"#ff0000"
#define	CGI_COLOR_GREEN					"#00ff00"
#define	CGI_COLOR_BLUE					"#0000ff"

#define	CGI_SPRINTF(_msg,_len, _fmt, ...)				\
	snprintf(_msg+_len, sizeof(_msg)-_len,  _fmt, ##__VA_ARGS__)

#define	CGI_INSTANCE_INFO(msg, color)	\
	do{\
		printf("<br><font color=\"%s\">%s</font>", color, msg); \
		fflush(stdout);	\
	}while(0)


/* form processing routines */
void unescape_url(char *url);
int read_cgi_input(llist* entries, int (*callback)(char *, void *), void *data);
int cgi_getline(char s[], int lim);

#if WITH_MISC_HTML
short accept_image();
char *cgi_value(llist l,char *name);
/* miscellaneous CGI routines */
int parse_cookies(llist *entries);
void print_cgi_env();
#endif

char **cgi_val_multi(llist l, char *name);
char **cgi_val_all(llist l);
void cgi_locale_filename(char *filename, char *localename);
node *cgi_locatename(llist *l, char *name,int rigor);
node *cgi_locatename_raw(llist *l, char *name, int note);
char *cgi_name(llist l,char *value);
char **cgi_name_multi(llist l, char *value);


#if DEBUG
void print_entries(llist l);
#endif


/* HTTP headers */
#define CGI_HTML_HEADE() 	\
	printf("Content-type: text/html\r\n\r\n");


char *escape_input(char *str);
int read_file_upload(llist *entries, int maxfilesize);

char *gettext(char *text);

/* from cgi_signal.c */
int cgi_directory_filt_by_number(const struct dirent *namelist);
void cgi_send_signal_to_process_by_name(int sig, char *prog_name);
void fork_cmd(const char *cmd) ;

/* from cgi_file_func.c */
int cgi_read_local_config(char *filename,void(*parseLocalConfig)(char *, void *), void *data);

/*check family*/
/*from filecommonfunc.c */
int readline(entrytype *entry,int itemnum, FILE* fname);
int readconfig(char *filename,int itemnum,char *ctype,llist *l);
int readconfig_raw(char *filename,llist *l);
int writeline(entrytype *entry,int itemnum, FILE* fname);
int writeconfig(char *filename,int itemnum,char *ctype,llist *l);
int writeconfig_no_override(char *filename,void (*dealline)(char *line,void *data,void *refdata), llist *l);


void kill_process(char * process_name);
void int_to_str(unsigned short d,char * str);
void cgi_syscfg_update_host_conf();


int get_nic_info(interface_info  * peth,char * eth_name);
network_info	*cgi_get_network_info(int isCgi);

char *cgi_domain_name_read();
int cgi_domain_name_write(char *domainname);
char *cgi_host_name_read();
int cgi_host_name_write(char *hostname);

int cgi_refresh_page(int timeout, char *target, char *header, char *msg);
int cgi_error_page(char *target, char *header, char *errmsg);
int cgi_info_page( char *header, char *infomsg, char *othermsg);
void cgi_invalidate_page( );
char *cgi_refresh_script(char *target, int seconds);
int cgi_future_page( char *cmdName, char *module);

char *cgi_help_button( cgi_help_type_t msgType);
char *cgi_button(char *name, char *onclick_target);
char *cgi_button_alert(char *name, char *onclick_target, char *alertMsg );
char *cgi_submit_button(char *name );
char *cgi_submit_button_with_alert(char *name , char *alertMsg, char *formName);
char *cgi_reset_button(char *name );

int	cgiRedirect(char *target);

int config_file_replace_field(char *filename, int itemnum,char *key, char *newValue);

int list_add_config_items(llist *list, conf_item_t *items, int size);

/* return 0 : not validate IP address */
int isValidateIpAddress(char *ipAddrStr);

int list_replace_with_new_list(llist *rawList, llist *newList);


int	cgi_nic_refresh_net_and_broad_address(llist *nicCfgs);
int	cgi_get_files_in_dir(char *dirName, llist *filesList);

current_time_value_t *systime_get_current_time(void);


int cgi_directory_filt_by_number(const struct dirent *namelist);
int  cgi_get_pid_by_name(char * appName);
int cgi_signal_register_handler(int sigkey, void (*sig_handler)(int, siginfo_t *, void *));
void cgi_sig_send(pid_t pid, int sigid, int sigcode);



int	cgiMuxPlayUrl(char *file, int winIndex, char *errRedirectPage, char *sucessRedirectPage);

/************ CGI of mux server ***************/

#define	CGI_MUX_SERVER_OP_GLOBAL					"global"

#define	CGI_MUX_SERVER_ACTION_GLOBAL_PLUGIN		"plugins"
#define	CGI_MUX_SERVER_ACTION_GLOBAL_CONTROLLER	"controller"
#define	CGI_MUX_SERVER_ACTION_GLOBAL_CAPTURER		"capturer"


#define	CGI_MUX_SERVER_OP_SERVER					"svrConfig"



#define	CGI_MUX_SERVER_OP_METADATA_INFO			"metadataInfo"
#define	CGI_MUX_SERVER_OP_MEDIA_INFO				"mediaInfo"
#define	CGI_MUX_SERVER_OP_LIST						"playerInfo"
#define	CGI_MUX_SERVER_OP_CONFIG					"config"
#define	CGI_MUX_SERVER_OP_SAVE						"save"
#define	CGI_MUX_SERVER_OP_WINDOW					"window"
#define	CGI_MUX_SERVER_OP_PLAYLIST					"playlist"

#define	CGI_MUX_SERVER_OP_MEDIAFILES				"mediafiles"
#define	CGI_MUX_SERVER_OP_RECORD					"record"


#define	CGI_MUX_SERVER_OP_HTTP					"playHTTP"
#define	CGI_MUX_SERVER_OP_RTSP					"playRTSP"
#define	CGI_MUX_SERVER_OP_RTP					"playRTP"
#define	CGI_MUX_SERVER_OP_UDP					"playUDP"
#define	CGI_MUX_SERVER_OP_WMSP					"playWMSP"
#define	CGI_MUX_SERVER_OP_RTMP					"playRTMP"
#define	CGI_MUX_SERVER_OP_INPUT					"playInput"

#define	WEB_URL_MUX_SERVER						WEB_CGI_ROOT"muxServer.cgi"	/* mux server */

/***** PlayList *****/
#define	MUXSERVER_HLINK_URLS						WEB_URL_MUX_SERVER"?"CGI_KEYWORD_OPERATOR"="CGI_MUX_SERVER_OP_CONFIG
#define	MUXSERVER_HLINK_WINDOWS					WEB_URL_MUX_SERVER"?"CGI_KEYWORD_OPERATOR"="CGI_MUX_SERVER_OP_WINDOW

#define	MUXSERVER_HLINK_PLAYLIST					WEB_URL_MUX_SERVER"?"CGI_KEYWORD_OPERATOR"="CGI_MUX_SERVER_OP_PLAYLIST


#define	MUXSERVER_HLINK_PLAYLIST_ADD			MUXSERVER_HLINK_PLAYLIST"&"CGI_ACTION"="CGI_ACTION_ADD

#define	MUXSERVER_HLINK_PLAYLIST_MODIFY			MUXSERVER_HLINK_PLAYLIST"&"CGI_ACTION"="CGI_ACTION_MODIFY
#define	MUXSERVER_HLINK_PLAYLIST_DELETE			MUXSERVER_HLINK_PLAYLIST"&"CGI_ACTION"="CGI_ACTION_DEL
#define	MUXSERVER_HLINK_PLAYLIST_SELECT			MUXSERVER_HLINK_PLAYLIST"&"CGI_ACTION"="CGI_ACTION_SELECT  /* seelct and play */

#define	MUXSERVER_HLINK_PLAYLIST_DELETE_FILE	MUXSERVER_HLINK_PLAYLIST"&"CGI_ACTION"=deletefile"


/***** Local Media File *****/
#define	MUXSERVER_HLINK_MEDIAFILE				WEB_URL_MUX_SERVER"?"CGI_KEYWORD_OPERATOR"="CGI_MUX_SERVER_OP_MEDIAFILES

#define	MUXSERVER_HLINK_MEDIAFILE_PLAY			MUXSERVER_HLINK_MEDIAFILE"&"CGI_ACTION"="CGI_ACTION_SELECT"&"CGI_MEDIA_KEYWORK_MDIA_NAME	/*select and play */
#define	MUXSERVER_HLINK_MEDIAFILE_DELETE		MUXSERVER_HLINK_MEDIAFILE"&"CGI_ACTION"="CGI_ACTION_DEL"&"CGI_MEDIA_KEYWORK_MDIA_NAME

/***** Record *****/
#define	CGI_ACTION_CONFIG							"config"
#define	CGI_ACTION_STATUS							"status"

#define	CGI_ACTION_START							"start"
#define	CGI_ACTION_STOP							"stop"

#define	MUXSERVER_HLINK_RECORD					WEB_URL_MUX_SERVER"?"CGI_KEYWORD_OPERATOR"="CGI_MUX_SERVER_OP_RECORD

#define	MUXSERVER_HLINK_RECORD_CONFIG			MUXSERVER_HLINK_RECORD"&"CGI_ACTION"="CGI_ACTION_CONFIG
#define	MUXSERVER_HLINK_RECORD_START			MUXSERVER_HLINK_RECORD"&"CGI_ACTION"="CGI_ACTION_START
#define	MUXSERVER_HLINK_RECORD_STOP				MUXSERVER_HLINK_RECORD"&"CGI_ACTION"="CGI_ACTION_STOP
#define	MUXSERVER_HLINK_RECORD_STATUS			MUXSERVER_HLINK_RECORD"&"CGI_ACTION"="CGI_ACTION_STATUS

/***** Informations *****/
#define	MUXSERVER_HLINK_PLAYER_INFO				WEB_URL_MUX_SERVER"?"CGI_KEYWORD_OPERATOR"="CGI_MUX_SERVER_OP_LIST
#define	MUXSERVER_HLINK_MEDIA_INFO				WEB_URL_MUX_SERVER"?"CGI_KEYWORD_OPERATOR"="CGI_MUX_SERVER_OP_MEDIA_INFO
#define	MUXSERVER_HLINK_METADATA_INFO			WEB_URL_MUX_SERVER"?"CGI_KEYWORD_OPERATOR"="CGI_MUX_SERVER_OP_METADATA_INFO


#define	MUXSERVER_HLINK_GLOBAL					WEB_URL_MUX_SERVER"?"CGI_KEYWORD_OPERATOR"="CGI_MUX_SERVER_OP_GLOBAL

#define	MUXSERVER_HLINK_GLOBAL_PLUGINS			MUXSERVER_HLINK_GLOBAL"&"CGI_ACTION"="CGI_MUX_SERVER_ACTION_GLOBAL_PLUGIN
#define	MUXSERVER_HLINK_GLOBAL_CONTROLLER		MUXSERVER_HLINK_GLOBAL"&"CGI_ACTION"="CGI_MUX_SERVER_ACTION_GLOBAL_CONTROLLER
#define	MUXSERVER_HLINK_GLOBAL_CAPTURER		MUXSERVER_HLINK_GLOBAL"&"CGI_ACTION"="CGI_MUX_SERVER_ACTION_GLOBAL_CAPTURER



#define	MUXSERVER_HLINK_SVR						WEB_URL_MUX_SERVER"?"CGI_KEYWORD_OPERATOR"="CGI_MUX_SERVER_OP_SERVER

#define	MUXSERVER_HLINK_SVR_CONFIG				MUXSERVER_HLINK_SVR"&"CGI_ACTION"="IPCMD_NAME_SERVER_CONFIG
#define	MUXSERVER_HLINK_SVR_CONNECTIONS		MUXSERVER_HLINK_SVR"&"CGI_ACTION"="IPCMD_NAME_SERVER_CONNS
#define	MUXSERVER_HLINK_SVR_SOURCE				MUXSERVER_HLINK_SVR"&"CGI_ACTION"="IPCMD_NAME_SERVER_FEEDS
#define	MUXSERVER_HLINK_SVR_URLS					MUXSERVER_HLINK_SVR"&"CGI_ACTION"="IPCMD_NAME_SERVER_URLS


#include "libCgiMux.h"

#define	SIG_CGI_TO_PBX				2
#define	SIG_SIP			60 //SIGRTMIN
#define	SIG_PBX_CGI	59//(SIGRTMIN+1)
//#define	SIG_PBX_COM	(SIGRTMAX)
#define   SIG_SIP_GUA	61
#define   SIG_BLACKLIST	62

/************** for upload file  */
#define UPLOADPATH		"/tmp"


#define		MAX_UPLOAD_FILE_SIZE_10M						10		/* 10M bytes */	

#define		CGI_UPLOAD_RESTORE							"restore"
#define		CGI_UPLOAD_UPDATE							"update"

#define		CGI_UPLOAD_SUCCESS							0
#define		CGI_UPLOAD_ERROR_NO_CONTENT				-1
#define		CGI_UPLOAD_ERROR_FORMAT_NOT_SUPPORT		-2
#define		CGI_UPLOAD_ERROR_TOO_BIG					-3
#define		CGI_UPLOAD_ERROR_FILE_NOT_EXIST				-4


int up_read_cgi_input(llist* entries);
int upload_file_deal_http_request();

int upload_parse_form_endcode(llist* entries, web_type_t type);

int upload_read_cgi_input(llist* entries, web_type_t type);
char *upload_add_buttons(web_type_t type);
char *upload_status_list( web_type_t type);

/********************** cig_sysinfo */


#define	CGI_SYSINFO_OP_VERSION						"version"
#define	CGI_SYSINFO_OP_NET_STATE						"NetStatus"
#define	CGI_SYSINFO_OP_SYS_PROC						"SysProc"
#define	CGI_SYSINFO_OP_LOCALE						"SysLocale"	/* language */
#define	CGI_SYSINFO_OP_TIME							"SysTime"	
#define	CGI_SYSINFO_OP_SERVICES						"services"	

#define	CGI_SYSINFO_OP_REBOOT						"reboot"

#define	CGI_SYSINFO_KEYWORK_PROC_NAME				"ProcName"

#define	HILINK_SERVICE_CFG		\
			WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_SERVICES

typedef	struct
{
	llist 				cgiVariables;
	
	llist				globalCfgs;

	network_info		*netInfo;
	
}SYS_INFO;

int cgiSysInfo_Versions(SYS_INFO *info);
int cgiSysInfo_Tasks(SYS_INFO *info);
int cgiSysInfo_Locale(SYS_INFO *info);
int cgiSysInfo_Time(SYS_INFO *info);

int	cgiSysInfo_ServiceConfig(SYS_INFO *info);
int cgiSysInfo_reboot(SYS_INFO *info);

/*************************** cgi_services */
#include <errno.h>


#define SERVICES_LIST_FILE			CONFIG_DIR"/etc/sys/services.conf"
#define SERVICES_LIST_SCRIPTS		"/sbin/scripts/services.sh"

#define	SERVICE_ACTION_START			"Start"
#define	SERVICE_ACTION_STOP			"Stop"

#define	SERVICE_NAME_SMTPD			"master"
#define	SERVICE_NAME_POP_IMAP		"dovecot"
#define	SERVICE_NAME_ADMIN			"boa"
#define	SERVICE_NAME_WWW			"www"

#define	SERVICE_NAME_DHCP			"udhcpd"
#define	SERVICE_NAME_FTP				"vsftpd"
//#define	SERVICE_NAME_UPNP			"upnpd"
#define	SERVICE_NAME_IPSET			"ipsetd"
#define	SERVICE_NAME_UPNP			"upnpd"
#define	SERVICE_NAME_UPNP_NEIGHBOUR	"uDev"					/* UPnP Device */
#define	SERVICE_NAME_UPNP_FORWARD	"uCtrl"					/* UPnP Control Point */

#define	SERVICE_NAME_SYSLOGD			"syslogd"
#define	SERVICE_NAME_KLOGD			"klogd"

#define	SERVICE_NAME_VPNSERVER		"pptpd"
#define	SERVICE_NAME_VPNCLIENT		"pptp"

#define	SERVICE_NAME_DDNSD			"inadyn"

#define	SERVICES_DEBUG					0 /*1*/


typedef enum// statenum
{
	SERVICE_STATE_INVALIDATE 		= -1 ,
	SERVICE_STATE_SLEEPING 		= 0 ,
	SERVICE_STATE_RUNNING
}service_state_t;

struct _own_node 
{
	char 			name[64];
	char 			display_name[128];	
	char 			command[256];
	int 				pid;
	int				startup;				/* weather startup when boot */
	
	service_state_t 	state;
};


typedef struct _own_node ServiceNode;

struct _proclist 
{
	ServiceNode		*node;
	struct _proclist 	*next;
//	struct _proclist 	*head;
};

typedef struct _proclist proclist;

proclist *cgi_services_list();
service_state_t cgi_service_get_state(char *command /* first field in config file*/);
service_state_t cgi_service_start(char *command /* first field in config file*/);
service_state_t cgi_service_stop(char *command /* first field in config file*/);
service_state_t cgi_service_restart(char *command /* first field in config file*/);

void cgi_services_stop_all();

char *cgi_service_get_state_name(char *command);
service_state_t  cgi_get_program_state( char *name ,char *command);
char *cgi_get_program_state_name(char *name);
int  cgi_get_program_pid_with_param(char *cmd, char *param);
char *cgi_service_state_name(service_state_t state);


#endif

