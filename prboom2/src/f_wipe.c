/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: f_wipe.c,v 1.2 2000/05/06 08:49:55 cph Exp $
 *
 *  LxDoom, a Doom port for Linux/Unix
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *   and Colin Phipps
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

static const char rcsid[] = "$Id: f_wipe.c,v 1.2 2000/05/06 08:49:55 cph Exp $";

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

// CPhipps - macros for the source and destination screens
#define SRC_SCR 2
#define DEST_SCR 3

static byte *wipe_scr_start;
static byte *wipe_scr_end;
static byte *wipe_scr;

static void wipe_shittyColMajorXform(short *array, int width, int height)
{
  short *dest = Z_Malloc(width*height*sizeof(short), PU_STATIC, 0);
  int x, y;

  for(y=0;y<height;y++)
    for(x=0;x<width;x++)
      dest[x*height+y] = array[y*width+x];
  memcpy(array, dest, width*height*sizeof(short));
  Z_Free(dest);
}

static int *y;

static int wipe_initMelt(int width, int height, int ticks)
{
  int i;

  // copy start screen to main screen
  memcpy(wipe_scr, wipe_scr_start, width*height);

  // makes this wipe faster (in theory)
  // to have stuff in column-major format
  wipe_shittyColMajorXform((short*)wipe_scr_start, width/2, height);
  wipe_shittyColMajorXform((short*)wipe_scr_end, width/2, height);

  // setup initial column positions (y<0 => not ready to scroll yet)
  y = (int *) Z_Malloc(width*sizeof(int), PU_STATIC, 0);
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

  width /= 2;

  while (ticks--)
    for (i=0;i<width;i++)
      if (y[i]<0)
        {
          y[i]++;
          done = false;
        }
      else
        if (y[i] < height)
          {
            short *s, *d;
            int j, dy, idx;

            dy = (y[i] < 16) ? y[i]+1 : 8;
            if (y[i]+dy >= height)
              dy = height - y[i];
            s = &((short *)wipe_scr_end)[i*height+y[i]];
            d = &((short *)wipe_scr)[y[i]*width+i];
            idx = 0;
            for (j=dy;j;j--)
              {
                d[idx] = *(s++);
                idx += width;
              }
            y[i] += dy;
            s = &((short *)wipe_scr_start)[i*height];
            d = &((short *)wipe_scr)[y[i]*width+i];
            idx = 0;
            for (j=height-y[i];j;j--)
              {
                d[idx] = *(s++);
                idx += width;
              }
            done = false;
          }
  return done;
}

// CPhipps - modified to allocate and deallocate screens[2 to 3] as needed, saving memory 

static int wipe_exitMelt(int width, int height, int ticks)
{
  Z_Free(y);
  Z_Free(wipe_scr_start);
  Z_Free(wipe_scr_end);
  // Paranoia
  y = NULL;
  wipe_scr_start = wipe_scr_end = screens[SRC_SCR] = screens[DEST_SCR] = NULL;
  return 0;
}

int wipe_StartScreen(int x, int y, int width, int height)
{
  wipe_scr_start = screens[SRC_SCR] = malloc(SCREENWIDTH * SCREENHEIGHT);
  V_CopyRect(x, y, 0,       width, height, x, y, SRC_SCR ); // Copy start screen to buffer
  return 0;
}

int wipe_EndScreen(int x, int y, int width, int height)
{
  wipe_scr_end = screens[DEST_SCR] = malloc(SCREENWIDTH * SCREENHEIGHT);
  V_CopyRect(x, y, 0,       width, height, x, y, DEST_SCR); // Copy end screen to buffer
  V_CopyRect(x, y, SRC_SCR, width, height, x, y, 0       ); // restore start screen
  return 0;
}

// killough 3/5/98: reformatted and cleaned up
int wipe_ScreenWipe(int x, int y, int width, int height, int ticks)
{
  static boolean go;                               // when zero, stop the wipe
  if (!go)                                         // initial stuff
    {
      go = 1;
      wipe_scr = screens[0];
      wipe_initMelt(width, height, ticks);
    }
  V_MarkRect(0, 0, width, height);                 // do a piece of wipe-in
  if (wipe_doMelt(width, height, ticks))     // final stuff
    {
      wipe_exitMelt(width, height, ticks);
      go = 0;
    }
  return !go;
}

//----------------------------------------------------------------------------
//
// $Log: f_wipe.c,v $
// Revision 1.2  2000/05/06 08:49:55  cph
// Minor header file fixing
//
// Revision 1.1.1.1  2000/05/04 08:01:33  proff_fs
// initial login on sourceforge as prboom2
//
// Revision 1.5  1999/10/12 13:01:10  cphipps
// Changed header to GPL
//
// Revision 1.4  1999/08/26 21:52:11  cphipps
// Removed old alternate wipe types (ColorXForm stuff), unused
// Macroise screen numbers for the stored screens
// Use standard V_CopyRect to move screens around
//
// Revision 1.3  1998/12/30 22:07:48  cphipps
// Updated for new patch drawing
//
// Revision 1.2  1998/12/28 21:25:20  cphipps
// Allocate screens for wipe storage as needed
//
// Revision 1.1  1998/09/13 16:49:50  cphipps
// Initial revision
//
// Revision 1.3  1998/05/03  22:11:24  killough
// beautification
//
// Revision 1.2  1998/01/26  19:23:16  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:54  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
