/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: m_swap.h,v 1.1 2000/05/04 08:10:35 proff_fs Exp $
 *
 *  LxDoom, a Doom port for Linux/Unix
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *   and Colin Phipps
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *      Endianess handling, swapping 16bit and 32bit.
 *
 *-----------------------------------------------------------------------------*/


#ifndef __M_SWAP__
#define __M_SWAP__

#ifdef __GNUG__
#pragma interface
#endif

/* Endianess handling. */

/* cph - First the macros to do the actual byte swapping */

/* leban
 * rather than continue the confusing tradition of redefining the
 * stardard macro, we now present the doom_ntoh and doom_hton macros....
 * might as well use the xdoom macros.
 */

#define doom_swap_l(x) \
        ((long int)((((unsigned long int)(x) & 0x000000ffU) << 24) | \
                             (((unsigned long int)(x) & 0x0000ff00U) <<  8) | \
                             (((unsigned long int)(x) & 0x00ff0000U) >>  8) | \
                             (((unsigned long int)(x) & 0xff000000U) >> 24)))

#define doom_swap_s(x) \
        ((short int)((((unsigned short int)(x) & 0x00ff) << 8) | \
                              (((unsigned short int)(x) & 0xff00) >> 8))) 

/* CPhipps - now the endianness handling, converting input or output to/from 
 * the machine's endianness to that wanted for this type of I/O
 *
 * To find our own endianness, use config.h
 */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

/* Macros are named doom_XtoYT, where 
 * X is thing to convert from, Y is thing to convert to, chosen from 
 * n for network, h for host (i.e our machine's), w for WAD (Doom data files)
 * and T is the type, l or s for long or short
 *
 * CPhipps - all WADs and network packets will be little endian for now
 * Use separate macros so network could be converted to big-endian later.
 */

#ifdef WORDS_BIGENDIAN

#define doom_wtohl(x) doom_swap_l(x)
#define doom_htowl(x) doom_swap_l(x)
#define doom_wtohs(x) doom_swap_s(x)
#define doom_htows(x) doom_swap_s(x)

#define doom_ntohl(x) doom_swap_l(x)
#define doom_htonl(x) doom_swap_l(x)
#define doom_ntohs(x) doom_swap_s(x)
#define doom_htons(x) doom_swap_s(x)

#else

#define doom_wtohl(x) (long int)(x)
#define doom_htowl(x) (long int)(x)
#define doom_wtohs(x) (short int)(x)
#define doom_htows(x) (short int)(x)

#define doom_ntohl(x) (long int)(x)
#define doom_htonl(x) (long int)(x)
#define doom_ntohs(x) (short int)(x)
#define doom_htons(x) (short int)(x)

#endif

/* CPhipps - Boom's old LONG and SHORT endianness macros are for WAD stuff */

#define LONG(x) doom_wtohl(x)
#define SHORT(x) doom_htows(x)

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: m_swap.h,v $
 * Revision 1.1  2000/05/04 08:10:35  proff_fs
 * Initial revision
 *
 * Revision 1.9  2000/05/01 17:50:36  Proff
 * made changes to compile with VisualC and SDL
 *
 * Revision 1.8  2000/04/05 10:47:31  cph
 * Remove dead #ifdef magic, rely on config.h now
 * Make sndserv work on (Open|Net)BSD, using libossaudio
 * Make --enable-debug compile with -g
 * Make asm stuff only compile on Linux and FreeBSD
 * (draw(col|span).s failed on OpenBSD, linker troubles)
 *
 * Revision 1.7  1999/10/12 13:00:57  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.6  1999/05/16 08:51:00  cphipps
 * Detect endianness using systen headers on Linux
 *
 * Revision 1.5  1999/01/10 16:01:45  cphipps
 * Remove "unsigned" from macro result casts
 *
 * Revision 1.4  1999/01/10 15:38:33  cphipps
 * Rewrote endianness handling
 * Make new endianness macros available
 * Clear distinction between host, network and data endiannesses
 *
 * Revision 1.3  1998/12/31 16:44:28  cphipps
 * Fast endianness handling for I386 targets
 *
 * Revision 1.2  1998/12/22 21:12:39  cphipps
 * Changed __inline__'s to inline's, as specified in gcc's docs
 * Added const's
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.3  1998/05/03  23:14:03  killough
 * Make endian independent, beautify
 *
 * Revision 1.2  1998/01/26  19:27:15  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:03:08  rand
 * Lee's Jan 19 sources
 *
 *
 *----------------------------------------------------------------------------*/
