/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 * $Id: f_wipe.c,v 1.11 2003/02/15 17:23:38 dukope Exp $
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
 *      Mission begin melt/wipe screen special effect.
 *
 *-----------------------------------------------------------------------------
 */

static const char rcsid[] = "$Id: f_wipe.c,v 1.11 2003/02/15 17:23:38 dukope Exp $";

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include "z_zone.h"
#include "doomdef.h"
#include "i_video.h"
#include "v_video.h"
#include "m_random.h"
#include "f_wipe.h"

//
// SCREEN WIPE PACKAGE
//

// Parts re-written to support true-color video modes. Column-major
// formatting removed. - POPE

// CPhipps - macros for the source and destination screens
#define SRC_SCR 2
#define DEST_SCR 3

static TScreenVars wipe_scr_start;
static TScreenVars wipe_scr_end;
static TScreenVars wipe_scr;

static int *y;


static int wipe_initMelt(int width, int height, int ticks)
{
  int i;

  // copy start screen to main screen
  memcpy(wipe_scr.data, wipe_scr_start.data, width*height*V_GetDepth());

  // setup initial column positions (y<0 => not ready to scroll yet)
  y = (int *) malloc(width*sizeof(int));
  y[0] = -(M_Random()%16);
  for (i=1;i<width;i++)
    {
      int r = (M_Random()%3) - 1;
      y[i] = y[i-1] + r;
      if (y[i] > 0)
        y[i] = 0;
      else
        if (y[i] == -16)
          y[i] = -15;
    }
  return 0;
}

static int wipe_doMelt(int width, int height, int ticks)
{
  boolean done = true;
  int i;
  const int depth = V_GetDepth();

  width /= 2;

  while (ticks--) {
    for (i=0;i<width;i++) {
      if (y[i]<0) {
        y[i]++;
        done = false;
        continue;
      }
      if (y[i] < height) {
        byte *s, *d;
        int j, k, dy;

        /* cph 2001/07/29 -
          *  The original melt rate was 8 pixels/sec, i.e. 25 frames to melt
          *  the whole screen, so make the melt rate depend on SCREENHEIGHT
          *  so it takes no longer in high res
          */
        dy = (y[i] < 16) ? y[i]+1 : SCREENHEIGHT/25;
        if (y[i]+dy >= height) dy = height - y[i];
        
        s = (byte*)wipe_scr_end.data    + (y[i]*width+i)  *depth*2;
        d = (byte*)wipe_scr.data        + (y[i]*width+i)  *depth*2;
        for (j=dy;j;j--) {
          for (k=0; k<depth*2; k++) d[k] = s[k];
          d += width*depth*2;
          s += width*depth*2;
        }
        y[i] += dy;
        s = (byte*)wipe_scr_start.data  + (i)             *depth*2;
        d = (byte*)wipe_scr.data        + (y[i]*width+i)  *depth*2;
        for (j=height-y[i];j;j--) {
          for (k=0; k<depth*2; k++) d[k] = s[k];
          d += width*depth*2;
          s += width*depth*2;
        }
        done = false;
      }
    }
  }
  return done;
}

// CPhipps - modified to allocate and deallocate screens[2 to 3] as needed, saving memory

static int wipe_exitMelt(int width, int height, int ticks)
{
  free(y);
  V_FreeScreen(&wipe_scr_start);
  wipe_scr_start.width = 0;
  wipe_scr_start.height = 0;
  V_FreeScreen(&wipe_scr_end);
  wipe_scr_end.width = 0;
  wipe_scr_end.height = 0;
  // Paranoia
  y = NULL;
  screens[SRC_SCR] = wipe_scr_start;
  screens[DEST_SCR] = wipe_scr_end;
  return 0;
}

int wipe_StartScreen(int x, int y, int width, int height)
{
  wipe_scr_start.width = SCREENWIDTH;
  wipe_scr_start.height = SCREENHEIGHT;
  wipe_scr_start.not_on_heap = false;
  V_AllocScreen(&wipe_scr_start);
  screens[SRC_SCR] = wipe_scr_start;
  V_CopyRect(x, y, 0,       width, height, x, y, SRC_SCR, VPT_NONE ); // Copy start screen to buffer
  return 0;
}

int wipe_EndScreen(int x, int y, int width, int height)
{
  wipe_scr_end.width = SCREENWIDTH;
  wipe_scr_end.height = SCREENHEIGHT;
  wipe_scr_end.not_on_heap = false;
  V_AllocScreen(&wipe_scr_end);
  screens[DEST_SCR] = wipe_scr_end;
  V_CopyRect(x, y, 0,       width, height, x, y, DEST_SCR, VPT_NONE); // Copy end screen to buffer
  V_CopyRect(x, y, SRC_SCR, width, height, x, y, 0       , VPT_NONE); // restore start screen
  return 0;
}


#include "m_argv.h" // for M_CheckParm - POPE

// killough 3/5/98: reformatted and cleaned up
int wipe_ScreenWipe(int x, int y, int width, int height, int ticks)
{
  static boolean go = 0;                               // when zero, stop the wipe
  
  if (M_CheckParm("-nowipe")) { // POPE  
    memcpy(screens[0].data, wipe_scr_end.data, width*height*V_GetDepth());
    return 1;
  }
  
  if (!go)                                         // initial stuff
    {
      go = 1;
      wipe_scr = screens[0];
      wipe_initMelt(width, height, ticks);
    }
  if (wipe_doMelt(width, height, ticks))     // final stuff
    {
      wipe_exitMelt(width, height, ticks);
      go = 0;
    }
  return !go;
}
