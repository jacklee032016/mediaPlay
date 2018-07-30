/*
 *
*/

#include "muxSvrWeb.h"
#ifndef	WIN32
#include <sys/resource.h>
#endif

#define ACCESS_DENY		0
#define ACCESS_ALLOW	1


typedef	enum
{
	S0A = 1,		/* no argument */
	S1A = 2,		/* one arugment */
	S2A = 3,		/* 2 arguments */
}CFG_T;

typedef	struct ccommand
{
	const char	*name;
	const int		type;
	void			(*action) (char *, char *, void *);
	void			*object;
}Command;


/* Fakery to keep the value passed to action() a void *,
   see usage in table and c_add_alias() below */
static ALIAS_TYPE_T script_number = ALIAS_T_SCRIPT;
static ALIAS_TYPE_T redirect_number = ALIAS_T_REDIRECT;
static ALIAS_TYPE_T alias_number = ALIAS_T_ALIAS;
static int access_allow_number = ACCESS_ALLOW;
static int access_deny_number = ACCESS_DENY;
static UID_T current_uid = 0;

#ifndef	WIN32	
static void __set_user(char *v1, char *v2, void *t)
{
	struct passwd *passwdbuf;
	char *endptr;
	int i;

//	ECPWS_LOG_INFO("User %s = ", v1);

	i = strtol(v1, &endptr, 0);
	if (*v1 != '\0' && *endptr == '\0')
	{
		_webSvr.cfg.server_uid = i;
	}
	else
	{
		passwdbuf = getpwnam(v1);
		if (!passwdbuf)
		{
			if (current_uid)
				return;
			ECPWS_FATAL("No such user: %s\n", v1);
		}
		_webSvr.cfg.server_uid = passwdbuf->pw_uid;
	}
	
	ECPWS_LOG_INFO("%d\n", _webSvr.cfg.server_uid);
}

static void __set_group(char *v1, char *v2, void *t)
{
	struct group *groupbuf;
	char *endptr;
	int i;

//	ECPWS_LOG_INFO("Group %s = ", v1);
	i = strtol(v1, &endptr, 0);
	if (*v1 != '\0' && *endptr == '\0')
	{
		_webSvr.cfg.server_gid = i;
	}
	else
	{
		groupbuf = getgrnam(v1);
		if (!groupbuf)
		{
			if (current_uid)
				return;
			ECPWS_FATAL( "No such group: %s\n", v1);
		}
		_webSvr.cfg.server_gid = groupbuf->gr_gid;
	}
//	ECPWS_LOG_INFO("%d\n", _webSvr.cfg.server_gid);
}
#endif
	

static void __set_string(char *v1, char *v2, void *t)
{
//	ECPWS_LOG_INFO("Setting pointer %p to string \"%s\"...", t, v1);
	
	if (t)
	{
		if (*(char **) t != NULL)
			free(*(char **) t);
		
		*(char **) t = STRDUP(v1);
		if (!*(char **) t)
		{
			ECPWS_FATAL("Unable to strdup in %s", __FUNCTION__);
		}
		
//		ECPWS_LOG_INFO("done.\n");
	}
	else
	{
//		ECPWS_LOG_INFO("skipped.\n");
	}
}

static void __set_int(char *v1, char *v2, void *t)
{
	char *endptr;
	int i;
	
//	ECPWS_LOG_INFO("Setting pointer %p to integer string %s ..", t, v1);
	
	if (t)
	{
		i = strtol(v1, &endptr, 0); /* Automatic base 10/16/8 switching */
		if (*v1 != '\0' && *endptr == '\0')
		{
			*(int *) t = i;
			
//			WEB_MSG_DEBUG(" Integer converted as %d, done\n", i);
		}
		else
		{/* XXX should tell line number to user */
			ECPWS_LOG_INFO("Error: \"%s\" found where integer expected\n", v1);
		}
	}
	else
	{
//		ECPWS_LOG_INFO("skipped.\n");
	}
}

static void __set_unity(char *v1, char *v2, void *t)
{
//	ECPWS_LOG_INFO("Setting pointer %p to unity\n", t);
	
	if (t)
		*(int *) t = 1;
}

static void __add_mime_type(char *v1, char *v2, void *t)
{
//	add_mime_type(v2, v1);
}

