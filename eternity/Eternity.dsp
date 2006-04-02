# Microsoft Developer Studio Project File - Name="Eternity" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=Eternity - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Eternity.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Eternity.mak" CFG="Eternity - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Eternity - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "Eternity - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Eternity - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "c:\software dev\SDL-1.2.7\include" /I "c:\software dev\sdl_mixer-1.2.5\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_SDL_VER" /D "OVER_UNDER" /D "R_PORTALS" /D "AMX_NODYNALOAD" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib sdl.lib sdlmain.lib sdl_mixer.lib oldnames.lib msvcrt.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib /nologo /subsystem:console /map /machine:I386 /nodefaultlib
# SUBTRACT LINK32 /profile

!ELSEIF  "$(CFG)" == "Eternity - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "c:\software dev\SDL-1.2.7\include" /I "c:\software dev\sdl_mixer-1.2.5\include" /D "_DEBUG" /D "RANGECHECK" /D "INSTRUMENTED" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "_SDL_VER" /D "OVER_UNDER" /D "R_PORTALS" /D "AMX_NODYNALOAD" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib sdl.lib sdlmain.lib sdl_mixer.lib oldnames.lib msvcrt.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib /pdbtype:sept
# SUBTRACT LINK32 /profile /incremental:no

!ENDIF 

# Begin Target

# Name "Eternity - Win32 Release"
# Name "Eternity - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "P_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\p_anim.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_ceilng.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_chase.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_cmd.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_doors.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_enemy.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_floor.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_genlin.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_henemy.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_hubs.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_info.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_inter.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_lights.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_map.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_maputl.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_mobj.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_partcl.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_plats.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_pspr.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_saveg.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_setup.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_sight.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_skin.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_spec.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_switch.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_telept.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_tick.c
# End Source File
# Begin Source File

SOURCE=.\Source\p_user.c
# End Source File
# End Group
# Begin Group "R_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\r_bsp.c
# End Source File
# Begin Source File

SOURCE=.\Source\r_data.c
# End Source File
# Begin Source File

SOURCE=.\Source\r_draw.c
# End Source File
# Begin Source File

SOURCE=.\Source\r_main.c
# End Source File
# Begin Source File

SOURCE=.\Source\r_plane.c
# End Source File
# Begin Source File

SOURCE=.\Source\r_portal.c
# End Source File
# Begin Source File

SOURCE=.\Source\r_ripple.c
# End Source File
# Begin Source File

SOURCE=.\Source\r_segs.c
# End Source File
# Begin Source File

SOURCE=.\Source\r_sky.c
# End Source File
# Begin Source File

SOURCE=.\Source\r_things.c
# End Source File
# End Group
# Begin Group "M_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\m_argv.c
# End Source File
# Begin Source File

SOURCE=.\Source\m_bbox.c
# End Source File
# Begin Source File

SOURCE=.\Source\m_cheat.c
# End Source File
# Begin Source File

SOURCE=.\Source\m_fcvt.c
# End Source File
# Begin Source File

SOURCE=.\Source\m_misc.c
# End Source File
# Begin Source File

SOURCE=.\Source\m_qstr.c
# End Source File
# Begin Source File

SOURCE=.\Source\m_queue.c
# End Source File
# Begin Source File

SOURCE=.\Source\m_random.c
# End Source File
# End Group
# Begin Group "Mn_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\mn_engin.c
# End Source File
# Begin Source File

SOURCE=.\Source\mn_htic.c
# End Source File
# Begin Source File

SOURCE=.\Source\mn_menus.c
# End Source File
# Begin Source File

SOURCE=.\Source\mn_misc.c
# End Source File
# Begin Source File

SOURCE=.\Source\mn_skinv.c
# End Source File
# End Group
# Begin Group "C_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\c_cmd.c
# End Source File
# Begin Source File

SOURCE=.\Source\c_io.c
# End Source File
# Begin Source File

SOURCE=.\Source\c_net.c
# End Source File
# Begin Source File

