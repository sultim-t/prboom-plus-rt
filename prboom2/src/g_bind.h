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
 * Key Bindings
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __G_BIND_H__
#define __G_BIND_H__

void G_InitKeyBindings(void);
boolean G_KeyResponder(event_t *ev);

void G_EditBinding(const char *action);
char *G_BoundKeys(const char *action);
void G_WriteBindings(FILE* file);

// action variables

extern int action_forward,  action_backward;
extern int action_left,     action_right;
extern int action_moveleft, action_moveright;
extern int action_use;
extern int action_speed;
extern int action_attack;
extern int action_strafe;
extern int action_flip;

extern int action_weapon1, action_weapon2, action_weapon3;
extern int action_weapon4, action_weapon5, action_weapon6;
extern int action_weapon7, action_weapon8, action_weapon9;
extern int action_toggleweapon;

extern int action_frags;

#endif

