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
 *  Some argument handling.
 *
 *-----------------------------------------------------------------------------*/

#include <string.h>
// CPhipps - include the correct header
#include "doomtype.h"
#include "z_zone.h"
#include "m_argv.h"

int    myargc;
char **myargv;

//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present
//

int M_CheckParm(const char *check)
{
  signed int i = myargc;
  while (--i>0)
    if (!strcasecmp(check, myargv[i]))
      return i;
  return 0;
}

//
// M_CheckParmEx
// Checks for the given parameter in the given params list.
// Returns the argument number (0 to paramscount-1) or -1 if not present
//

int M_CheckParmEx(const char *check, char **params, int paramscount)
{
  if (paramscount > 0 && check && params && *params)
  {
    while (--paramscount >= 0)
    {
      if (!strcasecmp(check, params[paramscount]))
      {
        return paramscount;
      }
    }
  }

  return -1;
}

//
// Add one parameter to myargv list
//

void M_AddParam(const char *param)
{
  myargv = realloc(myargv, sizeof(myargv[0]) * (myargc + 1));
  myargv[myargc] = strdup(param);
  myargc++;
}

//
// M_ParseCmdLine
//
// Parses the command line and sets up the argv[] array.
// On entry, cmdstart should point to the command line,
// argv should point to memory for the argv array, args
// points to memory to place the text of the arguments.
// If these are NULL, then no storing (only counting)
// is done.  On exit, *numargs has the number of
// arguments (plus one for a final NULL argument),
// and *numchars has the number of bytes used in the buffer
// pointed to by args.
//
// Entry:
//  char *cmdstart - pointer to command line
//  char **argv - where to build argv array; NULL means don't build array
//  char *args - where to place argument text; NULL means don't store text
//
// Exit:
//  no return value
//  int *numargs - returns number of argv entries created
//  int *numchars - number of characters used in args buffer
//

void M_ParseCmdLine(char *cmdstart, char **argv, char *args, int *numargs, int *numchars)
{
#define IsSpace(c) ((c) == 0x20 || ((c) >= 0x09 && (c) <= 0x0D))

  char *p;
  int inquote;                    /* 1 = inside quotes */
  int copychar;                   /* 1 = copy char to *args */
  unsigned numslash;              /* num of backslashes seen */

  *numchars = 0;
  *numargs = 0;

  p = cmdstart;

  inquote = 0;

  /* loop on each argument */
  for(;;) {

    while (IsSpace((int)*p)) ++p;

    if (*p == '\0')break;   /* end of args */

    /* scan an argument */
    if (argv) *argv++ = args;     /* store ptr to arg */
    ++*numargs;

    /* loop through scanning one argument */
    for (;;) {
      copychar = 1;
      /* Rules: 2N backslashes + " ==> N backslashes and begin/end quote
         2N+1 backslashes + " ==> N backslashes + literal "
         N backslashes ==> N backslashes */
      numslash = 0;
      while (*p == '\\') { /* count number of backslashes for use below */
        ++p;
        ++numslash;
      }
      if (*p == '\"') {
      /* if 2N backslashes before, start/end quote, otherwise
        copy literally */
        if ((numslash & 1) == 0) {
          if (inquote) {
            if (p[1] == '\"')
              p++;    /* Double quote inside quoted string */
            else        /* skip first quote char and copy second */
              copychar = 0;
          } else copychar = 0;       /* don't copy quote */
          inquote = !inquote;
        }
        numslash >>= 1;             /* divide numslash by two */
      }

      /* copy slashes */
      while (numslash--) {
        if (args) *args++ = '\\';
        ++*numchars;
      }

      /* if at end of arg, break loop */
      if (*p == '\0' || (!inquote && IsSpace((int)*p))) break;

      /* copy character into argument */
      if (copychar) {
        if (args) *args++ = *p;
        ++*numchars;
      }
      ++p;
    }

    /* null-terminate the argument */

    if (args) *args++ = '\0';          /* terminate string */
    ++*numchars;
  }
}
