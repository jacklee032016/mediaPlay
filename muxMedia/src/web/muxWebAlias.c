/*
 *
 */

#include "muxWeb.h"

typedef struct alias
{
	char				*fakename;             /* URI path to file */
	char				*realname;             /* Actual path to file */
	ALIAS_TYPE_T	type;
	unsigned int		fake_len;      /* strlen of fakename */
	unsigned int		real_len;      /* strlen of realname */
	struct alias		*next;
}ALIAS_T;

static ALIAS_T *alias_hashtable[ALIAS_HASHTABLE_SIZE];

/*
 * Description: adds the ASCII values of the file letters
 * and mods by the hashtable size to get the hash value
 * Note: stops at first '/' (or '\0')
 */
int _getAliasHashValue(const char *file)
{
	unsigned int hash = 0;
	unsigned int i = 0;
	unsigned char c;

	if (file == NULL)
	{
		WARN("file sent to _getAliasHashValue is NULL");
		return 0;
	}
	else if (file[0] == '\0')
	{
		WARN("file sent to _getAliasHashValue is empty");
		return 0;
	}

	hash = file[i++];
	while ((c = file[i++]) && c != '/')
		hash += (unsigned int) c;

	return hash % ALIAS_HASHTABLE_SIZE;
}


/*
 * Description: Performs full parsing on a ScriptAlias request Sets path_info and script_name
 * Return values:
 * 0: failure, shut down
 * 1: success, continue
 */
static int _initScriptAlias(WEB_CLIENT_CONN * conn, ALIAS_T *current1, unsigned int uri_len)
{
	static char pathname[MAX_HEADER_LENGTH + 1];
	struct stat statbuf;
	CmnMuxWebConfig *cfg = &conn->webSvr->cfg;

	int i = 0;
	unsigned char c;
	int err;

	/* copies the "real" path + the non-alias portion of the uri to pathname.	*/
	if (cfg->vhost_root)
	{
		/* vhost_root + IP + host + / + cgi-bin + resource */
		unsigned int l1, l2, l3;
		char *ap;

		l1 = strlen(cfg->vhost_root);
		l2 = strlen(conn->localIpAddr);
		ap = conn->host;
		l3 = strlen(ap);

		if( (l1 + 1 + l2 + 1 + l3 + 1 + current1->real_len +(uri_len - current1->fake_len) + 1) > sizeof(pathname))
		{
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "uri too long!\n");
			return 0;
		}
		
		memcpy(pathname, cfg->vhost_root, l1);
		pathname[l1] = DIR_SPLITTER_CHAR;
		memcpy(pathname + l1 + 1, conn->localIpAddr, l2);
		pathname[l1 + 1 + l2] = DIR_SPLITTER_CHAR;
		memcpy(pathname + l1 + 1 + l2 + 1, ap, l3);
		pathname[l1 + 1 + l2 + 1 + l3] = DIR_SPLITTER_CHAR;
		memcpy(pathname + l1 + 1 + l2 + 1 + l3 + 1, current1->realname, current1->real_len);
		memcpy(pathname + l1 + 1 + l2 + 1 + l3 + 1 + current1->real_len,  &conn->request_uri[current1->fake_len],	uri_len - current1->fake_len + 1); /* the +1 copies the NUL */
	}
	else
	{
		if (current1->real_len + uri_len - current1->fake_len + 1 > sizeof(pathname))
		{
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "uri too long!\n");
			return 0;
		}
		SPRINTF(pathname, sizeof(pathname), "%s%c%s", current1->realname, DIR_SPLITTER_CHAR, &conn->request_uri[current1->fake_len]);
	}

#if WEB_OPTIONS_DEBUG_ALIAS
	ECPWS_DEBUG_INFO( "- pathname in _initScriptAlias is: \"%s\" (\"%s\")\n", pathname, pathname + current1->real_len);
