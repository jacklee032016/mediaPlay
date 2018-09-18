/*
* $Id: cmn_time.c,v 1.2 2007/03/07 16:18:08 lizhijie Exp $
*/

#include <cmnOsPort.h>

#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <ctype.h>

int cmnGetIp(char *ip_addr)
{
	struct ifreq ifr;
	struct sockaddr_in *addr;
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	addr = (struct sockaddr_in *)&(ifr.ifr_addr);
	
	addr->sin_family = AF_INET;
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
	if (ioctl(sfd, SIOCGIFADDR, &ifr) < 0)
		return -1;
	strcpy(ip_addr, inet_ntoa(addr->sin_addr));
	return 0;
}

/*
 * Set IP and net mask of eth0
 * return value: 
 *	 0 - success
 *  -1 - socket error
 *	-2 - set ip failure
 *  -3 - set netmask failure
 */
int cmnSetIpNetmask(char *ip_address, char *net_mask)
{
	struct ifreq ifr;
	struct sockaddr_in *addr;
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));
	addr = (struct sockaddr_in *)&(ifr.ifr_addr);
	
	addr->sin_family = AF_INET;
	if (ip_address == NULL)
		return -2;
	addr->sin_addr.s_addr = inet_addr(ip_address);
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
	if (ioctl(sfd, SIOCSIFADDR, &ifr) < 0)
		return -2;

	if (net_mask)
		addr->sin_addr.s_addr = inet_addr(net_mask);
	else
		addr->sin_addr.s_addr = inet_addr("255.255.255.0");
	if (ioctl(sfd, SIOCSIFNETMASK, &ifr) < 0)
		return -3;
	return 0;
}

int set_dns(char *dns)
{
	FILE *fp;
	char line[80];
	int flag = 0;
	fp = fopen("/etc/resolv.conf", "a+");
	if (fp == NULL)
		return -1;
	if (dns == NULL)
		return -1;

	while (fscanf(fp, "%s", line) != -1)
	{
		if (!strncmp(line, "nameserver", 10))
		{
			flag = 1;
			continue;
		}
		if (flag)
		{
			if (!strncmp(line, dns, strlen(dns)))
				return -1;
			flag = 0;
		}
	}
	fprintf(fp, "nameserver %s\n", dns);

	fclose(fp);
	return 0;
}



void add_gettimeofday (struct timeval *atv, int ms)
{
	int m;
	if (ms>=1000000)
	{
		atv->tv_usec = 0;
		m = ms / 1000;
	}
	else
	{
		atv->tv_usec += ms * 1000;
		m = atv->tv_usec / 1000000;
		atv->tv_usec = atv->tv_usec % 1000000;
	}
	atv->tv_sec += m;
}

void min_timercmp (struct timeval *tv1, struct timeval *tv2)
{
	if (tv2->tv_sec == -1)
		return;

	if (timercmp (tv1, tv2, >))
	{
		/* replace tv1 with tv2 info */
		tv1->tv_sec = tv2->tv_sec;
		tv1->tv_usec = tv2->tv_usec;
	}
}


void *cmn_malloc(size_t size)
{
	void *ptr = malloc(size);
	if(ptr!=NULL)
	{
		memset(ptr,0,size);
	}
	else
	{
		fprintf(stderr, "cmn_malloc failed\n");
		exit(1);
	}
	return ptr;
}

void *cmn_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

void cmn_free(void *ptr)
{
	if (ptr==NULL)
		return;
	free(ptr);
	ptr = NULL;
}


void cmn_usleep (int useconds)
{
	struct timeval delay;
	int sec;

	sec = (int) useconds / 1000000;
	if (sec > 0)
	{
		delay.tv_sec = sec;
		delay.tv_usec = 0;
	}
	else
	{
		delay.tv_sec = 0;
		delay.tv_usec = useconds;
	}
	select (0, 0, 0, 0, &delay);

}

void cmn_delay(int ms)
{
	int was_error;
	struct timeval tv;

	/* Set the timeout interval - Linux only needs to do this once */
	tv.tv_sec = ms/1000;
	tv.tv_usec = (ms%1000)*1000;
	do
	{
		errno = 0;

#if _POSIX_THREAD_SYSCALL_SOFT
		pthread_yield_np();
#endif

		was_error = select(0, NULL, NULL, NULL, &tv);

	} while ( was_error && (errno == EINTR) );

}

long cmnGetTimeMs(void)
{
	struct timeval tv;

	gettimeofday(&tv,NULL);
	return (long long)tv.tv_sec * 1000 + (tv.tv_usec / 1000);
}


