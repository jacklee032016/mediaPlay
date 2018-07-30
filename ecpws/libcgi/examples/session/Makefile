# 

!include <Win32.Mak>

TARGET_TYPE=CUI

!include MakePre.mak

EXE_NAME=ecpws


# First, the object should be build
ECPWS_OBJS= \
    $(OBJDIR)\cgiCookie.obj    \
    $(OBJDIR)\cgiSession2.obj    \
    $(OBJDIR)\sessionMain.obj    \
    $(OBJDIR)\sessionLogin.obj    \
    $(OBJDIR)\sessionDestroy.obj    \
    $(OBJDIR)\sessionRestricted.obj    \
    $(OBJDIR)\sessionShowVars.obj    \
   
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


DEST= $(OUTDIR)\cgiSession2.exe $(OUTDIR)\sessionMain.exe $(OUTDIR)\sessionLogin.exe \
		  $(OUTDIR)\sessionRestricted.exe $(OUTDIR)\sessionShowVars.exe \
		 $(OUTDIR)\cgiCookie.exe

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
$(OUTDIR)\cgiCookie.exe: $(OBJDIR)\cgiCookie.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\cgiSession2.exe: $(OBJDIR)\cgiSession2.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\sessionMain.exe: $(OBJDIR)\sessionMain.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\sessionLogin.exe: $(OBJDIR)\sessionLogin.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\sessionDestroy.exe: $(OBJDIR)\sessionDestroy.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\sessionRestricted.exe: $(OBJDIR)\sessionRestricted.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\sessionShowVars.exe: $(OBJDIR)\sessionShowVars.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

	
!if "$(QUIET)"=="N"
    dumpbin /imports $@
!endif    


#	gdi32.lib 


clean:
        $(CLEANUP)