#endif

	if (strncmp("nph-", pathname + current1->real_len, 4) == 0  || (conn->http_version == WEB_HTTP09) )
		conn->cgi_type = CGI_T_NPH;
	else
		conn->cgi_type = CGI_T_CGI;

	/* start at the beginning of the actual uri...
	(in /cgi-bin/bob, start at the 'b' in bob */
	i = current1->real_len;
	c = '\0';

	/* go to first and successive '/' and keep checking if it is a full pathname
	* on success (stat (not lstat) of file is a *regular file*)
	*/
	do
	{
		c = pathname[++i];
		if (c == DIR_SPLITTER_CHAR)
		{
			pathname[i] = '\0';
			err = stat(pathname, &statbuf);
			pathname[i] = DIR_SPLITTER_CHAR;
			if (err == -1)
			{
				conn->response_status = WEB_RES_NOT_FOUND;
				return WEB_CLIENT_S_ERROR;
			}

			/* is it a dir? */
			if (!S_ISDIR(statbuf.st_mode))
			{
				/* check access */
				if (!S_IS_FILE(statbuf.st_mode) )
				{ /* o+rx */
					conn->response_status = WEB_RES_FORBIDDEN;
					return WEB_CLIENT_S_ERROR;
				}
				

				/* stop here */
				break;
			}
		}
	} while (c != '\0');

	conn->script_name = STRDUP(conn->request_uri);
	if (!conn->script_name)
	{
		ECPWS_LOG_REQ_NOERR(conn, "unable to strdup conn->request_uri for %s\n", conn->request_uri );
		return 0;
	}

	if (c == '\0')
	{
		err = stat(pathname, &statbuf);
		if (err == -1)
		{
			ECPWS_ERR_INFO("%s stat failure", pathname);
			conn->response_status = WEB_RES_NOT_FOUND;
			return WEB_CLIENT_S_ERROR;
		}

		/* is it a dir? */
		if (!S_ISDIR(statbuf.st_mode))
		{/* check access */

			if (!S_IS_FILE(statbuf.st_mode))
			{ /* o+rx */
				ECPWS_ERR_INFO("%s is not a file", pathname);
				conn->response_status = WEB_RES_FORBIDDEN;
				return WEB_CLIENT_S_ERROR;
			}
			/* stop here */

		}
		else
		{
			ECPWS_ERR_INFO("No privilege to access %s", pathname);
			conn->response_status = WEB_RES_FORBIDDEN;
			return WEB_CLIENT_S_ERROR;
		}
	}/* we have path_info if c == '/'... still have to check for query */
	else if (c == DIR_SPLITTER_CHAR)
	{
		int hash;
		ALIAS_T *current;
		int path_len;

		conn->path_info = STRDUP(pathname + i);
		if (!conn->path_info)
		{
			ECPWS_LOG_REQ_NOERR(conn, "unable to strdup %s + index for conn->path_info\n", pathname );
			conn->response_status = WEB_RES_ERROR;
			return WEB_CLIENT_S_ERROR;
		}
		pathname[i] = '\0'; /* strip path_info from path */
		path_len = strlen(conn->path_info);
		/* we need to fix script_name here */
		/* index points into pathname, which is
		* realname/cginame/foo
		* and index points to the '/foo' part
		*/
		conn->script_name[strlen(conn->script_name) - path_len] = '\0'; /* zap off the /foo part */

		/* now, we have to re-alias the extra path info....
		this sucks.
		*/
		hash = _getAliasHashValue(conn->path_info);
		current = alias_hashtable[hash];
		while (current && !conn->path_translated)
		{
			if (!strncmp(conn->path_info, current->fakename, current->fake_len))
			{
				static char buffer[MAX_HEADER_LENGTH + 1];

				if (current->real_len + path_len - current->fake_len + 1 > sizeof(buffer))
				{
					ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "uri too long!\n");
					return WEB_CLIENT_S_ERROR;
				}

				memcpy(buffer, current->realname, current->real_len);
				/*
				strcpy(buffer + current->real_len,  sizeof(), &conn->path_info[current->fake_len]);
				*/
				memcpy(buffer + current->real_len, conn->path_info + current->fake_len, 	path_len - current->fake_len + 1);/* +1 for NUL */
				conn->path_translated = STRDUP(buffer);
				if (!conn->path_translated)
				{
					ECPWS_LOG_REQ_NOERR(conn,"unable to strdup %s for conn->path_translated\n", buffer);
					return WEB_CLIENT_S_ERROR;
				}
			}
			current = current->next;
		}
		
		/* no alias... try userdir */
		if (!conn->path_translated && cfg->user_dir && conn->path_info[1] == '~')
		{
			char *user_homedir;
			char *p;

			p = strchr(pathname + i + 1, DIR_SPLITTER_CHAR);
			if (p)
				*p = '\0';

			user_homedir = get_home_dir(pathname + i + 2);
			if (p)
				*p = DIR_SPLITTER_CHAR;

			if (!user_homedir)
			{ /* no such user */
				conn->response_status = WEB_RES_NOT_FOUND;
				return WEB_CLIENT_S_ERROR;
			}
			
			{
				unsigned int l1 = strlen(user_homedir);
				unsigned int l2 = strlen(cfg->user_dir);
				unsigned int l3 = 0;
				if (p)
					l3 = strlen(p);

				conn->path_translated = malloc(l1 + 1 + l2 + l3 + 1);
				if (conn->path_translated == NULL)
				{
					ECPWS_LOG_REQ_NOERR(conn, "unable to malloc memory for conn->path_translated\n" );
					return WEB_CLIENT_S_ERROR;
				}
				
				memcpy(conn->path_translated, user_homedir, l1);
				conn->path_translated[l1] = DIR_SPLITTER_CHAR;
				memcpy(conn->path_translated + l1 + 1, cfg->user_dir, l2 + 1); /* copy the NUL just in case */
				
				if (p)
					memcpy(conn->path_translated + l1 + 1 + l2, p, l3 + 1);
			}
		}
		else if (!conn->path_translated && cfg->document_root)
		{
			/* no userdir, no aliasing... try document root */
			unsigned int l1, l2;
			l1 = strlen(cfg->document_root);
			l2 = path_len;

			conn->path_translated = malloc(l1 + l2 + 1);
			if (conn->path_translated == NULL)
			{
				ECPWS_LOG_REQ_NOERR(conn, "unable to malloc memory for conn->path_translated\n");
				return WEB_CLIENT_S_ERROR;
			}
			memcpy(conn->path_translated, cfg->document_root, l1);
			memcpy(conn->path_translated + l1, conn->path_info, l2 + 1);
		}
	}

	conn->pathname = STRDUP(pathname);
	if (!conn->pathname)
	{
		WEB_CONN_ERROR(conn, WEB_RES_ERROR);
		ECPWS_LOG_REQ_NOERR(conn, "unable to strdup pathname for %s\n", pathname);
		return WEB_CLIENT_S_ERROR;
	}

	return 1;
}

