
#include "libCgi.h"


int main()
{
	char *action;
	
	cgi_init();
	cgi_process_form();

	// session id will be stored in a cookie called "MY_COOKIE"
	cgi_session_cookie_name("MY_COOKIE");
	// tell to store session files into "session_files" directory
#ifdef	WIN32
	cgi_session_save_path("d:\\works\\");
#else
	cgi_session_save_path("session_files/");
#endif

	// Starts the session
	cgi_session_start();

	action = cgi_param("action");
	if (action) {
		if (!STRCASECMP("login", action) && cgi_param("str"))
			cgi_session_register_var("str", cgi_param("str"));
		else if (!STRCASECMP("logoff", action))
			cgi_session_destroy();
	}

	cgi_init_headers();

	// write initial html code
	puts("<html><body>");

	// includes the description for this example
	cgi_include(CGI_SESSION_HOME"session_ex2_desc.html");
  	
	// The user is logged?
	if (cgi_session_var_exists("str"))
	{		
		printf(""
		"<br><b>Congratulations, you've logged</b><br>"
		"If you want to finish the session, <a href='"ROOT_CGI_BIN"CgiSession2."CGI_EXTENSION"?action=logoff'>click here</a>!"
		"<br><br>"
		"You pass phrase was <b>%s</b><br>"
		"", cgi_session_var("str"));
	}
	else {// Nope, he isn't
		puts(""
			"<form action='"ROOT_CGI_BIN"CgiSession2."CGI_EXTENSION"' method='get'>"
			"	Pass phrase: <input type='text' name='str'>"
			"	<input type='submit' name='submit' value='Click to login'>"
			"	<input type='hidden' name='action' value='login'>"
			"</form>"
		"");
	}		

	// Print session configuration directives
	printf("Session configuration options: <br>"
		"saving session files at: <b>%s</b> directory<br>"
		"name of the (personalized) cookie: <b>%s</b><br>", SESSION_SAVE_PATH, SESSION_COOKIE_NAME);

	puts("</body></thml>");

	cgi_end();
	return 0;
}

