#
# $Author$
#


include $(RULE_DIR)/Makefile.pre

C_FLAGS += $(SHARED_CFLAGS)

SRC:= \
		apiCmdOpts.c \
		apiClient.c \
		


CODE_SUBDIRS = .

LOCAL_ALL := \
		apiClient \
		

include $(ROOT_DIR)/Makefile.post

LDFLAGS += -L$(BIN_DIR)/lib -lMux -lMedia -lShared $(LDFLAGS)

ifeq ($(ARCH),arm)
LDFLAGS += $(HI_LIB_PATHS) 
else
LDFLAGS += -L$(FFMPEG_HOME)/Linux.bin.$(ARCH)/lib $(FFMPEG_LDFLAGS) -ldl
endif

LDFLAGS += -lz -lpthread -lm


apiClient:$(OBJS)
	$(CC) -s -o $@ $^ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin  

