#
# $Id$
#

include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = .

C_FLAGS += $(SHARED_CFLAGS) -I$(ROOT_DIR)/cgi/include

# name of all files to be compiled (don't include header files)
SRC += \
				cgiSysinfoTime.c \
				cgiSysinfoServicesCfg.c \
				cgiSysinfoVersions.c \
				cgiSysinfoProcs.c \
				cgiSysinfoReboot.c \
				cgiSysinfoMain.c 


LDFLAGS := -lCgi -lMux -lMedia -lShared -lrt $(LDFLAGS) $(SHARED_LDFLAGS) $(HI_LIB_PATHS) -lm -ldl -lz

CGIS = \
	sysInfo.cgi 

		 
LOCAL_ALL:=$(CGIS) 

include $(ROOT_DIR)/Makefile.post


sysInfo.cgi:$(OBJS)  
	$(CC) -o $@ $^ $(LDFLAGS)
	$(STRIP) $@
	$(MOVE) $@ $(CGI_BIN_DIR)  


