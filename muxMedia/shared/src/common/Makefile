#
# $Id$
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = \
					md5 \
					portable \
					cjson \


C_FLAGS += $(SHARED_CFLAGS)


################################################
####   source file 
################################################
PORTABL_SRC += \
		portable/cmnMutex.c \
		portable/cmnList.c \
		portable/cmnFifo.c \
		portable/cmnTimer.c \
		portable/cmnUtils.c \
		portable/cmnThread.c \
		portable/cmnFsm.c \
		portable/cmnFtpLib.c \
		portable/cmnLogUtils.c \




SRC += \
		$(PORTABL_SRC) \
		md5/md5.c \
		md5/md5hl.c \
		md5/base64.c \
		cjson/cJSON.c \
		cjson/cJSON_Utils.c \
		cmn_version.c \
		
 
TARGET_LIB:=libShared

TARGET_LIB_SHARED:=$(TARGET_LIB).so
TARGET_LIB_STATIC:=$(TARGET_LIB).a
		 
LOCAL_ALL:=

include $(ROOT_DIR)/Makefile.post

