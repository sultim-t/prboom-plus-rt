// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: R_things.h,v 1.1 2000/04/09 18:19:49 proff_fs Exp $
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
//      Rendering of moving objects, sprites.
//
//-----------------------------------------------------------------------------

#ifndef __R_THINGS__
#define __R_THINGS__

// Constant arrays used for psprite clipping and initializing clipping.

extern short negonearray[MAX_SCREENWIDTH];         // killough 2/8/98:
extern short screenheightarray[MAX_SCREENWIDTH];   // change to MAX_*

// Vars for R_DrawMaskedColumn

extern short   *mfloorclip;
extern short   *mceilingclip;
extern fixed_t spryscale;
extern fixed_t sprtopscreen;
extern fixed_t pspritescale;
extern fixed_t pspriteiscale;
// proff 11/06/98: Added for high-res
extern fixed_t pspriteyscale;

void R_DrawMaskedColumn(column_t *column);
void R_SortVisSprites(void);
void R_AddSprites(sector_t *sec,int); // killough 9/18/98
void R_AddPSprites(void);
void R_DrawSprites(void);
void R_InitSprites(char **namelist);
void R_ClearSprites(void);
void R_DrawMasked(void);

void R_ClipVisSprite(vissprite_t *vis, int xl, int xh);

#endif

//----------------------------------------------------------------------------
//
// $Log: R_things.h,v $
// Revision 1.1  2000/04/09 18:19:49  proff_fs
// Initial revision
//
// Revision 1.4  1998/05/03  22:46:19  killough
// beautification
//
// Revision 1.3  1998/02/09  03:23:27  killough
// Change array decl to use MAX screen width/height
//
// Revision 1.2  1998/01/26  19:27:49  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:09  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
