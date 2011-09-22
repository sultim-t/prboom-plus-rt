# Microsoft Developer Studio Project File - Name="Doom" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Doom - Win32 Debug OpenGL NOASM
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Doom.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Doom.mak" CFG="Doom - Win32 Debug OpenGL NOASM"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Doom - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Doom - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Doom - Win32 Release OpenGL" (based on "Win32 (x86) Application")
!MESSAGE "Doom - Win32 Debug OpenGL" (based on "Win32 (x86) Application")
!MESSAGE "Doom - Win32 Release NOASM" (based on "Win32 (x86) Application")
!MESSAGE "Doom - Win32 Release OpenGL NOASM" (based on "Win32 (x86) Application")
!MESSAGE "Doom - Win32 Debug NOASM" (based on "Win32 (x86) Application")
!MESSAGE "Doom - Win32 Debug OpenGL NOASM" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /WX /GX /O2 /I "../VisualC6" /I "../src" /I "../src/PCRELIB" /I "../src/MUSIC/include/portmidi" /I "../src/MUSIC/include/dumb" /I "../src/MUSIC/include/libmad" /I "../src/MUSIC/include/fluidsynth" /I "../src/MUSIC/include/vorbis" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "I386_ASM" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x409 /i "./../VisualC6" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 user32.lib gdi32.lib /nologo /subsystem:windows /map /machine:I386 /out:"Release/prboom-plus.exe" /libpath:"../src/PCRELIB/VisualC6/lib" /libpath:"../src/MUSIC/lib"
# SUBTRACT LINK32 /profile
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=del Release\version.obj
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /WX /Gm /GX /ZI /Od /I "../VisualC6" /I "../src" /I "../src/PCRELIB" /I "../src/MUSIC/include/portmidi" /I "../src/MUSIC/include/dumb" /I "../src/MUSIC/include/libmad" /I "../src/MUSIC/include/fluidsynth" /I "../src/MUSIC/include/vorbis" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "INSTRUMENTED" /D "RANGECHECK" /D "SIMPLECHECKS" /D "TIMEDIAG" /D "HEAPDUMP" /D "HAVE_CONFIG_H" /D "I386_ASM" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x409 /i "./../VisualC6" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"Debug/prboom-plus.exe" /pdbtype:sept /libpath:"../src/PCRELIB/VisualC6/lib" /libpath:"../src/MUSIC/lib"
# SUBTRACT LINK32 /profile

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseGL"
# PROP BASE Intermediate_Dir "ReleaseGL"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseGL"
# PROP Intermediate_Dir "ReleaseGL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "HIGHRES" /YX /FD /c
# ADD CPP /nologo /MT /W3 /WX /GX /O2 /I "../VisualC6" /I "../src" /I "../src/PCRELIB" /I "../src/MUSIC/include/portmidi" /I "../src/MUSIC/include/dumb" /I "../src/MUSIC/include/libmad" /I "../src/MUSIC/include/fluidsynth" /I "../src/MUSIC/include/vorbis" /D "NDEBUG" /D "GL_DOOM" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "I386_ASM" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x409 /i "./../VisualC6" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib sdl.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 opengl32.lib glu32.lib user32.lib gdi32.lib /nologo /subsystem:windows /profile /map /machine:I386 /out:"ReleaseGL/glboom-plus.exe" /libpath:"../src/PCRELIB/VisualC6/lib" /libpath:"../src/MUSIC/lib"
# SUBTRACT LINK32 /debug /nodefaultlib
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=del ReleaseGL\version.obj
# End Special Build Tool

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "DebugGL"
# PROP BASE Intermediate_Dir "DebugGL"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugGL"
# PROP Intermediate_Dir "DebugGL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "HIGHRES" /D "INSTRUMENTED" /D "RANGECHECK" /D "SIMPLECHECKS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /WX /Gm /GX /ZI /Od /I "../VisualC6" /I "../src" /I "../src/PCRELIB" /I "../src/MUSIC/include/portmidi" /I "../src/MUSIC/include/dumb" /I "../src/MUSIC/include/libmad" /I "../src/MUSIC/include/fluidsynth" /I "../src/MUSIC/include/vorbis" /D "GL_DOOM" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "INSTRUMENTED" /D "RANGECHECK" /D "SIMPLECHECKS" /D "TIMEDIAG" /D "HEAPDUMP" /D "HAVE_CONFIG_H" /D "I386_ASM" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x409 /i "./../VisualC6" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib sdl.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 opengl32.lib glu32.lib user32.lib gdi32.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"DebugGL/glboom-plus.exe" /libpath:"../src/PCRELIB/VisualC6/lib" /libpath:"../src/MUSIC/lib"
# SUBTRACT LINK32 /profile /nodefaultlib

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseNOASM"
# PROP BASE Intermediate_Dir "ReleaseNOASM"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseNOASM"
# PROP Intermediate_Dir "ReleaseNOASM"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "../VisualC6" /I "../src" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD CPP /nologo /MT /W3 /WX /GX /O2 /I "../VisualC6" /I "../src" /I "../src/PCRELIB" /I "../src/MUSIC/include/portmidi" /I "../src/MUSIC/include/dumb" /I "../src/MUSIC/include/libmad" /I "../src/MUSIC/include/fluidsynth" /I "../src/MUSIC/include/vorbis" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x409 /i "./../VisualC6" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib sdl.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 user32.lib gdi32.lib /nologo /subsystem:windows /map /machine:I386 /out:"ReleaseNOASM/prboom-plus.exe" /libpath:"../src/PCRELIB/VisualC6/lib" /libpath:"../src/MUSIC/lib"
# SUBTRACT LINK32 /profile

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "ReleaseNOASMGL"
# PROP BASE Intermediate_Dir "ReleaseNOASMGL"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "ReleaseNOASMGL"
# PROP Intermediate_Dir "ReleaseNOASMGL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "../VisualC6" /I "../src" /D "NDEBUG" /D "GL_DOOM" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD CPP /nologo /MT /W3 /WX /GX /O2 /I "../VisualC6" /I "../src" /I "../src/PCRELIB" /I "../src/MUSIC/include/portmidi" /I "../src/MUSIC/include/dumb" /I "../src/MUSIC/include/libmad" /I "../src/MUSIC/include/fluidsynth" /I "../src/MUSIC/include/vorbis" /D "NDEBUG" /D "GL_DOOM" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x409 /i "./../VisualC6" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib sdl.lib opengl32.lib glu32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /profile /debug
# ADD LINK32 opengl32.lib glu32.lib user32.lib gdi32.lib /nologo /subsystem:windows /map /machine:I386 /out:"ReleaseNOASMGL/glboom-plus.exe" /libpath:"../src/PCRELIB/VisualC6/lib" /libpath:"../src/MUSIC/lib"
# SUBTRACT LINK32 /profile /debug

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Doom___Win32_Debug_NOASM"
# PROP BASE Intermediate_Dir "Doom___Win32_Debug_NOASM"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugNOASM"
# PROP Intermediate_Dir "DebugNOASM"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "../VisualC6" /I "../src" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "INSTRUMENTED" /D "RANGECHECK" /D "SIMPLECHECKS" /D "TIMEDIAG" /D "HEAPDUMP" /D "HAVE_CONFIG_H" /D "I386_ASM" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /WX /Gm /GX /ZI /Od /I "../VisualC6" /I "../src" /I "../src/PCRELIB" /I "../src/MUSIC/include/portmidi" /I "../src/MUSIC/include/dumb" /I "../src/MUSIC/include/libmad" /I "../src/MUSIC/include/fluidsynth" /I "../src/MUSIC/include/vorbis" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "INSTRUMENTED" /D "RANGECHECK" /D "SIMPLECHECKS" /D "TIMEDIAG" /D "HEAPDUMP" /D "HAVE_CONFIG_H" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x409 /i "./../VisualC6" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 user32.lib gdi32.lib sdl.lib sdl_mixer.lib sdl_net.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/prboom.exe" /pdbtype:sept
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 user32.lib gdi32.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"DebugNOASM/prboom-plus.exe" /pdbtype:sept /libpath:"../src/PCRELIB/VisualC6/lib" /libpath:"../src/MUSIC/lib"
# SUBTRACT LINK32 /profile

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Doom___Win32_Debug_OpenGL_NOASM"
# PROP BASE Intermediate_Dir "Doom___Win32_Debug_OpenGL_NOASM"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "DebugNOASMGL"
# PROP Intermediate_Dir "DebugNOASMGL"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "../VisualC6" /I "../src" /D "GL_DOOM" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "INSTRUMENTED" /D "RANGECHECK" /D "SIMPLECHECKS" /D "TIMEDIAG" /D "HEAPDUMP" /D "HAVE_CONFIG_H" /D "I386_ASM" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /WX /Gm /GX /ZI /Od /I "../VisualC6" /I "../src" /I "../src/PCRELIB" /I "../src/MUSIC/include/portmidi" /I "../src/MUSIC/include/dumb" /I "../src/MUSIC/include/libmad" /I "../src/MUSIC/include/fluidsynth" /I "../src/MUSIC/include/vorbis" /D "GL_DOOM" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "INSTRUMENTED" /D "RANGECHECK" /D "SIMPLECHECKS" /D "TIMEDIAG" /D "HEAPDUMP" /D "HAVE_CONFIG_H" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x409 /i "./../VisualC6" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 opengl32.lib glu32.lib user32.lib gdi32.lib sdl.lib sdl_mixer.lib sdl_net.lib /nologo /subsystem:windows /debug /machine:I386 /out:"DebugGL/glboom.exe"
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 opengl32.lib glu32.lib user32.lib gdi32.lib /nologo /subsystem:windows /map /debug /machine:I386 /out:"DebugNOASMGL/glboom-plus.exe" /libpath:"../src/PCRELIB/VisualC6/lib" /libpath:"../src/MUSIC/lib"
# SUBTRACT LINK32 /profile

