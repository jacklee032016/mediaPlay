/*
* $Id: sysInfoTime.c,v 1.6 2007/09/06 09:19:59 lizhijie Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

#include "libCgi.h"


#define	SYS_TIMEZONE_SHANGHAI			"Asia/Shanghai"
#define	SYS_TIMEZONE_TOKYO				"Asia/Tokyo"
#define	SYS_TIMEZONE_NEWYORK			"America/New_York"
#define	SYS_TIMEZONE_TORONTO				"America/Toronto" 

#define	SYS_TIMEMODE						"SYS_TIME_MODE"
#define	SYS_TIMEMODE_NTP					"NTP"
#define	SYS_TIMEMODE_HW					"HW"		/* hardware, ep. RTC 1322 */

#define	SYS_NTP_SERVER					"NTP_SERVER"

#define	SYSTIME_OP_SAVE_MODE					"mode"
#define	SYSTIME_OP_SAVE_NTP_ADDRESS			"NTP"
#define	SYSTIME_OP_SAVE_HW_TIME				"HW"

#define	SYSTIME_VALUE_YEAR					"YEAR"
#define	SYSTIME_VALUE_MONTH					"MONTH"
#define	SYSTIME_VALUE_DAY						"DAY"
#define	SYSTIME_VALUE_HOUR					"HOUR"
#define	SYSTIME_VALUE_MINUTE					"MINUTE"
#define	SYSTIME_VALUE_SECOND					"SECOND"



#define	NTP_SERVER_CHINA						"210.72.145.44"
#define	NTP_SERVER_JAPAN						"133.100.9.2"
#define	NTP_SERVER_AMERICA					"210.72.145.44"
#define	NTP_SERVER_CANADA					"132.163.4.101"

static llist  	sysTimeCfgs;
static current_time_value_t *currentValue;

static char *__systime_status_list( )
{
	char buf[4096];
	int 	length = 0;
	char	*zoneInfo;
	char	zoneStr[128];
	
	length += CGI_SPRINTF(buf,length, "<tr><td align=\"right\" bgcolor=\"#cccccc\"><strong>%s:</strong></TD>\n", 
		gettext("Current Time Zone") );
	
	zoneInfo = GET_VALUE(&sysTimeCfgs, SYS_TIMEZONE);
	if(!strcasecmp(SYS_TIMEZONE_TORONTO, zoneInfo) )
		sprintf(zoneStr, "%s", gettext("Canada/Eastern Time") );
	if(!strcasecmp(SYS_TIMEZONE_SHANGHAI, zoneInfo) )
		sprintf(zoneStr, "%s", gettext("CHINA-SHANGHAI") );
	else if(!strcasecmp(SYS_TIMEZONE_TOKYO, zoneInfo) )
		sprintf(zoneStr, "%s", gettext("JAP-TOKYO") );
	else
		sprintf(zoneStr, "%s", gettext("Canada/Eastern Time") );
//		sprintf(zoneStr, "%s", gettext("AMERICA-NEWYORK") );
	
	length += CGI_SPRINTF(buf,length, "<td>%s</td></TR>\n", zoneStr);

	length += CGI_SPRINTF(buf,length, "<tr><td align=\"right\" bgcolor=\"#cccccc\"><strong>%s:</strong></TD>\n", 
		gettext(SYSTIME_STR_TIME_MODE) );
	length += CGI_SPRINTF(buf,length, "<td>%s</td></TR>\n",
		(!strcasecmp(SYS_TIMEMODE_NTP, GET_VALUE(&sysTimeCfgs, SYS_TIMEMODE)))? gettext(SYSTIME_STR_NTP_SERVER):gettext("Hardware RTC"));

	if( !strcasecmp(SYS_TIMEMODE_NTP, GET_VALUE(&sysTimeCfgs, SYS_TIMEMODE) ) )
	{
		length += CGI_SPRINTF(buf,length, "<tr><td align=\"right\" bgcolor=\"#cccccc\"><strong>%s %s</strong></TD>", gettext(SYSTIME_STR_NTP_SERVER), gettext(CGI_STR_IP_ADDRESS) );
		length += CGI_SPRINTF(buf,length, "<td>%s</td></TR>\n", GET_VALUE(&sysTimeCfgs, SYS_NTP_SERVER) );
	}

	length += CGI_SPRINTF(buf,length, "<tr><td align=\"center\" bgcolor=\"#cccccc\"><strong>%s</strong></td>", 
		gettext(SYSTIME_STR_CURRENT_TIME) );

	length += CGI_SPRINTF(buf,length, "<td align=\"center\"><font color=\"#ff0000\">%s-%s-%s %s</font></td></TR\n>", 
		currentValue->year, currentValue->month, currentValue->day, currentValue->current_time);
	
	length += CGI_SPRINTF(buf,length,"\n<script language=\"JavaScript\">\n");
	length += CGI_SPRINTF(buf,length,"<!--\nsetTimeout('window.location.replace(\"%s\")', %d)\n", 
		       WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_TIME, 5*1000);
	length += CGI_SPRINTF(buf,length,"//-->\n</script>\n");

cgitrace();
	return strdup(buf);
}

