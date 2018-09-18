/*
* $Author: lizhijie $
* $Revision: 1.4 $
*/
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>


#include "libCgi.h"
#include "libCgiSyscfg.h"

static char *__service_status(ServiceNode *node)
{
	char buf[128];
	if(node->state==SERVICE_STATE_SLEEPING)
	{
		sprintf(buf, "<font color=#999999>%s</font>", gettext(CGI_STR_STOP) );
	}
	else
	{
		sprintf(buf, "<font color=#ff0000>%s</font>", gettext(CGI_STR_RUNNING) );
	}
	return strdup(buf);
}

static char *__service_status_list( )
{
	char buf[8192*2];
	int length = 0;
	char target[128];
	proclist *tmp = cgi_services_list();
	ServiceNode *node;

	length += sprintf(buf+length, "<form name=\"serviceForm\" method=\"post\" action=\"%s\">\r\n", WEB_URL_SYSTEM_INFO );
	length += sprintf(buf+length, "<input type=\"hidden\" name=\"%s\" value=\"%s\">\r\n", CGI_KEYWORD_OPERATOR, CGI_SYSINFO_OP_SERVICES);
	length += sprintf(buf+length, "<input type=\"hidden\" name=\"%s\" value=\"%s\">\r\n", CGI_ACTION, CGI_ACTION_SELECT);
	
	length += sprintf(buf+length, "<tr><td align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></td><td align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></td><td align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></td><td align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></td></tr>", 
		gettext(CGI_STR_NAME), gettext(CGI_STR_STATUS), gettext("Action"), gettext("Startup When Boot") );

	while(tmp)
	{
		node = tmp->node;
		
//	cgidebug("ret list length is in iteration %d\r\n", get_list_length(tmp));
		length += sprintf(buf+length, "<tr><td align=\"center\" ><strong>%s</strong></td>",
			node->display_name );
		length += sprintf(buf+length, "<td align=\"center\" ><strong>%s</strong></td>",
			__service_status(node) );

		sprintf(target, "%s?%s=%s&%s=%s&service=%s", WEB_URL_SYSTEM_INFO, CGI_KEYWORD_OPERATOR, CGI_SYSINFO_OP_SERVICES,
			CGI_ACTION, (node->state==SERVICE_STATE_RUNNING)?CGI_ACTION_DEACTIVATE:CGI_ACTION_ACTIVATE, node->name );
		length += sprintf(buf+length, "<td align=\"center\" ><strong>%s</strong></td>",
			cgi_button((node->state==SERVICE_STATE_RUNNING)? gettext(CGI_STR_STOP):gettext("Start"), target ) );

		length += sprintf(buf+length, "<td><input name=\"%s\" value=\"on\" type=\"radio\" %s>%s<input name=\"%s\" value=\"off\" type=\"radio\" %s>%s</td></tr>",
			node->name, (node->startup==1)?"checked":"" , gettext(CGI_STR_YES), node->name, (node->startup==0)?"checked":"" ,gettext(CGI_STR_NO) );

		tmp= tmp->next;
	}

	return strdup(buf);
}

static char *__service_add_buttons()
{
	char buf[2048*2];
	int length = 0;
	
	length += sprintf(buf+length, "<TR><TD align=\"center\">%s%s</TD></TR></FORM>\r\n",
		cgi_submit_button(gettext(CGI_STR_SAVE) ), cgi_help_button(CGI_HELP_MSG_SERVICES) );
	
	length += sprintf(buf+length, cgi_refresh_script(HILINK_SERVICE_CFG, 10) );

	return strdup(buf);
}

void __services_write_list_script(SYS_INFO *info)
{
	FILE *fp, *fp2;
	proclist *tmp = cgi_services_list();
	ServiceNode *node;
	char msg[128];
	char sysCmd[256];
	char *startup;
	
	fp = fopen(SERVICES_LIST_FILE, "w");
	if(!fp)
	{
		cgidebug("Can not open configuration file:'%s'\r\n", SERVICES_LIST_FILE);
		return;
	}
	fp2 = fopen(SERVICES_LIST_SCRIPTS, "w");
	if(!fp2)
	{
		cgidebug("Can not open configuration file:'%s'\r\n", SERVICES_LIST_SCRIPTS);
		return;
	}

	fprintf(fp, "#Services List file for SONiX IP Camera\r\n#Please don't edit this file manully\n\n");
	fprintf(fp2, "# /bin/sh\n#Services startup script executing when startup for SONiX IP Camera\n#Please don't edit this file manully\n");
	while(tmp)
	{
		node = tmp->node;

		startup = GET_VALUE(&info->cgiVariables,  node->name );
		if(! startup)
		{
			sprintf(msg, "%s '%s' %s", gettext("Service Name"), node->name, gettext("is not found") );
			cgi_error_page(NULL, gettext("Service Error"), msg);
			break;
		}

		if(!strcasecmp(startup, "on" ) )
		{
			fprintf(fp2, "echo startup %s .....\n%s\n\n", node->display_name, node->command );
			fprintf(fp, "%s\t%s\t%s\t%s\n", node->name, node->display_name, node->command,  "on" );
		}
		else// if(strcasecmp(startup, "off" ))
		{
			fprintf(fp, "%s\t%s\t%s\t%s\n", node->name, node->display_name, node->command,  "off" );
		}
		
		tmp= tmp->next;
	}

	sprintf(sysCmd, "chmod 755 %s", SERVICES_LIST_SCRIPTS );
	system(sysCmd);
	
	fclose(fp);
	fclose(fp2);

}

int	cgiSysInfo_ServiceConfig(SYS_INFO *info)
{
	char *action;
	char *service;

	proclist *tmp = cgi_services_list();
	if(tmp == NULL)
	{
		cgitrace();
		cgi_error_page(NULL, gettext("Local Operation Failed"), gettext( "No Configuration for System Service" ) );
		return -1;
	}

	action = GET_CGI_ACTION(&info->cgiVariables );
	if(!action|| !strlen(action))
	{
		cgi_info_page(gettext("Service List"), __service_status_list(), __service_add_buttons()  );
//		cgi_info_page(gettext("Service List"), service_status_list(), NULL );
		return 0;
	}
	else if(!strcasecmp(action,CGI_ACTION_ACTIVATE) || !strcasecmp(action, CGI_ACTION_DEACTIVATE) )
	{
		service = GET_VALUE( &info->cgiVariables,  "service");
		if(!service)
		{
			cgidebug("Service is not defined for  command '%s'\n", action );
			cgi_invalidate_page();
			return 0;
		}
		if(!strcasecmp(action, CGI_ACTION_ACTIVATE) )
			cgi_service_start(service);
		else
			cgi_service_stop(service);
		
		cgi_refresh_page(1, HILINK_SERVICE_CFG, gettext("Service Update"), gettext("Service is updating, please waiting....") );
//		cgi_info_page(gettext("Service List"), service_status_list(), service_add_buttons()  );
		return 0;
	}
	else if(!strcasecmp(action,CGI_ACTION_SELECT))
	{
		__services_write_list_script(info);
		cgi_info_page(gettext("Service List"), __service_status_list(), __service_add_buttons()  );
		return 0;
	}
	else
	{
//		cgidebug("Servicel command '%s' is not found\n", action );
		cgi_invalidate_page();
		return 0;
	}

	return 0;
}

