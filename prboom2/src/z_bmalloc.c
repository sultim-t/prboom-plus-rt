/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: z_bmalloc.c,v 1.3 2000/05/07 20:19:34 proff_fs Exp $
 *
 *  Block memory allocator for LxDoom, 
 *  Copyright (C) 1999 by Colin Phipps
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
 * This is designed to be a fast allocator for small, regularly used block sizes
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <memory.h>

#include "doomtype.h"
#include "z_zone.h"
#include "z_bmalloc.h"
#include "lprintf.h"

typedef struct bmalpool_s {
  struct bmalpool_s *nextpool;
  size_t             blocks;
  byte               used[0];
} bmalpool_t;

// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static void* getelem(bmalpool_t *p, size_t size, size_t n)
{
  return (((byte*)p) + sizeof(bmalpool_t) + sizeof(byte)*(p->blocks) + size*n);
}

// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static const int iselem(const bmalpool_t *pool, size_t size, const void* p)
{
  // CPhipps - need portable # of bytes between pointers
  int dif = (const char*)p - (const char*)pool;

  dif -= sizeof(bmalpool_t);
  dif -= pool->blocks;
  if (dif<0) return -1;
  dif /= size;
  return (((size_t)dif >= pool->blocks) ? -1 : dif);
}

enum { unused_block = 0, used_block = 1};

void* Z_BMalloc(struct block_memory_alloc_s *pzone)
{
  register bmalpool_t **pool = (bmalpool_t **)&(pzone->firstpool);
  while (*pool != NULL) {
    byte *p = memchr((*pool)->used, unused_block, (*pool)->blocks); // Scan for unused marker
    if (p) {
      int n = p - (*pool)->used;
#ifdef SIMPLECHECKS
      if ((n<0) || (n>=(*pool)->blocks)) 
	I_Error("Z_BMalloc: memchr returned pointer outside of array!");
#endif
      (*pool)->used[n] = used_block;
      return getelem(*pool, pzone->size, n);
    } else 
      pool = &((*pool)->nextpool);
  }
  {
    // Nothing available, must allocate a new pool
    bmalpool_t *newpool;

    // CPhipps: Allocate new memory, initialised to 0

    *pool = newpool = Z_Calloc(sizeof(*newpool) + (sizeof(byte) + pzone->size)*(pzone->perpool), 
			       1,  pzone->tag, NULL);
    newpool->nextpool = NULL; // NULL = (void*)0 so this is redundant

    // Return element 0 from this pool to satisfy the request
    newpool->used[0] = used_block;
    newpool->blocks = pzone->perpool;
    return getelem(newpool, pzone->size, 0);
  }
}

void Z_BFree(struct block_memory_alloc_s *pzone, void* p)
{
  register bmalpool_t **pool = (bmalpool_t**)&(pzone->firstpool);

  while (*pool != NULL) {
    int n = iselem(*pool, pzone->size, p);
    if (n >= 0) {
#ifdef SIMPLECHECKS
      if ((*pool)->used[n] == unused_block)
	I_Error("Z_BFree: refree in zone %s\n", pzone->desc);
#endif
      (*pool)->used[n] = unused_block;
      if (memchr(((*pool)->used), used_block, (*pool)->blocks) == NULL) {
	// Block is all unused, can be freed
	bmalpool_t *oldpool = *pool;
	*pool = (*pool)->nextpool;
	Z_Free(oldpool);
      }
      return;
    } else pool = &((*pool)->nextpool);
  }
  I_Error("Z_BFree: free not in zone %s\n", pzone->desc);
}

/*
 * $Log: z_bmalloc.c,v $
 * Revision 1.3  2000/05/07 20:19:34  proff_fs
 * changed use of colormaps from pointers to numbers.
 * That's needed for OpenGL.
 * The OpenGL part is slightly better now.
 * Added some typedefs to reduce warnings in VisualC.
 * Messages are also scaled now, because at 800x600 and
 * above you can't read them even on a 21" monitor.
 *
 * Revision 1.2  2000/05/06 08:49:55  cph
 * Minor header file fixing
 *
 * Revision 1.1.1.1  2000/05/04 08:19:19  proff_fs
 * initial login on sourceforge as prboom2
 *
 * Revision 1.7  2000/05/01 15:16:48  Proff
 * added __inline for VisualC
 *
 * Revision 1.6  2000/05/01 14:09:41  Proff
 * fixed #ifdef SIMPECHECKS to SIMPLECHECKS
 *
 * Revision 1.5  1999/10/31 16:25:22  cphipps
 * Change i_system.h include to lprintf.h, where I_Error is now
 *
 * Revision 1.4  1999/10/12 13:01:15  cphipps
 * Changed header to GPL
 *
 * Revision 1.3  1999/01/13 08:03:34  cphipps
 * Fix iselem() pointer casts
 *
 * Revision 1.2  1999/01/02 17:53:56  cphipps
 * Remove temporary debugging stuff
 *
 * Revision 1.1  1999/01/02 17:53:16  cphipps
 * Initial revision
 *
 */
