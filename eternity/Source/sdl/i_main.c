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
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//      Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_main.c,v 1.8 1998/05/15 00:34:03 killough Exp $";

#include "SDL.h"

#include "../doomdef.h"
#include "../m_argv.h"
#include "../d_main.h"
#include "../i_system.h"

// SoM 3/13/2001: Use SDL's handler

void I_Quit(void);

int main(int argc, char **argv)
{
   Uint32 sdlInitFlags;

   myargc = argc;
   myargv = argv;
   
   // SoM 3/11/2002: Disable the parachute for debugging.
   // haleyjd 04/15/02: fixed #ifdef and added joystick, restructured

   sdlInitFlags = SDL_INIT_VIDEO |                  
                  SDL_INIT_JOYSTICK;

#ifdef _DEBUG
   sdlInitFlags |= SDL_INIT_NOPARACHUTE;
#endif

   // haleyjd 04/15/02: added check for failure
   if(SDL_Init(sdlInitFlags) == -1)
   {
      printf("Failed to initialize SDL library.\n");
      return -1;
   }

   // haleyjd 02/23/04: ignore mouse events at startup
   SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
   
   SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
   
   Z_Init();
   atexit(I_Quit);
   
   D_DoomMain();
   
   return 0;
}


//----------------------------------------------------------------------------
//
// $Log: i_main.c,v $
//
//----------------------------------------------------------------------------
