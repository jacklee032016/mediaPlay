#
# $Id$
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = . x86

C_FLAGS += $(FFMPEG_CFLAGS)
ASM_FLAGS += -I libavswscale/x86/ 

################################################
####   source file 
################################################
X86_SRC += \
		x86/hscale_fast_bilinear_simd.c x86/input.asm x86/output.asm \
		x86/rgb2rgb.c x86/scale.asm x86/swscale.c x86/yuv2rgb.c 


SRC += \
		$(X86_SRC) \
		alphablend.c gamma.c hscale.c hscale_fast_bilinear.c \
		input.c options.c output.c rgb2rgb.c slice.c \
		swscale.c swscale_unscaled.c utils.c vscale.c \
		yuv2rgb.c


SRC2 += \


TARGET_LIB:=libswscale

TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a
		 
LOCAL_ALL:=

include $(ROOT_DIR)/Makefile.post

