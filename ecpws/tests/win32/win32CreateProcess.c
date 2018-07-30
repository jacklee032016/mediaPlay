
#include <windows.h>
#include <stdio.h>

//void _tmain( int argc, char *argv[] )
int main(int argc, char* argv[])
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
#if 0	
	char *szEnv[] = 
	{
		"TEST=Testing123",
		NULL
	};
#else
	char szEnv[128] = "TEST=Testing 123";
	szEnv[strlen(szEnv)] = 0;
#endif
	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	GetStartupInfo(&si);
	si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
	si.dwFlags    = STARTF_USESTDHANDLES;
	si.lpDesktop  = NULL;

	ZeroMemory( &pi, sizeof(pi) );

	if( argc != 2 )
	{
		printf("Usage: %s [cmdline]\n", argv[0]);
		return -1;
	}

	// Start the child process. 
	if( !CreateProcess( NULL,   // No module name (use command line)
		argv[1],        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		(LPVOID)&szEnv,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi )           // Pointer to PROCESS_INFORMATION structure
		) 
	{
		printf( "CreateProcess failed (%d)\n", GetLastError() );
		return -1;
	}

	// Wait until child process exits.
	WaitForSingleObject( pi.hProcess, INFINITE );

	printf( "Process (%s) ended!\n", argv[0] );
	// Close process and thread handles. 
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	return 0;
}

/*
GetEnvironmentStrings or GetEnvironmentVariable APIs.
*/

