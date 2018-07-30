#
# $Id$
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = . x86

C_FLAGS +=
ASM_FLAGS += -I libavswscale/x86/ 

################################################
####   source file 
################################################

SRC += \
		cmdutils.c \

TARGET_LIB:=libcmdutil

TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a
		 
LOCAL_ALL:=

include $(ROOT_DIR)/Makefile.post

