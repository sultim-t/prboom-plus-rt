// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: F_wipe.c,v 1.2 2000/04/26 20:00:02 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//      Mission begin melt/wipe screen special effect.
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: F_wipe.c,v 1.2 2000/04/26 20:00:02 proff_fs Exp $";

#include "doomdef.h"
#include "i_video.h"
#include "v_video.h"
#include "m_random.h"
#include "f_wipe.h"

//
// SCREEN WIPE PACKAGE
//

#ifndef GL_DOOM
// proff 11/99: not needed in OpenGL

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

static int wipe_initColorXForm(int width, int height, int ticks)
{
  memcpy(wipe_scr, wipe_scr_start, width*height);
  return 0;
}

// killough 3/5/98: reformatted and cleaned up
static int wipe_doColorXForm(int width, int height, int ticks)
{
  H_boolean unchanged = true;
  byte *w   = wipe_scr;
  byte *e   = wipe_scr_end;
  byte *end = wipe_scr+width*height;
  for (;w != end; w++, e++)
    if (*w != *e)
      {
        int newval;
        unchanged = false;
        *w = *w > *e ?
          (newval = *w - ticks) < *e ? *e : newval :
          (newval = *w + ticks) > *e ? *e : newval ;
      }
  return unchanged;
}

static int wipe_exitColorXForm(int width, int height, int ticks)
{
  return 0;
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
  H_boolean done = true;
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

static int wipe_exitMelt(int width, int height, int ticks)
{
  Z_Free(y);
  return 0;
}

int wipe_StartScreen(int x, int y, int width, int height)
{
  I_ReadScreen(wipe_scr_start = screens[2]);
  return 0;
}

int wipe_EndScreen(int x, int y, int width, int height)
{
  I_ReadScreen(wipe_scr_end = screens[3]);
  V_DrawBlock(x, y, 0, width, height, wipe_scr_start); // restore start scr.
  return 0;
}

static int (*const wipes[])(int, int, int) = {
  wipe_initColorXForm,
  wipe_doColorXForm,
  wipe_exitColorXForm,
  wipe_initMelt,
  wipe_doMelt,
  wipe_exitMelt
};

// killough 3/5/98: reformatted and cleaned up
int wipe_ScreenWipe(int wipeno, int x, int y, int width, int height, int ticks)
{
  static H_boolean go;                               // when zero, stop the wipe

//  ticks=(ticks*SCREENHEIGHT)/200; // proff -- added for hi-res

  if (!go)                                         // initial stuff
    {
      go = 1;
      wipe_scr = screens[0];
      wipes[wipeno*3](width, height, ticks);
    }
  // do a piece of wipe-in
  if (wipes[wipeno*3+1](width, height, ticks))     // final stuff
    {
      wipes[wipeno*3+2](width, height, ticks);
      go = 0;
    }
  return !go;
}
#endif // GL_DOOM

//----------------------------------------------------------------------------
//
// $Log: F_wipe.c,v $
// Revision 1.2  2000/04/26 20:00:02  proff_fs
// now using SDL for video and sound output.
// sound output is currently mono only.
// Get SDL from:
// http://www.devolution.com/~slouken/SDL/
//
// Revision 1.1.1.1  2000/04/09 18:17:41  proff_fs
// Initial login
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
