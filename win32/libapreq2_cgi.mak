# Microsoft Developer Studio Generated NMAKE File, Based on libapreq_cgi.dsp

!IF "$(APACHE)" == ""
!MESSAGE No Apache directory was specified.
!MESSAGE This makefile is not to be run directly.
!MESSAGE Please run Configure.bat, and then $(MAKE) on Makefile.
!ERROR
!ENDIF

!IF "$(CFG)" == ""
CFG=libapreq2_cgi - Win32 Release
!MESSAGE No configuration specified. Defaulting to libapreq2_cgi - Win32 Release.
!ENDIF

!IF "$(CFG)" != "libapreq2_cgi - Win32 Release" && "$(CFG)" != "libapreq2_cgi - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libapreq2_cgi.mak" CFG="libapreq2_cgi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libapreq2_cgi - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libapreq2_cgi - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

CFG_HOME=$(APREQ_HOME)\win32
OUTDIR=$(CFG_HOME)\libs
INTDIR=$(CFG_HOME)\libs

!IF  "$(CFG)" == "libapreq2_cgi - Win32 Release"

ALL : "$(OUTDIR)\libapreq2_cgi.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "LIBAPREQ_CGI_EXPORTS" /D "_WINDOWS" /I"$(APREQ_HOME)\src" /I"$(APACHE)\include" /Fp"$(INTDIR)\libapreq2_cgi.pch" /YX /Fo"$(INTDIR)\libapreq2_cgi.obj" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapreq2_cgi.bsc" 
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /nodefaultlib /out:"$(OUTDIR)\libapreq2_cgi.lib" 
LIB32_OBJS= \
        "$(INTDIR)\libapreq2_cgi.obj" \
        "$(INTDIR)\libapreq2.lib" \
        "$(APACHE)\lib\libapr.lib"

"$(OUTDIR)\libapreq2_cgi.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libapreq2_cgi - Win32 Debug"

ALL : "$(OUTDIR)\libapreq2_cgi.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "LIBAPREQ_CGI_EXPORTS" /D "_LIB" /I"$(APREQ_HOME)\src" /I"$(APACHE)\include" /Fp"$(INTDIR)\libapreq2_cgi.pch" /YX /Fo"$(INTDIR)\libapreq2_cgi.obj" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapreq2_cgi.bsc" 
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libapreq2_cgi.lib" 
LIB32_OBJS= \
        "$(INTDIR)\libapreq2_cgi.obj" \
        "$(INTDIR)\libapreq2.lib" \
        "$(APACHE)\lib\libapr.lib"

"$(OUTDIR)\libapreq2_cgi.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(CFG)" == "libapreq2_cgi - Win32 Release" || "$(CFG)" == "libapreq2_cgi - Win32 Debug"
SOURCE=$(APREQ_HOME)\env\libapreq_cgi.c

"$(INTDIR)\libapreq2_cgi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

