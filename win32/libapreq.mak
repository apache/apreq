# Microsoft Developer Studio Generated NMAKE File, Based on libapreq.dsp

!IF "$(APACHE)" == ""
!MESSAGE No Apache directory was specified.
!MESSAGE This makefile is not to be run directly.
!MESSAGE Please run Configure.bat, and then $(MAKE) on Makefile.
!ERROR
!ENDIF

!IF "$(CFG)" == ""
CFG=libapreq - Win32 Release
!MESSAGE No configuration specified. Defaulting to libapreq - Win32 Release.
!ENDIF

!IF "$(CFG)" != "libapreq - Win32 Release" && "$(CFG)" != "libapreq - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libapreq.mak" CFG="libapreq - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libapreq - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libapreq - Win32 Debug" (based on "Win32 (x86) Static Library")
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

OUTDIR=.\libs
INTDIR=.\libs
# Begin Custom Macros
OutDir=.\libs
# End Custom Macros


!IF  "$(CFG)" == "libapreq - Win32 Release"

ALL : "$(OUTDIR)\libapreq.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /I"$(APACHE)\include" /Fp"$(INTDIR)\libapreq.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapreq.bsc" 
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /nodefaultlib /out:"$(OUTDIR)\libapreq.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apreq.obj" \
	"$(INTDIR)\apreq_cookie.obj" \
	"$(INTDIR)\apreq_params.obj" \
	"$(INTDIR)\apreq_parsers.obj" \
	"$(INTDIR)\apreq_tables.obj"

"$(OUTDIR)\libapreq.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libapreq - Win32 Debug"

ALL : "$(OUTDIR)\libapreq.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /I"$(APACHE)\include" /Fp"$(INTDIR)\libapreq.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libapreq.bsc" 
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libapreq.lib" 
LIB32_OBJS= \
	"$(INTDIR)\apreq.obj" \
	"$(INTDIR)\apreq_cookie.obj" \
	"$(INTDIR)\apreq_params.obj" \
	"$(INTDIR)\apreq_parsers.obj" \
	"$(INTDIR)\apreq_tables.obj"

"$(OUTDIR)\libapreq.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
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


!IF "$(CFG)" == "libapreq - Win32 Release" || "$(CFG)" == "libapreq - Win32 Debug"
SOURCE=..\src\apreq.c

"$(INTDIR)\apreq.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\apreq_cookie.c

"$(INTDIR)\apreq_cookie.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\apreq_params.c

"$(INTDIR)\apreq_params.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\apreq_parsers.c

"$(INTDIR)\apreq_parsers.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\src\apreq_tables.c

"$(INTDIR)\apreq_tables.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

