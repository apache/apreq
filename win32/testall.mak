# Microsoft Developer Studio Generated NMAKE File, Based on testall.dsp

!IF "$(APACHE)" == ""
!MESSAGE No Apache directory was specified.
!MESSAGE This makefile is not to be run directly.
!MESSAGE Please run Configure.bat, and then $(MAKE) on Makefile.
!ERROR
!ENDIF

!IF "$(CFG)" == ""
CFG=testall - Win32 Release
!MESSAGE No configuration specified. Defaulting to testall - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "testall - Win32 Release" && "$(CFG)" != "testall - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "testall.mak" CFG="testall - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "testall - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "testall - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "testall - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\testall.exe"


CLEAN :
	-@erase "$(INTDIR)\cookie.obj"
	-@erase "$(INTDIR)\CuTest.obj"
	-@erase "$(INTDIR)\env.obj"
	-@erase "$(INTDIR)\params.obj"
	-@erase "$(INTDIR)\parsers.obj"
	-@erase "$(INTDIR)\performance.obj"
	-@erase "$(INTDIR)\tables.obj"
	-@erase "$(INTDIR)\testall.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\testall.exe"
        -@erase "$(OUTDIR)\testall.pch"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /I"$(APACHE)\include" /I"..\src" /Fp"$(INTDIR)\testall.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\testall.bsc" 
LINK32=link.exe
LINK32_FLAGS=kernel32.lib wsock32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\testall.pdb" /machine:I386 /out:"$(OUTDIR)\testall.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cookie.obj" \
	"$(INTDIR)\CuTest.obj" \
	"$(INTDIR)\env.obj" \
	"$(INTDIR)\params.obj" \
	"$(INTDIR)\tables.obj" \
	"$(INTDIR)\testall.obj" \
	"$(OUTDIR)\libapreq.lib" \
	"$(APACHE)\lib\libapr.lib" \
	"$(APACHE)\lib\libaprutil.lib" \
	"$(APACHE)\lib\libhttpd.lib" \
	"$(INTDIR)\parsers.obj" \
	"$(INTDIR)\performance.obj"

"$(OUTDIR)\testall.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "testall - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\testall.exe"


CLEAN :
	-@erase "$(INTDIR)\cookie.obj"
	-@erase "$(INTDIR)\CuTest.obj"
	-@erase "$(INTDIR)\env.obj"
	-@erase "$(INTDIR)\params.obj"
	-@erase "$(INTDIR)\parsers.obj"
	-@erase "$(INTDIR)\performance.obj"
	-@erase "$(INTDIR)\tables.obj"
	-@erase "$(INTDIR)\testall.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\testall.exe"
	-@erase "$(OUTDIR)\testall.ilk"
	-@erase "$(OUTDIR)\testall.pdb"
        -@erase "$(OUTDIR)\testall.pch"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /I"$(APACHE)\include" /I"..\src" /Fp"$(INTDIR)\testall.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\testall.bsc" 
LINK32=link.exe
LINK32_FLAGS=kernel32.lib wsock32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\testall.pdb" /debug /machine:I386 /out:"$(OUTDIR)\testall.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\cookie.obj" \
	"$(INTDIR)\CuTest.obj" \
	"$(INTDIR)\env.obj" \
	"$(INTDIR)\params.obj" \
	"$(INTDIR)\tables.obj" \
	"$(INTDIR)\testall.obj" \
	"$(OUTDIR)\libapreq.lib" \
	"$(APACHE)\lib\libapr.lib" \
	"$(APACHE)\lib\libaprutil.lib" \
	"$(APACHE)\lib\libhttpd.lib" \
	"$(INTDIR)\parsers.obj" \
	"$(INTDIR)\performance.obj"

"$(OUTDIR)\testall.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(CFG)" == "testall - Win32 Release" || "$(CFG)" == "testall - Win32 Debug"
SOURCE=..\t\cookie.c

"$(INTDIR)\cookie.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\t\CuTest.c

"$(INTDIR)\CuTest.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\t\env.c

"$(INTDIR)\env.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\t\params.c

"$(INTDIR)\params.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\t\parsers.c

"$(INTDIR)\parsers.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\t\performance.c

"$(INTDIR)\performance.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\t\tables.c

"$(INTDIR)\tables.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=..\t\testall.c

"$(INTDIR)\testall.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)



!ENDIF 
