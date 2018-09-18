#
# $Id: Makefile,v 1.1.1.1 2006/11/29 18:13:58 lizhijie Exp $
#


include $(RULE_DIR)/Makefile.pre


CODE_SUBDIRS += \
			http \
			cgi \

		

CFLAGS += $(SHARED_CFLAGS)
################################################
####   source file 
################################################



WEB_SRC += \
			cgi/muxWebCgiCreate.c \
			cgi/muxWebCgiHandlers.c \
			cgi/muxWebCgiInit.c \
			http/muxWebHttpFsm.c \
			http/muxWebHttpHandlers.c \
			http/muxWebHttpParseRequest.c \
			http/muxWebHttpReadRequest.c \
			http/muxWebHttpResponse.c \
			http/muxWebHttpStaticContent.c \
			muxWebAlias.c \
			muxWebBuffer.c \
			muxWebHash.c \
			muxWebLog.c \
			muxWebNetService.c \
			muxWebRange.c \
			muxWebShadow.c \
			muxWebUtils.c \
			muxWebMain.c \
			muxWebInit.c \
			muxWebJSon.c \

#			muxWebConfig.c \


ifeq ($(ARCH),arm)
	SRC += \
	
else
	SRC += \

endif

SRC += \
			$(WEB_SRC)  \


ifeq ($(PLUGINS),YES)
	TARGET_LIB:=MuxPluginWeb
	TARGET_LIB_SHARED:=$(TARGET_LIB).so
	TARGET_LIB_STATIC:=$(TARGET_LIB).a
else
	LOCAL_ALL := muxWeb          
endif

include $(ROOT_DIR)/Makefile.post

LDFLAGS := -L$(BIN_DIR) -L$(BIN_DIR)/lib \
		$(LDFLAGS)



LIBS += -lm -lpthread -lrt -ldl -lcrypt

# -Wl,-verbose -Wl,-static  -s 
$(LOCAL_ALL):$(OBJS)
		$(CC) -s -o $@ $(OBJS) -L$(ROOT_DIR)/Linux.bin.$(ARCH)/lib -lz  $(LIBS) $(LIB_LDFLAGS) -lShared
		$(MOVE) $@ $(BIN_DIR)/sbin
