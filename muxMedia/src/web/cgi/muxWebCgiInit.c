/*
 *
 */

#include "muxWeb.h"

/* store environment variables in form of KEY=VALUE for CGI. The +1 is for the the NULL in complete_env */
static char *common_cgi_env[COMMON_CGI_COUNT + 1];

/*
 *       (and via a not-so-tricky #define, env_gen)
 * This routine calls malloc: please free the memory when you are done
 */
static char *_env_gen_extra(const char *key, const char *value, unsigned int extra)
{
	char *result;
	unsigned int key_len, value_len;

	if (value == NULL)          /* ServerAdmin may not be defined, eg */
		value = "";
	
	key_len = strlen(key);
	value_len = strlen(value);

	/* leave room for '=' sign and null terminator */
	result = malloc(extra + key_len + value_len + 2);
	if (result)
	{
		memcpy(result + extra, key, key_len);
		*(result + extra + key_len) = '=';
		memcpy(result + extra + key_len + 1, value, value_len);
		*(result + extra + key_len + value_len + 1) = '\0';
	}
	else
	{
		ECPWS_LOG_INFO("tried to allocate (key=value) extra=%d: %s=%s\n", extra, key, value);
	}
	return result;
}

void clearCgiCommonEnvs(void)
{
	int i;

	for (i = 0; i <= COMMON_CGI_COUNT; ++i)
	{
		if (common_cgi_env[i] != NULL)
		{
			free(common_cgi_env[i]);
			common_cgi_env[i] = NULL;
		}
	}
}


/* Set up the environment variables that are common to all CGI scripts */
void createCgiCommonEnvs(SVR_WEB *webSvr)
{
	int ix = 0, i;

	/* NOTE NOTE NOTE:
	If you (the reader) someday modify this chunk of code to
	handle more "common" CGI environment variables, then bump the
	value COMMON_CGI_COUNT in defines.h UP

	Also, in the case of document_root and server_admin, two variables
	that may or may not be defined depending on the way the server
	is configured, we check for null values and use an empty
	string to denote a NULL value to the environment, as per the
	specification. The quote for which follows:

	"In all cases, a missing environment variable is
	equivalent to a zero-length (NULL) value, and vice versa."
	*/
	common_cgi_env[ix++] = _env_gen_extra("PATH", ((webSvr->cfg.cgi_path != NULL) ? webSvr->cfg.cgi_path : DEFAULT_PATH), 0);
	/* added following lines for support i18n in boa , lizhijie, 2005.01.27 */
	ECPWS_LOG_INFO("locale=%s\r\n", getenv("LC_ALL"));
	common_cgi_env[ix++] =	_env_gen_extra("LC_ALL", getenv("LC_ALL"), 0);
	/* end of added */

	common_cgi_env[ix++] = 	_env_gen_extra("LD_LIBRARY_PATH", "/media/sf_mux/muxMedia/Linux.bin.X86/lib", 0);

	common_cgi_env[ix++] = 	_env_gen_extra("SERVER_SOFTWARE", SERVER_VERSION, 0);
	common_cgi_env[ix++] = _env_gen_extra("SERVER_NAME", webSvr->cfg.server_name, 0);
	common_cgi_env[ix++] = 	_env_gen_extra("GATEWAY_INTERFACE", CGI_VERSION, 0);

	common_cgi_env[ix++] = 	_env_gen_extra("SERVER_PORT", simple_itoa(webSvr->cfg.server_port), 0);

	/* NCSA and APACHE added -- not in CGI spec */
#ifdef USE_NCSA_CGI_ENV
	common_cgi_env[ix++] =	_env_gen_extra("DOCUMENT_ROOT", webSvr->cfg.document_root, 0);
	/* NCSA added */
	common_cgi_env[ix++] = _env_gen_extra("SERVER_ROOT", webSvr->cfg.server_root, 0);
#endif

	/* APACHE added */
	common_cgi_env[ix++] = _env_gen_extra("SERVER_ADMIN", webSvr->cfg.server_admin, 0);
	common_cgi_env[ix] = NULL;

	/* Sanity checking -- make *sure* the memory got allocated */
	if (ix != (COMMON_CGI_COUNT ))
	{
		ECPWS_FATAL( "COMMON_CGI_COUNT not high enough.\n");
	}

	for (i = 0; i < ix; ++i)
	{
		if (common_cgi_env[i] == NULL)
		{
			ECPWS_FATAL("Unable to allocate a component of common_cgi_env - out of memory.\n");
		}
	}
}


