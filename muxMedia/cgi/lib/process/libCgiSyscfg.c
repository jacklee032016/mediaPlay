/*
* $Id: cgi_syscfg.c,v 1.7 2007/09/06 09:19:58 lizhijie Exp $
* Both for sysconfig and quickInstallWizard
*/

#include "libCgiSyscfg.h"

void dhcp_router_dealline(char *pline,void *data,void *refdata)
{
	node *pnode=(node *)data;
	if(!strcmp(pnode->entry.name,"opt"))
	{
		sprintf(pline,"opt\t\t router \t\t%s\r\n",pnode->entry.value);
	}
}

void dhcp_dealline(char *pline,void *data,void *refdata)
{
	node *pnode=(node *)data;
	if(!strcmp(pnode->entry.name,"opt"))
	{
		sprintf(pline,"top\t\t subnet \t\t%s\r\n",pnode->entry.value);
	}
	else if(!strcmp(pnode->entry.name,"top"))
	{
		sprintf(pline,"opt\t\t subnet \t\t%s\r\n",pnode->entry.value);
	}
	else
	{
		sprintf(pline,"%s  %s\n",pnode->entry.name,pnode->entry.value);
	}
}

int read_dhcp_conf(char *filename,char *startname,char *dhcpmask,int u,llist l)
{
	char cmd[128];
	char startip[128];
	char startip_max[128];
	int h,d,count=0;
	FILE *fp;
	memset(&startip,0,sizeof(startip));
	for(h=0;h<u;h++)
	{
		memset(&cmd,0,sizeof(cmd));
		if(startname[h]=='.')
		count++;
		sprintf(cmd,"%c",startname[h]);
		strcat(startip,cmd);
		if(count==3)
			break;
	}
	count=0;
	memset(&cmd,0,sizeof(cmd));
	if(1==u-h-1)
	sprintf(cmd,"%c",startname[h+1]);
	if(2==u-h-1)
	sprintf(cmd,"%c%c",startname[h+1],startname[h+2]);
	if(3==u-h-1)
	sprintf(cmd,"%c%c%c",startname[h+1],startname[h+2],startname[h+3]);	
	d=atoi(cmd);
	memset(&startip_max,0,sizeof(startip_max));
	for(h=0;h<u;h++)
	{
		memset(&cmd,0,sizeof(cmd));
		if(startname[h]=='.')
		count++;
		sprintf(cmd,"%c",startname[h]);
		strcat(startip_max,cmd);
		if(count==3)
			break;
	}
	if( (fp=fopen("/tmp/tmp.txt","w"))==NULL)
		return -1;
	fprintf(fp,"start  %s%d\n",startip,d+1);
	fprintf(fp,"end  %s254\n",startip_max);
	fprintf(fp,"opt %s\n",dhcpmask);
	fclose(fp);
	if( (fp=fopen("/tmp/tmp2.txt","w"))==NULL)
		return -1;
	fprintf(fp,"opt  %s\n",startname);
	fclose(fp);
	if( (fp=fopen("/tmp/tmp3.txt","w"))==NULL)
		return -1;
	fprintf(fp,"top  %s\n",dhcpmask);
	fclose(fp);
	
	readconfig("/tmp/tmp.txt",ITEM_DBL,NULL, &l);
	writeconfig_no_override(CONF_DH,dhcp_dealline,&l);
	system("rm /tmp/tmp.txt");
	readconfig("/tmp/tmp2.txt",ITEM_DBL,NULL, &l);
	writeconfig_no_override(CONF_DH,dhcp_router_dealline,&l);
	system("rm /tmp/tmp2.txt");
	readconfig("/tmp/tmp3.txt",ITEM_DBL,NULL, &l);
	writeconfig_no_override(CONF_DH,dhcp_dealline,&l);
	system("rm /tmp/tmp3.txt");
	return 0;
}	


