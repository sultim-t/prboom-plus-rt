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
 *      General system functions. Signal related stuff, exit function
 *      prototypes, and programmable Doom clock.
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __I_MAIN__
#define __I_MAIN__

//
// e6y: exeptions handling
//

typedef enum
{
  EXEPTION_NONE,
  EXEPTION_glFramebufferTexture2DEXT,
  EXEPTION_MAX
} ExeptionsList_t;

typedef struct
{
  const char * error_message;
} ExeptionParam_t;

extern ExeptionParam_t ExeptionsParams[];

void I_ExeptionBegin(ExeptionsList_t exception_index);
void I_ExeptionEnd(void);
void I_ExeptionProcess(void);

#if defined(_WIN32) && (defined(_MSC_VER) || defined(__INTEL_COMPILER))
void I_Warning(const char *message, ...);
#define PRBOOM_TRY(exception_index) __try
#define PRBOOM_EXCEPT(exception_index) __except(EXCEPTION_EXECUTE_HANDLER) { I_Warning("%s", ExeptionsParams[exception_index]); }
#else
#define PRBOOM_TRY(exception_index) I_ExeptionBegin(exception_index);
#define PRBOOM_EXCEPT(exception_index) I_ExeptionEnd();
#endif

void I_Init(void);
void I_SafeExit(int rc);

extern int (*I_GetTime)(void);

#endif
