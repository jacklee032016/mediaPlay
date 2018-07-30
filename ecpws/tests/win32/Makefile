# 

!include <Win32.Mak>

TARGET_TYPE=CUI

!include MakePre.mak

EXE_NAME=ecpws


# First, the object should be build
ECPWS_OBJS= \
    $(OBJDIR)\win32CreateProcess.obj    \
    $(OBJDIR)\win32Setjmp.obj    \
    $(OBJDIR)\win32Environment.obj    \
    
ODD = \

  
OBJS = \
		$(ECPWS_OBJS)	\
		$(HTTP_OBJS)	\
		$(CGI_OBJS)	\
		$(INDEX_OBJS)	\
		$(WINDOWS_OBJS)	\
		
		

!if "$(TARGET_TYPE)" == "GUI"
RESES= \
    $(OBJDIR)\$(EXE_NAME).res    
!else
RESES= 
!endif


{$(SRC_DIR)\http\}.c{$(HTTP_OBJS_DIR)\}.obj:
    $(cc) $(cflags) $(cdebug) /Fo"$(HTTP_OBJS_DIR)\\" /Fd"$(HTTP_OBJS_DIR)\\" /Fd"$(HTTP_OBJS_DIR)\\" $**


DEST= $(OUTDIR)\win32CreateProcess.exe $(OUTDIR)\win32Setjmp.exe $(OUTDIR)\win32Environment.exe 

#

# Third, the target dependences
all:$(OBJDIR) $(OBJS) $(DEST) 


$(OBJDIR) :
    if not exist "$(OBJDIR)/$(NULL)" mkdir $(OBJDIR)


LIBS = user32.lib 
# wsock32.lib 

ODD_LIB = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib \
		ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib 

#$(RESES)
$(OUTDIR)\win32CreateProcess.exe: $(OBJDIR)\win32CreateProcess.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\win32Setjmp.exe: $(OBJDIR)\win32Setjmp.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\win32Environment.exe: $(OBJDIR)\win32Environment.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)


	
!if "$(QUIET)"=="N"
    dumpbin /imports $@
!endif    


#	gdi32.lib 


clean:
        $(CLEANUP)

