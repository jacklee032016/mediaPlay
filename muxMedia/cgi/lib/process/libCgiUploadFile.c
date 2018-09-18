/*
* $Author: lizhijie $
* $Revision: 1.5 $
*/

#include "libCgi.h"

#define	DEBUG_UPLOAD			1

#define	WEB_URL_UPLOAD_FIRMWARE		WEB_CGI_ROOT"sip/uploadFirmware.cgi"
#define	WEB_URL_UPDATE_FIRMWARE		WEB_CGI_ROOT"sip/updateFirmware.cgi"
#define	PBX_FIRMWARE_HOME				"/etc/pbx"

char *upload_status_list( web_type_t type)
{
	char buf[8192];

	return strdup(buf);
}

char *upload_add_buttons(web_type_t type)
{
	char buf[2048];
	int length = 0;

	if(type==WEB_TYPE_ADMIN)
	{
		length += sprintf(buf+length, "<form name=\"upfiles\" method=\"post\" enctype=\"multipart/form-data\" action=\"%s\">\r\n", WEB_URL_UPLOAD_ADMIN);
		length += sprintf(buf+length, "<TR><TD >%s</TD></TR>\r\n", gettext("Remote Update can be implemented by this function")  );
	}
	else if(type==WEB_TYPE_WWW)
	{
		length += sprintf(buf+length, "<form name=\"upfiles\" method=\"post\" enctype=\"multipart/form-data\" action=\"%s\">\r\n", WEB_URL_UPLOAD_WWW);
		length += sprintf(buf+length, "<TR><TD >%s</TD></TR>\r\n", gettext("Upload the Web pages package for your Internet WWW Server") );
	}
	else
	{
		length += sprintf(buf+length, "<form name=\"upfiles\" method=\"post\" enctype=\"multipart/form-data\" action=\"%s\">\r\n", WEB_URL_UPLOAD_FIRMWARE);
		length += sprintf(buf+length, "<TR><TD >%s</TD></TR>\r\n", gettext("Upload the PBX Firmware for update") );
	}
	
	length += sprintf(buf+length, "<input type=\"hidden\" name=\"type\" value=\"%s\">\r\n", (type==WEB_TYPE_ADMIN)?"admin":"www");
	length += sprintf(buf+length, "<input type=\"hidden\" name=\"cmd\" value=\"Save\">\r\n");

	length += sprintf(buf+length, "<TR><TD ><br></TD></TR>\r\n");
	length += sprintf(buf+length, "<TR><TD ><input name=\"uploadfiles\" onChange=\"checkIsNotNULL(uploadfiles,'%s')\" type=\"file\" value=\"\"></TD></TR>\r\n",
		gettext("File can not be NULL") );

	length += sprintf(buf+length, "<TR><TD >(%s)</TD></TR>\r\n",gettext("Note : Only file which size is below 10M can be upload here"));
	length += sprintf(buf+length, "<TR><TD ><br></TD></TR>\r\n");

	
	length += sprintf(buf+length, "<TR><TD >\r\n");
	length += sprintf(buf+length, cgi_submit_button( gettext(CGI_STR_UPLOAD) ) );
	length += sprintf(buf+length, "</TD>\r\n");

	length += sprintf(buf+length, "<TD>\r\n");
	if (type==WEB_TYPE_ADMIN)
		length += sprintf(buf+length, cgi_help_button(CGI_HELP_MSG_UPLOAD_ADMIN ) );
	else if (type==WEB_TYPE_WWW)
		length += sprintf(buf+length, cgi_help_button(CGI_HELP_MSG_UPLOAD_WWW ) );

	length += sprintf(buf+length, "</TD></TR></FORM>\r\n");

	return strdup(buf);
}

