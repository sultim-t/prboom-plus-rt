/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_net.h,v 1.3 2000/09/16 20:20:36 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 *  Old network interface stuff.
 *
 *-----------------------------------------------------------------------------*/


#ifndef __I_NET__
#define __I_NET__


#ifdef __GNUG__
#pragma interface
#endif

// Called by D_DoomMain.

enum netcmdcode_e {
  NCMD_EXIT       = 0x80000000,
  NCMD_RETRANSMIT = 0x40000000,
  NCMD_SETUP      = 0x20000000,
  NCMD_KILL       = 0x10000000,      /* kill game */
  NCMD_NOSWAP     = 0xb0000000, // cph - endianness fixed by d_net.c _not_ i_net.c
  NCMD_CHECKSUM   = 0x0fffffff
};

void I_InitNetwork (void);
void I_NetCmd (enum netcmdcode_e flags);
int ExpandTics(int);

#endif
