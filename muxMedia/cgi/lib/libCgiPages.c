/* 
* html-lib.c - C routines that output various HTML constructs
* $Author: lizhijie $
* $Revision: 1.6 $
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "libCgi.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

int cgi_refresh_page(int timeout, char *target, char *header, char *msg)
{
	printf("<HTML><HEAD>	<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=utf-8\"><TITLE></TITLE>\r\n");
	printf("<META HTTP-EQUIV=\"Expires\" CONTENT=\"now\"><META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\r\n"); 
	printf("<SCRIPT SRC=\"/include/help.js\"></SCRIPT>\r\n");
	printf("<META HTTP-EQUIV=\"Refresh\" CONTENT=\"%d;URL=%s\">\r\n", timeout, target);
	printf("</HEAD><LINK href=\"/nb.css\" rel=stylesheet type=text/css></HEAD><BODY><CENTER>\r\n");
	printf("<TABLE border=0 cellPadding=0 cellSpacing=0 width=502>  <TBODY>\r\n");
	printf("<TR>  <TD class=title width=7><IMG height=24 src=\"/arc.gif\" width=7></TD>\r\n");
	printf("<TD align=left class=title vAlign=center width=495>%s</TD> </TR>\r\n", header);
	printf("<TR><TD colSpan=2><TABLE border=0 cellPadding=0 cellSpacing=0 width=502>        <TBODY>        <TR>          <TD class=vline rowSpan=15><BR></TD>          <TD width=500>            <TABLE align=center border=0 cellPadding=0 cellSpacing=0 class=space width=400>\r\n");
	printf("<TBODY> <TR> <TD><p>%s</p></TD></TR></TBODY></TABLE><TD class=vline rowSpan=15><BR></TD></TR><TR>   <TD class=hline></TD></TR> </TBODY></TABLE></TD></TR></TBODY></TABLE>\r\n", msg);
	printf("</CENTER></BODY></HTML>\r\n");

	return 0;
}

int cgi_error_page(char *target, char *header, char *errmsg)
{
	printf("<HTML><HEAD>	<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=utf-8\"><TITLE></TITLE>\r\n");
	printf("<META HTTP-EQUIV=\"Expires\" CONTENT=\"now\"><META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\r\n"); 
	printf("<SCRIPT SRC=\"/include/help.js\"></SCRIPT>\r\n");
	printf("</HEAD><LINK href=\"/nb.css\" rel=stylesheet type=text/css></HEAD><BODY><CENTER>\r\n");
	printf("<TABLE border=0 cellPadding=0 cellSpacing=0 width=502>  <TBODY>\r\n");
	printf("<TR>  <TD class=title width=7><IMG height=24 src=\"/arc.gif\" width=7></TD>\r\n");
	printf("<TD align=left class=title vAlign=center width=495>%s</TD> </TR>\r\n", header);
	printf("<TR><TD colSpan=2><TABLE border=0 cellPadding=0 cellSpacing=0 width=502>        <TBODY>        <TR>          <TD class=vline rowSpan=15><BR></TD>          <TD width=500>            <TABLE align=center border=0 cellPadding=0 cellSpacing=0 class=space width=400>\r\n");
	printf("<TBODY> <TR> <TD><p>%s</p></TD></TR></TBODY></TABLE><TD class=vline rowSpan=15><BR></TD></TR><TR>   <TD class=hline></TD></TR> </TBODY></TABLE></TD></TR></TBODY></TABLE>\r\n", errmsg);
	printf("</CENTER></BODY></HTML>\r\n");

	return 0;
}

int cgi_info_page( char *header, char *infomsg, char *othermsg)
{
	printf("<HTML><HEAD>	<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=utf-8\"><TITLE></TITLE>\r\n");
	printf("<META HTTP-EQUIV=\"Expires\" CONTENT=\"now\"><META HTTP-EQUIV=\"Pragma\" CONTENT=\"no-cache\">\r\n"); 
	printf("<SCRIPT SRC=\"/include/help.js\"></SCRIPT>\r\n");
	printf("</HEAD><LINK href=\"/nb.css\" rel=stylesheet type=text/css></HEAD>\r\n<BODY><CENTER>\r\n");
	printf("\t<TABLE border=0 cellPadding=0 cellSpacing=0 width=804><TBODY>\r\n");
	/* header line */
	printf("\t\t<TR><TD class=title width=7><IMG height=24 src=\"/arc.gif\" width=7></TD><TD align=left class=title vAlign=center width=790>%s</TD></TR>\r\n", header);
	printf("\t\t<TR><TD colSpan=2>\r\n");
	printf("\t\t\t<TABLE border=0 cellPadding=0 cellSpacing=0  width=804><TBODY>\r\n");
	printf("\t\t\t\t<TR><TD class=vline rowSpan=15><BR></TD><TD width=800>\r\n");