/*
  >0: ok;
  =-1:CONTENT_LENGTH is NULL;
  =-2:file is too big; great than MAXFILELENGTH
*/
int upload_parse_form_endcode(llist* entries, web_type_t type)
{
	long content_length;
	entrytype entry={NULL, NULL, NULL, NULL};
	node* window = NULL;
	FILE *uploadfile;
	char *uploadfname, *tempstr, *boundary;
	char *buffer = (char *)malloc(sizeof(char) * BUFSIZ + 1);
	char *prevbuf = (char *)malloc(sizeof(char) + BUFSIZ + 1);

	//    char cmd[256]="\0";
	int file_length=0;
	short isfile,done,start;
	int i,j;
	int bytesread,prevbytesread;
	int buffersize;
	int numentries = 0;

	char		statusMsg[1024];

	int m=0,n=0;
#ifdef WINDOWS
	setmode(fileno(stdin), O_BINARY);   /* define stdin as binary */
	_fmode = BINARY;                    /* default all file I/O as binary */
#endif

	if (CONTENT_LENGTH != NULL)
	{
		content_length = atol(CONTENT_LENGTH);
		file_length=content_length/1024/1024 +1 ;	

		if(file_length > MAX_UPLOAD_FILE_SIZE_10M)
			return CGI_UPLOAD_ERROR_TOO_BIG;
#if 0
		if(file_length>4)
		{
		 	sprintf(cmd,"mount -t tmpfs -o remount -o size=%dM tmpfs /tmp",file_length);
		 	system(cmd);
		}
#endif		
	}
	else
	{
		return CGI_UPLOAD_ERROR_NO_CONTENT;
	}

	/* get boundary */
	tempstr = cgi_new_string(CONTENT_TYPE);
	boundary = strstr(tempstr,"boundary=");
	boundary += (sizeof(char) * 9);

	/* create list */
	LIST_CREATE(entries);
	window = entries->head;
	{
		char target[128];
		char info[2048];
		int	printlen = 0;
		char	msg[128];

		if(type==WEB_TYPE_ADMIN )
		{
			sprintf(target, "%s", WEB_URL_UPLOAD_ADMIN );
		}
		else if(type == WEB_TYPE_WWW)
		{
			sprintf(target, "%s", WEB_URL_UPLOAD_WWW );
		}
		else
		{
			sprintf(target, "%s", WEB_URL_UPDATE_FIRMWARE);
		}
			
		printlen += sprintf(info + printlen, "%s,", gettext("Upload will consume some time on transfering and extracting files") );
		printlen += sprintf(info + printlen, "%s!", gettext("this is determined by the size of transfered file" ) );
		printlen += sprintf(info + printlen, "<br>%s", gettext(CGI_STR_PLEASE_WAIT ) );
		
		CGI_SPRINTF(msg, sizeof(msg), "%s.....", gettext(CGI_STR_UPLOAD) );
		cgi_refresh_page(10, target, msg, info );
		fflush(stdout);
	}

	/* ignore first boundary; this isn't so robust; improve it later */
	 cgi_getline(buffer, BUFSIZ);
	/* now start parsing */
	while ((bytesread=cgi_getline(buffer,BUFSIZ)) != 0)
	{
		start = 1;
		/* this assumes that buffer contains no binary characters.
		if the buffer contains the first valid header, then this
		is a safe assumption.  however, it should be improved for
		robustness sake. */
		buffer[bytesread] = '\0';
		tempstr = cgi_new_string(buffer);
		tempstr += (sizeof(char) * 38); /* 38 is header up to name */
		entry.name = tempstr;
		entry.value = (char *)malloc(sizeof(char) * BUFSIZ + 1);
		buffersize = BUFSIZ;
		strcpy(entry.value,"");

		while (*tempstr != '"')
			tempstr++;

		*tempstr = '\0';
		if (strstr(buffer,"filename=\"") != NULL)
		{
			isfile = 1;
			tempstr = cgi_new_string(buffer);
			tempstr = strstr(tempstr,"filename=\"");
			tempstr += (sizeof(char) * 10);

			if (strlen(tempstr) >= BUFSIZ)
				entry.value = (char *) realloc(entry.value, sizeof(char) *strlen(tempstr)+1);

			entry.value = tempstr;

			while (*tempstr != '"')
				tempstr++;

			*tempstr = '\0';

			if(strstr(entry.value,".tar")==NULL && strstr(entry.value,".rar")==NULL)
			{
				cgidebug("upload file is '%s'\n", entry.value );
				return CGI_UPLOAD_ERROR_FORMAT_NOT_SUPPORT;
			}
			/* Netscape's Windows browsers handle paths differently from its
			UNIX and Mac browsers.  It delivers a full path for the uploaded
			file (which it shouldn't do), and it uses backslashes rather than
			forward slashes.  No need to worry about Internet Explorer, since
			it doesn't support HTTP File Upload at all. */
			if (strstr(cgi_lower_case(HTTP_USER_AGENT),"win") != 0)
			{
				tempstr = strrchr(entry.value, '\\');
				if (tempstr)
				{
					tempstr++;
					entry.value = tempstr;
				}
			}

			window = list_insafter(entries,window,entry);
			numentries++;
			uploadfname = (char *)malloc(strlen(UPLOADPATH)+strlen(entry.value)+2);
#ifdef WINDOWS

			sprintf(uploadfname,"%s\\%s",UPLOADPATH,entry.value);
#else
			sprintf(uploadfname,"%s/%s",UPLOADPATH,entry.value);
#endif

			sprintf(statusMsg, "'%s' %s.....",entry.value, gettext("is saving"));
			CGI_INSTANCE_INFO(statusMsg, "#0000ff");

			if ( (uploadfile = fopen(uploadfname,"w")) == NULL)
			{
				/* null filename; for now, just don't save info.  later, save  to default file */
				isfile = 0;
			}
		}
		else
		{
			isfile = 0;
			cgidebug("upload file is '%s'\n", buffer );
		}

		/* ignore rest of headers and first blank line */
		while (cgi_getline(buffer, BUFSIZ) > 1)
		{
			/* DOS style blank line? */
			if ((buffer[0] == '\r') && (buffer[1] == '\n'))
				break;
		}

		if(strstr(buffer,"gz"))
			m=1;

		done = 0;
		j = 0;

		while (!done)
		{
			bytesread = cgi_getline(buffer,BUFSIZ);

			if((strstr(buffer,"application/octet-stream"))&&(m==1))
				return CGI_UPLOAD_ERROR_FILE_NOT_EXIST;

			buffer[bytesread] = '\0';

			if (bytesread && strstr(buffer,boundary) == NULL)
			{
				if (start)
				{
					i = 0;
					while (i < bytesread)
					{
						prevbuf[i] = buffer[i];
						i++;
					}
					prevbytesread = bytesread;
					start = 0;
				}
				else
				{
					/* flush buffer */
					i = 0;
					while (i < prevbytesread)
					{
						if (isfile)
							fputc(prevbuf[i],uploadfile);
						else
						{
							if (j > buffersize)
							{
								buffersize += BUFSIZ;
								entry.value = (char *) realloc(entry.value, sizeof(char)*buffersize+1);
							}
							entry.value[j] = prevbuf[i];
							j++;
						}
						i++;
					}

					/* buffer new input */
					i = 0;
					while (i < bytesread)
					{
						prevbuf[i] = buffer[i];
						i++;
					}
					prevbytesread = bytesread;
				}

			}
			else
			{
				done = 1;
				/* flush buffer except last two characters */
				i = 0;
				while (i < prevbytesread - 2)
				{
					if (isfile)
						fputc(prevbuf[i],uploadfile);
					else
					{
						if (j > buffersize)
						{
							buffersize += BUFSIZ;
							entry.value = (char *) realloc(entry.value, sizeof(char)*buffersize+1);
						}
						entry.value[j] = prevbuf[i];
						j++;
					}
					i++;
				}
			}

			n++;
		}

		if (isfile)
		{
//			cgidebug("savefile is %s.\n", uploadfname);
			char cmdline[150];
			char rootpath[128];
			fclose(uploadfile);

			
			if(n<=2 && m!=1)	
				return CGI_UPLOAD_ERROR_FILE_NOT_EXIST;

			if(type== WEB_TYPE_PBX_FIRMWARE )
				sprintf(rootpath, PBX_FIRMWARE_HOME );
			else if(type== WEB_TYPE_WWW )
				sprintf(rootpath, ROOT_PATH_WWW );
			else
				sprintf(rootpath, "/");
#if DEBUG_UPLOAD
			cgidebug("entry: %s, %s\n",entry.name, entry.value);
#endif
			if(strstr(entry.value,".tar.gz"))
			{
#if DEBUG_UPLOAD
				sprintf(cmdline,"tar zxvf %s/%s -C %s > /tmp/upload.txt ",UPLOADDIR,entry.value, rootpath);
#else
				sprintf(cmdline,"tar zxf %s/%s -C %s",UPLOADDIR,entry.value, rootpath);
#endif
			}
			else if(strstr(entry.value,".tar"))
			{
#if DEBUG_UPLOAD
				sprintf(cmdline,"tar xvf %s/%s -C %s > /tmp/upload.txt ",UPLOADDIR,entry.value, rootpath);
#else
				sprintf(cmdline,"tar xf %s/%s -C %s",UPLOADDIR,entry.value, rootpath);
#endif
			}
			else if(strstr(entry.value,".rar"))
			{/* x: extract; -y : assume YES for all query;  directory_to_extract */
#if DEBUG_UPLOAD
				sprintf(cmdline,"unrar x -y %s/%s %s > /tmp/upload.txt  ", UPLOADDIR,entry.value, rootpath );
#else
				sprintf(cmdline,"unrar x -y %s/%s %s > /dev/null ", UPLOADDIR,entry.value, rootpath );
#endif
			}
			
			sprintf(statusMsg, "'%s' %s.....",entry.value, gettext("is extracting"));
			CGI_INSTANCE_INFO(statusMsg, "#0000ff");
			
//			cgidebug("command for extract file is '%s'\n", cmdline );
			system(cmdline);
			
			sprintf(cmdline,"rm -f %s/%s",UPLOADDIR,entry.value);
//			cgidebug("command for remove compressed file is '%s'\n", cmdline );

			sprintf(statusMsg, "'%s' %s.....",entry.value, gettext("is removing"));
			CGI_INSTANCE_INFO(statusMsg, "#0000ff");
			system(cmdline);
		}
		else
		{
			entry.value[j] = '\0';
			window = list_insafter(entries,window,entry);
			numentries++;
			j = 0;
		}
	}

	return numentries;
}

