// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: Doomtype.h,v 1.1 2000/04/09 18:04:12 proff_fs Exp $
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
//      Simple basic typedefs, isolated here to make it easier
//       separating modules.
//
//-----------------------------------------------------------------------------


#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
// Fixed to use builtin bool type with C++.
#ifdef __cplusplus
typedef bool H_boolean;
#else
typedef enum {false, true} H_boolean;
#endif
typedef unsigned char byte;
#endif

#ifndef _WIN32 // proff: Visual C has no values.h so I have to add the values myself
#include <values.h>
#define MAXCHAR         ((char)0x7f)
#define MINCHAR         ((char)0x80)
#else // _WIN32

// proff 07/04/98: Added for CYGWIN32 compatibility
#ifdef _MSC_VER
#undef PATH_MAX
#define PATH_MAX 1024
#endif

#undef MAXCHAR
#define MAXCHAR         ((char)0x7f)
#undef MAXSHORT
#define MAXSHORT        ((short)0x7fff)
#undef MAXINT
#define MAXINT          ((int)0x7fffffff)       
#undef MAXLONG
#define MAXLONG         ((long)0x7fffffff)

#undef MINCHAR
#define MINCHAR         ((char)0x80)
#undef MINSHORT
#define MINSHORT        ((short)0x8000)
#undef MININT
#define MININT          ((int)0x80000000)       
#undef MINLONG
#define MINLONG         ((long)0x80000000)

#endif // _WIN32

#ifdef _MSC_VER
typedef __int64 longlong;
typedef unsigned __int64 ulonglong;
#else
typedef long long longlong;
typedef unsigned long long ulonglong;
#endif

#endif // __DOOMTYPE__

//----------------------------------------------------------------------------
//
// $Log: Doomtype.h,v $
// Revision 1.1  2000/04/09 18:04:12  proff_fs
// Initial revision
//
// Revision 1.3  1998/05/03  23:24:33  killough
// beautification
//
// Revision 1.2  1998/01/26  19:26:43  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:51  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
