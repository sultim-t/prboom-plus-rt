/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_system.c,v 1.1 2000/05/08 23:58:37 jessh Exp $
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
rcsid[] = "$Id: i_system.c,v 1.1 2000/05/08 23:58:37 jessh Exp $";

#include <stdio.h>

#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include "SDL.h"

#include "i_system.h"
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
/*  / killough 3/26/98: shuffle random seed, use the clock / 
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv,&tz);
  return (tv.tv_sec*1000ul + tv.tv_usec/1000ul);
*/
  return(SDL_GetTicks());
}

/* cphipps - I_GetVersionString
 * Returns a version string in the given buffer 
 */
const char* I_GetVersionString(char* buf, size_t sz)
{
//  snprintf(buf,sz,"LxDoom v%s (http://lxdoom.linuxgames.com/)",VERSION);
//  return buf;
  return NULL;
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
 * $Log: i_system.c,v $
 * Revision 1.1  2000/05/08 23:58:37  jessh
 * Moved SDL Target to src/SDL
 *
 * Revision 1.1.1.1  2000/05/04 08:07:52  proff_fs
 * initial login on sourceforge as prboom2
 *
 * Revision 1.2  2000/05/01 17:50:35  Proff
 * made changes to compile with VisualC and SDL
 *
 * Revision 1.1  2000/05/01 16:18:41  Proff
 * initial revision
 *
 *
 ********************************************************************************************/
