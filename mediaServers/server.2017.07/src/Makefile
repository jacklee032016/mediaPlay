#
# $Id: Makefile,v 1.1.1.1 2006/11/29 17:06:59 lizhijie Exp $
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = .

C_FLAGS += -I$(FFMPEG_HOME)

# name of all files to be compiled (don't include header files)
SRC += \
		muxSvrConfig.c \
		muxSvrCfgStream.c \
		muxSvrUtils.c \
		muxSvrStatus.c \
		muxSvrRtp.c \
		muxSvrRtsp.c \
		muxSvrHttp.c \
		muxSvrConn.c \
		muxSvrSend.c \
		muxSvrReceive.c \
		muxSvrInit.c \
		muxSvrLoop.c \
		muxSvrMain.c \

	 
C_FLAGS += -I$(ROOT_DIR)/programs/libcmd

LOCAL_ALL:=muxServer

include $(ROOT_DIR)/Makefile.post

LDFLAGS += -L$(FFMPEG_HOME)/Linux.bin.$(ARCH)/lib $(FFMPEG_LDFLAGS)

muxServer:$(OBJS)
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/sbin  