SOURCE=.\Source\c_runcmd.c
# End Source File
# End Group
# Begin Group "D_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\d_deh.c

!IF  "$(CFG)" == "Eternity - Win32 Release"

# ADD CPP /O2

!ELSEIF  "$(CFG)" == "Eternity - Win32 Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Source\d_dehtbl.c
# End Source File
# Begin Source File

SOURCE=.\Source\d_dialog.c
# End Source File
# Begin Source File

SOURCE=.\Source\d_gi.c
# End Source File
# Begin Source File

SOURCE=.\Source\d_io.c
# End Source File
# Begin Source File

SOURCE=.\Source\d_items.c
# End Source File
# Begin Source File

SOURCE=.\Source\d_main.c
# End Source File
# Begin Source File

SOURCE=.\Source\d_net.c
# End Source File
# End Group
# Begin Group "HU_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\hu_frags.c
# End Source File
# Begin Source File

SOURCE=.\Source\hu_fspic.c
# End Source File
# Begin Source File

SOURCE=.\Source\hu_over.c
# End Source File
# Begin Source File

SOURCE=.\Source\hu_stuff.c
# End Source File
# End Group
# Begin Group "G_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\g_bind.c
# End Source File
# Begin Source File

SOURCE=.\Source\g_cmd.c
# End Source File
# Begin Source File

SOURCE=.\Source\g_dmflag.c
# End Source File
# Begin Source File

SOURCE=.\Source\g_game.c
# End Source File
# Begin Source File

SOURCE=.\Source\g_gfs.c
# End Source File
# End Group
# Begin Group "SDL"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\Win32\i_fnames.c
# End Source File
# Begin Source File

SOURCE=.\Source\sdl\i_main.c
# End Source File
# Begin Source File

SOURCE=.\Source\sdl\i_net.c
# End Source File
# Begin Source File

SOURCE=.\Source\sdl\i_sound.c
# End Source File
# Begin Source File

SOURCE=.\Source\sdl\i_system.c
# End Source File
# Begin Source File

SOURCE=.\Source\sdl\i_video.c
# End Source File
# Begin Source File

SOURCE=.\Source\sdl\mmus2mid.c
# End Source File
# Begin Source File

SOURCE=.\Source\sdl\ser_main.c
# End Source File
# End Group
# Begin Group "AMX"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\a_fixed.c
# End Source File
# Begin Source File

SOURCE=.\Source\a_small.c
# End Source File
# Begin Source File

SOURCE=.\Source\amx.c
# End Source File
# Begin Source File

SOURCE=.\Source\amxcore.c
# End Source File
# End Group
# Begin Group "E_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\e_cmd.c
# End Source File
# Begin Source File

SOURCE=.\Source\e_edf.c
# End Source File
# Begin Source File

SOURCE=.\Source\e_exdata.c
# End Source File
# Begin Source File

SOURCE=.\Source\e_sound.c
# End Source File
# End Group
# Begin Group "Confuse"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\Confuse\confuse.c
# End Source File
# Begin Source File

SOURCE=.\Source\Confuse\lexer.c
# End Source File
# End Group
# Begin Group "ST_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\st_hbar.c
# End Source File
# Begin Source File

SOURCE=.\Source\st_lib.c
# End Source File
# Begin Source File

SOURCE=.\Source\st_stuff.c
# End Source File
# End Group
# Begin Group "V_"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\v_block.c
# End Source File
# Begin Source File

SOURCE=.\Source\v_font.c
# End Source File
# Begin Source File

SOURCE=.\Source\v_misc.c
# End Source File
# Begin Source File

SOURCE=.\Source\v_patch.c
# End Source File
# Begin Source File

SOURCE=.\Source\v_video.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\Source\am_color.c
# End Source File
# Begin Source File

SOURCE=.\Source\am_map.c
# End Source File
# Begin Source File

SOURCE=.\Source\doomdef.c
# End Source File
# Begin Source File

