/*
 *
 */

#ifndef __MUX_SVR_WEB_H__
#define __MUX_SVR_WEB_H__

#include <errno.h>
#include <stdlib.h>		/* malloc, free, etc. */
#include <stdio.h>			/* stdin, stdout, stderr */
#include <string.h>		/* strdup */

#include <sys/types.h>		/* socket, bind, accept */
#include <sys/stat.h>		/* open */
#include <time.h>			/* localtime, time */

#include <signal.h>		/* signal */
#include <setjmp.h>
#include <limits.h>             /* OPEN_MAX */

#include <stddef.h>             /* for offsetof */
#if WIN32
//#include <windows.h>
//#include <Winsock2.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>

#include <io.h>			/* _dup2  _chmod*/

#else
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>

#include <netinet/in.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/socket.h>         /* socket, bind, accept, setsockopt, */
#include <sys/wait.h>
#include <sys/stat.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>

#endif
#include <fcntl.h>
#include <stdarg.h>

#include "dirent.h"

#include "libCmn.h"

#include "muxSvrWebCompat.h"             /* oh what fun is porting */
#include "muxSvrWebDefines.h"
#include "muxSvrWebGlobals.h"



#endif

