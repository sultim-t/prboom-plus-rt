/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: doomtype.h,v 1.1 2000/05/04 08:01:10 proff_fs Exp $
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
 *      Simple basic typedefs, isolated here to make it easier
 *       separating modules.
 *
 *-----------------------------------------------------------------------------*/

#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
/* Fixed to use builtin bool type with C++. */
#ifdef __cplusplus
typedef bool boolean;
#else
typedef enum {false, true} boolean;
#endif
typedef unsigned char byte;
#endif

/* cph - Wrapper for the long long type, as Win32 used a different name.
 * Except I don't know what to test as it's compiler specific
 * Proff - I fixed it */
#ifndef _MSC_VER
typedef signed long long int_64_t; 
typedef unsigned long long uint_64_t; 
#else
typedef __int64 int_64_t;
typedef unsigned __int64 uint_64_t;
#undef PATH_MAX
#define PATH_MAX 1024
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define S_ISDIR(x) (((sbuf.st_mode & S_IFDIR)==S_IFDIR)?1:0)
#endif

/* CPhipps - use limits.h instead of depreciated values.h */
#include <limits.h>
#endif

/*----------------------------------------------------------------------------
 *
 * $Log: doomtype.h,v $
 * Revision 1.1  2000/05/04 08:01:10  proff_fs
 * Initial revision
 *
 * Revision 1.6  2000/05/01 17:50:33  Proff
 * made changes to compile with VisualC and SDL
 *
 * Revision 1.5  2000/04/26 12:59:06  cph
 * *** empty log message ***
 *
 * Revision 1.4  1999/10/12 13:00:56  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.3  1999/06/08 17:29:15  cphipps
 * Add typedefs for int_64_t types, to abstract 64 bit ints from the particular
 *  compiler type
 *
 * Revision 1.2  1999/01/25 22:44:51  cphipps
 * Use limits.h instead of depreciated values.h to get integer limit macros
 *
 * Revision 1.1  1998/09/13 16:49:50  cphipps
 * Initial revision
 *
 * Revision 1.3  1998/05/03  23:24:33  killough
 * beautification
 *
 * Revision 1.2  1998/01/26  19:26:43  phares
 * First rev with no ^Ms
 *
 * Revision 1.1.1.1  1998/01/19  14:02:51  rand
 * Lee's Jan 19 sources
 *
 *----------------------------------------------------------------------------*/
