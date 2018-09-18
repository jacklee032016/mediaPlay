#
# $Id$
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = .

C_FLAGS += $(SHARED_CFLAGS) -I$(ROOT_DIR)/cgi/include

# name of all files to be compiled (don't include header files)
SRC += \
				cgiMainNavigate.c \
				cgiMainContent.c	\

CGIS =	navigate.cgi content.cgi 
# index.cgi
		 
LOCAL_ALL:=$(CGIS) 

include $(ROOT_DIR)/Makefile.post

C_FLAGS +=
# -fPIC 
LDFLAGS := -lCgi -lMux -lMedia -lShared -lrt $(LDFLAGS) $(SHARED_LDFLAGS) -L$(HI_SHARED_LIB_DIR) -lm -ldl -lz

navigate.cgi:$(OBJ_DIR)/cgiMainNavigate.o
	$(CC) -o $@ $^ $(LDFLAGS)
	$(STRIP) $@
	$(MOVE) $@ $(CGI_BIN_DIR)   

content.cgi:$(OBJ_DIR)/cgiMainContent.o
	$(CC) -o $@ $^ $(LDFLAGS) 
	$(STRIP) $@
	$(MOVE) $@ $(CGI_BIN_DIR)  

# index.cgi:index_cgi.c
#	$(CC) -o $@ $^ $(LDFLAGS) 
#	$(STRIP) $@
#	$(MOVE) $@ $(CGI_BIN_DIR)  
