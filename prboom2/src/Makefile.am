#
# automake Makefile.am for the LxDoom source directory
#
#
# Process this file with automake to produce Makefile.in
#
#

SUBDIRS = SDL POSIX

gamesdir=$(prefix)/games
EXTRA_PROGRAMS = prboom
games_PROGRAMS = @BUILD_DOOM@ prboom-game-server

CFLAGS = @CFLAGS@ @SDL_CFLAGS@

UDP_SRC = i_udp_sdl.c

prboom_game_server_SOURCES = d_server.c i_udp_sdl.c protocol.h
prboom_game_server_LDADD = POSIX/libposixdoom.a @NET_LIBS@ @SDL_LIBS@

COMMON_SRC = \
 am_map.c        am_map.h        c_cmd.c         c_io.c \
 c_io.h          c_net.c         c_net.h         c_runcmd.c \
 c_runcmd.h      d_deh.c         d_deh.h \
 d_englsh.h      d_event.h       d_items.c       d_items.h \
 d_main.c        d_main.h        d_net.h         d_player.h \
 d_think.h       d_ticcmd.h      doomdata.h      doomdef.c \
 doomdef.h       doomstat.c      doomstat.h      doomtype.h \
 dstrings.c      dstrings.h      f_finale.c      f_finale.h \
 f_wipe.c        f_wipe.h        g_bind.c        g_bind.h \
 g_bindaxes.c    g_bindaxes.h    g_cmd.c         g_config.c \
 g_config.h      g_demo.c        g_game.c \
 g_game.h        hu_frags.c      hu_frags.h      hu_over.c \
 hu_over.h       hu_stuff.c      hu_stuff.h      i_axes.h \
 i_main.c        i_main.h        i_net.h         i_network.h \
 i_sound.h       i_system.h      i_video.h       info.c \
 info.h          lprintf.c       lprintf.h       m_argv.c \
 m_argv.h        m_bbox.c        m_bbox.h        m_cheat.c \
 m_cheat.h       m_fixed.h       m_misc.c        m_misc.h \
 m_random.c      m_random.h      m_swap.h        mn_engin.c \
 mn_engin.h      mn_menus.c      mn_menus.h      mn_misc.c \
 mn_misc.h       p_ceilng.c      p_chase.c       p_chase.h \
 p_cmd.c         p_doors.c       p_enemy.c       p_enemy.h \
 p_floor.c       p_genlin.c      p_hubs.c        p_hubs.h \
 p_info.c        p_info.h        p_inter.c       p_inter.h \
 p_lights.c      p_map.c         p_map.h         p_maputl.c \
 p_maputl.h      p_mobj.c        p_mobj.h        p_plats.c \
 p_pspr.c        p_pspr.h        p_saveg.c       p_saveg.h \
 p_setup.c       p_setup.h       p_sight.c       p_spec.c \
 p_spec.h        p_switch.c      p_telept.c      p_tick.c \
 p_tick.h        p_user.c        p_user.h        protocol.h \
 r_bsp.c         r_bsp.h         r_data.c        r_data.h \
 r_defs.h        r_draw.c        r_draw.h        r_filter.c \
 r_filter.h      r_main.c        r_main.h        r_patch.c \
 r_patch.h       r_plane.c       r_plane.h       r_segs.c \
 r_segs.h        r_sky.c         r_sky.h         r_state.h \
 r_things.c      r_things.h      s_sound.c       s_sound.h \
 sounds.c        sounds.h        st_lib.c        st_lib.h \
 st_stuff.c      st_stuff.h      t_func.c        t_func.h \
 t_oper.c        t_oper.h        t_parse.c       t_parse.h \
 t_prepro.c      t_prepro.h      t_script.c      t_script.h \
 t_spec.c        t_spec.h        t_vari.c        t_vari.h \
 tables.c        tables.h        v_misc.c        v_misc.h \
 v_video.c       v_video.h       version.c       version.h \
 w_wad.c         w_wad.h         wi_stuff.c      wi_stuff.h \
 z_bmalloc.c     z_bmalloc.h     z_zone.c        z_zone.h \
 gl_dyn.c        gl_dyn.h        gl_funcs.h      gl_intern.h \
 gl_main.c       gl_preprocess.c gl_struct.h     gl_texture.c

NET_CLIENT_SRC = d_client.c i_udp_sdl.c

if WAD_MMAP
WAD_SRC = w_mmap.c
else
WAD_SRC = w_memcache.c
endif

prboom_SOURCES = mmus2mid.c mmus2mid.h $(COMMON_SRC) $(NET_CLIENT_SRC) $(WAD_SRC)
prboom_LDADD = SDL/libsdldoom.a @MIXER_LIBS@ @NET_LIBS@ @SDL_LIBS@ @DLL_LIBS@ @MATH_LIB@

# Must remember to include 'optional' sources in the tarball
EXTRA_prboom_SOURCES =

