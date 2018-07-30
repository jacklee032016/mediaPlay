
#include "libCgi.h"

static int __getline(char s[], int lim)
{
	int c = 0, i=0, num;

	memset(s, 0, lim);
#ifdef	WIN32
	for (i=0; (i<lim) && ((c=getc(stdin))!=EOF) && (c!='\n'); i++)
#else		
	for (i=0; (i<lim) && ((c=getchar())!=EOF) && (c!='\n'); i++)
#endif		
	{
		s[i] = c;
	}
	
	if (c == '\n')
	{
		s[i] = c;
	}
	if ((i==0) && (c!='\n'))
		num = 0;
	else if (i == lim)
		num = i;
	else
		num = i+1;

	return num;
}


formvars *libcgiUpload(const char *boundary, formvars **start, formvars **last)
{
	FILE *uploadfile = NULL;
	char uploadFileName[256];
	char	 *tempstr= NULL, *t2= NULL;
	int		bytesread;
	char		buffer[BUFSIZ + 1];
	int i=1;

CGI_TRACE()	;
	while((bytesread = __getline(buffer, BUFSIZ)) )
	{
		libcgiLog("No.%d line:%s\n",i++, buffer);
		if (strstr(buffer, "filename=\"") != NULL)
		{
			tempstr = strstr(buffer,"filename=\"");
			tempstr += (sizeof(char) * 10);
			t2 = tempstr;
			
			while (*tempstr != '"')
				tempstr++;

			*tempstr = '\0';
			SNPRINTF(uploadFileName, sizeof(uploadFileName), "others\\%s", t2);

			if ( (uploadfile = fopen(uploadFileName,"w")) == NULL)
			{
				libcgiLog("open '%s' failed\n", uploadFileName);
			}
			
			libcgiLog("File name is '%s'\n", uploadFileName);

			break;
		}
	}

	if(uploadfile == NULL)
	{
		libcgi_error(E_WARNING, "No file name is found\n");
		return NULL;
	}

	
	/* ignore these 2 lines */
	bytesread = __getline(buffer, BUFSIZ);	/* sub content type */
	if(!strstr(buffer, "Content-Type: application/octet-stream"))
	{
		libcgi_error( E_WARNING, "Data is not validate format, ContentType:\"%s\"", buffer);
		fclose(uploadfile);
		return NULL;
	}	
	bytesread = __getline(buffer, BUFSIZ);	/* blank line */

	{
		formvars *data;
		data = (formvars *)malloc(sizeof(formvars));
		if (!data)
			libcgi_error(E_MEMORY, "%s, line %s", __FILE__, __LINE__);
			
		memset(data, 0, sizeof(formvars));
		
		data->name = (char *)malloc(256);
		if (data->name == NULL)
			libcgi_error(E_MEMORY, "%s, line %s", __FILE__, __LINE__);
		memset(data->name, 0, 256);
		strncpy(data->name, "filename", 8);

		data->value = (char *)malloc(256);
		if (data->value == NULL)
			libcgi_error(E_MEMORY, "%s, line %s", __FILE__, __LINE__);

		memset(data->value, 0, 256);
		strncpy(data->value, uploadFileName, strlen(uploadFileName));
		
		libcgiLog("CGI UPLOAD data %s=%s\n", data->name, data->value);
		slist_add(data, start, last);
	}
	
	while ((bytesread = __getline(buffer, BUFSIZ)) != 0)
	{
		i = 0;
		
		if ( strstr(buffer,boundary) == NULL)
		{

			while (i < bytesread )
			{
				fputc(buffer[i], uploadfile);
				i++;
			}
		}
		else
		{
			break;
#if 0		
			while (i < bytesread - 2)
			{
				fputc(buffer[i], uploadfile);
				i++;
			}
#endif
		}
	}
	
	fclose(uploadfile);

	return *start;
}


