/*
* $Id: content_cgi.c,v 1.4 2007/11/15 08:18:50 lizhijie Exp $
*/

#include <string.h>
#include "libCgi.h"

static char *__content_status()
{
	char 	buf[4096*4];
	int 		length = 0;
	
	length += CGI_SPRINTF(buf,length, "<HTML><HEAD>\n<TITLE>mainitem</TITLE>\n");
	length += CGI_SPRINTF(buf,length, "<META content=\"text/html; charset=utf-8\" http-equiv=Content-Type>\n<META HTTP-EQUIV=\"Cache-Control\" CONTENT=\"no-cache, must-revalidate\">\n");
	length += CGI_SPRINTF(buf,length, "<META HTTP-EQUIV=\"expires\" CONTENT=\"0\">\n<META content=no-cache http-equiv=pragma>\n");
	length += CGI_SPRINTF(buf,length, "<STYLE type=text/css>TD {	COLOR: #0052a4; \"Times New Roman\"; FONT-SIZE: 12px}\n");
	length += CGI_SPRINTF(buf,length, "TD.small {COLOR: #0052a4; FONT-FAMILY: \"Times New Roman\"; FONT-SIZE: 8px}</STYLE>\n");
	length += CGI_SPRINTF(buf,length, "</HEAD><BODY leftMargin=0 topMargin=0>\n");

	/* Bottom lines */
	length += CGI_SPRINTF(buf,length, "<div id=\"Layer0\" style=\"position:absolute; left:571px; top:454px; width:204px; height:8px; z-index:1\">\n");  
	length += CGI_SPRINTF(buf,length, "<div align=\"right\"> <font color=\"#0066FF\"size=\"2\">%s</font></div> \n</div>\n",gettext("MuxLab ProDigital"));
	length += CGI_SPRINTF(buf,length, "<div id=\"Layer1\" style=\"position:absolute; left:621px; top:475px; width:170px; height:9px; z-index:2\"><a href=\"http://www.muxlab.com\"><font size=\"2\" face=\"Times New Roman\">http://www.muxlab.com</font></a></div>\n");

	/* Toppest line */
	length += CGI_SPRINTF(buf,length, "<TABLE border=0 cellPadding=0 cellSpacing=0 ><TBODY>\n<TR>\n"); 
	length += CGI_SPRINTF(buf,length, "<TD height=10></TD> \n</TR><TR height=8>\n<TD>\n");
	length += CGI_SPRINTF(buf,length, "<TABLE width=807 height=\"15\" border=0 cellPadding=0 cellSpacing=0>\n<TBODY>\n<TR> \n");  
	length += CGI_SPRINTF(buf,length, "<TD width=2 height=\"21\"></TD> \n<TD width=\"17\">&nbsp;</TD>\n<TD width=1></TD>\n");
	length += CGI_SPRINTF(buf,length, "<TD width=\"82\"><img src=\"/title_dot.gif\" width=\"17\" height=\"17\" align=\"right\">&nbsp</TD>\n");
	length += CGI_SPRINTF(buf,length, "<TD>&nbsp;%s</TD>\n",gettext("MuxLab RX"));
	length += CGI_SPRINTF(buf,length, "<TD width=1></TD>\n<TD ><div align=\"right\"><IMG src=\"/title_dot.gif\" width=\"17\" height=\"17\"></div></TD>\n");
	length += CGI_SPRINTF(buf,length, "<TD width=\"251\">&nbsp;%s : %s</TD></TR></TBODY></TABLE>\n<p>&nbsp</p>\n",gettext("Model No."),gettext("RX762"));


	/* content table */
	/* title line of content table */
	length += CGI_SPRINTF(buf,length, "<table width=\"93%%\" height=\"456\" border=\"0\">\n<tr>\n<td width=\"13%%\"></td> <td width=\"87%%\" height=\"64\"> &nbsp;<div align=\"left\"><font size=\"6\"><strong> \n");
	length += CGI_SPRINTF(buf,length, "&nbsp;&nbsp;&nbsp;%s</strong></font><strong><font size=\"5\" face=\"Times New Roman, Times, seri\">--<font size=\"6\">RX762</font></font></strong></div></td></tr>\n",
		gettext("MuxLab"));
	length += CGI_SPRINTF(buf,length, "<tr><td colspan=\"2\"><div align=\"center\"></div></td>\n</tr>\n");
	length += CGI_SPRINTF(buf,length, "<tr><td height=\"18\" colspan=\"2\">&nbsp;</td></tr>\n");
	length += CGI_SPRINTF(buf,length, "<tr><td rowspan=\"9\"><div align=\"right\"></div><div align=\"right\"></div></td>\n<td height=\"26\">%s</td></tr>\n",
		gettext("support H.264/265"));
	length += CGI_SPRINTF(buf,length, "<tr><td height=\"26\">%s</td></tr>\n", gettext("") );

	length += CGI_SPRINTF(buf,length, "<tr><td height=\"18\">&nbsp;</td> <td rowspan=\"2\">&nbsp;</td></tr>\n");
	length += CGI_SPRINTF(buf,length, "<tr><td>&nbsp;</td> \n</tr>\n");
	length += CGI_SPRINTF(buf,length, "</table><p>&nbsp;</p><table width=\"101%%\" border=\"0\">\n<tr>\n<td height=\"2\"><div align=\"right\"></a></div></td></tr></table>\n");
	length += CGI_SPRINTF(buf,length, "</TD> \n </TR><META content=no-cache http-equiv=pragma></BODY></HTML>\n");

	return strdup(buf);
}


int main()
{
	CGI_HTML_HEADE();
//	cgidebug("init content.cgi....");

#if 1	
	printf("%s", __content_status() );
#else
	cgi_info_page("", __content_status(), "" );
#endif
	return 0;
}

