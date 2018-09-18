#
# $Id$
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = \
		process

C_FLAGS += $(SHARED_CFLAGS) -I$(ROOT_DIR)/cgi/include

################################################
####   source file 
################################################

# name of all files to be compiled (don't include header files)
SRC += \
	libCgiList.c \
	libCgiPages.c \
	libCgiUtils.c \
	libCgiCgi.c \
	libCgiNic.c \
	libCgiMux.c \
	process/libCgiSignal.c \
	process/libCgiUploadFile.c \
	process/libCgiSyscfg.c \
	process/libCgiServices.c \


TARGET_LIB:=libCgi
TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a
		 
LOCAL_ALL:=

include $(ROOT_DIR)/Makefile.post