static int __syscfg_restart_nic(web_type_t webType, char *ip, syscfg_type_t type)
{
	char		info[2048];
	char		target[128];
	char		title[128];
	int		len = 0;
	int		refreshTime = 2;
		
//		n=check_ip_entries_error(l);

	if(webType == WEB_TYPE_WWW)
	{
		if(type== SYSCFG_TYPE_NORMAL)
			sprintf(target, "%s?%s=%s", WEB_URL_SYSCONFIG,CGI_KEYWORD_OPERATOR, CGI_SYSCFG_OP_WAN );
		else
		{
			refreshTime = 1;
			sprintf(target, "%s?%s=%s", WEB_URL_QUICK_INSTALL, CGI_KEYWORD_OPERATOR, CGI_QUICK_OP_LAN );	/* after WAN, then goto LAN when QuickWizard */
		}
		
		sprintf(title, "%s", gettext(CGI_STR_ETH_CONFIG) );
		len += sprintf(info+len, "<TR><TD align=\"center\"><strong>%s '%s' %s</strong></TD></TR>",
			gettext("IP Address is set as"), (ip)?ip:"0.0.0.0", gettext("in WAN port") );
		len += sprintf(info+len, "<TR><TD align=\"center\"><strong>%s</strong></TD></TR>",
			gettext("Maybe WWW can be access with different URL in your browser") );
		len += sprintf(info+len, "<TR><TD align=\"center\"><strong>%s</strong></TD></TR>",
			gettext(CGI_STR_PLEASE_WAIT) );
	}
	else
	{
		if(type == SYSCFG_TYPE_NORMAL)
			sprintf(target, "%s?%s=%s", WEB_URL_SYSCONFIG, CGI_KEYWORD_OPERATOR, CGI_SYSCFG_OP_LAN );
		else
		{
			refreshTime = 1;
			sprintf(target, "%s?%s=%s", WEB_URL_QUICK_INSTALL, CGI_KEYWORD_OPERATOR, CGI_QUICK_OP_FINISH ); 
		}	
		sprintf(title, "%s", gettext(CGI_STR_LAN_CONFIG) );
		len += sprintf(info+len, "<TR><TD align=\"center\"><strong>%s '%s' %s</strong></TD></TR>",
			gettext("IP Address is set as"), ip, gettext("in LAN port") );
		len += sprintf(info+len, "<TR><TD align=\"center\"><strong>%s</strong></TD></TR>",
			gettext("Maybe WebAdmin can be access with different URL in your browser") );
		len += sprintf(info+len, "<TR><TD align=\"center\"><strong>%s</strong></TD></TR>",
			gettext(CGI_STR_PLEASE_WAIT) );
	}
	
	cgi_refresh_page(refreshTime, target, title, info );
	//system("/sbin/network-setup");
	//fork_cmd("/sbin/network-setup &");
	//lan_update_host_conf();
	fflush(stdout);
	
	kill_process("pppoe-connect");
	kill_process("pppd");
	kill_process("udhcpc");

	fork_cmd("/sbin/scripts/network_setup.sh > /dev/null ");
	
	cgi_syscfg_update_host_conf();

	return 0;
}

/* write ifcfg-eth0/1 and /etc/resolv.conf
* return 0 :success, other :failed
*/
int cgi_syscfg_write_nic_config(llist *l, syscfg_type_t type)
{
	char *dev;
	char *p;

	web_type_t	webType = WEB_TYPE_ADMIN;
		
	p = GET_VALUE(l, NIC_KEYWORD_PROTOCOL );
	if(!p || strcasecmp(p, NIC_KEYWORD_PROTOCOL_STATIC) )
	{
		cgi_error_page(NULL, gettext("Network Configuration"), gettext("Protocol is not defined") );
		return 1;
	}
	
	dev = GET_VALUE(l, NIC_KEYWORD_DEVICE);
	if(!dev )
	{
		cgi_error_page(NULL, gettext("Network Configuration"), gettext("Net Device is not defined") );
		return 1;
	}

	if(!strcasecmp(dev, ETH0) )
	{

		webType = WEB_TYPE_WWW;
	}

	cgi_nic_refresh_net_and_broad_address(l);

	list_append_raw(l, NIC_KEYWORD_ON_BOOT, CGI_STR_YES_CASE, NULL, NULL);
	
	writeconfig((webType==WEB_TYPE_ADMIN)?ETH1_CONFIG_FILE:ETH0_CONFIG_FILE, ITEM_EQU, NULL, l);
	
	__syscfg_restart_nic(webType, GET_VALUE(l, NIC_KEYWORD_IP_ADDRESS), type );
	return 0;
}

