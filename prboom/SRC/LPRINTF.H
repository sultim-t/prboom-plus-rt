// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: LPRINTF.H,v 1.1 2000/04/09 18:18:49 proff_fs Exp $
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
//    Declarations etc. for logical console output
//
//-----------------------------------------------------------------------------

#ifndef __LPRINTF__
#define __LPRINTF__

typedef enum                // Logical output levels
{
  LO_INFO=1,                // One of these is used in each physical output
  LO_CONFIRM=2,             // call. Which are output, or echoed to console
  LO_WARN=4,                // if output redirected is determined by the
  LO_ERROR=8,               // global masks: cons_output_mask,cons_error_mask.
  LO_FATAL=16,
  LO_DEBUG=32,
  LO_ALWAYS=64,
} OutputLevels;

#ifndef __GNUC__
#define __attribute__(x)
#endif

extern int lprintf(OutputLevels pri, const char *fmt, ...) __attribute__((format(printf,2,3)));

extern int cons_output_mask;
extern int cons_error_mask;

#endif

//----------------------------------------------------------------------------
//
// $Log: LPRINTF.H,v $
// Revision 1.1  2000/04/09 18:18:49  proff_fs
// Initial revision
//
// Revision 1.2  1998/09/09  21:21:43  phares
// Added Log line
//
//
//----------------------------------------------------------------------------
