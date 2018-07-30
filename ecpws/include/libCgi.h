/*
*/
#ifndef __LIBCGI_H__
#define __LIBCGI_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#ifdef	WIN32
#include <time.h>			/* localtime, time */
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SESSION_FILE_PREFIX
#define SESSION_FILE_PREFIX		"cgisess_"
#endif

#define	CGI_NAME_STR			"CGI"
#define	ROOT_CGI_BIN			"/cgi-bin/"
#define	CGI_EXTENSION			"exe"

#define	CGI_EXMAPLE_HOME		"..\\..\\libcgi\\examples\\"
#define	CGI_SESSION_HOME		CGI_EXMAPLE_HOME"session\\"

#ifdef	WIN32
#define	STRCASECMP		_stricmp
#define	STRNCASECMP	_strnicmp
#define	GET_CHAR		getchar
#define	SPRINTF(dest, size, ...)		sprintf_s((dest), (size), ##__VA_ARGS__)
#define	SNPRINTF(dest, size, ...)		_snprintf_s(dest, size,  _TRUNCATE, ##__VA_ARGS__)
#else
#define	STRCASECMP		strcasecmp
#define	STRNCASECMP	strncasecmp
#define	GET_CHAR		getchar
#define	SPRINTF(dest, size, ...)		sprintf((dest), ##__VA_ARGS__)
#define	SNPRINTF(dest, size, ...)		snprintf(dest, size,  ##__VA_ARGS__)
#endif



// general purpose linked list. Actualy isn't very portable
// because uses only 'name' and 'value' variables to store data.
// Problably, in a future release, this will be replaced by 
// another type of struct
typedef struct formvarsA
{
        char	*name;
        char	*value;
        struct formvarsA *next;
} formvars;

typedef	enum
{
	E_INFORMATION 	= 0,
	E_CAUTION,
	E_WARNING,
	E_MEMORY,
	E_FATAL
}E_CGI_T;

extern formvars *formvars_start;
extern formvars *formvars_last;
extern formvars *cookies_start;
extern formvars *cookies_last;

extern int headers_initialized;
extern int cgi_display_errors;


// Session stuff
// We can use this variable to get the error message from a ( possible ) session error
// Use it togheter with session_lasterror
// i.e: printf("Session error: %s<br>", session_error_message[session_last_error]);
extern const char *session_error_message[];
extern int session_lasterror;

extern formvars *sess_list_start;


extern char SESSION_SAVE_PATH[255];
extern char SESSION_COOKIE_NAME[50];

// cgi.c
extern void libcgi_error(E_CGI_T error_code, const char *msg, ...);
void libcgiLog(const char *format, ...);

#define	CGI_TRACE()		libcgiLog("%s.%s().No.%d line\n", __FILE__, __FUNCTION__, __LINE__)

extern formvars *process_data(char *query, formvars **start, formvars **last, const char delim, const char sep);


// General purpose cgi functions
extern void cgi_init_headers(void);
extern void cgi_redirect(char *url);
extern void cgi_fatal(const char *error);
extern char *cgi_unescape_special_chars(char *str);
extern char *cgi_escape_special_chars(char *str);
extern char *cgi_param_multiple(const char *name);
extern char *htmlentities(const char *str);
extern int cgi_include(const char *filename);
extern formvars *cgi_process_form(void);
extern int cgi_init(void);
extern void cgi_end(void);
extern char *cgi_param(const char *var_name);
extern void cgi_send_header(const char *header);

// Cookie functions
extern int cgi_add_cookie(const char *name, const char *value, const char *max_age, const char *path, const char *domain, const int secure);
extern formvars *cgi_get_cookies(void);
extern char *cgi_cookie_value(const char *cookie_name);

// General purpose string functions
extern int strnpos(char *s, char *ch, unsigned int count);
extern int strpos(char *s, char *ch);
extern char *strdel(char *s, int start, int count);
extern char **explode(char *src, const char *token, int *total);
extern char *substr(char *src, const int start, const int count);
extern char *stripnslashes(char *s, int n);
extern char *addnslashes(char *s, int n);
extern char *stripnslashes(char *s, int n);
extern char *str_nreplace(char *str, const char *delim, const char *with, int n);
extern char *str_replace(char *str, const char *delim, const char *with);
extern char *addslashes(char *str);
extern char *stripslashes(char *str);
extern char *str_base64_encode(char *str);
extern char *str_base64_decode(char *str);
extern char *recvline(FILE *fp);
extern char *md5(const char *str);

extern void slist_add(formvars *item, formvars **start, formvars **last);
extern int slist_delete(char *name, formvars **start, formvars **last);
extern char *slist_item(const char *name, formvars *start);

extern void slist_free(formvars **start);
 
 
extern void cgi_session_set_max_idle_time(unsigned long seconds);
extern int cgi_session_destroy();
extern int cgi_session_register_var(const char *name, const char *value);
extern int cgi_session_alter_var(const char *name, const char *new_value);
extern int cgi_session_var_exists(const char *name);
extern int cgi_session_unregister_var(char *name);
extern int cgi_session_start();
extern void cgi_session_cookie_name(const char *cookie_name);
extern char *cgi_session_var(const char *name);
extern void cgi_session_save_path(const char *path);

formvars *libcgiUpload(const char *boundary, formvars **start, formvars **last);

#define	SESSION_CGI_MAIN			ROOT_CGI_BIN"sessionMain."CGI_EXTENSION
#define	SESSION_CGI_LOGIN			ROOT_CGI_BIN"sessionLogin."CGI_EXTENSION
#define	SESSION_CGI_DESTORY		ROOT_CGI_BIN"sessionDestroy."CGI_EXTENSION
#define	SESSION_CGI_RESTRICTED	ROOT_CGI_BIN"sessionRestricted."CGI_EXTENSION
#define	SESSION_CGI_SHOWVARS		ROOT_CGI_BIN"sessionShowVars."CGI_EXTENSION

#ifdef __cplusplus
}
#endif

#endif

