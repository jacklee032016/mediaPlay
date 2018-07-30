
// crt_pipe_beeper.c

#include <stdio.h>
#include <string.h>

int main()
{
	int   i;
	for(i=0;i<10;++i)
	{
		printf("This is speaker beep number %d...\n\7", i+1);
	}
	return 0;
}
 
// crt_pipe_BeepFilter.C
// arguments: crt_pipe_beeper.exe

#include <windows.h>
#include <process.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#define   OUT_BUFF_SIZE 512
#define   READ_FD 0
#define   WRITE_FD 1
#define   BEEP_CHAR 7

char szBuffer[OUT_BUFF_SIZE];

int Filter(char* szBuff, ULONG nSize, int nChar)
{
	char* szPos = szBuff + nSize -1;
	char* szEnd = szPos;
	int nRet = nSize;

	while (szPos > szBuff)
	{
		if (*szPos == nChar)
		{
			memmove(szPos, szPos+1, szEnd - szPos);
			--nRet;
		}
		--szPos;
	}
	return nRet;
}

int main(int argc, char** argv)
{
	int nExitCode = STILL_ACTIVE;

	if (argc >= 2)
	{
		HANDLE hProcess;
		int fdStdOut;
		int fdStdOutPipe[2];

		// Create the pipe
		if(_pipe(fdStdOutPipe, 512, O_NOINHERIT) == -1)
			return   1;

		// Duplicate stdout file descriptor (next line will close original)
		fdStdOut = _dup(_fileno(stdout));

		// Duplicate write end of pipe to stdout file descriptor
		if(_dup2(fdStdOutPipe[WRITE_FD], _fileno(stdout)) != 0)
			return   2;

		// Close original write end of pipe
		_close(fdStdOutPipe[WRITE_FD]);
		/* data allocated before can be used in child process */
		
		// Spawn process
		hProcess = (HANDLE)_spawnvp(P_NOWAIT, argv[1], (const char* const*)&argv[1]);

		/* Duplicate copy of original stdout back into stdout: restore the stdout of parent process */
		if(_dup2(fdStdOut, _fileno(stdout)) != 0)
			return   3;

		// Close duplicate copy of original stdout
		_close(fdStdOut);

		if(hProcess)
		{
			int nOutRead;
			while   (nExitCode == STILL_ACTIVE)
			{
				nOutRead = _read(fdStdOutPipe[READ_FD], szBuffer, OUT_BUFF_SIZE);
				if(nOutRead)
				{
					nOutRead = Filter(szBuffer, nOutRead, BEEP_CHAR);
					fwrite(szBuffer, 1, nOutRead, stdout);
				}

				if(!GetExitCodeProcess(hProcess,(unsigned long*)&nExitCode))
					return 4;
			}
		}
	}
	
	return nExitCode;
}
 

