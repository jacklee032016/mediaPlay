/*
* $Id$
*/

#ifndef	___SERVICES_H__
#define	___SERVICES_H__


int httpEventError(SERVER_CONNECT *httpConn, MUX_SVR_EVENT *event);
int httpEventInfo(SERVER_CONNECT *httpConn, MUX_SVR_EVENT *event);
int httpEventLiveStream(SERVER_CONNECT *httpConn, MUX_SVR_EVENT *event);

int httpEventRedirectParsed(SERVER_CONNECT *httpConn, MUX_SVR_EVENT *event);

void httpLiveSendDataTailer(SERVER_CONNECT *httpConn );

int httpEventStatusPage(SERVER_CONNECT *httpConn, MUX_SVR_EVENT *event );
int httpEventClosing(SERVER_CONNECT *httpConn, MUX_SVR_EVENT *event);



int rtspEventError(SERVER_CONNECT *rtspConn, MUX_SVR_EVENT *event);
int rtspEventOptions(SERVER_CONNECT *rtspConn, MUX_SVR_EVENT *event);
int rtspEventDescribe(SERVER_CONNECT *rtspConn, MUX_SVR_EVENT *event);
int rtspEventPlay(SERVER_CONNECT *rtspConn, MUX_SVR_EVENT *event);
int rtspEventRecord(SERVER_CONNECT *rtspConn, MUX_SVR_EVENT *event);
int rtspEventPause(SERVER_CONNECT *rtspConn, MUX_SVR_EVENT *event );
int rtspEventTeardown(SERVER_CONNECT *rtspConn, MUX_SVR_EVENT *event);
int rtspEventSetup(SERVER_CONNECT *rtspConn, MUX_SVR_EVENT *event );
int rtspEventClosing(SERVER_CONNECT *rtspConn, MUX_SVR_EVENT *event);

void rtspReplyHeader(SERVER_CONNECT *rtspConn, enum RTSPStatusCode error_number);


int muxSvrConnectPrintf(SERVER_CONNECT *svrConn, const char *fmt, ...);
int muxSvrConnectFlushOut( SERVER_CONNECT *svrConn);
int muxSvrConnectReceive(SERVER_CONNECT *svrConn);

void muxSvrRtpDataConnClose(DATA_CONNECT *dataConn);
int muxSvrRtpDataConnOpen(DATA_CONNECT *dataConn, const char *info);

int	muxSvrFeedSendPacket(MuxFeed *feed, AVPacket *pkt);


int	muxSvrFeedInit(int index, void *ele, void *priv);

int	muxSvrCreateConsumer4Feed(MuxFeed *feed, MUX_SVR *muxSvr);



#endif

