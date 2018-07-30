#
# $Id$
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = . x86

C_FLAGS += $(FFMPEG_CFLAGS)
ASM_FLAGS += -I libswresample/x86/ 

################################################
####   source file 
################################################
X86_SRC += \
		x86/audio_convert.asm x86/audio_convert_init.c  \
		x86/rematrix.asm x86/rematrix_init.c \
		x86/resample.asm x86/resample_init.c 


SRC += \
		$(X86_SRC) \
		audioconvert.c dither.c options.c \
		rematrix.c resample.c \
		resample_dsp.c swresample.c \
		swresample_frame.c 
		


SRC2 += \



TARGET_LIB:=libswresample

TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a
		 
LOCAL_ALL:=

include $(ROOT_DIR)/Makefile.post