SOURCE=.\Source\doomstat.c
# End Source File
# Begin Source File

SOURCE=.\Source\dstrings.c
# End Source File
# Begin Source File

SOURCE=.\Source\f_finale.c
# End Source File
# Begin Source File

SOURCE=.\Source\f_wipe.c
# End Source File
# Begin Source File

SOURCE=.\Source\hi_stuff.c
# End Source File
# Begin Source File

SOURCE=.\Source\in_lude.c
# End Source File
# Begin Source File

SOURCE=.\Source\info.c
# End Source File
# Begin Source File

SOURCE=.\Source\psnprntf.c
# End Source File
# Begin Source File

SOURCE=.\Source\s_sound.c
# End Source File
# Begin Source File

SOURCE=.\Source\sounds.c
# End Source File
# Begin Source File

SOURCE=.\Source\tables.c
# End Source File
# Begin Source File

SOURCE=.\Source\version.c
# End Source File
# Begin Source File

SOURCE=.\Source\w_wad.c
# End Source File
# Begin Source File

SOURCE=.\Source\wi_stuff.c
# End Source File
# Begin Source File

SOURCE=.\Source\z_zone.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "C"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\c_io.h
# End Source File
# Begin Source File

SOURCE=.\Source\c_net.h
# End Source File
# Begin Source File

SOURCE=.\Source\c_runcmd.h
# End Source File
# End Group
# Begin Group "D"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\d_deh.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_dehtbl.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_dialog.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_englsh.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_event.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_french.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_gi.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_io.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_items.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_keywds.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_main.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_mod.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_net.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_player.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_textur.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_think.h
# End Source File
# Begin Source File

SOURCE=.\Source\d_ticcmd.h
# End Source File
# End Group
# Begin Group "Hu"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\Hu_frags.h
# End Source File
# Begin Source File

SOURCE=.\Source\hu_fspic.h
# End Source File
# Begin Source File

SOURCE=.\Source\Hu_over.h
# End Source File
# Begin Source File

SOURCE=.\Source\Hu_stuff.h
# End Source File
# End Group
# Begin Group "I"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\Win32\i_fnames.h
# End Source File
# Begin Source File

SOURCE=.\Source\i_net.h
# End Source File
# Begin Source File

SOURCE=.\Source\i_sound.h
# End Source File
# Begin Source File

SOURCE=.\Source\i_system.h
# End Source File
# Begin Source File

SOURCE=.\Source\i_video.h
# End Source File
# Begin Source File

SOURCE=.\Source\sdl\mmus2mid.h
# End Source File
# End Group
# Begin Group "M"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\m_argv.h
# End Source File
# Begin Source File

SOURCE=.\Source\m_bbox.h
# End Source File
# Begin Source File

SOURCE=.\Source\m_cheat.h
# End Source File
# Begin Source File

SOURCE=.\Source\m_fcvt.h
# End Source File
# Begin Source File

SOURCE=.\Source\m_fixed.h
# End Source File
# Begin Source File

SOURCE=.\Source\m_misc.h
# End Source File
# Begin Source File

SOURCE=.\Source\m_qstr.h
# End Source File
# Begin Source File

SOURCE=.\Source\m_queue.h
# End Source File
# Begin Source File

SOURCE=.\Source\m_random.h
# End Source File
# Begin Source File

SOURCE=.\Source\m_swap.h
# End Source File
# End Group
# Begin Group "Mn"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\mn_engin.h
# End Source File
# Begin Source File

SOURCE=.\Source\mn_htic.h
# End Source File
# Begin Source File

SOURCE=.\Source\mn_menus.h
# End Source File
# Begin Source File

SOURCE=.\Source\mn_misc.h
# End Source File
# End Group
# Begin Group "P"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\p_anim.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_chase.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_enemy.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_hubs.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_info.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_inter.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_map.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_maputl.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_mobj.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_partcl.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_pspr.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_saveg.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_setup.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_skin.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_spec.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_tick.h
# End Source File
# Begin Source File

