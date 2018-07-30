/*
 * Config.h is not the config.h
 * $Author: lizhijie $
 * $Log: config.h,v $
 * Revision 1.1.1.1  2006/11/29 17:51:53  lizhijie
 * CommonProgram Boa
 *
 * Revision 1.1.1.1  2006/07/10 08:53:53  lizhijie
 * rebuild BOA
 *
 * Revision 1.1.1.1  2004/10/26 07:04:42  lizhijie
 * re-import boa into repository
 *
 * Revision 1.1  2004/10/23 04:11:26  lizhijie
 * readd it in CVS repository
 *
 * Revision 1.1  2004/10/23 04:01:30  lizhijie
 * rename config.h as Config.h
 *
 * $Revision: 1.1.1.1 $
 */


#ifndef _CONFIG_GRP_H
#define _CONFIG_GRP_H

/*
 * Define GR_SCALE_DYNAMIC if you want grp to dynamically scale its read buffer
 * so that lines of any length can be used.  On very very small systems,
 * you may want to leave this undefined becasue it will make the grp functions
 * somewhat larger (because of the inclusion of malloc and the code necessary).
 * On larger systems, you will want to define this, because grp will _not_
 * deal with long lines gracefully (they will be skipped).
 */
#undef GR_SCALE_DYNAMIC

#ifndef GR_SCALE_DYNAMIC
/*
 * If scaling is not dynamic, the buffers will be statically allocated, and
 * maximums must be chosen.  GR_MAX_LINE_LEN is the maximum number of
 * characters per line in the group file.  GR_MAX_MEMBERS is the maximum
 * number of members of any given group.
 */
#define GR_MAX_LINE_LEN 128
/* GR_MAX_MEMBERS = (GR_MAX_LINE_LEN-(24+3+6))/9 */
#define GR_MAX_MEMBERS 11

#endif /* !GR_SCALE_DYNAMIC */


/*
 * Define GR_DYNAMIC_GROUP_LIST to make initgroups() dynamically allocate
 * space for it's GID array before calling setgroups().  This is probably
 * unnecessary scalage, so it's undefined by default.
 */
#undef GR_DYNAMIC_GROUP_LIST

#ifndef GR_DYNAMIC_GROUP_LIST
/*
 * GR_MAX_GROUPS is the size of the static array initgroups() uses for
 * its static GID array if GR_DYNAMIC_GROUP_LIST isn't defined.
 */
#define GR_MAX_GROUPS 64

#endif /* !GR_DYNAMIC_GROUP_LIST */

#endif /* !_CONFIG_GRP_H */
