#
#


include $(RULE_DIR)/Makefile.pre

CODE_SUBDIRS = .
			
# name of all files to be compiled (don't include header files)

SRC += \
		higo_animation.c \
    higo_bitmapfont.c \
    higo_dec.c \
    higo_effect.c \
    higo_encoder.c \
    higo_fillrect.c \
    higo_gdev.c \
    higo_logo.c \
    higo_performance.c \
    higo_surface.c \
    higo_wm.c \
    higo_text.c \
		higo_stereo.c \
 		sample_scrolltext.c \
   
ifeq ($(CFG_HIFB_SCROLLTEXT_SUPPORT),y)
SAMPLE_IMAGES += higo_scrolltext
endif

ifeq ($(CFG_HIFB_STEREO3D_SUPPORT),y)
endif


CFLAGS += -I$(ROOT_DIR)/arm/include $(SHARED_CFLAGS) $(HI_CFLAGS)  -I$(FFMPEG_HOME)

		 
LOCAL_ALL:=	\
			higo.animation \
			higo.bitmapfont \
			higo.dec \
			higo.effect \
			higo.encoder \
			higo.fillrect \
			higo.gdev \
			higo.logo \
			higo.performance \
			higo.surface \
			higo.wm \
			higo.text \
			higo.stereo \
			higo.scrolltext \
			

include $(ROOT_DIR)/Makefile.post


LIBS :=       
           
ifeq ($(ARCH),arm)
	LIBS += -lMuxRx -lMux -lMedia -lShared -lhi_common -lpng -lhigo -lhigoadp -lpng -lhi_msp -lfreetype -ljpeg -lhi_resampler -lz -lhi_subtitle -lhi_so -lhi_ttx -lhi_cc  -leffect -lrt -ldl
endif
#-lplayer -lhi_charsetMgr -lsubdec -lhiplayer_utils 

LIBS += -lm -lpthread -lrt -ldl $(LIBS)  -L$(HI_STATIC_LIB_DIR) -L$(HI_EXTERN_LIB_DIR) -lstdc++ 


higo.animation:$(OBJ_DIR)/higo_animation.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.bitmapfont:$(OBJ_DIR)/higo_bitmapfont.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.dec:$(OBJ_DIR)/higo_dec.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.effect:$(OBJ_DIR)/higo_effect.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.encoder:$(OBJ_DIR)/higo_encoder.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.fillrect:$(OBJ_DIR)/higo_fillrect.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.gdev:$(OBJ_DIR)/higo_gdev.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.logo:$(OBJ_DIR)/higo_logo.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.performance:$(OBJ_DIR)/higo_performance.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.surface:$(OBJ_DIR)/higo_surface.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.wm:$(OBJ_DIR)/higo_wm.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.text:$(OBJ_DIR)/higo_text.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.stereo:$(OBJ_DIR)/higo_stereo.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   

higo.scrolltext:$(OBJ_DIR)/higo_scrolltext.o
	$(CC) -s -o $@ $^ $(LIBS) $(LDFLAGS) 
	$(MOVE) $@ $(BIN_DIR)/usr/bin   
