/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: r_things.h,v 1.1 2000/05/04 08:16:51 proff_fs Exp $
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
 *      Rendering of moving objects, sprites.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __R_THINGS__
#define __R_THINGS__

#ifdef __GNUG__
#pragma interface
#endif

/* Constant arrays used for psprite clipping and initializing clipping. */

extern short negonearray[MAX_SCREENWIDTH];         /* killough 2/8/98: */
extern short screenheightarray[MAX_SCREENWIDTH];   /* change to MAX_*  */

/* Vars for R_DrawMaskedColumn */

extern short   *mfloorclip;
extern short   *mceilingclip;
extern fixed_t spryscale;
extern fixed_t sprtopscreen;
extern fixed_t pspritescale;
extern fixed_t pspriteiscale;
/* proff 11/06/98: Added for high-res */
extern fixed_t pspriteyscale;

void R_DrawMaskedColumn(const column_t *column);
void R_SortVisSprites(void);
void R_AddSprites(sector_t *sec, int lightlevel);
void R_AddPSprites(void);
void R_DrawSprites(void);
void R_InitSprites(const char * const * namelist);
void R_ClearSprites(void);
void R_DrawMasked(void);

void R_ClipVisSprite(vissprite_t *vis, int xl, int xh);

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: r_things.h,v $
 * Revision 1.1  2000/05/04 08:16:51  proff_fs
 * Initial revision
 *
 * Revision 1.6  1999/10/12 13:00:57  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.5  1998/12/31 23:06:56  cphipps
 * R_DrawMaskedColumn takes a const column_t*
 *
 * Revision 1.4  1998/12/24 10:10:30  cphipps
 * Parameter change for R_AddSprites
 * (undeerwater sprite lighting fix from MBF)
 *
 * Revision 1.3  1998/11/17 12:13:21  cphipps
 * Hi-res additions
 *
 * Revision 1.2  1998/10/16 23:11:37  cphipps
 * Made R_InitSprites take a const * const * parameter
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.4  1998/05/03  22:46:19  killough
 * beautification
 *
 * Revision 1.3  1998/02/09  03:23:27  killough
 * Change array decl to use MAX screen width/height
 *
 * Revision 1.2  1998/01/26  19:27:49  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:03:09  rand
 * Lee's Jan 19 sources
 *
 *
*----------------------------------------------------------------------------*/
