# 

!include <Win32.Mak>

TARGET_TYPE=CUI

!include MakePre.mak

EXE_NAME=ecpws


# First, the object should be build
ECPWS_OBJS= \
    $(OBJDIR)\cgiB64.obj    \
    $(OBJDIR)\cgiExplode.obj    \
    $(OBJDIR)\cgiHtmlEntities.obj    \
    $(OBJDIR)\cgiMd5.obj    \
    $(OBJDIR)\cgiRedirect.obj    \
   
ODD = \

  
OBJS = \
		$(ECPWS_OBJS)	\
		
		

!if "$(TARGET_TYPE)" == "GUI"
RESES= \
    $(OBJDIR)\$(EXE_NAME).res    
!else
RESES= 
!endif


{$(SRC_DIR)\http\}.c{$(HTTP_OBJS_DIR)\}.obj:
    $(cc) $(cflags) $(cdebug) /Fo"$(HTTP_OBJS_DIR)\\" /Fd"$(HTTP_OBJS_DIR)\\" /Fd"$(HTTP_OBJS_DIR)\\" $**


DEST= $(OUTDIR)\cgiB64.exe $(OUTDIR)\cgiExplode.exe $(OUTDIR)\cgiHtmlEntities.exe \
		 $(OUTDIR)\cgiMd5.exe  $(OUTDIR)\cgiRedirect.exe 

#

# Third, the target dependences
all:$(OBJDIR) $(OBJS) $(DEST) 


$(OBJDIR) :
    if not exist "$(OBJDIR)/$(NULL)" mkdir $(OBJDIR)


LIBS = user32.lib $(OUTDIR)\libCgi.lib
# wsock32.lib 

ODD_LIB = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib \
		ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib 

#$(RESES)
$(OUTDIR)\cgiB64.exe: $(OBJDIR)\cgiB64.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\cgiExplode.exe: $(OBJDIR)\cgiExplode.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\cgiHtmlEntities.exe: $(OBJDIR)\cgiHtmlEntities.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\cgiMd5.exe: $(OBJDIR)\cgiMd5.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\cgiRedirect.exe: $(OBJDIR)\cgiRedirect.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)


	
!if "$(QUIET)"=="N"
    dumpbin /imports $@
!endif    


#	gdi32.lib 


clean:
        $(CLEANUP)

