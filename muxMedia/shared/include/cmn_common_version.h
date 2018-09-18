/*
 * $Author: lizhijie $
 * $Id: cmn_common_version.h,v 1.4 2007/04/25 09:10:44 lizhijie Exp $
*/
#ifndef __CMN_COMMON_VERSION_H__
#define __CMN_COMMON_VERSION_H__

#define CMN_SETUP_MODULE_NAME				"setdeft"
#define CMN_STC_ISP_MODULE_NAME				"isp"
#define CMN_MONITOR_MODULE_NAME				"mntrd"

#define CMN_VERSION_LOGPATH					"/var/ver"

#if 0
#define CMN_VERSION_INFO(moduleName)	 "%s (version: %s.%s.%s-%s %s %s)\r\n", \
			moduleName, CMN_VERSION_MAJOR,CMN_VERSION_MINOR, \
			CMN_VERSION_RELESNXE, CMN_VERSION_MODIFY, __DATE__, __TIME__  
#endif

#define CMN_VERSION_INFO(moduleName)	  "" \
			CMN_VERSION_MAJOR"." CMN_VERSION_MINOR"."	CMN_VERSION_RELEASE \
			"-" CMN_VERSION_MODIFY", " __DATE__ " " __TIME__ ""  


#if  __KERNEL__
	#define CMN_VERSION_DEBUG					printk
#else
	#define CMN_VERSION_DEBUG  					printf
	#define CMN_VERSION_LOG  					fprintf
		
	//int ver_log(int argc, char **argv) ;
	//int ver_opt (int argc, char **argv) ;
#endif

#endif