static void pppoe_dealine(char * line, void * data, void * refdata)
{
	node *pnode=(node *)data;	
	//if(strstr(pnode->entry.name,"_enable"))	
		//{		
	//		sprintf(line,"%s=%s\n",pnode->entry.name,atoi(pnode->entry.value)?CGI_STR_YES_CASE:CGI_STR_NO_CASE);	
		//}		
	sprintf(line,"%s=%s\n",pnode->entry.name,pnode->entry.value);	
		
}


void pppoe_wirte_conf(char *fname, llist *cgiVariables )
{
	llist	cfgs, newNode;
	entrytype item={NULL, NULL, NULL, NULL};

	memset(&cfgs, 0 ,sizeof(llist));
	readconfig(fname, ITEM_FOR, NULL, &cfgs);

	LIST_CREATE(&newNode);
	item.name = cgi_new_string( GET_VALUE(cgiVariables, PPPOE_KEYWORD_SERVER_NAME) );
	item.type = cgi_new_string( GET_VALUE(cgiVariables, PPPOE_KEYWORD_USER) );
	item.value = cgi_new_string( GET_VALUE(cgiVariables, PPPOE_KEYWORD_PASSWORD) );
	item.rule = cgi_new_string( GET_VALUE(cgiVariables, PPPOE_KEYWORD_SERVER_NAME_ASTERIC) );

	list_append(&newNode, &item );
	
	if(list_replace_with_new_list(&cfgs, &newNode) != 1)
	{
		list_append(&cfgs, &item);
	}
	writeconfig(fname, ITEM_FOR, NULL, &cfgs);
	
}

void cgi_syscfg_update_host_conf()
{
#define	CONF_HOSTNAME		"/etc/hosts"
	char	*hostname = NULL;
	interface_info wan;
	FILE * fp;
	char *domain;

	memset(&wan,0,sizeof(interface_info) );
#if 0	
	ret=gethostname(hostname,128) ;
	if ( ( 0 != ret ) && ( 0 == strlen(hostname) ) )
	{
		strncpy(hostname,"IXP422",128);
	}
#else
	hostname = cgi_host_name_read();
#endif
	if( 1 != get_nic_info(&wan, PPP0) )
	{
		memset(&wan,0,sizeof(interface_info) );
		get_nic_info(&wan, ETH0);
	}

	fp = fopen (CONF_HOSTNAME, "w") ;
	if( NULL == fp )
	{
		cgidebug("open file %s failed\n",CONF_HOSTNAME);
		return ;
	}

	domain = cgi_domain_name_read();

	if( 0 != strlen(wan.addr) )
		fprintf(fp,"%s\t\t%s\t\t%s.%s\n",wan.addr,  hostname, hostname,domain );
	
	fprintf(fp,"%s\t\t%s\n","127.0.0.1","localhost");
	
	fclose(fp);
	
}

void cgi_syscfg_wan_activate_dhcp(llist *cgiVariables , syscfg_type_t type)
{
	char target[256], title[256];
	char	info[2048];
	int 	len = 0;
	int	refreshTime = 5;
	
	config_file_replace_field(ETH0_CONFIG_FILE, ITEM_EQU, NIC_KEYWORD_PROTOCOL, 
		GET_VALUE( cgiVariables,  NIC_KEYWORD_PROTOCOL ) );

	if(type == SYSCFG_TYPE_NORMAL)
		sprintf(target, "%s?%s=%s", WEB_URL_SYSCONFIG,CGI_KEYWORD_OPERATOR, CGI_SYSCFG_OP_WAN);
	else
	{
		sprintf(target, "%s?%s=%s", WEB_URL_QUICK_INSTALL, CGI_KEYWORD_OPERATOR, CGI_QUICK_OP_LAN);
		refreshTime = 5;
	}	

	sprintf(title, "%s", gettext(CGI_STR_ETH_CONFIG) );
	len += sprintf(info+len, "<TR><TD align=\"center\"><strong>%s %s</strong></TD></TR>",
			gettext("Detecting DHCP Server "), gettext("in WAN port") );
	len += sprintf(info+len, "<TR><TD align=\"center\"><strong>%s</strong></TD></TR>",
		gettext("Maybe WWW can be access with different URL in your browser") );
	len += sprintf(info+len, "<TR><TD align=\"center\"><strong>%s</strong></TD></TR>",
			gettext(CGI_STR_PLEASE_WAIT) );
	
	cgi_refresh_page(refreshTime, target, title, info );
	fflush(stdout);

	kill_process("udhcpc");
	kill_process("pppoe-connect");
	kill_process("pppd");

	sprintf(target, "/sbin/ifconfig %s 0.0.0.0 down > /dev/null", ETH0 );
	system(target);
	system("/sbin/scripts/eth0.sh >/tmp/udhcpc.tmp");

	cgi_syscfg_update_host_conf();

}

