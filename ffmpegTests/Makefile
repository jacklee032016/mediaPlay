# $Id$

ROOT_DIR:=$(shell pwd)
export ROOT_DIR

export HISI_LINUX_ENV=ok

ifeq ($(CFG_HI_EXPORT_FLAG),)
NEW_SDK_DIR := $(NEW_SDK)
SDK_DIR := $(NEW_SDK_DIR)
include $(NEW_SDK_DIR)/base.mak
endif

include $(ROOT_DIR)/Rules.mak

RELEASES_NAME=ffmpegTests_$(GCC_VERSION)_$(ARCH)_$(EDITION)_$(BUILDTIME).tar.gz  

export ARCH
export BUILDTIME
export RELEASES_NAME


ifeq ($(ARCH),arm)
SUBDIRS += libavformat
SUBDIRS += libavcodec
SUBDIRS += libavutil
else

# SUBDIRS += examples
#SUBDIRS += tests

 SUBDIRS += programs/libCmd

# SUBDIRS += programs/server
 SUBDIRS += programs/play

#SUBDIRS += programs/ffmpeg
#SUBDIRS += programs/prob
# SUBDIRS += record
endif

# SUBDIRS += tests/encTests

 SUBDIRS += splay

all: 
	for i in $(SUBDIRS) ; do ( cd $$i && $(MAKE) $@ ) ; done

clean: 
	rm -rf shared_*
	rm -rf pkg
	- find . -name Linux.bin.$(ARCH) -prune -exec rm -r -f {} \;
	- find . -name Linux.obj.$(ARCH) -prune -exec rm -r -f {} \;
	
install:all
	@$(SHELL) $(ROOT_DIR)/buildver.sh $(ROOT_DIR)
	@$(SHELL) $(ROOT_DIR)/install.sh $(ROOT_DIR)/Linux.bin.$(ARCH) $(ROOT_DIR)/releases  

package:clean
	cd ..; tar -cvjf ffmpegTests.$(BUILDTIME).tar.bz2 ffmpegTests
			