# Microsoft Developer Studio Project File - Name="Doom" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Doom - Win32 Debug OpenGL
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Doom.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Doom.mak" CFG="Doom - Win32 Debug OpenGL"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Doom - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Doom - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "Doom - Win32 Release OpenGL" (based on "Win32 (x86) Application")
!MESSAGE "Doom - Win32 Debug OpenGL" (based on "Win32 (x86) Application")
!MESSAGE "Doom - Win32 Release NOASM" (based on "Win32 (x86) Application")
!MESSAGE "Doom - Win32 Release OpenGL NOASM" (based on\
 "Win32 (x86) Application")
!MESSAGE 

# Begin Project
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
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\src" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "I386_ASM" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 user32.lib gdi32.lib sdl.lib sdlmain.lib sdl_mixer.lib sdl_net.lib /nologo /subsystem:windows /machine:I386 /out:"Release/prboom.exe"

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
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\src" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "INSTRUMENTED" /D "RANGECHECK" /D "SIMPLECHECKS" /D "TIMEDIAG" /D "HEAPDUMP" /D "HAVE_CONFIG_H" /D "I386_ASM" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 user32.lib gdi32.lib sdl.lib sdlmain.lib sdl_mixer.lib sdl_net.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/prboom.exe" /pdbtype:sept

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_SDL" /D "HIGHRES" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\src" /D "NDEBUG" /D "GL_DOOM" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D "I386_ASM" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib sdl.lib sdlmain.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 opengl32.lib glu32.lib user32.lib gdi32.lib sdl.lib sdlmain.lib sdl_mixer.lib sdl_net.lib /nologo /subsystem:windows /machine:I386 /out:"ReleaseGL/glboom.exe"
# SUBTRACT LINK32 /profile /debug

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
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "USE_SDL" /D "HIGHRES" /D "INSTRUMENTED" /D "RANGECHECK" /D "SIMPLECHECKS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /I "..\src" /D "GL_DOOM" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "INSTRUMENTED" /D "RANGECHECK" /D "SIMPLECHECKS" /D "TIMEDIAG" /D "HEAPDUMP" /D "HAVE_CONFIG_H" /D "I386_ASM" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib sdl.lib sdlmain.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 opengl32.lib glu32.lib user32.lib gdi32.lib sdl.lib sdlmain.lib sdl_mixer.lib sdl_net.lib /nologo /subsystem:windows /debug /machine:I386 /out:"DebugGL/glboom.exe" /pdbtype:sept

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\src" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\src" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib sdl.lib sdlmain.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 user32.lib gdi32.lib sdl.lib sdlmain.lib sdl_mixer.lib sdl_net.lib /nologo /subsystem:windows /machine:I386 /out:"ReleaseNOASM/prboom.exe"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\src" /D "NDEBUG" /D "GL_DOOM" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\src" /D "NDEBUG" /D "GL_DOOM" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib sdl.lib sdlmain.lib opengl32.lib glu32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /profile /debug
# ADD LINK32 opengl32.lib glu32.lib user32.lib gdi32.lib sdl.lib sdlmain.lib sdl_mixer.lib sdl_net.lib /nologo /subsystem:windows /machine:I386 /out:"ReleaseNOASMGL/glboom.exe"
# SUBTRACT LINK32 /profile /debug

!ENDIF 

# Begin Target

# Name "Doom - Win32 Release"
# Name "Doom - Win32 Debug"
# Name "Doom - Win32 Release OpenGL"
# Name "Doom - Win32 Debug OpenGL"
# Name "Doom - Win32 Release NOASM"
# Name "Doom - Win32 Release OpenGL NOASM"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\am_map.c
# End Source File
# Begin Source File

SOURCE=..\src\am_map.h
# End Source File
# Begin Source File

SOURCE=..\config.h
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

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\f_wipe.h

!IF  "$(CFG)" == "Doom - Win32 Release"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\src\g_game.c
# End Source File
# Begin Source File

SOURCE=..\src\g_game.h
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

SOURCE=..\src\i_joy.c
# End Source File
# Begin Source File

SOURCE=..\src\i_joy.h
# End Source File
# Begin Source File

SOURCE=..\src\i_main.c
# End Source File
# Begin Source File

SOURCE=..\src\i_main.h
# End Source File
# Begin Source File

SOURCE=..\src\i_net.h
# End Source File
# Begin Source File

SOURCE=..\src\i_network.h
# End Source File
# Begin Source File

SOURCE=..\src\sdl\i_sound.c
# End Source File
# Begin Source File

SOURCE=..\src\i_sound.h
# End Source File
# Begin Source File

SOURCE=..\src\sdl\i_system.c
# End Source File
# Begin Source File

SOURCE=..\src\i_system.h
# End Source File
# Begin Source File

SOURCE=..\src\i_udp_sdl.c
# End Source File
# Begin Source File

SOURCE=..\src\sdl\i_video.c
# End Source File
# Begin Source File

SOURCE=..\src\i_video.h
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

SOURCE=..\src\Mmus2mid.c
# End Source File
# Begin Source File

SOURCE=..\src\Mmus2mid.h
# End Source File
# Begin Source File

SOURCE=..\src\p_ceilng.c
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

SOURCE=..\src\qmus2mid.c
# End Source File
# Begin Source File

SOURCE=..\src\qmus2mid.h
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

SOURCE=..\src\r_draw.c
# End Source File
# Begin Source File

SOURCE=..\src\r_draw.h
# End Source File
# Begin Source File

SOURCE=..\src\r_main.c
# End Source File
# Begin Source File

SOURCE=..\src\r_main.h
# End Source File
# Begin Source File

SOURCE=..\src\r_plane.c
# End Source File
# Begin Source File

SOURCE=..\src\r_plane.h
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

SOURCE=..\src\s_sound.c
# End Source File
# Begin Source File

SOURCE=..\src\s_sound.h
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

SOURCE=..\src\v_video_trans.c
# End Source File
# Begin Source File

SOURCE=..\src\v_video_trans.h
# End Source File
# Begin Source File

SOURCE=..\src\version.c
# End Source File
# Begin Source File

SOURCE=..\src\version.h
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

!ENDIF 

# End Source File
# End Group
# Begin Group "Asm Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\src\Drawasm.nas

!IF  "$(CFG)" == "Doom - Win32 Release"

# Begin Custom Build - Compiling drawasm.nas
IntDir=.\Release
InputPath=..\src\Drawasm.nas
InputName=Drawasm

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

# Begin Custom Build - Compiling drawasm.nas
IntDir=.\Debug
InputPath=..\src\Drawasm.nas
InputName=Drawasm

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\Icons\icons.rc

!IF  "$(CFG)" == "Doom - Win32 Release"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Debug OpenGL"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release NOASM"

!ELSEIF  "$(CFG)" == "Doom - Win32 Release OpenGL NOASM"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\Icons\resource.h
# End Source File
# End Group
# End Target
# End Project
