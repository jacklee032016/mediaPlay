#$Id$

ROOT_DIR:=$(shell pwd)
export ROOT_DIR


include $(ROOT_DIR)/Rules.mak

ECPWS_RELEASES_NAME=_ecpws_$(GCC_VERSION)_$(EDITION)_$(BUILDTIME).tar.gz

export BUILDTIME
export ECPWS_RELEASES_NAME

SUBDIRS += src \
		libcgi/src

	
all:
	for i in $(SUBDIRS) ; do ( cd $$i && $(MAKE) $@ ) ; done

clean: 
	rm -rf pkg
	rm -rf _mserver_*
	- find . -name Linux.bin.$(ARCH) -prune -exec rm -r -f {} \;
	- find . -name Linux.obj.$(ARCH) -prune -exec rm -r -f {} \;

	
install: all
	@$(SHELL) $(ROOT_DIR)/buildver.sh $(ROOT_DIR)/Linux.bin.$(ARCH)
	@$(SHELL) $(ROOT_DIR)/install.sh $(ROOT_DIR)/Linux.bin.$(ARCH) 


package:clean
	cd ..; tar -cvjf ecpws.$(BUILDTIME).tar.bz2 ecpws

