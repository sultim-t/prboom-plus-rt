// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: M_cheat.h,v 1.1 2000/04/09 17:59:31 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//      Cheat code checking.
//
//-----------------------------------------------------------------------------

#ifndef __M_CHEAT__
#define __M_CHEAT__

// killough 4/16/98: Cheat table structure

extern struct cheat_s {
  const unsigned char *cheat;
  const char *const deh_cheat;
  enum { 
    always   = 0,
    not_dm   = 1,
    not_coop = 2,
    not_demo = 4, 
    not_menu = 8,
    not_deh = 16,
    not_net = not_dm | not_coop
  } const when;
  void (*const func)();
  const int arg;
  ulonglong code, mask;
  H_boolean deh_modified; // killough 9/12/98
} cheat[];

H_boolean M_FindCheats(int key);

extern int idmusnum;

#endif

//----------------------------------------------------------------------------
//
// $Log: M_cheat.h,v $
// Revision 1.1  2000/04/09 17:59:31  proff_fs
// Initial revision
//
// Revision 1.5  1998/05/03  22:10:56  killough
// Cheat engine, moved from st_stuff
//
// Revision 1.4  1998/05/01  14:38:08  killough
// beautification
//
// Revision 1.3  1998/02/09  03:03:07  killough
// Rendered obsolete by st_stuff.c
//
// Revision 1.2  1998/01/26  19:27:08  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:58  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
