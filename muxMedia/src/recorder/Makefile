#
# $Id: Makefile,v 1.1.1.1 2006/11/29 18:13:58 lizhijie Exp $
#


include $(RULE_DIR)/Makefile.pre


CODE_SUBDIRS += \
			.


CFLAGS += -I$(FFMPEG_HOME) $(SHARED_CFLAGS) 
################################################
####   source file 
################################################



WEB_SRC += \
			muxRecorderInit.c \



ifeq ($(ARCH),arm)
	SRC += \
	
else
	SRC += \

endif

SRC += \
			$(WEB_SRC)  \


TARGET_LIB:=MuxPluginRecorder
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


$(LOCAL_ALL):$(OBJS)
		$(CC) -s -o $@ $(OBJS) -lMuxRx -L$(ROOT_DIR)/Linux.bin.$(ARCH)/lib -lz  $(LIBS) $(LIB_LDFLAGS) -lMedia -lCmnRx $(SHARED_LDFLAGS)
		$(MOVE) $@ $(BIN_DIR)/sbin