static void __filter_node(llist *l,char * name)
{
	node * current;
	int found=0;
	current=l->head;
	while(current!=NULL)
	{
		if((!strcmp(current->entry.name,name))
			&& (!strcmp(current->entry.value, PPPOE_KEYWORD_DNSTYPE_SERVER )))
		{
			found=1;
		}
		if(!strcmp(current->entry.name,"PWD"))
			list_delete_node(l,current);
		current=current->next;	
	}

	if(found!=0)
	{	
		current=l->head;
		while(current!=NULL)
		{
			if((!strcmp(current->entry.name, PPPOE_KEYWORD_DNS1 ))
			 ||(!strcmp(current->entry.name, PPPOE_KEYWORD_DNS2 )))
			 list_delete_node(l,current);
			current=current->next;
		}
	}

}

void cgi_syscfg_wan_activate_pppoe(llist *cgiVariables )
{
	char cmd[128];
	
	config_file_replace_field(ETH0_CONFIG_FILE, ITEM_EQU, NIC_KEYWORD_PROTOCOL, 
		GET_VALUE(cgiVariables,NIC_KEYWORD_PROTOCOL )  );

	pppoe_wirte_conf(CONF_PPP_PAP,		cgiVariables );
	pppoe_wirte_conf(CONF_PPP_CHAP,	cgiVariables);	
	__filter_node(cgiVariables,  PPPOE_KEYWORD_DNSTYPE );
	writeconfig_no_override(CONF_PPPOE, pppoe_dealine, cgiVariables );
	system("touch  /etc/sys/ppp0");
	
	system("killall -9 pppoe-connect");
	system("killall -9 pppd");
	system("killall -9 udhcpc");
	sleep(2);
	 /*
		   *When ADSL startup, check link status. If account 
		   *is wrong, kill PPPoE process,and display message;
		   */
	CGI_SPRINTF(cmd, 0, "/usr/sbin/pppoe-connect > %s &", PPPOE_LOG);
	system(cmd);
	//fork_cmd(cmd);	
	
//		CGI_SPRINTF(cmd, sizeof(cmd), "%s %s > /dev/null &", PPP_UP_CHECK, PPPOE_LOG); 
//		system(cmd);
	/*
	//The flag created by "ppp_up_check"
	if ( !access(PPP_ERR_FLAG, F_OK) )
	{
		printf("   ADSL's user name or password error, please input again!\n");
		remove(PPP_ERR_FLAG);
	}
	*/

	return;
}