static char *__systime_select_buttons(SYS_INFO *info)
{
	char 	buf[4096];
	int 		length = 0;

	length += CGI_SPRINTF(buf, length, "<form name=\"systimeForm\" method=\"post\" action=\"%s\">\n", WEB_URL_SYSTEM_INFO );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n", CGI_KEYWORD_OPERATOR, GET_CGI_OP(&info->cgiVariables) );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n", CGI_ACTION, SYSTIME_OP_SAVE_MODE );

	length += CGI_SPRINTF(buf,length, "<TR><TD align=\"right\">%s</TD>\r\n", gettext(SYSTIME_STR_TIME_ZONE) );

	length += CGI_SPRINTF(buf, length, "\t<TD>\n\t<select name=\"%s\"  maxlength=\"30\">\n", SYS_TIMEZONE );
	length += CGI_SPRINTF(buf,length, "\t\t<option value=\"%s\" %s>%s</option>\n", SYS_TIMEZONE_TORONTO, 
		(!strcasecmp(SYS_TIMEZONE_TORONTO, GET_VALUE(&sysTimeCfgs, SYS_TIMEZONE)))?"selected":"" , 
		gettext("CANADA-TORONTO"));
	length += CGI_SPRINTF(buf,length, "\t\t<option value=\"%s\" %s>%s</option>\n", SYS_TIMEZONE_TOKYO, 
		(!strcasecmp(SYS_TIMEZONE_TOKYO, GET_VALUE(&sysTimeCfgs, SYS_TIMEZONE)))?"selected":"" , 
		gettext("JAP-TOKYO"));
//	cgidebug("=%s\n", GET_VALUE(&sysTimeCfgs, SYS_TIMEZONE_TOKYO) );
	length += CGI_SPRINTF(buf, length, "\t\t<option value=\"%s\" %s>%s</option>\n", SYS_TIMEZONE_SHANGHAI, 
		(!strcasecmp(SYS_TIMEZONE_SHANGHAI, GET_VALUE(&sysTimeCfgs, SYS_TIMEZONE)))?"selected":"" , 
		gettext("CHINA-SHANGHAI"));
	length += CGI_SPRINTF(buf, length, "\t\t<option value=\"%s\" %s>%s</option>\n", SYS_TIMEZONE_NEWYORK, 
		(!strcasecmp(SYS_TIMEZONE_NEWYORK, GET_VALUE(&sysTimeCfgs, SYS_TIMEZONE)))?"selected":"" , 
		gettext("AMERICA-NEWYORK"));
	length += CGI_SPRINTF(buf,length, "\t</select>\n</TD></TR>\r\n");


	length += CGI_SPRINTF(buf,length, "<TR><TD align=\"right\">%s</TD>\r\n",
		gettext(SYSTIME_STR_TIME_MODE) );

	length += CGI_SPRINTF(buf, length, "<TD colspan=2>\n\t<select name=\"%s\"  maxlength=\"30\">\n", SYS_TIMEMODE );
	length += CGI_SPRINTF(buf,length, "\t\t<option value=\"%s\" %s>%s</option>\n", SYS_TIMEMODE_NTP, 
		(!strcasecmp(SYS_TIMEMODE_NTP, GET_VALUE(&sysTimeCfgs, SYS_TIMEMODE)))?"selected":"" , 
		gettext(SYSTIME_STR_NTP_SERVER));
	length += CGI_SPRINTF(buf, length, "\t\t<option value=\"%s\" %s>%s</option>\n", SYS_TIMEMODE_HW, 
		(!strcasecmp(SYS_TIMEMODE_HW, GET_VALUE(&sysTimeCfgs, SYS_TIMEMODE)))?"selected":"" , 
		gettext("Hardware RTC"));
	length += CGI_SPRINTF(buf,length, "\t</select>\n</TD></TR>\r\n");

	length += CGI_SPRINTF(buf,length, "<TR><TD align=\"center\">%s</TD>\r\n", cgi_submit_button( gettext(CGI_STR_NEXT) ) );//, msg) );

	length += CGI_SPRINTF(buf,length, "<TD align=\"center\">%s</TD></TR>\r\n", cgi_help_button(CGI_HELP_MSG_SYSTIME) );

