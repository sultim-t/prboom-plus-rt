# Microsoft Developer Studio Project File - Name="prboom" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=prboom - Win32 Debug GL
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "prboom.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "prboom.mak" CFG="prboom - Win32 Debug GL"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "prboom - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "prboom - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "prboom - Win32 Release_NOASM" (based on "Win32 (x86) Application")
!MESSAGE "prboom - Win32 Release GL" (based on "Win32 (x86) Application")
!MESSAGE "prboom - Win32 Debug GL" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "prboom - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj/win32"
# PROP Intermediate_Dir "obj/win32"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G4 /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "DOGS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib sdl.lib sdlmain.lib /nologo /subsystem:windows /machine:I386 /out:"bin/win32/prboom.exe"
# SUBTRACT LINK32 /profile /incremental:yes /debug

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "objdebug/win32"
# PROP Intermediate_Dir "objdebug/win32"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "INSTRUMENTED" /D "RANGECHECK" /D "ZONEIDCHECK" /D "WIN32" /D "_WINDOWS" /D "DOGS" /YX /FD /c
# SUBTRACT CPP /WX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib sdl.lib sdlmain.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /out:"bindebug/win32/prboom.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "prboom - Win32 Release_NOASM"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "prboom__"
# PROP BASE Intermediate_Dir "prboom__"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj/win32woasm"
# PROP Intermediate_Dir "obj/win32woasm"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "NDEBUG" /D "NOASM" /D "WIN32" /D "_WINDOWS" /D "DOGS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib dsound.lib ddraw.lib wsock32.lib /nologo /subsystem:windows /machine:I386 /out:"bin/win32/prboom.exe"
# SUBTRACT BASE LINK32 /profile /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib sdl.lib sdlmain.lib /nologo /subsystem:windows /machine:I386 /out:"bin/win32woasm/prboom.exe"
# SUBTRACT LINK32 /profile /debug

!ELSEIF  "$(CFG)" == "prboom - Win32 Release GL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "prboom__"
# PROP BASE Intermediate_Dir "prboom__"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "obj/win32gl"
# PROP Intermediate_Dir "obj/win32gl"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G4 /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /G4 /W3 /GX /O2 /D "NDEBUG" /D "GL_DOOM" /D "WIN32" /D "_WINDOWS" /D "DOGS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib dsound.lib ddraw.lib wsock32.lib /nologo /subsystem:windows /machine:I386 /out:"bin/win32/prboom.exe"
# SUBTRACT BASE LINK32 /profile /incremental:yes /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib opengl32.lib glu32.lib sdl.lib sdlmain.lib /nologo /subsystem:windows /profile /machine:I386 /out:"bin/win32gl/glboom.exe"
# SUBTRACT LINK32 /debug

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug GL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "prboom_0"
# PROP BASE Intermediate_Dir "prboom_0"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "objdebug/win32gl"
# PROP Intermediate_Dir "objdebug/win32gl"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "INSTRUMENTED" /D "RANGECHECK" /YX /FD /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "INSTRUMENTED" /D "RANGECHECK" /D "ZONEIDCHECK" /D "GL_DOOM" /D "WIN32" /D "_WINDOWS" /D "DOGS" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib ddraw.lib dsound.lib wsock32.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /out:"bindebug/win32/prboom.exe"
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winmm.lib wsock32.lib opengl32.lib glu32.lib sdl.lib sdlmain.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386 /out:"bindebug/win32gl/glboom.exe"
# SUBTRACT LINK32 /profile

!ENDIF 

# Begin Target

# Name "prboom - Win32 Release"
# Name "prboom - Win32 Debug"
# Name "prboom - Win32 Release_NOASM"
# Name "prboom - Win32 Release GL"
# Name "prboom - Win32 Debug GL"
# Begin Group "Source Files"

# PROP Default_Filter "c;h;nas"
# Begin Source File

SOURCE=.\src\Am_map.c
# End Source File
# Begin Source File

SOURCE=.\src\Am_map.h
# End Source File
# Begin Source File

SOURCE=.\src\D_deh.c
# End Source File
# Begin Source File

SOURCE=.\src\D_deh.h
# End Source File
# Begin Source File

SOURCE=.\src\D_englsh.h
# End Source File
# Begin Source File

SOURCE=.\src\D_event.h
# End Source File
# Begin Source File

SOURCE=.\src\D_french.h
# End Source File
# Begin Source File

SOURCE=.\src\D_items.c
# End Source File
# Begin Source File

SOURCE=.\src\D_items.h
# End Source File
# Begin Source File

SOURCE=.\src\D_main.c
# End Source File
# Begin Source File

SOURCE=.\src\D_main.h
# End Source File
# Begin Source File

