# 

!include <Win32.Mak>

TARGET_TYPE=CUI

!include MakePre.mak

EXE_NAME=ecpws


# First, the object should be build
ECPWS_OBJS= \
    $(OBJDIR)\simplestCgi.obj    \

  
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


DEST= $(OUTDIR)\simplestCgi.exe 

# Third, the target dependences
all:$(OBJDIR) $(OBJS) $(DEST) $(OUTDIR)\simplestCgi.exe


$(OBJDIR) :
    if not exist "$(OBJDIR)/$(NULL)" mkdir $(OBJDIR)


LIBS = user32.lib 
# wsock32.lib 

ODD_LIB = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib \
		ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib 

#$(RESES)
$(OUTDIR)\simplestCgi.exe: $(ECPWS_OBJS) $(HTTP_OBJS) $(CGI_OBJS) $(WINDOWS_OBJS) 
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)


	
!if "$(QUIET)"=="N"
    dumpbin /imports $@
!endif    


#	gdi32.lib 


clean:
        $(CLEANUP)