static void __add_mime_types_file(char *v1, char *v2, void *t)
{
	/* v1 is the file */
	FILE *f;
	char buf[256], *p;
	char *type, *extension, *c2;
	int len;

	f = fopen(v1, "r");
	if (f == NULL)
	{
		ECPWS_FATAL("Can't open mime-types file %s", v1);
	}
	
	while (fgets(buf, 255, f) != NULL)
	{
		if (buf[0] == '\0' || buf[0] == '#' || buf[0] == '\n')
			continue;

		c2 = strchr(buf, '\n');
		if (c2)
			*c2 = '\0';

		len = strcspn(buf, "\t ");
		if (!len)
		{
			ECPWS_LOG_INFO("Improperly formatted mime-type: \"%s\"\n", buf);
			continue;
		}

		buf[len] = '\0';
		type = buf;
		for (p = buf + len + 1; *p; ++p) {
			if (isalnum(*p))
			break;
		}

		for (len = strcspn(p, "\t "); len; len = strcspn(p, "\t "))
		{
			p[len] = '\0';
			extension = p;

			add_mime_type(extension, type);
			/* blah blah */
			for (p = p + len + 1; *p; ++p) {
				if (isalnum(*p))
					break;
			}
		}
	}
	fclose(f);
}


static void __add_alias(char *v1, char *v2, void *t)
{
//	ECPWS_LOG_INFO("Calling add_alias with args \"%s\", \"%s\", and %d\n", v1, v2, *(int *) t);
	
	add_alias(v1, v2, *(ALIAS_TYPE_T *) t);
}

static void __add_access(char *v1, char *v2, void *t)
{
#ifdef ACCESS_CONTROL
	access_add(v1, *(int *) t);
#else
	ECPWS_LOG_INFO( "This version of Boa doesn't support access controls.\n"
		"Please recompile with --enable-access-control.\n");
#endif
}


static struct ccommand clist[] = {
	{"Port", S1A, __set_int, &_webSvr.cfg.server_port},
	{"Listen", S1A, __set_string, &_webSvr.cfg.server_ip}, /* In Linux, it is Eth0; Windows, it is IP address */
	{"BackLog", S1A, __set_int, &_webSvr.cfg.backlog},
#ifdef	WIN32
	{"User", S1A, __set_string, &_webSvr.cfg.server_uid},
	{"Group", S1A, __set_string, &_webSvr.cfg.server_gid},
#else
	{"User", S1A, __set_user, NULL},
	{"Group", S1A, __set_group, NULL},
#endif	
	{"ServerAdmin", S1A, __set_string, &_webSvr.cfg.server_admin},
	{"ServerRoot", S1A, __set_string, &_webSvr.cfg.server_root},
	{"UseLocaltime", S0A, __set_unity, &_webSvr.cfg.use_localtime},
	{"ErrorLog", S1A, __set_string, &_webSvr.cfg.error_log_name},
	{"AccessLog", S1A, __set_string, &_webSvr.cfg.access_log_name},
	{"CgiLog", S1A, __set_string, &_webSvr.cfg.cgi_log_name}, /* compatibility with CGILog */
	{"CGILog", S1A, __set_string, &_webSvr.cfg.cgi_log_name},
	{"VerboseCGILogs", S0A, __set_unity, &_webSvr.cfg.verbose_cgi_logs},
	{"ServerName", S1A, __set_string, &_webSvr.cfg.server_name},
	{"VirtualHost", S0A, __set_unity, &_webSvr.cfg.virtualhost},
	{"VHostRoot", S1A, __set_string, &_webSvr.cfg.vhost_root},
	{"DefaultVHost", S1A, __set_string, &_webSvr.cfg.default_vhost},
	{"DocumentRoot", S1A, __set_string, &_webSvr.cfg.document_root},
	{"UserDir", S1A,  __set_string, &_webSvr.cfg.user_dir},
	{"DirectoryIndex", S1A, __set_string, &_webSvr.cfg.directory_index},
	{"DirectoryMaker", S1A, __set_string, &_webSvr.cfg.dirmaker},
	{"DirectoryCache", S1A, __set_string, &_webSvr.cfg.cachedir},
	{"PidFile", S1A,  __set_string, &_webSvr.cfg.pid_file},
	{"KeepAliveMax", S1A, __set_int, &_webSvr.cfg.ka_max},
	{"KeepAliveTimeout", S1A, __set_int, &_webSvr.cfg.ka_timeout},
	{"MimeTypes", S1A, __add_mime_types_file, NULL},
	{"DefaultType", S1A, __set_string, &_webSvr.cfg.default_type},
	{"DefaultCharset", S1A, __set_string, &_webSvr.cfg.default_charset},
	{"AddType", S2A, __add_mime_type, NULL},
	{"ScriptAlias", S2A, __add_alias, &script_number},
	{"Redirect", S2A, __add_alias, &redirect_number},
	{"Alias", S2A, __add_alias, &alias_number},
	{"SinglePostLimit", S1A, __set_int, &_webSvr.cfg.single_post_limit},
	{"CGIPath", S1A, __set_string, &_webSvr.cfg.cgi_path},
	{"CGIumask", S1A, __set_int, &_webSvr.cfg.cgi_umask},
	{"MaxConnections", S1A, __set_int, &_webSvr.cfg.maxConnections},
	{"Allow", S1A, __add_access, &access_allow_number},
	{"Deny", S1A, __add_access, &access_deny_number},