SOURCE=.\src\D_net.c
# End Source File
# Begin Source File

SOURCE=.\src\D_net.h
# End Source File
# Begin Source File

SOURCE=.\src\D_player.h
# End Source File
# Begin Source File

SOURCE=.\src\D_textur.h
# End Source File
# Begin Source File

SOURCE=.\src\D_think.h
# End Source File
# Begin Source File

SOURCE=.\src\D_ticcmd.h
# End Source File
# Begin Source File

SOURCE=.\src\Doomdata.h
# End Source File
# Begin Source File

SOURCE=.\src\Doomdef.c
# End Source File
# Begin Source File

SOURCE=.\src\Doomdef.h
# End Source File
# Begin Source File

SOURCE=.\src\Doomstat.c
# End Source File
# Begin Source File

SOURCE=.\src\Doomstat.h
# End Source File
# Begin Source File

SOURCE=.\src\Doomtype.h
# End Source File
# Begin Source File

SOURCE=.\src\drawasm.nas

!IF  "$(CFG)" == "prboom - Win32 Release"

# Begin Custom Build - Compiling drawasm.nas
IntDir=.\obj/win32
InputPath=.\src\drawasm.nas
InputName=drawasm

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug"

# Begin Custom Build - Compiling drawasm.nas
IntDir=.\objdebug/win32
InputPath=.\src\drawasm.nas
InputName=drawasm

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasmw -o $(IntDir)\$(InputName).obj -f win32 $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "prboom - Win32 Release_NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Release GL"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug GL"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\Dstrings.c
# End Source File
# Begin Source File

SOURCE=.\src\Dstrings.h
# End Source File
# Begin Source File

SOURCE=.\src\F_finale.c
# End Source File
# Begin Source File

SOURCE=.\src\F_finale.h
# End Source File
# Begin Source File

SOURCE=.\src\F_wipe.c
# End Source File
# Begin Source File

SOURCE=.\src\F_wipe.h
# End Source File
# Begin Source File

SOURCE=.\src\G_game.c
# End Source File
# Begin Source File

SOURCE=.\src\G_game.h
# End Source File
# Begin Source File

SOURCE=.\src\Hu_conf.c
# End Source File
# Begin Source File

SOURCE=.\src\Hu_conf.h
# End Source File
# Begin Source File

SOURCE=.\src\Hu_lib.c
# End Source File
# Begin Source File

SOURCE=.\src\Hu_lib.h
# End Source File
# Begin Source File

SOURCE=.\src\Hu_stuff.c
# End Source File
# Begin Source File

SOURCE=.\src\Hu_stuff.h
# End Source File
# Begin Source File

SOURCE=.\src\I_main.c
# End Source File
# Begin Source File

SOURCE=.\src\I_net.c
# End Source File
# Begin Source File

SOURCE=.\src\I_net.h
# End Source File
# Begin Source File

SOURCE=.\src\I_sound.c
# End Source File
# Begin Source File

SOURCE=.\src\I_sound.h
# End Source File
# Begin Source File

SOURCE=.\src\I_system.c
# End Source File
# Begin Source File

SOURCE=.\src\I_system.h
# End Source File
# Begin Source File

SOURCE=.\src\I_video.c
# End Source File
# Begin Source File

SOURCE=.\src\I_video.h
# End Source File
# Begin Source File

SOURCE=.\src\Info.c
# End Source File
# Begin Source File

SOURCE=.\src\Info.h
# End Source File
# Begin Source File

SOURCE=.\src\lprintf.c
# End Source File
# Begin Source File

SOURCE=.\src\lprintf.h
# End Source File
# Begin Source File

SOURCE=.\src\M_argv.c
# End Source File
# Begin Source File

SOURCE=.\src\M_argv.h
# End Source File
# Begin Source File

SOURCE=.\src\M_bbox.c
# End Source File
# Begin Source File

SOURCE=.\src\M_bbox.h
# End Source File
# Begin Source File

SOURCE=.\src\M_cheat.c
# End Source File
# Begin Source File

SOURCE=.\src\M_cheat.h
# End Source File
# Begin Source File

SOURCE=.\src\M_fixed.h
# End Source File
# Begin Source File

SOURCE=.\src\M_menu.c
# End Source File
# Begin Source File

SOURCE=.\src\M_menu.h
# End Source File
# Begin Source File

SOURCE=.\src\M_misc.c
# End Source File
# Begin Source File

SOURCE=.\src\M_misc.h
# End Source File
# Begin Source File

SOURCE=.\src\M_random.c
# End Source File
# Begin Source File

SOURCE=.\src\M_random.h
# End Source File
# Begin Source File

