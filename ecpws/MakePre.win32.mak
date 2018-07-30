# Modified in Nov.04, 2015, Zhijie Li
# Add conditional build for /MT, /MTd, /MD and /MDd


!include <Win32.Mak>


MAKEEXE = nmake
IMPLIB  = lib
CC      = cl
LIBU    = lib
LINK    = link
RC      = rc
MC      = mc
HC      = start /wait hcrtf


hcvars = -xn

# Common compiler flags:
#   -c   - compile without linking
#   -W3  - Set warning level to level 3    (-W4 for 64-bit compilations)
#   -Zi  - generate debugging information
#   -Od  - disable all optimizations
#   -Ox  - use maximum optimizations
#   -Zd  - generate only public symbols and line numbers for debugging
#   -GS  - enable security checks
#
# i386 specific compiler flags:
#   -Gz  - stdcall   (only if scall is added to makefile's compiler build rules)

# cdebug = -Gh -Ox -DNDEBUG

#cdebug = $(cdebug) -I./include 

conlflags = $(lflags) -subsystem:console,$(EXEVER)
guilflags = $(lflags) -subsystem:windows,$(EXEVER)
dlllflags = $(lflags) -entry:_DllMainCRTStartup$(DLLENTRY) -dll


# basic subsystem specific libraries, less the C Run-Time
baselibs    = kernel32.lib $(optlibs) $(winsocklibs) advapi32.lib
winlibs     = $(baselibs) user32.lib gdi32.lib comdlg32.lib winspool.lib

# for Windows applications that use the C Run-Time libraries
conlibs     = $(baselibs)
guilibs     = $(winlibs)

# for OLE applications
olelibs     = ole32.lib uuid.lib oleaut32.lib $(guilibs)


rcflags = $(rcflags) /Iinclude /D_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS
cflags = $(cflags) /Iinclude 
# /D_CRT_SECURE_NO_WARNINGS  /DUNICODE /D_UNICODE /DUNICODE /D_UNICODE 


OBJDIR=$(OUTDIR)\$(SRC_DIR)

# Link options
# all command and macro can not be indented 
!IF "$(TARGET_TYPE)" == "CUI"	# build console executable
LINK_FLAGS=$(conlflags) $(conlibsdll)
LINK_LIBS=$(conlibs)
!ELSE
!IF "$(TARGET_TYPE)" == "GUI"	# build windows executable
LINK_FLAGS=$(guiflags)
LINK_LIBS=$(guilibs)
!IF "$(TARGET_TYPE)" == "DLL" # build DLL library
LINK_FLAGS=$(dlllflags)
LINK_LIBS=$(guilibsdll)
!ELSE # build static library
LINK_FLAGS=
LINK_LIBS=
!ENDIF
!ENDIF
!ENDIF


# compile options

#!IFDEF NODEBUG
!IF "$(NODEBUG)"==""
!IF "$(DLL_SUPPORT)"=="YES"
cflags   = $(cflags) -D_MT -D_DLL -MDd /D_DEBUG 
!ELSE
cflags    = $(cflags) -D_MT -MTd /D_DEBUG 
!ENDIF
!ELSE
!IF "$(DLL_SUPPORT)"=="YES"
cflags   = $(cflags)  -D_MT -D_DLL -MD
!ELSE
cflags    = $(cflags) -D_MT -MT
!ENDIF
!ENDIF



#inferencal rule:
# 1: defination with directory
# 2: only target of object is defined when using these rules
#{tests}.c{$(OBJDIR)}.obj:
#    $(cc) $(cflags) $(cdebug) $(cvars) /Fo"$(OBJDIR)\\" /Fd"$(OBJDIR)\\" $**

#!if "$(LOG)"=="YES"
#    @echo $(CC) $(CL) $(crtflags) $(cDefines) $< $(LogCmd)
#!endif

# Second, the suffix rule
# C++ Targets $(cdebug) $(cdebug) 
{$(SRC_DIR)}.cpp{$(OBJDIR)}.obj:
    $(cc) $(cflags) /Fo"$(OBJDIR)\\" /Fd"$(OBJDIR)\\" /Fd"$(OBJDIR)\\" $**
    
{$(SRC_DIR)}.c{$(OBJDIR)}.obj:
    $(cc) $(cflags) /Fo"$(OBJDIR)\\" /Fd"$(OBJDIR)\\" /Fd"$(OBJDIR)\\" $**


#$(OUTDIR)\$(EXE_NAME).res: $(EXE_NAME).rc $(EXE_NAME).ico
#    $(rc) $(rcflags) $(rcvars) /fo $@ $(EXE_NAME).rc
    
{$(SRC_DIR)}.rc{$(OBJDIR)}.res:
    $(rc) $(rcflags) $(rcvars) /fo $@ $**


# When some errors,using this as link and compile flags, the detailed info can be displayed..
# /VERBOSE  
#
MY_DATE=%date:~0,4%.%date:~5,2%.%date:~8,2%
