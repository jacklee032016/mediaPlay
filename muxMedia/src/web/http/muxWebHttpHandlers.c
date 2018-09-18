/*
 *
*/

#define _GNU_SOURCE
#include <crypt.h>

#include "muxWeb.h"

struct spwd *getspnam(char *name);


#define PASSWD_LENGTH			32


static char codelist[64]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int _strlocation(char s[] ,char f)
{
	int i=-1,j;
	int len;
 
	len=strlen(s);
	for (j=0;j<len;j++)
		if (s[j]==f)
		{
			i=j;
			break;
		} 
	return i;
}   

static char *_base64decode(char *dest,int length)
{
	int count;
	int i=0,j;
	char *source,*dest1;
	int find;
   
	if ( length%4 !=0)
	{
		ECPWS_LOG_INFO("The String need to decode is not correct,  length is %d\n",  length);
		return NULL;
	}
	count=length/4;
  
	source=(char *)malloc(count*3+1);

  	memset(source, 0, count*3+1);
	source[count/4*3+1]=0;
  
	dest1 =(char *)malloc(length);
  
	for(i=0;i<count;i++)
	{
		for (j=0;j<4;j++)
		{
			find = _strlocation(codelist,dest[i*4+j]);
			if(find==-1) 
				dest1[i*4+j]=100;
			else 
				dest1[i*4+j]= find;   
		} 
		source[i*3]  =( (dest1[i*4]<<2) | ((dest1[i*4+1]&48)>>4) );
		if (dest1[i*4+2]!=100)
		{   
			source[i*3+1]=( ((dest1[i*4+1]&15)<<4) | ((dest1[i*4+2]&63)>>2) );
			if (dest1[i*4+3]!=100)
				source[i*3+2]=( ((dest1[i*4+2]&3 )<<6) | ((dest1[i*4+3]&63)));
		}
	}
	free(dest1);
	return source;
}

#ifdef WIN32
struct passwd *getpwnam(const char *name)
{
	struct passwd *_passwd = NULL;

	if(name == NULL || name[0]=='\0' )
		return NULL;
	_passwd = malloc(sizeof(struct passwd));
	if(_passwd == NULL)
		return NULL;

	memset(_passwd, 0, sizeof(struct passwd));
	
	_passwd->pw_name = STRDUP(name);

	return _passwd;
}

static int _system_authen(char *user, char *pass)
{
	if( (!STRCASECMP(cfg->server_uid, user)) && (!STRCASECMP(user, pass)))
		return WWW_AUTHEN_SUCCESS;
	return WWW_AUTHEN_FAIL;
}

#else
struct passwd *getpwnam(const char *name);

/* use system account info to authenticate user. return 1 : success; 0: failed*/
static int _system_authen(char *user, char *pass)
{
	struct passwd  *pw;

	ECPWS_DEBUG_INFO("authening user %s in /etc/passwd %s\n" , user, __FUNCTION__ );

/*	pw = my_getpwnam ( user );*/
	/* this pointer is a static variable which can not be free */
	pw = getpwnam ( user ); 
	if(pw==NULL)
	{
		ECPWS_LOG_INFO("no valid user %s in /etc/passwd\n" , user );
		return WWW_AUTHEN_EXCEP_EXPLORE;
	}
	
	if ( ( strlen ( pw->pw_passwd ) == 0 ) &&  (strlen(pass) == 0 ) )
	{
		ECPWS_LOG_INFO( "password of %s user is not used\n" ,user);
		return WWW_AUTHEN_SUCCESS;
	}

	ECPWS_DEBUG_INFO("password for %s user is %s, len is %d; pass user input is %s,len is %d\n", user , pw->pw_passwd, strlen(pw->pw_passwd), pass, strlen(pass) );

	if (( strcmp ( pw->pw_passwd, "x" ) == 0 ) || ( strcmp ( pw->pw_passwd, "*" ) == 0 )) 
	{
		char  *encrypted, *correct;
		struct spwd *sp = get_spnam ( pw->pw_name );

		ECPWS_DEBUG_INFO( "password of %s user is used\n" , user);

		if ( !sp )
		{
			ECPWS_LOG_INFO("no valid shadow password for %s" , pw->pw_name );
			return WWW_AUTHEN_FAIL;
		}
		correct = sp->sp_pwdp;
		ECPWS_DEBUG_INFO( "correct='%s'  passwd='%s'\n", correct, pass); 

		encrypted = crypt ( pass,  correct );

		ECPWS_DEBUG_INFO("encrypted='%s'\n", encrypted); 

//	memset ( unencrypted, 0, xstrlen ( unencrypted ));
		if ( strcmp ( encrypted, correct ) == 0 ) 
		{
			return WWW_AUTHEN_SUCCESS;
		}
	
	}

	return WWW_AUTHEN_FAIL;
}
#endif

