// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: F_WIPE.H,v 1.2 2000/04/26 20:00:02 proff_fs Exp $
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
//      Mission start screen wipe/melt, special effects.
//
//-----------------------------------------------------------------------------

#ifndef __F_WIPE_H__
#define __F_WIPE_H__

//
// SCREEN WIPE PACKAGE
//

#ifndef GL_DOOM
// proff 11/99: not needed in OpenGL

enum {
  // simple gradual pixel change for 8-bit only
  wipe_ColorXForm,
  wipe_Melt,        // weird screen melt
  wipe_NUMWIPES
};

int wipe_ScreenWipe (int wipeno,
                     int x, int y, int width, int height, int ticks);
int wipe_StartScreen(int x, int y, int width, int height);
int wipe_EndScreen  (int x, int y, int width, int height);

#endif // GL_DOOM

#endif

//----------------------------------------------------------------------------
//
// $Log: F_WIPE.H,v $
// Revision 1.2  2000/04/26 20:00:02  proff_fs
// now using SDL for video and sound output.
// sound output is currently mono only.
// Get SDL from:
// http://www.devolution.com/~slouken/SDL/
//
// Revision 1.1.1.1  2000/04/09 17:59:16  proff_fs
// Initial login
//
// Revision 1.3  1998/05/03  22:11:27  killough
// beautification
//
// Revision 1.2  1998/01/26  19:26:49  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:54  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
