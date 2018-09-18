/*
 *
 */

#include "muxWeb.h"

#define INT_TO_HEX(x) \
    ((((x)-10)>=0)?('A'+((x)-10)):('0'+(x)))

/*
 * Description: Buffers data before sending to client.
 * Returns: -1 for error, otherwise how much is stored
 */
int req_write(WEB_CLIENT_CONN * conn, const char *format, ...)
{
	unsigned int msg_len;
	static char  msg[SOCKETBUF_SIZE];
	va_list marker;

	/* Initialize variable arguments. */
	memset(msg, 0, sizeof(msg));
	va_start( marker, format );

//	ecpwsLog(NULL, FALSE_T, FALSE_T, ERR_HANDLE_LOCAL, format);
	
	/* vsprintf : param of va_list; sprintf : param of varied params such as 'format ...' */
	vsprintf(msg, format, marker);
	va_end( marker);

	msg_len = strlen(msg);

#if WEB_OPTIONS_DEBUG_DATA			
	ECPWS_LOG_INFO( "CONN%d: MSG(%d bytes)='%s'\n",conn->index, msg_len, msg);
#endif

	if (!msg_len )//|| conn->status == WEB_CLIENT_S_ERROR)
		return conn->buffer_end;

	if (conn->buffer_end + msg_len > WEB_BUFFER_SIZE)
	{
		ECPWS_LOG_INFO_WITH_REQ(conn, "There is not enough room in the buffer to copy %d bytes (%d available). Shutting down connection.\n",
			msg_len,	WEB_BUFFER_SIZE - conn->buffer_end);
#ifdef FACSIST_LOGGING
		*(conn->buffer + conn->buffer_end) = '\0';
		ECPWS_LOG_INFO("The request looks like this:\n%s\n", conn->buffer);
#endif
		conn->status = WEB_CLIENT_S_ERROR;
		return -1;
	}
	
	memcpy(conn->buffer + conn->buffer_end, msg, msg_len);
	conn->buffer_end += msg_len;
	return conn->buffer_end;
}


/*
 * Description: Buffers and "escapes" data before sending to client.
 *  as above, but translates as it copies, into a form suitably
 *  encoded for URLs in HTTP headers.
 * Returns: -1 for error, otherwise how much is stored
 */
char *req_write_escape_http(WEB_CLIENT_CONN *conn, const char *msg)
{
	char c, *dest = conn->webSvr->tempBuffer;
	const char *inp;

	int left = WEB_BUFFER_SIZE;
	inp = msg;
//	dest = conn->buffer + conn->buffer_end;
	/* 3 is a guard band, since we don't check the destination pointer
	* in the middle of a transfer of up to 3 bytes */
//	left = WEB_BUFFER_SIZE - conn->buffer_end;
	while ((c = *inp++) && left >= 3)
	{
		if (needs_escape((unsigned int) c))
		{
			*dest++ = '%';
			*dest++ = INT_TO_HEX(c >> 4);
			*dest++ = INT_TO_HEX(c & 15);
			left -= 3;
		}
		else
		{
			*dest++ = c;
			left--;
		}
	}
	
	--inp;
//	conn->buffer_end = dest - conn->buffer;

#ifdef TESTING
	if (left < 0)
	{
		ECPWS_LOG_INFO( "Overflowed buffer space!\n");
		CHDIR("/tmp");
		abort();
	}
#endif

	if (*inp != '\0')
	{
		ECPWS_LOG_INFO_WITH_REQ(conn, "Ran out of Buffer space in %s!\n", __FUNCTION__);
		conn->status = WEB_CLIENT_S_ERROR;
		return NULL;
	}
	
//	return conn->buffer_end;
	return conn->webSvr->tempBuffer;
}

/*
 * Description: Buffers and "escapes" data before sending to client.
 *  as above, but translates as it copies, into a form suitably encoded for HTML bodies.
 * Returns: -1 for error, otherwise how much is stored
 */
