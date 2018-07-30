
#ifdef	WIN32
#include <windows.h>
#endif
#include <stdio.h>

int main(void)
{
#if 0
	char		env[256];
#else
	char		*env;
#endif
	char		*key = "SERVER_SOFTWARE";
	printf("Content-Length: 46\r\n");
	printf("Content-Type: text/plain\r\n\r\n");
	printf("CGI C Example \n");

#if 0
	if(!GetEnvironmentVariable(key, env, sizeof(env)) )
#else		
	if(!(env = getenv(key)) )
#endif
	{
		printf("Environment Variable '%s' is not found (%d)\n", key, GetLastError() );
	}
	else
	{
		printf("%s=%s\n", key, env);
	}

	return 0;
}

