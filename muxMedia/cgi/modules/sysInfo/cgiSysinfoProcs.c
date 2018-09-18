/*
* $Id: sysInfoProcs.c,v 1.6 2007/09/06 09:19:59 lizhijie Exp $
*/

#include "libCgi.h"

#define	SYS_PROC_MUX_PLAYER		"muxPlayer"

typedef struct
{
	char 	description[128];
	char		command[128];
	
	int		isRunning;
	pid_t 	pid;
}program_state;

program_state sys_program[]=
{
	{
		CGI_STR_MUX_SERVER,
		SYS_PROC_MUX_PLAYER,
		1
	}
};

static int __sysproc_manipulate(char *command)
{
	char 			cmd[128];
	program_state 	*proc = NULL;
	int				i;

	for(i=0; i<sizeof(sys_program)/sizeof(sys_program[0]) ;i++)
	{
		sys_program[i].pid=(pid_t)cgi_get_pid_by_name(sys_program[i].command );
		if(sys_program[i].pid<0)
			sys_program[i].isRunning = 0;
		else
			sys_program[i].isRunning = 1;
		
		if(!strcasecmp(command, sys_program[i].command) )
		{
			proc = &sys_program[i];
		}
	}

	if(!proc)
	{
		cgi_error_page(NULL, gettext(CGI_STR_ERROR), gettext("Task Can not be Found") );
		return -1;
	}

	if( proc->isRunning )
	{/* stop */
#if 0	
		pid=(pid_t)cgi_get_pid_by_name(proc->command );				
		if(pid>0)
#endif			
		{
			if(!strcmp(proc->command, SYS_PROC_MUX_PLAYER) )
			{
//				kill(sys_program[i].pid,SIG_CGI_TO_PBX);
//				kill(proc->pid, SIG_CGI_TO_PBX);
				kill(proc->pid, SIG_CGI_TO_PBX);
				return 0;
			}
//			kill(sys_program[i].pid,9);	/* why ? */
			kill(proc->pid,9);	/* why ? */

			return 0;
		}
#if 0		
		else
		{
			sprintf(cmd, "'%s' %s", proc->description, gettext("is not running now") );
			cgi_error_page(NULL, gettext(CGI_STR_ERROR), cmd );
			return -1;
		}
#endif		
//		cgidebug("kill %s(PID:%d)\n",proc->command, proc->pid);
		/*memset(cmd,0,sizeof(cmd));*/
		sprintf(cmd,"killall -9 %s &",proc->command);
		system(cmd);
		
	}
	else
	{/* start */
#if 0	
		pid=(pid_t)cgi_get_pid_by_name( proc->command);		
		if(pid<0)
#endif			
		{
			memset(cmd,0,sizeof(cmd));
//			sprintf(cmd,"/usr/bin/%s >/dev/null &", proc->command );
			sprintf(cmd,"/sbin/%s 1>&2 > /dev/null &", proc->command );
			system(cmd);
			return 0;
		}
#if 0
		else
		{
			sprintf(cmd, "'%s' %s", proc->description, gettext("has been in running now") );
			cgi_error_page(NULL, gettext(CGI_STR_ERROR), cmd );
			return -1;
		}
#endif		
	}

	return 0;	
}

/* Server IP Input box for both restore and backup*/
char *__sysinfo_tasks_button(SYS_INFO *info )
{
	char 	buf[8192*5];
	int 		length = 0;
	char		target[256];
	int		i;

/* table 1 */	
	length += CGI_SPRINTF(buf,length, "<table width=\"85%%\" border=\"1\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	length += sprintf(buf+length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n</TR>\n", 
		gettext("Task Name"), gettext(CGI_STR_STATUS), gettext(CGI_STR_MANAGEMENT) );

	for(i=0; i<sizeof(sys_program)/sizeof(sys_program[0]) ;i++)
	{
		sprintf(target, "%s?%s=%s&%s=%s", WEB_URL_SYSTEM_INFO, CGI_KEYWORD_OPERATOR, CGI_SYSINFO_OP_SYS_PROC,
			CGI_SYSINFO_KEYWORK_PROC_NAME, sys_program[i].command );

		length += sprintf(buf+length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
			gettext(sys_program[i].description), (sys_program[i].isRunning==0)?gettext(CGI_STR_STOP):gettext(CGI_STR_RUNNING),
			cgi_button((sys_program[i].isRunning==0)?gettext("Start"):gettext(CGI_STR_STOP), target) );

	}
	
//	length += CGI_SPRINTF(buf,length, "<TR><TD align=\"center\" colspan=3>%s\n</TD></TR>\r\n",cgi_help_button(CGI_HELP_MSG_SYSTEM_PROC) );

	length += CGI_SPRINTF(buf,length, "</TABLE><table width=\"85%%\" border=\"0\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\" bgcolor=\"#FFFFFF\">\n");
	length += CGI_SPRINTF(buf,length, "<TR><TD><strong>%s</strong><br>* %s<strong>'%s'</strong>,%s, %s. \r\n</TD></TR>\r\n",
		gettext(CGI_STR_NOTE), gettext("Stop "), 
		gettext(CGI_STR_MUX_SERVER), gettext(" will cost some more time") , gettext("So you may wait this page refreshing several times to get the result") );
	
	sprintf(target, "%s",WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_SYS_PROC);
	length += sprintf(buf+length, cgi_refresh_script(WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_SYS_PROC, 5) );

	return strdup(buf);
}

int cgiSysInfo_Tasks(SYS_INFO *info)
{
	char 	*taskName;
	int		i;
	
	taskName = GET_VALUE(&info->cgiVariables, CGI_SYSINFO_KEYWORK_PROC_NAME );
	if(taskName && strlen(taskName) )
	{
		
		if(__sysproc_manipulate(taskName))
			return 1;
		
		cgi_refresh_page(1, WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_SYS_PROC, gettext("System Task"), gettext("Please waiting") );
		return 0;
	}
 	
	for(i=0; i<sizeof(sys_program)/sizeof(sys_program[0]) ;i++)
	{
		sys_program[i].pid=(pid_t)cgi_get_pid_by_name(sys_program[i].command );
		if(sys_program[i].pid<0)
			sys_program[i].isRunning = 0;
		else
			sys_program[i].isRunning = 1;
	}
	cgi_info_page(gettext("System Task"), gettext("Status of System Task") , __sysinfo_tasks_button(info) );

	return 0;	
}


