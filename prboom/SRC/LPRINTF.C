// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: LPRINTF.C,v 1.1 2000/04/09 18:18:56 proff_fs Exp $
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
//  Provides a logical console output routine that allows what is
//  output to console normally and when output is redirected to
//  be controlled..
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: LPRINTF.C,v 1.1 2000/04/09 18:18:56 proff_fs Exp $";
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifdef _MSC_VER //proff
#include <direct.h>
#include <io.h>
#else //_MSC_VER
#include <unistd.h>
#endif //_MSC_VER
#ifdef _WIN32
#include "winstuff.h"
#endif //_WIN32
#include "lprintf.h"

int cons_error_mask = -1-LO_INFO; // all but LO_INFO when redir'd
int cons_output_mask = -1;        // all output enabled

#define MAX_MESSAGE_SIZE 1024

int lprintf(OutputLevels pri, const char *s, ...)
{
  int r=0;
  static char msg[MAX_MESSAGE_SIZE];
  int lvl=pri;

  va_list v;
  va_start(v,s);
  vsprintf(msg,s,v);                      // print message in buffer
  va_end(v);

  if (lvl&cons_output_mask)               // mask output as specified
    r=fprintf(stdout,"%s",msg);
  if (!isatty(1) && lvl&cons_error_mask)  // if stdout redirected 
    r=fprintf(stderr,"%s",msg);           // select output at console

#ifdef _WIN32
  if (lvl&cons_output_mask)               // mask output as specified
    I_ConPrintString(msg);
#endif // _MSC_VER

  return r;
}

//----------------------------------------------------------------------------
//
// $Log: LPRINTF.C,v $
// Revision 1.1  2000/04/09 18:18:56  proff_fs
// Initial revision
//
// Revision 1.2  1998/09/14  18:49:49  jim
// fix log comments
//
// Revision 1.1  1998/09/07  20:10:56  jim
// Logical output routine added
//
//
//----------------------------------------------------------------------------

