// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: I_system.c,v 1.2 2000/04/26 20:00:02 proff_fs Exp $
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
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: I_system.c,v 1.2 2000/04/26 20:00:02 proff_fs Exp $";

#include <stdio.h>

#ifdef _WIN32 // proff: Includes for Windows
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef _MSC_VER
#include <mmsystem.h>
#include <time.h>
#else
DWORD STDCALL timeGetTime(void);
#endif
#else //_WIN32
#include <allegro.h>
extern void (*keyboard_lowlevel_callback)(int);  // should be in <allegro.h>
#include <stdarg.h>
#include <gppconio.h>
#endif //_WIN32

#include "i_system.h"
#include "i_sound.h"
#include "doomstat.h"
#include "m_misc.h"
#include "g_game.h"
#include "w_wad.h"
#include "lprintf.h"  // jff 08/03/98 - declaration of lprintf

#include "i_system.h"

ticcmd_t *I_BaseTiccmd(void)
{
  static ticcmd_t emptycmd; // killough
  return &emptycmd;
}

void I_WaitVBL(int count)
{
#ifdef _WIN32 // proff: Added Sleep-function
    // proff: Changed time-calculation
    Sleep((count*500)/TICRATE);
#elif DJGPP
    rest((count*500)/TICRATE);
#elif SGI // proff: Added functions from original Doom-source
    sginap(1);                                           
#elif SUN
    sleep(0);
#else
    usleep (count * (1000000/70) );                                
#endif
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

// Most of the following has been rewritten by Lee Killough
//
// I_GetTime
//

static volatile int realtic;

#ifdef DJGPP
void I_timer(void)
{
  realtic++;
}
END_OF_FUNCTION(I_timer);

int  I_GetTime_RealTime (void)
{
  return realtic;
}
#else //DJGPP

#ifdef _WIN32
//
// I_GetTrueTime
// returns time in 1/70th second tics
//
int  I_GetTrueTime (void)
{
// proff 08/15/98: Added QueryPerfomanceCounter and changed the function a bit
  static int            first=1;
  int                   tm;
  static int            basetime;
#ifdef _MSC_VER
  static int            QPC_Available=0;
  static LARGE_INTEGER  QPC_Freq;
  LARGE_INTEGER         QPC_tm;
#endif

  if (first==1)
  {
#ifdef _MSC_VER
    if (QueryPerformanceFrequency(&QPC_Freq))
    {
      QPC_Available=1;
      QueryPerformanceCounter(&QPC_tm);
      basetime=(int)(QPC_tm.QuadPart*1000/QPC_Freq.QuadPart);
      lprintf (LO_INFO, "Using QueryPerformanceCounter\n");
    }
    else
#endif
    {
      basetime=timeGetTime();
      lprintf (LO_INFO, "Using timeGetTime\n");
    }
    first=0;
  }
#ifdef _MSC_VER
  if (QPC_Available)
  {
    QueryPerformanceCounter(&QPC_tm);
    tm=(int)(QPC_tm.QuadPart*1000/QPC_Freq.QuadPart);
  }
  else
#endif
  {
    tm = timeGetTime();
  }
  return (tm-basetime);
}
#endif

//
// I_GetTime
// returns time in 1/70th second tics
//
int  I_GetTime_RealTime (void)
{
#ifdef _WIN32 // proff: Added function for Windows
// proff 10/31/98: Moved the core of this function to I_GetTrueTime
  realtic = (I_GetTrueTime()*TICRATE)/1000;
  return realtic;
#else //_WIN32
  struct      timeval  tp;
  struct      timezone  tzp;
  static int  basetime=0;

  gettimeofday(&tp, &tzp);
  if (!basetime)
  basetime = tp.tv_sec;
  realtic = (tp.tv_sec-basetime)*TICRATE + tp.tv_usec*TICRATE/1000000;
  return realtic;
#endif //_WIN32
}
#endif //DJGPP

// killough 4/13/98: Make clock rate adjustable by scale factor
int realtic_clock_rate = 100;
static longlong I_GetTime_Scale = 1<<24;
int I_GetTime_Scaled(void)
{
#ifndef _MSC_VER
  return (longlong) realtic * I_GetTime_Scale >> 24;
#else //_MSC_VER
  return (int)((longlong) I_GetTime_RealTime() * I_GetTime_Scale >> 24);
#endif //_MSC_VER
}

static int  I_GetTime_FastDemo(void)
{
  static int fasttic;
  return fasttic++;
}

static int I_GetTime_Error()
{
  I_Error("Error: GetTime() used before initialization");
  return 0;
}

int (*I_GetTime)() = I_GetTime_Error;                           // killough

// killough 3/21/98: Add keyboard queue

#ifndef _WIN32 // proff: This is provided by Windows, so we don't need it
struct keyboard_queue_s keyboard_queue;

static void keyboard_handler(int scancode)
{
  keyboard_queue.queue[keyboard_queue.head++] = scancode;
  keyboard_queue.head &= KQSIZE-1;
}
static END_OF_FUNCTION(keyboard_handler);

int mousepresent;
int joystickpresent;                                         // phares 4/3/98

static int orig_key_shifts;  // killough 3/6/98: original keyboard shift state
extern int autorun;          // Autorun state
#endif //_WIN32

int leds_always_off;         // Tells it not to update LEDs

void I_Shutdown(void)
{
#ifndef _WIN32 // proff: Not needed in Windows
  if (mousepresent!=-1)
    remove_mouse();

  // killough 3/6/98: restore keyboard shift state
  key_shifts = orig_key_shifts;

  remove_keyboard();

  remove_timer();
#endif //_WIN32
}

void I_Init(void)
{
#ifndef _WIN32 // proff: Not needed in Windows
  extern int key_autorun;

  //init timer
  LOCK_VARIABLE(realtic);
  LOCK_FUNCTION(I_timer);
  install_timer();
  install_int_ex(I_timer,BPS_TO_TIMER(TICRATE));
#endif //_WIN32

  // killough 4/14/98: Adjustable speedup based on realtic_clock_rate
  if (fastdemo)
    I_GetTime = I_GetTime_FastDemo;
  else
    if (realtic_clock_rate != 100)
      {
        I_GetTime_Scale = ((longlong) realtic_clock_rate << 24) / 100;
        I_GetTime = I_GetTime_Scaled;
      }
    else
      I_GetTime = I_GetTime_RealTime;

#ifndef _WIN32 // proff: Not needed in Windows
  // killough 3/21/98: Install handler to handle interrupt-driven keyboard IO
  LOCK_VARIABLE(keyboard_queue);
  LOCK_FUNCTION(keyboard_handler);
  keyboard_lowlevel_callback = keyboard_handler;

  install_keyboard();

  // killough 3/6/98: save keyboard state, initialize shift state and LEDs:

  orig_key_shifts = key_shifts;  // save keyboard state

  key_shifts = 0;        // turn off all shifts by default

  if (autorun)  // if autorun is on initially, turn on any corresponding shifts
    switch (key_autorun)
      {
      case KEYD_CAPSLOCK:
        key_shifts = KB_CAPSLOCK_FLAG;
        break;
      case KEYD_NUMLOCK:
        key_shifts = KB_NUMLOCK_FLAG;
        break;
      case KEYD_SCROLLLOCK:
        key_shifts = KB_SCROLOCK_FLAG;
        break;
      }

  // Either keep the keyboard LEDs off all the time, or update them
  // right now, and in the future, with respect to key_shifts flag.
  set_leds(leds_always_off ? 0 : -1);
  // killough 3/6/98: end of keyboard / autorun state changes

  //init the mouse
  mousepresent=install_mouse();
  if (mousepresent!=-1)
    show_mouse(NULL);

  // phares 4/3/98:
  // Init the joystick
  // For now, we'll require that joystick data is present in allegro.cfg.
  // The ASETUP program can be used to obtain the joystick data.

  if (load_joystick_data(NULL) == 0)
    joystickpresent = true;
  else
    joystickpresent = false;

#endif //_WIN32
  atexit(I_Shutdown);

  { // killough 2/21/98: avoid sound initialization if no sound & no music
    extern H_boolean nomusicparm, nosfxparm;
    if (!(nomusicparm && nosfxparm))
      I_InitSound();
  }
}

//
// I_Quit
//

static char errmsg[2048];    // buffer of error message -- killough

static int has_exited;

void I_Quit (void)
{
  has_exited=1;   /* Prevent infinitely recursive exits -- killough */

#ifdef _WIN32 // proff: Needed different order in Windows
  if (*errmsg)
  {
    //proff 9/17/98 use logical output routine
    lprintf (LO_ERROR, "%s\n", errmsg);
// proff 07/29/98: Changed MessageBox-Title to "PrBoom"
    MessageBox(NULL,errmsg,"PrBoom",0);
  }
  else
  {
    if (demorecording)
      G_CheckDemoStatus();
    M_SaveDefaults ();
    I_EndDoom();
    // proff: Changed to I_WaitVBL and made longer
    I_WaitVBL(140);
  }
#else //_WIN32
  if (demorecording)
    G_CheckDemoStatus();
  M_SaveDefaults ();

  if (*errmsg)
    //jff 8/3/98 use logical output routine
    lprintf (LO_ERROR, "%s\n", errmsg);
  else
    I_EndDoom();
#endif //_WIN32
}

//
// I_Error
//

void I_Error(const char *error, ...) // killough 3/20/98: add const
{
  if (!*errmsg)   // ignore all but the first message -- killough
    {
      va_list argptr;
      va_start(argptr,error);
      vsprintf(errmsg,error,argptr);
      va_end(argptr);
    }

  if (!has_exited)    // If it hasn't exited yet, exit now -- killough
    {
      has_exited=1;   // Prevent infinitely recursive exits -- killough
      exit(-1);
    }
}

// killough 2/22/98: Add support for ENDOOM, which is PC-specific
#ifdef _WIN32 // proff: Functions to access the console

extern int Init_ConsoleWin(void);

extern HWND con_hWnd;

void textattr(byte a)
{
  int r,g,b,col;
  HDC conDC;

  conDC=GetDC(con_hWnd);
  r=0; g=0; b=0;
  if (a & FOREGROUND_INTENSITY) col=255;
  else col=128;
  if (a & FOREGROUND_RED) r=col;
  if (a & FOREGROUND_GREEN) g=col;
  if (a & FOREGROUND_BLUE) b=col;
  SetTextColor(conDC, PALETTERGB(r,g,b));
  r=0; g=0; b=0;
  if (a & BACKGROUND_INTENSITY) col=255;
  else col=128;
  if (a & BACKGROUND_RED) r=col;
  if (a & BACKGROUND_GREEN) g=col;
  if (a & BACKGROUND_BLUE) b=col;
 	SetBkColor(conDC, PALETTERGB(r,g,b));
  ReleaseDC(con_hWnd,conDC);
}

#endif //_WIN32

void I_EndDoom(void)
{
  int lump = W_CheckNumForName("ENDOOM"); //jff 4/1/98 sign our work
#ifdef _WIN32
  Init_ConsoleWin();
#endif
  if (lump != -1)
    {
      const char (*endoom)[2] = W_CacheLumpNum(lump, PU_STATIC);
      int i, l = W_LumpLength(lump) / 2;
      for (i=0; i<l; i++)
        {
          textattr(endoom[i][1]);
#ifdef _WIN32
          lprintf(LO_ALWAYS,"%c",endoom[i][0]);
        }
//      lprintf(LO_ALWAYS,"\b");   // hack workaround for extra newline at bottom of screen
//      lprintf(LO_ALWAYS,"\r");
#else
          putch(endoom[i][0]);
        }
      putch('\b');   // hack workaround for extra newline at bottom of screen
      putch('\r');
#endif
    }
}