/* Locates uri in the alias hashtable if it exists */
static ALIAS_T *_findAlias(char *uri, unsigned int urilen)
{
	ALIAS_T *current;
	int hash;

	/* Find ScriptAlias, Alias, or Redirect */
	if (urilen == 0)
	{
		WARN("uri len is 0!");
		urilen = strlen(uri);
	}

	hash = _getAliasHashValue(uri);

#if WEB_OPTIONS_DEBUG_ALIAS
	ECPWS_DEBUG_INFO(	" - looking for \"%s\" (hash=%d,len=%d)...\n", uri, hash, urilen);
#endif

	current = alias_hashtable[hash];
	while (current)
	{
#if WEB_OPTIONS_DEBUG_ALIAS
		ECPWS_DEBUG_INFO(	"comparing \"%s\" (request) to \"%s\" (alias): \n", uri, current->fakename);
#endif
		
		/* current->fake_len must always be:  shorter or equal to the uri */
		
		/*
		* when performing matches:
		* If the virtual part of the URL ends in '/', and
		* we get a match, stop there.
		* Otherwise, we require '/' or '\0' at the end of the URL.
		* We only check if the virtual path does *not* end in '/'
		*/
		/* fakename is URL format, so '/' is splitter */
		if (current->fake_len <= urilen &&
			!memcmp(uri, current->fakename, current->fake_len) &&
			(current->fakename[current->fake_len - 1] == '/' ||
			(current->fakename[current->fake_len - 1] != '/' &&
			(uri[current->fake_len] == '\0' ||
			uri[current->fake_len] == '/'))))
		{
#if WEB_OPTIONS_DEBUG_ALIAS
			ECPWS_DEBUG_INFO(	 "Got it!\n");
#endif
			return current;
		}
		else
		{
			ECPWS_DEBUG_INFO(	 "Don't Got it!\n");
		}
		current = current->next;
	}
	return current;
}

