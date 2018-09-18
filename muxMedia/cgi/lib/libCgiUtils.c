/*
* $Author: lizhijie $
*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

#include "libCgi.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

llist entries;


int cgi_read_local_config(char *filename,void(*parseLocalConfig)(char *, void *data), void *data)
{
	FILE *cfgfp;
	char line[PARSE_FILE_MAX_LENGTH];
    //    char value[SIP_VALUE_MAX_LENGTH];
//	int keyCount, curParsePos=0;
	int i,lineLen;

	if((cfgfp=fopen(filename,"r"))==NULL)
	{
		fprintf(stderr, "Open file '%s' failure :%s!\n", filename, strerror(errno) );
		return EXIT_FAILURE;
	}

	if(entries.head!=NULL)
		list_clear(&entries);

	while (fgets(line,PARSE_FILE_MAX_LENGTH,cfgfp))
	{
		lineLen = strlen(line);
		if(lineLen<1)
			continue;

		if(line[0]=='#'||line[0]=='\n'||line[0]=='\r')
			continue;
		
		/*tram line end, delete blank and '\t' '\n' '\r' char*/
		for(i=lineLen-1;i>0;i--)
		{
			if(line[i]==' '|| line[i]=='\t' || line[i]=='\n' || line[i]=='\r')
				line[i]='\0';
			else
				break;
		}
		
		if(parseLocalConfig)
			(*parseLocalConfig)(line, data);
        //        fprintf(stderr,"%s too few paramters!\n",key);
	}
	fclose(cfgfp);

	return EXIT_SUCCESS;
}


int cgi_domain_name_write(char *domainname)
{
	FILE *fp;
	
	if(( fp=fopen(CGI_DOMAIN_FILE,"w"))==NULL)
	{
		fprintf(stderr, "Open file %s failure!", CGI_DOMAIN_FILE );
		return -1;
	}
	
	fprintf(fp, "#domain for AS600\n\n");
	fprintf(fp, "DOMAIN_NAME=%s\n" , domainname);

	fclose(fp);

	return 0;
}

char *cgi_domain_name_read()
{
	llist list = {NULL};

	if (!readconfig(CGI_DOMAIN_FILE,ITEM_EQU, "DOMAIN_NAME", &list) )
	{
		return strdup(list.head->entry.value);
	}

	cgidebug( "No DomainName is read in file of %s!" , CGI_DOMAIN_FILE );
	return "";

}

int cgi_host_name_write(char *hostname)
{
	FILE *fp;
	
	if(( fp=fopen(CGI_HOST_NAME_FILE,"w"))==NULL)
	{
		cgidebug("Open file %s failure!", CGI_DOMAIN_FILE );
		return -1;
	}
	
	fprintf(fp, "#hostname for AS600\n\n");
	fprintf(fp, "HOST_NAME=%s\n" , hostname);

	fclose(fp);

	return 0;
}

char *cgi_host_name_read()
{
	llist list ={NULL};

	if (!readconfig(CGI_HOST_NAME_FILE,ITEM_EQU, "HOST_NAME", &list) )
	{
		cgidebug( "No DomainName isof %s!" , CGI_DOMAIN_FILE );
		cgidebug( "No DomainName is read in file of %s!" , list.head->entry.value );
		return strdup(list.head->entry.value);
	}

	cgidebug( "No DomainName is read in file of %s!" , CGI_DOMAIN_FILE );
	return strdup("SONiX");
}

FILE		*localeFp = NULL;

void _close_locale_fp(void )
{
	fclose(localeFp);
	localeFp = NULL;
}