cgitrace();
	return strdup(buf);
}


static char *__systime_time_input_buttons(SYS_INFO *info)
{
	char 	buf[4096];
	int 		length = 0;

	length += CGI_SPRINTF(buf, length, "<form name=\"systimeForm\" method=\"post\" action=\"%s\">\n", WEB_URL_SYSTEM_INFO );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n", CGI_KEYWORD_OPERATOR, GET_CGI_OP(&info->cgiVariables) );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n", CGI_ACTION, SYSTIME_OP_SAVE_HW_TIME );

	length += CGI_SPRINTF(buf,length, "<TR><TD bgcolor=\"#cccccc\" align=\"right\"><strong>%s:</strong></TD><TD align=\"left\"><input name=\"%s\" onKeyPress=\"checkNumber(this)\" type=\"text\" size=\"15\" value=\"%s\" maxlength=\"50\"></TD></TR>\r\n", 
		gettext("Year"), SYSTIME_VALUE_YEAR, currentValue->year);
	length += CGI_SPRINTF(buf,length, "<TR><TD bgcolor=\"#cccccc\" align=\"right\"><strong>%s:</strong></TD><TD align=\"left\"><input name=\"%s\" onKeyPress=\"checkNumber(this)\" type=\"text\" size=\"15\" value=\"%s\" maxlength=\"50\"></TD></TR>\r\n", 
		gettext("Month"), SYSTIME_VALUE_MONTH, currentValue->month);
	length += CGI_SPRINTF(buf,length, "<TR><TD bgcolor=\"#cccccc\" align=\"right\"><strong>%s:</strong></TD><TD align=\"left\"><input name=\"%s\" onKeyPress=\"checkNumber(this)\" type=\"text\" size=\"15\" value=\"%s\" maxlength=\"50\"></TD></TR>\r\n", 
		gettext("Day"), SYSTIME_VALUE_DAY, currentValue->day);

	length += CGI_SPRINTF(buf,length, "<TR><TD bgcolor=\"#cccccc\" align=\"right\"><strong>%s:</strong></TD><TD align=\"left\"><input name=\"%s\" onKeyPress=\"checkNumber(this)\" type=\"text\" size=\"15\" value=\"%s\" maxlength=\"50\"></TD></TR>\r\n", 
		gettext("Hour"), SYSTIME_VALUE_HOUR, "");//currentValue->year);
	length += CGI_SPRINTF(buf,length, "<TR><TD bgcolor=\"#cccccc\" align=\"right\"><strong>%s:</strong></TD><TD align=\"left\"><input name=\"%s\" onKeyPress=\"checkNumber(this)\" type=\"text\" size=\"15\" value=\"%s\" maxlength=\"50\"></TD></TR>\r\n", 
		gettext("Minute"), SYSTIME_VALUE_MINUTE, "");//currentValue->year);
#if 0	
	length += CGI_SPRINTF(buf,length, "<TR><TD align=\"left\">%s</TD><TD><input name=\"%s\" onKeyPress=\"checkNumber(this)\" onChange=\"checkIP(%s.value,this)\"  type=\"text\" size=\"15\" value=\"%s\" maxlength=\"50\"></TD></TR>\r\n", 
		SYSTIME_VALUE_SECOND, SYSTIME_VALUE_SECOND, currentValue->year);
