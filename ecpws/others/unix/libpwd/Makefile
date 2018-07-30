# Makefile 
# $Author: lizhijie $
# $Revision: 1.1.1.1 $
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = src

C_FLAGS:= -Wall -Os -fomit-frame-pointer -D_GNU_SOURCE \
	-mbig-endian -DCONFIG_VER='"$(VERSION)"' \
	-DCONFIG_BT='"$(BUILDTIME)"' \
	-I. -DUSE_SYSTEM_SHADOW  
 
SRC := 	\
	__getpwent.c \
	pwent.c \
	getpwnam.c \
	getpwuid.c \
	putpwent.c \
	getpw.c \
	fgetpwent.c \
	__getgrent.c \
	grent.c \
	getgrnam.c \
	getgrgid.c \
	fgetgrent.c \
	initgroups.c \
	setgroups.c 
	
LOCAL_CLEAN:=clear

$(LOCAL_CLEAN):
	
TARGET_LIB:=libboa_pwd

include $(ROOT_DIR)/Makefile.post