/* adds a variable to CGI's environment Used for HTTP_ headers */
int add_cgi_env(WEB_CLIENT_CONN * conn, const char *key, const char *value, BOOL_T hasHttpPrefix)
{
	char *p;
	unsigned int prefix_len;

	if (IS_TRUE(hasHttpPrefix))
	{
		prefix_len = 5;
	}
	else
	{
		prefix_len = 0;
	}

	if (conn->cgi_env_index < CGI_ENV_MAX)
	{
		p = _env_gen_extra(key, value, prefix_len);
		if (!p)
		{
			ECPWS_LOG_INFO_WITH_REQ(conn, "Unable to generate additional CGI environment variable -- ran out of memory!\n");
			return WEB_CLIENT_S_ERROR;
		}
		
		if (prefix_len)
			memcpy(p, "HTTP_", 5);
		conn->cgi_env[conn->cgi_env_index++] = p;
		return WEB_CLIENT_S_CONTINUE;
	}
	
	ECPWS_LOG_INFO_WITH_REQ(conn,  "Unable to generate additional CGI Environment variable \"%s%s=%s\" -- not enough space!\n",
		(prefix_len ? "HTTP_" : ""), key, value);
	return WEB_CLIENT_S_ERROR;
}


#if WEB_OPTIONS_DEBUG_CGI
 #define my_add_cgi_env(conn, key, value) { \
    int ok = add_cgi_env(conn, key, value, FALSE_T); \
    if ((ok == WEB_CLIENT_S_ERROR)) {ECPWS_ERR_INFO("CGI variable '%s':'%s' failed\n", key, value );return WEB_CLIENT_S_ERROR;} \
   ecpwsOutputMsg(key);ecpwsOutputMsg("=");\
    ecpwsOutputMsg(value);ecpwsOutputMsg("\n");}
#else
#define my_add_cgi_env(conn, key, value) { \
    int ok = add_cgi_env(conn, key, value, FALSE_T); \
    if ((ok == WEB_CLIENT_S_ERROR)) {ECPWS_ERR_INFO("CGI variable '%s':'%s' failed\n", key, value );return WEB_CLIENT_S_ERROR;} \
		}
#endif


/* adds the known client header env variables and terminates the environment array */
int complete_env(WEB_CLIENT_CONN * conn)
{
	int i;

	for (i = 0; common_cgi_env[i]; i++)
		conn->cgi_env[i] = common_cgi_env[i];

	{
		const char *w;
		switch (conn->method)
		{
			case WEB_M_POST:
				w = "POST";
				break;
				
			case WEB_M_HEAD:
				w = "HEAD";
				break;
				
			case WEB_M_GET:
				w = "GET";
				break;
			
			default:
				w = "UNKNOWN";
				break;
		}
		my_add_cgi_env(conn, "REQUEST_METHOD", (char *)w);
	}

	if (conn->header_host)
		my_add_cgi_env(conn, "HTTP_HOST", conn->header_host);
	if (conn->cookies)
		my_add_cgi_env(conn, "HTTP_COOKIE", conn->cookies);
	
	my_add_cgi_env(conn, "SERVER_ADDR", conn->localIpAddr);
	my_add_cgi_env(conn, "SERVER_PROTOCOL",(char *)getHeaderVerString(conn));
	my_add_cgi_env(conn, "REQUEST_URI", conn->request_uri);

	if (conn->path_info)
		my_add_cgi_env(conn, "PATH_INFO", conn->path_info);

	
	if (conn->path_translated)
	/* while path_translated depends on path_info,
	* there are cases when path_translated might
	* not exist when path_info does
	*/
		my_add_cgi_env(conn, "PATH_TRANSLATED", conn->path_translated);

	my_add_cgi_env(conn, "SCRIPT_NAME", conn->script_name);

	if (conn->query_string)
		my_add_cgi_env(conn, "QUERY_STRING", conn->query_string);
	my_add_cgi_env(conn, "REMOTE_ADDR", conn->remoteIpAddr);
	my_add_cgi_env(conn, "REMOTE_PORT", simple_itoa(conn->remotePort));

	if (conn->method == WEB_M_POST)
	{
		if (conn->content_type)
		{
			my_add_cgi_env(conn, "CONTENT_TYPE", conn->content_type);
		}
		else
		{
			my_add_cgi_env(conn, "CONTENT_TYPE", conn->webSvr->cfg.default_type);
		}
		
		if (conn->content_length)
		{
			my_add_cgi_env(conn, "CONTENT_LENGTH", conn->content_length);
		}
	}
#ifdef ACCEPT_ON
	if (conn->accept[0])
		my_add_cgi_env(conn, "HTTP_ACCEPT", conn->accept);
#endif

	if (conn->cgi_env_index < CGI_ENV_MAX + 1)
	{
		conn->cgi_env[conn->cgi_env_index] = NULL; /* terminate */
		return 1;
	}
	
	ECPWS_LOG_INFO_WITH_REQ(conn, "Not enough space in CGI environment for remainder of variables.\n");
	return 0;
}


