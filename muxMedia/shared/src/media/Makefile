#
# $Id$
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = .


C_FLAGS += $(SHARED_CFLAGS) -I$(FFMPEG_HOME)



SRC += \
		cmnMediaScanFileList.c \
		cmnMedia.c \
		

TARGET_LIB:=libMedia

TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a
		 
LOCAL_ALL:=

include $(ROOT_DIR)/Makefile.post

LD_FLAGS:=-L$(FFMPEG_HOME)/Linux.bin.$(ARCH)/lib -lavformat -lavcodec -lavutil
ifeq ($(ARCH),arm)
else
	LD_FLAGS += -lswresample
endif


$(TARGET_LIB_SHARED):$(OBJS)  
	$(CC) $(LDFLAGS) -shared -Wl,--version-script=link.script -s  $^ $(SHARED_LD_FLAGS) $(LIBS) $(LDFLAGS) $(LD_FLAGS) -o $@  
	$(MOVE) $(TARGET_LIB_SHARED) $(BIN_DIR)/lib

