/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_joy.h,v 1.1 2000/05/04 08:02:55 proff_fs Exp $
 *
 *  New joystick code for LxDoom
 *  Copyright (C) 1999 by Colin Phipps
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
 *  Joystick interface.
 *
 *-----------------------------------------------------------------------------*/

extern int joybfire;
extern int joybstrafe;
extern int joybuse;
extern int joybspeed;

extern int joyleft;
extern int joyright;
extern int joyup;
extern int joydown;

extern int usejoystick;

void I_InitJoystick();
void I_PollJoystick();

/*
 * $Log: i_joy.h,v $
 * Revision 1.1  2000/05/04 08:02:55  proff_fs
 * Initial revision
 *
 * Revision 1.2  1999/10/12 13:00:56  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 */


