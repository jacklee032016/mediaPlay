#
# $Id: Makefile,v 1.1.1.1 2006/11/29 17:06:59 lizhijie Exp $
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = .

# name of all files to be compiled (don't include header files)
SRC += \
		playOpts.c \
		playUtils.c \
		playQueues.c \
		playSdlDisplay.c \
		playSdlAudio.c \
		playSdlEventLoop.c \
		playFilter.c \
		playStream.c \
		playVideoRefresh.c \
		playThreadMedias.c \
		playThreadRead.c \
		playMain.c \


C_FLAGS += -I$(ROOT_DIR)/programs/libcmd

LOCAL_ALL:=play

include $(ROOT_DIR)/Makefile.post

LDFLAGS += -lpthread -lm -lcmdutil -lavdevice 

play:$(OBJS)
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   