#endif

	length += CGI_SPRINTF(buf,length, "<TR><TD align=\"center\">%s</TD>\r\n",cgi_submit_button( gettext(CGI_STR_ACTIVATE ) ) );

	length += CGI_SPRINTF(buf,length, "<TD align=\"center\">%s</TD></TR>\r\n" , cgi_help_button(CGI_HELP_MSG_SYSTIME) );

	return strdup(buf);
}


static char *__systime_ntp_server_buttons(SYS_INFO *info)
{
	char 	buf[4096];
	int 		length = 0;

	length += CGI_SPRINTF(buf, length, "<form name=\"systimeForm\" method=\"post\" action=\"%s\">\n", WEB_URL_SYSTEM_INFO );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n", CGI_KEYWORD_OPERATOR, GET_CGI_OP(&info->cgiVariables) );
	length += CGI_SPRINTF(buf, length,  "<input type=\"hidden\" name=\"%s\" value=\"%s\">\n", CGI_ACTION, SYSTIME_OP_SAVE_NTP_ADDRESS );

	length += CGI_SPRINTF(buf,length, "<TR><TD align=\"right\" bgcolor=\"#cccccc\"><strong>%s</strong></TD>\r\n", gettext("NTP Server IP Address") );

	length += CGI_SPRINTF(buf,length, "<TD align=\"left\"><input name=\"%s\" onKeyPress=\"checkNumber(this)\" onChange=\"checkIP(%s.value,this)\"  type=\"text\" size=\"15\" value=\"%s\" maxlength=\"50\"></TD></TR>\r\n", 
		SYS_NTP_SERVER, SYS_NTP_SERVER, GET_VALUE(&sysTimeCfgs, SYS_NTP_SERVER ) );
#if 0
	length += CGI_SPRINTF(buf,length, "<TR><TD colspan=2><strong>%s:</strong><br>%s <strong>%s</strong><br>%s <strong>%s</strong></TD></TR>\r\n",
		gettext("Suggestion"),gettext("In China, NTP Server can be set as"),NTP_SERVER_CHINA, 
		gettext("In Japan, NTP Server can be set as"), NTP_SERVER_CANADA );
#endif
	length += CGI_SPRINTF(buf,length, "<TR><TD align=\"center\">%s</TD>\r\n", cgi_submit_button( gettext(CGI_STR_SAVE) ) );

	length += CGI_SPRINTF(buf,length, "<TD align=\"center\">%s</TD></TR>\r\n", cgi_help_button(CGI_HELP_MSG_SYSTIME) );

	return strdup(buf);
}

static int __systime_init()
{

	if (readconfig(SYS_LOCALE_CONF, ITEM_EQU, NULL, &sysTimeCfgs) != 0)
	{
cgitrace();
		cgi_error_page(NULL, gettext("Locale Operation Failed"), gettext( "No Configuration for System Time" ) );
		return -1;
	}

cgitrace();
	currentValue = systime_get_current_time();
	return 0;
}

static int __systime_activate_ntp(SYS_INFO *info)
{
	char 	*ip;
	char		command[128];
	ip = GET_VALUE( &info->cgiVariables,  SYS_NTP_SERVER );
	if(!ip)
	{
		cgi_error_page(NULL, gettext(CGI_STR_ERROR), gettext("No IP Address of NTP Server is defined"));
		return -1;
	}

	list_replace_value_by_key(&sysTimeCfgs, SYS_NTP_SERVER, ip);// GET_VALUE(&info->cgiVariables, SYS_NTP_SERVER ) );

	writeconfig(SYS_LOCALE_CONF, ITEM_EQU, NULL, &sysTimeCfgs);

	sprintf(command, "ntpd -p %s -q -d >> /tmp/ntp.data", ip );
	system(command);
	
	cgi_refresh_page(1, WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_TIME, gettext(CGI_STR_WAIT), gettext("Get Time From NTP Server") );
	return 0;	
}

