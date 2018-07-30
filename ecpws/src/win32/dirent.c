/*
*  Implementation of POSIX directory browsing functions and types for Win32.
*/

#include <errno.h>
#include <io.h> /* _findfirst and _findnext set errno iff they return -1 */
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#ifdef __cplusplus
extern "C"
{
#endif

DIR *opendir(const char *name)
{
	DIR *dir = 0;

	if(name && name[0])
	{
		size_t base_length = strlen(name);
		/* search pattern must end with suitable wildcard */
		const char *all = strchr("/\\", name[base_length - 1]) ? "*" : "/*";


		if((dir = (DIR *) malloc(sizeof *dir)) != 0 &&
			(dir->name = (char *) malloc(base_length + strlen(all) +1)) != 0)
		{
			memset(dir->name, 0, base_length + strlen(all) + 1);
			strcpy_s(dir->name, base_length + strlen(all) + 1, name);
			strcat(dir->name, all);

			if((dir->handle = (handle_type) _findfirst(dir->name, &dir->info)) != -1)
			{
				dir->result.d_name = 0;
			}
			else /* rollback */
			{
				free(dir->name);
				free(dir);
				dir = NULL;
			}
		}
		else /* rollback */
		{
			free(dir);
			dir   = NULL;
			errno = ENOMEM;
		}
	}
	else
	{
		errno = EINVAL;
	}

	return dir;
}

int closedir(DIR *dir)
{
    int result = -1;

    if(dir)
    {
        if(dir->handle != -1)
        {
            result = _findclose(dir->handle);
        }

        free(dir->name);
        free(dir);
    }

    if(result == -1) /* map all errors to EBADF */
    {
        errno = EBADF;
    }

    return result;
}

struct dirent *readdir(DIR *dir)
{
	struct dirent *result = 0;

	if(dir && dir->handle != -1)
	{
		if(!dir->result.d_name || _findnext(dir->handle, &dir->info) != -1)
		{
			result = &dir->result;
			result->d_name = dir->info.name; /* Null-terminated  string with length of MAX_PATH*/
#if DEBUG_INDEX_DIR	
			printf("file name :'%s(%s)'\n", dir->info.name, result->d_name);
#endif
		}
	}
	else
	{
		errno = EBADF;
	}

	return result;
}

void rewinddir(DIR *dir)
{
    if(dir && dir->handle != -1)
    {
        _findclose(dir->handle);
        dir->handle = (handle_type) _findfirst(dir->name, &dir->info);
        dir->result.d_name = 0;
    }
    else
    {
        errno = EBADF;
    }
}

#ifdef __cplusplus
}
#endif

