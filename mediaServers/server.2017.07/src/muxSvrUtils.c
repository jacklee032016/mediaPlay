
#include "muxSvr.h"

static inline void _cpHtmlEntity (char *buffer, const char *entity)
{
	if (!buffer || !entity)
		return;
	while (*entity)
		*buffer++ = *entity++;
}
/**
 * Substitutes known conflicting chars on a text string with their corresponding HTML entities.
 *
 * Returns the number of bytes in the 'encoded' representation not including the terminating NUL.
 */
size_t htmlencode (const char *src, char **dest)
{
	const char *amp = "&amp;";
	const char *lt  = "&lt;";
	const char *gt  = "&gt;";
	const char *start;
	char *tmp;
	size_t final_size = 0;

	if (!src)
		return 0;

	start = src;

	/* Compute needed dest size */
	while (*src != '\0')
	{
		switch(*src)
		{
			case 38: /* & */
				final_size += 5;
				break;
				
			case 60: /* < */
			case 62: /* > */
				final_size += 4;
				break;
				
			default:
				final_size++;
		}
		src++;
	}

	src = start;
	*dest = av_mallocz(final_size + 1);
	if (!*dest)
		return 0;

	/* Build dest */
	tmp = *dest;
	while (*src != '\0')
	{
		switch(*src)
		{
			case 38: /* & */
				_cpHtmlEntity (tmp, amp);
				tmp += 5;
				break;
			
			case 60: /* < */
				_cpHtmlEntity (tmp, lt);
				tmp += 4;
				break;
			
			case 62: /* > */
				_cpHtmlEntity (tmp, gt);
				tmp += 4;
				break;
			
			default:
				*tmp = *src;
				tmp += 1;
		}
		src++;
	}
	*tmp = '\0';

	return final_size;
}

static FILE *_logfile = NULL;

static char *_ctime1(char *buf2, size_t buf_size)
{
	time_t ti;
	char *p;

	ti = time(NULL);
	p = ctime(&ti);

	if (!p || !*p)
	{
		*buf2 = '\0';
		return buf2;
	}
	
	av_strlcpy(buf2, p, buf_size);
	p = buf2 + strlen(buf2) - 1;
	if (*p == '\n')
		*p = '\0';
	return buf2;
}

static void _httpvLlog(const char *fmt, va_list vargs)
{
	static int print_prefix = 1;
	char buf[32];

	if (!_logfile)
		return;

	if (print_prefix)
	{
		_ctime1(buf, sizeof(buf));
		fprintf(_logfile, "%s ", buf);
	}

	print_prefix = strstr(fmt, "\n") != NULL;
	vfprintf(_logfile, fmt, vargs);
	fflush(_logfile);
}

void muxSvrLog(const char *fmt, ...)
{
	va_list vargs;
	va_start(vargs, fmt);
	_httpvLlog(fmt, vargs);
	va_end(vargs);
}

static void _avLogCallback(void *ptr, int level, const char *fmt, va_list vargs)
{
	static int print_prefix = 1;
	AVClass *avc = ptr ? *(AVClass**)ptr : NULL;
	if (level > av_log_get_level())
		return;
	if (print_prefix && avc)
		muxSvrLog("[%s @ %p]", avc->item_name(ptr), ptr);
	print_prefix = strstr(fmt, "\n") != NULL;
	_httpvLlog(fmt, vargs);
}

void muxSvrInitLog(MUX_SVR *muxSvr)
{
	/* open log file if needed */
	if (muxSvr->config.logfilename[0] != '\0')
	{
		if (!strcmp(muxSvr->config.logfilename, "-"))
			_logfile = stdout;
		else
			_logfile = fopen(muxSvr->config.logfilename, "a");
		av_log_set_callback(_avLogCallback);
	}
}

void muxUtilGetWord(char *buf, int buf_size, const char **pp)
{
	const char *p;
	char *q;

#define SPACE_CHARS " \t\r\n"

	p = *pp;
	p += strspn(p, SPACE_CHARS);
	q = buf;
	while (!av_isspace(*p) && *p != '\0')
	{
		if ((q - buf) < buf_size - 1)
			*q++ = *p;
		p++;
	}
	
	if (buf_size > 0)
		*q = '\0';
	*pp = p;
}

void muxUtilGetArg(char *buf, int buf_size, const char **pp)
{
	const char *p;
	char *q;
	int quote = 0;

	p = *pp;
	q = buf;

	while (av_isspace(*p))
		p++;

	if (*p == '\"' || *p == '\'')
		quote = *p++;

	while (*p != '\0')
	{
		if ( (quote && *p == quote) ||( !quote && av_isspace(*p)))
			break;
		
		if ((q - buf) < buf_size - 1)
			*q++ = *p;
		p++;
	}

	*q = '\0';
	if (quote && *p == quote)
		p++;
	
	*pp = p;
}


