
/* called by WWWROOT/uploadFile.html */

#include "libCgi.h"


int main()
{	
	cgi_init();
	cgi_process_form();	
	cgi_init_headers();

	if (cgi_param("filename"))
	{
		printf("<html><head></head><body><b>File '%s' upload successfully</b> %s<br>", cgi_param("filename"));
		printf( "<b>Please click <a href=\"/others\">here</a> to check it</b></BODY></HTML> " );
	}
	else
		cgi_include("uploadFile.html");

	cgi_end();
	return 0;
}

