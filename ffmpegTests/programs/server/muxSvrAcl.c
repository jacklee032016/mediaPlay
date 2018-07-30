static FFServerIPAddressACL* parse_dynamic_acl(FFServerStream *stream, MuxContext *c)
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
		ffserver_get_arg(cmd, sizeof(cmd), &p);

		if (!av_strcasecmp(cmd, "ACL"))
			ffserver_parse_acl_row(NULL, NULL, acl, p, stream->dynamic_acl, line_num);
	}
	
	fclose(f);
	return acl;
}


static void free_acl_list(FFServerIPAddressACL *in_acl)
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

static int validate_acl_list(FFServerIPAddressACL *in_acl, MuxContext *c)
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

static int validate_acl(FFServerStream *stream, MuxContext *c)
{
	int ret = 0;
	FFServerIPAddressACL *acl;

	/* if stream->acl is null validate_acl_list will return 1 */
	ret = validate_acl_list(stream->acl, c);

	if (stream->dynamic_acl[0])
	{
		acl = parse_dynamic_acl(stream, c);
		ret = validate_acl_list(acl, c);
		free_acl_list(acl);
	}

	return ret;
}


