#
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = \
			callback  \
			command  \
			init	\


CFLAGS += $(HI_CFLAGS) $(SHARED_CFLAGS)

################################################
####   source file 
################################################
HISILISON_CMN_SRC += \
		hi_adp_data.c \
		hi_adp_demux.c \
		hi_adp_hdmi.c \
		hi_adp_mpi.c \
		hi_adp_search.c \
		hi_filter.c \
			muxRxLibUtils.c \
			muxRxPlayM3u9.c \

MUX_SRC += \
			callback/muxRxPlayCbEvents.c \
			callback/muxRxPlayCbSubtitle.c \
			command/muxRxPlayCmdSetMedia.c \
			command/muxRxPlayCmdWindow.c \
			init/muxRxInitPlayer.c \
			init/muxRxCaptureInit.c \
			init/muxRxInit.c \
			muxRxCaptureHisi.c \
			muxRxOsdInit.c \
			muxRxOsdOutput.c \

#			muxRxPlayDrm.c \
#			command/muxRxPlayCmdAttributes.c \


SRC += \
		$(HISILISON_CMN_SRC) \


#		$(MUX_SRC)		\


TARGET_LIB:=libMuxRx

TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a

#-L$(HI_SHARED_LIB_DIR) -L$(HI_STATIC_LIB_DIR) -L$(HI_SHARED_LIB_DIR) -L$(HI_EXTERN_LIB_DIR) 
ifeq ($(ARCH),arm)
	LIBS += $(SHARED_LDFLAGS) -lMux -lMedia $(LIB_LDFLAGS)
#	LIBS += -Wl,--start-group -lhi_ttx -lhi_cc -lfreetype -ljpeg -lpng -lhi_common -lhigo -lhigoadp -lhi_msp -lhi_resampler -lhi_subtitle -lhi_so -lz -Wl,--end-group
endif

# stdc++ is required for : undefined symbol: __gxx_personality_v0
# LIBS += -lm -lpthread -lrt -ldl -lstdc++ $(SYS_LIBS)

#LIBS += $(HI_LDFLAGS)
LIBS += -L$(HI_STATIC_LIB_DIR) 
LIBS += -Wl,--start-group  -lplayer -lhiplayer_utils -lhi_charsetMgr -lsubdec -lhi_ttx -lhi_cc -lfreetype -lpng -lhi_common -lhigo -lhigoadp -lhi_msp -lhi_resampler -lhi_subtitle -lhi_so  -lhi_ttx -lhi_cc -ljpeg -lz  -Wl,--end-group -L$(HI_SHARED_LIB_DIR) -L$(HI_EXTERN_LIB_DIR)  -lstdc++ 
		 
LOCAL_ALL:=

include $(ROOT_DIR)/Makefile.post