!ENDIF 

# Begin Target

# Name "Doom - Win32 Release"
# Name "Doom - Win32 Debug"
# Name "Doom - Win32 Release OpenGL"
# Name "Doom - Win32 Debug OpenGL"
# Name "Doom - Win32 Release NOASM"
# Name "Doom - Win32 Release OpenGL NOASM"
# Name "Doom - Win32 Debug NOASM"
# Name "Doom - Win32 Debug OpenGL NOASM"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\am_map.c
# End Source File
# Begin Source File

SOURCE=..\src\am_map.h
# End Source File
# Begin Source File

SOURCE=config.h
# End Source File
# Begin Source File

SOURCE=..\src\d_client.c
# End Source File
# Begin Source File

SOURCE=..\src\d_deh.c
# End Source File
# Begin Source File

SOURCE=..\src\d_deh.h
# End Source File
# Begin Source File

SOURCE=..\src\d_englsh.h
# End Source File
# Begin Source File

SOURCE=..\src\d_event.h
# End Source File
# Begin Source File

SOURCE=..\src\d_items.c
# End Source File
# Begin Source File

SOURCE=..\src\d_items.h
# End Source File
# Begin Source File

SOURCE=..\src\d_main.c
# End Source File
# Begin Source File

SOURCE=..\src\d_main.h
# End Source File
# Begin Source File

