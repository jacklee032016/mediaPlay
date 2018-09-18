/*
 *
 *  range.c  to test, build with
 *     gcc -Wall -DSTANDALONE_TEST -DFASCIST_LOGGING  range.c -o range
 *  and test with
 *     echo "bytes=0-10,15-9000,33-,-44,50-,30-50" | ./range 10000
 *  Parses client header information according to RFC2616, section 14.35.1
 */

#include "muxWeb.h"

RANGE_T *range_pool = NULL;

static void range_abort(WEB_CLIENT_CONN * conn)
{
	/* free all ranges starting with head */
	ranges_reset(conn);

	/* flag that we had an error, so no future ranges will be accepted */
	conn->ranges = range_pool_pop();
	conn->ranges->stop = ULONG_MAX;
}


static void range_add(WEB_CLIENT_CONN * conn, unsigned long start, unsigned long stop)
{
	RANGE_T *prev;
	RANGE_T *r = range_pool_pop();

 #if WEB_OPTIONS_DEBUG_ALIAS
       ECPWS_DEBUG_INFO("range.c, range_add: got: %lu-%lu\n", start, stop);
#endif

	for(prev = conn->ranges;prev;prev = prev->next)
	{
		if (prev->next == NULL)
			break;
	}

	if (prev) {
		prev->next = r;
	}
	else {
		conn->ranges = r;
	}

	r->start = start;
	r->stop = stop;
	conn->numranges++;
}

void ranges_reset(WEB_CLIENT_CONN * conn)
{
	RANGE_T *r = conn->ranges;

	while (r)
	{
		RANGE_T *bob;

		bob = r->next;
		range_pool_push(r);
		r = bob;
	}
	conn->ranges = NULL;
}

RANGE_T *range_pool_pop(void)
{
	RANGE_T *r;

	r = range_pool;
	if (r != NULL)
		range_pool = range_pool->next;
	else
	{/* have to make new range and return that */
		r = malloc(sizeof (RANGE_T));
		if (r == NULL)
			DIE("ran out of memory for new Range...");
	}
	
	r->start = 0;
	r->stop = 0;
	r->next = NULL;
	return r;
}

void range_pool_push(RANGE_T * r)
{
	r->next = range_pool;       /* whether or not range_pool is NULL */
	range_pool = r;
}


void range_pool_empty(void)
{
	while (range_pool != NULL)
	{
		RANGE_T *r = range_pool_pop();
		free(r);
		r = NULL;
	}
}

/* parse_range converts the range string to a binary form _before_
 * we know the size of the file.  ranges_fixup touches up that
 * binary form _after_ we have conn->filesize to work with.
 */
int ranges_fixup(WEB_CLIENT_CONN * conn)
{
	/* loop through the ranges */
	RANGE_T *prev, *r;

	/* a -1 start means -xx
	* a -1 stop  means xx-
	*/
	prev = NULL;
	r = conn->ranges;
	conn->ranges = NULL;

	if (conn->filesize == 0)
	{
		RANGE_T *temp;

		while(r)
		{
			temp = r;
			r = r->next;
			range_pool_push(temp);
		}
	}

	while(r)
	{
		/* possible situations:
		* 1) start == -1 :: valid (so far)
		* 2) stop  == -1 :: valid (so far)
		* 3) both are -1 :: impossible
		* 4) start <= stop :: valid
		* 5) start > stop && start != -1 :: invalid
		*/
#if WEB_OPTIONS_DEBUG_ALIAS
		ECPWS_DEBUG_INFO( "range.c: ranges_fixup: %lu-%lu\n", r->start, r->stop);
#endif

		/* no stop range specified or stop is too big.
		* RFC says it gets conn->filesize - 1
		*/
		if (r->stop == -1 || r->stop >= conn->filesize) {
			/* r->start is *not* -1 */
			r->stop = conn->filesize - 1;
		}

		/* no start range specified.
		* (such as -499 == last *500* bytes
		* thus, bytes (filesize - 499) through (filesize - 1)
		* assuming a 600 byte file:
		*       -=> 600 - 499 = 100 through 599.
		* assuming a 6 byte file, and last 4 bytes:
		*       -=> 6 - 4 = 2 through 6 - 1 = 5
		* RFC says it gets filesize - stop.
		* Stop is already valid from a filesize point of view.
		*/
		if ((long) r->start == -1) {
			/* last N bytes of the entity body.
			* r->stop contains is N
			* due to the above test we are guaranteed
			* that r->stop is < conn->filesize
			* we have to reset r->stop here, though
			*/
			r->start = conn->filesize - r->stop;
			r->stop = conn->filesize - 1;
		}

		/* start may only be <= stop.
		* in the case that start is == stop,
		* we write 1 byte.
		* in the case that start is < stop,
		*  we're OK so long as start > 0
		*/
		/* since start <= stop and stop < filesize,
		* start < filesize
		*/
		if ((long) r->start < 0 || r->start > r->stop)
		{
			RANGE_T *temp;

			temp = r;
			if (prev)
				prev->next = r->next;
			r = r->next;
			range_pool_push(temp);
			ECPWS_DEBUG_INFO( "start or end of range is invalid. skipping.\n");
			continue;
		}

		/* r->stop and r->start are now both guaranteed < conn->filesize */
		/* r->stop and r->start may be the same, however */

#if WEB_OPTIONS_DEBUG_ALIAS
		ECPWS_DEBUG_INFO( "ending with start: %lu\tstop: %lu\n", r->start, r->stop);
#endif
		if (prev == NULL)
			conn->ranges = r;

		prev = r;
		r = r->next;
	}

	if (conn->ranges == NULL)
	{/* bad range */
		conn->response_status = WEB_RES_INVALID_RANGE;
		return WEB_CLIENT_S_ERROR;
	}

#if WEB_OPTIONS_DEBUG_ALIAS
	ECPWS_DEBUG_INFO( "ranges_fixup returning 1\n");
#endif
	return 1;
}

