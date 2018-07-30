#
# $Id: Makefile,v 1.1.1.1 2006/11/29 17:06:59 lizhijie Exp $
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = .

# name of all files to be compiled (don't include header files)
SRC += \
		exMuxing.c  \
		exRemuxing.c  \
		exDemuxingDecoding.c  \
		avio_reading.c \
		remuxing.c \
		

C_FLAGS += -I$(ROOT_DIR)/programs/libcmd

	 
LOCAL_ALL:= \
			remuxing \

OTHER += \
			muxing \
			demuxing_decoding \
			avio_reading \
			re \


include $(ROOT_DIR)/Makefile.post

LDFLAGS += -lpthread -lm 
#-lcmdutil -lavdevice 

muxing:$(OBJ_DIR)/exMuxing.o
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

remuxing:$(OBJ_DIR)/exRemuxing.o
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

demuxing_decoding:$(OBJ_DIR)/exDemuxingDecoding.o
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

avio_reading:$(OBJ_DIR)/avio_reading.o
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

re:$(OBJ_DIR)/remuxing.o
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   
