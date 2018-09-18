/*
 * $Id: cmn_version.c,v 1.3 2007/03/16 19:29:34 lizhijie Exp $
*/

#include "cmn_common_version.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>
#include "libCmn.h"

static int	_cmnLibOps = 0;

int cmn_ver_log( char *modulename, char *info)
{
	FILE *fp;
	char file[256];

	sprintf(file,"%s/%s.ver",CMN_VERSION_LOGPATH,modulename);

	fp=fopen(file,"w");
	
	if(!fp)
	{
		printf("can not create version log %s\r\n", file );
		exit(-1);
	}

	fprintf(fp, "%s", info);

	fclose(fp);
       return 0;
}

int cmn_ver_opt (int argc, char **argv, char *info) 
{
	 int c;

	int option_index = 0;
	static struct option long_options[] = 
	{
		{"version", 0, 0, 0},
		{0, 0, 0, 0}
	};
       
	while (1)
	{
		c = getopt_long (argc, argv, "v",long_options, &option_index);
		if (c == -1)
		break;

		switch (c)
		{
			case 'v':
			case 0:

				#if  __KERNEL__
					printk("%s", info);
				#else
					printf("%s", info);						
				#endif
				
				exit(0);
				
				break;

		}
		option_index++;
	}
	return 0;
}

int cmn_ver_debug( char *verInfo)
{
	#if  __KERNEL__
		printk(verInfo);
	#else
		printf(verInfo);
	#endif
	
	return 0;
}

/* return : 0 : success */
int	cmnLibInit(int	options)
{
	int 	res = EXIT_SUCCESS;
#ifdef   __CMN_RELEASE__
#else
	_cmnLibOps |= CMNLIB_OPS_DEBUG;
#endif

	if( (_cmnLibOps&CMNLIB_OPS_DEBUG) != (options&CMNLIB_OPS_DEBUG) )
	{
		if( (_cmnLibOps&CMNLIB_OPS_DEBUG) )
			printf("Shared Library Support Debug, but your program is not\r\n");
		else
			printf("Shared Library not support Debug, but your program dose\r\n");
		exit(1);
	}
	
	res = cmnThreadLibInit();

#if CMN_TIMER_START_DELAY
#else
#if CMN_TIMER_DEBUG
	fprintf(stderr,"Timer inited");
#endif
	res = cmn_timer_init();
	if(res != EXIT_SUCCESS)
	{
		MUX_ERROR("timer initialization failed");
		exit(1);
	}
#endif

	return res;
}

void cmnLibDestroy(void)
{
	cmn_timer_quit();

	cmnThreadLibDestroy();
}

