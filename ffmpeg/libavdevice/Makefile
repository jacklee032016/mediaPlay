#
# $Id$
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = .

C_FLAGS += $(FFMPEG_CFLAGS)

################################################
####   source file 
################################################
PORTABL_SRC += \


SRC += \
			alldevices.c \
			avdevice.c \
			dv1394.c \
			fbdev_common.c \
			fbdev_dec.c \
			fbdev_enc.c \
			lavfi.c \
			oss.c \
			oss_dec.c \
			oss_enc.c \
			sdl2.c \
			timefilter.c \
			utils.c \
			v4l2-common.c \
			v4l2.c \
			v4l2enc.c \
			xcbgrab.c 
			

TARGET_LIB:=libavdevice

TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a
		 
LOCAL_ALL:=

include $(ROOT_DIR)/Makefile.post