SOURCE=.\src\M_swap.h
# End Source File
# Begin Source File

SOURCE=.\src\Mmus2mid.c
# End Source File
# Begin Source File

SOURCE=.\src\Mmus2mid.h
# End Source File
# Begin Source File

SOURCE=.\src\P_ceilng.c
# End Source File
# Begin Source File

SOURCE=.\src\P_doors.c
# End Source File
# Begin Source File

SOURCE=.\src\P_enemy.c
# End Source File
# Begin Source File

SOURCE=.\src\P_enemy.h
# End Source File
# Begin Source File

SOURCE=.\src\P_floor.c
# End Source File
# Begin Source File

SOURCE=.\src\P_genlin.c
# End Source File
# Begin Source File

SOURCE=.\src\P_inter.c
# End Source File
# Begin Source File

SOURCE=.\src\P_inter.h
# End Source File
# Begin Source File

SOURCE=.\src\P_lights.c
# End Source File
# Begin Source File

SOURCE=.\src\P_map.c
# End Source File
# Begin Source File

SOURCE=.\src\P_map.h
# End Source File
# Begin Source File

SOURCE=.\src\P_maputl.c
# End Source File
# Begin Source File

SOURCE=.\src\P_maputl.h
# End Source File
# Begin Source File

SOURCE=.\src\P_mobj.c
# End Source File
# Begin Source File

SOURCE=.\src\P_mobj.h
# End Source File
# Begin Source File

SOURCE=.\src\P_plats.c
# End Source File
# Begin Source File

SOURCE=.\src\P_pspr.c
# End Source File
# Begin Source File

SOURCE=.\src\P_pspr.h
# End Source File
# Begin Source File

SOURCE=.\src\P_saveg.c
# End Source File
# Begin Source File

SOURCE=.\src\P_saveg.h
# End Source File
# Begin Source File

SOURCE=.\src\P_setup.c
# End Source File
# Begin Source File

SOURCE=.\src\P_setup.h
# End Source File
# Begin Source File

SOURCE=.\src\P_sight.c
# End Source File
# Begin Source File

SOURCE=.\src\P_spec.c
# End Source File
# Begin Source File

SOURCE=.\src\P_spec.h
# End Source File
# Begin Source File

SOURCE=.\src\P_switch.c
# End Source File
# Begin Source File

SOURCE=.\src\P_telept.c
# End Source File
# Begin Source File

SOURCE=.\src\P_tick.c
# End Source File
# Begin Source File

SOURCE=.\src\P_tick.h
# End Source File
# Begin Source File

SOURCE=.\src\P_user.c
# End Source File
# Begin Source File

SOURCE=.\src\P_user.h
# End Source File
# Begin Source File

SOURCE=.\src\R_bsp.c
# End Source File
# Begin Source File

SOURCE=.\src\R_bsp.h
# End Source File
# Begin Source File

SOURCE=.\src\R_data.c
# End Source File
# Begin Source File

SOURCE=.\src\R_data.h
# End Source File
# Begin Source File

SOURCE=.\src\R_defs.h
# End Source File
# Begin Source File

SOURCE=.\src\R_draw.c
# End Source File
# Begin Source File

SOURCE=.\src\R_draw.h
# End Source File
# Begin Source File

SOURCE=.\src\R_main.c
# End Source File
# Begin Source File

SOURCE=.\src\R_main.h
# End Source File
# Begin Source File

SOURCE=.\src\R_plane.c
# End Source File
# Begin Source File

SOURCE=.\src\R_plane.h
# End Source File
# Begin Source File

SOURCE=.\src\R_segs.c
# End Source File
# Begin Source File

SOURCE=.\src\R_segs.h
# End Source File
# Begin Source File

SOURCE=.\src\R_sky.c
# End Source File
# Begin Source File

SOURCE=.\src\R_sky.h
# End Source File
# Begin Source File

SOURCE=.\src\R_state.h
# End Source File
# Begin Source File

SOURCE=.\src\R_things.c
# End Source File
# Begin Source File

SOURCE=.\src\R_things.h
# End Source File
# Begin Source File

SOURCE=.\src\S_sound.c
# End Source File
# Begin Source File

SOURCE=.\src\S_sound.h
# End Source File
# Begin Source File

SOURCE=.\src\Sounds.c
# End Source File
# Begin Source File

SOURCE=.\src\Sounds.h
# End Source File
# Begin Source File

SOURCE=.\src\St_lib.c
# End Source File
# Begin Source File

SOURCE=.\src\St_lib.h
# End Source File
# Begin Source File

SOURCE=.\src\St_stuff.c
# End Source File
# Begin Source File

SOURCE=.\src\St_stuff.h
# End Source File
# Begin Source File

