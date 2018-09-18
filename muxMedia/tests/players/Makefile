#
# $Id: Makefile,v 1.1.1.1 2006/11/29 17:06:59 lizhijie Exp $
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = .

CFLAGS += -I$(ROOT_DIR)/arm/include $(SHARED_CFLAGS) $(HI_CFLAGS)  -I$(FFMPEG_HOME)

# name of all files to be compiled (don't include header files)
SRC += \
		ipplay.c \
		localplay.c \
		player_mosaic_ip.c \
		player_mosaic_ip_auto.c \
		player_mosaic_ts_auto.c \
				
		

LDFLAGS += -L$(BIN_DIR)/lib 

		 
LOCAL_ALL:=localplay 

# ipplay mipa mip mts


include $(ROOT_DIR)/Makefile.post

LIBS :=       
           
ifeq ($(ARCH),arm)
	LIBS += -lMuxRx -lMux -lMedia -lShared -lplayer -lhi_charsetMgr -lsubdec -lhiplayer_utils -lhi_common -lpng -lhigo -lhigoadp -lpng -lhi_msp -lfreetype -ljpeg -lhi_resampler -lz -lhi_subtitle -lhi_so -lhi_ttx -lhi_cc  -lrt -ldl
endif

LIBS += -lm -lpthread -lrt -ldl $(LIBS)  -L$(HI_STATIC_LIB_DIR) -L$(HI_EXTERN_LIB_DIR) -lstdc++ 



ipplay:$(OBJ_DIR)/ipplay.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

localplay:$(OBJ_DIR)/localplay.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

mipa:$(OBJ_DIR)/player_mosaic_ip_auto.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

mip:$(OBJ_DIR)/player_mosaic_ip.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

mts:$(OBJ_DIR)/player_mosaic_ts_auto.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   
