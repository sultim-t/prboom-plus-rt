/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $$
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 2001 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 *      Transparent access to data in WADs using mmap
 *
 *-----------------------------------------------------------------------------
 */

static const char
rcsid[] = "$Id: w_mmap.c,v 1.1 2001/07/12 20:55:54 cph Exp $";

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <unistd.h>
#include <sys/mman.h>

#include "doomstat.h"
#include "doomtype.h"

#ifdef __GNUG__
#pragma implementation "w_wad.h"
#endif
#include "w_wad.h"
#include "z_zone.h"
#include "lprintf.h"

void ** mapped_wad;

void W_InitCache(void)
{
  int maxfd = 0;
  {
    int i;
    for (i=0; i<numlumps; i++)
      if (lumpinfo[i].handle > maxfd) maxfd = lumpinfo[i].handle;
  }
  mapped_wad = calloc(maxfd+1,sizeof *mapped_wad);
  {
    int i;
    for (i=0; i<numlumps; i++) {
      int fd = lumpinfo[i].handle;
      if (!mapped_wad[fd])
        if (!(mapped_wad[fd] = mmap(NULL,W_Filelength(fd),PROT_READ,MAP_SHARED,fd,0))) 
          I_Error("W_InitCache: failed to mmap");
    }
  }
}

const void* W_CacheLumpNum(int lump)
{
  return (mapped_wad[lumpinfo[lump].handle]+lumpinfo[lump].position);
}

void W_UnlockLumpNum(int lump) { }