/************************
 * return: <0 error in QUERY_STRING or POST string
 *            =0 no QUERY_STRING or POST string
 *            >0 normal
 ************************/
int upload_read_cgi_input(llist* entries, web_type_t type)
{
	int status;

    /* check for form upload.  this needs to be first, because the
       standard way of checking for POST data is inadequate.  If you
       are uploading a 100 MB file, you are unlikely to have a buffer
       in memory large enough to store the raw data for parsing.
       Instead, parse_form_encoded parses stdin directly.

       In the future, I may modify parse_CGI_encoded so that it also
       parses POST directly from stdin.  I'm undecided on this issue,
       because doing so will make parse_CGI_encoded less general. */
	if ((CONTENT_TYPE != NULL) &&	(strstr(CONTENT_TYPE,"multipart/form-data") != NULL) )
	{
		char	msg[256];
		int len = 0;

		len += CGI_SPRINTF(msg, len, "%s ", gettext(CGI_STR_UPLOAD) );
		status = upload_parse_form_endcode( entries , type );
		switch(status)
		{
			case CGI_UPLOAD_ERROR_NO_CONTENT:
				len += CGI_SPRINTF(msg, len, "%s", gettext(CGI_STR_FAIL) );
				cgi_error_page(NULL, msg, gettext("File Length is not validate"));// gettext("CONTENT_LENGTH is NULL!"));
				return -1;
			case CGI_UPLOAD_ERROR_FORMAT_NOT_SUPPORT:
				len += CGI_SPRINTF(msg, len, "%s", gettext(CGI_STR_FAIL) );
				cgi_error_page(NULL, msg, gettext("File has a wrong format, file .tar, .tar.gz or .rar expected"));
				return -1;
			case CGI_UPLOAD_ERROR_TOO_BIG:
				len += CGI_SPRINTF(msg, len, "%s", gettext(CGI_STR_FAIL) );
				cgi_error_page(NULL, msg, gettext("File is too long,max long 10M"));
				return -1;
			case CGI_UPLOAD_ERROR_FILE_NOT_EXIST:
				len += CGI_SPRINTF(msg, len, "%s", gettext(CGI_STR_FAIL) );
				cgi_error_page(NULL, msg, gettext("File is not exist"));
				return -1;
			default:
				len += CGI_SPRINTF(msg, len, "%s", gettext("Success") );
				cgi_error_page(NULL, msg, gettext("File upload ok!"));
				return 0;
		}
		
	}

	return 1;

//	return cgi_error_page(NULL, gettext("Upload Failed"), gettext("Web Browser not support file load"));
#if 0      	

    /* get the input */
	if (REQUEST_METHOD == NULL)
		input = get_DEBUG();
	else if (!strcmp(REQUEST_METHOD,"POST"))
		input = get_POST();
	else if (!strcmp(REQUEST_METHOD,"GET"))
		input = get_GET();
	else
	{ /* error: invalid request method */
		fprintf(stderr,"caught by cgihtml: REQUEST_METHOD '%s' invalid\n", REQUEST_METHOD);
	}
	
    /* parse the input */
	if (input == NULL)
		return 0;
	status = parse_CGI_encoded(entries,input);

	free(input);
	return status;
#endif
}