SOURCE=..\src\d_net.h
# End Source File
# Begin Source File

SOURCE=..\src\d_player.h
# End Source File
# Begin Source File

SOURCE=..\src\d_think.h
# End Source File
# Begin Source File

SOURCE=..\src\d_ticcmd.h
# End Source File
# Begin Source File

SOURCE=..\src\doomdata.h
# End Source File
# Begin Source File

SOURCE=..\src\doomdef.c
# End Source File
# Begin Source File

SOURCE=..\src\doomdef.h
# End Source File
# Begin Source File

SOURCE=..\src\doomstat.c
# End Source File
# Begin Source File

SOURCE=..\src\doomstat.h
# End Source File
# Begin Source File

SOURCE=..\src\doomtype.h
# End Source File
# Begin Source File

SOURCE=..\src\drawasm.h
# End Source File
# Begin Source File

SOURCE=..\src\dstrings.c
# End Source File
# Begin Source File

SOURCE=..\src\dstrings.h
# End Source File
# Begin Source File

SOURCE=..\src\e6y.c
# End Source File
# Begin Source File

SOURCE=..\src\e6y.h
# End Source File
# Begin Source File

SOURCE=..\src\e6y_launcher.c
# End Source File
# Begin Source File

SOURCE=..\src\e6y_launcher.h
# End Source File
# Begin Source File

