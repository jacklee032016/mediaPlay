
#include "libCgi.h"

static const char *libcgi_error_type[] =
{
	CGI_NAME_STR" Information",
	CGI_NAME_STR" Caution",
	CGI_NAME_STR" Warning",
	CGI_NAME_STR" Out of memory"
	CGI_NAME_STR" Fatal",
};

void libcgi_error(E_CGI_T error_code, const char *msg, ...)
{
	va_list arguments;

	if (!cgi_display_errors)
		return;

	cgi_init_headers();
	va_start(arguments, msg);	

	printf("<b>%s</b>: ", libcgi_error_type[error_code]);
	vprintf(msg, arguments);
	puts("<br>");

	va_end(arguments);

	if ((error_code == E_FATAL) || (error_code == E_MEMORY))
	{
		cgi_end();
		exit(EXIT_FAILURE);
	}
}

void libcgiLog(const char *format, ...)
{
	static char debugStr[8192];

	va_list marker;
	va_start( marker, format );

	/* Initialize variable arguments. */
	memset(debugStr, 0, sizeof(debugStr));

	/* vsprintf : param of va_list; sprintf : param of varied params such as 'format ...' */
	vsprintf(debugStr, format, marker);
	va_end( marker);

	fprintf(stderr, debugStr, strlen(debugStr));
}


