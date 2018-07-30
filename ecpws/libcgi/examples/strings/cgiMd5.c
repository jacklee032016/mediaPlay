#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "libCgi.h"

char *example_description()
{
	char *desc = CGI_NAME_STR" examples, md5 function demostration. <br>";
	
	return desc;
}

int main()
{
	char *to_encode;
	
	libcgiLog("CGIMd5 init.....\n");
	cgi_init();
	cgi_process_form();
	cgi_init_headers();
	
	if (cgi_param("action"))
	{
		libcgiLog("action data is found\n");
		to_encode = cgi_param("encode");
		
		// First check if we need to encode some string
		if (to_encode) {
			printf("<i>md5()</i> example: original string: <b>%s</b><br>"
			"Encoded string: <b>%s</b><br><br>", to_encode, to_encode);//md5(to_encode));
		}
		
		puts("<hr>");
	}
	libcgiLog("No action data is found\n");
	
	printf("<html><head><title>"CGI_NAME_STR" examples - md5()</title></head>"
	"<body>"
	"%s<br><br>"
	"<form action='"ROOT_CGI_BIN"cgiMd5."CGI_EXTENSION"' method='post'>"
	"String to encode: <input type='text' name='encode' size='30'><br>"
	"<input type='submit' value='OK' name='action'>"
	"</form>"
	"</body>"
	"</html>"
	"", example_description());

	cgi_end();
	return 0;
}


