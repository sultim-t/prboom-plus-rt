/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze, Andrey Budko
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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
 *      Uncapped framerate stuff
 *
 *---------------------------------------------------------------------
 */

#include "doomstat.h"

extern int movement_smooth;

extern fixed_t r_TicFrac;
extern int otic;
extern boolean NewThinkerPresent;

extern fixed_t oviewx;
extern fixed_t oviewy;
extern fixed_t oviewz;
extern angle_t oviewangle;
extern angle_t oviewpitch;

extern boolean isExtraDDisplay;
extern boolean skipDDisplay;
extern unsigned int DDisplayTime;

void Extra_D_Display(void);
fixed_t I_GetTimeFrac (void);
void I_GetTime_SaveMS(void);
void R_InterpolateView (player_t *player, fixed_t frac);

extern boolean r_NoInterpolate;

void R_ResetViewInterpolation ();
void updateinterpolations();
void stopallinterpolation(void);
void dointerpolations(fixed_t smoothratio);
void restoreinterpolations();
void P_ActivateAllInterpolations();
void SetInterpolationIfNew(thinker_t *th);
void StopInterpolationIfNeeded(thinker_t *th);