static int __systime_activate_hw_time(SYS_INFO *info)
{
	char 	*y, *mon, *day;
	char		*h, *min;//, *sec;
	char		command[256];
	
	y = GET_VALUE( &info->cgiVariables,  SYSTIME_VALUE_YEAR);
	mon = GET_VALUE( &info->cgiVariables,  SYSTIME_VALUE_MONTH);
	day = GET_VALUE( &info->cgiVariables,  SYSTIME_VALUE_DAY);
	h = GET_VALUE( &info->cgiVariables,  SYSTIME_VALUE_HOUR);
	min = GET_VALUE( &info->cgiVariables,  SYSTIME_VALUE_MINUTE);
//	sec = cgi_value( &info->cgiVariables,  SYSTIME_VALUE_SECOND);
	
	if(!y || !mon ||!day || !h ||!min)
	{
		cgi_error_page(NULL, gettext(CGI_STR_ERROR), gettext("Some Fields are not defined"));
		return -1;
	}

	sprintf(command, "/bin/date -s %s%s%s%s%s >> /tmp/time.data", 
		(strlen(mon)!=0)?mon:"00", (strlen(day)!=0)?day:"00", 
		(strlen(h)!=0)?h:"00", (strlen(min)!=0)?min:"00", (strlen(y)!=0)?y:"2005"  );
	system(command);
	cgidebug("date command=%s\n", command);
	
	cgi_refresh_page(1, WEB_URL_SYSTEM_INFO"?"CGI_KEYWORD_OPERATOR"="CGI_SYSINFO_OP_TIME, gettext(CGI_STR_WAIT), gettext("Reset Current System Time") );
	return 0;	
}


static int __systime_step2(SYS_INFO *info)
{
	char 	*cmd=NULL;
	char 	*msg;
	char		title[256];
	int		len = 0;
	
	cmd = GET_VALUE( &info->cgiVariables,  SYS_TIMEMODE );
	if(!cmd)
	{
		cgi_error_page(NULL, gettext(CGI_STR_ERROR), gettext("No This command is defined"));
		return -1;
	}

	list_replace_value_by_key(&sysTimeCfgs, SYS_TIMEMODE, cmd);
	list_replace_value_by_key(&sysTimeCfgs, SYS_TIMEZONE, GET_VALUE(&info->cgiVariables, SYS_TIMEZONE) );

	writeconfig(SYS_LOCALE_CONF, ITEM_EQU, NULL, &sysTimeCfgs);

	len += CGI_SPRINTF(title, len, "%s : %s 2, ", gettext(SYSTIME_STR_CONFIGURATION), gettext(SYSTIME_STR_STEP) );
	
	if(!strcasecmp(SYS_TIMEMODE_NTP, cmd ) )
	{
		len += CGI_SPRINTF(title, len, "%s %s", gettext(SYSTIME_STR_NTP_SERVER), gettext(CGI_STR_IP_ADDRESS) );
		msg = __systime_ntp_server_buttons(info);
	}
	else
	{
		len += CGI_SPRINTF(title, len, "%s %s", gettext("Input"), gettext(SYSTIME_STR_CURRENT_TIME) );
		msg = __systime_time_input_buttons(info);
	}
	cgi_info_page(title, "", msg);

	return 0;	
}


int cgiSysInfo_Time(SYS_INFO *info)
{
	char 	*action =NULL;
	char	 	title[256];
	int		len = 0;

cgitrace();
	if(__systime_init() )
	{
		return 1;
	}

cgitrace();
	action = GET_CGI_ACTION(&info->cgiVariables );
	if( !action || !strlen(action) )
	{
cgitrace();
		len += CGI_SPRINTF(title, len, "%s : %s 1, ", gettext(SYSTIME_STR_CONFIGURATION), (SYSTIME_STR_STEP) );
		len += CGI_SPRINTF(title, len, "%s, %s", gettext(SYSTIME_STR_TIME_ZONE), gettext(SYSTIME_STR_TIME_MODE) );
		cgi_info_page(title , __systime_status_list(), __systime_select_buttons(info)  );
		return 0;
	}
	else if(!strcasecmp(action, SYSTIME_OP_SAVE_MODE) )
	{
		__systime_step2(info);
		return 0;
	}
	else if(!strcasecmp(action, SYSTIME_OP_SAVE_NTP_ADDRESS) )
	{
		__systime_activate_ntp(info);
		return 0;
	}
	else if(!strcasecmp(action, SYSTIME_OP_SAVE_HW_TIME) )
	{
		__systime_activate_hw_time(info);
		return 0;
	}
	else
	{
		cgi_invalidate_page();
		return 0;
	}
	
	return 0;	
}