char *req_write_escape_html(WEB_CLIENT_CONN * conn, const char *msg)
{
	char c, *dest;
	const char *inp;
	int left = WEB_BUFFER_SIZE;
	dest = conn->webSvr->tempBuffer;

	inp = msg;
//	dest = conn->buffer + conn->buffer_end;
	/* 6 is a guard band, since we don't check the destination pointer
	* in the middle of a transfer of up to 6 bytes
	*/
//	left = WEB_BUFFER_SIZE - conn->buffer_end;
	while ((c = *inp++) && left >= 6)
	{
		switch (c)
		{
			case '>':
				*dest++ = '&';
				*dest++ = 'g';
				*dest++ = 't';
				*dest++ = ';';
				left -= 4;
				break;
			
			case '<':
				*dest++ = '&';
				*dest++ = 'l';
				*dest++ = 't';
				*dest++ = ';';
				left -= 4;
				break;
				
			case '&':
				*dest++ = '&';
				*dest++ = 'a';
				*dest++ = 'm';
				*dest++ = 'p';
				*dest++ = ';';
				left -= 5;
				break;
				
			case '\"':
				*dest++ = '&';
				*dest++ = 'q';
				*dest++ = 'u';
				*dest++ = 'o';
				*dest++ = 't';
				*dest++ = ';';
				left -= 6;
				break;
				
			default:
				*dest++ = c;
				left--;
		}
	}
	
	--inp;
///	conn->buffer_end = dest - conn->buffer;

#ifdef TESTING
	if (left < 0)
	{
		ECPWS_LOG_INFO("Overflowed buffer space! %s]\n", __FUNCTION__);
		CHDIR("/tmp");
		abort();
	}
#endif

	if (*inp != '\0')
	{
		ECPWS_LOG_INFO_WITH_REQ(conn, "Ran out of Buffer space (%d chars left) in %s\n", left, __FUNCTION__);
		conn->status = WEB_CLIENT_S_ERROR;
		return NULL;
	}
	
//	return conn->buffer_end;
	return conn->webSvr->tempBuffer;
}


/*
 * Description: Sends any backlogged buffer to client.
 * Returns: -2 for error, -1 for blocked, otherwise how much is stored
 */
int req_flush(WEB_CLIENT_CONN * conn)
{
	unsigned bytes_to_write;

	bytes_to_write = conn->buffer_end - conn->buffer_start;
#if WEB_OPTIONS_DEBUG_DATA			
	ECPWS_DEBUG_INFO("CONN %d: %d bytes to send\n",conn->index, bytes_to_write);
#endif
	if (bytes_to_write)
	{
		int bytes_written = svcWebConnWrite(conn, conn->buffer + conn->buffer_start, bytes_to_write);
		if (bytes_written < 0)
		{
			conn->buffer_start = conn->buffer_end = 0;
			conn->buffer_end = 0;
			return -2;
		}

		{
			conn->buffer[conn->buffer_start+ bytes_written] = '\0';
#if WEB_OPTIONS_DEBUG_DATA			
			ECPWS_DEBUG_INFO("CONN %d:Wrote \"%s\" (%d bytes)\n", conn->index, conn->buffer + conn->buffer_start, bytes_written );
#endif
		}

		conn->buffer_start += bytes_written;
	}
	
	if (conn->buffer_start == conn->buffer_end)
		conn->buffer_start = conn->buffer_end = 0;
	
	return conn->buffer_end;     /* successful */
}

