// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//  System specific network interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef __I_NET__
#define __I_NET__

// Called by D_DoomMain.

void I_InitNetwork (void);
void I_NetCmd (void);

#endif

//----------------------------------------------------------------------------
//
// $Log: i_net.h,v $
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
