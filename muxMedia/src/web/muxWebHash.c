/*
 *
 */

#ifdef TEST
#include <stdio.h>
#include <stdlib.h>
#else
#include "muxWeb.h"
#endif

/*
 unreserved = alnum | mark
 alnum = "0".."9" | "A".."Z" | "a".."z"
 mark = "-" | "_" | "." | "!" | "~" | "*" | "'" | "(" | ")"
 noescape = unreserved | ":" | "@" | "&" | "=" | "+" | "$" | "," | "/"
 */

unsigned long _needs_escape[(NEEDS_ESCAPE_BITS + NEEDS_ESCAPE_WORD_LENGTH -1) / NEEDS_ESCAPE_WORD_LENGTH];

void build_needs_escape(void)
{
	unsigned int a, b;
	const unsigned char special[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789"
		"-_.!~*'():@&=+$,/?";
	
	/* 21 Mar 2002 - jnelson - confirm with Apache 1.3.23 that '?'  is safe to leave unescaped.
	*/
	unsigned short i, j;

	b = 1;
	for (a = 0; b != 0; a++)
		b = b << 1;
	
	/* I found $a bit positions available in an unsigned long. */
	if (a < NEEDS_ESCAPE_WORD_LENGTH)
	{
#if 0	/* in order to be used in indexDir program */
		ECPWS_FATAL("NEEDS_ESCAPE_SHIFT configuration error -- %d should be <= log2(%d)\n", NEEDS_ESCAPE_SHIFT, a);
#else
		printf("NEEDS_ESCAPE_SHIFT configuration error -- %d should be <= log2(%d)\n", NEEDS_ESCAPE_SHIFT, a);
#endif
	}
	else if (a >= 2 * NEEDS_ESCAPE_WORD_LENGTH)
	{
		/* needs_escape_shift configuration suboptimal */
	}
	else
	{
		/* Ahh, just right! */ ;
	}
	
//	memset(_needs_escape, ~0, sizeof (_needs_escape));
	memset(_needs_escape, 0, sizeof (_needs_escape));
	for (i = 0; i < sizeof (special) - 1; ++i)
	{
		j = special[i];
		if (j >= NEEDS_ESCAPE_BITS)
		{
			/* warning: character $j will be needlessly escaped. */
		}
		else
		{
			_needs_escape[NEEDS_ESCAPE_INDEX(j)] &= ~NEEDS_ESCAPE_MASK(j);
		}
	}
}

#ifdef TEST
int main(void)
{
    int i;
    build_needs_escape();
    for (i = 0; i <= NEEDS_ESCAPE_BITS; ++i) {
        if (needs_escape(i)) {
            fprintf(stdout, "%3d needs escape.\n", i);
        }
    }
    return (0);
}
#endif



#ifndef MAX_HASH_LENGTH
#define MAX_HASH_LENGTH 4
#endif

/*
 * There are two hash tables used, each with a key/value pair stored in a hash_struct.  They are:
 *
 * mime_hashtable:
 *     key = file extension
 *   value = mime type
 *
 * passwd_hashtable:
 *     key = username
 *   value = home directory
 *
 */

typedef struct _hash_struct_
{
	char		*key;
	char		*value;
	struct _hash_struct_ *next;
}hash_struct;


static hash_struct *mime_hashtable[MIME_HASHTABLE_SIZE];
static hash_struct *passwd_hashtable[PASSWD_HASHTABLE_SIZE];

#ifdef WANT_ICKY_HASH
static unsigned four_char_hash(const char *buf)
{
	unsigned int hash = (buf[0] +    (buf[1] ? buf[1] : 241 +
		(buf[2] ? buf[2] : 251 +
		(buf[3] ? buf[3] : 257))));
	
	ECPWS_DEBUG_INFO("four_char_hash(%s) = %u\n", buf, hash);
	return hash;
}
#define _boa_hash four_char_hash

/* The next two hashes taken from http://www.cs.yorku.ca/~oz/hash.html
 * In my (admittedly) very brief testing, djb2_hash performed
 * very slightly better than sdbm_hash.
 */
#else
#ifdef WANT_SDBM_HASH
static unsigned sdbm_hash(const char *str)
{
	unsigned hash = 0;
	int c;
	short count = MAX_HASH_LENGTH;

	ECPWS_DEBUG_INFO("sdbm_hash(%s) = ", str);

	while ((c = *str++) && count--)
		hash = c + (hash << 6) + (hash << 16) - hash;

	ECPWS_DEBUG_INFO("%u\n", hash);
	return hash;
}
#define _boa_hash sdbm_hash
#else
#ifdef WANT_DJB2_HASH
static unsigned djb2_hash(const char *str)
{
	unsigned hash = 5381;
	int c;
	short count = MAX_HASH_LENGTH;

	ECPWS_DEBUG_INFO("djb2_hash(%s) = ", str);

	while ((c = *(str++)) && count--)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	ECPWS_DEBUG_INFO("%u\n", hash);

	return hash;
}
#define _boa_hash djb2_hash
#else
/*
 * magic 32 bit FNV1a hash constants
 * See: http://www.isthe.com/chongo/tech/comp/fnv/index.html
 */
#define OFFSET_BASIS 2166136261U
#define FNV_PRIME 16777619U
/*
 * hash.c:152: warning: width of integer constant may change on other systems with -traditional
 */
static unsigned fnv1a_hash(const char *str)
{
	unsigned int hash = OFFSET_BASIS;
	short count = MAX_HASH_LENGTH;

	/* compute FNV1a hash of the first file component*/
	do {
		hash ^= *str++;
		hash *= FNV_PRIME;
	} while (*str != '\0' && count--);

	return hash;
}
#define _boa_hash fnv1a_hash
#endif
#endif
#endif

static unsigned _hashValue(const char *str)
{
	if (str == NULL || str[0] == '\0')
	{
		ECPWS_LOG_INFO("Attempt to hash NULL or empty stringin %s\n", __FILE__);
		return 0;
	}
	return _boa_hash(str);
}

/*
 * Description: adds the ASCII values of the file extension letters
 * and mods by the hashtable size to get the hash value
 */
#define	GET_HASH_VALUE(extension, tableSize)   (_hashValue((extension))% (tableSize) ) 

/*
 * Description: Adds a key/value pair to the provided hashtable
 */
static hash_struct *_hashInsert(hash_struct * table[], const unsigned int hash, const char *key, const char *value)
{
	hash_struct *current, *trailer;

	if (!key || key[0] == '\0') 
	{
		ECPWS_LOG_INFO("Yipes! Null or empty value sent as key in %s!\n", __FUNCTION__);
		return NULL;
	}

	if (!value ||value[0] == '\0' )
	{
		ECPWS_LOG_INFO( "Yipes! Null or empty value sent as value in %s!\n", __FUNCTION__);
		return NULL;
	}

	/* should we bother to check table, hash, and key for NULL/0 ? */
//	ECPWS_DEBUG_INFO( "Adding key \"%s\" for value \"%s\" (hash=%d)\n", key, value, hash);

	current = table[hash];
	while (current)
	{
		if (!strcmp(current->key, key))
			return current;     /* don't add extension twice */
		
		if (current->next)
			current = current->next;
		else
			break;
	}

	/* not found */
	trailer = (hash_struct *) malloc(sizeof (hash_struct));
	if (trailer == NULL) {
		WARN("unable to allocate memory for _hashInsert");
		return NULL;
	}

	trailer->key = STRDUP(key);
	trailer->value = STRDUP(value);
	trailer->next = NULL;

	if (trailer->key == NULL || trailer->value == NULL)
	{
		int errno_save = errno;

		free(trailer);
		if (trailer->key)
			free(trailer->key);
		if (trailer->value)
			free(trailer->value);
		
		errno = errno_save;
		WARN("allocated key or value is NULL");
		return NULL;
	}

	/* successfully allocated and built new hash structure */
	if (!current) {
		/* no entries for this hash value */
		table[hash] = trailer;
	}
	else
	{
		current->next = trailer;
	}

	return trailer;
}

static hash_struct *_hashFind(hash_struct * table[], const char *key, const unsigned int hash)
{
	hash_struct *current;

	current = table[hash];

	if (!key || key[0] == '\0')
	{
		ECPWS_LOG_INFO("Yipes! Null or empty value sent as key in hash in %s!\n", __FUNCTION__);
		return NULL;
	}

	while (current)
	{
		if (!strcmp(current->key, key)) /* hit */
			return current;
		
		current = current->next;
	}

	return NULL;
}

static void _hashClear(hash_struct * table[], int size)
{
	int i;
	hash_struct *temp;
	for (i = 0; i < size; ++i)
	{ /* these limits OK? */
		temp = table[i];
		while (temp)
		{
			hash_struct *temp_next;

			temp_next = temp->next;
			free(temp->key);
			free(temp->value);
			free(temp);

			temp = temp_next;
		}
		table[i] = NULL;
	}
}

void hash_show_stats(void)
{
	int i;
	hash_struct *temp;
	int total = 0;
	int count;

	for (i = 0; i < MIME_HASHTABLE_SIZE; ++i) 
	{ /* these limits OK? */
		if (mime_hashtable[i])
		{
			count = 0;
			temp = mime_hashtable[i];
			while (temp)
			{
				temp = temp->next;
				++count;
			}

			ECPWS_DEBUG_INFO("mime_hashtable[%d] has %d entries\n", i, count);

			total += count;
		}
	}

	ECPWS_DEBUG_INFO("mime_hashtable has %d total entries\n", total);

	total = 0;
	for (i = 0; i < PASSWD_HASHTABLE_SIZE; ++i)
	{ /* these limits OK? */
		if (passwd_hashtable[i])
		{
			temp = passwd_hashtable[i];
			count = 0;
			while (temp)
			{
				temp = temp->next;
				++count;
			}

			ECPWS_DEBUG_INFO( "passwd_hashtable[%d] has %d entries\n", i, count);

			total += count;
		}
	}

	ECPWS_LOG_INFO( "passwd_hashtable has %d total entries\n", total);

}


/******* Generic Hash Functions Above, Specfic Below ***************/

/* Adds a key/value pair to the mime_hashtable  */
void ecpwsAddMimeType(const char *extension, const char *type)
{
	unsigned int hash;

	hash = GET_HASH_VALUE(extension, MIME_HASHTABLE_SIZE);
	_hashInsert(mime_hashtable, hash, extension, type);
}


/*Returns the mime type for a supplied filename. Returns default type if not found */
char *get_mime_type(const char *filename)
{
#define	FAULT_MIME		_webSvr.cfg.default_type
	char *extension;
	hash_struct *current;

	unsigned int hash;

	if (filename == NULL || filename[0] == '\0')
	{
		ECPWS_LOG_INFO("Attempt to hash NULL string in %s!\n", __FUNCTION__);
		return FAULT_MIME;
	}

	extension = strrchr(filename, '.');

	/* remember, extension points to the *last* '.' in the filename,
	* which may be NULL or look like:
	*  foo.bar
	*  foo. (in which case extension[1] == '\0')
	*/
	/* extension[0] *can't* be NIL */
	if (!extension || extension[1] == '\0')
		return FAULT_MIME;

	/* make sure we hash on the 'bar' not the '.bar' */
	++extension;

	hash = GET_HASH_VALUE(extension, MIME_HASHTABLE_SIZE);
	current = _hashFind(mime_hashtable, extension, hash);
	
	return (current ? current->value : FAULT_MIME);
}


/* Returns a point to the supplied user's home directory. Adds to the hashtable if it's not already present.
 */
char *get_home_dir(const char *name)
{
	hash_struct *current;

	unsigned int hash;

	hash = GET_HASH_VALUE(name, PASSWD_HASHTABLE_SIZE);

	current = _hashFind(passwd_hashtable, name, hash);

	if (!current)
	{/* not found */
#ifndef	WIN32	
		struct passwd *passwdbuf;

		passwdbuf = getpwnam(name);

		if (!passwdbuf)         /* does not exist */
			return NULL;

		current = _hashInsert(passwd_hashtable, hash, name, passwdbuf->pw_dir);
#else
		current = _hashInsert(passwd_hashtable, hash, name, DEFAULT_HOME_DIR);
#endif
	}

	return (current ? current->value : NULL);
}

void clearupAllHash(void)
{
	_hashClear(mime_hashtable, MIME_HASHTABLE_SIZE);
	_hashClear(passwd_hashtable, PASSWD_HASHTABLE_SIZE);
}


