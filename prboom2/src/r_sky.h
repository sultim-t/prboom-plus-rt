/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: r_sky.h,v 1.1 2000/05/04 08:16:34 proff_fs Exp $
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
 *      Sky rendering.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __R_SKY__
#define __R_SKY__

#include "m_fixed.h"

#ifdef __GNUG__
#pragma interface
#endif

/* SKY, store the number for name. */
#define SKYFLATNAME  "F_SKY1"

/* The sky map is 256*128*4 maps. */
#define ANGLETOSKYSHIFT         22

extern int skytexture;
extern int skytexturemid;

/* Called whenever the view size changes. */
void R_InitSkyMap(void);

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: r_sky.h,v $
 * Revision 1.1  2000/05/04 08:16:34  proff_fs
 * Initial revision
 *
 * Revision 1.2  1999/10/12 13:00:57  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.4  1998/05/03  22:56:25  killough
 * Add m_fixed.h #include
 *
 * Revision 1.3  1998/05/01  14:15:29  killough
 * beautification
 *
 * Revision 1.2  1998/01/26  19:27:46  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:03:09  rand
 * Lee's Jan 19 sources
 *
 *
 *----------------------------------------------------------------------------*/
