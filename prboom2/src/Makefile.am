#
# automake Makefile.am for the LxDoom source directory
#
# $Id: Makefile.am,v 1.1 2000/05/04 07:57:55 proff_fs Exp $
#
# Process this file with automake to produce Makefile.in
#
#
gamesdir=$(prefix)/games
EXTRA_PROGRAMS = lsdoom lxdoom sndserv
games_PROGRAMS = @BUILD_LSDOOM@ @BUILD_LXDOOM@ @BUILD_SNDSERV@ lxdoom-game-server

if I386_ASM
ASMS = drawspan.s drawcol.s
else
ASMS = 
endif

sndserv_SOURCES = version.c l_soundsrv.c sounds.c l_soundgen.c \
	l_system.c i_system.h l_soundsrv.h l_soundgen.h
sndserv_LDADD = @LIB_SND@

lxdoom_game_server_SOURCES = d_server.c l_udp.c protocol.h l_system.c
lxdoom_game_server_LDADD = 

COMMON_SRC = \
 am_map.c       g_game.c           p_maputl.h       r_plane.h   \
 am_map.h       g_game.h           p_mobj.c         r_segs.c    \
 hu_lib.c       lprintf.c          p_mobj.h         r_segs.h    \
 d_client.c     hu_lib.h           lprintf.h        p_plats.c   r_sky.c	    \
 d_deh.c        hu_stuff.c         m_argv.c         p_pspr.c    r_sky.h	    \
 d_deh.h        hu_stuff.h         m_argv.h         p_pspr.h    r_state.h   \
 d_englsh.h     i_joy.h            m_bbox.c         p_saveg.c   r_things.c  \
 d_event.h      i_net.h            m_bbox.h         p_saveg.h   r_things.h  \
 d_items.c      i_network.h        m_cheat.c        p_setup.c   s_sound.c   \
 d_items.h      i_sound.h          m_cheat.h        p_setup.h   s_sound.h   \
 d_main.c       i_system.h         m_fixed.h        p_sight.c   sounds.c    \
 d_main.h       i_video.h          m_menu.c         p_spec.c    sounds.h    \
 info.c         m_menu.h           p_spec.h         st_lib.c    \
 d_net.h        info.h             m_misc.c         p_switch.c  st_lib.h    \
 d_player.h     l_joy.c            m_misc.h         p_telept.c  st_stuff.c  \
 m_random.c     p_tick.c           st_stuff.h       l_main.c    i_main.h    \
 d_think.h      m_random.h         p_tick.h         tables.c    \
 d_ticcmd.h     m_swap.h           p_user.c         tables.h    l_system.c  \
 doomdata.h     l_sound.c          p_ceilng.c       p_user.h    v_video.c   \
 doomdef.c      p_doors.c          protocol.h       v_video.h   \
 doomdef.h      p_enemy.c          r_bsp.c          version.c   \
 doomstat.c     p_enemy.h          r_bsp.h          version.h   \
 doomstat.h     p_floor.c          r_data.c         w_wad.c	\
 doomtype.h     p_genlin.c         r_data.h         w_wad.h	\
 dstrings.c     l_udp.c            p_inter.c        r_defs.h    wi_stuff.c  \
 dstrings.h     p_inter.h          r_draw.c         wi_stuff.h  \
 f_finale.c     p_lights.c         r_draw.h         z_bmalloc.c \
 f_finale.h     p_map.c            r_main.c         z_bmalloc.h \
 f_wipe.c       p_map.h            r_main.h         z_zone.c    \
 f_wipe.h       p_maputl.c         r_plane.c        z_zone.h    $(ASMS)	\
 l_network.c

lxdoom_SOURCES = l_video_trans.h   l_video_trans.c  l_video_x.c $(COMMON_SRC)
lsdoom_SOURCES = l_video_svgalib.c $(COMMON_SRC)

lxdoom_LDADD = @X_LIBS@ @X_PRE_LIBS@ -lX11 @X_EXTRA_LIBS@ @LIB_XEXT@ @LIB_XDGA@
lsdoom_LDADD = -lvga

EXTRA_lxdoom_SOURCES  = $(ASMS)

#
# $Log: Makefile.am,v $
# Revision 1.1  2000/05/04 07:57:55  proff_fs
# Initial revision
#
# Revision 1.10  2000/04/05 10:47:31  cph
# Remove dead #ifdef magic, rely on config.h now
# Make sndserv work on (Open|Net)BSD, using libossaudio
# Make --enable-debug compile with -g
# Make asm stuff only compile on Linux and FreeBSD
# (draw(col|span).s failed on OpenBSD, linker troubles)
#
# Revision 1.9  2000/04/03 17:06:10  cph
# Split client specific stuff from l_udp.c to new l_network.c
# Move server specific stuff from l_udp.c to d_server.c
# Update copyright notices
# Restructure ready for IPv6 support
# Use fcntl instead of ioctl to set socket non-blocking
#
# Revision 1.8  2000/03/29 10:32:57  cph
# Merge v1.4 fixes
#
# Revision 1.7.2.1  2000/03/28 17:13:43  cph
# Remove old makefile
#
# Revision 1.7  2000/03/19 20:14:32  cph
# Sound code cleaning: DosDoom, IPC and such unused code removed
#
# Revision 1.6  1999/12/13 17:24:08  cphipps
# Add i_main.h to lxdoom/lsdoom
#
# Revision 1.5  1999/10/31 16:32:21  cphipps
# Added l_system.c to lxdoom-game-server
#
# Revision 1.4  1999/10/27 07:41:53  cphipps
# Remove overriding of CFLAGS, so configure can set them according to
# architecture
#
# Revision 1.3  1999/10/02 15:00:48  cphipps
# Added X_CFLAGS to the compile flags, so autoconf can pass the path to
# the X11 include files for systems that don't put them in the main include
# tree (fixes compile problem on FreeBSD)
#
# Revision 1.2  1999/09/16 22:19:22  cphipps
# Add missing header files for sound and netgame servers
#
# Revision 1.1  1999/09/10 20:10:47  cphipps
# Initial revision
#
#
