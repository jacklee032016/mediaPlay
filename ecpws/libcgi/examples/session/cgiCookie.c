
#include "libCgi.h"


int main()
{
	char *myCookieValue ="CookieValue";
	char	*myCookie = "ecpwsCookie";
	char	*value;
	
	cgi_init();
	cgi_process_form();

	value = cgi_cookie_value(myCookie);

	if (!value) {
		cgi_add_cookie(myCookie, myCookieValue, NULL, NULL, NULL, 0);
	}

	cgi_init_headers();

	// write initial html code
	puts("<html><body>");

	if (value)
	{		
		printf(""
		"<br><b>Congratulations, you've have cookie %s</b><br>"
		"", value);
	}
	else {// Nope, he isn't
		puts("No cookie exist now, please reload with 'LOAD' button"
			"<form action='"ROOT_CGI_BIN"CgiCookie."CGI_EXTENSION"' method='get'>"
			"	<input type='submit' name='submit' value='LOAD'>"
			"	<input type='hidden' name='action' value='login'>"
			"</form>"
		"");
	}		

	puts("</body></thml>");

	cgi_end();
	return 0;
}

