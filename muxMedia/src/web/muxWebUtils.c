/*
 *
 *
 */

/* $Id: util.c,v 1.1.1.1 2006/11/29 17:51:53 lizhijie Exp $ */

#include "muxWeb.h"

#define HEX_TO_DECIMAL(char1, char2)	\
    (((char1 >= 'A') ? (((char1 & 0xdf) - 'A') + 10) : (char1 - '0')) * 16) + \
    (((char2 >= 'A') ? (((char2 & 0xdf) - 'A') + 10) : (char2 - '0')))

const char day_tab[28] = "Sun,Mon,Tue,Wed,Thu,Fri,Sat,";

extern	const char month_tab[48];

/*
 * Replaces unsafe/incorrect instances of:
 *  //[...] with /
 *  /./ with /
 *  /../ with / (technically not what we want, but browsers should deal with this, not servers)
 */
void clean_pathname(char *pathname)
{
	char *cleanpath, c;

	cleanpath = pathname;
	while ((c = *pathname++))
	{
		if (c == '/')
		{
			while (1)
			{
				if (*pathname == '/')
					pathname++;
				else if (*pathname == '.' && *(pathname + 1) == '/')
					pathname += 2;
				else if (*pathname == '.' && *(pathname + 1) == '.' && *(pathname + 2) == '/')
				{
					pathname += 3;
				}
				else
					break;
			}
			c = '/';
		}
		*cleanpath++ = c;
	}

	*cleanpath = '\0';
}

/* Turns a three letter month into a 0-11 int . */
int month2int(const char *monthname)
{
	switch (*monthname)
	{
		case 'A':
			return (*++monthname == 'p' ? 3 : 7);
		case 'D':
			return (11);
		case 'F':
			return (1);
		case 'J':
			if (*++monthname == 'a')
				return 0;
			return (*++monthname == 'n' ? 5 : 6);
		case 'M':
			return (*(monthname + 2) == 'r' ? 2 : 4);
		case 'N':
			return (10);
		case 'O':
			return (9);
		case 'S':
		return (8);
			default:
			return (-1);
	}
}

/*
 * Description: Decides whether a file's mtime is newer than the
 * If-Modified-Since header of a request.
 Sun, 06 Nov 1994 08:49:37 GMT    ; RFC 822, updated by RFC 1123
 Sunday, 06-Nov-94 08:49:37 GMT   ; RFC 850, obsoleted by RFC 1036
 Sun Nov  6 08:49:37 1994         ; ANSI C's asctime() format
 31 September 2000 23:59:59 GMT   ; non-standard

 * RETURN VALUES:
 *  0: File has not been modified since specified time.
 *  1: File has been.
 * -1: Error!
 */
int modified_since(time_t *mtime, const char *if_modified_since)
{
	struct tm *file_gmt = NULL;
	const char *ims_info;
	char monthname[10 + 1];
	int day, month, year, hour, minute, second;
	int comp;

	ECPWS_LOG_INFO("if_modified_since:%s\n", if_modified_since);
	/* skip non-whitespace */
	ims_info = if_modified_since;
	while (*ims_info != ' ' && *ims_info != '\0')
		++ims_info;
	if (*ims_info != ' ')
		return -1;

	/* the pre-space in the third scanf skips whitespace for the string */
	if (SSCANF(ims_info, "%d %3s %d %d:%d:%d GMT", &day, monthname, &year, &hour, &minute, &second) == 6)
	{
		ECPWS_LOG_INFO("RFC1123 time format\n");/* RFC 1123 */
	}
	else if (SSCANF(ims_info, "%d-%3s-%d %d:%d:%d GMT", &day, monthname, &year, &hour, &minute, &second) == 6)
	{/* RFC 1036 */
		ECPWS_LOG_INFO("RFC1036 time format\n");
		year += 1900;
	}
	else if (SSCANF(ims_info, " %3s %d %d:%d:%d %d", monthname, &day, &hour, &minute, &second, &year) == 6)
	{/* asctime() format */
		ECPWS_LOG_INFO("asctime time format\n");
	}
	/*  allow this non-standard date format: 31 September 2000 23:59:59 GMT */
	/* NOTE: Use if_modified_since here, because the date *starts*
	*       with the day, versus a throwaway item
	*/
	else if (SSCANF(if_modified_since, "%d %10s %d %d:%d:%d GMT",	&day, monthname, &year, &hour, &minute, &second) == 6)
	{
		ECPWS_LOG_INFO("non-standard time format\n");
	}
	else
	{
		ECPWS_LOG_INFO("Error scanning \"%s\" in modified_since\n", ims_info);
		return -1;              /* error */
	}

	GM_TIME(file_gmt, mtime);

	month = month2int(monthname);

	if (month == -1)
	{
		ECPWS_LOG_INFO("Invalid month name: \"%s\"\n", monthname);
		return -1;
	}

	/* Go through from years to seconds -- if they are ever unequal, we know which one is newer and can return */
	if ((comp = 1900 + file_gmt->tm_year - year))
		return (comp > 0);
	if ((comp = file_gmt->tm_mon - month))
		return (comp > 0);
	if ((comp = file_gmt->tm_mday - day))
		return (comp > 0);
	if ((comp = file_gmt->tm_hour - hour))
		return (comp > 0);
	if ((comp = file_gmt->tm_min - minute))
		return (comp > 0);
	if ((comp = file_gmt->tm_sec - second))
		return (comp > 0);

	return 0; /* this person must really be into the latest/greatest */
}


