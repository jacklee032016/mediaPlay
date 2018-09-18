/*
* $Id: cgi_services.c,v 1.4 2007/03/06 11:18:21 lizhijie Exp $
*/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libCgi.h"

#define SERVICE_IS_VALIDATE		1
#define SERVICE_UN_VALIDATE		0

static proclist *servicesList = NULL;

#if SERVICES_DEBUG
int __proclist_length(proclist *list)
{
	proclist *tmp = list;
	int i=0;

	if(!list)
		return 0;

	while( tmp )
	{
		i++;
		tmp=tmp->next;
	}
	return i;
}
#endif

proclist *__proclist_create( )
{
	proclist *newnode;
	newnode=(proclist*)malloc(sizeof(proclist));
	if (!newnode)
		return NULL;
	memset(newnode, 0, sizeof(proclist));
	
	return newnode;
}

void __proclist_add(proclist *list, ServiceNode *node)
{
	proclist *tmp = list, *next;
	if(!list->node )
	{
		list->node = node;
		return ;
	}

	next = tmp->next;
	while( next )
	{
		tmp = next;
		next = next->next;
	}

	tmp->next = __proclist_create() ;
	tmp->next->node=node;

	return;
}


/* callback function passed into config file parser library */
static void __services_conf_parser(char *line, void *data)
{
	proclist *conf = (proclist *)data;
	char command[128],  display[64] ,name[64], startup[64];

	memset(command, 0, 64);
	memset(display, 0, 64);
	
	if(sscanf(line, "%s %s %s %s",name, display, command, startup )== 4 )
	{
		ServiceNode *node;
		node=(ServiceNode *)malloc(sizeof(ServiceNode));
		memset(node, 0, sizeof(ServiceNode));
		
		strcpy(node->name, name);
		strcpy(node->display_name, display);
		strcpy(node->command, command);
		if(!strcasecmp(startup, "on" ) )
			node->startup = 1;
		else
			node->startup = 0;
#if SERVICES_DEBUG
		cgidebug( "%s %s %s %s\r\n", name, display, command, startup);
#endif

		__proclist_add( conf, node);
	}
}


/* check whether this program is in our services list 
and update the pid data member in node
*/
static int __is_valid_service(char *program_name, int pid, proclist *list)
{
	proclist 		*tmp = list;
	ServiceNode	 *node;

	while(tmp)
	{
		node = tmp->node;
		if(strcmp(program_name, node->name )==0)
		{
			node->state = SERVICE_STATE_RUNNING;
			node->pid = pid;
			return node->state;
		}
		tmp= tmp->next;
	}
	
	return SERVICE_STATE_INVALIDATE;
}

static void __service_check_service_status(proclist *list)
{
	struct dirent **namelist;
	int n;
	char file[256];
	FILE *fp;

	struct info_type
	{
 		char waste[16];
		char value[64];
	}serverinfo;
	proclist *tmp = list;
	ServiceNode *node;
	
	while(tmp)
	{
	   	node = tmp->node;
		node->pid=0;
		node->state=0;
       	tmp= tmp->next;
	}
		
	n = scandir("/proc", &namelist,  cgi_directory_filt_by_number, alphasort);
	while(n--)
	{
		sprintf(file,"/proc/%s/status", namelist[n]->d_name);
		if((fp=fopen(file,"r"))==NULL)
		{
			cgidebug("open status file '%s' fail:%s\n", file, strerror(errno) );
			continue;
		}

		memset(&serverinfo, 0, sizeof(struct info_type));
		
		/*get the server name*/
		fscanf(fp, "%s %s", serverinfo.waste, serverinfo.value);
		__is_valid_service(serverinfo.value, atoi(namelist[n]->d_name), list) ;
		fclose(fp);
		free(namelist[n]);
	}
	
	free(namelist);
}


int __service_command_action(proclist *list, char *command, char *action)
{
	proclist *tmp = list;
	ServiceNode *node;

	while(tmp)
	{
		node = tmp->node;
		
		if( !strcmp(node->name, command) )
		{
			if(!strcmp(action, SERVICE_ACTION_START ) )
			{/* start */
				if(node->state==SERVICE_STATE_SLEEPING)
					fork_cmd(node->command );
				if(!strcmp(node->name,"klogd"))
				{
					system("/sbin/syslogd");
				}
			}
			else
			{/* stop */
				cgi_send_signal_to_process_by_name(SIGKILL, node->name);
				if(!strcmp(node->name,"klogd"))
				{
					pid_t pid;
					char cmd[128];
					memset(&cmd,0,sizeof(cmd));
					pid = (pid_t)cgi_get_pid_by_name("syslogd");
					sprintf(cmd,"kill -9 %d",pid);
					system(cmd);
				}
			}
		}
		tmp= tmp->next;
	}

	return 0;
}


