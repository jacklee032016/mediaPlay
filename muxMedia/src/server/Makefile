#
# $Id: Makefile,v 1.1.1.1 2006/11/29 18:13:58 lizhijie Exp $
#


include $(RULE_DIR)/Makefile.pre


CODE_SUBDIRS = \
			http \
			rtsp \
			threads \
			utils	\


CFLAGS += $(SHARED_CFLAGS) -I$(FFMPEG_HOME) -I$(ROOT_DIR)/include
################################################
####   source file 
################################################


SERVER_SRC +=	\
			http/muxSvrHttpMain.c \
			http/muxSvrHttpHandlers.c \
			http/muxSvrHttpStatus.c \
			rtsp/muxSvrRtspMain.c \
			rtsp/muxSvrRtspHandlers.c \
			rtsp/muxSvrRtp.c \
			threads/muxSvrThreadInit.c \
			threads/muxSvrThreadMain.c \
			threads/muxSvrThreadData.c \
			utils/muxSvrUtils.c \
			utils/muxSvrConnUtils.c \
			muxSvrConfig.c \
			muxSvrServiceConns.c	\
			muxSvrFeed.c	\
			muxSvrMediaConsumer.c \
			muxSvrInit.c \
			muxSvrJSon.c \


SRC += \
			$(SERVER_SRC)  \


#LOCAL_ALL=muxMedia 

TARGET_LIB:=MuxPluginServer
TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a

include $(ROOT_DIR)/Makefile.post

LDFLAGS := -L$(BIN_DIR) -L$(BIN_DIR)/lib \
		$(LDFLAGS)

          
LIBS += -L$(FFMPEG_HOME)/Linux.bin.$(ARCH)/lib -lavformat -lavcodec -lavutil 
LIBS += -lm -lpthread -lrt -ldl -lMux -lMedia $(SHARED_LDFLAGS) -lShared

ifeq ($(ARCH),arm)
else
LIBS += -lswresample
endif


#  -lavformat -lavcodec -lMsUtils  -lDriver $(LDFLAGS) -L$(HI_STATIC_LIB_DIR) 
# -Wl,-verbose -Wl,-static  -s 
$(LOCAL_ALL):$(OBJS)
		$(CC) -s -o $@ $(OBJS) -lMuxRx -L$(ROOT_DIR)/Linux.bin.$(ARCH)/lib -lz $(LIB_LDFLAGS) -lMedia -lCmnRx $(SHARED_LDFLAGS) $(LIBS) 
		$(MOVE) $@ $(BIN_DIR)/sbin