//----------------------------------------------------------------------------
//
// $Log: I_system.c,v $
// Revision 1.2  2000/04/26 20:00:02  proff_fs
// now using SDL for video and sound output.
// sound output is currently mono only.
// Get SDL from:
// http://www.devolution.com/~slouken/SDL/
//
// Revision 1.1.1.1  2000/04/09 18:00:32  proff_fs
// Initial login
//
// Revision 1.15  1998/09/07  20:06:44  jim
// Added logical output routine
//
// Revision 1.14  1998/05/03  22:33:13  killough
// beautification
//
// Revision 1.13  1998/04/27  01:51:37  killough
// Increase errmsg size to 2048
//
// Revision 1.12  1998/04/14  08:13:39  killough
// Replace adaptive gametics with realtic_clock_rate
//
// Revision 1.11  1998/04/10  06:33:46  killough
// Add adaptive gametic timer
//
// Revision 1.10  1998/04/05  00:51:06  phares
// Joystick support, Main Menu re-ordering
//
// Revision 1.9  1998/04/02  05:02:31  jim
// Added ENDOOM, BOOM.TXT mods
//
// Revision 1.8  1998/03/23  03:16:13  killough
// Change to use interrupt-driver keyboard IO
//
// Revision 1.7  1998/03/18  16:17:32  jim
// Change to avoid Allegro key shift handling bug
//
// Revision 1.6  1998/03/09  07:12:21  killough
// Fix capslock bugs
//
// Revision 1.5  1998/03/03  00:21:41  jim
// Added predefined ENDBETA lump for beta test
//
// Revision 1.4  1998/03/02  11:31:14  killough
// Fix ENDOOM message handling
//
// Revision 1.3  1998/02/23  04:28:14  killough
// Add ENDOOM support, allow no sound FX at all
//
// Revision 1.2  1998/01/26  19:23:29  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:07  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
