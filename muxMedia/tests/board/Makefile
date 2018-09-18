#
# $Id: Makefile,v 1.1.1.1 2006/11/29 17:06:59 lizhijie Exp $
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = .

CFLAGS += -I$(ROOT_DIR)/arm/include $(SHARED_CFLAGS) $(HI_CFLAGS)  -I$(FFMPEG_HOME)

# name of all files to be compiled (don't include header files)
SRC += \
	muxds.c 

		 
LOCAL_ALL:=muxds

include $(ROOT_DIR)/Makefile.post

LIBS :=       
           
ifeq ($(ARCH),arm)
	LIBS += -lMux -lMedia -lShared -lhi_common -lpng -lhigo -lhigoadp -lpng -lhi_msp -lfreetype -ljpeg -lhi_resampler -lz -lhi_subtitle -lhi_so -lhi_ttx -lhi_cc  -leffect -lrt -ldl
endif
#-lplayer -lhi_charsetMgr -lsubdec -lhiplayer_utils -lMuxRx 

LIBS += -lm -lpthread -lrt -ldl $(LIBS)  -L$(HI_STATIC_LIB_DIR) -L$(HI_EXTERN_LIB_DIR) -lstdc++ 


muxds:$(OBJS)
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   
