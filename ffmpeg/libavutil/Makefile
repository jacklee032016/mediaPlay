#
# $Id$
#

include $(RULE_DIR)/Makefile.pre


C_FLAGS += $(FFMPEG_CFLAGS)
#-MMD -MF 
ifeq ($(ARCH),arm)
	ASM_FLAGS += -I libavutil/arm/ 
else
	ASM_FLAGS += -I libavutil/x86/ 
endif

################################################
####   source file 
################################################
ifeq ($(ARCH),arm)
	ARCH_SRC += \
		arm/cpu.c 
	CODE_SUBDIRS = . arm
	
else
	ARCH_SRC += \
		x86/cpu.c x86/cpuid.asm \
		x86/fixed_dsp.asm x86/fixed_dsp_init.c \
		x86/float_dsp.asm x86/float_dsp_init.c \
		x86/imgutils.asm x86/imgutils_init.c \
		x86/lls.asm x86/lls_init.c \
		x86/pixelutils.asm x86/pixelutils_init.c \
		
	CODE_SUBDIRS = . x86
endif


# add for media server
SERVER_SRC += \
		hmac.c \


SRC += \
		$(ARCH_SRC) \
		aes.c \
		aes_ctr.c \
		avstring.c \
		base64.c \
		bprint.c \
		buffer.c \
		channel_layout.c \
		color_utils.c \
		cpu.c \
		crc.c \
		des.c \
		dict.c \
		display.c \
		error.c \
		eval.c \
		fifo.c \
		file.c \
		file_open.c \
		frame.c \
		hwcontext.c \
		hwcontext_cuda.c \
		imgutils.c \
		lfg.c \
		log.c \
		log2_tab.c \
		lzo.c \
		rational.c \
		rc4.c \
		mathematics.c \
		md5.c \
		mem.c \
		opt.c \
		parseutils.c \
		pixdesc.c \
		random_seed.c \
		reverse.c \
		samplefmt.c \
		sha.c \
		sha512.c \
		spherical.c \
		stereo3d.c \
		time.c \
		timecode.c \
		utils.c \
		$(SERVER_SRC) \
		


ifeq ($(ARCH),arm)
else
SRC += \
		adler32.c \
		audio_fifo.c \
		blowfish.c \
		camellia.c \
		cast5.c \
		downmix_info.c \
		fixed_dsp.c \
		float_dsp.c \
		hash.c \
		integer.c \
		intmath.c \
		lls.c \
		mastering_display_metadata.c \
		murmur3.c \
		pixelutils.c \
		ripemd.c \
		tea.c \
		threadmessage.c \
		tree.c \
		twofish.c \
		xga_font_data.c \
		xtea.c \
		
endif		


TARGET_LIB:=libavutil

TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a

LOCAL_ALL:=

include $(ROOT_DIR)/Makefile.post

