// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: I_main.c,v 1.1 2000/04/09 18:14:41 proff_fs Exp $
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
//      Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: I_main.c,v 1.1 2000/04/09 18:14:41 proff_fs Exp $";

#ifdef _WIN32 // proff: includes for Windows

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <signal.h>
#ifdef GL_DOOM
#include "gl_struct.h"
#endif
#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"
#include "i_system.h"
#include "lprintf.h"  // jff 08/03/98 - declaration of lprintf

extern int Init_ConsoleWin(HINSTANCE hInstance);
extern int Init_Win();
#ifdef GL_DOOM
extern int Init_GLWin();
#endif
extern void FindResponseFile (void);

#else //_WIN32

#include "doomdef.h"
#include "m_argv.h"
#include "d_main.h"
#include "i_system.h"

#include <signal.h>
#include <sys/nearptr.h>  /* needed for __djgpp_nearptr_enable() -- stan */
#include <dpmi.h>
#include <allegro.h>
#include "lprintf.h"  // jff 08/03/98 - declaration of lprintf

#endif //_WIN32

// cleanup handling -- killough:
static void handler(int s)
{
  char buf[2048];

  signal(s,SIG_IGN);  // Ignore future instances of this signal.

  strcpy(buf,
         s==SIGSEGV ? "Segmentation Violation" :
         s==SIGINT  ? "Interrupted by User" :
         s==SIGILL  ? "Illegal Instruction" :
         s==SIGFPE  ? "Floating Point Exception" :
         s==SIGTERM ? "Killed" : "Terminated by signal");

  // If corrupted memory could cause crash, dump memory
  // allocation history, which points out probable causes

  if (s==SIGSEGV || s==SIGILL || s==SIGFPE)
    Z_DumpHistory(buf);

  I_Error(buf);
}

void I_Quit(void);

// proff 07/04/98: Added for CYGWIN32 compatibility
#if defined(_WIN32) && !defined(_MSC_VER)
#define MAX_ARGC 128
static char    *argvbuf[MAX_ARGC];
static char    *cmdLineBuffer;

static char ** commandLineToArgv(int *pArgc)
{
    char    *p, *pEnd;
    int     argc = 0;

    cmdLineBuffer=GetCommandLine();
    if (cmdLineBuffer == NULL)
        I_Error("No commandline!");

    p = cmdLineBuffer;
    pEnd = p + strlen(cmdLineBuffer);
    if (pEnd >= &cmdLineBuffer[1022]) pEnd = &cmdLineBuffer[1022];

    while (1) {
        while ((*p == ' ') && (p<pEnd)) p++;
        if (p >= pEnd) break;

        if (*p=='\"')
        {
            p++;
            if (p >= pEnd) break;
            argvbuf[argc++] = p;
            if (argc >= MAX_ARGC) break;
            while (*p && (*p != ' ') && (*p != '\"')) p++;
            if ((*p == ' ') | (*p == '\"')) *p++ = 0;
        }
        else
        {
            argvbuf[argc++] = p;
            if (argc >= MAX_ARGC) break;
            while (*p && (*p != ' ')) p++;
            if (*p == ' ') *p++ = 0;
        }
    }

    *pArgc = argc;
    return argvbuf;
}
#endif

#ifdef _WIN32  // proff: main-procedure for Windows
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
PSTR szCmdLine, int iCmdShow)
{
  int i;
  int rsp_found;
  char fname[PATH_MAX+1];

    Z_Init();                  // 1/18/98 killough: start up memory stuff first
    atexit(I_Quit);
    signal(SIGSEGV, handler);
    signal(SIGTERM, handler);
    signal(SIGILL,  handler);
    signal(SIGFPE,  handler);
    signal(SIGILL,  handler);
    signal(SIGINT,  handler);
    signal(SIGABRT, handler);
// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef _MSC_VER
    myargc = __argc; 
    myargv = __argv; 
#else
    myargv = commandLineToArgv(&myargc);
#endif
    if (M_CheckParm("-condump"))
    {
        freopen(strcat(strcpy(fname,D_DoomExeDir()),"disp.txt"),"w",stdout);
        freopen(strcat(strcpy(fname,D_DoomExeDir()),"error.txt"),"w",stderr);
    }
    do {
      rsp_found=false;
      for (i=0; i<myargc; i++)
        if (myargv[i][0]=='@')
          rsp_found=true;
      FindResponseFile();
    } while (rsp_found==true);
    Init_ConsoleWin(hInstance);
#ifdef GL_DOOM
    // proff 11/99: for OpenGL use Init_GLWin()
    if (!Init_GLWin())
      return 0;
#else
    // proff 11/99: for software use Init_Win()
    Init_Win();
#endif

    D_DoomMain (); 

    return 0;
}

#else //_WIN32

int main(int argc, char **argv)
{
  myargc = argc;
  myargv = argv;

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

  allegro_init();
  Z_Init();                  // 1/18/98 killough: start up memory stuff first
  atexit(I_Quit);
  signal(SIGSEGV, handler);
  signal(SIGTERM, handler);
  signal(SIGILL,  handler);
  signal(SIGFPE,  handler);
  signal(SIGILL,  handler);
  signal(SIGINT,  handler);  // killough 3/6/98: allow CTRL-BRK during init
  signal(SIGABRT, handler);

  // 2/2/98 Stan
  // Must call this here.  It's required by both netgames and i_video.c.

  if (__djgpp_nearptr_enable())  //handle nearptr now
    D_DoomMain ();
  else  //jff 8/3/98 use logical output routine
    lprintf (LO_FATAL,"Failed trying to allocate DOS near pointers.\n");

  return 0;
}

#endif //_WIN32

//----------------------------------------------------------------------------
//
// $Log: I_main.c,v $
// Revision 1.1  2000/04/09 18:14:41  proff_fs
// Initial revision
//
// Revision 1.9  1998/09/07  20:10:02  jim
// Logical output routine added
//
// Revision 1.8  1998/05/15  00:34:03  killough
// Remove unnecessary crash hack
//
// Revision 1.7  1998/05/13  22:58:04  killough
// Restore Doom bug compatibility for demos
//
// Revision 1.6  1998/05/03  22:38:36  killough
// beautification
//
// Revision 1.5  1998/04/27  02:03:11  killough
// Improve signal handling, to use Z_DumpHistory()
//
// Revision 1.4  1998/03/09  07:10:47  killough
// Allow CTRL-BRK during game init
//
// Revision 1.3  1998/02/03  01:32:58  stan
// Moved __djgpp_nearptr_enable() call from I_video.c to i_main.c
//
// Revision 1.2  1998/01/26  19:23:24  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:57  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
