/*
 * $Id$
*/

#ifndef __LIB_COMMON_H__
#define __LIB_COMMON_H__

#ifdef __cplusplus
	extern "C"
	{
#endif

#ifdef ARM
//#warning		"build for ARM platform"
#elif  X86
//#warning		"build for X86 platform"
#else
#error	"unknowm platform, build stopped"
#endif


/* build options for all modules */
#define	CMN_SHARED_DEBUG						1

#define	CMN_TIMER_DEBUG						0

/* timer thread start up after daemonized */
#define	CMN_TIMER_START_DELAY				1

#define	CMN_THREAD_DEBUG						0

#define	DEBUG_CONFIG_FILE						0

#define	MUX_THREAD_SUPPORT_DYNAMIC			1


#define	MUX_OPTIONS_DEBUG_CAPTURE			0
#define	MUX_OPTIONS_DEBUG_IP_COMMAND		0

#define	DEBUG_FFMPEG							0


/* build options for player */
#define	WITH_HIPLAYER_STATE_CHECK			0

#define	WITH_HIGO_DEBUG						0

/* some event only for info alert, no handle is needed, so these events can be ignored in FSM */
#define	PLAYER_ENABLE_INFO_EVENT				0

#define	PLAYER_DEBUG_HIPLAY_EVENT			0

#define	PLAYER_DEBUG_HIPLAY_LOG				0	/* HiPlayer log : max log level */

#define	PLAYER_ENABLE_SUBTITLE				0

#define	MUX_WITH_IMAGE_PLAY					1

/* send background parameter of API as a string in form of '0xFFRRGGBB' */
#define	BACKGROUND_AS_STRING					1

#define	OSD_ENABLE_DOUBLE_BUFFER			0

/* playlist which is added is only saved in testing. In release version, no any playlist is saved in configuration file */
#define	MUX_SAVE_PLAYLIST						0

/* Macros for constants */

#define	CMN_NAME_LENGTH			256


// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define	ENV_64
#else
#define	ENV_32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define	ENV_64
#define	POINTRT_TO_INTEGER	long int
#else
#define	ENV_32
#define	POINTRT_TO_INTEGER	int
#endif
#endif

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "pthread.h"


#define	DEBUG_RAW_RW						1

#define CMN_OK 								0
#define CMN_FAIL 							-1

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

#ifndef	false
#define	false								0
#endif

#ifndef	true
#define	true									(!false)
#endif

#ifndef	OK
#define	OK									0
#endif

#ifndef	FAIL
#define	FAIL								-1
#endif


#define EXIT_FAILURE 			1
#define EXIT_SUCCESS 			0


#define KB(x)						((x) / 1024)
#define PERCENTAGE(x,total)		(((x) * 100) / (total))

#define	UNIT_OF_KILO					1024
#define	UNIT_OF_MEGA					(UNIT_OF_KILO*UNIT_OF_KILO)
#define	UNIT_OF_GIGA					(UNIT_OF_KILO*UNIT_OF_MEGA)



#define	CMNLIB_OPS_DEBUG						0x01
#define	CMNLIB_OPS_OTHER						0x02



/* define board, libMedia uses it, so defined here */
typedef	enum _MUX_BOARD_TYPE
{
	MUX_BOARD_TYPE_RX769 = 0,
	MUX_BOARD_TYPE_RX762,
	MUX_BOARD_TYPE_RX,
	MUX_BOARD_TYPE_UNKNOWN
}MUX_BOARD_TYPE;


typedef	enum _MEDIA_DEVICE_TYPE
{
	MEDIA_DEVICE_USBDISK = 0, 		/* USB disk is default device */
	MEDIA_DEVICE_SDCARD,
	MEDIA_DEVICE_FTP,
	MEDIA_DEVICE_UNKNOWN
}MEDIA_DEVICE_T;

/* also used as the stream index of capture media and capture feed s*/
typedef	enum _MUX_MEDIA_TYPE
{
	MUX_MEDIA_TYPE_VIDEO = 0,
	MUX_MEDIA_TYPE_AUDIO,
	MUX_MEDIA_TYPE_SUBTITLE,
	MUX_MEDIA_TYPE_ERROR,
}MUX_MEDIA_TYPE;


/*
* ESC (27, 0x1b) charactor is '\e' or '\x1b'
*/



#define ANSI_COLOR_RED				"\x1b[31m"	/* ESC[31m : red */
#define ANSI_COLOR_GREEN			"\x1b[32m"
#define ANSI_COLOR_YELLOW			"\x1b[33m"
#define ANSI_COLOR_BLUE			"\x1b[34m"
#define ANSI_COLOR_MAGENTA		"\x1b[35m"
#define ANSI_COLOR_CYAN			"\x1b[36m"
#define ANSI_COLOR_RESET			"\x1b[0m"


#define	ERROR_TEXT_BEGIN			"\t\e[31m ERR:"
#define	ERROR_TEXT_END			"\e[0m"


#define	WARN_TEXT_BEGIN			""ANSI_COLOR_MAGENTA"WARN:"

#define	INFO_TEXT_BEGIN			""ANSI_COLOR_BLUE"INFO:"



#define		STR_BOOL_VALUE(bValue)	\
			(((bValue)==0)?"NO":"YES")


#if	ARCH_ARM
#define	CONFIG_FILE_HOME_PROJECT			"/etc/mLab/"
#else
//#error	"root dir is " ROOT_DIR
#define	CONFIG_FILE_HOME_PROJECT			ROOT_DIR"/../muxMedia/releases/etc/mLab/"
#endif


#define ADD_ELEMENT(header, element)	\
	if (header == NULL){					\
		header  = element;				\
		element->next   = NULL;			\
	}									\
	else	{								\
		element->next   = header;		\
		header = element;				\
	}


#define REMOVE_ELEMENT(header, element, type)	\
{type **cp, *c1; cp = &header; \
	while ((*cp) != NULL){  c1 = *cp; \
		if (c1 == element ) \
		{ *cp = element->next;} \
		else	{ cp = &c1->next;} \
	}; }


#define	GET_BIT(value, bit)				(((value)>>(bit))&0x01)
#define	SET_BIT(value, bit)				((value) << (bit))



#define	CMN_SET_BIT(flags, bitPosition)	\
		flags |= SET_BIT(0x01, (bitPosition) ) 

#define	CMN_CLEAR_BIT(flags, bitPosition)	\
		flags &= ~(SET_BIT(0x01, (bitPosition) ) )	

#define	CMN_CHECK_BIT(flags, bitPosition)	\
		( (flags&SET_BIT(0x01,(bitPosition) ) )!=0 )

#define	CMN_SET_FLAGS(flags, value)	\
		flags |= (value) 

#define	CMN_CLEAR_FLAGS(flags, value)	\
		flags &= ~((value) ) )	