char *cmnGetCtime1(char *buf2, size_t buf_size)
{
	time_t ti;
	char *p;

	ti = time(NULL);
	p = ctime(&ti);

	if (!p || !*p)
	{
		*buf2 = '\0';
		return buf2;
	}

#if 0	
	av_strlcpy(buf2, p, buf_size);
#else
	snprintf(buf2, buf_size, "%s", p );
#endif
	p = buf2 + strlen(buf2) - 1;
	if (*p == '\n')
		*p = '\0';
	return buf2;
}

/*
#if defined(CONFIG_WIN32) || defined(CONFIG_OS2)
static int64_t getutime(void)
{
	return av_gettime();
}
#else
static int64_t getutime(void)
{
	struct rusage rusage;

	getrusage(RUSAGE_SELF, &rusage);
	return (rusage.ru_utime.tv_sec * 1000000LL) + rusage.ru_utime.tv_usec;
}
#endif
*/


void cmnHexDump(const uint8_t *buf, int size)
{
	int len, i, j, c;

	for (i = 0; i < size; i += 16)
	{
		len = size - i;
		if (len > 16)
			len = 16;

		fprintf(stderr, "%08x ", i);
		for (j = 0; j < 16; j++)
		{
			if (j < len)
				fprintf(stderr, " %02x", buf[i + j]);
			else
				fprintf(stderr, "   ");
		}
		
		fprintf(stderr, " ");

		for (j = 0; j < len; j++)
		{
			c = buf[i + j];
			if (c < ' ' || c > '~')
				c = '.';
			fprintf(stderr, "%c", c);
		}
		
		fprintf(stderr, "\n");
	}
}

void cmnParseGetArg(char *buf, int buf_size, const char **pp)
{
	const char *p;
	char *q;
	int quote = 0;

	p = *pp;
	q = buf;

	while (isspace(*p))
		p++;

	if (*p == '\"' || *p == '\'')
		quote = *p++;

	while (*p != '\0')
	{
		if ( (quote && *p == quote) ||( !quote && isspace(*p)))
			break;
		
		if ((q - buf) < buf_size - 1)
			*q++ = *p;
		p++;
	}

	*q = '\0';
	if (quote && *p == quote)
		p++;
	
	*pp = p;
}

#if 0
void cmnParseGetArg(char *buf, int buf_size, const char **pp)
{
	const char *p;
	char *q;
	int quote;

	p = *pp;
	while (isspace(*p))
		p++;
	
	q = buf;
	quote = 0;
	if (*p == '\"' || *p == '\'')
		quote = *p++;
	
	for(;;) 
	{
		if (quote)
		{
			if (*p == quote)
				break;
		}
		else
		{
			if (isspace(*p))
				break;
		}
		if (*p == '\0')
			break;
		
		if ((q - buf) < buf_size - 1)
			*q++ = *p;
		
		p++;
	}
	*q = '\0';
	
	if (quote && *p == quote)
		p++;
	*pp = p;
}
#endif

int cmnParseGetIntValue( const char **pp)
{
	char arg[1024];
	
	cmnParseGetArg(arg, sizeof(arg), pp);
	return atoi(arg);
}

int cmnParseGetBoolValue( const char **pp)
{
	char arg[1024];
	
	cmnParseGetArg(arg, sizeof(arg), pp);
	
	if (!strcasecmp(arg, "YES"))
	{
		return TRUE;
	}
	
	return FALSE;
}

int cmnParseGetIpAddress(struct in_addr *ipAddress, const char *p)
{
	if (!inet_aton(p, ipAddress))
	{
		fprintf(stderr, "Invalid IP address: %s\n", p);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


int	cmnParseGetHexIntValue(char *hexString)
{
	int value =0;
	
	sscanf(hexString, "%x", &value);

	return value;
}



#include <dirent.h>
int cmnDirectoryFiltByNumber(const struct dirent *namelist)
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


int cmnGetPidByName(char *prog_name)
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
	
	n = scandir("/proc", &namelist,  cmnDirectoryFiltByNumber, alphasort);
	while(n--)
	{
		sprintf(file,"/proc/%s/status", namelist[n]->d_name);
		if((fp=fopen(file,"r"))==NULL)
		{
			fprintf(stderr, "open status file '%s' fail:%s\n", file, strerror(errno) );
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



void cmnKillProcess(char * processName)
{
	pid_t pid=-1;
	char cmd[128]="\0";
	while(1)
	{
		pid=cmnGetPidByName(processName);
		if(pid<0)
		{
			fprintf(stderr, "Process '%s' is not found\n", processName);
			break;
		}
		sprintf(cmd,"kill -9 %d",pid);
		system(cmd);
		pid=-1;
	}
	return ;
}


void cmnForkCommand(const char *cmd) 
{
	char command[1024];
	snprintf(command, sizeof(command), "%s >> /dev/null" , cmd);
	
	if(!fork()) 
	{
		system(command);
		exit(0);
	}
	sleep(1); /* one second */
	return;
}


