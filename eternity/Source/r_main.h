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
//      System specific interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef __R_MAIN__
#define __R_MAIN__

#include "d_player.h"
#include "r_data.h"
#include "r_defs.h"
#include "p_chase.h"
#ifdef R_PORTALS
#include "r_portal.h"
#endif

//
// POV related.
//

extern fixed_t  viewcos;
extern fixed_t  viewsin;
extern int      viewwidth;
extern int      viewheight;
extern int      viewwindowx;
extern int      viewwindowy;
// SoM: ANYRES
extern int      scaledwindowx;
extern int      scaledwindowy;
extern int      addscaleshift;

extern int      centerx;
extern int      centery;
extern fixed_t  centerxfrac;
extern fixed_t  centeryfrac;
extern fixed_t  projection;
// ANYRES
extern fixed_t  yaspectmul;

extern int      validcount;
extern int      linecount;
extern int      loopcount;

extern boolean  showpsprites;

//
// Lighting LUT.
// Used for z-depth cuing per column/row,
//  and other lighting effects (sector ambient, flash).
//

// Lighting constants.

#define LIGHTLEVELS       16
#define LIGHTSEGSHIFT      4
#define LIGHTSCALESHIFT   12
#define LIGHTZSHIFT       20

// killough 3/20/98: Allow colormaps to be dynamic (e.g. underwater)
extern lighttable_t *(*scalelight)[MAXLIGHTSCALE];
extern lighttable_t *(*zlight)[MAXLIGHTZ];
extern lighttable_t *fullcolormap;
extern int numcolormaps;    // killough 4/4/98: dynamic number of maps
extern lighttable_t **colormaps;
// killough 3/20/98, 4/4/98: end dynamic colormaps

extern int          extralight;
extern lighttable_t *fixedcolormap;

// Number of diminishing brightness levels.
// There a 0-31, i.e. 32 LUT in the COLORMAP lump.

#define NUMCOLORMAPS 32

//
// Function pointer to switch refresh/drawing functions.
//

extern void (*colfunc)(void);

//
// Utility functions.
//

int R_PointOnSide(fixed_t x, fixed_t y, node_t *node);
int R_PointOnSegSide(fixed_t x, fixed_t y, seg_t *line);
angle_t R_PointToAngle(fixed_t x, fixed_t y);
angle_t R_PointToAngle2(fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2);
fixed_t R_ScaleFromGlobalAngle(angle_t visangle);
subsector_t *R_PointInSubsector(fixed_t x, fixed_t y);
void R_SectorColormap(sector_t *s);

//
// REFRESH - the actual rendering functions.
//

                // sf: camera point added
void R_RenderPlayerView(player_t *player, camera_t *viewcamera); // Called by G_Drawer.
                                // sf: G_Drawer???
void R_Init(void);                           // Called by startup code.
void R_SetViewSize(int blocks);              // Called by M_Responder.

void R_InitLightTables(void);                // killough 8/9/98

extern boolean setsizeneeded;
// SoM
void R_SetupViewScaling(void);
void R_ExecuteSetViewSize(void);

angle_t R_WadToAngle(int wadangle);

extern int viewdir;
extern int zoom;

#endif

//----------------------------------------------------------------------------
//
// $Log: r_main.h,v $
// Revision 1.7  1998/05/03  23:00:42  killough
// beautification
//
// Revision 1.6  1998/04/06  04:43:17  killough
// Make colormaps fully dynamic
//
// Revision 1.5  1998/03/23  03:37:44  killough
// Add support for arbitrary number of colormaps
//
// Revision 1.4  1998/03/09  07:27:23  killough
// Avoid using FP for point/line queries
//
// Revision 1.3  1998/02/02  13:29:10  killough
// performance tuning
//
// Revision 1.2  1998/01/26  19:27:41  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:08  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
