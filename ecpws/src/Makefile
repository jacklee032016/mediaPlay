#
# $Id: Makefile 1834 2009-11-05 02:36:26Z lizj $
#

IS_STATIC=yes

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = cgi http index 

C_FLAGS +=-DSHARED_BUILD
CFLAGS +=$(C_CFLAGS_1)
CFLAGS +=$(C_CFLAGS_2)

################################################
####   source file 
################################################
EXTRA_SRC += \
    index/indexDir.c \
    index/indexScanDir.c \
    ecpwsEscape.c \
	
SRC += \
    cgi/ecpwsCgiInit.c \
    cgi/ecpwsCgiCreate.c \
    cgi/ecpwsCgiHandlers.c \

SRC += \
    http/ecpwsHttpHandlers.c \
    http/ecpwsHttpBaseAuthen.c \
    http/ecpwsHttpReadRequest.c \
    http/ecpwsHttpParseRequest.c \
    http/ecpwsHttpStaticContent.c \
    http/ecpwsHttpResponse.c \
    http/ecpwsHttpFsm.c \
		

SRC += \
    ecpwsSignals.c \
    ecpwsLog.c \
    ecpwsConfig.c \
    ecpwsAlias.c \
    ecpwsHash.c \
    ecpwsRange.c \
    ecpwsUtil.c \
    ecpwsEscape.c \
    ecpwsMmapCache.c \
    ecpwsNetIp.c \
    ecpwsNetService.c \
    ecpwsBuffer.c \
    ecpwsIoShuffle.c \
    ecpwsShadow.c \
    ecpwsReqQueue.c \
    ecpwsSelect.c \
    ecpws.c 
 

	 
LOCAL_ALL:= indexDir ecpws 



EXTRA_OBJS += \
	$(OBJ_DIR)/index/indexDir.o \
	$(OBJ_DIR)/index/indexScanDir.o \
	$(OBJ_DIR)/ecpwsEscape.o 
	

include $(ROOT_DIR)/Makefile.post

ecpws:$(OBJS)
	$(CC) -s -o $@ $(OBJS) $(LDFLAGS) $(LIBS) $(LIB_LDFLAGS) -lcrypt
	$(MOVE) $@ $(BIN_DIR)/sbin

indexDir:$(EXTRA_OBJS) 
	$(CC) -s -o $@ $^ $(LDFLAGS) $(LIBS) $(LIB_LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/sbin
	