#ifndef DIRENT_INCLUDED
#define DIRENT_INCLUDED

/*
*   Declaration of POSIX directory browsing functions and types for Win32.
*/
#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif


struct dirent
{
    char *d_name;
};

typedef ptrdiff_t handle_type; /* C99's intptr_t not sufficiently portable */

typedef struct DIR
{
	handle_type		handle; /* -1 for failed rewind */
	struct _finddata_t	info;
	struct dirent		result; /* d_name null iff first time */
	char				*name;  /* null-terminated char string */
}DIR;

DIR           *opendir(const char *);
int           closedir(DIR *);
struct dirent *readdir(DIR *);
void          rewinddir(DIR *);

#ifdef __cplusplus
}
#endif

#endif

