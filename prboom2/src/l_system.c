/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: l_system.c,v 1.1 2000/05/04 08:07:52 proff_fs Exp $
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
 *  Misc system stuff needed by Doom, implemented for Linux.
 *  Mainly timer handling, and ENDOOM/ENDBOOM.
 *
 *-----------------------------------------------------------------------------
 */

static const char
rcsid[] = "$Id: l_system.c,v 1.1 2000/05/04 08:07:52 proff_fs Exp $";

#include <stdio.h>

#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <ctype.h>
#include <signal.h>

#include "i_system.h"
#include "doomtype.h"
#include "doomdef.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"

void I_uSleep(unsigned long usecs)
{
  usleep(usecs);
}

/* CPhipps - believe it or not, it is possible with consecutive calls to 
 * gettimeofday to receive times out of order, e.g you query the time twice and 
 * the second time is earlier than the first. Cheap'n'cheerful fix here.
 * NOTE: only occurs with bad kernel drivers loaded, e.g. pc speaker drv
 */

static unsigned long lasttimereply;
static unsigned long basetime;

int I_GetTime_RealTime (void)
{
  struct timeval tv;
  struct timezone tz;
  unsigned long thistimereply;

  gettimeofday(&tv, &tz);

  thistimereply = (tv.tv_sec * TICRATE + (tv.tv_usec * TICRATE) / 1000000);

  /* Fix for time problem */
  if (!basetime) {
    basetime = thistimereply; thistimereply = 0;
  } else thistimereply -= basetime;

  if (thistimereply < lasttimereply)
    thistimereply = lasttimereply;

  return (lasttimereply = thistimereply);
}

/*
 * I_GetRandomTimeSeed
 *
 * CPhipps - extracted from G_ReloadDefaults because it is O/S based
 */
unsigned long I_GetRandomTimeSeed(void)
{                            
  /* killough 3/26/98: shuffle random seed, use the clock */ 
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv,&tz);
  return (tv.tv_sec*1000ul + tv.tv_usec/1000ul);
}

/* cphipps - I_GetVersionString
 * Returns a version string in the given buffer 
 */
const char* I_GetVersionString(char* buf, size_t sz)
{
  snprintf(buf,sz,"LxDoom v%s (http://lxdoom.linuxgames.com/)",VERSION);
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


/********************************************************************************************
 * $Log: l_system.c,v $
 * Revision 1.1  2000/05/04 08:07:52  proff_fs
 * Initial revision
 *
 * Revision 1.33  2000/02/26 19:18:23  cph
 * Be safe with version message
 *
 * Revision 1.32  1999/10/31 15:38:07  cphipps
 * Moved most of the LxDoom main program specific stuff to l_main.c
 * This file is now just for general system function wrappers, like
 * the time functions.
 * Added a function to return a signal name from a number, which uses
 * sys_siglist[] if configure told us it was available.
 * Added a function to return the LxDoom version, using the VERSION macro from
 * config.h.
 * Removed in-file RCS log, which mostly pertained to function now moved to
 * l_main.c
 *
 ********************************************************************************************/