/* Build argv list for a CGI script according to spec */
void create_argv(WEB_CLIENT_CONN *conn, char **aargv)
{
	char *p, *q, *r;
	int aargc;

	q = conn->query_string;
	aargv[0] = conn->pathname;

	/* here, we handle a special "indexed" query string.
	* Taken from the CGI/1.1 SPEC:
	* This is identified by a GET or HEAD request with a query string
	* with no *unencoded* '=' in it.
	* For such a request, I'm supposed to parse the search string
	* into words, according to the following rules:

	search-string = search-word *( "+" search-word )
	search-word   = 1*schar
	schar         = xunreserved | escaped | xreserved
	xunreserved   = alpha | digit | xsafe | extra
	xsafe         = "$" | "-" | "_" | "."
	xreserved     = ";" | "/" | "?" | ":" | "@" | "&"

	After parsing, each word is URL-decoded, optionally encoded in a system
	defined manner, and then the argument list
	is set to the list of words.


	Thus, schar is alpha|digit|"$"|"-"|"_"|"."|";"|"/"|"?"|":"|"@"|"&"

	As of this writing, escape.pl escapes the following chars:

	"-", "_", ".", "!", "~", "*", "'", "(", ")",
	"0".."9", "A".."Z", "a".."z",
	";", "/", "?", ":", "@", "&", "=", "+", "\$", ","

	Which therefore means
	"=", "+", "~", "!", "*", "'", "(", ")", ","
	are *not* escaped and should be?
	Wait, we don't do any escaping, and nor should we.
	According to the RFC draft, we unescape and then re-escape
	in a "system defined manner" (here: none).

	The CGI/1.1 draft (03, latest is 1999???) is very unclear here.

	I am using the latest published RFC, 2396, for what does and does
	not need escaping.

	Since boa builds the argument list and does not call /bin/sh,
	(boa uses execve for CGI)
	*/

	if (q && !strchr(q, '='))
	{
		/* we have an 'index' style */
		q = STRDUP(q);
		if (!q)
		{
			ECPWS_FATAL_WITH_REQ(conn, "unable to strdup 'q' in create_argv!\n");
		}
		
		for (aargc = 1; q && (aargc < CGI_ARGC_MAX);)
		{
			r = q;
			/* for an index-style CGI, + is used to separate arguments
			* an escaped '+' is of no concern to us
			*/
			if ((p = strchr(q, '+')))
			{
				*p = '\0';
				q = p + 1;
			}
			else
			{
				q = NULL;
			}
			
			if (unescape_uri(r, NULL))
			{
				/* printf("parameter %d: %s\n",aargc,r); */
				aargv[aargc++] = r;
			}
		}
		
		aargv[aargc] = NULL;
	}
	else
	{
		aargv[1] = NULL;
	}
}

