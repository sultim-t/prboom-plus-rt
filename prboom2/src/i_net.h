/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_net.h,v 1.1 2000/05/04 08:02:55 proff_fs Exp $
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

//----------------------------------------------------------------------------
//
// $Log: i_net.h,v $
// Revision 1.1  2000/05/04 08:02:55  proff_fs
// Initial revision
//
// Revision 1.3  1999/10/12 13:01:15  cphipps
// Changed header to GPL
//
// Revision 1.2  1999/01/13 10:51:32  cphipps
// Parameter changes for revised net code
// Move net command flags here, wrapped into an enum
//
// Revision 1.1  1998/09/13 16:49:50  cphipps
// Initial revision
//
// Revision 1.3  1998/05/16  09:52:27  jim
// add temporary switch for Lee/Stan's code in d_net.c
//
// Revision 1.2  1998/01/26  19:26:56  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:08  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
