/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: p_map.h,v 1.2 2000/05/09 21:45:39 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Colin Phipps (cph@lxdoom.linuxgames.com), 
 *  Jess Haas (JessH@lbjhs.net)
 *  and Florian Schulze (florian.proff.schulze@gmx.net)
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
 *      Map functions
 *
 *-----------------------------------------------------------------------------*/

#ifndef __P_MAP__
#define __P_MAP__

#include "r_defs.h"
#include "d_player.h"

#define USERANGE        (64*FRACUNIT)
#define MELEERANGE      (64*FRACUNIT)
#define MISSILERANGE    (32*64*FRACUNIT)

// MAXRADIUS is for precalculated sector block boxes the spider demon
// is larger, but we do not have any moving sectors nearby
#define MAXRADIUS       (32*FRACUNIT)

// killough 3/15/98: add fourth argument to P_TryMove
boolean P_TryMove(mobj_t *thing, fixed_t x, fixed_t y, boolean dropoff);

boolean P_TeleportMove(mobj_t *thing, fixed_t x, fixed_t y);
void    P_SlideMove(mobj_t *mo);
boolean P_CheckSight(mobj_t *t1, mobj_t *t2);
void    P_UseLines(player_t *player);
boolean P_ChangeSector(sector_t *sector, boolean crunch);
fixed_t P_AimLineAttack(mobj_t *t1, angle_t angle, fixed_t distance);
void    P_LineAttack(mobj_t *t1, angle_t angle, fixed_t distance,
                     fixed_t slope, int damage );
void    P_RadiusAttack(mobj_t *spot, mobj_t *source, int damage);
boolean P_CheckPosition(mobj_t *thing, fixed_t x, fixed_t y);

//jff 3/19/98 P_CheckSector(): new routine to replace P_ChangeSector()
boolean P_CheckSector(sector_t *sector, boolean crunch);
void    P_DelSeclist(msecnode_t *);                         // phares 3/16/98
void    P_CreateSecNodeList(mobj_t*,fixed_t,fixed_t);       // phares 3/14/98
int     P_GetMoveFactor(mobj_t* mo);                        // phares  3/6/98
boolean Check_Sides(mobj_t *, int, int);                    // phares


// If "floatok" true, move would be ok if within "tmfloorz - tmceilingz".
extern boolean floatok;
extern fixed_t tmfloorz;
extern fixed_t tmceilingz;
extern line_t *ceilingline;
extern mobj_t *linetarget;     // who got hit (or NULL)
extern msecnode_t *sector_list;                             // phares 3/16/98
extern fixed_t tmbbox[4];         // phares 3/20/98

#endif // __P_MAP__
