// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//      Zone Memory Allocation, perhaps NeXT ObjectiveC inspired.
//      Remark: this was the only stuff that, according
//       to John Carmack, might have been useful for
//       Quake.
//
// Rewritten by Lee Killough, though, since it was not efficient enough.
//
//---------------------------------------------------------------------

#ifndef __Z_ZONE__
#define __Z_ZONE__

#include "d_keywds.h" // haleyjd 05/22/02

// Remove all definitions before including system definitions

#undef malloc
#undef free
#undef realloc
#undef calloc
#undef strdup

// Include system definitions so that prototypes become
// active before macro replacements below are in effect.

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "psnprntf.h"

// ZONE MEMORY
// PU - purge tags.

enum {PU_FREE, PU_STATIC, PU_SOUND, PU_MUSIC, PU_LEVEL, PU_LEVSPEC, PU_CACHE,
      /* Must always be last -- killough */ PU_MAX};

#define PU_PURGELEVEL PU_CACHE        /* First purgable tag's level */

void *(Z_Malloc)(size_t size, int tag, void **ptr, const char *, int);
void (Z_Free)(void *ptr, const char *, int);
void (Z_FreeTags)(int lowtag, int hightag, const char *, int);
void (Z_ChangeTag)(void *ptr, int tag, const char *, int);
void (Z_Init)(void);
void *(Z_Calloc)(size_t n, size_t n2, int tag, void **user, const char *, int);
void *(Z_Realloc)(void *p, size_t n, int tag, void **user, const char *, int);
char *(Z_Strdup)(const char *s, int tag, void **user, const char *, int);
void (Z_CheckHeap)(const char *,int);   // killough 3/22/98: add file/line info
int (Z_CheckTag)(void *,const char *,int);
void Z_DumpHistory(char *);

#define Z_Free(a)          (Z_Free)     (a,      __FILE__,__LINE__)
#define Z_FreeTags(a,b)    (Z_FreeTags) (a,b,    __FILE__,__LINE__)
#define Z_ChangeTag(a,b)   (Z_ChangeTag)(a,b,    __FILE__,__LINE__)
#define Z_Malloc(a,b,c)    (Z_Malloc)   (a,b,c,  __FILE__,__LINE__)
#define Z_Strdup(a,b,c)    (Z_Strdup)   (a,b,c,  __FILE__,__LINE__)
#define Z_Calloc(a,b,c,d)  (Z_Calloc)   (a,b,c,d,__FILE__,__LINE__)
#define Z_Realloc(a,b,c,d) (Z_Realloc)  (a,b,c,d,__FILE__,__LINE__)
#define Z_CheckHeap()      (Z_CheckHeap)(__FILE__,__LINE__)
#define Z_CheckTag(a)      (Z_CheckTag) (a,__FILE__,__LINE__)

#define malloc(n)          (Z_Malloc) (n,    PU_STATIC,0,__FILE__,__LINE__)
#define free(p)            (Z_Free)   (p,                __FILE__,__LINE__)
#define realloc(p,n)       (Z_Realloc)(p,n,  PU_STATIC,0,__FILE__,__LINE__)
#define calloc(n1,n2)      (Z_Calloc) (n1,n2,PU_STATIC,0,__FILE__,__LINE__)
#define strdup(s)          (Z_Strdup) (s,    PU_STATIC,0,__FILE__,__LINE__)

// Doom-style printf
void doom_printf(const char *, ...) __attribute__((format(printf,1,2)));

void Z_ZoneHistory(char *);

#ifdef INSTRUMENTED
extern int printstats;        // killough 8/23/98
void Z_PrintStats(void);      // killough 8/23/98
#endif

void Z_PrintZoneHeap(void);

#endif

//----------------------------------------------------------------------------
//
// $Log: z_zone.h,v $
// Revision 1.7  1998/05/08  20:32:12  killough
// fix __attribute__ redefinition
//
// Revision 1.6  1998/05/03  22:38:11  killough
// Remove unnecessary #include
//
// Revision 1.5  1998/04/27  01:49:42  killough
// Add history of malloc/free and scrambler (INSTRUMENTED only)
//
// Revision 1.4  1998/03/23  03:43:54  killough
// Make Z_CheckHeap() more diagnostic
//
// Revision 1.3  1998/02/02  13:28:06  killough
// Add doom_printf
//
// Revision 1.2  1998/01/26  19:28:04  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:03:06  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
