# $Id: Makefile.am,v 1.3 2000/05/21 03:19:21 jessh Exp $
#
# Copyright (C) 2000 by Colin Phipps (cph@lxdoom.linuxgames.com)
# License: GPL

CFLAGS=@CFLAGS@

gamesdir=$(prefix)/games
EXTRA_PROGRAMS = sndserv
games_PROGRAMS = @BUILD_SNDSERV@ 
noinst_LIBRARIES = libossdoom.a

sndserv_SOURCES = i_soundsrv.c sounds.c i_soundgen.c \
	i_soundsrv.h i_soundgen.h
sndserv_LDADD = @LIB_SND@ ../POSIX/libposixdoom.a

libossdoom_a_SOURCES = i_sound.c


