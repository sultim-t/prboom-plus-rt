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
//
//   General-purpose system-specific routines, including timer
//   installation, keyboard, mouse, and joystick code.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_system.c,v 1.14 1998/05/03 22:33:13 killough Exp $";

// Julian: 6/6/2001: replaced by local inclusion
#include "allegro.h"

extern void (*keyboard_lowlevel_callback)(int);  // should be in <allegro.h>
#include <gppconio.h>
#include <sys/nearptr.h>
#include "../z_zone.h"

#include "ser_main.h"

#include "../c_runcmd.h"
#include "../i_system.h"
#include "../i_sound.h"
#include "../i_video.h"
#include "../doomstat.h"
#include "../m_misc.h"
#include "../g_game.h"
#include "../w_wad.h"
#include "../v_video.h"
#include "../m_argv.h"
#include "../g_bind.h"

ticcmd_t *I_BaseTiccmd(void)
{
  static ticcmd_t emptycmd; // killough
  return &emptycmd;
}

void I_WaitVBL(int count)
{
  rest((count*500)/TICRATE);
}

// Most of the following has been rewritten by Lee Killough
//
// I_GetTime
//

static volatile int realtic;

void I_timer(void)
{
  realtic++;
}
END_OF_FUNCTION(I_timer);

int  I_GetTime_RealTime (void)
{
  return realtic;
}

void I_SetTime(int newtime)
{
  asm("cli");
  realtic = newtime;
  asm("sti");
}

        //sf: made a #define, changed to 16
#define CLOCK_BITS 16