SOURCE=..\src\f_finale.c
# End Source File
# Begin Source File

SOURCE=..\src\f_finale.h
# End Source File
# Begin Source File

SOURCE=..\src\f_wipe.c

!IF  "$(CFG)" == "Doom - Win32 Release"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\f_wipe.h

!IF  "$(CFG)" == "Doom - Win32 Release"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\g_game.c
# End Source File
# Begin Source File

SOURCE=..\src\g_game.h
# End Source File
# Begin Source File

SOURCE=..\src\g_overflow.c
# End Source File
# Begin Source File

SOURCE=..\src\g_overflow.h
# End Source File
# Begin Source File

SOURCE=..\src\hu_lib.c
# End Source File
# Begin Source File

SOURCE=..\src\hu_lib.h
# End Source File
# Begin Source File

SOURCE=..\src\hu_stuff.c
# End Source File
# Begin Source File

SOURCE=..\src\hu_stuff.h
# End Source File
# Begin Source File

SOURCE=..\src\hu_tracers.c
# End Source File
# Begin Source File

SOURCE=..\src\hu_tracers.h
# End Source File
# Begin Source File

SOURCE=..\src\i_capture.c
# End Source File
# Begin Source File

SOURCE=..\src\i_capture.h
# End Source File
# Begin Source File

SOURCE=..\src\SDL\i_joy.c
# End Source File
# Begin Source File

SOURCE=..\src\i_joy.h
# End Source File
# Begin Source File

SOURCE=..\src\SDL\i_main.c
# End Source File
# Begin Source File

SOURCE=..\src\i_main.h
# End Source File
# Begin Source File

SOURCE=..\src\SDL\i_network.c
# End Source File
# Begin Source File

SOURCE=..\src\i_network.h
# End Source File
# Begin Source File

SOURCE=..\src\i_pcsound.c
# End Source File
# Begin Source File

SOURCE=..\src\i_pcsound.h
# End Source File
# Begin Source File

SOURCE=..\src\i_simd.c
# End Source File
# Begin Source File

SOURCE=..\src\i_simd.h
# End Source File
# Begin Source File

SOURCE=..\src\i_smp.c
# End Source File
# Begin Source File

SOURCE=..\src\i_smp.h
# End Source File
# Begin Source File

SOURCE=..\src\SDL\i_sound.c
# End Source File
# Begin Source File

SOURCE=..\src\i_sound.h
# End Source File
# Begin Source File

SOURCE=..\src\SDL\i_sshot.c
# End Source File
# Begin Source File

SOURCE=..\src\SDL\i_system.c
# End Source File
# Begin Source File

SOURCE=..\src\i_system.h
# End Source File
# Begin Source File

SOURCE=..\src\SDL\i_video.c
# End Source File
# Begin Source File

SOURCE=..\src\i_video.h
# End Source File
# Begin Source File

SOURCE=..\src\icon.c
# End Source File
# Begin Source File

SOURCE=..\src\info.c
# End Source File
# Begin Source File

SOURCE=..\src\info.h
# End Source File
# Begin Source File

SOURCE=..\src\lprintf.c
# End Source File
# Begin Source File

SOURCE=..\src\lprintf.h
# End Source File
# Begin Source File

SOURCE=..\src\m_argv.c
# End Source File
# Begin Source File

SOURCE=..\src\m_argv.h
# End Source File
# Begin Source File

SOURCE=..\src\m_bbox.c
# End Source File
# Begin Source File

SOURCE=..\src\m_bbox.h
# End Source File
# Begin Source File

SOURCE=..\src\m_cheat.c
# End Source File
# Begin Source File

SOURCE=..\src\m_cheat.h
# End Source File
# Begin Source File

SOURCE=..\src\m_fixed.h
# End Source File
# Begin Source File

SOURCE=..\src\m_menu.c
# End Source File
# Begin Source File

SOURCE=..\src\m_menu.h
# End Source File
# Begin Source File

SOURCE=..\src\m_misc.c
# End Source File
# Begin Source File

