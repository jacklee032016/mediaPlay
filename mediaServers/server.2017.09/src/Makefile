#
# $Id: Makefile,v 1.1.1.1 2006/11/29 17:06:59 lizhijie Exp $
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = \
						http \
						rtsp \
						threads \
						utils	\
						web/http \
						web/cgi \
						 
			

C_FLAGS += -I$(FFMPEG_HOME) $(SHARED_CFLAGS) -I$(ROOT_DIR)/include/web 


WEB_SRC += \
			web/muxSvrWebAlias.c \
			web/muxSvrWebBuffer.c \
			web/muxSvrWebConfig.c \
			web/muxSvrWebHash.c \
			web/muxSvrWebLog.c \
			web/muxSvrWebNetService.c \
			web/muxSvrWebRange.c \
			web/muxSvrWebShadow.c \
			web/muxSvrWebUtils.c \
			web/muxSvrWebMain.c \
			web/muxSvrWebThread.c \
			web/cgi/muxSvrWebCgiCreate.c \
			web/cgi/muxSvrWebCgiHandlers.c \
			web/cgi/muxSvrWebCgiInit.c \
			web/http/muxSvrWebHttpFsm.c \
			web/http/muxSvrWebHttpHandlers.c \
			web/http/muxSvrWebHttpParseRequest.c \
			web/http/muxSvrWebHttpReadRequest.c \
			web/http/muxsvrWebHttpResponse.c \
			web/http/muxSvrWebHttpStaticContent.c \
			

HTTP_SRC +=	\
		http/muxSvrHttpMain.c \
		http/muxSvrHttpHandlers.c \
		http/muxSvrHttpStatus.c \

#		http/muxSvrHttpAuthen.c \



RTSP_SRC +=	\
		rtsp/muxSvrRtspMain.c \
		rtsp/muxSvrRtspHandlers.c \
		rtsp/muxSvrRtp.c \


THREADS_SRC +=	\
		threads/muxSvrThreadCtrlInit.c \
		threads/muxSvrThreadCtrlMain.c \
		threads/muxSvrThreadCtrlData.c \
		threads/muxSvrThreadFileFeed.c \


UTILS_SRC +=	\
		utils/muxSvrUtils.c \
		utils/muxSvrConnUtils.c \


#		utils/muxSvrMulticast.c \



# name of all files to be compiled (don't include header files)
SRC += \
		$(WEB_SRC) \
		muxSvrConfig.c \
		muxSvrCfgStream.c \
		$(HTTP_SRC)	\
		$(RTSP_SRC)	\
		$(THREADS_SRC)	\
		$(UTILS_SRC)	\
		muxSvrServiceConns.c	\
		muxSvrFeed.c	\
		muxSvrMain.c \

	 

LOCAL_ALL:=muxServer

include $(ROOT_DIR)/Makefile.post


LDFLAGS += $(SHARED_LDFLAGS) -L$(FFMPEG_HOME)/Linux.bin.$(ARCH)/lib -lavformat -lavcodec -lavutil -lm
ifeq ($(ARCH),arm)
	LDFLAGS +=  -L$(HI_SHARED_LIB_DIR) 
else
	LDFLAGS += -lswresample
endif
LDFLAGS += -lpthread -lm -ldl -lz -lcrypt

#-lavdevice -lavfilter -lswscale -lswresample 

muxServer:$(OBJS)
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/sbin  
