#$Id: Makefile,v 1.1.1.1 2006/11/29 17:51:53 lizhijie Exp $

ROOT_DIR:=$(shell pwd)
export ROOT_DIR

ASSIST_IXP_RELEASES_HOME=/works/ixp_releases
#BUILDTIME := $(shell TZ=UTC date -u "+%Y_%m_%d-%H_%M")
BUILDTIME := $(shell TZ=CN date -u "+%Y_%m_%d")
ASSIST_RELEASES_NAME=assist_boa_$(BUILDTIME).tar.gz
export BUILDTIME
export ASSIST_RELEASES_NAME

# 这个库不再需要，直接使用libc的用户管理库
SUBDIRS += src/libpwd
SUBDIRS += src 
		     

all: 
	for i in $(SUBDIRS) ; do ( cd $$i && $(MAKE) $@ ) ; done

clean: 
	rm -rf assist_boa_*
	rm -rf pkg
	for i in $(SUBDIRS) ; do ( cd $$i && $(MAKE) $@ ) ; done

cleancdr:
	for i in $(SUBDIRS) ; do ( cd $$i && $(MAKE) $@ ) ; done
	
install:
	@$(SHELL) $(ROOT_DIR)/installboa.sh $(ROOT_DIR)/Linux.bin.arm $(ROOT_DIR)/releases

#	rm -rf assist_vsftpd_*
#	cp -R $(ROOT_DIR)/releases/* Linux.bin.arm
#	- find Linux.bin.arm -name \CVS -exec rm -r -f {} \;
#	cd Linux.bin.arm; tar cvzf $(ASSIST_RELEASES_NAME) *; mv $(ASSIST_RELEASES_NAME) ../ 
#	mv $(ASSIST_RELEASES_NAME)  $(ASSIST_IXP_RELEASES_HOME)
#	cd assist && $(MAKE) $@
