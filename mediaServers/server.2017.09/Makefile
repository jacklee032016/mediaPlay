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

include $(RULE_DIR)/Rules.mak

RELEASES_NAME=muxServer_$(GCC_VERSION)_$(ARCH)_$(EDITION)_$(BUILDTIME).tar.gz  

export ARCH
export BUILDTIME
export RELEASES_NAME


	SUBDIRS += src

ifeq ($(ARCH),arm)
	

else

endif


all: 
	for i in $(SUBDIRS) ; do ( cd $$i && $(MAKE) $@ ) ; done

clean: 
	rm -rf muxServer_*
	rm -rf pkg
	rm -rf Linux.bin.* 
	rm -rf *.ts
	rm -rf *.m3u8
	- find . -name Linux.obj.* -prune -exec rm -r -f {} \;
	
install:all
	@$(SHELL) $(ROOT_DIR)/install.sh $(ROOT_DIR)/Linux.bin.$(ARCH) $(ROOT_DIR)/releases  

#	@$(SHELL) $(ROOT_DIR)/buildver.sh $(ROOT_DIR)

package:clean
	cd ..; tar -cvjf muxServer.$(BUILDTIME).tar.bz2 muxServer
			