# 

!include <Win32.Mak>

TARGET_TYPE=CUI
#GUI

EXE_NAME=indexDir

# First, the object should be build
OBJS= \
    $(OBJDIR)\indexDir.obj    \

!if "$(TARGET_TYPE)" == "GUI"
RESES= \
    $(OBJDIR)\$(EXE_NAME).res    
!else
RESES= 
!endif


DEST=$(OUTDIR)\$(EXE_NAME).exe

# Second, Suffix and rules
!include MakePre.mak
    

# Third, the target dependences
all: $(OBJDIR) $(OBJS) $(DEST) $(OUTDIR)\indexDir.exe 

$(OBJDIR) :
    if not exist "$(OBJDIR)/$(NULL)" mkdir $(OBJDIR)

# $@ : full name of current target
# $** : the names of all the dependent files $(LINK_LIBS)
$(DEST): $(RESES) $(OBJDIR)\$(EXE_NAME).obj 
	$(link) /out:$@ $(IMLIBS) $** advapi32.lib
!if "$(QUIET)"=="N"
    dumpbin /imports $@
!endif    

$(OUTDIR)\indexDir.exe:$(OBJDIR)\indexDir.obj
	$(link) /out:$@ $(IMLIBS) $** advapi32.lib

$(OUTDIR)\testCryptoHashHMac.exe:$(OBJDIR)\testCryptoHashHMac.obj
	$(link) /out:$@ $(IMLIBS) $** advapi32.lib

$(OUTDIR)\testCryptoBlocks.exe:$(OBJDIR)\testCryptoBlocks.obj
	$(link) /out:$@ $(IMLIBS) $** advapi32.lib

$(OUTDIR)\testCryptoStreams.exe:$(OBJDIR)\testCryptoStreams.obj
	$(link) /out:$@ $(IMLIBS) $** advapi32.lib

$(OUTDIR)\testCryptoPublicKey.exe:$(OBJDIR)\testCryptoPublicKey.obj
	$(link) /out:$@ $(IMLIBS) $** advapi32.lib Ws2_32.lib

$(OUTDIR)\testCryptoMisc.exe:$(OBJDIR)\testCryptoMisc.obj
	$(link) /out:$@ $(IMLIBS) $** advapi32.lib

#	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(OUTDIR)\.lib advapi32.lib

clean:
        $(CLEANUP)
