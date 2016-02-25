/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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
 *      Startup and quit functions. Handles signals, inits the
 *      memory management, then calls D_DoomMain. Also contains
 *      I_Init which does other system-related startup stuff.
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef BOOL (WINAPI *SetAffinityFunc)(HANDLE hProcess, DWORD mask);
#else
#include <sched.h>
#endif

#include <errno.h>

#include "TEXTSCREEN/txt_main.h"

#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"
#include "m_fixed.h"
#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "lprintf.h"
#include "m_random.h"
#include "doomstat.h"
#include "g_game.h"
#include "m_misc.h"
#include "i_sound.h"
#include "i_main.h"
#include "r_fps.h"
#include "lprintf.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "e6y.h"

/* Most of the following has been rewritten by Lee Killough
 *
 * I_GetTime
 * killough 4/13/98: Make clock rate adjustable by scale factor
 * cphipps - much made static
 */

int realtic_clock_rate = 100;
static int_64_t I_GetTime_Scale = 1<<24;

static int I_GetTime_Scaled(void)
{
  return (int)( (int_64_t) I_GetTime_RealTime() * I_GetTime_Scale >> 24);
}



static int  I_GetTime_FastDemo(void)
{
  static int fasttic;
  return fasttic++;
}



static int I_GetTime_Error(void)
{
  I_Error("I_GetTime_Error: GetTime() used before initialization");
  return 0;
}



int (*I_GetTime)(void) = I_GetTime_Error;

void I_Init(void)
{
  /* killough 4/14/98: Adjustable speedup based on realtic_clock_rate */
  if (fastdemo)
    I_GetTime = I_GetTime_FastDemo;
  else
    if (realtic_clock_rate != 100)
      {
        I_GetTime_Scale = ((int_64_t) realtic_clock_rate << 24) / 100;
        I_GetTime = I_GetTime_Scaled;
      }
    else
      I_GetTime = I_GetTime_RealTime;

  {
    /* killough 2/21/98: avoid sound initialization if no sound & no music */
    if (!(nomusicparm && nosfxparm))
      I_InitSound();
  }

  R_InitInterpolation();
}

//e6y
void I_Init2(void)
{
  if (fastdemo)
    I_GetTime = I_GetTime_FastDemo;
  else
  {
    if (realtic_clock_rate != 100)
      {
        I_GetTime_Scale = ((int_64_t) realtic_clock_rate << 24) / 100;
        I_GetTime = I_GetTime_Scaled;
      }
    else
      I_GetTime = I_GetTime_RealTime;
  }
  R_InitInterpolation();
  force_singletics_to = gametic + BACKUPTICS;
}

/* cleanup handling -- killough:
 */
static void I_SignalHandler(int s)
{
  char buf[2048];

  signal(s,SIG_IGN);  /* Ignore future instances of this signal.*/

  I_ExeptionProcess(); //e6y

  strcpy(buf,"Exiting on signal: ");
  I_SigString(buf+strlen(buf),2000-strlen(buf),s);

  /* If corrupted memory could cause crash, dump memory
   * allocation history, which points out probable causes
   */
  if (s==SIGSEGV || s==SIGILL || s==SIGFPE)
    Z_DumpHistory(buf);

  I_Error("I_SignalHandler: %s", buf);
}

//
// e6y: exeptions handling
//

static ExeptionsList_t current_exception_index;

ExeptionParam_t ExeptionsParams[EXEPTION_MAX + 1] =
{
  {NULL},
  {"gld_CreateScreenSizeFBO: Access violation in glFramebufferTexture2DEXT.\n\n"
    "Are you using ATI graphics? Try to update your drivers "
    "or change gl_compatibility variable in cfg to 1.\n"},
  {NULL}
};

void I_ExeptionBegin(ExeptionsList_t exception_index)
{
  if (current_exception_index == EXEPTION_NONE)
  {
    current_exception_index = exception_index;
  }
  else
  {
    I_Error("I_SignalStateSet: signal_state set!");
  }
}

void I_ExeptionEnd(void)
{
  current_exception_index = EXEPTION_NONE;
}

void I_ExeptionProcess(void)
{
  if (current_exception_index > EXEPTION_NONE && current_exception_index < EXEPTION_MAX)
  {
    I_Error("%s", ExeptionsParams[current_exception_index].error_message);
  }
}


/* killough 2/22/98: Add support for ENDBOOM, which is PC-specific
 *
 * this converts BIOS color codes to ANSI codes.
 * Its not pretty, but it does the job - rain
 * CPhipps - made static
 */

