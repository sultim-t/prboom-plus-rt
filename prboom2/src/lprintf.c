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
 *  Provides a logical console output routine that allows what is
 *  output to console normally and when output is redirected to
 *  be controlled..
 *
 *-----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef _MSC_VER
#include <io.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "doomtype.h"
#include "lprintf.h"
#include "i_main.h"
#include "i_system.h"
#include "m_argv.h"
#include "e6y.h"//e6y

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
  doom_vsnprintf(msg,sizeof(msg),s,v);    /* print message in buffer  */
  va_end(v);

  if (lvl&cons_output_mask)               /* mask output as specified */
  {
#ifdef _WIN32
    // do not crash with unicode dirs
    if (fileno(stdout) != -1)
#endif
    r=fprintf(stdout,"%s",msg);
  }
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
  doom_vsnprintf(errmsg,sizeof(errmsg),error,argptr);
  va_end(argptr);
  lprintf(LO_ERROR, "%s\n", errmsg);
#ifdef _MSC_VER
  if (!M_CheckParm ("-nodraw")) {
    I_MessageBox(errmsg, PRB_MB_OK);
  }
#endif
  I_SafeExit(-1);
}

// Attempt to compensate for lack of va_copy

#ifndef va_copy
# ifdef __va_copy
#  define va_copy(a,b) __va_copy(a,b)
# else
#  define va_copy(a,b) ((a)=(b))
# endif
#endif

// Wrapper to handle non-standard stdio implementations

int doom_vsnprintf(char *buf, size_t max, const char *fmt, va_list va)
{
  int rv;
  va_list vc;

  assert((max == 0 && buf == NULL) || (max != 0 && buf != NULL));
  assert(fmt != NULL);

  va_copy(vc, va);
  rv = vsnprintf(buf, max, fmt, vc);
  va_end(vc);

  if (rv < 0) // Handle an unhelpful return value.
  {
    // write into a scratch buffer that keeps growing until the output fits
    static char *backbuffer;
    static size_t backsize = 1024;

    for (; rv < 0; backsize *= 2)
    {
      if (backsize <= max) continue;

      backbuffer = (realloc)(backbuffer, backsize);
      assert(backbuffer != NULL);

      va_copy(vc, va);
      rv = vsnprintf(backbuffer, backsize, fmt, vc);
      va_end(vc);
    }

    if (buf)
    {
      size_t end = (size_t) rv >= max ? max-1 : rv;
      memmove(buf, backbuffer, end);
      buf[end] = '\0';
    }
  }

  if (buf && (size_t) rv >= max && buf[max-1]) // ensure null-termination
    buf[max-1] = '\0';

  return rv;
}

int doom_snprintf(char *buf, size_t max, const char *fmt, ...)
{
  int rv;
  va_list va;

  va_start(va, fmt);
  rv = doom_vsnprintf(buf, max, fmt, va);
  va_end(va);

  return rv;
}