/*
 * Description: Takes a char* string and extracts Range information from it.
 * Expects a null-terminated string in a format similar to the following:
 *   "bytes=0-10,15-20000,33-,-44,50-,50-30"
 * Does not modify the input string.
 * Returns: 0 on error, 1 on success
 * Individual ranges are recorded using add_range.  "-1" is used
 * as a marker for "not given" (as with 33- and -44 above).
 */
int range_parse(WEB_CLIENT_CONN * conn, const char *str)
{
    const char *initial_str = str;
#ifdef FASCIST_LOGGING
    fprintf(stderr, "parsing: %s\n", str);
#endif

    /* technically, this should be bytes (whitespace) = .... */
	if (STRNCASECMP(str, "bytes=", 6))
	{/* error.  Doesn't start with 'bytes=' */
		ECPWS_LOG_INFO_WITH_REQ(conn, "range \"%s\" doesn't start with \"bytes=\"\n",str);
		WEB_CONN_RES_ERR(conn,WEB_RES_ERROR);
	}

    /* the only stuff in a Range field is
     *  whitespace, digits, ',' and '-'
     */
    str += 6;

    {
        /* States */
#define initial 0
#define startnum 1
#define gap1 2
#define gap2 3
#define stopnum 4
        int mode = initial;

        /* Character codes */
#define digit 0
#define white 1
#define comma 2
#define hyphen 3
#define null 4
#define other 5
        int ccode;
        unsigned long start = 0, stop = 0;

#define ACTMASK1 (0xE0)
#define PB  (0x20)              /* Push Beginning */
#define PE  (0x40)              /* Push End */
#define DB  (0x60)              /* Disable Beginning */
#define DE  (0x80)              /* Disable End */
#define ACTMASK2 (0x18)
#define AR  (0x10)              /* Abort Range */
#define SR  (0x18)              /* Submit Range */
#define STATEMASK (0x07)
        int stable[] = {
            /* digit      white       comma          hyphen   null   other */
            PB + startnum, 0 + initial, 0 + initial, DB + gap2, 0, AR, /* IN - initial */
            PB + startnum, 0 + gap1, AR, 0 + gap2, AR, AR, /* startnum */
            AR, 0 + gap1, AR, 0 + gap2, AR, AR, /* gap1 */
            PE + stopnum, 0 + gap2, DE + SR + initial, AR, DE + SR, AR, /* gap2 */
            PE + stopnum, SR + initial, SR + initial, AR, SR, AR
        };                      /* stopnum */

        int c;
        int fcode;

        for (;;) {
            c = *str++;
            /* There's probably a better way to code this character
             * code lookup, but this will do for now.  Cache pollution
             * is more important than raw speed. */
            switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                ccode = digit;
                break;
            case ' ':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
            case '\v':
                ccode = white;
                break;          /* whitespace not actually legal */
            case ',':
                ccode = comma;
                break;
            case '-':
                ccode = hyphen;
                break;
            case '\0':
                ccode = null;
                break;
            default:
                ccode = other;
            }

            fcode = stable[mode * 6 + ccode];
            if ((fcode & ACTMASK1) == PB)
                start = start * 10 + (c - '0');
            else if ((fcode & ACTMASK1) == DB)
                start = ULONG_MAX;
            else if ((fcode & ACTMASK1) == PE)
                stop = stop * 10 + (c - '0');
            else if ((fcode & ACTMASK1) == DE)
                stop = ULONG_MAX;
            if ((fcode & ACTMASK2) == AR) {
                ECPWS_LOG_REQ_NOERR(conn, "Invalid range request \"%s\".\n", initial_str);
                range_abort(conn);
                return 0;
            }
			else if ((fcode & ACTMASK2) == SR)
            {
                if ((start == stop) && (start == ULONG_MAX))
			{
                    /* neither was specified, or they were very big. */
                    ECPWS_LOG_REQ_NOERR(conn, "Invalid range request (neither start nor stop were specified).\n");
                    range_abort(conn);
                    return 0;
                }
                range_add(conn, start, stop);
                start = 0;
                stop = 0;
            }
            if (ccode == null)
                return 1;
            mode = fcode & STATEMASK;
        }
    }
    return 1;
}

#ifdef STANDALONE_TEST
void send_r_invalid_range(WEB_CLIENT_CONN * conn)
{
	fprintf(stderr, "send_r_invalid_range\n");
}


int main(int argc, char *argv[])
{
    WEB_CLIENT_CONN conn;
    int c, fake_size = 10000;
    char buff[1024], *p;

    conn.ranges = NULL;
    if (argc >= 2)
        fake_size = simple_itoa(argv[1]);
    while (fgets(buff, 1024, stdin)) {
        p = buff + strlen(buff) - 1;
        if (p >= buff && *p == '\n')
            *p = '\0';
        conn.filesize = -666;
        c = range_parse(&conn, buff);
        printf("parse_range returned %d\n", c);
        conn.filesize = fake_size;
        c = ranges_fixup(&conn);
        printf("ranges_fixup returned %d\n", c);
        ranges_reset(&conn);
    }
    return 0;
}
#endif