void muxSvrUpdateDatarate(MUX_SVR *muxSvr, DataRateData *drd, int64_t count)
{
	if (!drd->time1 && !drd->count1)
	{
		drd->time1 = drd->time2 = muxSvr->currentTime;
		drd->count1 = drd->count2 = count;
	}
	else if (muxSvr->currentTime - drd->time2 > 5000)
	{
		drd->time1 = drd->time2;
		drd->count1 = drd->count2;
		drd->time2 = muxSvr->currentTime;
		drd->count2 = count;
	}
}

/* In bytes per second */
int muxSvrComputeDatarate(MUX_SVR *muxSvr, DataRateData *drd, int64_t count)
{
	if (muxSvr->currentTime == drd->time1)
		return 0;

	return ((count - drd->count1) * 1000) / (muxSvr->currentTime - drd->time1);
}

int64_t ffm_read_write_index(int fd)
{
	uint8_t buf[8];

	if (lseek(fd, 8, SEEK_SET) < 0)
		return AVERROR(EIO);
	if (read(fd, buf, 8) != 8)
		return AVERROR(EIO);
	return AV_RB64(buf);
}

int ffm_write_write_index(int fd, int64_t pos)
{
	uint8_t buf[8];
	int i;

	for(i=0;i<8;i++)
		buf[i] = (pos >> (56 - i * 8)) & 0xff;
	if (lseek(fd, 8, SEEK_SET) < 0)
		goto bail_eio;
	if (write(fd, buf, 8) != 8)
		goto bail_eio;

	return 8;

bail_eio:
	return AVERROR(EIO);
}

void ffm_set_write_index(AVFormatContext *s, int64_t pos, int64_t file_size)
{
	av_opt_set_int(s, "server_attached", 1, AV_OPT_SEARCH_CHILDREN);
	av_opt_set_int(s, "ffm_write_index", pos, AV_OPT_SEARCH_CHILDREN);
	av_opt_set_int(s, "ffm_file_size", file_size, AV_OPT_SEARCH_CHILDREN);
}

void unlayer_stream(AVStream *st, LayeredAVStream *lst)
{
	avcodec_free_context(&st->codec);
	avcodec_parameters_free(&st->codecpar);
#define COPY(a) st->a = lst->a;
	COPY(index)
	COPY(id)
	COPY(codec)
	COPY(codecpar)
	COPY(time_base)
	COPY(pts_wrap_bits)
	COPY(sample_aspect_ratio)
	COPY(recommended_encoder_configuration)
}


/* FIXME: make ffserver work with IPv6 */
/* resolve host with also IP address parsing */
int muxUtilResolveHost(struct in_addr *sin_addr, const char *hostname)
{
	if (!ff_inet_aton(hostname, sin_addr))
	{
#if HAVE_GETADDRINFO
		struct addrinfo *ai, *cur;
		struct addrinfo hints = { 0 };
		hints.ai_family = AF_INET;

		if (getaddrinfo(hostname, NULL, &hints, &ai))
			return -1;
		/* getaddrinfo returns a linked list of addrinfo structs.
		* Even if we set ai_family = AF_INET above, make sure
		* that the returned one actually is of the correct type. */
		for (cur = ai; cur; cur = cur->ai_next)
		{
			if (cur->ai_family == AF_INET)
			{
				*sin_addr = ((struct sockaddr_in *)cur->ai_addr)->sin_addr;
				freeaddrinfo(ai);
				return 0;
			}
		}
		freeaddrinfo(ai);
		return -1;
#else
		struct hostent *hp;
		hp = gethostbyname(hostname);
		if (!hp)
			return -1;
		
		memcpy(sin_addr, hp->h_addr_list[0], sizeof(struct in_addr));
#endif
	}
	return 0;
}

static FFServerIPAddressACL* _parseDynamicAcl(MuxStream *stream, MuxContext *c)
{
	FILE* f;
	char line[1024];
	char  cmd[1024];
	FFServerIPAddressACL *acl = NULL;
	int line_num = 0;
	const char *p;

	f = fopen(stream->dynamic_acl, "r");
	if (!f)
	{
		perror(stream->dynamic_acl);
		return NULL;
	}

	acl = av_mallocz(sizeof(FFServerIPAddressACL));
	if (!acl)
	{
		fclose(f);
		return NULL;
	}

	/* Build ACL */
	while (fgets(line, sizeof(line), f))
	{
		line_num++;
		p = line;
		while (av_isspace(*p))
			p++;
		
		if (*p == '\0' || *p == '#')
			continue;
		muxUtilGetArg(cmd, sizeof(cmd), &p);

		if (!av_strcasecmp(cmd, "ACL"))
			muxSvrParseAclRow(NULL, NULL, acl, p, stream->dynamic_acl, line_num);
	}
	
	fclose(f);
	return acl;
}


