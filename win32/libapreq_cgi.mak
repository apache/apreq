# Microsoft Developer Studio Generated NMAKE File, Based on libapreq_cgi.dsp

!IF "$(APACHE)" == ""
!MESSAGE No Apache directory was specified.
!MESSAGE This makefile is not to be run directly.
!MESSAGE Please run Configure.bat, and then $(MAKE) on Makefile.
!ERROR
!ENDIF

!IF "$(CFG)" == ""
CFG=libapreq_cgi - Win32 Release
!MESSAGE No configuration specified. Defaulting to libapreq_cgi - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "libapreq_cgi - Win32 Release" && "$(CFG)" != "libapreq_cgi - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libapreq_cgi.mak" CFG="libapreq_cgi - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libapreq_cgi - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libapreq_cgi - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libapreq_cgi - Win32 Release"

OUTDIR=.\libs
INTDIR=.\libs
# Begin Custom Macros
OutDir=.\libs
# End Custom Macros

ALL : "$(OUTDIR)\libapreq_cgi.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBAPREQ_CGI_EXPORTS" /I"$(APACHE)\include" /I"..\src" /Fp"$(INTDIR)\libapreq_cgi.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapreq_cgi.bsc" 
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /def:libapreq_cgi.def /machine:I386 /out:"$(OUTDIR)\libapreq_cgi.so" /implib:"$(OUTDIR)\libapreq_cgi.lib" 
LINK32_OBJS= \
	"$(INTDIR)\libapreq_cgi.obj" \
	"$(OUTDIR)\libapreq.lib" \
	"$(APACHE)\lib\libapr.lib" \
	"$(APACHE)\lib\libaprutil.lib"

"$(OUTDIR)\libapreq_cgi.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libapreq_cgi - Win32 Debug"

OUTDIR=.\libs
INTDIR=.\libs
# Begin Custom Macros
OutDir=.\libs
# End Custom Macros

ALL : "$(OUTDIR)\libapreq_cgi.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBAPREQ_CGI_EXPORTS" /I"$(APACHE)\include"  /I"..\src" /Fp"$(INTDIR)\libapreq_cgi.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ  /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapreq_cgi.bsc" 
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /def:libapreq_cgi.def /pdb:"$(OUTDIR)\libapreq_cgi.pdb" /debug /machine:I386 /out:"$(OUTDIR)\libapreq_cgi.so" /implib:"$(OUTDIR)\libapreq_cgi.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\libapreq_cgi.obj" \
	"$(OUTDIR)\libapreq.lib" \
	"$(APACHE)\lib\libapr.lib" \
	"$(APACHE)\lib\libaprutil.lib"

"$(OUTDIR)\libapreq_cgi.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
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


!IF "$(CFG)" == "libapreq_cgi - Win32 Release" || "$(CFG)" == "libapreq_cgi - Win32 Debug"
SOURCE=..\env\libapreq_cgi.c

"$(INTDIR)\libapreq_cgi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

