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
// DESCRIPTION:
//      System specific interface stuff.
//
//-----------------------------------------------------------------------------

#ifndef __I_VIDEO__
#define __I_VIDEO__

#ifdef DJGPP
  #include <allegro.h>
#endif

#include "doomtype.h"

// Called by D_DoomMain,
// determines the hardware configuration
// and sets up the video mode

// sf: made lots into function ptrs

void I_InitGraphics (void);
void I_ShutdownGraphics(void);

// Takes full 8 bit values.
extern void (*I_SetPalette) (byte* palette);

void I_UpdateNoBlit (void);
extern void (*I_FinishUpdate) (void);

// Wait for vertical retrace or pause a bit.
void I_WaitVBL(int count);

void I_ReadScreen (byte* scr);

int I_DoomCode2ScanCode(int);   // killough
int I_ScanCode2DoomCode(int);   // killough

#ifdef DJGPP
 extern int use_vsync;  // killough 2/8/98: controls whether vsync is called
 extern int page_flip;  // killough 8/15/98: enables page flipping (320x200)
 extern int disk_icon;  // killough 10/98
 extern int vesamode;
 //SoM: removed hires HACK  ANYRES
 extern BITMAP *screens0_bitmap;   // killough 12/98
#endif

// video modes

typedef struct videomode_s
{
  int anyres;
  int pageflip;
  int vesa;
  char *description;
} videomode_t;

extern videomode_t *videomodes;

void I_CheckVESA(); // DJGPP only

extern void (*I_SetMode)(int i);

typedef struct viddriver_s viddriver_t;

struct viddriver_s
{
  // driver name
  char *driver_name;

  /*** Graphics ***/
  void (*FinishUpdate)();         // draw scren
  void (*SetPalette)(byte *pal);
  void (*ShutdownGraphics)();
  void (*ResetScreen)();          // reset vid mode
  void (*InitGraphics)();         
  void (*SetMode)(int i);         // set video mode
  
  /*** Input functions ***/
  void (*StartTic)();             // get key/mouse events
  
  videomode_t *videomodes;        // array of video modes for this driver
};


#endif

//----------------------------------------------------------------------------
//
// $Log: i_video.h,v $
// Revision 1.4  1998/05/03  22:40:58  killough
// beautification
//
// Revision 1.3  1998/02/09  03:01:51  killough
// Add vsync for flicker-free blits
//
// Revision 1.2  1998/01/26  19:27:01  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:08  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