SOURCE=..\src\m_misc.h
# End Source File
# Begin Source File

SOURCE=..\src\m_random.c
# End Source File
# Begin Source File

SOURCE=..\src\m_random.h
# End Source File
# Begin Source File

SOURCE=..\src\m_swap.h
# End Source File
# Begin Source File

SOURCE=..\src\md5.c
# End Source File
# Begin Source File

SOURCE=..\src\md5.h
# End Source File
# Begin Source File

SOURCE=..\src\memio.c
# End Source File
# Begin Source File

SOURCE=..\src\memio.h
# End Source File
# Begin Source File

SOURCE=..\src\mus2mid.c
# End Source File
# Begin Source File

SOURCE=..\src\mus2mid.h
# End Source File
# Begin Source File

SOURCE=..\src\p_ceilng.c
# End Source File
# Begin Source File

SOURCE=..\src\p_checksum.c
# End Source File
# Begin Source File

SOURCE=..\src\p_checksum.h
# End Source File
# Begin Source File

SOURCE=..\src\p_doors.c
# End Source File
# Begin Source File

SOURCE=..\src\p_enemy.c
# End Source File
# Begin Source File

SOURCE=..\src\p_enemy.h
# End Source File
# Begin Source File

SOURCE=..\src\p_floor.c
# End Source File
# Begin Source File

SOURCE=..\src\p_genlin.c
# End Source File
# Begin Source File

SOURCE=..\src\p_inter.c
# End Source File
# Begin Source File

SOURCE=..\src\p_inter.h
# End Source File
# Begin Source File

SOURCE=..\src\p_lights.c
# End Source File
# Begin Source File

SOURCE=..\src\p_map.c
# End Source File
# Begin Source File

SOURCE=..\src\p_map.h
# End Source File
# Begin Source File

SOURCE=..\src\p_maputl.c
# End Source File
# Begin Source File

SOURCE=..\src\p_maputl.h
# End Source File
# Begin Source File

SOURCE=..\src\p_mobj.c
# End Source File
# Begin Source File

SOURCE=..\src\p_mobj.h
# End Source File
# Begin Source File

SOURCE=..\src\p_plats.c
# End Source File
# Begin Source File

SOURCE=..\src\p_pspr.c
# End Source File
# Begin Source File

SOURCE=..\src\p_pspr.h
# End Source File
# Begin Source File

SOURCE=..\src\p_saveg.c
# End Source File
# Begin Source File

SOURCE=..\src\p_saveg.h
# End Source File
# Begin Source File

SOURCE=..\src\p_setup.c
# End Source File
# Begin Source File

SOURCE=..\src\p_setup.h
# End Source File
# Begin Source File

SOURCE=..\src\p_sight.c
# End Source File
# Begin Source File

SOURCE=..\src\p_spec.c
# End Source File
# Begin Source File

SOURCE=..\src\p_spec.h
# End Source File
# Begin Source File

SOURCE=..\src\p_switch.c
# End Source File
# Begin Source File

SOURCE=..\src\p_telept.c
# End Source File
# Begin Source File

SOURCE=..\src\p_tick.c
# End Source File
# Begin Source File

SOURCE=..\src\p_tick.h
# End Source File
# Begin Source File

SOURCE=..\src\p_user.c
# End Source File
# Begin Source File

SOURCE=..\src\p_user.h
# End Source File
# Begin Source File

SOURCE=..\src\protocol.h
# End Source File
# Begin Source File

SOURCE=..\src\qsort.h
# End Source File
# Begin Source File

SOURCE=..\src\r_bsp.c
# End Source File
# Begin Source File

SOURCE=..\src\r_bsp.h
# End Source File
# Begin Source File

SOURCE=..\src\r_data.c
# End Source File
# Begin Source File

SOURCE=..\src\r_data.h
# End Source File
# Begin Source File

SOURCE=..\src\r_defs.h
# End Source File
# Begin Source File

SOURCE=..\src\r_demo.c
# End Source File
# Begin Source File

SOURCE=..\src\r_demo.h
# End Source File
# Begin Source File

SOURCE=..\src\r_draw.c
# End Source File
# Begin Source File

