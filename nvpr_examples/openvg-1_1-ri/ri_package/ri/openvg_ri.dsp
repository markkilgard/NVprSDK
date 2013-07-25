# Microsoft Developer Studio Project File - Name="openvg_ri" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=openvg_ri - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "openvg_ri.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "openvg_ri.mak" CFG="openvg_ri - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "openvg_ri - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "openvg_ri - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "Perforce Project"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "openvg_ri - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../lib"
# PROP Intermediate_Dir "obj/release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENVG_RI_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W4 /GX /Zi /O2 /I "include/VG" /I "include" /I "include/EGL" /I "src" /I "../lib" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENVG_DLL_EXPORTS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /pdb:"bin/openvg_ri.pdb" /machine:I386 /out:"../bin/win32/libOpenVG.dll" /libpath:"../lib"
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "openvg_ri - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../lib"
# PROP Intermediate_Dir "obj/debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENVG_RI_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /GX /Zi /Od /I "include/VG" /I "include" /I "include/EGL" /I "src" /I "../lib" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENVG_DLL_EXPORTS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /incremental:no /pdb:"bin/openvg_ri-dbg.pdb" /debug /machine:I386 /out:"../bin/win32/libOpenVG-dbg.dll" /pdbtype:sept /libpath:"../lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "openvg_ri - Win32 Release"
# Name "openvg_ri - Win32 Debug"
# Begin Group "src"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\riApi.cpp
# End Source File
# Begin Source File

SOURCE=.\src\riArray.h
# End Source File
# Begin Source File

SOURCE=.\src\riContext.cpp
# End Source File
# Begin Source File

SOURCE=.\src\riContext.h
# End Source File
# Begin Source File

SOURCE=.\src\riDefs.h
# End Source File
# Begin Source File

SOURCE=.\src\win32\riEGLOS.cpp
# End Source File
# Begin Source File

SOURCE=.\src\riFont.cpp
# End Source File
# Begin Source File

SOURCE=.\src\riFont.h
# End Source File
# Begin Source File

SOURCE=.\src\riImage.cpp
# End Source File
# Begin Source File

SOURCE=.\src\riImage.h
# End Source File
# Begin Source File

SOURCE=.\src\riMath.cpp
# End Source File
# Begin Source File

SOURCE=.\src\riMath.h
# End Source File
# Begin Source File

SOURCE=.\src\riMiniEGL.cpp
# End Source File
# Begin Source File

SOURCE=.\src\riPath.cpp
# End Source File
# Begin Source File

SOURCE=.\src\riPath.h
# End Source File
# Begin Source File

SOURCE=.\src\riPixelPipe.cpp
# End Source File
# Begin Source File

SOURCE=.\src\riPixelPipe.h
# End Source File
# Begin Source File

SOURCE=.\src\riRasterizer.cpp
# End Source File
# Begin Source File

SOURCE=.\src\riRasterizer.h
# End Source File
# Begin Source File

SOURCE=.\src\riVGU.cpp
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\include\EGL\egl.h
# End Source File
# Begin Source File

SOURCE=.\include\vg\openvg.h
# End Source File
# Begin Source File

SOURCE=.\include\vg\vgext.h
# End Source File
# Begin Source File

SOURCE=.\include\vg\vgplatform.h
# End Source File
# Begin Source File

SOURCE=.\include\vg\vgu.h
# End Source File
# End Group
# End Target
# End Project
