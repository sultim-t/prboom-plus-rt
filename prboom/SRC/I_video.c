// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: I_video.c,v 1.1 2000/04/09 18:15:46 proff_fs Exp $
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
//      DOOM graphics stuff
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: I_video.c,v 1.1 2000/04/09 18:15:46 proff_fs Exp $";

#ifdef _WIN32 // proff: Video-routines for Windows
              // this file works together with winstuff.c

#include "z_zone.h"  /* memory allocation wrappers -- killough */

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_argv.h"
#include "d_main.h"
#include "am_map.h"
#include "st_stuff.h"
#include "wi_stuff.h"
#include "r_main.h"
#include "doomdef.h"
#include "winstuff.h"

static int in_graphics_mode;

void I_StartFrame (void)
{
}

unsigned char key_ascii_table[128] =
{
/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F             */
   0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,   9,       /* 0 */
   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13,  0,   'a', 's',     /* 1 */
   'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39,  '`', 0,   92,  'z', 'x', 'c', 'v',     /* 2 */
   'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   3,   3,   3,   3,   8,       /* 3 */
   3,   3,   3,   3,   3,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,       /* 4 */
   0,   0,   0,   127, 0,   0,   92,  3,   3,   0,   0,   0,   0,   0,   0,   0,       /* 5 */
   13,  0,   '/', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   127,     /* 6 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0        /* 7 */
};

int I_ScanCode2DoomCode (int a)
{
  // proff: a was sometimes out of range
  if (a>127)
    return 0;
  switch (a)
    {
    default:   return key_ascii_table[a]>8 ? key_ascii_table[a] : a+0x80;
    case 0x7b: return KEYD_PAUSE;
    case 0x0e: return KEYD_BACKSPACE;
    case 0x48: return KEYD_UPARROW;
    case 0x4d: return KEYD_RIGHTARROW;
    case 0x50: return KEYD_DOWNARROW;
    case 0x4b: return KEYD_LEFTARROW;
    case 0x38: return KEYD_LALT;
    case 0x79: return KEYD_RALT;
    case 0x1d:
    case 0x78: return KEYD_RCTRL;
    case 0x36:
    case 0x2a: return KEYD_RSHIFT;
  }
}

// Automatic caching inverter, so you don't need to maintain two tables.
// By Lee Killough

int I_DoomCode2ScanCode (int a)
{
  static int inverse[256], cache;
  for (;cache<256;cache++)
    inverse[I_ScanCode2DoomCode(cache)]=cache;
  return inverse[a];
}

void I_StartTic (void)
{
    V_GetMessages();
}

void I_UpdateNoBlit (void)
{
}

void I_FinishUpdate (void)
{
    static int    lasttic;
    int        tics;
    int        i;

    if (noblit || !in_graphics_mode)
        return;

    if (devparm)
    {
        i = I_GetTime();
        tics = i - lasttic;
        lasttic = i;
        if (tics > 20) 
            tics = 20;
        for (i=0 ; i<tics*2 ; i+=2)
            screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
        for ( ; i<20*2 ; i+=2)
            screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    }
    V_EndFrame();
#ifdef GL_DOOM
    // proff 11/99: swap OpenGL buffers
 	  gld_Finish();
#endif
}

void I_ReadScreen (byte* scr)
{
  if (!in_graphics_mode)
    return;
  memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

void I_SetPalette (byte* palette)
{
    char newpal[256*3];
    int i;

  if (!in_graphics_mode)
    return;

    for(i = 0;i<256*3;i++)
    {
        newpal[i]=gammatable[usegamma][palette[i]];
    }
    V_SetPal(newpal);
}

void I_ShutdownGraphics(void)
{
  if (in_graphics_mode)
    Done_Winstuff();
  in_graphics_mode=0;
}

void I_InitGraphics(void)
{
    static int firsttime=1;

    if (!firsttime)
        return;

    firsttime=0;

    if (!nodrawers)
    {
        if (Init_Winstuff() == false)
            I_Error("Error: Init_Winstuff failed");
        atexit(I_ShutdownGraphics);
        in_graphics_mode=1;
    }
}

void I_ResetScreen(void)
{
  return;

  if (!in_graphics_mode)
    {
      setsizeneeded = true;
      V_Init();
      return;
    }

  I_ShutdownGraphics();     // Switch out of old graphics mode

  I_InitGraphics();     // Switch to new graphics mode

  if (automapactive)
    AM_Start();             // Reset automap dimensions

  ST_Start();               // Reset palette

  if (gamestate == GS_INTERMISSION)
    {
      WI_DrawBackground();
      V_CopyRect(0, 0, 1, SCREENWIDTH, SCREENHEIGHT, 0, 0, 0);
    }

  Z_CheckHeap();
}

#else // _WIN32

#include "z_zone.h"  /* memory allocation wrappers -- killough */

#include <stdio.h>
#include <signal.h>
#include <allegro.h>
#include <dpmi.h>
#include <sys/nearptr.h>

#include "doomstat.h"
#include "v_video.h"
#include "d_main.h"

/////////////////////////////////////////////////////////////////////////////
//
// JOYSTICK                                                  // phares 4/3/98
//
/////////////////////////////////////////////////////////////////////////////

extern int usejoystick;
extern int joystickpresent;
extern int joy_x,joy_y;
extern int joy_b1,joy_b2,joy_b3,joy_b4;

void poll_joystick(void);

// I_JoystickEvents() gathers joystick data and creates an event_t for
// later processing by G_Responder().

void I_JoystickEvents()
{
  event_t event;

  if (!joystickpresent || !usejoystick)
    return;
  poll_joystick(); // Reads the current joystick settings
  event.type = ev_joystick;
  event.data1 = 0;

  // read the button settings

  if (joy_b1)
    event.data1 |= 1;
  if (joy_b2)
    event.data1 |= 2;
  if (joy_b3)
    event.data1 |= 4;
  if (joy_b4)
    event.data1 |= 8;

  // Read the x,y settings. Convert to -1 or 0 or +1.

  if (joy_x < 0)
    event.data2 = -1;
  else if (joy_x > 0)
    event.data2 = 1;
  else
    event.data2 = 0;
  if (joy_y < 0)
    event.data3 = -1;
  else if (joy_y > 0)
    event.data3 = 1;
  else
    event.data3 = 0;

  // post what you found

  D_PostEvent(&event);
}


//
// I_StartFrame
//
void I_StartFrame (void)
{
  I_JoystickEvents(); // Obtain joystick data                 phares 4/3/98
}

/////////////////////////////////////////////////////////////////////////////
//
// END JOYSTICK                                              // phares 4/3/98
//
/////////////////////////////////////////////////////////////////////////////

//
// Keyboard routines
// By Lee Killough
// Based only a little bit on Chi's v0.2 code
//

int I_ScanCode2DoomCode (int a)
{
  switch (a)
    {
    default:   return key_ascii_table[a]>8 ? key_ascii_table[a] : a+0x80;
    case 0x7b: return KEYD_PAUSE;
    case 0x0e: return KEYD_BACKSPACE;
    case 0x48: return KEYD_UPARROW;
    case 0x4d: return KEYD_RIGHTARROW;
    case 0x50: return KEYD_DOWNARROW;
    case 0x4b: return KEYD_LEFTARROW;
    case 0x38: return KEYD_LALT;
    case 0x79: return KEYD_RALT;
    case 0x1d:
    case 0x78: return KEYD_RCTRL;
    case 0x36:
    case 0x2a: return KEYD_RSHIFT;
  }
}

// Automatic caching inverter, so you don't need to maintain two tables.
// By Lee Killough

int I_DoomCode2ScanCode (int a)
{
  static int inverse[256], cache;
  for (;cache<256;cache++)
    inverse[I_ScanCode2DoomCode(cache)]=cache;
  return inverse[a];
}

// killough 3/22/98: rewritten to use interrupt-driven keyboard queue

void I_GetEvent()
{
  event_t event;
  int tail;

  while ((tail=keyboard_queue.tail) != keyboard_queue.head)
    {
      int k = keyboard_queue.queue[tail];
      keyboard_queue.tail = (tail+1) & (KQSIZE-1);
      event.type = k & 0x80 ? ev_keyup : ev_keydown;
      event.data1 = I_ScanCode2DoomCode(k & 0x7f);
      D_PostEvent(&event);
    }

  if (mousepresent!=-1)     /* mouse movement */
    {
      static int lastbuttons;
      int xmickeys,ymickeys,buttons=mouse_b;
      get_mouse_mickeys(&xmickeys,&ymickeys);
      if (xmickeys || ymickeys || buttons!=lastbuttons)
        {
          lastbuttons=buttons;
          event.data1=buttons;
          event.data3=-ymickeys;
          event.data2=xmickeys;
          event.type=ev_mouse;
          D_PostEvent(&event);
        }
    }
}

//
// I_StartTic
//

void I_StartTic()
{
  I_GetEvent();
}


//
// I_UpdateNoBlit
//

void I_UpdateNoBlit (void)
{
}

// 1/25/98 killough: faster blit for Pentium, PPro and PII CPUs:
extern void ppro_blit(void *, size_t);
extern void pent_blit(void *, size_t);

int use_vsync = 1;   // killough 2/8/98: controls whether vsync is called

static byte *dascreen;

void I_FinishUpdate(void)
{
  static int     lasttic;
  int            tics;
  int            i;
  extern H_boolean noblit;        // killough 1/31/98

  if (noblit)
    return;

    // draws little dots on the bottom of the screen
  if (devparm)
    {
      i = I_GetTime();
      tics = i - lasttic;
      lasttic = i;
      if (tics > 20)
        tics = 20;
      for (i=0 ; i<tics*2 ; i+=2)
        screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
      for ( ; i<20*2 ; i+=2)
        screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
    }

  //blast it to the screen

  // killough 2/7/98: unless -timedemo or -fastdemo is
  // selected, or the user does not want to use vsync,
  // use vsync() to prevent screen breaks.

  if (!timingdemo && use_vsync)
    vsync();

  // 1/16/98 killough: optimization based on CPU type

  if (cpu_family == 6)     // PPro, PII
    ppro_blit(dascreen,SCREENWIDTH*SCREENHEIGHT);
  else
    if (cpu_family == 5)   // Pentium
      pent_blit(dascreen,SCREENWIDTH*SCREENHEIGHT);
    else                   // Others
      memcpy(dascreen,screens[0],SCREENWIDTH*SCREENHEIGHT);
}

//
// I_ReadScreen
//

void I_ReadScreen (byte* scr)
{
  // 1/18/98 killough: optimized based on CPU type:

  if (cpu_family == 6)     // PPro or PII
    ppro_blit(scr,SCREENWIDTH*SCREENHEIGHT);
  else
    if (cpu_family == 5)   // Pentium
      pent_blit(scr,SCREENWIDTH*SCREENHEIGHT);
    else                     // Others
      memcpy(scr,screens[0],SCREENWIDTH*SCREENHEIGHT);
}

void I_SetPalette (byte *palette)
{
  int i;
  outportb(0x3c8,0);
  for (i=0;i<256;i++)
    {
      outportb(0x3c9,gammatable[usegamma][*palette++]>>2);
      outportb(0x3c9,gammatable[usegamma][*palette++]>>2);
      outportb(0x3c9,gammatable[usegamma][*palette++]>>2);
    }
}

void I_ShutdownGraphics(void)
{
  set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
}

void I_InitGraphics(void)
{
  static int firsttime=1;

  if (!firsttime)
    return;

  firsttime=0;

  check_cpu();    // 1/16/98 killough -- sets cpu_family based on CPU

#ifndef RANGECHECK
  asm("fninit");  // 1/16/98 killough -- prevents FPU exceptions
#endif

  timer_simulate_retrace(0);  // turn retrace simulator off -- killough 2/7/98

  //enter graphics mode

  // killough 2/7/98: use allegro set_gfx_mode

  if (!nodrawers) // killough 3/2/98: possibly avoid gfx mode
    {
      signal(SIGINT, SIG_IGN);  // ignore CTRL-C in graphics mode
      set_color_depth(8);
      set_gfx_mode(GFX_AUTODETECT, SCREENWIDTH, SCREENHEIGHT,
                   SCREENWIDTH, SCREENHEIGHT);
      atexit(I_ShutdownGraphics);
    }

  dascreen=(byte *)(__djgpp_conventional_base+0xa0000);
  screens[0]=(byte *)calloc(SCREENWIDTH,SCREENHEIGHT);  // killough
}

#endif //_WIN32

//----------------------------------------------------------------------------
//
// $Log: I_video.c,v $
// Revision 1.1  2000/04/09 18:15:46  proff_fs
// Initial revision
//
// Revision 1.12  1998/05/03  22:40:35  killough
// beautification
//
// Revision 1.11  1998/04/05  00:50:53  phares
// Joystick support, Main Menu re-ordering
//
// Revision 1.10  1998/03/23  03:16:10  killough
// Change to use interrupt-driver keyboard IO
//
// Revision 1.9  1998/03/09  07:13:35  killough
// Allow CTRL-BRK during game init
//
// Revision 1.8  1998/03/02  11:32:22  killough
// Add pentium blit case, make -nodraw work totally
//
// Revision 1.7  1998/02/23  04:29:09  killough
// BLIT tuning
//
// Revision 1.6  1998/02/09  03:01:20  killough
// Add vsync for flicker-free blits
//
// Revision 1.5  1998/02/03  01:33:01  stan
// Moved __djgpp_nearptr_enable() call from I_video.c to i_main.c
//
// Revision 1.4  1998/02/02  13:33:30  killough
// Add support for -noblit
//
// Revision 1.3  1998/01/26  19:23:31  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/26  05:59:14  killough
// New PPro blit routine
//
// Revision 1.1.1.1  1998/01/19  14:02:50  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
