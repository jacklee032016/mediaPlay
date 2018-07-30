/*
*
*/

#include "muxSvrWeb.h"

/*
 * XXX This is a simple hack version which doesn't sort the data, and just passes all unsorted.
 */
int scandir(const char *dir, struct dirent ***namelist,
        int (*select) (const struct dirent *),
        int (*compar) (const struct dirent **, const struct dirent **))
{
	struct dirent *current;
	struct dirent **names;
	int count = 0;
	int pos = 0;
//	int result = -1;
	DIR *d;
	
	d = opendir(dir);
	if (NULL == d)
		return -1;

	while (NULL != readdir(d))
		count++;

#if DEBUG_INDEX_DIR	
	printf("%d items in %s\n", count, dir);
#endif
	names = malloc(sizeof (struct dirent *) * count);

	closedir(d);
	d = opendir(dir);
	if (NULL == d)
		return -1;

	while (NULL != (current = readdir(d)))
	{
		if (NULL == select || select(current))
		{
#ifdef	WIN32
			struct dirent *copyentry = malloc(sizeof(struct dirent));
			memcpy(copyentry, current, sizeof(struct dirent));
			copyentry->d_name = malloc(MAX_PATH);//strlen(current->d_name));
			memset(copyentry->d_name, 0, MAX_PATH);//strlen(current->d_name));
			strcpy_s(copyentry->d_name, MAX_PATH, current->d_name);//, strlen(current->d_name) );
#else
			struct dirent *copyentry = malloc(current->d_reclen);
			memcpy(copyentry, current, current->d_reclen);
#endif
			names[pos] = copyentry;
			pos++;
		}
	}
	//result = 
	closedir(d);

	if (pos != count)
		names = realloc(names, sizeof (struct dirent *) * pos);

	*namelist = names;

	return pos;
}

