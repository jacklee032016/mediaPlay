#
# $Id: Makefile,v 1.1.1.1 2006/11/29 17:06:59 lizhijie Exp $
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = record

# name of all files to be compiled (don't include header files)

RECORD_SRC += \
			record/muxRecorder.c \
			record/muxFfmpegRecord.c \
			record/muxFfmpegRecordInput.c \


SRC += \
			play_m3u9.c \
			muxAttributes.c \
			muxControlCmds.c \
			muxEventCallback.c \
			muxExtAvPlay.c \
			muxHigoUtils.c \
			muxSetMedia.c \
			muxUtils.c \
			muxInit.c \
			$(RECORD_SRC) \
			muxPlay.c \


CFLAGS += -I$(FFMPEG_HOME) -Wno-error

L_LDFLAGS += -L$(FFMPEG_HOME)/Linux.bin.arm/lib -lavformat -lavcodec -lavutil 

LDFLAGS += -L$(BIN_DIR)/lib 

		 
LOCAL_ALL:=play

include $(ROOT_DIR)/Makefile.post

LDFLAGS += -lMuxTests

play:$(OBJS)
	$(CC) -s -o $@ $(LDFLAGS)  -lhi_charsetMgr -lsubdec -lplayer -lhiplayer_utils $^ $(L_LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   
