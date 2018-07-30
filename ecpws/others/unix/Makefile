# $Log: Makefile,v $
# Revision 1.1.1.1  2006/11/29 17:51:53  lizhijie
# CommonProgram Boa
#
# Revision 1.1.1.1  2006/07/10 08:53:53  lizhijie
# rebuild BOA
#
# Revision 1.1.1.1  2005/01/15 08:08:23  lizhijie
# e2fs-programs
#
# $Id: Makefile,v 1.1.1.1 2006/11/29 17:51:53 lizhijie Exp $

include $(ROOT_DIR)/Makefile.pre

C_FLAGS := -g -O2 -pipe -Wall -I.

CODE_SUBDIRS = . 


SRC = \
	alias.c \
	boa.c \
	buffer.c \
	cgi.c \
	cgi_header.c \
	config.c \
	escape.c \
	get.c \
	hash.c \
	ip.c \
	log.c \
	mmap_cache.c \
	pipe.c \
	queue.c \
	range.c \
	read.c \
	request.c \
	response.c \
	signals.c \
	util.c \
	sublog.c \
	select.c \
	baseAuthen.c \
	shadow.c \
	timestamp.c 

SPROGS=	boa boa_indexer

LOCAL_ALL=$(SPROGS)


LOCAL_CLEAN:=clear

$(LOCAL_CLEAN):
	
LIBS:= -lboa_pwd -lcrypt

include $(ROOT_DIR)/Makefile.post

boa:$(OBJS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) 
	$(STRIP) $@
	$(MOVE) $@ $(BIN_DIR)/usr/bin

boa_indexer:index_dir.c  $(OBJ_DIR)/escape.o 
	$(CC) $(LDFLAGS) -o $@ $^ 
	$(STRIP) $@
	$(MOVE) $@ $(BIN_DIR)/usr/bin
	