#define	CMN_CHECK_FLAGS(flags, value)	\
		( (flags&(value) )!=0 )


#include "cmnLogUtils.h"

int cmn_log_init(log_stru_t *lobj);


#if CMN_SHARED_DEBUG
	#ifndef	TRACE
	#define	TRACE()	CMN_MSG_LOG(CMN_LOG_DEBUG,"" )
	#endif
#else
	#ifndef	TRACE
	#define	TRACE()	do{}while(0)
	#endif
#endif


#define	FILE_TO_STDOUT			0

#ifndef   __CMN_RELEASE__
#if FILE_TO_STDOUT
#define	CMN_MSG_LOG(level, format ,...)   \
	do{ char buf[4096]; snprintf(buf, sizeof(buf), __FILE__".%d|%s\n", __LINE__, format ) ;\
		printf(buf, ##__VA_ARGS__);			\
		}while(0)
#else
#define	CMN_MSG_LOG(level, format ,...)   \
	do{ 					\
		if ( level <= CMN_LOG_DEBUG && get_current_level() >= level) \
			log_information(level, __FILE__, __LINE__, format, ##__VA_ARGS__);	\
	}while(0)
#endif
#else
#define	CMN_MSG_LOG(level, format ,...) \
	do{ 					\
		if ( level <= CMN_LOG_DEBUG && get_current_level() >= level) \
			log_information(level, format, ##__VA_ARGS__);	\
	}while(0)
#endif


#define	CMN_ABORT(format ,...)		\
			{CMN_MSG_LOG(CMN_LOG_ERR, format, ##__VA_ARGS__); CMN_MSG_LOG(CMN_LOG_ERR,"ABORT Now!!!");abort();}

#ifndef   __CMN_RELEASE__
#if FILE_TO_STDOUT
#define	CMN_MSG_DEBUG(level, format ,...) 	 \
	do{ char buf[4096]; snprintf(buf, sizeof(buf), __FILE__".%d|%s\n", __LINE__, format ) ;\
		printf(buf, ##__VA_ARGS__);			\
		}while(0)

	//printf(format, ##__VA_ARGS__)
#else
#define	CMN_MSG_DEBUG(level, format ,...) 	\
	do{ 					\
		if ( level <= CMN_LOG_DEBUG && get_current_level() >= level) \
			log_information(level, __FILE__, __LINE__, format, ##__VA_ARGS__);	\
	}while(0)
#endif

#else
#define	CMN_MSG_DEBUG(level, format ,...) 	{}
#endif


#define	IS_STRING_NULL( str)	\
	(((str)==NULL)||(!strlen((str))) )


#define	IS_STRING_EQUAL( str1, str2)	\
	( !strcasecmp((str1), (str2)))


int safe_open (const char *pathname,int flags);
void safe_rewind (int fd, int offset, const char *filename);
void safe_read (int fd,const char *filename,void *buf, size_t count, int verbose);


/* time for warm-reset, when both set to 0, it set to maxium: about 64 seconds */
int cmn_watchdog_enable(int seconds, int millseconds);
int cmn_watchdog_disable();
int cmn_watchdog_update();
void cmn_beep_long(int times);
void cmn_beep_short(int times);
int cmn_get_button_states(void);
int cmn_ver_log( char *modulename, char *info);
int cmn_ver_opt (int argc, char **argv, char *info);
int cmn_ver_debug( char *verInfo);

int cmn_write_serial_no (unsigned char *serNO);
int cmn_read_serial_no(unsigned char *read_serial_no, int length);

#define FLASH_SERIAL_OFFSET 			131072L		/* offset is 128 K bytes */
#define FLASH_SERIAL_DEVICE			"/dev/mtd0"	

/* cmd-line flags */
#define FLAG_NONE			0x00
#define FLAG_VERBOSE		0x01
#define FLAG_HELP			0x02
#define FLAG_FILENAME		0x04
#define FLAG_DEVICE			0x08

/* 6 blocks and 32K per block, total 192 K for bootloader partition in FLASH mtd 
* First 3 blocks are occupied by bootloader
*/
#define LOCK_START 			0
#define LOCK_SECTORS 		3


/* size of read/write buffer */
#define BUFSIZE 				(10 * 1024)
#define SERIAL_BUFSIZE 		20


#define	CMN_FLASH_MAGIC_START			0xAB
#define	CMN_FLASH_MAGIC_END			0xCD
#define	CMN_FLASH_MAGIC_LENGTH		2

#define	CMN_FLASH_ITEM_LENGTH			32
typedef	struct
{
	char		magicStart[CMN_FLASH_MAGIC_LENGTH];
	
	char		serialNo[CMN_FLASH_ITEM_LENGTH];
	char		userName[CMN_FLASH_ITEM_LENGTH];
	char		passwd[CMN_FLASH_ITEM_LENGTH];

	char		wanMac[CMN_FLASH_ITEM_LENGTH];
	char		wanIp[CMN_FLASH_ITEM_LENGTH];
	char		wanMask[CMN_FLASH_ITEM_LENGTH];

	char		lanMac[CMN_FLASH_ITEM_LENGTH];
	char		lanIp[CMN_FLASH_ITEM_LENGTH];
	char		lanMask[CMN_FLASH_ITEM_LENGTH];

	char		magicEnd[CMN_FLASH_MAGIC_LENGTH];
}CMN_FLASH_INFO_T;

extern	CMN_FLASH_INFO_T		factoryDefault;

int	cmn_default_write(CMN_FLASH_INFO_T *defaultInfo);
int	cmn_default_read(CMN_FLASH_INFO_T *defaultInfo);
void cmn_default_info_debug(CMN_FLASH_INFO_T *info);


#include "md5.h"
#include "base64.h"

#include <cmnOsPort.h>


/* return : 0 : success */
int	cmnLibInit(int	options);
void cmnLibDestroy(void);


#ifdef   __CMN_RELEASE__
#define	CMN_SHARED_INIT()	\
{ int options = 0;	\
	cmnLibInit(options);}
#else
#define	CMN_SHARED_INIT()	\
{ int options = 0;	\
	options |= CMNLIB_OPS_DEBUG;	\
	cmnLibInit(options);}
#endif	

int64_t cmnGetTimeFromStartup(void);

long cmnGetTimeMs(void);
char *cmnGetCtime1(char *buf2, size_t buf_size);

void cmnHexDump(const uint8_t *buf, int size);

int	cmnParseGetHexIntValue(char *hexString);

unsigned int cmnMuxCRC32b(void *message, int len);

#include "cmnList.h"


#include "cJSON.h"

char* cmnGetStrFromJsonObject(cJSON* json, const char * key);
int cmnGetIntegerFromJsonObject(cJSON* json, const char * key);



#include "cmnFtpLib.h"

typedef	struct CMN_FTP_CLIENT
{
	char		username[CMN_NAME_LENGTH];
	char		password[CMN_NAME_LENGTH];

	char		host[CMN_NAME_LENGTH];	/* IP address or domain name */
	int		port;

	char		remotePath[CMN_NAME_LENGTH];
	char		filename[CMN_NAME_LENGTH];
	
	cmn_list_t	*downloadFileList;

	cmn_list_t	downloadResults;


	fsz_t			fsz;
	netbuf		*ctrlConn;

	void 		*priv;
}CMN_FTP_CLIENT;


int cmnMuxFtpAddEvent(char *server, 	char *username, char *password, char *path, cmn_list_t *list, void *dataConn);


void cmnForkCommand(const char *cmd);
void cmnKillProcess(char * processName);

/* parse configuration file */
void cmnParseGetArg(char *buf, int buf_size, const char **pp);
int cmnParseGetIntValue( const char **pp);
int cmnParseGetBoolValue( const char **pp);
int cmnParseGetIpAddress(struct in_addr *ipAddress, const char *p);


#define  MUX_DEBUG(...)		{CMN_MSG_DEBUG(CMN_LOG_DEBUG, __VA_ARGS__);}

#define  MUX_INFO(...)		{CMN_MSG_LOG(CMN_LOG_INFO, __VA_ARGS__);}

#define  MUX_WARN(...)		{CMN_MSG_LOG(CMN_LOG_WARNING, __VA_ARGS__);}

#define  MUX_ERROR(...)		{CMN_MSG_LOG(CMN_LOG_ERR, __VA_ARGS__);}



extern volatile int recvSigalTerminal;


#define	SYSTEM_SIGNAL_EXIT() \
		{ recvSigalTerminal = TRUE; \
		}

#define	SYSTEM_IS_EXITING() \
		(recvSigalTerminal == TRUE)


#ifdef __cplusplus
};
#endif

#endif

