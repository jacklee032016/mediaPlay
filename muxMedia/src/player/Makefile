#
# $Id: Makefile,v 1.1.1.1 2006/11/29 18:13:58 lizhijie Exp $
#


include $(RULE_DIR)/Makefile.pre


CODE_SUBDIRS = \
			libRx/callback  \
			libRx/command  \
			libRx/init	\


CFLAGS += $(HI_CFLAGS) $(SHARED_CFLAGS)
################################################
####   source file 
################################################

HISILISON_CMN_SRC += \
		libRx/hi_adp_data.c \
		libRx/hi_adp_demux.c \
		libRx/hi_adp_hdmi.c \
		libRx/hi_adp_mpi.c \
		libRx/hi_adp_search.c \
		libRx/hi_filter.c \


MUX_SRC += \
			libRx/callback/muxRxPlayCbEvents.c \
			libRx/callback/muxRxPlayCbSubtitle.c \
			libRx/command/muxRxPlayCmdAttributes.c \
			libRx/command/muxRxPlayCmdSetMedia.c \
			libRx/command/muxRxPlayCmdWindow.c \
			libRx/init/muxRxInitPlayer.c \
			libRx/init/muxRxCaptureInit.c \
			libRx/init/muxRxInit.c \
			libRx/muxRxCaptureHisi.c \
			libRx/muxRxLibUtils.c \
			libRx/muxRxOsdInit.c \
			libRx/muxRxOsdOutput.c \
			libRx/muxRxHdmiCtrl.c \
			libRx/muxRxPlayM3u9.c \


PLAYER_SRC += \
			muxPlayerFsm.c \
			muxPlayerInit.c \
			muxPlayerJSon.c \
			muxPlayerJSonMedia.c \
			muxPlayerPlaylist.c \


ifeq ($(ARCH),arm)
	SRC += \
	
else
	SRC += \
	
endif

SRC += \
			$(HISILISON_CMN_SRC)  \
			$(MUX_SRC)  \
			$(PLAYER_SRC)  \


TARGET_LIB:=MuxPluginPlayer
TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a


include $(ROOT_DIR)/Makefile.post

LDFLAGS := -L$(BIN_DIR) -L$(BIN_DIR)/lib \
		$(LDFLAGS)

          

ifeq ($(ARCH),arm)
#	LIBS += -lMuxRx 
	LIBS += $(SHARED_LDFLAGS) -lMux -lMedia $(LIB_LDFLAGS)
endif

LIBS += -lm -lpthread -lrt -ldl 

LIBS += -L$(HI_STATIC_LIB_DIR) 
LIBS += -Wl,--start-group  -lplayer -lhiplayer_utils -lhi_charsetMgr -lsubdec -lhi_ttx -lhi_cc -lfreetype -lpng -lhi_common -lhigo -lhigoadp -lhi_msp -lhi_resampler -lhi_subtitle -lhi_so  -lhi_ttx -lhi_cc -ljpeg -lz  -Wl,--end-group -L$(HI_SHARED_LIB_DIR) -L$(HI_EXTERN_LIB_DIR)  -lstdc++ 

$(LOCAL_ALL):$(OBJS)
		$(CC) -s -o $@ $(OBJS) -lMuxRx -L$(ROOT_DIR)/Linux.bin.$(ARCH)/lib -lz $(LIBS) $(SHARED_LDFLAGS)
		$(MOVE) $@ $(BIN_DIR)/sbin
