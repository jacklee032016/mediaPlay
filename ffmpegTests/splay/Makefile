#
# $Id: Makefile,v 1.1.1.1 2006/11/29 17:06:59 lizhijie Exp $
#

include $(ROOT_DIR)/Makefile.pre

CODE_SUBDIRS = .

# name of all files to be compiled (don't include header files)
SRC += \
		simPlay.c  \
		simPlayQueue.c  \
		simPlayWin.c  \
		simPlayer02.c  \
		simPlayer03.c  \
		simPlayer04.c  \
		simPlayer05.c  \
		simPlayer06.c  \


OTHER_SRC += \
		sdl01.c \
		sdlAudio01.c  \
		sdlAudio02.c  \


C_FLAGS += -I$(ROOT_DIR)/programs/libcmd -DARCH_X86 -DARCH_X86_32 

	 
LOCAL_ALL:= \
			sp2 \
			sp4 \
			sp5 \
			sp6 \




OTHER := \
			sp3 \
			t1 \
			t2 \
			t3 \
			s1 \
			sa1 \
			sa2 \


include $(ROOT_DIR)/Makefile.post

LDFLAGS +=
#-lcmdutil -lavdevice 

s1:$(OBJ_DIR)/sdl01.o
	$(CC) -s  $^ -o $@ -lSDL2
	$(MOVE) $@ $(BIN_DIR)/usr/bin   


sp2:$(OBJ_DIR)/simPlayer02.o
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

sa1:$(OBJ_DIR)/sdlAudio01.o
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

sa2:$(OBJ_DIR)/sdlAudio02.o
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

sp3:$(OBJ_DIR)/simPlayer03.o $(OBJ_DIR)/simPlay.o $(OBJ_DIR)/simPlayQueue.o $(OBJ_DIR)/simPlayWin.o
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

sp4:$(OBJ_DIR)/simPlay.o $(OBJ_DIR)/simPlayer04.o $(OBJ_DIR)/simPlayQueue.o $(OBJ_DIR)/simPlayWin.o
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

sp5:$(OBJ_DIR)/simPlay.o $(OBJ_DIR)/simPlayer05.o $(OBJ_DIR)/simPlayQueue.o $(OBJ_DIR)/simPlayWin.o
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

sp6:$(OBJ_DIR)/simPlay.o $(OBJ_DIR)/simPlayer06.o $(OBJ_DIR)/simPlayQueue.o $(OBJ_DIR)/simPlayWin.o
	$(CC) -s  $^ -o $@ $(LDFLAGS)
	$(MOVE) $@ $(BIN_DIR)/usr/bin   
