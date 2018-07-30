#
# $Id: Makefile,v 1.1.1.1 2006/11/29 17:06:59 lizhijie Exp $
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = .

# name of all files to be compiled (don't include header files)
SRC += \
		ffmpeg_opt.c \
		ffmpeg_filter.c \
		ffmpeg_cuvid.c	\
		ffmpeg.c  \

C_FLAGS += -I$(ROOT_DIR)/programs/libcmd

	 
LOCAL_ALL:=ffmpeg

include $(ROOT_DIR)/Makefile.post

LDFLAGS += -lpthread -lm -lcmdutil -lavdevice 

ffmpeg:$(OBJS)
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   