inline static int convert(int color, int *bold)
{
  if (color > 7) {
    color -= 8;
    *bold = 1;
  }
  switch (color) {
  case 0:
    return 0;
  case 1:
    return 4;
  case 2:
    return 2;
  case 3:
    return 6;
  case 4:
    return 1;
  case 5:
    return 5;
  case 6:
    return 3;
  case 7:
    return 7;
  }
  return 0;
}

/* CPhipps - flags controlling ENDOOM behaviour */
enum {
  endoom_colours = 1,
  endoom_nonasciichars = 2,
  endoom_droplastline = 4
};

int endoom_mode;

static void PrintVer(void)
{
  char vbuf[200];
  lprintf(LO_INFO,"%s\n",I_GetVersionString(vbuf,200));
}

//
// ENDOOM support using text mode emulation
//
static void I_EndDoom(void)
{
  int lump_eb, lump_ed, lump = -1;

  const unsigned char *endoom_data;
  unsigned char *screendata;

#ifndef _WIN32
  PrintVer();
#endif

  if (!showendoom || demorecording)
  {
    return;
  }

  /* CPhipps - ENDOOM/ENDBOOM selection */
  lump_eb = W_CheckNumForName("ENDBOOM");/* jff 4/1/98 sign our work    */
  lump_ed = W_CheckNumForName("ENDOOM"); /* CPhipps - also maybe ENDOOM */

  if (lump_eb == -1)
    lump = lump_ed;
  else if (lump_ed == -1)
    lump = lump_eb;
  else
  { /* Both ENDOOM and ENDBOOM are present */
#define LUMP_IS_NEW(num) (!((lumpinfo[num].source == source_iwad) || (lumpinfo[num].source == source_auto_load)))
    switch ((LUMP_IS_NEW(lump_ed) ? 1 : 0 ) |
      (LUMP_IS_NEW(lump_eb) ? 2 : 0)) {
    case 1:
      lump = lump_ed;
      break;
    case 2:
      lump = lump_eb;
      break;
    default:
      /* Both lumps have equal priority, both present */
      lump = (P_Random(pr_misc) & 1) ? lump_ed : lump_eb;
      break;
    }
  }

  if (lump != -1)
  {
    endoom_data = W_CacheLumpNum(lump);
    
    // Set up text mode screen
    TXT_Init();
    
    // Make sure the new window has the right title and icon
    I_SetWindowCaption();
    I_SetWindowIcon();
    
    // Write the data to the screen memory
    screendata = TXT_GetScreenData();
    memcpy(screendata, endoom_data, 4000);
    
    // Wait for a keypress
    while (true)
    {
      TXT_UpdateScreen();
      
      if (TXT_GetChar() > 0)
      {
        break;
      }
      
      TXT_Sleep(0);
    }
    
    // Shut down text mode screen
    TXT_Shutdown();
  }
}

static int has_exited;

/* I_SafeExit
 * This function is called instead of exit() by functions that might be called
 * during the exit process (i.e. after exit() has already been called)
 * Prevent infinitely recursive exits -- killough
 */

void I_SafeExit(int rc)
{
  if (!has_exited)    /* If it hasn't exited yet, exit now -- killough */
    {
      has_exited=rc ? 2 : 1;
      exit(rc);
    }
}

static void I_Quit (void)
{
  if (!has_exited)
    has_exited=1;   /* Prevent infinitely recursive exits -- killough */

  if (has_exited == 1) {
    if (!demorecording)
      I_EndDoom();
    if (demorecording)
      G_CheckDemoStatus();
    M_SaveDefaults ();
    I_DemoExShutdown();
  }
}

#ifdef SECURE_UID
uid_t stored_euid = -1;
#endif

//
// Ability to use only the allowed CPUs
//