/* Description: Turns a string into all upper case (for HTTP_ header forming) AND changes - into _ */
char *to_upper(char *str)
{
	char *start = str;

	while (*str)
	{
		if (*str == '-')
			*str = '_';
		else
			*str = toupper(*str);
		
		str++;
	}

	return start;
}

/* Decodes a uri, changing %xx encodings with the actual character.  The query_string should already be gone.
 * Return values: *  1: success;  0: illegal string
 */
int unescape_uri(char *uri, char **query_string)
{
	char c, d;
	char *uri_old;

	uri_old = uri;

	while ((c = *uri_old))
	{
		if (c == '%')
		{
			uri_old++;
			if ((c = *uri_old++) && (d = *uri_old++))
			{
				*uri = HEX_TO_DECIMAL(c, d);
				if (*uri < 32 || *uri > 126)
				{/* control chars in URI */
					*uri = '\0';
					return 0;
				}
			}
			else
			{
				*uri = '\0';
				return 0;
			}
			++uri;
		}
		else if (c == '?')
		{  /* query string */
			if (query_string)
				*query_string = ++uri_old;
			/* stop here */
			*uri = '\0';
			return (1);
		}
		else if (c == '#')
		{  /* fragment */
			/* legal part of URL, but we do *not* care.
			* However, we still have to look for the query string */
			if (query_string)
			{
				++uri_old;
				while ((c = *uri_old))
				{
					if (c == '?')
					{
						*query_string = ++uri_old;
						break;
					}
					++uri_old;
				}
			}
			break;
		}
		else
		{
			*uri++ = c;
			uri_old++;
		}
	}

	*uri = '\0';
	return 1;
}


/* rfc822 (1123) time is exactly 29 characters long
 * "Sun, 06 Nov 1994 08:49:37 GMT"
 */
void rfc822_time_buf(char *buf, time_t s)
{
	struct tm *t = NULL;
	time_t				current_time;
	char *p;
	unsigned int a;

	if (!s)
	{
		time(&current_time);
		GM_TIME(t, &current_time);
	}
	else
	{
		GM_TIME(t, &s);
	}

	p = buf + 28;
	/* p points to the last char in the buf */

	p -= 3;
	/* p points to where the ' ' will go */
	memcpy(p--, " GMT", 4);

	a = t->tm_sec;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = t->tm_min;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ':';
	a = t->tm_hour;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ' ';
	a = 1900 + t->tm_year;
	
	while (a)
	{
		*p-- = '0' + a % 10;
		a /= 10;
	}
	
	/* p points to an unused spot to where the space will go */
	p -= 3;
	/* p points to where the first char of the month will go */
	memcpy(p--, month_tab + 4 * (t->tm_mon), 4);
	*p-- = ' ';
	a = t->tm_mday;
	*p-- = '0' + a % 10;
	*p-- = '0' + a / 10;
	*p-- = ' ';
	p -= 3;
	memcpy(p, day_tab + t->tm_wday * 4, 4);
}