/*
 * Description: Parse a request's virtual path.
 * Sets query_string, pathname directly.
 * Also sets path_info, path_translated, and script_name via  _initScriptAlias
 *
 * Note: CGI_T_NPH in user dir is currently broken
 * Note -- this should be broken up.
 * Return values:
 *   0: failure, close it down
 *   1: success, continue
 */
int translate_uri(WEB_CLIENT_CONN * conn)
{
	static char buffer[MAX_HEADER_LENGTH + 1];
	ALIAS_T *current;
	char *p;
	unsigned int uri_len;
	CmnMuxWebConfig *cfg = &conn->webSvr->cfg;

#if WEB_OPTIONS_DEBUG_ALIAS
	ECPWS_DEBUG_INFO("request_uri:'%s'\n", conn->request_uri);
#endif
	if (conn->request_uri[0] != '/')
	{
		conn->response_status = WEB_RES_BAD_REQUEST;
		return WEB_CLIENT_S_ERROR;
	}

	/* 1: alias, eg. script, redirect, alias */
	uri_len = strlen(conn->request_uri);
	current = _findAlias(conn->request_uri, uri_len);
	if (current)
	{
		if (current->type == ALIAS_T_SCRIPT) /* Script */
			return _initScriptAlias(conn, current, uri_len);

		/* not a script alias, therefore begin filling in data */
		if (current->real_len + uri_len - current->fake_len + 1 > sizeof(buffer))
		{
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "uri too long!\n");
			return WEB_CLIENT_S_ERROR;
		}
		
		memcpy(buffer, current->realname, current->real_len);
		memcpy(buffer + current->real_len, conn->request_uri + current->fake_len, uri_len - current->fake_len + 1);

		ECPWS_DEBUG_INFO("realname:'%s';\t buffer:'%s'\n", current->realname, buffer);
		if (current->type == ALIAS_T_REDIRECT)
		{ /* Redirect */
			if (conn->method == WEB_M_POST)
			{ /* POST to non-script */
				/* it's not a cgi, but we try to POST??? */
				ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "POST to non-script is disallowed.\n");
				return WEB_CLIENT_S_ERROR;       /* not a script alias, therefore begin filling in data */
			}
			conn->response_status = WEB_RES_MOVED_TEMP;
			return WEB_CLIENT_S_ERROR;
		}
		else
		{ /* Alias */
			conn->pathname = STRDUP(buffer);
			if (!conn->pathname)
			{
				ECPWS_LOG_REQ_NOERR(conn, "unable to strdup %s onto conn->pathname\n", buffer);
				return WEB_CLIENT_S_ERROR;
			}
			return 1;
		}
	}

	/*
	The reason why this is *not* an 'else if' is that,
	after aliasing, we still have to check for '~' expansion
	*/
	if ( cfg->user_dir && conn->request_uri[1] == '~')
	{/* 2 : user's dir */
		char *user_homedir;
		char *req_urip;

		if (conn->request_uri[2] == '\0')
		{
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "Empty user-dir in URI\n");
			return WEB_CLIENT_S_ERROR;
		}

		req_urip = conn->request_uri + 2;

		/* since we have uri_len which is from strlen(conn->request_uri) */
		p = memchr(req_urip, '/', uri_len - 2);
		if (p)
			*p = '\0';

		user_homedir = get_home_dir(req_urip);
		if (p)/* have to restore request_uri in case of error */
		{
			*p = DIR_SPLITTER_CHAR;
		}

		if (!user_homedir)
		{/*no such user */
			conn->response_status = WEB_RES_NOT_FOUND;
			return WEB_CLIENT_S_ERROR;
		}
		else
		{
			unsigned int l1 = strlen(user_homedir);
			unsigned int l2 = strlen(cfg->user_dir);
			unsigned int l3 = (p ? strlen(p) : 0);

			/* we need l1 + '/' + l2 + l3 + '\0' */
			if (l1 + 1 + l2 + l3 + 1 > sizeof(buffer))
			{
				ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "uri too long!\n");
				return WEB_CLIENT_S_ERROR;
			}

			memcpy(buffer, user_homedir, l1);
			
			buffer[l1] = '/';
			/* copy the NUL in case 'p' is NULL */
			memcpy(buffer + l1 + 1, cfg->user_dir, l2 + 1);
			if (p)
				memcpy(buffer + l1 + 1 + l2, p, l3 + 1);
		}
	}
	else if (cfg->vhost_root)
	{/* 3 : virtual host */
		/* no aliasing, no userdir... */
		unsigned int l1, l2, l3, l4, l5;
		char *ap = NULL;

		/* form
		* vhost_root + '/' + ip + '/' + host + '/' + htdocs + '/' + resource
		*/

		l1 = strlen(cfg->vhost_root);
		l2 = strlen(conn->localIpAddr);
		ap = conn->host;
		l3 = strlen(ap);
		l4 = strlen("htdocs");
		l5 = strlen(conn->request_uri);

		if (l1 + 1 + l2 + 1 + l3 + 1 + l4 + l5 + 1 > sizeof(buffer))
		{
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "uri too long!\n");
			return WEB_CLIENT_S_ERROR;
		}

		memcpy(buffer, cfg->vhost_root, l1);
		buffer[l1] = DIR_SPLITTER_CHAR;
		memcpy(buffer + l1 + 1, conn->localIpAddr, l2);
		buffer[l1 + 1 + l2] = DIR_SPLITTER_CHAR;
		memcpy(buffer + l1 + 1 + l2 + 1, ap, l3);
		buffer[l1 + 1 + l2 + 1 + l3] = DIR_SPLITTER_CHAR;
		memcpy(buffer + l1 + 1 + l2 + 1 + l3 + 1, "htdocs", l4);
		/* request_uri starts with '/' */
		memcpy(buffer + l1 + 1 + l2 + 1 + l3 + 1 + l4, conn->request_uri, l5 + 1);
	}
	else if (cfg->document_root)
	{/* 4 : doc root */
		/* no aliasing, no userdir... */
		unsigned int l1, l2, l3;

		l1 = strlen(cfg->document_root);
		l2 = strlen(conn->request_uri);
		if (cfg->virtualhost)
			l3 = strlen(conn->localIpAddr);
		else
			l3 = 0;

		if (l1 + 1 + l2 + l3 + 1 > sizeof(buffer))
		{
			ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "uri too long!\n");
			return WEB_CLIENT_S_ERROR;
		}

		/* the 'l2 + 1' is there so we copy the '\0' as well */
		memcpy(buffer, cfg->document_root, l1);
		if (cfg->virtualhost)
		{
			memcpy(buffer + l1 + 1, conn->localIpAddr, l3);
			memcpy(buffer + l1 + 1 + l3, conn->request_uri, l2 + 1);
			buffer[l1] = DIR_SPLITTER_CHAR;
			buffer[l1+l3] = DIR_SPLITTER_CHAR;
		}
		else
		{
			memcpy(buffer + l1, conn->request_uri, l2 + 1);
			buffer[l1] = DIR_SPLITTER_CHAR;
			if(buffer[strlen(buffer)-1] ==DIR_SPLITTER_CHAR )
				buffer[strlen(buffer)-1] = '\0';
		}
	}
	else
	{/* 5: error */
		/* not aliased.  not userdir.  not part of document_root.  BAIL */
		conn->response_status = WEB_RES_NOT_FOUND;
		return WEB_CLIENT_S_ERROR;
	}

	/* if here,
	* o it may be aliased but it's not a redirect or a script...
	* o it may be a homedir
	* o it may be a document_root resource (with or without virtual host)
	*/
	conn->pathname = STRDUP(buffer);
	if (!conn->pathname)
	{
		ECPWS_LOG_REQ_NOERR(conn, "Could not strdup %s for conn->pathname!\n", buffer );
		return WEB_CLIENT_S_ERROR;
	}

