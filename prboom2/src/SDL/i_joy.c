/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_joy.c,v 1.1.2.1 2001/02/18 18:07:22 proff_fs Exp $
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
 *   Joystick handling for Linux
 *
 *-----------------------------------------------------------------------------
 */

#ifndef lint
static const char rcsid[] = "$Id: i_joy.c,v 1.1.2.1 2001/02/18 18:07:22 proff_fs Exp $";
#endif /* lint */

#include "SDL.h"
#include "doomdef.h"
#include "doomtype.h"
#include "m_argv.h"
#include "d_event.h"
#include "d_main.h"
#include "i_joy.h"
#include "lprintf.h"

int joyleft;
int joyright;
int joyup;
int joydown;

int usejoystick;

static SDL_Joystick *joystick;

void I_EndJoystick(void)
{
  lprintf(LO_DEBUG, "I_EndJoystick : closing joystick\n");
}

void I_PollJoystick(void)
{
#ifdef JOY_CODE
  if (!usejoystick || (joy_fd == -1)) return;
  if (!I_ReadJoystick()) I_ReopenJoystick();
	if (I_ReadJoystick())	 {
    event_t ev;

#ifndef DOSDOOM
    ev.type = ev_joystick;
    ev.data1 = jdata.buttons;
#else
    ev.type = ev_analogue;
    ev.data1 = ev.data3 = 0;
#endif

    ev.data2=(jdata.x < joyleft) ? -1 : ((jdata.x > joyright) ? 1 : 0);

#ifndef DOSDOOM
    ev.data3 = 
#else
    ev.data4 = 
#endif
      (jdata.y < joyup) ? -1 : ((jdata.y > joydown ) ? 1 : 0);
    D_PostEvent(&ev);

#ifdef DOSDOOM
    /* Buttons handled as keypress events */
    {
      static unsigned int old_buttons = 0;
      int button_num;

      for (button_num = 0; button_num<3; button_num++) {
	unsigned int mask = 1 << button_num;

	if ((old_buttons & mask) != (jdata.buttons & mask)) {
	  ev.type = (jdata.buttons & mask) ? ev_keydown : ev_keyup;
	  ev.data1 = KEYD_JOY1 + button_num;
	}
      }
    }  
#endif
  }
#endif
}

void I_InitJoystick(void)
{
  const char* fname = "I_InitJoystick : ";
  int num_joysticks;

  if (!usejoystick) return;
  num_joysticks=SDL_NumJoysticks();
  if (M_CheckParm("-nojoy") || (usejoystick>num_joysticks) || (usejoystick<0)) {
    if ((usejoystick > num_joysticks) || (usejoystick < 0))
      lprintf(LO_WARN, "%sinvalid joystick %d\n", fname, usejoystick);
    else
      lprintf(LO_INFO, "%suser disabled\n", fname);
    return;
  }
  joystick=SDL_JoystickOpen(usejoystick);
  if (!joystick)
    lprintf(LO_ERROR, "%serror opening joystick %s\n", fname, SDL_JoystickName(usejoystick));
  else {
    atexit(I_EndJoystick);
    lprintf(LO_INFO, "%sopened %s\n", fname, SDL_JoystickName(usejoystick));
    joyup = 32767;
    joydown = -32768;
    joyright = 32767;
    joyleft = -32768;
  }
}
