/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_axes.c,v 1.2 2002/08/10 18:23:08 proff_fs Exp $
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
 *   Generic Axis handling for Linux
 *
 *-----------------------------------------------------------------------------
 */

#include <stdlib.h>
#include "m_argv.h"
#include "lprintf.h"
#include "i_axes.h"
#include "SDL.h"

SDL_Joystick** joysticks;

void I_InitAxes(void)
{
  const char* fname = "I_InitAxes :";
  int num_joysticks;
  int i;

  if (M_CheckParm("-noaxes") /*|| !enableaxis */) {
    lprintf(LO_INFO, "%s user disabled\n", fname);
    // How do I actually disable mouse listening?
    return;
  }

  lprintf(LO_INFO, "%s device 0 set to system mouse\n", fname);

  if (M_CheckParm("-nojoy")) {
    lprintf(LO_INFO, "%s joysticks disabled by user\n", fname);
    lprintf(LO_INFO, "%s axis module ready\n", fname);
    return;
  }

  SDL_InitSubSystem(SDL_INIT_JOYSTICK);

  SDL_JoystickEventState(SDL_ENABLE);

  num_joysticks=SDL_NumJoysticks();

  joysticks = (SDL_Joystick **)calloc(num_joysticks, sizeof(SDL_Joystick *));

  for(i=0; i<num_joysticks;i++) {
    lprintf(LO_INFO, "%s device %i set to %s\n", fname, i, SDL_JoystickName(i));
    joysticks[i] = SDL_JoystickOpen(i);
  }

  lprintf(LO_INFO, "%s axis module ready\n", fname);
  return;
}

int I_GetNumberOfDevices(void)
{
  if( (SDL_JoystickEventState(SDL_QUERY)) == SDL_ENABLE ) {
    return (SDL_NumJoysticks());
  }
  return 0;
}

int I_GetAxesForDevice(int device)
{
  // Get Number of axes for joystick
  return SDL_JoystickNumAxes(SDL_JoystickOpen(device));
}

const char *I_GetDeviceName(int device)
{
  return SDL_JoystickName(device);
}
