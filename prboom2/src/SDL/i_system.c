/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
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
 *  Misc system stuff needed by Doom, implemented for Linux.
 *  Mainly timer handling, and ENDOOM/ENDBOOM.
 *
 *-----------------------------------------------------------------------------
 */

static const char

#include <stdio.h>

#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#ifdef _MSC_VER
#define    F_OK    0    /* Check for file existence */
#define    W_OK    2    /* Check for write permission */
#define    R_OK    4    /* Check for read permission */
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>

#include "SDL.h"

#include "i_system.h"
#include "m_argv.h"
#include "lprintf.h"
#include "doomtype.h"
#include "doomdef.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

void I_uSleep(unsigned long usecs)
{
    SDL_Delay(usecs/1000);
}

int I_GetTime_RealTime (void)
{
  return (SDL_GetTicks()*TICRATE)/1000;
}

/*
 * I_GetRandomTimeSeed
 *
 * CPhipps - extracted from G_ReloadDefaults because it is O/S based
 */
unsigned long I_GetRandomTimeSeed(void)
{
/* This isnt very random */
  return(SDL_GetTicks());
}

/* cphipps - I_GetVersionString
 * Returns a version string in the given buffer
 */
const char* I_GetVersionString(char* buf, size_t sz)
{
#ifdef HAVE_SNPRINTF
  snprintf(buf,sz,"%s v%s (http://prboom.sourceforge.net/)",PACKAGE,VERSION);
#else
  sprintf(buf,"%s v%s (http://prboom.sourceforge.net/)",PACKAGE,VERSION);
#endif
  return buf;
}

/* cphipps - I_SigString
 * Returns a string describing a signal number
 */
const char* I_SigString(char* buf, size_t sz, int signum)
{
#ifdef SYS_SIGLIST_DECLARED
  if (strlen(sys_siglist[signum]) < sz)
    strcpy(buf,sys_siglist[signum]);
  else
#endif
    sprintf(buf,"signal %d",signum);
  return buf;
}

#ifndef PRBOOM_SERVER

// Return the path where the executable lies -- Lee Killough
// proff_fs 2002-07-04 - moved to i_system
#ifdef _WIN32
char *I_DoomExeDir(void)
{
  static const char current_dir_dummy[] = {"./"};
  static char *base;
  if (!base)        // cache multiple requests
    {
      size_t len = strlen(*myargv);
      char *p = (base = malloc(len+1)) + len - 1;
      strcpy(base,*myargv);
      while (p > base && *p!='/' && *p!='\\')
        *p--=0;
      if (*p=='/' || *p=='\\')
        *p--=0;
      if (strlen(base)<2)
      {
        free(base);
        base = malloc(1024);
        if (!getcwd(base,1024))
          strcpy(base, current_dir_dummy);
      }
    }
  return base;
}
#else
// cph - V.Aguilar (5/30/99) suggested return ~/.lxdoom/, creating
//  if non-existant
static const char prboom_dir[] = {"/.prboom/"};

char *I_DoomExeDir(void)
{
  static char *base;
  if (!base)        // cache multiple requests
    {
      char *home = getenv("HOME");
      size_t len = strlen(home);

      base = malloc(len + strlen(prboom_dir) + 1);
      strcpy(base, home);
      // I've had trouble with trailing slashes before...
      if (base[len-1] == '/') base[len-1] = 0;
      strcat(base, prboom_dir);
      mkdir(base, S_IRUSR | S_IWUSR | S_IXUSR); // Make sure it exists
    }
  return base;
}
#endif

/*
 * HasTrailingSlash
 *
 * cphipps - simple test for trailing slash on dir names
 */

static boolean HasTrailingSlash(const char* dn)
{
  return (dn[strlen(dn)-1] == '/');
}

/*
 * I_FindFile
 *
 * proff_fs 2002-07-04 - moved to i_system
 *
 * cphipps 19/1999 - writen to unify the logic in FindIWADFile and the WAD
 *      autoloading code.
 * Searches the standard dirs for a named WAD file
 * The dirs are:
 * .
 * DOOMWADDIR
 * ~/doom
 * /usr/share/games/doom
 * /usr/local/share/games/doom
 * ~
 */

char* I_FindFile(const char* wfname, const char* ext)
{
  int   i;
  /* Precalculate a length we will need in the loop */
  size_t  pl = strlen(wfname) + strlen(ext) + 4;

  for (i=0; i<8; i++) {
    char  * p;
    const char  * d = NULL;
    const char  * s = NULL;
    /* Each entry in the switch sets d to the directory to look in,
     * and optionally s to a subdirectory of d */
    switch(i) {
    case 1:
      if (!(d = getenv("DOOMWADDIR"))) continue;
    case 0:
      break;
    case 2:
      d = DOOMWADDIR;
      break;
    case 4:
      d = "/usr/share/games/doom";
      break;
    case 5:
      d = "/usr/local/share/games/doom";
      break;
    case 6:
      d = I_DoomExeDir();
      break;
    case 3:
      s = "doom";
    case 7:
      if (!(d = getenv("HOME"))) continue;
      break;
#ifdef SIMPLECHECKS
    default:
      I_Error("FindWADFile: Internal failure");
#endif
    }

    p = malloc((d ? strlen(d) : 0) + (s ? strlen(s) : 0) + pl);
    sprintf(p, "%s%s%s%s%s", d ? d : "", (d && !HasTrailingSlash(d)) ? "/" : "",
                             s ? s : "", (s && !HasTrailingSlash(s)) ? "/" : "",
                             wfname);

    if (access(p,F_OK))
      strcat(p, ext);
    if (!access(p,F_OK)) {
      lprintf(LO_INFO, " found %s\n", p);
      return p;
    }
    free(p);
  }
  return NULL;
}

#endif // PRBOOM_SERVER
