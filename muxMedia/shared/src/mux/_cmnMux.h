/*
* Local header for libMux
*/

#ifndef	___CMN_MUX_H__
#define	___CMN_MUX_H__

int cmnMuxCtrlDataHandle( struct DATA_CONN *dataConn );

cJSON *cmnMuxJsonLoadConfiguration(char *cfgFileName);

int	cmnMuxJsonHandle4Plugin(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data);
int	cmnMuxJsonHandle4SetMedia(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data);
int	cmnMuxJsonHandle4GetMedia(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data);
int	cmnMuxJsonHandle4DownloadMedia(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data);

int	cmnMuxJsonHandle4SysAdmin(MUX_PLUGIN_TYPE dest, struct DATA_CONN *dataConn, cJSON *data);

int cmnMuxCtrlResponse(struct DATA_CONN *dataConn, void *buf, int size);



#endif