#if WEB_OPTIONS_DEBUG_ALIAS
	ECPWS_LOG_INFO("REQ pathname=%s\n", conn->pathname);
#endif

#ifdef FASCIST_LOGGING
	ECPWS_DEBUG_INFO("- buffer is: \"%s\"\n", buffer);
	ECPWS_DEBUG_INFO(" - compare \"%s\" and \"%s\": %d\n", get_mime_type(buffer), CGI_MIME_TYPE, strcmp(CGI_MIME_TYPE, get_mime_type(buffer)));
#endif

	/* below we support cgis outside of a ScriptAlias */
	if (strcmp(CGI_MIME_TYPE, get_mime_type(conn->pathname)) == 0)
	{ /* cgi */
		/* FIXME */
		/* script_name could end up as /cgi-bin/bob/extra_path */
		conn->script_name = STRDUP(conn->request_uri);
		if (!conn->script_name)
		{
			ECPWS_LOG_REQ_NOERR(conn, "Could not strdup conn->request_uri for conn->script_name.\n" );
			return WEB_CLIENT_S_ERROR;
		}
		
		if (conn->http_version == WEB_HTTP09)
			conn->cgi_type = CGI_T_NPH;
		else
			conn->cgi_type = CGI_T_CGI;
		
		return 1;
	}
	else if (conn->method == WEB_M_POST)
	{ /* POST to non-script */
		/* it's not a cgi, but we try to POST??? */
		ECPWS_LOG_REQ_NOERR_SEND_BAD_REQ(conn, "POST to non-script disallowed.\n");
		return WEB_CLIENT_S_ERROR;
	}

	 /* we are done!! */
	return 1;
}

