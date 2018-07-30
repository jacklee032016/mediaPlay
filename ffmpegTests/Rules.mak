# rule for every directory

# support PC environments
# ARCH=
# ARCH=arm

#################   init Hisilicon build environment
ifeq ($(CFG_HI_EXPORT_FLAG),)
	NEW_SDK_DIR := $(NEW_SDK)
	SDK_DIR := $(NEW_SDK_DIR)
	include $(NEW_SDK_DIR)/base.mak
endif



# released or debug version, different on debug and log info£¬2007.03.15
# must be 'release' or 'debug'
EDITION=debug
# release

ifeq ($(ARCH),arm)
	C_FLAGS += -D__ARM_CMN__=1 -DARCH_ARM=1  -DARCH_X86=0 -DARCH_X86_32=0 
	CXXFLAGS = -D__ARM_CMN__=1 -DARCH_ARM=1  -DARCH_X86=0 -DARCH_X86_32=0 
	CROSS_COMPILER=arm-histbv310-linux-
	LDFLAGS+=  
	flag=
	C_FLAGS +=-DARM
	
else
	ARCH=X86
	C_FLAGS +=-D$(ARCH) -DARCH_X86=1 -DARCH_X86_32=1 -DARCH_ARM=0 
	EXTENSION=
endif


CC	= $(CROSS_COMPILER)gcc
CXX 	= $(CROSS_COMPILER)g++ 
STRIP	= $(CROSS_COMPILER)strip
LD	= $(CROSS_COMPILER)ld
RANLIB 	= $(CROSS_COMPILER)ranlib
STRIP 	= $(CROSS_COMPILER)strip
AR 	= $(CROSS_COMPILER)ar

ASM = yasm

RM	= rm -r -f
MKDIR	= mkdir -p
MODE	= 700
OWNER	= root
CHOWN	= chown
CHMOD	= chmod
COPY	= cp
MOVE	= mv

LN		= ln -sf

#BUILDTIME := $(shell TZ=UTC date -u "+%Y_%m_%d-%H_%M")
BUILDTIME := $(shell TZ=CN date -u "+%Y_%m_%d")
GCC_VERSION := $(shell $(CC) -dumpversion )

BIN_DIR=$(ROOT_DIR)/Linux.bin.$(ARCH)
OBJ_DIR=Linux.obj.$(ARCH)


FFMPEG_HOME := $(ROOT_DIR)/../ffmpeg

##################################  Added for HiSilicon SoC, from sample/base.mak

#==============COMPILE OPTIONS================================================================
ifeq ($(ARCH),arm)

	HI_FLOAT_OPTIONS := -mfloat-abi=softfp -mfpu=vfpv3-d16

	CFG_HI_SAMPLE_CFLAGS := -Werror -Wall
	
	ifneq ($(findstring $(CFG_HI_CHIP_TYPE), hi3798mv100 hi3796mv100 hi3716dv100),)
			CFG_HI_SAMPLE_CFLAGS += -mcpu=cortex-a7
	else ifneq ($(findstring $(CFG_HI_CHIP_TYPE), hi3798cv200 hi3798mv200 hi3798mv200_a),)
			CFG_HI_SAMPLE_CFLAGS += -mcpu=cortex-a53
	else
			CFG_HI_SAMPLE_CFLAGS += -mcpu=cortex-a9
	endif
	
	CFG_HI_SAMPLE_CFLAGS+= -D_GNU_SOURCE -Wall -O2 -ffunction-sections -fdata-sections -Wl,--gc-sections
	CFG_HI_SAMPLE_CFLAGS+= -DCHIP_TYPE_$(CFG_HI_CHIP_TYPE) -DCFG_HI_SDK_VERSION=$(CFG_HI_SDK_VERSION)
	CFG_HI_SAMPLE_CFLAGS+= $(CFG_HI_BOARD_CONFIGS)
	
	ifeq ($(CFG_HI_HDMI_RX_SUPPORT),y)
			CFG_HI_SAMPLE_CFLAGS += -DHI_HDMI_RX_INSIDE
	endif
	ifeq ($(CFG_HI_ADVCA_FUNCTION),FINAL)
	    CFG_HI_SAMPLE_CFLAGS += -DHI_ADVCA_FUNCTION_RELEASE
	else
	    CFG_HI_SAMPLE_CFLAGS += -DHI_ADVCA_FUNCTION_$(CFG_HI_ADVCA_FUNCTION)
	endif
	
	
	SYS_LIBS := -lpthread -lrt -lm -ldl -lstdc++
	
	HI_LIBS := -lhi_common -lpng -lhigo -lhigoadp -lfreetype -ljpeg
	
	HI_LIBS += -lhi_msp
	
	ifeq ($(CFG_HI_TEE_SUPPORT),y)
			HI_LIBS += -lteec
	endif
	
	ifeq ($(CFG_HI_AUDIO_RESAMPLER_SUPPORT),y)
			HI_LIBS += -lhi_resampler
	endif
	
	ifeq ($(CFG_HI_HAEFFECT_BASE_SUPPORT),y)
			HI_LIBS += -lhi_aef_base
	endif
	
	ifeq ($(CFG_HI_HAEFFECT_SRS_SUPPORT),y)
			HI_LIBS += -lhi_aef_srs
	endif
	
	ifeq ($(CFG_HI_ZLIB_SUPPORT),y)
			HI_LIBS += -lz
	endif
	
	ifeq ($(CFG_HI_CAPTION_SUBT_SUPPORT),y)
			HI_LIBS += -lhi_subtitle
	endif
	
	ifeq ($(CFG_HI_CAPTION_SO_SUPPORT),y)
			HI_LIBS += -lhi_so
	endif
	
	ifeq ($(CFG_HI_CAPTION_TTX_SUPPORT),y)
			HI_LIBS += -lhi_ttx
	endif
	
	ifeq ($(CFG_HI_CAPTION_CC_SUPPORT),y)
			HI_LIBS += -lhi_cc
	endif
	
	ifeq ($(CFG_HI_VP_SUPPORT),y)
			HI_LIBS += -lhi_vp -lrtp
	endif
	
