/*
 */
//#define	DLL_LIBRARY_EXPORT

#include <stdio.h>
#include <string.h>				// for getc()

#include "muxSvrWeb.h"

int     opterr = 1,             /* if error message should be printed */
        optind = 1,             /* index into parent argv vector */
        optopt,                 /* character checked for validity */
        optreset;               /* reset getopt */
//DLL_DECLARE
	char    *optarg=NULL;                /* argument associated with option */

#define BADCH   (int)'?'		/**/
#define BADARG  (int)':'
#define EMSG    ""


/*
 * getopt --
 *      Parse argc/argv argument vector.
 Usage : 
 		-a     --- argument without parameter
 		-a:    ---  argument with parameter
 		-a::   --- argument with optional parameter
 */
int getopt(int nargc, char * const *nargv,const char *ostr)
{
        static char *place = EMSG;/* option letter processing */
        char *oli;/* option letter list index */
        int ret;

	if (optreset || !*place)
	{/* update scanning pointer */
		optreset = 0;

		if (optind >= nargc || *(place = nargv[optind]) != '-')
		{
			place = EMSG;
			return (-1);
		}
		
		if (place[1] && *++place == '-')
		{/* found "--" */
			++optind;
			place = EMSG;
			return (-1);
		}
	}                                       /* option letter okay? */
		
	if ((optopt = (int)*place++) == (int)':' ||!(oli = strchr(ostr, optopt)))
	{
		/*
		* if the user didn't specify '-' as an option,
		* assume it means -1.
		*/
		if (optopt == (int)'-')
			return (-1);
		if (!*place)
			++optind;

		if (opterr && *ostr != ':' && optopt != BADCH)
			(void)fprintf(stderr, "%s: illegal option -- %c\n", nargv[0], optopt);
		return (BADCH);
	}

	if (*++oli != ':')
	{/* don't need argument */
		optarg = NULL;
		if (!*place)
			++optind;
	}
	else
	{/* need an argument */
		if (*place)/* no white space */
			optarg = place;
		else if (nargc <= ++optind)
		{/* no arg */
			place = EMSG;
			
			if (*ostr == ':')
				ret = BADARG;
			else
				ret = BADCH;
			
			if (opterr)
				(void)fprintf(stderr, "%s: option requires an argument -- %c\n", nargv[0], optopt);
			return (ret);
		}
		else/* white space */
			optarg = nargv[optind];
		
		place = EMSG;
		++optind;
	}

//	DEBUG_MSG("OptArg : '%s'\n", optarg);
        return (optopt);                        /* dump back option letter */
}