SOURCE=.\Source\p_user.h
# End Source File
# End Group
# Begin Group "R"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\r_bsp.h
# End Source File
# Begin Source File

SOURCE=.\Source\r_data.h
# End Source File
# Begin Source File

SOURCE=.\Source\r_defs.h
# End Source File
# Begin Source File

SOURCE=.\Source\r_draw.h
# End Source File
# Begin Source File

SOURCE=.\Source\r_main.h
# End Source File
# Begin Source File

SOURCE=.\Source\r_plane.h
# End Source File
# Begin Source File

SOURCE=.\Source\r_portal.h
# End Source File
# Begin Source File

SOURCE=.\Source\r_ripple.h
# End Source File
# Begin Source File

SOURCE=.\Source\r_segs.h
# End Source File
# Begin Source File

SOURCE=.\Source\r_sky.h
# End Source File
# Begin Source File

SOURCE=.\Source\r_state.h
# End Source File
# Begin Source File

SOURCE=.\Source\r_things.h
# End Source File
# End Group
# Begin Group "G"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\g_bind.h
# End Source File
# Begin Source File

SOURCE=.\Source\g_dmflag.h
# End Source File
# Begin Source File

SOURCE=.\Source\g_game.h
# End Source File
# Begin Source File

SOURCE=.\Source\g_gfs.h
# End Source File
# End Group
# Begin Group "E"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\e_edf.h
# End Source File
# Begin Source File

SOURCE=.\Source\e_exdata.h
# End Source File
# Begin Source File

SOURCE=.\Source\e_sound.h
# End Source File
# End Group
# Begin Group "V"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Source\v_font.h
# End Source File
# Begin Source File

SOURCE=.\Source\v_misc.h
# End Source File
# Begin Source File

SOURCE=.\Source\v_patch.h
# End Source File
# Begin Source File

SOURCE=.\Source\v_video.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Source\a_small.h
# End Source File
# Begin Source File

SOURCE=.\Source\am_map.h
# End Source File
# Begin Source File

SOURCE=.\Source\amx.h
# End Source File
# Begin Source File

SOURCE=.\Source\Confuse\confuse.h
# End Source File
# Begin Source File

SOURCE=.\Source\dhticstr.h
# End Source File
# Begin Source File

SOURCE=.\Source\doomdata.h
# End Source File
# Begin Source File

SOURCE=.\Source\doomdef.h
# End Source File
# Begin Source File

SOURCE=.\Source\doomstat.h
# End Source File
# Begin Source File

SOURCE=.\Source\doomtype.h
# End Source File
# Begin Source File

SOURCE=.\Source\dstrings.h
# End Source File
# Begin Source File

SOURCE=.\Source\f_finale.h
# End Source File
# Begin Source File

SOURCE=.\Source\f_wipe.h
# End Source File
# Begin Source File

SOURCE=.\Source\hi_stuff.h
# End Source File
# Begin Source File

SOURCE=.\Source\in_lude.h
# End Source File
# Begin Source File

SOURCE=.\Source\info.h
# End Source File
# Begin Source File

SOURCE=.\Source\osdefs.h
# End Source File
# Begin Source File

SOURCE=.\Source\psnprntf.h
# End Source File
# Begin Source File

SOURCE=.\Source\s_sound.h
# End Source File
# Begin Source File

SOURCE=.\Source\sounds.h
# End Source File
# Begin Source File

SOURCE=.\Source\st_lib.h
# End Source File
# Begin Source File

SOURCE=.\Source\st_stuff.h
# End Source File
# Begin Source File

SOURCE=.\Source\tables.h
# End Source File
# Begin Source File

SOURCE=.\Source\version.h
# End Source File
# Begin Source File

SOURCE=.\Source\w_wad.h
# End Source File
# Begin Source File

SOURCE=.\Source\wi_stuff.h
# End Source File
# Begin Source File

SOURCE=.\Source\z_zone.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
