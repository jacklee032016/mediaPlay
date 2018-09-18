#
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = .

C_FLAGS += $(SHARED_CFLAGS) -I$(ROOT_DIR)/cgi/include

# name of all files to be compiled (don't include header files)
SRC += \
					cgiMuxMain.c \
					cgiMuxStatus.c \
					cgiMuxWindow.c \
					cgiMuxPlaylist.c \
					cgiMuxMediafiles.c \
					cgiMuxRecord.c \
					cgiMuxInfos.c \
					cgiMuxGlobalInfo.c \
					cgiMuxSvrRuntime.c \


CFLAGS += -fno-strict-aliasing

LDFLAGS := -lCgi $(LDFLAGS) $(SHARED_LDFLAGS) -L$(HI_SHARED_LIB_DIR) -lrt -lm -ldl -lz

CGIS = \
				muxServer.cgi

		 
LOCAL_ALL:=$(CGIS) 

include $(ROOT_DIR)/Makefile.post


muxServer.cgi:$(OBJS)  
		$(CC) -o $@ $^ $(LDFLAGS)
		$(STRIP) $@
		$(MOVE) $@ $(CGI_BIN_DIR)/ 