	/* added following for Internet WWW Service, lizhijie 2006.07.25 */
	{"BaseAuthen", S1A, __set_string, &_webSvr.cfg.baseAuthen },
};


static struct ccommand *_lookup_keyword(char *c)
{
	struct ccommand *p;
	
//	WEB_MSG_DEBUG("Checking string '%s' against keyword list\n", c);
	for (p = clist; p < clist + (sizeof (clist) / sizeof (struct ccommand)); p++)
	{
		if (STRCASECMP(c, p->name) == 0)
			return p;
	}
	return NULL;
}

static void _apply_command(Command *p, char *args)
{
	char *second;

	switch (p->type)
	{
		case S0A:
			(p->action) (NULL, NULL, p->object);
			break;
			
		case S1A:
			(p->action) (args, NULL, p->object);
			break;
			
		case S2A:
			/* FIXME: if no 2nd arg exists, we use NULL. Desirable? */
			while (isspace(*args))
				++args;
			
			if (*args == '\0')
			{
				ECPWS_FATAL("expected at least 1 arg! (%s)\n", p->name);
			}

			second = args;
			while (!isspace(*second))
				++second;
			
			if (*second == '\0') {
				/* nuthin but spaces */
				second = NULL;
			}
			else
			{
				*second = '\0';
				++second;
				while (isspace(*second))
					++second;
				
				if (*second == '\0')
				{
					second = NULL;
				}
			}

			(p->action) (args, second, p->object);
			break;
		
		default:
			exit(33);
	}
}

static void _trim(char *s)
{
	char *c = s + strlen(s) - 1;

	while (isspace(*c) && c > s) {
		*c = '\0';
		--c;
	}
}


static void _parse(FILE * f, WEB_CONFIG_T *cfg)
{
	char buf[1025], *c;
	Command *p;
	int line = 0;

	current_uid = GETUID();
	
trace();
	while (fgets(buf, 1024, f) != NULL)
	{
		++line;
		if (buf[0] == '\0' || buf[0] == '#' || buf[0] == '\n')
			continue;

		/* kill the linefeed and any trailing whitespace */
		_trim(buf);
		if (buf[0] == '\0')
			continue;

		/* look for multiple arguments */
		c = buf;
		while (!isspace(*c))
			++c;

		if (*c == '\0')
		{/* no args */
			c = NULL;
		}
		else 
		{/* one or more args */
			*c = '\0';
			++c;
			while(isspace(*c))
				++c;
		}
		
		p = _lookup_keyword(buf);
		if (!p)
		{
			ECPWS_FATAL("Line %d: Did not find keyword \"%s\"\n", line, buf);
		}
		else
		{
//			ECPWS_LOG_INFO("Found keyword %s(%s) \"%s\"!\n", p->name, buf, c);

			_apply_command(p, c);
		}
	}
}



