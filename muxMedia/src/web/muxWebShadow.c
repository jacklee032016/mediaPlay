/*
 *
*/

#include "muxWeb.h"

static FILE *shadow;
static char spwbuf[BUFSIZ];
static struct spwd spwd;

#define	FIELDS	9
#define	OFIELDS	5


#define SHADOW_FILE	"/etc/shadow"

/* setspent - initialize access to shadow text and DBM files */
void setspent(void)
{
	if (shadow) 
	{
		rewind(shadow);
	} 
	else 
	{
		if ((shadow = fopen(SHADOW_FILE, "r")) == NULL)
		{
			ECPWS_FATAL("%s is not opened\r\n", SHADOW_FILE);
		}	
	}
}

/* endspent - terminate access to shadow text and DBM files */
void endspent(void)
{
	if (shadow)
		(void) fclose(shadow);
	shadow = (FILE *) 0;
}


/* sgetspent - convert string in shadow file format to (struct spwd *) */
/* returns NULL on error */
struct spwd *sgetspent(const char *string)
{
	char *fields[FIELDS];
	char *cp;
	char *cpp;
	int i;

	/*
	 * Copy string to local buffer.  It has to be tokenized and we
	 * have to do that to our private copy.
	 */

	if (strlen(string) >= sizeof spwbuf)
		/* return 0; */
		return NULL;
	STRCPY(spwbuf, sizeof(spwbuf), string);

	if ((cp = strrchr(spwbuf, '\n')))
		*cp = '\0';

	/*
	 * Tokenize the string into colon separated fields.  Allow up to
	 * FIELDS different fields.
	 */

	for (cp = spwbuf, i = 0; *cp && i < FIELDS; i++) {
		fields[i] = cp;
		while (*cp && *cp != ':')
			cp++;

		if (*cp)
			*cp++ = '\0';
	}

	/*
	 * It is acceptable for the last SVR4 field to be blank.  This
	 * results in the loop being terminated early.  In which case,
	 * we just make the last field be blank and be done with it.
	 */

	if (i == (FIELDS - 1))
		fields[i++] = cp;

	if ((cp && *cp) || (i != FIELDS && i != OFIELDS))
		/* return 0; */
		return NULL;

	/*
	 * Start populating the structure.  The fields are all in
	 * static storage, as is the structure we pass back.  If we
	 * ever see a name with '+' as the first character, we try
	 * to turn on NIS processing.
	 */

	spwd.sp_namp = fields[0];
	spwd.sp_pwdp = fields[1];

	/*
	 * Get the last changed date.  For all of the integer fields,
	 * we check for proper format.  It is an error to have an
	 * incorrectly formatted number, unless we are using NIS.
	 */

	if ((spwd.sp_lstchg = strtol(fields[2], &cpp, 10)) == 0 && *cpp) {
		/* return 0; */
		return NULL;
	} else if (fields[2][0] == '\0')
		spwd.sp_lstchg = -1;

	/*
	 * Get the minimum period between password changes.
	 */

	if ((spwd.sp_min = strtol(fields[3], &cpp, 10)) == 0 && *cpp) {
		/* return 0; */
		return NULL;
	} else if (fields[3][0] == '\0')
		spwd.sp_min = -1;

	/*
	 * Get the maximum number of days a password is valid.
	 */

	if ((spwd.sp_max = strtol(fields[4], &cpp, 10)) == 0 && *cpp) {
		/* return 0; */
		return NULL;
	} else if (fields[4][0] == '\0')
		spwd.sp_max = -1;

	/*
	 * If there are only OFIELDS fields (this is a SVR3.2 /etc/shadow
	 * formatted file), initialize the other field members to -1.
	 */

	if (i == OFIELDS) {
		spwd.sp_warn = spwd.sp_inact = spwd.sp_expire = spwd.sp_flag = -1;

		return &spwd;
	}

	/*
	 * The rest of the fields are mandatory for SVR4, but optional
	 * for anything else.  However, if one is present the others
	 * must be as well.
	 */

	/*
	 * Get the number of days of password expiry warning.
	 */

	if ((spwd.sp_warn = strtol(fields[5], &cpp, 10)) == 0 && *cpp) {
		/* return 0; */
		return NULL;
	} else if (fields[5][0] == '\0')
		spwd.sp_warn = -1;

	/*
	 * Get the number of days of inactivity before an account is
	 * disabled.
	 */

	if ((spwd.sp_inact = strtol(fields[6], &cpp, 10)) == 0 && *cpp) {
		/* return 0; */
		return NULL;
	} else if (fields[6][0] == '\0')
		spwd.sp_inact = -1;

	/*
	 * Get the number of days after the epoch before the account is
	 * set to expire.
	 */

	if ((spwd.sp_expire = strtol(fields[7], &cpp, 10)) == 0 && *cpp) {
		/* return 0; */
		return NULL;
	} else if (fields[7][0] == '\0')
		spwd.sp_expire = -1;

	/*
	 * This field is reserved for future use.  But it isn't supposed
	 * to have anything other than a valid integer in it.
	 */

	if ((spwd.sp_flag = strtol(fields[8], &cpp, 10)) == 0 && *cpp) {
		/* return 0; */
		return NULL;
	} else if (fields[8][0] == '\0')
		spwd.sp_flag = -1;

	return (&spwd);
}

/* fgetspent - get an entry from an /etc/shadow formatted stream */
struct spwd *fgetspent(FILE *fp)
{
	char buf[BUFSIZ];
	char *cp;

TRACE();
	if (!fp)
	{/* return (0); */
		return NULL;
	}
		
	if (fgets(buf, sizeof buf, fp) != (char *) 0) {
		if ((cp = strchr(buf, '\n')))
			*cp = '\0';
		return (sgetspent(buf));
	}

	/* return 0; */
	return NULL;
}

/* getspent - get a (struct spwd *) from the current shadow file */
struct spwd *getspent(void)
{
		
	if (!shadow)
	{
		setspent();
	}	
	return (fgetspent(shadow));
}

/*
 * putspent - put a (struct spwd *) into the (FILE *) you provide.
 * 
 *	this was described in shadow_.h but not implemented, so here
 *	I go.  -beppu
 *
 */
int putspent(const struct spwd *sp, FILE *fp)
{
	int ret;

	/* seek to end */
	ret = fseek(fp, 0, SEEK_END);
	if (ret == -1) {
		/* return -1; */
		return 1;
	}

	/* powered by fprintf */
	fprintf(fp, "%s:%s:%ld:%ld:%ld:%ld:%ld:%ld:%s\n", sp->sp_namp,	/* login name */
			sp->sp_pwdp,		/* encrypted password */
			sp->sp_lstchg,		/* date of last change */
			sp->sp_min,			/* minimum number of days between changes */
			sp->sp_max,			/* maximum number of days between changes */
			sp->sp_warn,		/* number of days of warning before password expires */
			sp->sp_inact,		/* number of days after password expires until 
								   the account becomes unusable */
			sp->sp_expire,		/* days since 1/1/70 until account expires */
			"");
	return 0;
}


/* getspnam - get a shadow entry by name */
struct spwd *get_spnam(const char *name)
{
	struct spwd *sp;

	if (!name || !strlen(name))
		return NULL;
	
	setspent();
	while ((sp = getspent()) != NULL) {
		if (strcmp(name, sp->sp_namp) == 0)
			break;
	}
	endspent();

	return (sp);
}