SOURCE=.\src\Tables.c
# End Source File
# Begin Source File

SOURCE=.\src\Tables.h
# End Source File
# Begin Source File

SOURCE=.\src\V_video.c
# End Source File
# Begin Source File

SOURCE=.\src\V_video.h
# End Source File
# Begin Source File

SOURCE=.\src\Version.c
# End Source File
# Begin Source File

SOURCE=.\src\Version.h
# End Source File
# Begin Source File

SOURCE=.\src\vscale.nas
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\src\W_wad.c
# End Source File
# Begin Source File

SOURCE=.\src\W_wad.h
# End Source File
# Begin Source File

SOURCE=.\src\Wi_stuff.c
# End Source File
# Begin Source File

SOURCE=.\src\Wi_stuff.h
# End Source File
# Begin Source File

SOURCE=.\src\Winstuff.c
# End Source File
# Begin Source File

SOURCE=.\src\Winstuff.h
# End Source File
# Begin Source File

SOURCE=.\src\Z_zone.c
# End Source File
# Begin Source File

SOURCE=.\src\Z_zone.h
# End Source File
# End Group
# Begin Group "Text Files"

# PROP Default_Filter ""
# Begin Group "boom"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\txts\boom\Boom.txt
# End Source File
# Begin Source File

SOURCE=.\txts\boom\Boomdeh.txt
# End Source File
# Begin Source File

SOURCE=.\txts\boom\Boomref.txt
# End Source File
# Begin Source File

SOURCE=.\txts\boom\Boomsrc.txt
# End Source File
# Begin Source File

SOURCE=.\txts\boom\Copying
# End Source File
# Begin Source File

SOURCE=.\txts\boom\Log_jff.txt
# End Source File
# Begin Source File

SOURCE=.\txts\boom\Log_lee.txt
# End Source File
# Begin Source File

SOURCE=.\txts\boom\Log_rsp.txt
# End Source File
# Begin Source File

SOURCE=.\txts\boom\Log_ty.txt
# End Source File
# Begin Source File

SOURCE=.\txts\boom\Snddrvr.txt
# End Source File
# End Group
# Begin Group "doom"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\txts\doom\ChangeLog
# End Source File
# Begin Source File

SOURCE=.\txts\doom\Doomlic.txt
# End Source File
# Begin Source File

SOURCE=.\txts\doom\Files
# End Source File
# Begin Source File

SOURCE=.\txts\doom\Files2
# End Source File
# Begin Source File

SOURCE=.\txts\doom\Makefile
# End Source File
# Begin Source File

SOURCE=.\txts\doom\Readme.asm
# End Source File
# Begin Source File

SOURCE=.\txts\doom\Readme.b
# End Source File
# Begin Source File

SOURCE=.\txts\doom\README.book
# End Source File
# Begin Source File

SOURCE=.\txts\doom\Readme.gl
# End Source File
# Begin Source File

SOURCE=.\txts\doom\README.sound
# End Source File
# Begin Source File

SOURCE=.\txts\doom\Readme.txt
# End Source File
# Begin Source File

SOURCE=.\txts\doom\Todo
# End Source File
# End Group
# Begin Source File

SOURCE=.\txts\compile.txt
# End Source File
# Begin Source File

SOURCE=.\txts\Log_pr.txt
# End Source File
# Begin Source File

SOURCE=.\txts\Readme.txt
# End Source File
# Begin Source File

SOURCE=.\txts\whatsnew.txt
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\icons\barrel.ico
# End Source File
# Begin Source File

SOURCE=.\icons\fouch.ico
# End Source File
# Begin Source File

SOURCE=.\icons\god.ico
# End Source File
# Begin Source File

SOURCE=.\icons\Heada1.ico
# End Source File
# Begin Source File

SOURCE=.\icons\icons.rc

!IF  "$(CFG)" == "prboom - Win32 Release"

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug"

!ELSEIF  "$(CFG)" == "prboom - Win32 Release_NOASM"

!ELSEIF  "$(CFG)" == "prboom - Win32 Release GL"

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug GL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\icons\skull.ico
# End Source File
# End Group
# Begin Group "GL_Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\src\gl_intern.h

!IF  "$(CFG)" == "prboom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Release_NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Release GL"

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug GL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_pr_tex.c

!IF  "$(CFG)" == "prboom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Release_NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Release GL"

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug GL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_proff.c

!IF  "$(CFG)" == "prboom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Release_NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Release GL"

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug GL"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\src\gl_struct.h

!IF  "$(CFG)" == "prboom - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Release_NOASM"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Release GL"

# PROP BASE Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "prboom - Win32 Debug GL"

# PROP BASE Exclude_From_Build 1

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