static void __write_dhcp_server_config( char *start,  char *end, char *addr, char *mask, char *dns)
{
#define	DHCP_LINE_NUM		25
#define	DHCP_CONF_FILE		"/etc/udhcpd.conf"

	FILE 		*fp;
	char 		line[1024];
	DHCP_line  	dhcp[DHCP_LINE_NUM];
	int 			i=0;
	
	if( (fp=fopen(DHCP_CONF_FILE,"r") ) == NULL )
		return ;

	memset(line,0,1024);
	while( (i<DHCP_LINE_NUM) && fgets(line,1023,fp) )
	{
		memset(&dhcp[i],0,sizeof(DHCP_line) );
		sscanf(line,"%s %s %s",dhcp[i].name,dhcp[i].value1,dhcp[i].value2);
		if(strlen(dhcp[i].name) )
			i++;
	}

	fclose(fp);
	
	for(i=0;i<DHCP_LINE_NUM;i++)
	{
		if( !strcmp(dhcp[i].name,"start") )
			strcpy(dhcp[i].value1,start);
		if( !strcmp(dhcp[i].name,"end") )
			strcpy(dhcp[i].value1,end);
		if(!strcmp(dhcp[i].name,"opt") )  
		{
			if(!strcmp(dhcp[i].value1,"subnet") )
				strcpy(dhcp[i].value2,mask);
			if(!strcmp(dhcp[i].value1,"router") )
				strcpy(dhcp[i].value2,addr);
			if(!strcmp(dhcp[i].value1,"dns") )
				strcpy(dhcp[i].value2,dns);
		}
	}

	if( (fp=fopen(DHCP_CONF_FILE,"w") ) == NULL )
		return ;

	for(i=0;i<DHCP_LINE_NUM;i++)
	{
		if(!strcmp(dhcp[i].name,"opt") )
			fprintf(fp,"%s %s %s\n",dhcp[i].name,dhcp[i].value1,dhcp[i].value2);
		else
			fprintf(fp,"%s %s\n",dhcp[i].name,dhcp[i].value1);
	}

	fclose(fp);	
}

static void __get_dns_name( char *dns)
{
	char tmp[64];
	
	FILE * fp;
	if( (fp=fopen(CONF_DNS, "r") ) == NULL )
	{
		cgidebug("open file %s error",CONF_DNS);
		return ;
	}

	fscanf(fp,"%s %s",tmp, dns);

	fclose(fp);
}

int cgi_syscfg_lan_update_dhcp_server()
{
	struct in_addr 	addr_in,bcast_in;
	interface_info 		lanInfo;

	char *record=NULL;
	
	char cmd[128];
	char start[20];
	char end[20];
	char dns[20];
	
	memset(dns,0, sizeof(dns) );
	__get_dns_name( dns);

	get_nic_info(&lanInfo, ETH1);
	if(!strlen(lanInfo.addr) || !strlen(lanInfo.bcast) )
	{
		return -1;
	}	
	
	memset(cmd,0,128);
	memset(start,0,20);
	memset(end,0,20);
	
	inet_aton(lanInfo.addr, &addr_in);
	inet_aton(lanInfo.bcast, &bcast_in);

	addr_in.s_addr=addr_in.s_addr+1;
	record=inet_ntoa(addr_in);
	strcpy(start,record);
	record=NULL;
	bcast_in.s_addr=bcast_in.s_addr-1;
	record=inet_ntoa(bcast_in);
	strcpy(end,record);

	__write_dhcp_server_config(start,end, lanInfo.addr,lanInfo.netmask, dns);

	system("killall udhcpd");
	sleep(1);
	system("/usr/sbin/udhcpd & ");	

	return 0;
}



static void __get_iptable_network_address(char * netaddr,char * host, char * mask)
{
	struct in_addr in_host;
	struct in_addr in_mask;
	struct in_addr in_netaddr;

	char * addr;
	
	memset(&in_host,0,sizeof(struct in_addr) );
	memset(&in_mask,0,sizeof(struct in_addr) );
	memset(&in_netaddr,0,sizeof(struct in_addr) );
	
	if(! inet_aton(host,&in_host) )
	{
		fprintf(stderr, "install_getNetworkaddr():inet_aton error" );
		return ;
	}

	if(! inet_aton(mask,&in_mask) )
	{
		fprintf(stderr, "install_getNetworkaddr():inet_aton error"  );
		return ;
	}

	in_netaddr.s_addr=in_host.s_addr & in_mask.s_addr;
	addr= inet_ntoa(in_netaddr);
	sprintf(netaddr,"%s/%s",addr,mask);
}


