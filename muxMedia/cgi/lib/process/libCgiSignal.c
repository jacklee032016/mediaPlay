/*
* $Author: lizhijie $
* $Revision: 1.2 $
*/

#include "libCgi.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

int cgi_directory_filt_by_number(const struct dirent *namelist)
{
	char  *c;
	int isnum=1;

	c = (char *)namelist->d_name;
	while( *c != '\0' )
	{
		if((*c)<48 || (*c)>57)/*'0'=48,'9'=57*/
		{
			isnum=0;
			break;
		}
		c++;
	}
	return isnum;
}


int cgi_signal_register_handler(int sigkey, void (*sig_handler)(int, siginfo_t *, void *))
{
	struct sigaction act;

	sigemptyset(&act.sa_mask);
	act.sa_flags=SA_SIGINFO;
	act.sa_sigaction=sig_handler;

	return sigaction(sigkey, &act, NULL);
}

void cgi_sig_send(pid_t pid, int sigid, int opcode)
{
	union sigval sval;

	sval.sival_int=opcode;

	// should enhanced uClibc, lzj
	sigqueue(pid, sigid, sval);
}

int cgi_get_pid_by_name(char *prog_name)
{
	struct dirent **namelist;
	int n;
	char file[256];
	FILE *fp;
	int pid = -1;
	
	struct info_type
	{
		char waste[16];
		char value[64];
	}serverinfo;
	
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
		if(!strcmp( serverinfo.value, prog_name) )
			pid = atoi( namelist[n]->d_name );
		if(!strcmp(serverinfo.waste,"State:"))
			if(strchr(serverinfo.value,'Z'))
				pid=-1;
		fclose(fp);
		free(namelist[n]);
	}
		
	free(namelist);
	return pid;
}

void cgi_send_signal_to_process_by_name(int sig, char *prog_name)
{
	pid_t pid;

	pid = (pid_t)cgi_get_pid_by_name( prog_name);
	if( pid <0 )
		return;

	kill(pid, sig);
}

void fork_cmd(const char *cmd) 
{
	char command[1024];
//	snprintf(command, sizeof(command), "%s >> /dev/null" , cmd);
	snprintf(command, sizeof(command), "%s 1>&2 /dev/null" , cmd);
	
	if(!fork()) 
	{
		system(command);
		exit(0);
	}
	sleep(1); /* one second */
	return;
}