char *simple_itoa(unsigned int i)
{
	/* 21 digits plus null terminator, good for 64-bit or smaller ints
	* for bigger ints, use a bigger buffer!
	*
	* 4294967295 is, incidentally, MAX_UINT (on 32bit systems at this time)
	* and is 10 bytes long
	*/
	static char local[22];
	char *p = &local[21];
	*p = '\0';
	do {
		*--p = '0' + i % 10;
		i /= 10;
	} while (i != 0);
	
	return p;
}

/* I don't "do" negative conversions Therefore, -1 indicates error */
int web_atoi(const char *s)
{
	int retval;
	char *reconv;

	if (!isdigit(*s))
		return -1;

	retval = atoi(s);
	if (retval < 0)
		return -1;

	reconv = simple_itoa((unsigned) retval);
	if (memcmp(s, reconv, strlen(s)) != 0) {
		return -1;
	}
	
	return retval;
}

int create_temporary_file(short want_unlink, char *storage, unsigned int size, char *tempdir)
{
	static char _tempfile[MAX_PATH_LENGTH + 1];
	int fd = 0;

	SNPRINTF(_tempfile, MAX_PATH_LENGTH, "%s/webSvr-temp.XXXXXX", tempdir);

	/* open temp file */
	fd = mkstemp(_tempfile);
	if (fd == -1)
	{
		ECPWS_ERR_INFO("mkstemp '%s'", _tempfile);
		return 0;
	}

	if (storage != NULL)
	{
		unsigned int len = strlen(_tempfile);
		if (len < size)
		{
			memcpy(storage, _tempfile, len + 1);
		}
		else
		{
			CLOSE(fd);
			fd = 0;
			ECPWS_LOG_INFO( "not enough memory for memcpy in storage\n");
			want_unlink = 1;
		}
	}

	if (want_unlink)
	{
		if (UNLINK(_tempfile) == -1)
		{
			CLOSE(fd);
			fd = 0;
			ECPWS_ERR_INFO("unlink temp file '%s'\n", _tempfile);
		}
	}

	return (fd);
}

int real_set_block_fd(int fd)
{
	int flags = 0;
#ifndef	WIN32
	flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		return -1;

	flags &= ~NOBLOCK;
	flags = fcntl(fd, F_SETFL, flags);
#endif	
	return flags;
}

int real_set_nonblock_fd(int fd)
{
	int flags = 0;

#ifndef	WIN32
	flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		return -1;

	flags |= NOBLOCK;
	flags = fcntl(fd, F_SETFL, flags);
#endif	
	return flags;
}

/* Quoting from rfc1034:
<domain> ::= <subdomain> | " "

<subdomain> ::= <label> | <subdomain> "." <label>

<label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]

<ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>

<let-dig-hyp> ::= <let-dig> | "-"

<let-dig> ::= <letter> | <digit>

<letter> ::= any one of the 52 alphabetic characters A through Z in
upper case and a through z in lower case

<digit> ::= any one of the ten digits 0 through 9

and

The labels must follow the rules for ARPANET host names.  They must
start with a letter, end with a letter or digit, and have as interior
characters only letters, digits, and hyphen.  There are also some
restrictions on the length.  Labels must be 63 characters or less.
*/
int check_host(const char *r)
{
	/* a hostname can only consist of chars and numbers, and sep. by only one period.
	* It may not end with a period, and must not start with a number.
	* >0: correct
	* -1: error
	*  0: not returned
	*/
	const char *c;
	short period_ok = 0;
	short len = 0;

	c = r;
	if (c == NULL)
	{
		return -1;
	}

	/* must start with a letter */
	if (!isalpha(*c))
		return -1;

	if (strlen(c) > 63)
		return -1;

	len = 1;
	while (*(++c) != '\0')
	{
		/* interior letters may be alphanumeric, '-', or '.' */
		/* '.' may not follow '.' */
		if (isalnum(*c) || *c == '-')
			period_ok = 1;
		else if (*c == '.' && period_ok)
			period_ok = 0;
		else
			return -1;
		++len;
	}
	
	/* c points to '\0' */
	--c;
	/* must end with a letter or digit */
	if (!isalnum(*c))
		return -1;
	return len;
}

void strlower(char *s)
{
	while (*s != '\0')
	{
		*s = tolower(*s);
		++s;
	}
}

