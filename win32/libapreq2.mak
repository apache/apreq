# Microsoft Developer Studio Generated NMAKE File, Based on libapreq.dsp

!IF "$(APACHE)" == ""
!MESSAGE No Apache directory was specified.
!MESSAGE This makefile is not to be run directly.
!MESSAGE Please use Perl Makefile.PL, and then $(MAKE) on Makefile.
!ERROR
!ENDIF

!IF "$(APR_LIB)" == ""
!MESSAGE No apr lib was specified.
!MESSAGE This makefile is not to be run directly.
!MESSAGE Please use Perl Makefile.PL, and then $(MAKE) on Makefile.
!ERROR
!ENDIF

!IF "$(APU_LIB)" == ""
!MESSAGE No aprutil lib was specified.
!MESSAGE This makefile is not to be run directly.
!MESSAGE Please use Perl Makefile.PL, and then $(MAKE) on Makefile.
!ERROR
!ENDIF

!IF "$(APACHE)" == ""
!MESSAGE No Apache directory was specified.
!MESSAGE This makefile is not to be run directly.
!MESSAGE Please use Perl Makefile.PL, and then $(MAKE) on Makefile.
!ERROR
!ENDIF

!IF "$(CFG)" == ""
CFG=libapreq2 - Win32 Release
!MESSAGE No configuration specified. Defaulting to libapreq2 - Win32 Release.
!ENDIF

!IF "$(CFG)" != "libapreq2 - Win32 Release" && "$(CFG)" != "libapreq2 - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libapreq2.mak" CFG="libapreq2 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libapreq2 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libapreq2 - Win32 Debug" (based on "Win32 (x86) Static Library")
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

!IF  "$(CFG)" == "libapreq2 - Win32 Release"

ALL : "$(OUTDIR)\libapreq2.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /I"$(APACHE)\include" /I"$(APREQ_HOME)\src" /Fp"$(INTDIR)\libapreq2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapreq2.bsc" 
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /machine:I386 /out:"$(OUTDIR)\libapreq2.dll" /implib:"$(OUTDIR)\libapreq2.lib" 
LINK32_OBJS= \
	"$(INTDIR)\apreq.obj" \
	"$(INTDIR)\apreq_cookie.obj" \
	"$(INTDIR)\apreq_params.obj" \
	"$(INTDIR)\apreq_parsers.obj" \
        "$(INTDIR)\apreq_version.obj" \
        "$(INTDIR)\apreq_env.obj" \
        "$(INTDIR)\apreq_env_custom.obj" \
        "$(INTDIR)\apreq_env_cgi.obj" \
	"$(APR_LIB)" \
	"$(APU_LIB)"

"$(OUTDIR)\libapreq2.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(DEF_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libapreq2 - Win32 Debug"

ALL : "$(OUTDIR)\libapreq2.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /I"$(APACHE)\include" /I"$(APREQ_HOME)\src" /Fp"$(INTDIR)\libapreq2.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ  /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapreq2.bsc" 
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:yes /pdb:"$(OUTDIR)\libapreq2.pdb" /debug /machine:I386 /out:"$(OUTDIR)\libapreq2.dll" /implib:"$(OUTDIR)\libapreq2.lib" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\apreq.obj" \
	"$(INTDIR)\apreq_cookie.obj" \
	"$(INTDIR)\apreq_params.obj" \
	"$(INTDIR)\apreq_parsers.obj" \
        "$(INTDIR)\apreq_version.obj" \
        "$(INTDIR)\apreq_env.obj" \
        "$(INTDIR)\apreq_env_custom.obj" \
        "$(INTDIR)\apreq_env_cgi.obj" \
	"$(APR_LIB)" \
	"$(APU_LIB)"

"$(OUTDIR)\libapreq2.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(DEF_FLAGS) $(LINK32_OBJS)
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


!IF "$(CFG)" == "libapreq2 - Win32 Release" || "$(CFG)" == "libapreq2 - Win32 Debug"
SOURCE=$(APREQ_HOME)\src\apreq.c

"$(INTDIR)\apreq.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=$(APREQ_HOME)\src\apreq_cookie.c

"$(INTDIR)\apreq_cookie.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=$(APREQ_HOME)\src\apreq_params.c

"$(INTDIR)\apreq_params.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=$(APREQ_HOME)\src\apreq_parsers.c

"$(INTDIR)\apreq_parsers.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=$(APREQ_HOME)\src\apreq_version.c

"$(INTDIR)\apreq_version.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(APREQ_HOME)\src\apreq_env.c

"$(INTDIR)\apreq_env.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(APREQ_HOME)\src\apreq_env_custom.c

"$(INTDIR)\apreq_env_custom.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=$(APREQ_HOME)\src\apreq_env_cgi.c

"$(INTDIR)\apreq_env_cgi.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

!ENDIF 