/* Only WWW Base Authentication is supportted now 
 * return 1: authen passwd, 0 : authen failed
 */
int www_base_authen(WEB_CLIENT_CONN *conn, char *buf)
{
	char *find;
	char *end;
	char *credent;
	char userID[PASSWD_LENGTH];
	char passwd[PASSWD_LENGTH];
	
	find=strstr(buf, "Basic");
	if(!find)
	{
		ECPWS_LOG_INFO( "Not BASIC authention: \n", buf);
		return WWW_AUTHEN_EXCEP_HEADER;
	}
	find+=6; 			

	credent = _base64decode(find, strlen(find) );	
	if(!credent)
	{
		ECPWS_LOG_INFO( "Not validate Base64 auth data: \n", find);
		return WWW_AUTHEN_EXCEP_HEADER;
	}

	ECPWS_DEBUG_INFO("credent='%s'(%d)\n", credent, strlen(find));

	memset(userID, 0, 32);
	memset(passwd, 0, 32); 			
	end = strchr(credent,':'); 
	if(!end)
	{
		free(credent);
		ECPWS_LOG_INFO( "No validate user and passwors data: \n", credent);
		return WWW_AUTHEN_EXCEP_HEADER;
	}
	strncpy(userID, credent, end-credent); 
	strncpy(passwd, end+1, strlen(credent)-(end+1-credent) ); 

	free(credent);

	ECPWS_DEBUG_INFO( "user='%s' passwd='%s' \n", userID, passwd); 

	return _system_authen(userID,  passwd);

}



int ecpwsHttpAuthen(WEB_CLIENT_CONN *conn)
{
	if(!STRCASECMP(conn->webSvr->cfg.baseAuthen, "No") ||conn->authen_status == REQUEST_AUTHENED )
		return 1;

	WEB_CONN_RES_ERR(conn, WEB_RES_UNAUTHORIZED);
}


/*
 * Description: Reads body from a request socket for POST CGI
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: request done, close it down
 *   1: more to do, leave on ready list
 As quoted from RFC1945:
 A valid Content-Length is required on all HTTP/1.0 POST requests. An
 HTTP/1.0 server should respond with a 400 (bad request) message if it
 cannot determine the length of the request message's content.
 */
WEB_CLIENT_STATUS ecpwsHttpHandlerReadPost(WEB_CLIENT_CONN * conn)
{
	int bytes_read;
	unsigned int bytes_to_read, bytes_free;

	bytes_free = WEB_BUFFER_SIZE - (conn->header_end - conn->header_line);
	bytes_to_read = conn->filesize - conn->filepos;

	/*
	if(!www_base_authen(conn->header_line))
	{
		conn->response_status = WEB_RES_UNAUTHORIZED;//(conn, "SIP Gateway");
		return 0;
	}
	*/

	if (bytes_to_read > bytes_free)
		bytes_to_read = bytes_free;

#if WEB_OPTIONS_DEBUG_DATA
	ECPWS_LOG_INFO("%d position; used offset %d\n", conn->client_stream_pos, conn->header_line-conn->client_stream);
#endif
	if (bytes_to_read <= 0 || conn->client_stream_pos -(conn->header_line-conn->client_stream)>= (int)conn->filesize )
	{
		return WEB_CLIENT_S_BODY_WRITE; /* go write it */
	}

	bytes_read = svcWebConnRead(conn, conn->header_end, bytes_to_read);
	if (bytes_read <= 0 )
	{/* peer close this socket */
		return ecpwsHandlerTimeout( conn);
	}

	
	conn->header_end[bytes_read] = '\0';

#if WEB_OPTIONS_DEBUG_DATA
	ECPWS_DEBUG_INFO("- read %d bytes.\n\"%s\"\n", bytes_read, conn->header_end);
#endif

	conn->header_end += bytes_read;
	return WEB_CLIENT_S_BODY_WRITE;
}

/*
 * Writes a chunk of data from POST method to a local file
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: EOF or error, close it down
 *   1: successful write, recycle in ready queue
 */
WEB_CLIENT_STATUS ecpwsHttpHandlerWritePost(WEB_CLIENT_CONN * conn)
{
	int bytes_written;
	unsigned int bytes_to_write = conn->header_end - conn->header_line;

	if (conn->filepos + bytes_to_write > conn->filesize)
		bytes_to_write = conn->filesize - conn->filepos;

	if (bytes_to_write == 0)
	{  /* nothing left in buffer to write */
		conn->header_line = conn->header_end = conn->buffer;
		if (conn->filepos >= conn->filesize)
			return ecpwsCgiCreate(conn);
		
		/* if here, we can safely assume that there is more to read */
		return WEB_CLIENT_S_BODY_READ;
	}
	
	bytes_written = WRITE(conn->post_data_fd, conn->header_line, bytes_to_write);
	if (bytes_written == -1)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return WEB_CLIENT_S_CONTINUE;          /* request blocked at the pipe level, but keep going */
		else if (errno == EINTR)
			return WEB_CLIENT_S_CONTINUE;
		else if (errno == ENOSPC)
		{
			/* No test was originally done in this case, which might  */
			/* lead to a "no space left on device" error.             */
			ECPWS_LOG_REQ_ERR(conn, "write body1"); /* OK to disable if your logs get too big */
			return WEB_CLIENT_S_ERROR;
		}
		else
		{
			ECPWS_LOG_REQ_ERR(conn, "write body2"); /* OK to disable if your logs get too big */
			return WEB_CLIENT_S_ERROR;
		}
	}
	
