/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: lprintf.c,v 1.1 2000/05/04 08:08:40 proff_fs Exp $
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
 *  Provides a logical console output routine that allows what is
 *  output to console normally and when output is redirected to
 *  be controlled..
 *
 *-----------------------------------------------------------------------------*/

static const char rcsid[] = "$Id: lprintf.c,v 1.1 2000/05/04 08:08:40 proff_fs Exp $";
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef HAVE_UNISTD
#include <unistd.h>
#endif
#include "lprintf.h"
#include "i_main.h"

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

int cons_error_mask = -1-LO_INFO; /* all but LO_INFO when redir'd */
int cons_output_mask = -1;        /* all output enabled */

/* cphipps - enlarged message buffer and made non-static 
 * We still have to be careful here, this function can be called after exit
 */
#define MAX_MESSAGE_SIZE 2048

int lprintf(OutputLevels pri, const char *s, ...)
{
  int r=0;
  char msg[MAX_MESSAGE_SIZE];
  int lvl=pri;

  va_list v;
  va_start(v,s);
#ifdef HAVE_vsnprintf
  vsnprintf(msg,sizeof(msg),s,v);         /* print message in buffer  */
#else 
  vsprintf(msg,s,v);
#endif
  va_end(v);

  if (lvl&cons_output_mask)               /* mask output as specified */
    r=fprintf(stdout,"%s",msg);
  if (!isatty(1) && lvl&cons_error_mask)  /* if stdout redirected     */
    r=fprintf(stderr,"%s",msg);           /* select output at console */

  return r;
}

/*
 * I_Error
 *
 * cphipps - moved out of i_* headers, to minimise source files that depend on
 * the low-level headers. All this does is print the error, then call the 
 * low-level safe exit function.
 * killough 3/20/98: add const
 */

void I_Error(const char *error, ...)
{
  char errmsg[MAX_MESSAGE_SIZE];
  va_list argptr;
  va_start(argptr,error);
#ifdef HAVE_vsnprintf
  vsnprintf(errmsg,sizeof(errmsg),error,argptr);
#else
  vsprintf(errmsg,error,argptr);
#endif
  va_end(argptr);
  fprintf(stderr,"%s\n",errmsg);
#ifdef _MSC_VER
  MessageBox(NULL,errmsg,"PrBoom",0);
#endif

  I_SafeExit(-1);
}

/*----------------------------------------------------------------------------
 *
 * $Log: lprintf.c,v $
 * Revision 1.1  2000/05/04 08:08:40  proff_fs
 * Initial revision
 *
 * Revision 1.5  2000/05/01 17:50:35  Proff
 * made changes to compile with VisualC and SDL
 *
 * Revision 1.4  2000/02/26 19:17:54  cph
 * Prevent buffer overflows
 *
 * Revision 1.3  1999/10/31 12:54:09  cphipps
 * Moved I_Error to lprintf.c
 * Finished off C++ comments
 *
 * Revision 1.2  1999/10/12 13:01:12  cphipps
 * Changed header to GPL
 *
 * Revision 1.1  1998/10/27 20:51:53  cphipps
 * Initial revision
 *
 * Revision 1.2  1998/09/14  18:49:49  jim
 * fix log comments
 *
 * Revision 1.1  1998/09/07  20:10:56  jim
 * Logical output routine added
 *
 *
 *----------------------------------------------------------------------------*/

