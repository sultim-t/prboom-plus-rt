/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 *
 * Axis Bindings
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __G_BINDAXES_H__
#define __G_BINDAXES_H__

void G_InitAxisBindings();
boolean G_AxisResponder(event_t *ev);

void G_EditAxisBinding(char *action);
const char *G_BoundAxes(char *action);
void G_WriteAxisBindings(FILE* file);

// action variables

extern int axis_forward_value;
extern int axis_side_value;
extern int axis_turn_value;
extern int axis_cycleweapon_value;
extern int axis_changeweapon_value;

#endif
