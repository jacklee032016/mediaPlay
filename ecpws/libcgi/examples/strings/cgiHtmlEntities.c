#include <stdio.h>

#include "libCgi.h"

char *example_description()
{
	char *desc = CGI_NAME_STR	" examples, htmltities() function demostration. <br>"
	"Type an html code into textarea and then click OK"
	"";
	
	return desc;
}

int main()
{
	char *q;
	
CGI_TRACE()	;
	cgi_init();
CGI_TRACE()	;
	cgi_process_form();
CGI_TRACE()	;
	cgi_init_headers();
	
CGI_TRACE()	;
	q = cgi_param("q123");
	
CGI_TRACE()	;
	if (q)
	{
CGI_TRACE()	;
		puts(htmlentities(q));
//		puts((q));
	}
	else {
CGI_TRACE()	;
		printf(""
		"<html><head><title>"CGI_NAME_STR" examples - htmlentities()</title></head>"
		"<body>"
		"%s<br><br>"
		"<form action='"ROOT_CGI_BIN"cgiHtmlEntities."CGI_EXTENSION"' method='post'>"
		"HTML code: <textarea rows='8' name='q123'><html><body>test, test</body></html></textarea><br>"
		"<input type='submit' value='OK' name='action'>"
		"</form>"
		"</body>"
		"</html>"
		"", example_description());	
	}
	
CGI_TRACE()	;
	cgi_end();
	return 0;
}

