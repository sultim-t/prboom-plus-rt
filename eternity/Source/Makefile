############################################################################
# Eternity Makefile

MODE = RELEASE
O = $(O_$(MODE))
O_RELEASE = obj
O_DEBUG = objdebug
DEPENDENCIES = 1

# 15/10/99 sf: added multi-platform support

# select platform here:
DJGPPDOS=1

################################ DJGPP ####################################

ifdef DJGPPDOS

PLATFORM = djgpp
        
# compiler
CC=gcc
        
# the command you use to delete files
RM=del
        
# the command you use to copy files
CP=copy /y
        
# the exe file name -sf
EXE=$(O)/eternity.exe
        
# options common to all builds
# Note on NO_FCVT: My version of DJGPP doesn't have fcvt. If yours does, you 
# can remove -DNO_FCVT to save a couple kb of redundant code.
CFLAGS_COMMON=-Wall -DNO_FCVT

# new features; comment out what you don't want at the moment
CFLAGS_NEWFEATURES=-DOVER_UNDER -DR_PORTAL
        
# debug options
CFLAGS_DEBUG=-g -DRANGECHECK -DINSTRUMENTED
LDFLAGS_DEBUG=
        
# optimized (release) options
CFLAGS_RELEASE=-O3 -ffast-math -fomit-frame-pointer -m486 -mreg-alloc=adcbSDB
LDFLAGS_RELEASE=
# -s
       
# libraries to link in
LIBS=-lalleg -lm -lemu
        
# this selects flags based on debug and release tagets
CFLAGS=$(CFLAGS_COMMON)  $(CFLAGS_$(MODE)) $(CFLAGS_NEWFEATURES)
LDFLAGS=$(LDFLAGS_COMMON) $(LDFLAGS_$(MODE))
        
# system-specific object files
PLATOBJS =                  \
	$(O)/i_main.o       \
	$(O)/i_system.o     \
	$(O)/i_sound.o      \
	$(O)/i_video.o      \
	$(O)/i_net.o        \
	$(O)/ser_main.o     \
	$(O)/ser_port.o     \
	$(O)/keyboard.o     \
	$(O)/mmus2mid.o     \
	$(O)/pproblit.o     \
	$(O)/drawspan.o     \
	$(O)/emu8kmid.o
# haleyjd: this is currently out of sync with the C version, needs fixing
#	     \
#	$(O)/drawcol.o

build : $(EXE)

$(EXE): $(OBJS) $(O)/version.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(O)/version.o -o $@ $(LIBS)

release: sources binaries

sources:
# build source zip
	make clean
	$(RM) etc-src.zip
	pkzip -a -ex -rp etc-src