SOURCE=..\src\r_draw.h
# End Source File
# Begin Source File

SOURCE=..\src\r_filter.c
# End Source File
# Begin Source File

SOURCE=..\src\r_filter.h
# End Source File
# Begin Source File

SOURCE=..\src\r_fps.c
# End Source File
# Begin Source File

SOURCE=..\src\r_fps.h
# End Source File
# Begin Source File

SOURCE=..\src\r_main.c
# End Source File
# Begin Source File

SOURCE=..\src\r_main.h
# End Source File
# Begin Source File

SOURCE=..\src\r_patch.c
# End Source File
# Begin Source File

SOURCE=..\src\r_patch.h
# End Source File
# Begin Source File

SOURCE=..\src\r_plane.c
# End Source File
# Begin Source File

SOURCE=..\src\r_plane.h
# End Source File
# Begin Source File

SOURCE=..\src\r_screenmultiply.c
# End Source File
# Begin Source File

SOURCE=..\src\r_screenmultiply.h
# End Source File
# Begin Source File

SOURCE=..\src\r_segs.c
# End Source File
# Begin Source File

SOURCE=..\src\r_segs.h
# End Source File
# Begin Source File

SOURCE=..\src\r_sky.c
# End Source File
# Begin Source File

SOURCE=..\src\r_sky.h
# End Source File
# Begin Source File

SOURCE=..\src\r_state.h
# End Source File
# Begin Source File

SOURCE=..\src\r_things.c
# End Source File
# Begin Source File

SOURCE=..\src\r_things.h
# End Source File
# Begin Source File

SOURCE=..\src\s_advsound.c
# End Source File
# Begin Source File

SOURCE=..\src\s_advsound.h
# End Source File
# Begin Source File

SOURCE=..\src\s_sound.c
# End Source File
# Begin Source File

SOURCE=..\src\s_sound.h
# End Source File
# Begin Source File

SOURCE=..\src\sc_man.c
# End Source File
# Begin Source File

SOURCE=..\src\sc_man.h
# End Source File
# Begin Source File

SOURCE=..\src\sounds.c
# End Source File
# Begin Source File

SOURCE=..\src\sounds.h
# End Source File
# Begin Source File

SOURCE=..\src\st_lib.c
# End Source File
# Begin Source File

SOURCE=..\src\st_lib.h
# End Source File
# Begin Source File

SOURCE=..\src\st_stuff.c
# End Source File
# Begin Source File

SOURCE=..\src\st_stuff.h
# End Source File
# Begin Source File

SOURCE=..\src\tables.c
# End Source File
# Begin Source File

SOURCE=..\src\tables.h
# End Source File
# Begin Source File

SOURCE=..\src\v_video.c
# End Source File
# Begin Source File

SOURCE=..\src\v_video.h
# End Source File
# Begin Source File

SOURCE=..\src\version.c
# End Source File
# Begin Source File

SOURCE=..\src\version.h
# End Source File
# Begin Source File

SOURCE=..\src\w_mmap.c
# End Source File
# Begin Source File

SOURCE=..\src\w_wad.c
# End Source File
# Begin Source File

SOURCE=..\src\w_wad.h
# End Source File
# Begin Source File

SOURCE=..\src\wi_stuff.c
# End Source File
# Begin Source File

SOURCE=..\src\wi_stuff.h
# End Source File
# Begin Source File

SOURCE=..\src\z_bmalloc.c
# End Source File
# Begin Source File

SOURCE=..\src\z_bmalloc.h
# End Source File
# Begin Source File

SOURCE=..\src\z_zone.c
# End Source File
# Begin Source File

SOURCE=..\src\z_zone.h
# End Source File
# End Group
# Begin Group "OpenGL Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\gl_clipper.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_detail.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_drawinfo.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_fbo.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_gamma.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_hires.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_hqresize.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_intern.h

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_light.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_main.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_missingtexture.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_opengl.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_opengl.h

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_preprocess.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_shadow.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_sky.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_soft2gl.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_struct.h

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_texture.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_vertex.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\gl_wipe.c

!IF  "$(CFG)" == "Doom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

!ENDIF 

# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Icons\barrel.ico
# End Source File
# Begin Source File

