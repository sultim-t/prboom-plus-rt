# Microsoft Developer Studio Project File - Name="viddsys" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=viddsys - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "viddsys.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "viddsys.mak" CFG="viddsys - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "viddsys - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "viddsys - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "viddsys - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "viddsys___Win32_Release"
# PROP BASE Intermediate_Dir "viddsys___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "viddsys___Win32_Release"
# PROP Intermediate_Dir "viddsys___Win32_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VIDDSYS_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VIDDSYS_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /dll /machine:I386 /out:"Release/viddsys.dll"

!ELSEIF  "$(CFG)" == "viddsys - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "viddsys___Win32_Debug"
# PROP BASE Intermediate_Dir "viddsys___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "viddsys___Win32_Debug"
# PROP Intermediate_Dir "viddsys___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VIDDSYS_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "VIDDSYS_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /dll /debug /machine:I386 /out:"Debug/viddsys.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "viddsys - Win32 Release"
# Name "viddsys - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\ViddSys\zlib\adler32.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\bzlib\blocksort.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\bzlib\bzlib.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\bzlib\compress.c

!IF  "$(CFG)" == "viddsys - Win32 Release"

# PROP Intermediate_Dir "viddsys___Win32_Release\zlib"

!ELSEIF  "$(CFG)" == "viddsys - Win32 Debug"

# PROP Intermediate_Dir "viddsys___Win32_Debug\zlib"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\compress.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\bzlib\crctable.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\bzlib\decompress.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\deflate.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\XML\Elements.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\XML\ErrorMessages.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\bzlib\huffman.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\infblock.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\infcodes.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\inffast.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\inflate.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\infutil.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\bzlib\randtable.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\XML\Reader.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\trees.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\ViddSys\XML\Utils.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddBasic.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddController.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddElementTrack.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddEventTrack.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddFile.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddPlayer.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddRecorder.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddSys.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\XML\Writer.cpp
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\zutil.c
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\ViddSys\XML\AdvXMLParser.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\XML\AdvXMLParserConfig.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\XML\AdvXMLParserDefs.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\XML\AdvXMLParserPrimitiveTypes.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\XML\AdvXMLParserUtils.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\AnimTrack.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\AssocVector.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\bzlib\bzlib.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\bzlib\bzlib_private.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\deflate.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\infblock.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\infcodes.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\inffast.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\infutil.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\trees.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddBasic.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddController.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddElementTrack.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddEventTrack.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddFile.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddPlayer.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddRecorder.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddSys.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\ViddTypedPropertyTracks.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\zconf.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\zlib.h
# End Source File
# Begin Source File

SOURCE=..\ViddSys\zlib\zutil.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