#if WEB_OPTIONS_DEBUG_DATA
	ECPWS_LOG_INFO("CONN%d- wrote %d bytes (bytes_to_write:%d; content_length:%ld):\"%s\"\n", 
		conn->index, bytes_written,bytes_to_write, conn->filesize, conn->header_line);
#endif

	conn->filepos += bytes_written;
	conn->header_line += bytes_written;

	if(conn->filepos == conn->filesize)
		return ecpwsCgiCreate(conn);

	if(bytes_written == bytes_to_write)
	{
		return WEB_CLIENT_S_BODY_READ;
	}
	
	return WEB_CLIENT_S_BODY_WRITE;/* more to do */
}


/* Writes a chunk of data to the socket.
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: EOF or error, close it down
 *   1: successful write, recycle in ready queue
 */
WEB_CLIENT_STATUS ecpwsHttpHandlerWriteHtml(WEB_CLIENT_CONN * conn)
{
	int bytes_written;
	volatile unsigned int bytes_to_write;

	if( ecpwsHttpPrepareStaticResponse( conn) == WEB_CLIENT_S_ERROR)
		return WEB_CLIENT_S_ERROR;

	if (conn->method == WEB_M_HEAD)
	{
		return complete_response(conn);
	}

	bytes_to_write = (conn->ranges->stop - conn->ranges->start) + 1;

#if 0
	if (bytes_to_write > system_bufsize)
		bytes_to_write = system_bufsize;
#endif

#if WITH_JUMP
	if (setjmp(env) == 0)
#else
	if(1)
#endif
	{
#ifndef	WIN32	
		handle_sigbus = 1;
#endif

#if WEB_OPTIONS_DEBUG_DATA
		ECPWS_DEBUG_INFO("%d bytes need to write:\n", bytes_to_write);//, conn->data_mem + conn->ranges->start);
#endif

		bytes_written = svcWebConnWrite(conn, conn->data_mem + conn->ranges->start, bytes_to_write);
#ifndef	WIN32	
		handle_sigbus = 0;
#endif
		/* OK, SIGBUS **after** this point is very bad! */
	}
	else
	{
		/* sigbus! */
		conn->status = WEB_CLIENT_S_ERROR;
		ECPWS_LOG_REQ_NOERR(conn, "%s Got SIGBUS in write(2)!\n", get_commonlog_time(conn->webSvr));
		/* sending an error here is inappropriate
		* if we are here, the file is mmapped, and thus,
		* a content-length has been sent. If we send fewer bytes
		* the client knows there has been a problem.
		* We run the risk of accidentally sending the right number
		* of bytes (or a few too many) and the client
		* won't be the wiser.
		*/
		return 0;
	}

	if (bytes_written < 0)
	{
		ECPWS_LOG_REQ_ERR(conn, "Write ERROR");
		WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
	}

#if WEB_OPTIONS_DEBUG_DATA
//	ECPWS_DEBUG_INFO("Write: %d bytes : \"%s\"\n", bytes_written, conn->data_mem + conn->ranges->start);
	ECPWS_DEBUG_INFO("Write: %d bytes\n", bytes_written );
#endif

	conn->filepos += bytes_written;
	conn->bytes_written += bytes_written;
	conn->ranges->start += bytes_written;

	if ((conn->ranges->stop + 1 - conn->ranges->start) == 0)
	{
		complete_response(conn);
	}

	if(bytes_to_write == bytes_written)
	{
		freeOneRequest(conn->webSvr, conn);
		return WEB_CLIENT_S_READ_HEADER;
	}
	
	return WEB_CLIENT_S_CONTINUE;
}

WEB_CLIENT_STATUS ecpwsHandlerTimeout(WEB_CLIENT_CONN *conn)
{

	ECPWS_CLEAR_READ(conn->webSvr, conn->socket);
	ECPWS_CLEAR_WRITE(conn->webSvr, conn->socket);
	CloseSocket(conn->socket);
	conn->socket = 0;
	
	conn->webSvr->totalConnections--;

	QUEUE_DE_READY(conn->webSvr, conn);
	QUEUE_EN_FREE(conn->webSvr, conn);

	return WEB_CLIENT_S_READ_HEADER;
}

