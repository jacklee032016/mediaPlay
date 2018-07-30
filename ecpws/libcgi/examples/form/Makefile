# 

!include <Win32.Mak>

TARGET_TYPE=CUI

!include MakePre.mak

EXE_NAME=ecpws


# First, the object should be build
ECPWS_OBJS= \
    $(OBJDIR)\cgiForm1.obj    \
    $(OBJDIR)\cgiForm2.obj    \
    $(OBJDIR)\cgiUser.obj    \
    $(OBJDIR)\cgiUpload.obj    \
    $(OBJDIR)\cgiMultiple.obj    \
   
#    $(OBJDIR)\cgiGuestBook.obj    \

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


DEST= $(OUTDIR)\cgiForm1.exe $(OUTDIR)\cgiForm2.exe  \
		 $(OUTDIR)\cgiMultiple.exe $(OUTDIR)\cgiUser.exe $(OUTDIR)\cgiUpload.exe 

#$(OUTDIR)\cgiGuestBook.exe

# Third, the target dependences
all:$(OBJDIR) $(OBJS) $(DEST) 


$(OBJDIR) :
    if not exist "$(OBJDIR)/$(NULL)" mkdir $(OBJDIR)


LIBS = user32.lib $(OUTDIR)\libCgi.lib
# wsock32.lib 

ODD_LIB = kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib \
		ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib 

#$(RESES)
$(OUTDIR)\cgiForm1.exe: $(OBJDIR)\cgiForm1.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\cgiForm2.exe: $(OBJDIR)\cgiForm2.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

#$(OUTDIR)\cgiGuestBook.exe: $(OBJDIR)\cgiGuestBook.obj
#	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\cgiMultiple.exe: $(OBJDIR)\cgiMultiple.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\cgiUser.exe: $(OBJDIR)\cgiUser.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

$(OUTDIR)\cgiUpload.exe: $(OBJDIR)\cgiUpload.obj
	$(link) $(linkdebug) $(LINK_FLAGS) -out:$@ $** $(LINK_LIBS) $(LIBS)

	
!if "$(QUIET)"=="N"
    dumpbin /imports $@
!endif    


#	gdi32.lib 


clean:
        $(CLEANUP)