char *gettext(char *text)
{
	char fname[128];
//	FILE *fp;
	char line[LINE_MAX_LENGTH];
	char *retptr,*p,*q;
	char	*localeName= NULL;

	if(!HTTP_ACCEPT_LANGUAGE || strlen(HTTP_ACCEPT_LANGUAGE)==0)
	{
		if( !LOCALE|| strlen(LOCALE)==0 )
		{/* locale is 'C' or null */
			return text;
		}
		else
			localeName = LOCALE;
	}
	else
	{
		localeName = HTTP_ACCEPT_LANGUAGE;
	}

//	cgidebug("LOCALE NAME = %s\n", localeName );
//	cgidebug("HTTP_ACCEPT_LANGUAGE = %s\n", HTTP_ACCEPT_LANGUAGE );
	
	if(localeFp== NULL)
	{
		sprintf(fname,"%s/locale/%s_%s.po",ROOT_PATH_ADMIN,PACKAGE, localeName );
		localeFp = fopen(fname, "r" );
		if(!localeFp )
		{
//			cgidebug("gettext : '%s', because of '%s'\n" ,fname, strerror(errno) );
			return text;
//			exit(-1);
		}
		atexit( _close_locale_fp );
	}

	rewind(localeFp);
	while((retptr=fgets(line,LINE_MAX_LENGTH, localeFp) )!=NULL)
	{
		if(strlen(line)<1)
			continue;
		if(line[0]=='#'||line[0]=='\n'||line[0]=='\r')
			continue;
		if(!strstr(line,"msgid"))
			continue;
		p=strchr(line,'\"')+1;
		q=strrchr(line,'\"');
		if(!p||!q)
		{
			printf("locale file: double quotation expected\n");
			exit(-1);
		}
		*q=0;
		if(!strcmp(p,text))
		{
			retptr=fgets(line,LINE_MAX_LENGTH, localeFp );
			break;
		}
	}
	
//	fclose(fp);
	if(!retptr)
	{
//		cgidebug("%s is not found\n", text);
//		return TEXT_NOT_FOUND;
		return strdup(text);
	}

	if(!strstr(line,"msgstr"))
	{
		printf("locale file: no msgstr of text(%s) found\n",text);
		exit(-1);
	}
	p=strchr(line,'\"');
	q=strrchr(line,'\"');
	if(!p||!q)
	{
		printf("locale file: double quotation expected\n");
		exit(-1);
	}
	p++;
	*q=0;
	if(strlen(p))
		return strdup(p);
	else
	{
		return strdup(text);
	}

}

char *cgi_new_string(char *str)
{
	char *tempstr = NULL;
	int length = ITEM_LENGTH +1;
	
//	if( str &&  (strlen(str)>ITEM_LENGTH) )
//	if( str &&  (strlen(str) < ITEM_LENGTH) )
//		length = strlen(str);
	
	tempstr = (char *)malloc(sizeof(char) * length );
	memset(tempstr, 0 ,length);

	if (tempstr != NULL )
	{
		if( str)
		{
			strncpy(tempstr,str, (strlen(str)>ITEM_LENGTH)?ITEM_LENGTH:strlen(str) );
		}
		else
			memset(tempstr, 0, length);
	}	
	
	return tempstr;
}

#if WITH_MISC_HTML
char *cgi_sub_string(char *str, int offset, int len)
{
	int slen, start, i;
	char *nstr;

	if (str == NULL)
		return NULL;
	else
		slen = strlen(str);

	nstr = (char *)malloc(sizeof(char) * slen + 1);
	if (offset >= 0)
		start = offset;
	else
		start = slen + offset - 1;

	if ( (start < 0) || (start > slen) ) /* invalid offset */
		return NULL;

	for (i = start; i < start+len; i++)
		nstr[i - start] = str[i];

	nstr[len] = '\0';
	return nstr;
}
#endif

char *cgi_replace_ltgt(char *str)
{
	unsigned int i,j = 0;
	char *newstring;

	if (str == NULL)
		return NULL;
	newstring = (char *)malloc(sizeof(char) * (strlen(str) * 4 + 1));
	for (i = 0; i < strlen(str); i++) 
	{
		if (str[i] == '<') 
		{
			newstring[j] = '&';
			newstring[j+1] = 'l';
			newstring[j+2] = 't';
			newstring[j+3] = ';';
			j += 3;
		}
		else if (str[i] == '>') 
		{
			newstring[j] = '&';
			newstring[j+1] = 'g';
			newstring[j+2] = 't';
			newstring[j+3] = ';';
		j += 3;
	}
	else
		newstring[j] = str[i];

	j++;
	}
	newstring[j] = '\0';

	return newstring;
}

char *cgi_lower_case(char *buffer)
{
	char *tempstr = buffer;

	while (*buffer != '\0') 
	{
		if (isupper(*buffer))
			*buffer = tolower(*buffer);
		buffer++;
	}
	return tempstr;
}

	

