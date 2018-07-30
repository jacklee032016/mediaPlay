
#define _WIN32_WINNT 0x0501
#include <Windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h> 
#include <wchar.h> 
#include <strsafe.h>

#define BUFSIZE MAX_PATH

int _tmain(int argc, TCHAR *argv[])
{
   WIN32_FIND_DATA FindFileData;
   HANDLE hFind = INVALID_HANDLE_VALUE;
   DWORD dwError;
   LPTSTR DirSpec;
   size_t length_of_arg;
   INT retval;

   DirSpec = (LPTSTR) malloc (BUFSIZE);

   if( DirSpec == NULL )
   {
      printf( "Insufficient memory available\n" );
      retval = 1;
      goto Cleanup;
   }

   
   // Check for the directory to query, specified as 
   // a command-line parameter; otherwise, print usage.
   if(argc != 2)
   {
       _tprintf(TEXT("Usage: Test <dir>\n"));
      retval = 2;
      goto Cleanup;
   }

   // Check that the input is not larger than allowed.
   StringCbLength(argv[1], BUFSIZE, &length_of_arg);

   if (length_of_arg > (BUFSIZE - 2))
   {
      _tprintf(TEXT("Input directory is too large.\n"));
      retval = 3;
      goto Cleanup;
   }

   _tprintf (TEXT("Target directory is %s.\n"), argv[1]);

   // Prepare string for use with FindFile functions.  First, 
   // copy the string to a buffer, then append '\*' to the 
   // directory name.
   StringCbCopyN (DirSpec, BUFSIZE, argv[1], length_of_arg+1);
   StringCbCatN (DirSpec, BUFSIZE, TEXT("\\*"), 2*sizeof(TCHAR));

   // Find the first file in the directory.
   hFind = FindFirstFile(DirSpec, &FindFileData);

   if (hFind == INVALID_HANDLE_VALUE) 
   {
      _tprintf (TEXT("Invalid file handle. Error is %u.\n"), 
                GetLastError());
      retval = (-1);
   } 
   else 
   {
      _tprintf (TEXT("First file name is: %s\n"), 
                FindFileData.cFileName);
   
      // List all the other files in the directory.
      while (FindNextFile(hFind, &FindFileData) != 0) 
      {
         _tprintf (TEXT("Next file name is: %s\n"), 
                   FindFileData.cFileName);
      }
    
      dwError = GetLastError();
      FindClose(hFind);
      if (dwError != ERROR_NO_MORE_FILES) 
      {
         _tprintf (TEXT("FindNextFile error. Error is %u.\n"), 
                   dwError);
      retval = (-1);
      goto Cleanup;
      }
   }
   retval  = 0;

Cleanup:
   free(DirSpec);
   return retval;

}


