#
# $Id: Makefile,v 1.1.1.1 2006/11/29 17:06:59 lizhijie Exp $
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = .

# name of all files to be compiled (don't include header files)
SRC += \
		muxFfmpegRecord.c	\
		muxFfmpegRecordInput.c \
		recordMain.c \
		

C_FLAGS += -I$(ROOT_DIR)/programs/libcmd

LOCAL_ALL:= record

include $(ROOT_DIR)/Makefile.post

#LDFLAGS += -lpthread -lm -lcmdutil -lavdevice 

record:$(OBJS)
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   
