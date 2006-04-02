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
// Mission begin melt/wipe screen special effect.
//
// Rewritten by Simon Howard
// Portions which deal with the movement of the columns adapted
// from the original sources
//
//-----------------------------------------------------------------------------

// 13/12/99: restored movement of columns to being the same as in the
// original, while retaining the new 'engine'

#include "c_io.h"
#include "doomdef.h"
#include "d_main.h"
#include "i_video.h"
#include "v_video.h"
#include "m_random.h"
#include "f_wipe.h"

// array of pointers to the
// column data for 'superfast' melt
static char *start_screen[MAX_SCREENWIDTH] = {0};

// y co-ordinate of various columns
static int worms[SCREENWIDTH];

boolean        inwipe = false;
static int     starting_height;

void Wipe_Initwipe(void)
{
   int x;
   
   inwipe = true;
   
   // SoM 2-4-04: ANYRES
   // use console height
   starting_height = (current_height * globalyscale) >> FRACBITS;
   
   worms[0] = starting_height - M_Random()%16;
   
   for(x = 1; x < SCREENWIDTH; ++x)
   {
      int r = (M_Random()%3) - 1;
      worms[x] = worms[x-1] + r;
      if (worms[x] > 0)
         worms[x] = 0;
      else
         if (worms[x] == -16)
            worms[x] = -15;
   }
}

void Wipe_StartScreen(void)
{
   register int x, y;
   register byte *dest, *src;

   Wipe_Initwipe();
  
   if(!start_screen[0])
   {
      // SoM: Reformatted and cleaned up (ANYRES)
      byte *buffer = start_screen[0] = Z_Malloc(MAX_SCREENHEIGHT * MAX_SCREENWIDTH,PU_STATIC,0);
      for(x = 0; x < MAX_SCREENWIDTH; x++)
         start_screen[x] = buffer + (x * MAX_SCREENHEIGHT);    
   }

   // SoM 2-4-04: ANYRES
   for(x = 0; x < v_width; ++x)
   {
      // limit check
      int wormx = (x << FRACBITS) / globalxscale;
      int wormy = realyarray[worms[wormx] > 0 ? worms[wormx] : 0];
      
      src = screens[0] + x;
      dest = start_screen[x];
      
      for(y = 0; y < v_height - wormy; y++)
      {
         *dest = *src;
         src += v_width;
         dest++;
      }
   }
   
   return;
}

void Wipe_Drawer(void)
{
   register int x, y;
   register char *dest, *src;

   // SoM 2-4-04: ANYRES
   for(x = 0; x < v_width; ++x)
   {
      int wormy, wormx;
      
      wormx = (x << FRACBITS) / globalxscale;
      wormy = worms[wormx] > 0 ? worms[wormx] : 0;  // limit check
	  
      wormy = realyarray[wormy];

      src = start_screen[x];
      dest = screens[0] + v_width * wormy + x;
      
      for(y = v_height - wormy; y--;)
      {
         *dest = *src++;
         dest += v_width;
      }
   }
 
   redrawsbar = true; // clean up status bar
}

void Wipe_Ticker(void)
{
   boolean done;
   int x;
  
   done = true;  // default to true

   // SoM 2-4-04: ANYRES
   for(x = 0; x < SCREENWIDTH; ++x)
      if(worms[x] < 0)
      {
         ++worms[x];
         done = false;
      }
      else if(worms[x] < SCREENHEIGHT)
      {
         int dy;

         dy = (worms[x] < 16) ? worms[x] + 1 : 8;

         if(worms[x] + dy >= SCREENHEIGHT)
            dy = SCREENHEIGHT - worms[x];
         worms[x] += dy;
         done = false;
      }
  
   if(done)
      inwipe = false;
}

// EOF