// killough 4/13/98: Make clock rate adjustable by scale factor
int realtic_clock_rate = 100;
static long long I_GetTime_Scale = 1<<CLOCK_BITS;
int I_GetTime_Scaled(void)
{
  return (long long) realtic * I_GetTime_Scale >> CLOCK_BITS;
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

struct keyboard_queue_s keyboard_queue;

static void keyboard_handler(int scancode)
{
  keyboard_queue.queue[keyboard_queue.head++] = scancode;
  keyboard_queue.head &= KQSIZE-1;
}
static END_OF_FUNCTION(keyboard_handler);

int mousepresent;
int joystickpresent;                                         // phares 4/3/98

int keyboard_installed = 0;
static int orig_key_shifts;  // killough 3/6/98: original keyboard shift state
extern int autorun;          // Autorun state
int leds_always_off;         // Tells it not to update LEDs

void I_Shutdown(void)
{
  if (mousepresent!=-1)
    remove_mouse();

  // killough 3/6/98: restore keyboard shift state
  key_shifts = orig_key_shifts;

  remove_keyboard();
  keyboard_installed = false;

  remove_timer();
}

void I_ResetLEDs(void)
{
  // Either keep the keyboard LEDs off all the time, or update them
  // right now, and in the future, with respect to key_shifts flag.
  //
  // killough 10/98: moved to here

  set_leds(leds_always_off ? 0 : -1);
}

void I_InitKeyboard()
{
  // killough 3/21/98: Install handler to handle interrupt-driven keyboard IO
  LOCK_VARIABLE(keyboard_queue);
  LOCK_FUNCTION(keyboard_handler);
  keyboard_lowlevel_callback = keyboard_handler;

  install_keyboard();
  keyboard_installed = true;

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

  I_ResetLEDs();
}

void I_Init(void)
{
  int clock_rate = realtic_clock_rate, p;

  if ((p = M_CheckParm("-speed")) && p < myargc-1 &&
      (p = atoi(myargv[p+1])) >= 10 && p <= 1000)
    clock_rate = p;
    
  //init timer
  LOCK_VARIABLE(realtic);
  LOCK_FUNCTION(I_timer);
  install_timer();
  install_int_ex(I_timer,BPS_TO_TIMER(TICRATE));

  // killough 4/14/98: Adjustable speedup based on realtic_clock_rate
  if (fastdemo)
    I_GetTime = I_GetTime_FastDemo;
  else
    if (clock_rate != 100)
      {
        I_GetTime_Scale = ((long long) clock_rate << CLOCK_BITS) / 100;
        I_GetTime = I_GetTime_Scaled;
      }
    else
      I_GetTime = I_GetTime_RealTime;

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

  atexit(I_Shutdown);

  { // killough 2/21/98: avoid sound initialization if no sound & no music
    extern boolean nomusicparm, nosfxparm;
    if (!(nomusicparm && nosfxparm))
      I_InitSound();
  }

  // get modem cfg

  Ser_Init();
}

//
// I_Quit
//

static char errmsg[2048];    // buffer of error message -- killough

static int has_exited;

void I_Quit (void)
{
  has_exited = 1;   /* Prevent infinitely recursive exits -- killough */

  if (demorecording)
    G_CheckDemoStatus();

        // sf : rearrange this so the errmsg doesn't get messed up
  if (*errmsg)
    puts(errmsg);   // killough 8/8/98
  else
    I_EndDoom();

  M_SaveDefaults();
  G_SaveDefaults(); // haleyjd
}

//
// I_Error
//

void I_Error(const char *error, ...) // killough 3/20/98: add const
{
   if(!*errmsg)   // ignore all but the first message -- killough
   {
      va_list argptr;
      va_start(argptr, error);
      pvsnprintf(errmsg, sizeof(errmsg), error, argptr);
      va_end(argptr);
   }
   
   if(!has_exited)    // If it hasn't exited yet, exit now -- killough
   {
      has_exited = 1; // Prevent infinitely recursive exits -- killough
      exit(-1);
   }
}

void I_ErrorVA(const char *error, va_list args)
{
   if(!*errmsg)
      pvsnprintf(errmsg, sizeof(errmsg), error, args);

   if(!has_exited)
   {
      has_exited = 1;
      exit(-1);
   }
}

// killough 2/22/98: Add support for ENDBOOM, which is PC-specific
// killough 8/1/98: change back to ENDOOM

void I_EndDoom(void)
{
  int lump;

  if (lumpinfo && (lump = W_CheckNumForName("ENDOOM")) != -1) // killough 10/98
    {  // killough 8/19/98: simplify
      memcpy(0xb8000 + (byte *) __djgpp_conventional_base,
	     W_CacheLumpNum(lump, PU_CACHE), 0xf00);
      gotoxy(1,24);
    }
}

        // check for ESC button pressed, regardless of keyboard handler
int I_CheckAbort()
{
  if(keyboard_installed)
    {
      event_t *ev;
      
      I_StartTic ();       // build events
      
      // use the keyboard handler
      for ( ; eventtail != eventhead ; eventtail = (++eventtail)&(MAXEVENTS-1) )
	{ 
	  ev = &events[eventtail]; 
	  if (ev->type == ev_keydown && ev->data1 == key_escape)      // phares
	    {                   // abort
	      return true;
	    }
	}
    }
  else
    {
      // check normal keyboard handler
      while(kbhit()) if(getch() == 27) return true;
    }
  return false;
}

/*************************
        CONSOLE COMMANDS
 *************************/

VARIABLE_BOOLEAN(leds_always_off, NULL,     yesno);
VARIABLE_INT(realtic_clock_rate, NULL,  0, 500, NULL);

CONSOLE_VARIABLE(i_gamespeed, realtic_clock_rate, 0)
{
  if (realtic_clock_rate != 100)
    {
      I_GetTime_Scale = ((long long) realtic_clock_rate << CLOCK_BITS) / 100;
      I_GetTime = I_GetTime_Scaled;
    }
  else
    I_GetTime = I_GetTime_RealTime;
  
  ResetNet();         // reset the timers and stuff
}

CONSOLE_VARIABLE(i_ledsoff, leds_always_off, 0)
{
   I_ResetLEDs();
}

extern void I_Sound_AddCommands();
extern void I_Video_AddCommands();
extern void I_Input_AddCommands();
extern void Ser_AddCommands();

        // add system specific commands
void I_AddCommands()
{
  C_AddCommand(i_ledsoff);
  C_AddCommand(i_gamespeed);
  
  I_Video_AddCommands();
  I_Sound_AddCommands();
  Ser_AddCommands();
}

//----------------------------------------------------------------------------
//
// $Log: i_system.c,v $
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
