#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>

#define BUFSIZE 4096

int _tmain( int argc, char *argv[] )
{
	TCHAR chNewEnv[BUFSIZE];
	LPTSTR lpszCurrentVariable; 
	DWORD dwFlags=0;
	TCHAR szAppName[]=TEXT("WIN7_DEBUG\\simpleCgi.exe");
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	BOOL fSuccess; 

	// Copy environment strings into an environment block. 

	lpszCurrentVariable = (LPTSTR) chNewEnv;
	if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, TEXT("MySetting=A"))))
	{
		printf("String copy failed\n"); 
		return FALSE;
	}

	lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1; 
	if (FAILED(StringCchCopy(lpszCurrentVariable, BUFSIZE, TEXT("MyVersion=2")))) 
	{
		printf("String copy failed\n"); 
		return FALSE;
	}

	// Terminate the block with a NULL byte. 

	lpszCurrentVariable += lstrlen(lpszCurrentVariable) + 1; 
	*lpszCurrentVariable = (TCHAR)0; 

	// Create the child process, specifying a new environment block. 

	SecureZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	ZeroMemory( &pi, sizeof(pi) );

#ifdef UNICODE
	dwFlags = CREATE_UNICODE_ENVIRONMENT;
#endif

	fSuccess = CreateProcess( NULL, argv[1], NULL, NULL, TRUE, dwFlags,
		(LPVOID) chNewEnv,   // new environment block
		NULL, &si, &pi); 

	if (! fSuccess) 
	{
		printf("CreateProcess failed (%d)\n", GetLastError());
		return FALSE;
	}
	
	WaitForSingleObject(pi.hProcess, INFINITE);
	return TRUE;
}

