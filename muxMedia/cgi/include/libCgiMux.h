
#ifndef	__LIB_CGI_MUX_H__
#define	__LIB_CGI_MUX_H__

#include "libCmn.h"
#include "libMedia.h"
#include "libMux.h"

#include "cmnMuxApi.h"

#define	CMN_LOG_FILE_WEBADMIN	"/tmp/cgi.log"

/*
 Common data structure for all CGI program
*/
typedef	struct
{
	llist						cgiVariables;

	MuxMain					cgiMain;
	MuxPlayerConfig			cgiCfgPlay;

	CmnMuxRecordConfig		cgiCfgRecord;
	CmnMuxWebConfig			cgiCfgWeb;

//	CMG_MQ_T			mqCtx;
	cJSON				*response;
}MUX_WEB_CMN;

int cgiMuxInit(MUX_WEB_CMN *muxWeb);


#endif