/*
 * Description: escapes the string inp.  Uses variable buf.  If buf is
 *  NULL when the program starts, it will attempt to dynamically allocate
 *  the space that it needs, otherwise it will assume that the user
 *  has already allocated enough space for the variable buf, which
 *  could be up to 3 times the size of inp.  If the routine dynamically
 *  allocates the space, the user is responsible for freeing it afterwords
 * Returns: NULL on error, pointer to string otherwise.
 * Note: this function doesn't really belong here, I plopped it here to
 *  work around a "bug" in escape.h (it defines a global, so can't be
 *  used in multiple source files).  Actually, this routine shouldn't
 *  exist anywhere, it's only usage is in get.c's handling of on-the-fly
 *  directory generation, which would be better configured to use a combination
 *  of req_write_escape_http and req_write_escape_html.  That would involve
 *  more work than I'm willing to put in right now, though, so here we are.
 */

char *escape_string(const char *inp, char *buf)
{
	int max;
	char *ix;
	unsigned char c;

	max = strlen(inp) * 3;

	if (buf == NULL && max)
		buf = malloc(sizeof (char) * max + 1);
	if (buf == NULL)
	{
		ECPWS_LOG_INFO("malloc");
		return NULL;
	}

	ix = buf;
	while ((c = *inp++) && max > 0)
	{
		if (needs_escape((unsigned int) c))
		{
			*ix++ = '%';
			*ix++ = INT_TO_HEX(c >> 4);
			*ix++ = INT_TO_HEX(c & 15);
		}
		else
			*ix++ = c;
	}
	
	*ix = '\0';
	return buf;
}

char x2c(char *what)
{
	register char digit;
	digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A')+10 : (what[0] - '0'));
	digit *= 16;
	digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A')+10 : (what[1] - '0'));
	return(digit);
}

void unescape_url(char *url)
{
	register int x,y;
	
	for (x=0,y=0; url[y]; ++x,++y)
	{
		if((url[x] = url[y]) == '%')
		{
			url[x] = x2c(&url[y+1]);
			y+=2;
		}
	}
	url[x] = '\0';
}

int mmap_list_entries_used = 0;
int mmap_list_total_requests = 0;
int mmap_list_hash_bounces = 0;

#define MMAP_LIST_NEXT(i)					(((i)+1)&MMAP_LIST_MASK)
#define MMAP_LIST_HASH(dev,ino,size)		((ino)&MMAP_LIST_MASK)

/* define local table variable */
static MMAP_T mmap_list[MMAP_LIST_SIZE];