#if 1	
	printf("\t\t\t\t\t<TABLE align=center border=\"1\" align=\"center\" cellpadding=\"1\" cellspacing=\"1\"  class=space width=400>\r\n");
#else/* lzj , 2007.05.31 */
	printf("\t\t\t\t\t<TABLE align=center border=\"1\" align=\"center\" cellpadding=\"1\" cellspacing=\"1\"  class=space \"85%%\">\r\n");
#endif
	printf("%s\r\n", infomsg);
	printf("\t\t\t\t\t</TABLE>");
	
	if(othermsg)
	{
		printf("\r\n\t\t\t\t\t<TABLE align=center border=\"0\" align=\"center\" cellpadding=\"0\" cellspacing=\"0\"  class=space width=600>\r\n");
		printf("%s\r\n", othermsg );
		printf("\t\t\t\t\t</TABLE>\r\n\t\t\t\t");
	}
	
	printf("\r\n\t\t\t\t</TD><TD class=vline rowSpan=15><BR></TD></TR>\r\n\t\t\t\t<TR><TD class=hline></TD></TR>\r\n");
	printf("\r\n\t\t\t</TBODY></TABLE>\r\n");
	printf("\r\n\t\t</TD></TR>\r\n\t</TBODY></TABLE>\r\n</CENTER></BODY>\r\n</HTML>\r\n");

	return 0;
}

void cgi_invalidate_page()
{
	cgi_error_page(NULL, gettext("Invalidate Operation"), gettext(CGI_STR_NO_OPERATION) );
}

char *cgi_submit_button_with_alert(char *name , char *alertMsg, char *formName)
{
	char buf[1024];
	snprintf(buf, sizeof(buf),  "<input type=\"button\" name=\"WriteToConf\" value=\"%s\" class=\"button\" onclick=\"submitConfirm( '%s', %s )\">\r\n", 
		name, alertMsg, formName);

	return strdup(buf);
}

char *cgi_submit_button(char *name )
{
	char buf[1024];
	snprintf(buf, sizeof(buf),  "<input type=\"submit\" name=\"WriteToConf\" value=\"%s\" class=\"button\">\r\n", name);
	return strdup(buf);
}

char *cgi_reset_button(char *name )
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "<input type=\"reset\" name=\"reset\" value=\"%s\" class=\"button\">\r\n", name);
	return strdup(buf);
}

char *cgi_button_alert(char *name, char *onclick_target, char *alertMsg )
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "<input type=\"button\" name=\"%s\" value=\"%s\" class=\"button\" onclick=\"alertConfirm('%s', '%s')\">\r\n", 
		SUBMIT, name, onclick_target , alertMsg );

	return strdup(buf);
}

char *cgi_button(char *name, char *onclick_target)
{
	char buf[1024];
	snprintf(buf, sizeof(buf), "<input type=\"button\" name=\"%s\" value=\"%s\" class=\"button\" onclick=\"open_page('%s')\">\r\n", 
		SUBMIT, name, onclick_target );

	return strdup(buf);
}

char *cgi_help_button( cgi_help_type_t msgType)
{
	char buf[1024];
	snprintf(buf, sizeof(buf),  "<input type=\"button\" name=\"cdef\" value=\"%s\" class=\"button\" onclick=\"open_help_window('/cgi-bin/help.cgi?msg=%d')\">\r\n", 
		gettext("Help"), msgType );

	return strdup(buf);
}

char *cgi_refresh_script(char *target, int seconds)
{
	char buf[1024];
	int 	length = 0;
	
	length += CGI_SPRINTF(buf,length,"\n<script language=\"JavaScript\">\n");
	length += CGI_SPRINTF(buf,length,"<!--\nsetTimeout('window.location.replace(\"%s\")', %d)\n", 
		       target, seconds*1000);
	length += CGI_SPRINTF(buf, length,"//-->\n</script>\n");

	return strdup(buf);
}

int cgi_future_page( char *cmdName, char *module)
{
	char		msg[256];
	sprintf(msg, "'%s' %s" ,gettext(cmdName) , gettext(CGI_STR_FUTURE_VERSION ) );
	return cgi_info_page(gettext(module), gettext(CGI_STR_SORRY), msg);
}

int	cgiRedirect(char *target)
{
	printf("<META HTTP-EQUIV=refresh CONTENT=\"1;URL=http://%s\">\n", target );
	return 0;	
}

