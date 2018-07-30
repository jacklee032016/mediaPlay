#
# $Id: Makefile,v 1.1.1.1 2006/11/29 17:06:59 lizhijie Exp $
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = .

# name of all files to be compiled (don't include header files)
SRC += \
		ffserver_config.c \
		ffserver.c

	 
C_FLAGS += -I$(ROOT_DIR)/programs/libcmd

LOCAL_ALL:=ffserver

include $(ROOT_DIR)/Makefile.post

LDFLAGS += -lpthread -lm -lcmdutil -lavdevice -lavfilter -lswscale  -lswresample

ffserver:$(OBJS)
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   