SOURCE=..\Icons\fouch.ico
# End Source File
# Begin Source File

SOURCE=..\Icons\god.ico
# End Source File
# Begin Source File

SOURCE=..\Icons\heada1.ico
# End Source File
# Begin Source File

SOURCE=..\Icons\icons.rc

!IF  "$(CFG)" == "Doom - Win32 Release"

# ADD BASE RSC /l 0x419 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT BASE RSC /i "./../VisualC6"
# ADD RSC /l 0x409 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT RSC /i "./../VisualC6"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# ADD BASE RSC /l 0x419 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT BASE RSC /i "./../VisualC6"
# ADD RSC /l 0x409 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT RSC /i "./../VisualC6"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

# ADD BASE RSC /l 0x419 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT BASE RSC /i "./../VisualC6"
# ADD RSC /l 0x419 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT RSC /i "./../VisualC6"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

# ADD BASE RSC /l 0x419 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT BASE RSC /i "./../VisualC6"
# ADD RSC /l 0x409 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT RSC /i "./../VisualC6"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# ADD BASE RSC /l 0x419 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT BASE RSC /i "./../VisualC6"
# ADD RSC /l 0x409 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT RSC /i "./../VisualC6"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

# ADD BASE RSC /l 0x419 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT BASE RSC /i "./../VisualC6"
# ADD RSC /l 0x409 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT RSC /i "./../VisualC6"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug NOASM"

# ADD BASE RSC /l 0x419 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT BASE RSC /i "./../VisualC6"
# ADD RSC /l 0x409 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT RSC /i "./../VisualC6"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL NOASM"

# ADD BASE RSC /l 0x419 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT BASE RSC /i "./../VisualC6"
# ADD RSC /l 0x409 /i "\andre\prg\doom\prboom-plus\branches\prboom-plus-24\Icons" /i "\andre\prg\prboom-plus\branches\prboom-plus-24\Icons"
# SUBTRACT RSC /i "./../VisualC6"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\ICONS\prboom.exe.manifest
# End Source File
# Begin Source File

SOURCE=..\Icons\resource.h
# End Source File
# Begin Source File

SOURCE=..\Icons\skull.ico
# End Source File
# End Group
# Begin Group "PCSound Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\pcsound\pcsound.c
# End Source File
# Begin Source File

SOURCE=..\src\pcsound\pcsound.h
# End Source File
# Begin Source File

SOURCE=..\src\pcsound\pcsound_linux.c
# End Source File
# Begin Source File

SOURCE=..\src\pcsound\pcsound_sdl.c
# End Source File
# Begin Source File

SOURCE=..\src\pcsound\pcsound_win32.c
# End Source File
# End Group
# Begin Group "TextScreen Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\TEXTSCREEN\doomkeys.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\textscreen.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_button.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_button.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_checkbox.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_checkbox.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_desktop.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_desktop.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_dropdown.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_dropdown.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_font.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_gui.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_gui.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_inputbox.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_inputbox.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_io.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_io.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_label.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_label.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_main.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_radiobutton.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_radiobutton.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_scrollpane.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_scrollpane.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_sdl.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_sdl.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_separator.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_separator.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_smallfont.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_spinctrl.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_spinctrl.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_strut.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_strut.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_table.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_table.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_widget.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_widget.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_window.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_window.h
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_window_action.c
# End Source File
# Begin Source File

SOURCE=..\src\TEXTSCREEN\txt_window_action.h
# End Source File
# End Group
# Begin Group "Music"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\MUSIC\dbopl.c
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\dbopl.h
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\dumbplayer.c
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\dumbplayer.h
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\flplayer.c
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\flplayer.h
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\madplayer.c
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\madplayer.h
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\midifile.c
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\midifile.h
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\musicplayer.h
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\opl.c
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\opl.h
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\opl_queue.c
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\opl_queue.h
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\oplplayer.c
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\oplplayer.h
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\portmidiplayer.c
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\portmidiplayer.h
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\vorbisplayer.c
# End Source File
# Begin Source File

SOURCE=..\src\MUSIC\vorbisplayer.h
# End Source File
# End Group
# Begin Source File

SOURCE=..\src\SDL\SDL_win32_main.c
# End Source File
# End Target
# End Project
