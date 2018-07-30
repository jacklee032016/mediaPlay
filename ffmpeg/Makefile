# $Id$

ROOT_DIR:=$(shell pwd)
RULE_DIR:=$(ROOT_DIR)/../

export ROOT_DIR
export RULE_DIR

export HISI_LINUX_ENV=ok

ifeq ($(CFG_HI_EXPORT_FLAG),)
NEW_SDK_DIR := $(NEW_SDK)
SDK_DIR := $(NEW_SDK_DIR)
include $(NEW_SDK_DIR)/base.mak
endif

include $(ROOT_DIR)/Rules.mak

RELEASES_NAME=ffmpeg_$(GCC_VERSION)_$(ARCH)_$(EDITION)_$(BUILDTIME).tar.gz  

export ARCH
export BUILDTIME
export RELEASES_NAME


ifeq ($(ARCH),arm)
SUBDIRS += libavformat
SUBDIRS += libavcodec
SUBDIRS += libavutil
else


SUBDIRS += libavdevice
SUBDIRS += libavfilter
SUBDIRS += libavformat
SUBDIRS += libavcodec
SUBDIRS += libswscale
SUBDIRS += libavutil
SUBDIRS += libswresample

#SUBDIRS += programs/server
#SUBDIRS += programs/ffmpeg
#SUBDIRS += programs/prob

#SUBDIRS += programs/libcmd
#SUBDIRS += programs/play
#SUBDIRS += examples
#SUBDIRS += tests
#SUBDIRS += record
endif

#SUBDIRS += tests/encTests


all: 
	for i in $(SUBDIRS) ; do ( cd $$i && $(MAKE) $@ ) ; done

clean: 
	rm -rf shared_*
	rm -rf pkg
	rm -rf Linux.bin.* 
	- find . -name Linux.obj.* -prune -exec rm -r -f {} \;
	rm -rf ffmpeg_*.tar.gz

#	- find . -name Linux.obj.$(ARCH) -prune -exec rm -r -f {} \;
	
install:all
	@$(SHELL) $(ROOT_DIR)/buildver.sh $(ROOT_DIR)
	@$(SHELL) $(ROOT_DIR)/install.sh $(ROOT_DIR)/Linux.bin.$(ARCH) $(ROOT_DIR)/releases  

package:clean
	cd ..; tar -cvjf ffmpeg.arm.$(BUILDTIME).tar.bz2 ffmpeg
			