void cgi_syscfg_lan_auto_iptables( )
{
#define	IPTABLES_DIR		"/usr/sbin"

	interface_info ppp0, eth0, eth1;
	char cmd[256];
	char wan[20];
	char lan[20];
	char local_netaddr[40];

	memset(&ppp0,0,sizeof(interface_info) );
	memset(&eth0,0,sizeof(interface_info) );
	memset(&eth1,0,sizeof(interface_info) );

	get_nic_info(&ppp0,PPP0);
	get_nic_info(&eth0,ETH0);
	get_nic_info(&eth1,ETH1);
	
	if( ( eth1.state==1) && strlen(eth1.addr) )
	{
		__get_iptable_network_address(local_netaddr, eth1.addr, eth1.netmask);
		strcpy(lan, ETH1);
		memset(cmd,0,256);
		if(ppp0.state==1)
		{
			sprintf(cmd,"%s/iptables -t nat -A POSTROUTING -s %s -o ppp0 -p all -j MASQUERADE",IPTABLES_DIR,local_netaddr);
			strcpy(wan,PPP0);
		}
		else
		{
			sprintf(cmd,"%s/iptables -t nat -A POSTROUTING -s %s -o eth0 -p all -j SNAT --to-source %s",IPTABLES_DIR,local_netaddr, eth0.addr);
			strcpy(wan,ETH0);
		}
		system(cmd);	

	}
	else
		return ;

	memset(cmd,0,256);
	//permit local host visit Inernet
	sprintf(cmd,"%s/iptables -A FORWARD -s %s -i %s -o %s -p all -j ACCEPT", IPTABLES_DIR,local_netaddr,lan,wan);
	system(cmd);
	
	//permit Internet response packet pass 
	sprintf(cmd,"%s/iptables -A FORWARD -d %s -i %s -o %s -p all -j ACCEPT", IPTABLES_DIR,local_netaddr,wan,lan);
	system(cmd);

}

current_time_value_t *systime_get_current_time(void)
{
#define	TMP_FILE			"/tmp/current_date.txt"
	current_time_value_t *value;
	FILE *fp;
	char line[PARSE_FILE_MAX_LENGTH];
	char tmp1[64], tmp2[64], month[32];
	char		command[256];
	llist  	sysTimeCfgs;

cgitrace();
	memset(&sysTimeCfgs, 0, sizeof(llist));
	if (readconfig(SYS_LOCALE_CONF, ITEM_EQU, NULL, &sysTimeCfgs) != 0)
	{
		cgi_error_page(NULL, gettext("Locale Operation Failed"), gettext( "No Configuration for System Time" ) );
		return NULL;
	}
cgitrace();
	
	value = malloc(sizeof(current_time_value_t) );
	if(!value)
	{
		cgi_error_page(NULL, gettext(CGI_STR_ERROR), gettext(SYSTIME_STR_SYSTEM_ERROR) );
		return NULL;
	}
	memset(value, 0, sizeof(current_time_value_t ) );

cgitrace();
	sprintf(command,"export TZ=%s; date > "TMP_FILE, GET_VALUE(&sysTimeCfgs, SYS_TIMEZONE)  );
	system(command);
cgitrace();

	if( ( fp=fopen(TMP_FILE,"r"))==NULL)
	{
		cgi_error_page(NULL, gettext(CGI_STR_ERROR), gettext(SYSTIME_STR_SYSTEM_ERROR) );
		return NULL;
	}
	
	memset(line,0,sizeof(line));
	while (fgets(line,PARSE_FILE_MAX_LENGTH, fp))
	{
		sscanf(line,"%s %s %s %s %s %s",
			tmp1, month,value->day,value->current_time,tmp2,value->year );

		if(!strcmp(month,"Jan"))
			strcpy(value->month,"01");	
		else if(!strcmp(month,"Feb"))
			strcpy(value->month,"02");
		else  if(!strcmp(month,"Mar"))
			strcpy(value->month,"03");
		else if(!strcmp(month,"Apr"))
			strcpy(value->month,"04");
		else if(!strcmp(month,"May"))
			strcpy(value->month,"05");
		else if(!strcmp(month,"Jun"))
			strcpy(value->month,"06");
		else if(!strcmp(month,"Jul"))
			strcpy(value->month,"07");
		else if(!strcmp(month,"Aug"))
			strcpy(value->month,"08");
		else if(!strcmp(month,"Sep"))
			strcpy(value->month,"09");
		else if(!strcmp(month,"Oct"))
			strcpy(value->month,"10");
		else if(!strcmp(month,"Nov"))
			strcpy(value->month,"11");
		else if(!strcmp(month,"Dec"))
			strcpy(value->month,"12");

	}
	
	fclose(fp);
cgitrace();
	
	return value;
}


