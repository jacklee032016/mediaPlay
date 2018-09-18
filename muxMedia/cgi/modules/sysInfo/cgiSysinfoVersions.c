/*
* $Id: sysInfoVersions.c,v 1.1.1.1 2006/11/29 17:06:59 lizhijie Exp $
*/

#include "libCgi.h"
#include "libCgiSyscfg.h"

typedef struct
{
	char		description[64];
	char		verInfoFile[64];
}version_desc_t;

version_desc_t	 verFiles[] =
{
	{
		_("Platform Info"),
		"/etc/version.info"
	},
	{
		_("Shared Lib"),
		"/var/ver/shared.ver"
	},
	{
		CGI_STR_MUX_SERVER,
		"/var/ver/muxPlayer.ver"
	},
	{
		_("Web Server"),
		"/var/ver/whttpd.ver"
	},
	{
		_("Web Admin"),
		"/var/ver/webadmin.ver"
	}
};

static char *__read_verInfo(char *filename)
{
	int i;
	int linelen;
	FILE *fp;
	char line[PARSE_FILE_MAX_LENGTH];
	
	if( (fp=fopen(filename,"r"))==NULL)
		return strdup("");

	memset(line,0,sizeof(line));
	while (fgets(line,PARSE_FILE_MAX_LENGTH, fp))
	{
		linelen=strlen(line);
		for(i=linelen-1;i>0;i--)
		{
			if( (line[i]=='\t' ) || ( line[i]=='\n') || (line[i]=='\r') )
			line[i]=' ';
		}
		
	}

	return strdup(line);
}


/* Server IP Input box for both restore and backup*/
char *__sysinfo_version_button(SYS_INFO *info )
{
	char 	buf[8192*5];
	int 		length = 0;
	int		i;
	
	length += CGI_SPRINTF(buf,length, "<tr>\n\t<td><strong>%s<p></strong></TD></TR>\n", gettext("version of System Task") );

	length += sprintf(buf+length, "<TR>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n\t<TD align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\n</TR>\n", 
		gettext("Software Name"), gettext("Version") );

	for(i=0; i<sizeof(verFiles)/sizeof(verFiles[0]) ;i++)
	{

		length += sprintf(buf+length, "<TR>\n\t<TD align=\"center\">%s</TD>\n\t<TD align=\"center\">%s</TD>\n</TR>\n", 
			gettext(verFiles[i].description), __read_verInfo(verFiles[i].verInfoFile) );
	}

	return strdup(buf);
}


int cgiSysInfo_Versions(SYS_INFO *info)
{
	cgi_info_page(gettext("Software Version Info"), "", __sysinfo_version_button(info ) );
	return 0;	
}

