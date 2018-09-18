/*
* Media Server Utilities
* $Author$
* $Date$
* Implement http authentication for media server.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libMsUtils.h"

#define ZEROARRAY(array,size)		memset(array,'\0',size)

int isalnum(int);

static char base64DecodeTable[256];

static void initBase64DecodeTable() 
{
	int i;
	for (i = 0; i < 256; ++i) base64DecodeTable[i] = (char)0x80;
	for (i = 'A'; i <= 'Z'; ++i) base64DecodeTable[i] = 0 + (i - 'A');
	for (i = 'a'; i <= 'z'; ++i) base64DecodeTable[i] = 26 + (i - 'a');
	for (i = '0'; i <= '9'; ++i) base64DecodeTable[i] = 52 + (i - '0');
	base64DecodeTable[(unsigned char)'+'] = 62;
	base64DecodeTable[(unsigned char)'/'] = 63;
	base64DecodeTable[(unsigned char)'='] = 0;
}

static int base64Decode(const char *in, char *out, int iLen) 
{ 
	initBase64DecodeTable(); 

	const char *p = in;
	char *q = out;
	int i = 0, j = 0, k = 0;
	const int jMax = iLen -3;

	for(j = 0; j < jMax; j += 4) 
	{ 
		char inTmp[4], outTmp[4];
		for (i = 0; i < 4; ++i) 
		{
			inTmp[i] = p[i+j];
			outTmp[i] = base64DecodeTable[(int)inTmp[i]];
			if ((outTmp[i]&0x80) != 0) 
				outTmp[i] = 0; 
		}
		q[k++] = (outTmp[0]<<2) | (outTmp[1]>>4);
		q[k++] = (outTmp[1]<<4) | (outTmp[2]>>2);
		q[k++] = (outTmp[2]<<6) | outTmp[3];
	}
	return 1;
} 

static int parse_authorized_header(const char *buffer, char *authen_info)
{
	const char *p = buffer;
	char *auth = authen_info;
	while(1)
	{
		if(*p == '\0') 
		{
			return 0; /* not found */
		}
		if(!strncasecmp(p,"Authorization: Basic ", 21))
		{
	    		break; /* found */
		}
		p++;
    	}
	p+=21;

	while(*p == ' ')
		p++;
	while(*p != '\0' && *p != '\r' && *p != '\n')
	{
		*auth = *p;
		auth++;
		p++;
	}
	return 1;
}

static int getUserPwd(const char *in, char *username, char *password)
{
	const char *str = in;
	char *user = username;
	char *pass = password;
	while(*str != '\0')
	{
		while(*str != ':')
		{
			*user = *str;
			user++;
			str++;
		}
		*user = '\0';
		while(*str == ':')
			str++;
		while(isalnum(*str))
		{
			*pass = *str;
			pass++;
			str++;
		}
		*pass = '\0';
	}
	return 1;
}

static int find_correspond_pwd(char *username, char *password)
{
    FILE* fp = NULL;
    char buf[128];
    char pass[32];
    char user[32];
    char* p = NULL;
    char* q = NULL;
    int success = 0;

    ZEROARRAY(buf, 128);
    ZEROARRAY(pass, 32);
    ZEROARRAY(user, 32);

    fp = fopen(MS_AUTH_PASSWDFILE, "r");

    if(fp == NULL){
	return -1;
    }

    while(1){

	if(fgets(buf, 128, fp) == NULL){
	    break;
	}

	q = buf;
	if(*q == '#'){ //comment
	    continue;
	}

	p = user;
	while(*q != ' ' && *q != '\t'){
	    *p = *q;
	    p++;
	    q++;
	}
	*p='\0';

	if(!strcmp(user, username)){

	    p = pass;

	    while(*q == ' ' || *q == '\t'){
		q++;
	    }

	   while(isalnum(*q)){
		*p = *q;
		p++;
		q++;	
	    }
	    *p = '\0';

	    if(!strcmp(pass, password))
			success = 1;
	    break;
	}
    }

    fclose(fp);

    return success;
}

#if 0
/* *
 * @requeststr: command string with authentication information from client .
 * when server starting, server request "HTTP GET", then server is as "client".
 * return 1 if http request come from server, else return 0.
 * */
int snx_http_auth_client_is_server(const char* requeststr)
{
	const char *p = requeststr;
	int fromServer = 0 ;
	while(1)
	{
		if(!strncasecmp(p, "User-Agent: ", 12))
		{
			p+=12;
			if(!strncmp(p,"snxAgent", 8))
			{
				fromServer = 1;
			}
		}
		p++;
		
		if(*p == '\0')
			break;
    	}
	return fromServer;
}
#endif

/**
 * @requeststr: command string with authentication information from client .
 * 
 * return value: 1 on success, 0 on fail
 **/
int msSvrHttpAuthenticate(const char *requeststr)
{
	const char *str = requeststr;
	char strstr[64];
	char result[64];
	char user[32];
	char pwd[32];
	int success = 0;

	ZEROARRAY(strstr, 64);
	ZEROARRAY(result, 64);
	ZEROARRAY(user, 32);
	ZEROARRAY(pwd, 32);

	do{
		if(!parse_authorized_header(str, strstr))
		{
			break;
		}
		
		int len = (int)strlen(strstr);	
		
		if(!base64Decode(strstr, result, len))
		{
			break;
		}
		
		if(!getUserPwd(result, user, pwd))
		{
			break;
		}
		
		if(find_correspond_pwd(user, pwd))
		{
			success = 1;
		}
	}while(0);
	
	return success;
}