/*
 * Empties the alias hashtable, deallocating any allocated memory.
 */
void cleanupAlias(void)
{
	int i;
	ALIAS_T *temp;

	for (i = 0; i < ALIAS_HASHTABLE_SIZE; ++i)
	{ /* these limits OK? */
		if (alias_hashtable[i])
		{
			temp = alias_hashtable[i];
			while (temp)
			{
				ALIAS_T *temp_next;

				if (temp->fakename)
					free(temp->fakename);
				if (temp->realname)
					free(temp->realname);
				
				temp_next = temp->next;
				free(temp);
				temp = temp_next;
			}
			alias_hashtable[i] = NULL;
		}
	}
}


/*
 * Description: add an Alias, Redirect, or ScriptAlias to the alias hash table.
 */
void ecpwsAddAlias(const char *fakename, const char *realname, int _type)
{
	int hash;
	ALIAS_T *old, *newAlias;
	int fakelen, reallen;
	ALIAS_TYPE_T type = (ALIAS_TYPE_T)_type;

	/* sanity checking */
	if (fakename == NULL || realname == NULL)
	{
		DIE("NULL values sent to ecpwsAddAlias");
	}

	fakelen = strlen(fakename);
	reallen = strlen(realname);
	if (fakelen == 0 || reallen == 0)
	{
		DIE("empty values sent to ecpwsAddAlias");
	}

	hash = _getAliasHashValue(fakename);

	ECPWS_DEBUG_INFO(	" - Going to add alias: \"%s\" -=> \"%s\" (hash: %d)\n", fakename, realname, hash);

	old = alias_hashtable[hash];

	if (old)
	{
		while (old->next)
		{
			if (!strcmp(fakename, old->fakename)) /* don't add twice */
				return;
			old = old->next;
		}
	}

	newAlias = (ALIAS_T *) malloc(sizeof (ALIAS_T));
	if (!newAlias)
	{
		DIE("out of memory adding alias to hash");
	}

	if (old)
		old->next = newAlias;
	else
		alias_hashtable[hash] = newAlias;

	newAlias->fakename = STRDUP(fakename);
	if (!newAlias->fakename) {
		DIE("failed strdup");
	}
	
	newAlias->fake_len = fakelen;
	/* check for "here" */
	newAlias->realname = STRDUP(realname);
	if (!newAlias->realname)
	{
		DIE("strdup of realname failed");
	}
	newAlias->real_len = reallen;

	newAlias->type = type;
	newAlias->next = NULL;

#if WEB_OPTIONS_DEBUG_ALIAS
	ECPWS_DEBUG_INFO("- ADDED alias: \"%s\" -=> \"%s\" hash: %d\n", fakename, realname, hash);
#endif

}