void webSvrParseConfigFile(SVR_WEB *webSvr)
{
	FILE *configFd;
	WEB_CONFIG_T	*cfg = &webSvr->cfg;

	cfg->cgi_umask = 027;
	cfg->single_post_limit = SINGLE_POST_LIMIT_DEFAULT;
	cfg->backlog = SO_MAXCONN;

	current_uid = GETUID();

	configFd = fopen(cfg->configFileName, "r");
	if (!configFd) {
		ECPWS_FATAL("Could not open %s for reading:\n", cfg->configFileName);
	}

	_parse(configFd, cfg);
	fclose(configFd);

	if (!cfg->server_name)
	{
		//        struct hostent *he;
		char temp_name[100];

		if (gethostname(temp_name, 100) == -1)
		{
			ECPWS_FATAL("gethostname:");
		}
		/*
		he = gethostbyname(temp_name);
		if (he == NULL) {
		ECPWS_FATAL("gethostbyname:");
		}

		server_name = STRDUP(he->h_name);
		*/
		cfg->server_name = STRDUP(temp_name);
		if (cfg->server_name == NULL)
		{
			ECPWS_FATAL("strdup:");
		}
	}
		
	cfg->tempdir = getenv("TMP");
	if (cfg->tempdir == NULL)
		cfg->tempdir = "/tmp";

	if (cfg->single_post_limit < 0)
	{
		fprintf(stderr, "Invalid value for single_post_limit: %d\n", cfg->single_post_limit);
		exit(1);
	}

	if (cfg->vhost_root && cfg->virtualhost)
	{
		DIE( "Both VHostRoot and VirtualHost were enabled, and they are mutually exclusive.\n");
	}

	if (cfg->vhost_root && cfg->document_root)
	{
		DIE( "Both VHostRoot and DocumentRoot were enabled, and they are mutually exclusive.\n");
	}

	/* lizhijie, 2006.09.14 */
#if WITH_LOCALE_DIR
	if(cfg->document_root)
	{
		char *locale_name;
		char document_root_with_locale[256];

		locale_name = getenv("LC_ALL");
		//		locale_name = setlocale(LC_ALL, "");
		//		fprintf(stderr, "with locale name '%s'\r\n",  locale_name);

		if( locale_name && (strlen(locale_name)>1 /* for case of 'C' */) )
		{
			SPRINTF(document_root_with_locale, sizeof(document_root_with_locale), "%s/%s", cfg->document_root, locale_name);
			//			fprintf(stderr, "\"%s\" with locale name %s\r\n", document_root_with_locale, locale_name);
			free(cfg->document_root);

			cfg->document_root = STRDUP(document_root_with_locale);
		}	
		//		fprintf(stderr, "document root '%s'\r\n",  document_root);
	}	
#endif

#if 0
	if(directory_index)
	{
		char *locale_name;
		char directory_index_with_locale[256];

		locale_name = getenv("LC_ALL");
		SPRINTF( directory_index_with_locale,sizeof(directory_index_with_locale), "%s/%s", locale_name, directory_index);
		fprintf(stderr, "\"%s\" with locale name %s\r\n", directory_index_with_locale, locale_name);
		free(directory_index);

		directory_index = STRDUP(directory_index_with_locale);
	}	
#endif


	if (!cfg->default_vhost) 
	{
		cfg->default_vhost = DEFAULT_VHOST;
	}

	if (!cfg->baseAuthen) 
	{
		cfg->baseAuthen = "No";
	}
#ifndef	WIN32
	if (cfg->maxConnections < 1)
	{
		struct rlimit rl;
		int c;

		/* has not been set explicitly */
		c = getrlimit(RLIMIT_NOFILE, &rl);
		if (c < 0) {
			DIE("getrlimit");
		}
		
		cfg->maxConnections = rl.rlim_cur;
	}
#endif

	ECPWS_LOG_INFO("maxConnections = %d, FD_SETSIZE=%d\n", cfg->maxConnections, FD_SETSIZE);
	if (cfg->maxConnections > FD_SETSIZE - 20)
		cfg->maxConnections = FD_SETSIZE - 20;

	ECPWS_LOG_INFO("server_name = %s\n", cfg->server_name);
	ECPWS_LOG_INFO("tempdir = %s\n", cfg->tempdir);
	ECPWS_LOG_INFO("single_post_limit = %d\n", cfg->single_post_limit);
	ECPWS_LOG_INFO("virtualhost = %d\n", cfg->virtualhost);
	ECPWS_LOG_INFO("vhost_root = %s\n", cfg->vhost_root);
	ECPWS_LOG_INFO("document_root = %s\n", cfg->document_root);
	ECPWS_LOG_INFO("maxConnections = %d\n", cfg->maxConnections);
	
	ECPWS_LOG_INFO("directory_index = %s\n", cfg->directory_index);
	ECPWS_LOG_INFO("dirmaker = %s\n", cfg->dirmaker);
	ECPWS_LOG_INFO("cachedir = %s\n", cfg->cachedir);

}

