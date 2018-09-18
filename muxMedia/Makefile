# $Id$

# released or debug version, different on debug and log info£¬2007.03.15
# must be 'release' or 'debug'
EDITION=debug
EDITION=release

# default: build all as plugins
ifeq ($(PLUGINS),NO)

else
	PLUGINS=YES
endif


ROOT_DIR:=$(shell pwd)
RULE_DIR:=$(ROOT_DIR)/../

# conditional build options: X86/ARM, Player/Server/Web


export ROOT_DIR
export RULE_DIR

export HISI_LINUX_ENV=ok
export EDITION
export PLUGINS


ifeq ($(CFG_HI_EXPORT_FLAG),)
NEW_SDK_DIR := $(NEW_SDK)
SDK_DIR := $(NEW_SDK_DIR)
include $(NEW_SDK_DIR)/base.mak
endif

RELEASES_NAME=muxMedia_$(GCC_VERSION)_$(ARCH)_$(EDITION)_$(BUILDTIME).zip
#.tar.gz  

export ARCH
export BUILDTIME
export RELEASES_NAME

include $(RULE_DIR)/Rules.mak

	SHARED_DIRS += shared/src/common
	SHARED_DIRS += shared/src/media
	SHARED_DIRS += shared/src/mux
	SHARED_DIRS += shared/programs/api

	WEB_ADMIN_DIRS += cgi/lib
	WEB_ADMIN_DIRS += cgi/modules/navigate
	WEB_ADMIN_DIRS += cgi/modules/mux
	WEB_ADMIN_DIRS += cgi/modules/sysInfo


	SUBDIRS += \
			$(SHARED_DIRS)


ifeq ($(PLUGINS),YES)
	SUBDIRS += src
	SUBDIRS += src/recorder
#	SUBDIRS += src/server
endif
	
#	SUBDIRS += src/web




ifeq ($(ARCH),arm)
#	SUBDIRS += src/player/libRx
	SUBDIRS += src/player
else
endif

		TEST_DIRS	+= src/player/libRx
#		TEST_DIRS	+= tests/muxWins
		TEST_DIRS	+= tests/players
#		TEST_DIRS	+= tests/higo
#		TEST_DIRS	+= tests/code

#		TEST_DIRS	+= tests/muxPlays

#		TEST_DIRS += tests/board

# SUBDIRS += $(TEST_DIRS)

#	SUBDIRS += $(WEB_ADMIN_DIRS)


all: 
	for i in $(SUBDIRS) ; do ( cd $$i && $(MAKE) $@ ) ; done

clean: 
	rm -rf muxMedia_*
	rm -rf pkg
	rm -rf Linux.bin.* 
	rm -rf *.ts
	rm -rf *.m3u8
	rm -rf *.log
	- find . -name Linux.obj.* -prune -exec rm -r -f {} \;
	
# all	
install:
	@$(SHELL) $(ROOT_DIR)/buildver.sh $(ROOT_DIR)
	@$(SHELL) $(ROOT_DIR)/install.sh $(ROOT_DIR)/Linux.bin.$(ARCH) $(ROOT_DIR)/releases  

#	@$(SHELL) $(ROOT_DIR)/buildver.sh $(ROOT_DIR)

package:clean
	cd ..; tar -cvjf muxMedia.$(BUILDTIME).tar.bz2 muxMedia
			
