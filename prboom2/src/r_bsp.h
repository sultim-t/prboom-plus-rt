/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: r_bsp.h,v 1.1 2000/05/04 08:15:25 proff_fs Exp $
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
 *      Refresh module, BSP traversal and handling.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __R_BSP__
#define __R_BSP__

#ifdef __GNUG__
#pragma interface
#endif

extern seg_t    *curline;
extern side_t   *sidedef;
extern line_t   *linedef;
extern sector_t *frontsector;
extern sector_t *backsector;
extern int      rw_x;
extern int      rw_stopx;
extern boolean  segtextured;
extern boolean  markfloor;      /* false if the back side is the same plane */
extern boolean  markceiling;

/* old code -- killough:
 * extern drawseg_t drawsegs[MAXDRAWSEGS];
 * new code -- killough: */
extern drawseg_t *drawsegs;
extern unsigned maxdrawsegs;

extern drawseg_t *ds_p;

void R_ClearClipSegs(void);
void R_ClearDrawSegs(void);
void R_RenderBSPNode(int bspnum);
int R_DoorClosed(void);   /* killough 1/17/98 */

/* killough 4/13/98: fake floors/ceilings for deep water / fake ceilings: */
sector_t *R_FakeFlat(sector_t *, sector_t *, int *, int *, boolean);

#endif

/*----------------------------------------------------------------------------
 *
 * $Log: r_bsp.h,v $
 * Revision 1.1  2000/05/04 08:15:25  proff_fs
 * Initial revision
 *
 * Revision 1.2  1999/10/12 13:00:57  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.5  1998/05/03  22:48:03  killough
 * beautification, use new headers, change decls
 *
 * Revision 1.4  1998/04/14  08:16:15  killough
 * Fix light levels on 2s textures
 *
 * Revision 1.3  1998/02/02  13:31:53  killough
 * Add HOM detector
 *
 * Revision 1.2  1998/01/26  19:27:33  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:03:10  rand
 * Lee's Jan 19 sources
 *
 *
 *----------------------------------------------------------------------------*/