else 
	ASM_FLAGS = -f elf  -g dwarf2 -I./ -I.// -I$(ROOT_DIR) -Pconfig.asm 
endif
# else for ARCH=arm

FFMPEG_CFLAGS += -D_ISOC99_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE \
			-D_POSIX_C_SOURCE=200112 -D_XOPEN_SOURCE=600 -DZLIB_CONST \
			-DHAVE_AV_CONFIG_H -std=c11 -fomit-frame-pointer -pthread -D_REENTRANT \

FFMPEG_CFLAGS += -I/usr/include/SDL2 -I$(ROOT_DIR) -I$(FFMPEG_HOME)

FFMPEG_CFLAGS += -g -Wno-error -Wno-error=deprecated-declarations -Wdeprecated-declarations -Wno-deprecated  \
			-Wno-strict-overflow	-fwrapv \
			-Wdeclaration-after-statement -Wall -Wdisabled-optimization -Wpointer-arith \
			-Wredundant-decls -Wwrite-strings -Wtype-limits -Wundef -Wmissing-prototypes \
			-Wno-pointer-to-int-cast -Wstrict-prototypes -Wempty-body -Wno-parentheses \
			-Wno-switch -Wno-format-zero-length -Wno-pointer-sign \
			-O3 -fno-math-errno -fno-signed-zeros -fno-tree-vectorize -Werror=format-security \
			-Werror=return-type \
			-Werror=vla -Wformat -fdiagnostics-color=auto -Wno-maybe-uninitialized 

# -Wno-error : disable all warns as error
#-MMD -MF -MT\
#			 -Werror=missing-prototypes -Werror=implicit-function-declaration

# not support by ARM toolchains
# -Wno-unused-const-variable 

ifeq ($(ARCH),arm)
	FFMPEG_LDFLAGS += \
		-Wl,--as-needed -Wl,-z,noexecstack -Wl,--warn-common -Wl,--start-group -lavformat -lavcodec -lavutil -Wl,--end-group \
		-Wl,-rpath-link=libpostproc:libavformat:libavcodec:libavutil   \
		-lavformat -lavcodec -lavutil -ldl -lm -lz -pthread 
else
	FFMPEG_LDFLAGS += \
		-L$(FFMPEG_HOME)/Linux.bin.$(ARCH)/lib \
		-Wl,--as-needed -Wl,-z,noexecstack -Wl,--warn-common -Wl,--start-group -lavdevice -lavfilter -lavformat -lavcodec -lswresample -lswscale -lavutil -Wl,--end-group \
		-Wl,-rpath-link=libpostproc:libswresample:libswscale:libavfilter:libavdevice:libavformat:libavcodec:libavutil:libavresample   \
		-lavdevice -lavfilter -lavformat -lavcodec -lswresample -lswscale -lavutil -ldl -lxcb -lxcb-shm -lxcb-xfixes -lxcb-shape -lSDL2 -lm -lz -pthread 
endif