MMAP_T *find_mmap(WEB_CLIENT_CONN *conn, struct stat *s)
{
	char *m;
	int i, start;

	mmap_list_total_requests++;
	
	i = start = MMAP_LIST_HASH(s->st_dev, s->st_ino, s->st_size);

	for (; mmap_list[i].use_count;)
	{
		if (mmap_list[i].dev == s->st_dev && mmap_list[i].ino == s->st_ino && mmap_list[i].len == s->st_size)
		{
			mmap_list[i].use_count++;
			ECPWS_DEBUG_INFO( "Old mmap_list entry %d use_count now %d (hash was %d)\n", i, mmap_list[i].use_count, start);

			return mmap_list + i;
		}
		
		mmap_list_hash_bounces++;
		i = MMAP_LIST_NEXT(i);
		if (i == start)
		{
			/* didn't find an entry that matches our dev/inode/size.
			There might be an entry that matches later in the table,
			but that _should_ be rare.  The worst case is that we
			needlessly mmap() a file that is already mmap'd, but we
			did that all the time before this code was written,
			so it shouldn't be _too_ bad.
			*/
			/* we've looped, and found neither a free one nor a
			* match. Thus, there is no room for a new entry.
			*/
			/*        WARN("mmap hash table is full. Consider enlarging."); */
			return NULL;
		}
	}

	/* Enforce a size limit here */
	/* Disallow more entries than MMAP_LIST_USE_MAX, despite
	* having found an available slot.
	*/
	if (mmap_list_entries_used > MMAP_LIST_USE_MAX)
	{/*        WARN("Too many entries in mmap hash table."); */
		return NULL;
	}

#ifdef	WIN32
	mmap_list[i].hFile = CreateFile(conn->openedFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (mmap_list[i].hFile == INVALID_HANDLE_VALUE)
	{
		ECPWS_ERR_INFO("CreateFile %s failed", conn->openedFile);
		return NULL;
	}

	mmap_list[i].hFileMap = CreateFileMapping(mmap_list[i].hFile, 0, PAGE_READONLY, 0, 0, NULL);
	if (mmap_list[i].hFileMap == INVALID_HANDLE_VALUE)
	{
		ECPWS_ERR_INFO("CreateFileMapping %s error", conn->openedFile);
		return NULL;
	}
	m = (char *)MapViewOfFile(mmap_list[i].hFileMap, FILE_MAP_READ, 0, 0, 0);
	if ( m == NULL)
	{
		ECPWS_ERR_INFO( "Unable to mmap file: %s", conn->openedFile);
		return NULL;
	}
#else
	m = mmap(0, s->st_size, PROT_READ, MAP_OPTIONS, conn->data_fd, 0);
	if ( m == MAP_FAILED)
	{
		ECPWS_ERR_INFO( "Unable to mmap file: ");
		return NULL;
	}

#ifdef HAVE_MADVISE
	start = madvise(m, s->st_size, MADV_SEQUENTIAL);
	if (start == -1)
	{
		ECPWS_ERR_INFO("Unable to madvise file: ");
		munmap(m, s->st_size);
		return NULL;
	}
#endif

#endif


	mmap_list_entries_used++;
	mmap_list[i].dev = s->st_dev;
	mmap_list[i].ino = s->st_ino;
	mmap_list[i].mmap = m;
	mmap_list[i].use_count = 1;
	mmap_list[i].len = (int)s->st_size;
	
	return mmap_list + i;
}


void release_mmap(MMAP_T *e)
{
	if (!e)
		return;

	if (!e->use_count)
	{
		ECPWS_LOG_INFO( "mmap_list(%p)->use_count already zero!\n", e);
		return;
	}

	if (!--(e->use_count))
	{
#ifdef	WIN32
		if(!UnmapViewOfFile(e->mmap) )
		{
			ECPWS_ERR_INFO("unmap file failed\n");
		}
		CloseHandle(e->hFileMap);
		CloseHandle(e->hFile);
#else
		munmap(e->mmap, e->len);
#endif
		mmap_list_entries_used--;
	}

}

#if 0
static struct MMAP_T *find_named_mmap(char *fname)
{
	int data_fd;
	struct stat statbuf;
	struct MMAP_T *e;
	data_fd = OPEN(fname, O_RDONLY);
	if (data_fd == -1) {
		perror(fname);
		return NULL;
	}
	
	fstat(data_fd, &statbuf);
	if (S_ISDIR(statbuf.st_mode)) {
		ECPWS_DEBUG_INFO( "%s is a directory\n", fname);
		return NULL;
	}

	e = find_mmap(data_fd, &statbuf);
	CLOSE(data_fd);
	return e;
}
#endif


#ifdef HAVE_SENDFILE
WEB_CLIENT_STATUS io_shuffle_sendfile(WEB_CLIENT_CONN * conn)
{
	int bytes_written;
	unsigned int bytes_to_write;

	if (conn->method == WEB_M_HEAD)
	{
		return complete_response(conn);
	}

	bytes_to_write = (conn->ranges->stop - conn->ranges->start) + 1;

	if (bytes_to_write > system_bufsize)
		bytes_to_write = system_bufsize;

retrysendfile:
	if (bytes_to_write == 0)
	{/* shouldn't get here, but... */
		bytes_written = 0;
	}
	else
	{
		bytes_written = sendfile(conn->socket, conn->data_fd, &(conn->filepos), bytes_to_write);
		if (bytes_written < 0)
		{
			if (errno == EWOULDBLOCK || errno == EAGAIN) {
				return -1;          /* request blocked at the pipe level, but keep going */
			}
			else if (errno == EINTR) {
				goto retrysendfile;
			}
			else
			{
				conn->status = WEB_CLIENT_S_ERROR;
				if (errno != EPIPE && errno != ECONNRESET)
				{
					WEB_CONN_ERROR(conn, WEB_RES_ERROR);
					ECPWS_LOG_REQ_ERR(conn, "sendfile write");
				}
			}
			return 0;
		} /* bytes_written */
	} /* bytes_to_write */

	/* sendfile automatically updates conn->filepos,
	* don't touch!
	* conn->filepos += bytes_written;
	*/
	conn->ranges->start += bytes_written;
	conn->bytes_written += bytes_written;

	if (conn->ranges->stop + 1 == conn->ranges->start) {
		return complete_response(conn);
	}
	
	return 1;
}
#endif


/* always try to read unless data_fs is 0 (and there is space) then try to write
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: EOF or error, close it down
 *   1: successful read, recycle in ready queue
 */
WEB_CLIENT_STATUS io_shuffle(WEB_CLIENT_CONN * conn)
{
	int bytes_to_read;
	int bytes_written, bytes_to_write;

	if (conn->method == WEB_M_HEAD) {
		return complete_response(conn);
	}

	/* FIXME: This function doesn't take into account conn->filesize
	* when *reading* into the buffer. Grr.
	*/
	bytes_to_read = WEB_BUFFER_SIZE - conn->buffer_end - 256;

	bytes_to_write = (conn->ranges->stop - conn->ranges->start) + 1;

	if (bytes_to_read > bytes_to_write)
		bytes_to_read = bytes_to_write;

	if (bytes_to_read > 0 && conn->data_fd)
	{
		int bytes_read;
		off_t temp;

		temp = LSEEK(conn->data_fd, conn->ranges->start, SEEK_SET);
		if (temp < 0)
		{
			conn->status = WEB_CLIENT_S_ERROR;
			ECPWS_LOG_REQ_ERR(conn, "ioshuffle lseek");
			return 0;
		}

restartread:
		bytes_read = READ(conn->data_fd, conn->buffer + conn->buffer_end, bytes_to_read);
		if (bytes_read == -1)
		{
			if (errno == EINTR)
				goto restartread;
			else if (errno == EWOULDBLOCK || errno == EAGAIN)
			{
				/* not a fatal error, don't worry about it */
				/* buffer is empty, we're blocking on read! */
				if (conn->buffer_end - conn->buffer_start == 0)
					return -1;
			}
			else
			{
				conn->status = WEB_CLIENT_S_ERROR;
				ECPWS_LOG_REQ_ERR(conn, "ioshuffle read");
				return 0;
			}
		}
		else if (bytes_read == 0)
		{ /* eof, write rest of buffer */
			CLOSE(conn->data_fd);
			conn->data_fd = 0;
		}
		else
		{
			conn->buffer_end += bytes_read;
			conn->ranges->start += bytes_read;

			if ((conn->ranges->stop + 1 - conn->ranges->start) == 0)
			{
				return complete_response(conn);
			}
		}
	}

	bytes_to_write = conn->buffer_end - conn->buffer_start;
	if (bytes_to_write == 0)
	{
		if (conn->data_fd == 0)
			return 0;           /* done */
		conn->buffer_end = conn->buffer_start = 0;
		return 1;
	}

//restartwrite:
	bytes_written = svcWebConnWrite(conn, conn->buffer + conn->buffer_start, bytes_to_write);
	if (bytes_written == -1)
	{
//		else if (errno == EINTR)
//			goto restartwrite;
		ECPWS_LOG_REQ_ERR(conn, "ioshuffle write");
		WEB_CONN_RES_ERR(conn, WEB_RES_ERROR);
	}

	conn->buffer_start += bytes_written;
	/* conn->filepos is not used here */
	conn->bytes_written += bytes_written;

	if (bytes_to_write == bytes_written) {
		conn->buffer_end = conn->buffer_start = 0;
	}

	return 1;
}