static void I_SetAffinityMask(void)
{
  // Forcing single core only for "SDL MIDI Player"
  process_affinity_mask = 0;
  if (!strcasecmp(snd_midiplayer, midiplayers[midi_player_sdl]))
  {
    process_affinity_mask = 1;
  }

  // Set the process affinity mask so that all threads
  // run on the same processor.  This is a workaround for a bug in 
  // SDL_mixer that causes occasional crashes.
  if (process_affinity_mask)
  {
    const char *errbuf = NULL;
#ifdef _WIN32
    HMODULE kernel32_dll;
    SetAffinityFunc SetAffinity = NULL;
    int ok = false;

    // Find the kernel interface DLL.
    kernel32_dll = LoadLibrary("kernel32.dll");

    if (kernel32_dll)
    {
      // Find the SetProcessAffinityMask function.
      SetAffinity = (SetAffinityFunc)GetProcAddress(kernel32_dll, "SetProcessAffinityMask");

      // If the function was not found, we are on an old (Win9x) system
      // that doesn't have this function.  That's no problem, because
      // those systems don't support SMP anyway.

      if (SetAffinity)
      {
        ok = SetAffinity(GetCurrentProcess(), process_affinity_mask);
      }
    }

    if (!ok)
    {
      errbuf = WINError();
    }
#elif defined(HAVE_SCHED_SETAFFINITY)
    // POSIX version:
    int i;
    {
      cpu_set_t set;

      CPU_ZERO(&set);

      for(i = 0; i < 16; i++)
      {
        CPU_SET((process_affinity_mask>>i)&1, &set);
      }

      if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
      {
        errbuf = strerror(errno);
      }
    }
#else
    return;
#endif

    if (errbuf == NULL)
    {
      lprintf(LO_INFO, "I_SetAffinityMask: manual affinity mask is %d\n", process_affinity_mask);
    }
    else
    {
      lprintf(LO_ERROR, "I_SetAffinityMask: failed to set process affinity mask (%s)\n", errbuf);
    }
  }
}

//
// Sets the priority class for the prboom-plus process
//

void I_SetProcessPriority(void)
{
  if (process_priority)
  {
    const char *errbuf = NULL;

#ifdef _WIN32
    {
      DWORD dwPriorityClass = NORMAL_PRIORITY_CLASS;

      if (process_priority == 1)
        dwPriorityClass = HIGH_PRIORITY_CLASS;
      else if (process_priority == 2)
        dwPriorityClass = REALTIME_PRIORITY_CLASS;

      if (SetPriorityClass(GetCurrentProcess(), dwPriorityClass) == 0)
      {
        errbuf = WINError();
      }
    }
#else
    return;
#endif

    if (errbuf == NULL)
    {
      lprintf(LO_INFO, "I_SetProcessPriority: priority for the process is %d\n", process_priority);
    }
    else
    {
      lprintf(LO_ERROR, "I_SetProcessPriority: failed to set priority for the process (%s)\n", errbuf);
    }
  }
}

//int main(int argc, const char * const * argv)
int main(int argc, char **argv)
{
#ifdef SECURE_UID
  /* First thing, revoke setuid status (if any) */
  stored_euid = geteuid();
  if (getuid() != stored_euid)
    if (seteuid(getuid()) < 0)
      fprintf(stderr, "Failed to revoke setuid\n");
    else
      fprintf(stderr, "Revoked uid %d\n",stored_euid);
#endif

  myargc = argc;
  myargv = malloc(sizeof(myargv[0]) * myargc);
  memcpy(myargv, argv, sizeof(myargv[0]) * myargc);

  // e6y: Check for conflicts.
  // Conflicting command-line parameters could cause the engine to be confused 
  // in some cases. Added checks to prevent this.
  // Example: glboom.exe -record mydemo -playdemo demoname
  ParamsMatchingCheck();

  // e6y: was moved from D_DoomMainSetup
  // init subsystems
  //jff 9/3/98 use logical output routine
  lprintf(LO_INFO,"M_LoadDefaults: Load system defaults.\n");
  M_LoadDefaults();              // load before initing other systems

  /* Version info */
  lprintf(LO_INFO,"\n");
  PrintVer();

  /* cph - Z_Close must be done after I_Quit, so we register it first. */
  atexit(Z_Close);
  /*
     killough 1/98:

     This fixes some problems with exit handling
     during abnormal situations.

     The old code called I_Quit() to end program,
     while now I_Quit() is installed as an exit
     handler and exit() is called to exit, either
     normally or abnormally. Seg faults are caught
     and the error handler is used, to prevent
     being left in graphics mode or having very
     loud SFX noise because the sound card is
     left in an unstable state.
  */

  Z_Init();                  /* 1/18/98 killough: start up memory stuff first */

  atexit(I_Quit);
#ifndef PRBOOM_DEBUG
  if (!M_CheckParm("-devparm"))
  {
    signal(SIGSEGV, I_SignalHandler);
  }
  signal(SIGTERM, I_SignalHandler);
  signal(SIGFPE,  I_SignalHandler);
  signal(SIGILL,  I_SignalHandler);
  signal(SIGINT,  I_SignalHandler);  /* killough 3/6/98: allow CTRL-BRK during init */
  signal(SIGABRT, I_SignalHandler);
#endif

  // Ability to use only the allowed CPUs
  I_SetAffinityMask();

  // Priority class for the prboom-plus process
  I_SetProcessPriority();

  /* cphipps - call to video specific startup code */
  I_PreInitGraphics();

  D_DoomMain ();
  return 0;
}
