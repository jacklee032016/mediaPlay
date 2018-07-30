/*
 *
 */


#ifndef __MUX_SVR_WEB_COMPAT_H__
#define __MUX_SVR_WEB_COMPAT_H__

#define	WITH_JUMP		0

#ifdef _WIN32
#include <direct.h>
/*
* __declspec(dllexport) must be before return type of C function, 
and it must be after the class keyword in C++ class
*/
#ifdef DLL_LIBRARY_EXPORT
#define	DLL_DECLARE	__declspec(dllexport) //	__stdcall
#else
#define	DLL_DECLARE	__declspec(dllimport) //	__stdcall
#endif

#ifdef __cplusplus
	/* export the names in C++ as C */
	#define STD_CAPI	extern "C" 
#else
	#define STD_CAPI
#endif

#define	UID_T			unsigned int
#define	GID_T			unsigned int
#define	GETUID()		0
#define	GETPID			_getpid

#define	STDERR_FILENO 		_fileno( stderr )
#define	STDOUT_FILENO 		_fileno( stdout )
#define	STDIN_FILENO 		_fileno( stdin )

#define	JMP_BUFFER		jmp_buf

#define	DUP2			_dup2
#define	PIPE(fdSet)		_pipe(fdSet, 512, O_NOINHERIT)

#define	UNLINK			_unlink
#define	CHDIR			_chdir
#define 	CHMOD			_chmod
#define	UMASK			_umask		
#define	OPEN			_open
#define	CLOSE			_close
#define	LSEEK			_lseek
#define	READ			_read
#define	WRITE			_write

//#define	SSCANF			sscanf_s
#define	SSCANF			sscanf

#if 0
#define	GM_TIME(tm, time)		\
		{if(gmtime_s((tm), (time)) ) {ECPWS_ERR_INFO("gmtime_s"); /*exit(1);*/}}
#endif

#define	GM_TIME(tm, time)		{(tm) = gmtime((time));}

#define	F_OPEN(file, name, flags)		\
		{if(fopen_s((file), (name), (flags)) ) {ECPWS_ERR_INFO("fopen_s"); exit(1);}}

#define	FILENO			_fileno
#define	STRNCASECMP	_strnicmp
#define	STRCASECMP		_stricmp

#define	SPRINTF(dest, size, ...)		sprintf_s((dest), (size), ##__VA_ARGS__)

#define	SNPRINTF(dest, size, ...)		_snprintf_s(dest, size,  _TRUNCATE, ##__VA_ARGS__)
#define	STRCPY(dest, size, src)		strcpy_s((dest), (size), (src) )

#define	STRDUP			_strdup

#define	S_ISDIR(mode)	(mode & _S_IFDIR )
#define	S_IS_FILE(mode)	(mode & _S_IFREG)
#define	S_ISREG(mode)		S_IS_FILE(mode)
#define	CloseSocket(s)		closesocket(s)
#define	SEND(socket, buf, size)	send(socket, buf, size, 0)
#define	RECV(socket, buf, size)	recv(socket, buf, size, 0)

#else	/* else WIN32 */
#define	DLL_DECLARE

#define	UID_T			uid_t
#define	GID_T			gid_t
#define	GETUID()		getuid()
#define	GETPID			getpid

#define	JMP_BUFFER		sigjmp_buf /* ?? should be jmp_buf */

#define	DUP2			dup2
#define	PIPE(fdSet)		pipe(fdSet)

#define	UNLINK			unlink
#define	CHDIR			chdir
#define 	CHMOD			chmod
#define	UMASK			umask		
#define	OPEN			open
#define	CLOSE			close
#define	LSEEK			lseek
#define	READ			read
#define	WRITE			write

#define	SSCANF			sscanf

#define	GM_TIME(tm, time)		{(tm) = gmtime((time));}

#define	FILENO			fileno
#define	STRNCASECMP	strncasecmp
#define	STRCASECMP		strcasecmp

#define	SPRINTF(dest, size, ...)		sprintf((dest),##__VA_ARGS__)

#define	SNPRINTF(dest, size,...)		snprintf((dest), (size), ##__VA_ARGS__)
#define	STRCPY(dest, size, src)		strcpy((dest), (src) )

#define	STRDUP			strdup

 /* regular file | u+rx | g+rx */
#define	S_IS_FILE(mode)		(mode & (S_IFREG |(S_IRUSR | S_IXUSR) | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH)) )

#define	CloseSocket(s)	close(s)
#define	SEND(socket, buf, size)	write(socket, buf, size)
#define	RECV(socket, buf, size)	read(socket, buf, size)
#define	INVALID_SOCKET			-1
#define	SOCKET_ERROR			-1

/* Wild guess time, probably better done with configure */
#ifdef O_NONBLOCK
#define NOBLOCK		O_NONBLOCK      /* Linux */
#else                           /* O_NONBLOCK */
#ifdef O_NDELAY
#define NOBLOCK		O_NDELAY        /* Sun */
#else                           /* O_NDELAY */
#error "Can't find a way to #define NOBLOCK"
#endif                          /* O_NDELAY */
#endif                          /* O_NONBLOCK */

#ifndef MAP_FILE
#define MAP_OPTIONS	MAP_PRIVATE /* Sun */
#else
#define MAP_OPTIONS	MAP_FILE|MAP_PRIVATE /* Linux */
#endif

#if 1//def HAVE_TM_GMTOFF
#define TIMEZONE_OFFSET(foo)	foo->tm_gmtoff
#else
#define TIMEZONE_OFFSET(foo)	timezone
#endif

#if __GNUC__
#if __x86_64__ || __ppc64__
#define	ENV_64
#define	POINTRT_TO_INTEGER	long int
#else
#define	ENV_32
#define	POINTRT_TO_INTEGER	int
#endif
#endif



#endif


#ifdef INET6
    typedef struct sockaddr_in6 SOCKADDR_IN_T;
    #define AF_INET_V    AF_INET6
#else
    typedef struct sockaddr_in  SOCKADDR_IN_T;
    #define AF_INET_V    AF_INET
#endif

#ifdef INET6
/* #define S_FAMILY __s_family */
#define SOCKADDR sockaddr_storage
#define SERVER_PF PF_INET6
#define S_FAMILY sin6_family
#define BOA_NI_MAXHOST NI_MAXHOST
#else /* ifdef INET6 */
#define SOCKADDR sockaddr_in
#define SERVER_PF PF_INET
#define S_FAMILY sin_family
#endif /* ifdef INET6 */

#endif