char *cgi_service_state_name(service_state_t state)
{
	char		msg[128];
	switch(state)
	{
		case SERVICE_STATE_RUNNING:
			sprintf(msg, "<font color=#00ff00>%s</font>", gettext(CGI_STR_RUNNING));
			break;
		case SERVICE_STATE_SLEEPING:
			sprintf(msg, "<font color=#ff0000>%s</font>", gettext(CGI_STR_STOP));
			break;
		default:
			sprintf(msg, "<font color=#ff0000>%s</font>", gettext("Invalidate"));
			break;
	}

	return strdup(msg);
}

proclist *cgi_services_list()
{
	if(!servicesList )
	{
		servicesList = __proclist_create();

		if(cgi_read_local_config(SERVICES_LIST_FILE, __services_conf_parser, servicesList) == EXIT_FAILURE)
		{
			return NULL;
		}
//	cgidebug("ret list length is before iteration 1: %d\r\n", get_list_length(conf_list));
		__service_check_service_status(servicesList);
	}
	
	return servicesList;	
}

service_state_t cgi_service_get_state(char *command /* first field in config file*/)
{
	proclist *services = cgi_services_list();
	ServiceNode	 *node;

	while( services )
	{
		node = services->node;
		if(strcmp(command, node->name )==0)
		{
			return node->state;
		}
		services= services->next;
	}
	return SERVICE_STATE_INVALIDATE;
}

char *cgi_service_get_state_name(char *command)
{
	return cgi_service_state_name(cgi_service_get_state(command) );
}

service_state_t cgi_service_start(char *command /* first field in config file*/)
{
	proclist *services = cgi_services_list();
	__service_command_action(services, command, SERVICE_ACTION_START);
	return 0;
}

service_state_t cgi_service_stop(char *command /* first field in config file*/)
{
	proclist *services = cgi_services_list();
	__service_command_action(services, command, SERVICE_ACTION_STOP);
	return 0;
}

service_state_t cgi_service_restart(char *command /* first field in config file*/)
{
	if(cgi_service_get_state(command) == SERVICE_STATE_RUNNING)
	{
		cgi_service_stop(command);
		sleep(3);
	}
	cgi_service_start(command);
	return 0;
}

void cgi_services_stop_all()
{
	ServiceNode	 *node;
	proclist *servicesInfo = cgi_services_list();
	proclist *tmp = servicesInfo;

	while( tmp )
	{
		node = tmp->node;
		if(node->state == SERVICE_STATE_RUNNING)
			__service_command_action(servicesInfo, node->name, SERVICE_ACTION_STOP);
		tmp = tmp->next;
	}
	
	return;
}

service_state_t  cgi_get_program_state( char *name ,char *command)
{
	proclist  *programs = __proclist_create();
	ServiceNode *node = (ServiceNode *)malloc(sizeof(ServiceNode));
	memset(node, 0, sizeof(ServiceNode));
		
	strcpy(node->name, name);
//	strcpy(node->display_name, display);
	if(command)
		strcpy(node->command, command);

	__proclist_add(programs, node);
	__service_check_service_status( programs);

	return node->state;
}

/* get status of a program which is not in service list */
char *cgi_get_program_state_name(char *name)
{
	return cgi_service_state_name( cgi_get_program_state(name, NULL) );
}

/* detect program is running. >0 running eg. process ID; <= 0, not running */
int  cgi_get_program_pid_with_param(char *cmd, char *param)
{
	struct dirent **namelist;
	int n;
	char file[256];
	FILE *fp;
	int pid = 0;

	char line[1024];
	
	n = scandir("/proc", &namelist,  cgi_directory_filt_by_number, alphasort);
	while(n--)
	{
		sprintf(file,"/proc/%s/cmdline", namelist[n]->d_name);
		if((fp=fopen(file,"r"))==NULL)
		{
			cgidebug("open status file '%s' fail:%s\n", file, strerror(errno) );
			continue;
		}

		memset(line, 0, sizeof(line) );
		fscanf(fp, "%s", line );
		
		fclose(fp);
		free(namelist[n]);
		
		if( strstr(line, cmd) )
		{
			if(!param )
				pid = cgi_get_pid_by_name(cmd);
			else if( strstr(line, param) )
				pid = cgi_get_pid_by_name(cmd);
			/* continue to free namelist */
		}
	}
	
	free(namelist);

	return pid;
}


