
/* called by WWWROOT/post.html */

#include "libCgi.h"

int main()
{	
	cgi_init();
	cgi_process_form();	
	cgi_init_headers();

	if (cgi_param("action"))
	{
		printf("<b>First name:</b> %s<br>", cgi_param("firstname"));
		printf( "<b>Last name:</b> %s<br>", cgi_param("lastname"));
		printf("<b>E-Mail: </b>%s<br>", cgi_param("email"));
		printf( "<b>sex: </b>%s<br><br>", cgi_param("sex"));
	}
	else
		cgi_include("post.html");

	cgi_end();
	return 0;
}