binaries:
# build binaries zip
	del $(O)\eternity.exe
	make build
	strip $(O)/eternity.exe
	djp $(O)/eternity.exe
	if exist etc.zip del etc.zip
	pkzip -a -ex eternity $(O)/eternity.exe $(O)/eternity.wad               \
			smmu.txt smmuedit.txt                       \
			doomlic.txt copying copying.dj 
	pkzip -a -ex -rp smmu examples/*.wad

clean:
	del *.c~
	del *.h~
	del $(O)\tranmap.dat
	del $(O)\eternity.cfg
	if exist $(O_RELEASE)\*.exe del $(O_RELEASE)\*.exe
	if exist $(O_DEBUG)\*.exe del $(O_DEBUG)\*.exe
	if exist $(O_RELEASE)\*.o del $(O_RELEASE)\*.o
	if exist $(O_DEBUG)\*.o del $(O_DEBUG)\*.o

endif

nothing : 
	@echo SMMU makefile:
	@echo you need to specify a platform in the Makefile!

############################## OBJECT FILES #################################

# subdirectory for objects (depends on target, to allow you
# to build debug and release versions simultaneously)

# object files
OBJS=   \
	$(PLATOBJS) \
	$(O)/amx.o \
	$(O)/amxcore.o \
	$(O)/am_color.o \
	$(O)/am_map.o \
	$(O)/a_fixed.o \
	$(O)/a_small.o \
	$(O)/c_cmd.o \
	$(O)/c_io.o \
	$(O)/c_net.o \
	$(O)/c_runcmd.o \
	$(O)/doomdef.o \
	$(O)/doomstat.o \
	$(O)/dstrings.o \
	$(O)/d_deh.o \
	$(O)/d_dehtbl.o \
	$(O)/d_dialog.o \
	$(O)/d_gi.o \
	$(O)/d_io.o \
	$(O)/d_items.o \
	$(O)/d_main.o \
	$(O)/d_net.o \
	$(O)/e_cmd.o \
	$(O)/e_edf.o \
	$(O)/e_exdata.o \
	$(O)/e_sound.o \
	$(O)/f_finale.o \
	$(O)/f_wipe.o \
	$(O)/g_bind.o \
	$(O)/g_cmd.o \
	$(O)/g_dmflag.o \
	$(O)/g_game.o \
	$(O)/g_gfs.o \
	$(O)/hi_stuff.o \
	$(O)/hu_frags.o \
	$(O)/hu_over.o \
	$(O)/hu_stuff.o \
	$(O)/info.o \
	$(O)/in_lude.o \
	$(O)/mn_engin.o \
	$(O)/mn_htic.o \
	$(O)/mn_menus.o \
	$(O)/mn_misc.o \
	$(O)/mn_skinv.o \
	$(O)/m_argv.o \
	$(O)/m_bbox.o \
	$(O)/m_cheat.o \
	$(O)/m_fcvt.o \
	$(O)/m_misc.o \
	$(O)/m_qstr.o \
	$(O)/m_queue.o \
	$(O)/m_random.o \
	$(O)/psnprntf.o \
	$(O)/p_anim.o \
	$(O)/p_ceilng.o \
	$(O)/p_chase.o \
	$(O)/p_cmd.o \
	$(O)/p_doors.o \
	$(O)/p_enemy.o \
	$(O)/p_floor.o \
	$(O)/p_genlin.o \
	$(O)/p_henemy.o \
	$(O)/p_hubs.o \
	$(O)/p_info.o \
	$(O)/p_inter.o \
	$(O)/p_lights.o \
	$(O)/p_map.o \
	$(O)/p_maputl.o \
	$(O)/p_mobj.o \
	$(O)/p_partcl.o \
	$(O)/p_plats.o \
	$(O)/p_pspr.o \
	$(O)/p_saveg.o \
	$(O)/p_setup.o \
	$(O)/p_sight.o \
	$(O)/p_skin.o \
	$(O)/p_spec.o \
	$(O)/p_switch.o \
	$(O)/p_telept.o \
	$(O)/p_tick.o \
	$(O)/p_user.o \
	$(O)/r_bsp.o \
	$(O)/r_data.o \
	$(O)/r_draw.o \
	$(O)/r_main.o \
	$(O)/r_plane.o \
	$(O)/r_portal.o \
	$(O)/r_ripple.o \
	$(O)/r_segs.o \
	$(O)/r_sky.o \
	$(O)/r_things.o \
	$(O)/sounds.o \
	$(O)/st_hbar.o \
	$(O)/st_lib.o \
	$(O)/st_stuff.o \
	$(O)/s_sound.o \
	$(O)/tables.o \
	$(O)/v_block.o \
	$(O)/v_misc.o \
	$(O)/v_patch.o \
	$(O)/v_video.o \
	$(O)/wi_stuff.o \
	$(O)/w_wad.o \
	$(O)/z_zone.o \
	$(O)/confuse.o \
	$(O)/lexer.o

debug:
	$(MAKE) MODE=DEBUG

$(O)/%.o:   %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(O)/%.o:   $(PLATFORM)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	
$(O)/%.o:   Confuse/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(O)/%.o:   $(PLATFORM)/%.s
	$(CC) $(CFLAGS) -c $< -o $@

# Very important that all sources #include this one
$(OBJS): z_zone.h

# If you change the makefile, everything should rebuild
$(OBJS): Makefile

# individual file depedencies follow

ifdef DEPENDENCIES

# rebuild version.c if anything changes
$(O)/version.o: version.c $(OBJS)

$(O)/a_fixed.o: a_fixed.c m_fixed.h d_keywds.h i_system.h d_ticcmd.h doomtype.h \
 a_small.h p_mobj.h tables.h d_think.h doomdata.h info.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h doomdef.h m_swap.h version.h d_mod.h r_portal.h p_partcl.h amx.h
$(O)/a_small.o: a_small.c z_zone.h d_keywds.h psnprntf.h doomtype.h w_wad.h \
 v_misc.h c_io.h doomstat.h doomdata.h d_net.h d_player.h d_items.h \
 doomdef.h m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h \
 d_ticcmd.h tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h \
 st_stuff.h d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h \
 c_net.h c_runcmd.h p_spec.h p_info.h m_misc.h a_small.h amx.h
$(O)/am_color.o: am_color.c doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h c_runcmd.h
$(O)/am_map.o: am_map.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 d_main.h p_chase.h r_main.h r_data.h r_state.h p_setup.h p_maputl.h \
 r_draw.h w_wad.h v_video.h v_misc.h p_spec.h am_map.h dstrings.h \
 d_englsh.h d_deh.h
$(O)/amx.o: amx.c osdefs.h amx.h
$(O)/amxcore.o: amxcore.c amx.h
$(O)/c_cmd.o: c_cmd.c c_io.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 c_runcmd.h c_net.h m_random.h a_small.h amx.h
$(O)/c_io.o: c_io.c z_zone.h d_keywds.h psnprntf.h d_io.h doomtype.h c_io.h \
 doomstat.h doomdata.h d_net.h d_player.h d_items.h doomdef.h m_swap.h \
 version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h \
 info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h \
 r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h c_runcmd.h \
 c_net.h d_main.h g_game.h hu_stuff.h hu_over.h i_video.h \
 w_wad.h s_sound.h d_gi.h mn_engin.h \
 mn_menus.h mn_htic.h g_bind.h a_small.h amx.h
$(O)/c_net.o: c_net.c z_zone.h d_keywds.h psnprntf.h c_io.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h doomdef.h m_swap.h \
 version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h \
 info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h \
 r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h c_runcmd.h \
 c_net.h d_main.h g_game.h dstrings.h d_englsh.h
$(O)/c_runcmd.o: c_runcmd.c z_zone.h d_keywds.h psnprntf.h d_io.h \
 doomtype.h c_io.h doomstat.h doomdata.h d_net.h d_player.h d_items.h \
 doomdef.h m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h \
 d_ticcmd.h tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h \
 st_stuff.h d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h \
 v_misc.h c_runcmd.h c_net.h mn_engin.h g_game.h
$(O)/d_deh.o: d_deh.c doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h \
 version.h d_mod.h doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 d_io.h m_cheat.h p_inter.h g_game.h d_main.h p_chase.h w_wad.h \
 m_misc.h d_dehtbl.h e_edf.h
$(O)/d_dehtbl.o: d_dehtbl.c z_zone.h d_keywds.h psnprntf.h m_fixed.h \
 i_system.h d_ticcmd.h doomtype.h d_io.h doomdef.h m_swap.h version.h \
 d_mod.h dstrings.h d_englsh.h dhticstr.h d_dehtbl.h sounds.h info.h \
 d_think.h
$(O)/d_dialog.o: d_dialog.c z_zone.h d_keywds.h psnprntf.h d_main.h \
 d_event.h doomtype.h p_chase.h p_mobj.h tables.h m_fixed.h i_system.h \
 d_ticcmd.h d_think.h doomdata.h info.h p_skin.h sounds.h st_stuff.h \
 r_defs.h doomdef.h m_swap.h version.h d_mod.h d_player.h d_items.h \
 p_pspr.h doomstat.h d_net.h d_dialog.h hu_stuff.h v_video.h r_data.h \
 r_state.h v_misc.h w_wad.h
$(O)/d_gi.o: d_gi.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 hi_stuff.h wi_stuff.h d_gi.h mn_engin.h c_runcmd.h mn_menus.h \
 mn_htic.h v_video.h r_data.h r_state.h p_chase.h v_misc.h p_info.h \
 c_io.h e_edf.h
$(O)/d_io.o: d_io.c z_zone.h d_keywds.h psnprntf.h doomtype.h d_io.h \
 w_wad.h
$(O)/d_items.o: d_items.c info.h d_think.h d_items.h doomdef.h d_keywds.h \
 z_zone.h psnprntf.h m_swap.h version.h d_mod.h e_edf.h
$(O)/d_main.o: d_main.c z_zone.h d_keywds.h psnprntf.h d_io.h doomtype.h \
 doomdef.h m_swap.h version.h d_mod.h doomstat.h doomdata.h d_net.h \
 d_player.h d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h dstrings.h d_englsh.h c_runcmd.h c_io.h v_video.h \
 r_data.h r_state.h p_chase.h v_misc.h c_net.h w_wad.h s_sound.h \
 f_finale.h f_wipe.h m_argv.h m_misc.h mn_engin.h i_sound.h i_video.h \
 g_game.h hu_stuff.h am_map.h p_setup.h r_draw.h r_main.h d_main.h \
 d_deh.h g_bind.h d_dialog.h d_gi.h mn_menus.h mn_htic.h in_lude.h \
 a_small.h amx.h g_gfs.h d_dehtbl.h g_dmflag.h e_edf.h
$(O)/d_net.o: d_net.c c_io.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 c_net.h c_runcmd.h d_main.h d_englsh.h f_wipe.h m_random.h mn_engin.h \
 i_video.h i_net.h r_draw.h g_game.h \
 p_partcl.h d_gi.h mn_menus.h mn_htic.h g_dmflag.h
$(O)/doomdef.o: doomdef.c doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h \
 version.h d_mod.h
$(O)/doomstat.o: doomstat.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h
$(O)/dstrings.o: dstrings.c dstrings.h d_englsh.h
$(O)/e_cmd.o: e_cmd.c z_zone.h info.h d_think.h c_io.h doomstat.h doomdata.h \
 d_net.h d_player.h d_event.h doomtype.h v_video.h doomdef.h r_data.h
$(O)/e_edf.o: e_edf.c z_zone.h d_keywds.h psnprntf.h i_system.h d_ticcmd.h \
 doomtype.h d_main.h d_event.h p_chase.h p_mobj.h tables.h m_fixed.h \
 d_think.h doomdata.h info.h p_skin.h sounds.h st_stuff.h r_defs.h \
 doomdef.h m_swap.h version.h d_mod.h d_player.h d_items.h p_pspr.h \
 d_dehtbl.h d_io.h g_game.h m_argv.h m_cheat.h m_misc.h doomstat.h \
 d_net.h p_enemy.h p_partcl.h f_finale.h e_edf.h Confuse/confuse.h
$(O)/e_exdata.o: e_exdata.c z_zone.h d_keywds.h psnprntf.h doomdef.h \
 m_swap.h version.h d_mod.h d_io.h doomtype.h p_info.h c_io.h \
 doomstat.h doomdata.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h p_skin.h \
 sounds.h st_stuff.h d_event.h r_defs.h v_video.h r_data.h r_state.h \
 p_chase.h v_misc.h w_wad.h d_dehtbl.h Confuse/confuse.h
$(O)/e_sound.o: e_sound.c z_zone.h psnprntf.h d_keywds.h d_io.h i_system.h \
 w_wad.h d_dehtbl.h sounds.h i_sound.h Confuse/confuse.h doomtype.h d_ticcmd.h \
 info.h d_think.h
$(O)/f_finale.o: f_finale.c z_zone.h d_keywds.h psnprntf.h i_video.h \
 doomtype.h doomstat.h doomdata.h d_net.h \
 d_player.h d_items.h doomdef.h m_swap.h version.h d_mod.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 p_skin.h sounds.h st_stuff.h d_event.h r_defs.h v_video.h r_data.h \
 r_state.h p_chase.h v_misc.h w_wad.h s_sound.h dstrings.h d_englsh.h \
 mn_engin.h c_runcmd.h d_deh.h p_info.h c_io.h d_gi.h mn_menus.h \
 mn_htic.h f_finale.h e_edf.h
$(O)/f_wipe.o: f_wipe.c c_io.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 d_main.h i_video.h m_random.h f_wipe.h
$(O)/g_bind.o: g_bind.c doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h \
 version.h d_mod.h doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 c_io.h v_video.h r_data.h r_state.h p_chase.h v_misc.h c_runcmd.h \
 d_main.h d_deh.h g_game.h m_argv.h mn_engin.h mn_misc.h m_misc.h \
 w_wad.h d_io.h
$(O)/g_cmd.o: g_cmd.c doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h \
 version.h d_mod.h doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 d_main.h p_chase.h c_io.h v_video.h r_data.h r_state.h v_misc.h \
 c_runcmd.h c_net.h f_wipe.h g_game.h hu_stuff.h mn_engin.h mn_misc.h \
 m_random.h p_inter.h w_wad.h s_sound.h p_partcl.h d_gi.h mn_menus.h \
 mn_htic.h
$(O)/g_dmflag.o: g_dmflag.c z_zone.h d_keywds.h psnprntf.h g_dmflag.h \
 doomtype.h c_net.h c_runcmd.h c_io.h doomstat.h doomdata.h d_net.h \
 d_player.h d_items.h doomdef.h m_swap.h version.h d_mod.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 p_skin.h sounds.h st_stuff.h d_event.h r_defs.h v_video.h r_data.h \
 r_state.h p_chase.h v_misc.h
$(O)/g_game.o: g_game.c d_io.h doomtype.h d_keywds.h c_io.h doomstat.h \
 doomdata.h d_net.h d_player.h d_items.h doomdef.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 c_net.h c_runcmd.h p_info.h f_finale.h f_wipe.h m_argv.h m_misc.h \
 mn_engin.h mn_menus.h m_random.h p_setup.h p_saveg.h p_tick.h \
 d_main.h hu_stuff.h hu_frags.h am_map.h w_wad.h r_main.h r_draw.h \
 r_things.h p_map.h s_sound.h dstrings.h d_englsh.h r_sky.h d_deh.h \
 p_inter.h g_game.h p_hubs.h g_bind.h d_dialog.h d_gi.h mn_htic.h \
 in_lude.h a_small.h amx.h g_dmflag.h e_edf.h
$(O)/g_gfs.o: g_gfs.c z_zone.h d_keywds.h psnprntf.h i_system.h d_ticcmd.h \
 doomtype.h m_misc.h doomstat.h doomdata.h d_net.h d_player.h \
 d_items.h doomdef.h m_swap.h version.h d_mod.h p_pspr.h m_fixed.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h g_gfs.h d_io.h Confuse/confuse.h
$(O)/hi_stuff.o: hi_stuff.c z_zone.h d_keywds.h psnprntf.h w_wad.h \
 doomstat.h doomdata.h doomtype.h d_net.h d_player.h d_items.h \
 doomdef.h m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h \
 d_ticcmd.h tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h \
 st_stuff.h d_event.h r_defs.h g_game.h in_lude.h v_video.h r_data.h \
 r_state.h p_chase.h v_misc.h mn_htic.h mn_engin.h c_runcmd.h \
 s_sound.h d_gi.h mn_menus.h
$(O)/hu_frags.o: hu_frags.c hu_frags.h c_io.h doomstat.h doomdata.h \
 doomtype.h d_net.h d_player.h d_items.h doomdef.h d_keywds.h z_zone.h \
 psnprntf.h m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h \
 d_ticcmd.h tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h \
 st_stuff.h d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h \
 v_misc.h c_runcmd.h g_game.h r_draw.h w_wad.h g_bind.h
$(O)/hu_over.o: hu_over.c z_zone.h d_keywds.h psnprntf.h doomdef.h m_swap.h \
 version.h d_mod.h doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 c_runcmd.h d_deh.h g_game.h hu_frags.h hu_over.h hu_stuff.h v_video.h \
 r_data.h r_state.h p_chase.h v_misc.h p_info.h c_io.h p_map.h \
 p_setup.h r_draw.h s_sound.h w_wad.h d_gi.h mn_engin.h mn_menus.h \
 mn_htic.h
$(O)/hu_stuff.o: hu_stuff.c doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h c_net.h c_runcmd.h d_deh.h g_game.h hu_frags.h \
 hu_stuff.h v_video.h r_data.h r_state.h p_chase.h v_misc.h hu_over.h \
 p_info.h c_io.h p_map.h p_setup.h p_spec.h r_draw.h s_sound.h w_wad.h \
 am_map.h d_gi.h mn_engin.h mn_menus.h mn_htic.h
$(O)/in_lude.o: in_lude.c doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h \
 version.h d_mod.h doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 m_random.h p_tick.h in_lude.h v_video.h r_data.h r_state.h p_chase.h \
 v_misc.h r_main.h s_sound.h d_gi.h mn_engin.h c_runcmd.h mn_menus.h \
 mn_htic.h p_enemy.h e_edf.h
info.o: info.c z_zone.h d_keywds.h psnprntf.h info.h d_think.h
$(O)/m_argv.o: m_argv.c d_io.h doomtype.h d_keywds.h
$(O)/m_bbox.o: m_bbox.c m_bbox.h z_zone.h d_keywds.h psnprntf.h doomtype.h \
 m_fixed.h i_system.h d_ticcmd.h
$(O)/m_cheat.o: m_cheat.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h c_runcmd.h c_net.h g_game.h r_data.h r_state.h \
 p_chase.h p_inter.h m_cheat.h m_argv.h s_sound.h dstrings.h \
 d_englsh.h d_deh.h d_io.h d_gi.h mn_engin.h mn_menus.h mn_htic.h \
 v_video.h v_misc.h e_edf.h
$(O)/m_fcvt.o: m_fcvt.c m_fcvt.h
$(O)/m_misc.o: m_misc.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 m_argv.h g_game.h mn_engin.h c_runcmd.h am_map.h w_wad.h i_sound.h \
 i_video.h v_video.h r_data.h r_state.h p_chase.h v_misc.h hu_stuff.h \
 dstrings.h d_englsh.h m_misc.h s_sound.h d_main.h r_sky.h r_draw.h \
 c_io.h c_net.h d_io.h p_partcl.h r_things.h d_gi.h mn_menus.h \
 mn_htic.h
$(O)/m_qstr.o: m_qstr.c i_system.h d_ticcmd.h doomtype.h m_qstr.h
$(O)/m_queue.o: m_queue.c z_zone.h psnprntf.h d_keywds.h m_queue.h
$(O)/m_random.o: m_random.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h m_random.h a_small.h amx.h
$(O)/mn_engin.o: mn_engin.c doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h c_io.h v_video.h r_data.h r_state.h p_chase.h \
 v_misc.h c_runcmd.h d_main.h g_game.h hu_over.h i_video.h \
 mn_engin.h mn_menus.h mn_misc.h r_draw.h \
 s_sound.h w_wad.h g_bind.h d_gi.h mn_htic.h
$(O)/mn_htic.o: mn_htic.c z_zone.h d_keywds.h psnprntf.h d_deh.h doomtype.h \
 doomdef.h m_swap.h version.h d_mod.h doomstat.h doomdata.h d_net.h \
 d_player.h d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h c_io.h v_video.h r_data.h r_state.h p_chase.h \
 v_misc.h d_gi.h mn_engin.h c_runcmd.h mn_menus.h mn_htic.h mn_misc.h \
 w_wad.h
$(O)/mn_menus.o: mn_menus.c d_io.h doomtype.h d_keywds.h doomdef.h z_zone.h \
 psnprntf.h m_swap.h version.h d_mod.h doomstat.h doomdata.h d_net.h \
 d_player.h d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h c_io.h v_video.h r_data.h r_state.h p_chase.h \
 v_misc.h c_runcmd.h d_deh.h d_main.h dstrings.h d_englsh.h dhticstr.h \
 g_game.h hu_over.h i_video.h m_random.h \
 mn_engin.h mn_misc.h r_draw.h s_sound.h w_wad.h d_dialog.h mn_htic.h \
 d_gi.h mn_menus.h g_dmflag.h e_edf.h
$(O)/mn_misc.o: mn_misc.c z_zone.h d_keywds.h psnprntf.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h doomdef.h m_swap.h \
 version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h \
 info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h \
 r_defs.h d_main.h p_chase.h s_sound.h v_video.h r_data.h r_state.h \
 v_misc.h w_wad.h mn_engin.h c_runcmd.h mn_misc.h d_gi.h mn_menus.h \
 mn_htic.h
$(O)/mn_skinv.o: mn_skinv.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h mn_engin.h c_runcmd.h r_draw.h v_video.h r_data.h \
 r_state.h p_chase.h v_misc.h w_wad.h s_sound.h m_random.h d_gi.h \
 mn_menus.h mn_htic.h e_edf.h
$(O)/p_anim.o: p_anim.c z_zone.h d_keywds.h psnprntf.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h doomdef.h m_swap.h \
 version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h \
 info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h \
 r_defs.h r_data.h r_state.h p_chase.h r_sky.h m_random.h p_setup.h \
 s_sound.h p_info.h c_io.h v_video.h v_misc.h a_small.h amx.h
$(O)/p_ceilng.o: p_ceilng.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h r_main.h r_data.h r_state.h p_chase.h p_info.h \
 c_io.h v_video.h v_misc.h p_spec.h p_tick.h s_sound.h
$(O)/p_chase.o: p_chase.c c_io.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 c_runcmd.h d_main.h p_map.h p_maputl.h r_main.h g_game.h a_small.h \
 amx.h
$(O)/p_cmd.o: p_cmd.c c_io.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 c_net.h c_runcmd.h d_main.h f_wipe.h g_game.h m_random.h p_info.h \
 p_inter.h p_spec.h r_draw.h mn_engin.h
$(O)/p_doors.o: p_doors.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h g_game.h p_info.h c_io.h v_video.h r_data.h \
 r_state.h p_chase.h v_misc.h p_spec.h p_tick.h s_sound.h r_main.h \
 dstrings.h d_englsh.h d_deh.h hu_stuff.h d_gi.h mn_engin.h c_runcmd.h \
 mn_menus.h mn_htic.h
$(O)/p_enemy.o: p_enemy.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h m_random.h r_main.h r_data.h r_state.h p_chase.h \
 p_maputl.h p_map.h p_setup.h p_spec.h s_sound.h p_inter.h g_game.h \
 p_enemy.h p_tick.h m_bbox.h p_anim.h c_io.h v_video.h v_misc.h \
 c_runcmd.h w_wad.h p_partcl.h p_info.h a_small.h amx.h e_edf.h
$(O)/p_floor.o: p_floor.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h c_io.h v_video.h r_data.h r_state.h p_chase.h \
 v_misc.h r_main.h p_info.h p_map.h p_spec.h p_tick.h s_sound.h
$(O)/p_genlin.o: p_genlin.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h r_main.h r_data.h r_state.h p_chase.h p_info.h \
 c_io.h v_video.h v_misc.h p_spec.h p_tick.h m_random.h s_sound.h
$(O)/p_henemy.o: p_henemy.c z_zone.h d_keywds.h psnprntf.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h doomdef.h m_swap.h \
 version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h \
 info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h \
 r_defs.h m_random.h s_sound.h p_inter.h p_tick.h r_main.h r_data.h \
 r_state.h p_chase.h p_maputl.h p_enemy.h p_map.h p_spec.h e_edf.h
$(O)/p_hubs.o: p_hubs.c c_io.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 g_game.h p_maputl.h p_setup.h p_spec.h d_io.h
$(O)/p_info.o: p_info.c z_zone.h d_keywds.h psnprntf.h d_io.h doomtype.h \
 doomstat.h doomdata.h d_net.h d_player.h d_items.h doomdef.h m_swap.h \
 version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h \
 info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h \
 r_defs.h c_io.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 c_runcmd.h w_wad.h p_setup.h p_info.h d_gi.h mn_engin.h mn_menus.h \
 mn_htic.h
$(O)/p_inter.o: p_inter.c c_io.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 dstrings.h d_englsh.h m_random.h g_game.h hu_stuff.h hu_frags.h \
 am_map.h r_main.h r_segs.h s_sound.h p_tick.h d_deh.h p_inter.h \
 d_gi.h mn_engin.h c_runcmd.h mn_menus.h mn_htic.h g_dmflag.h e_edf.h
$(O)/p_lights.o: p_lights.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h m_random.h r_main.h r_data.h r_state.h p_chase.h \
 p_spec.h p_tick.h
$(O)/p_map.o: p_map.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 r_main.h r_data.h r_state.h p_chase.h p_maputl.h p_map.h p_setup.h \
 p_spec.h s_sound.h p_inter.h m_random.h r_segs.h m_bbox.h p_partcl.h \
 p_tick.h p_user.h d_gi.h mn_engin.h c_runcmd.h mn_menus.h mn_htic.h \
 v_video.h v_misc.h e_edf.h
$(O)/p_maputl.o: p_maputl.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h m_bbox.h r_main.h r_data.h r_state.h p_chase.h \
 p_maputl.h p_map.h p_setup.h
$(O)/p_mobj.o: p_mobj.c doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h \
 version.h d_mod.h doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 m_random.h r_main.h r_data.h r_state.h p_chase.h p_maputl.h p_map.h \
 p_tick.h hu_stuff.h v_video.h v_misc.h s_sound.h g_game.h p_inter.h \
 p_spec.h p_partcl.h in_lude.h d_gi.h mn_engin.h c_runcmd.h mn_menus.h \
 mn_htic.h p_user.h g_dmflag.h e_edf.h
$(O)/p_partcl.o: p_partcl.c z_zone.h d_keywds.h psnprntf.h d_main.h \
 d_event.h doomtype.h p_chase.h p_mobj.h tables.h m_fixed.h i_system.h \
 d_ticcmd.h d_think.h doomdata.h info.h p_skin.h sounds.h st_stuff.h \
 r_defs.h doomdef.h m_swap.h version.h d_mod.h d_player.h d_items.h \
 p_pspr.h doomstat.h d_net.h m_random.h p_partcl.h p_setup.h r_main.h \
 r_data.h r_state.h r_things.h v_video.h v_misc.h w_wad.h p_spec.h \
 c_runcmd.h
$(O)/p_plats.o: p_plats.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h m_random.h r_main.h r_data.h r_state.h p_chase.h \
 p_info.h c_io.h v_video.h v_misc.h p_spec.h p_tick.h s_sound.h
$(O)/p_pspr.o: p_pspr.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 g_game.h m_random.h p_enemy.h p_inter.h p_map.h p_maputl.h p_tick.h \
 r_main.h r_data.h r_state.h p_chase.h r_segs.h r_things.h s_sound.h \
 e_edf.h d_dehtbl.h
p_saveg.o: p_saveg.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h r_main.h r_data.h r_state.h p_chase.h p_maputl.h \
 p_spec.h p_tick.h p_saveg.h m_random.h am_map.h p_enemy.h p_hubs.h \
 p_setup.h e_edf.h
$(O)/p_setup.o: p_setup.c c_io.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 c_runcmd.h d_main.h hu_stuff.h hu_frags.h m_bbox.h m_argv.h g_game.h \
 w_wad.h p_hubs.h r_main.h r_things.h r_sky.h p_maputl.h p_map.h \
 p_setup.h p_spec.h p_tick.h p_enemy.h p_info.h s_sound.h p_anim.h \
 p_partcl.h d_dialog.h d_io.h in_lude.h a_small.h amx.h
$(O)/p_sight.o: p_sight.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h r_main.h r_data.h r_state.h p_chase.h p_maputl.h \
 p_setup.h m_bbox.h
$(O)/p_skin.o: p_skin.c z_zone.h d_keywds.h psnprntf.h c_runcmd.h c_io.h \
 doomstat.h doomdata.h doomtype.h d_net.h d_player.h d_items.h \
 doomdef.h m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h \
 d_ticcmd.h tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h \
 st_stuff.h d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h \
 v_misc.h c_net.h d_main.h p_info.h r_things.h s_sound.h w_wad.h \
 d_io.h e_edf.h
$(O)/p_spec.o: p_spec.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 p_spec.h p_tick.h p_setup.h m_random.h d_englsh.h m_argv.h w_wad.h \
 r_main.h r_data.h r_state.h p_chase.h p_maputl.h p_map.h g_game.h \
 p_inter.h s_sound.h m_bbox.h d_deh.h r_plane.h p_info.h c_io.h \
 v_video.h v_misc.h c_runcmd.h hu_stuff.h r_ripple.h d_gi.h mn_engin.h \
 mn_menus.h mn_htic.h p_user.h e_edf.h
$(O)/p_switch.o: p_switch.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h g_game.h p_info.h c_io.h v_video.h r_data.h \
 r_state.h p_chase.h v_misc.h p_spec.h r_main.h s_sound.h w_wad.h
$(O)/p_telept.o: p_telept.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h p_chase.h p_maputl.h p_map.h p_spec.h r_main.h \
 r_data.h r_state.h p_tick.h s_sound.h p_user.h d_gi.h mn_engin.h \
 c_runcmd.h mn_menus.h mn_htic.h v_video.h v_misc.h e_edf.h
$(O)/p_tick.o: p_tick.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 d_main.h p_chase.h p_user.h p_spec.h p_tick.h p_anim.h p_partcl.h
$(O)/p_user.o: p_user.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 c_net.h c_runcmd.h g_game.h r_main.h r_data.h r_state.h p_chase.h \
 p_map.h p_spec.h p_user.h s_sound.h a_small.h amx.h e_edf.h
$(O)/psnprntf.o: psnprntf.c psnprntf.h
$(O)/r_bsp.o: r_bsp.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 m_bbox.h r_main.h r_data.h r_state.h p_chase.h r_segs.h r_plane.h \
 r_things.h
$(O)/r_data.o: r_data.c z_zone.h d_keywds.h psnprntf.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h doomdef.h m_swap.h \
 version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h \
 info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h \
 r_defs.h c_io.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 d_main.h w_wad.h p_setup.h r_main.h r_sky.h p_info.h d_io.h
$(O)/r_draw.o: r_draw.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 w_wad.h r_draw.h r_main.h r_data.h r_state.h p_chase.h v_video.h \
 v_misc.h mn_engin.h c_runcmd.h d_gi.h mn_menus.h mn_htic.h
$(O)/r_main.o: r_main.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 i_video.h c_runcmd.h g_game.h mn_engin.h \
 r_main.h r_data.h r_state.h p_chase.h r_things.h r_plane.h r_ripple.h \
 r_bsp.h r_draw.h m_bbox.h r_sky.h s_sound.h v_video.h v_misc.h \
 w_wad.h d_deh.h
$(O)/r_plane.o: r_plane.c z_zone.h d_keywds.h psnprntf.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h doomdef.h m_swap.h \
 version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h \
 info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h \
 r_defs.h c_io.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 w_wad.h r_main.h r_draw.h r_things.h r_sky.h r_ripple.h r_plane.h \
 p_info.h p_anim.h p_user.h
$(O)/r_portal.o: r_portal.c c_io.h doomstat.h doomdata.h doomtype.h \
 d_event.h v_video.h doomdef.h m_swap.h version.h d_mod.h d_keywds.h \
 r_data.h r_defs.h m_fixed.h d_think.h p_mobj.h tables.h info.h p_skin.h \
 sounds.h st_stuff.h r_portal.h p_partcl.h r_state.h d_player.h d_items.h \
 p_pspr.h p_chase.h v_misc.h r_draw.h r_main.h r_plane.h r_bsp.h r_things.h
$(O)/r_ripple.o: r_ripple.c doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h r_data.h r_state.h p_chase.h w_wad.h v_video.h \
 v_misc.h
$(O)/r_segs.o: r_segs.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 r_main.h r_data.h r_state.h p_chase.h r_bsp.h r_plane.h r_things.h \
 r_draw.h w_wad.h p_user.h
$(O)/r_sky.o: r_sky.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 r_sky.h r_data.h r_state.h p_chase.h p_info.h c_io.h v_video.h \
 v_misc.h w_wad.h d_gi.h mn_engin.h c_runcmd.h mn_menus.h mn_htic.h
$(O)/r_things.o: r_things.c c_io.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 w_wad.h g_game.h d_main.h r_main.h r_bsp.h r_segs.h r_draw.h \
 r_things.h m_argv.h p_user.h e_edf.h
$(O)/s_sound.o: s_sound.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h d_main.h p_chase.h s_sound.h i_sound.h r_main.h \
 r_data.h r_state.h m_random.h w_wad.h c_io.h v_video.h v_misc.h \
 c_runcmd.h p_info.h d_io.h d_gi.h mn_engin.h mn_menus.h mn_htic.h \
 a_small.h amx.h
$(O)/sounds.o: sounds.c doomtype.h sounds.h p_skin.h info.h d_think.h \
 st_stuff.h d_event.h r_defs.h doomdef.h d_keywds.h z_zone.h \
 psnprntf.h m_swap.h version.h d_mod.h m_fixed.h i_system.h d_ticcmd.h \
 p_mobj.h tables.h doomdata.h d_player.h d_items.h p_pspr.h
$(O)/st_hbar.o: st_hbar.c doomstat.h m_random.h p_mobj.h v_video.h w_wad.h \
 st_stuff.h doomdata.h d_net.h d_player.h doomtype.h tables.h m_fixed.h \
 d_think.h info.h p_skin.h d_event.h doomdef.h r_data.h r_defs.h
$(O)/st_lib.o: st_lib.c doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h \
 version.h d_mod.h doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 v_video.h r_data.h r_state.h p_chase.h v_misc.h w_wad.h st_lib.h \
 r_main.h
$(O)/st_stuff.o: st_stuff.c doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h c_runcmd.h d_main.h p_chase.h m_random.h i_video.h \
 w_wad.h st_lib.h v_video.h r_data.h \
 r_state.h v_misc.h r_main.h am_map.h m_cheat.h s_sound.h dstrings.h \
 d_englsh.h
$(O)/tables.o: tables.c tables.h m_fixed.h d_keywds.h i_system.h d_ticcmd.h \
 doomtype.h
$(O)/v_block.o: v_block.c z_zone.h d_keywds.h psnprntf.h v_video.h \
 doomtype.h doomdef.h m_swap.h version.h d_mod.h m_fixed.h i_system.h \
 d_ticcmd.h v_patch.h r_data.h r_defs.h d_think.h p_mobj.h tables.h \
 doomdata.h info.h p_skin.h sounds.h st_stuff.h d_event.h d_player.h \
 d_items.h p_pspr.h p_partcl.h r_state.h p_chase.h v_misc.h
$(O)/v_misc.o: v_misc.c c_io.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 c_runcmd.h i_video.h w_wad.h d_gi.h \
 mn_engin.h mn_menus.h mn_htic.h
$(O)/v_patch.o: v_patch.c v_video.h doomtype.h doomdef.h d_keywds.h \
 z_zone.h psnprntf.h m_swap.h version.h d_mod.h m_fixed.h i_system.h \
 d_ticcmd.h v_patch.h r_data.h r_defs.h d_think.h p_mobj.h tables.h \
 doomdata.h info.h p_skin.h sounds.h st_stuff.h d_event.h d_player.h \
 d_items.h p_pspr.h p_partcl.h r_state.h p_chase.h v_misc.h
$(O)/v_video.o: v_video.c c_io.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 r_main.h m_bbox.h r_draw.h w_wad.h i_video.h
$(O)/w_wad.o: w_wad.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 d_io.h c_io.h v_video.h r_data.h r_state.h p_chase.h v_misc.h w_wad.h
$(O)/wi_stuff.o: wi_stuff.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h d_keywds.h z_zone.h psnprntf.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h m_random.h w_wad.h g_game.h r_main.h r_data.h \
 r_state.h p_chase.h p_info.h c_io.h v_video.h v_misc.h wi_stuff.h \
 s_sound.h d_gi.h mn_engin.h c_runcmd.h mn_menus.h mn_htic.h \
 hu_stuff.h am_map.h p_tick.h in_lude.h
$(O)/z_zone.o: z_zone.c z_zone.h d_keywds.h psnprntf.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h doomdef.h m_swap.h \
 version.h d_mod.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h \
 info.h d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h d_event.h \
 r_defs.h m_argv.h
$(O)/confuse.o: Confuse/confuse.c Confuse/confuse.h z_zone.h d_keywds.h \
 psnprntf.h d_io.h doomtype.h i_system.h d_ticcmd.h w_wad.h
$(O)/lexer.o: Confuse/lexer.c Confuse/confuse.h z_zone.h d_keywds.h \
 psnprntf.h d_io.h doomtype.h w_wad.h

$(O)/i_main.o: $(PLATFORM)/i_main.c doomdef.h d_keywds.h z_zone.h \
 psnprntf.h m_swap.h version.h d_mod.h m_argv.h \
 d_main.h d_event.h doomtype.h p_chase.h p_mobj.h \
 tables.h m_fixed.h i_system.h d_ticcmd.h d_think.h \
 doomdata.h info.h p_skin.h sounds.h st_stuff.h \
 r_defs.h d_player.h d_items.h p_pspr.h $(PLATFORM)/allegro.h
$(O)/i_net.o: $(PLATFORM)/i_net.c z_zone.h d_keywds.h psnprntf.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h \
 doomdef.h m_swap.h version.h d_mod.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h \
 d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h \
 d_event.h r_defs.h m_argv.h i_net.h
$(O)/i_sound.o: $(PLATFORM)/i_sound.c $(PLATFORM)/allegro.h \
  z_zone.h d_keywds.h psnprntf.h c_runcmd.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h doomdef.h m_swap.h \
 version.h d_mod.h p_pspr.h m_fixed.h i_system.h \
 d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 p_skin.h sounds.h st_stuff.h d_event.h r_defs.h \
 i_sound.h w_wad.h g_game.h d_main.h p_chase.h \
 v_misc.h d_gi.h mn_engin.h mn_menus.h mn_htic.h \
 v_video.h r_data.h r_state.h
$(O)/i_system.o: $(PLATFORM)/i_system.c $(PLATFORM)/allegro.h \
 z_zone.h d_keywds.h psnprntf.h $(PLATFORM)/ser_main.h c_runcmd.h i_system.h \
 d_ticcmd.h doomtype.h i_sound.h sounds.h i_video.h doomstat.h \
 doomdata.h d_net.h d_player.h d_items.h doomdef.h \
 m_swap.h version.h d_mod.h p_pspr.h m_fixed.h \
 tables.h info.h d_think.h p_mobj.h p_skin.h \
 st_stuff.h d_event.h r_defs.h m_misc.h g_game.h \
 w_wad.h v_video.h r_data.h r_state.h p_chase.h \
 v_misc.h m_argv.h g_bind.h
$(O)/i_video.o: $(PLATFORM)/i_video.c z_zone.h d_keywds.h \
 psnprntf.h $(PLATFORM)/allegro.h \
 doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h m_swap.h version.h d_mod.h \
 p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h \
 info.h d_think.h p_mobj.h p_skin.h sounds.h \
 st_stuff.h d_event.h r_defs.h am_map.h c_io.h \
 v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 c_runcmd.h d_main.h i_video.h m_argv.h m_bbox.h \
 mn_engin.h r_draw.h w_wad.h d_gi.h mn_menus.h \
 mn_htic.h
$(O)/ser_main.o: $(PLATFORM)/ser_main.c doomdef.h d_keywds.h z_zone.h \
 psnprntf.h m_swap.h version.h d_mod.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h \
 p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h \
 info.h d_think.h p_mobj.h p_skin.h sounds.h \
 st_stuff.h d_event.h r_defs.h $(PLATFORM)/ser_main.h c_io.h \
 v_video.h r_data.h r_state.h p_chase.h v_misc.h \
 c_runcmd.h c_net.h d_main.h g_game.h m_random.h \
 i_video.h mn_engin.h
$(O)/ser_port.o: $(PLATFORM)/ser_port.c $(PLATFORM)/ser_main.h \
 d_main.h d_event.h doomtype.h p_chase.h p_mobj.h tables.h m_fixed.h \
 d_keywds.h i_system.h d_ticcmd.h d_think.h doomdata.h \
 info.h p_skin.h sounds.h st_stuff.h r_defs.h \
 doomdef.h z_zone.h psnprntf.h m_swap.h version.h \
 d_mod.h d_player.h d_items.h p_pspr.h d_net.h \
 c_io.h doomstat.h v_video.h r_data.h r_state.h \
 v_misc.h
$(O)/mmus2mid.o: $(PLATFORM)/mmus2mid.c $(PLATFORM)/allegro.h \
 $(PLATFORM)/mmus2mid.h z_zone.h d_keywds.h psnprntf.h
$(O)/pproblit.o: $(PLATFORM)/pproblit.s
$(O)/drawspan.o: $(PLATFORM)/drawspan.s

# haleyjd: this is currently out of sync with the C version, needs fixing
# $(O)/drawcol.o: $(PLATFORM)/drawcol.s

# Allegro patches required to function satisfactorily

$(O)/emu8kmid.o: $(PLATFORM)/emu8kmid.c djgpp/emu8k.h djgpp/internal.h \
  djgpp/interndj.h djgpp/allegro.h

$(O)/keyboard.o: $(PLATFORM)/keyboard.c djgpp/internal.h  \
  djgpp/interndj.h djgpp/allegro.h
	$(CC) $(CFLAGS_COMMON) -O $(CFLAGS_NEWFEATURES) -c $< -o $@

endif

# bin2c utility

bin2c: $(O)/bin2c.exe
	$(CP) $(O)\bin2c.exe .

$(O)/bin2c.exe: $(O)/bin2c.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(O)/bin2c.o -o $@ $(LIBS)

$(O)/bin2c.o: bin2c.c
