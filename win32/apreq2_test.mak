# Microsoft Developer Studio Generated NMAKE File, Based on apreq2_test.dsp
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

!IF "$(CFG)" == ""
CFG=apreq2_test - Win32 Release
!MESSAGE No configuration specified. Defaulting to apreq2_test - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "apreq2_test - Win32 Release" && "$(CFG)" != "apreq2_test - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "apreq2_test.mak" CFG="apreq2_test - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "apreq2_test - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "apreq2_test - Win32 Debug" (based on "Win32 (x86) Static Library")
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
CPP_PROJ=/nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /I"$(APACHE)\include" /I"$(APREQ_HOME)\src" /Fp"$(INTDIR)\testall.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

CFG_HOME=$(APREQ_HOME)\win32
OUTDIR=$(CFG_HOME)\libs
INTDIR=$(CFG_HOME)\libs
TDIR=$(APREQ_HOME)\t
PROGRAMS="$(TDIR)\params.exe" "$(TDIR)\version.exe" \
  "$(TDIR)\parsers.exe" "$(TDIR)\cookie.exe"

!IF  "$(CFG)" == "apreq2_test - Win32 Release"

ALL : "$(OUTDIR)\apreq2_test.lib" $(PROGRAMS)

CLEAN :
	-@erase "$(INTDIR)\at.obj"
	-@erase "$(INTDIR)\test_env.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\apreq2_test.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /Fp"$(INTDIR)\apreq2_test.pch" /I"$(APACHE)\include" /I"$(APREQ_HOME)\src" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\apreq2_test.bsc"	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\apreq2_test.lib" 
LIB32_OBJS= \
	"$(INTDIR)\test_env.obj" \
	"$(INTDIR)\at.obj" \
	"$(APR_LIB)" \
	"$(APU_LIB)"

"$(OUTDIR)\apreq2_test.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib wsock32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes  /debug /machine:I386 /pdbtype:sept 

LINK32_OBJS= \
	"$(OUTDIR)\libapreq2.lib" \
	"$(OUTDIR)\apreq2_test.lib" \
	"$(APR_LIB)" \
	"$(APU_LIB)" \
	"$(APACHE)\lib\libhttpd.lib"

"$(TDIR)\cookie.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\cookie.obj"
    $(LINK32) /pdb:"$(TDIR)\cookie.pdb" /out:"$(TDIR)\cookie.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\cookie.obj"
<<

"$(TDIR)\params.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\params.obj"
    $(LINK32) /pdb:"$(TDIR)\params.pdb" /out:"$(TDIR)\params.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\params.obj"
<<

"$(TDIR)\parsers.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\parsers.obj"
    $(LINK32) /pdb:"$(TESTFILE)\parsers.pdb" /out:"$(TDIR)\parsers.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\parsers.obj"
<<

"$(TDIR)\version.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\version.obj"
    $(LINK32) /pdb:"$(TDIR)\version.pdb" /out:"$(TDIR)\version.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\version.obj"
<<

!ELSEIF  "$(CFG)" == "apreq2_test - Win32 Debug"

ALL : "$(OUTDIR)\apreq2_test.lib" $(PROGRAMS)

CLEAN :
	-@erase "$(INTDIR)\at.obj"
	-@erase "$(INTDIR)\test_env.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\apreq2_test.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /Fp"$(INTDIR)\apreq2_test.pch" /YX /I"$(APACHE)\include" /I"$(APREQ_HOME)\src" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ  /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\apreq2_test.bsc" 
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\apreq2_test.lib" 
LIB32_OBJS= \
	"$(INTDIR)\test_env.obj" \
	"$(INTDIR)\at.obj" \
	"$(APR_LIB)" \
	"$(APU_LIB)"

"$(OUTDIR)\apreq2_test.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

"$(TDIR)\cookie.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\cookie.obj"
    $(LINK32) /pdb:"$(TDIR)\cookie.pdb" /out:"$(TDIR)\cookie.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\cookie.obj"
<<

"$(TDIR)\params.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\params.obj"
    $(LINK32) /pdb:"$(TDIR)\params.pdb" /out:"$(TDIR)\params.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\params.obj"
<<

"$(TDIR)\parser.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\parser.obj"
    $(LINK32) /pdb:"$(TESTFILE)\parser.pdb" /out:"$(TDIR)\parser.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\parser.obj"
<<

"$(TDIR)\version.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS) "$(OUTDIR)\version.obj"
    $(LINK32) /pdb:"$(TDIR)\version.pdb" /out:"$(TDIR)\version.exe" @<<
  $(LINK32_FLAGS) $(LINK32_OBJS) "$(OUTDIR)\version.obj"
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


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("apreq2_test.dep")
!INCLUDE "apreq2_test.dep"
!ELSE 
!MESSAGE Warning: cannot find "apreq2_test.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "apreq2_test - Win32 Release" || "$(CFG)" == "apreq2_test - Win32 Debug"
SOURCE=t\at.c

"$(INTDIR)\at.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=t\test_env.c

"$(INTDIR)\test_env.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=t\cookie.c

"$(OUTDIR)\cookie.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=t\params.c

"$(OUTDIR)\params.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=t\parsers.c

"$(OUTDIR)\parsers.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

SOURCE=t\version.c

"$(OUTDIR)\version.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 