static void _freeAclList(FFServerIPAddressACL *in_acl)
{
	FFServerIPAddressACL *pacl, *pacl2;

	pacl = in_acl;
	while(pacl)
	{
		pacl2 = pacl;
		pacl = pacl->next;
		av_freep(pacl2);
	}
}

static int _validateAclList(FFServerIPAddressACL *in_acl, MuxContext *c)
{
	enum FFServerIPAddressAction last_action = IP_DENY;
	FFServerIPAddressACL *acl;
	struct in_addr *src = &c->from_addr.sin_addr;
	unsigned long src_addr = src->s_addr;

	for (acl = in_acl; acl; acl = acl->next)
	{
		if (src_addr >= acl->first.s_addr && src_addr <= acl->last.s_addr)
			return (acl->action == IP_ALLOW) ? 1 : 0;
		last_action = acl->action;
	}

	/* Nothing matched, so return not the last action */
	return (last_action == IP_DENY) ? 1 : 0;
}

int muxSvrValidateAcl(MuxStream *stream, MuxContext *c)
{
	int ret = 0;
	FFServerIPAddressACL *acl;

	/* if stream->acl is null validate_acl_list will return 1 */
	ret = _validateAclList(stream->acl, c);

	if (stream->dynamic_acl[0])
	{
		acl = _parseDynamicAcl(stream, c);
		ret = _validateAclList(acl, c);
		_freeAclList(acl);
	}

	return ret;
}


void muxSvrParseAclRow(MuxStream *stream, MuxStream* feed, FFServerIPAddressACL *ext_acl, const char *p, const char *filename, int line_num)
{
	char arg[1024];
	FFServerIPAddressACL acl;
	FFServerIPAddressACL *nacl;
	FFServerIPAddressACL **naclp;

	muxUtilGetArg(arg, sizeof(arg), &p);
	if (av_strcasecmp(arg, "allow") == 0)
		acl.action = IP_ALLOW;
	else if (av_strcasecmp(arg, "deny") == 0)
		acl.action = IP_DENY;
	else
	{
		fprintf(stderr, "%s:%d: ACL action '%s' should be ALLOW or DENY.\n", filename, line_num, arg);
		goto bail;
	}

	muxUtilGetArg(arg, sizeof(arg), &p);

	if (muxUtilResolveHost(&acl.first, arg))
	{
		fprintf(stderr,	"%s:%d: ACL refers to invalid host or IP address '%s'\n", filename, line_num, arg);
		goto bail;
	}

	acl.last = acl.first;

	muxUtilGetArg(arg, sizeof(arg), &p);

	if (arg[0])
	{
		if (muxUtilResolveHost(&acl.last, arg))
		{
			fprintf(stderr,	"%s:%d: ACL refers to invalid host or IP address '%s'\n", filename, line_num, arg);
			goto bail;
		}
	}

	nacl = av_mallocz(sizeof(*nacl));
	if (!nacl)
	{
		fprintf(stderr, "Failed to allocate FFServerIPAddressACL\n");
		goto bail;
	}

	naclp = 0;

	acl.next = 0;
	*nacl = acl;

	if (stream)
		naclp = &stream->acl;
	else if (feed)
		naclp = &feed->acl;
	else if (ext_acl)
		naclp = &ext_acl;
	else
		fprintf(stderr, "%s:%d: ACL found not in <Stream> or <Feed>\n", filename, line_num);

	if (naclp)
	{
		while (*naclp)
		naclp = &(*naclp)->next;

		*naclp = nacl;
	}
	else
		av_free(nacl);

bail:
	return;
}

static const char * const _httpStates[] = {
    "HTTP_WAIT_REQUEST",
    "HTTP_SEND_HEADER",

    "SEND_DATA_HEADER",
    "SEND_DATA",
    "SEND_DATA_TRAILER",
    "RECEIVE_DATA",
    "WAIT_FEED",
    "READY",

    "RTSP_WAIT_REQUEST",
    "RTSP_SEND_REPLY",
    "RTSP_SEND_PACKET",
};


const char *muxSvrStatusName(int status)
{
	if(status < MUX_ARRAY_ELEMS(_httpStates))
		return _httpStates[status];

	return "Unknown";
}

