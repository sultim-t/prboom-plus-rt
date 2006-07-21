/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include "doomstat.h"
#include "doomtype.h"

#ifdef __GNUG__
#pragma implementation "w_wad.h"
#endif
#include "w_wad.h"
#include "z_zone.h"
#include "lprintf.h"
#include "i_system.h"

#ifdef HEAPDUMP
void W_PrintLump(FILE* fp, void* p) {
}
#endif

#ifdef _WIN32
typedef struct {
  HANDLE hnd;
  OFSTRUCT fileinfo;
  HANDLE hnd_map;
  void   *data;
} mmap_info_t;

mmap_info_t *mapped_wad;

void W_DoneCache(void)
{
  size_t i;
  if (!mapped_wad)
    return;
  for (i=0; i<numwadfiles; i++)
  {
    if (mapped_wad[i].data)
    {
      UnmapViewOfFile(mapped_wad[i].data);
      mapped_wad[i].data=NULL;
    }
    if (mapped_wad[i].hnd_map)
    {
      CloseHandle(mapped_wad[i].hnd_map);
      mapped_wad[i].hnd_map=NULL;
    }
    if (mapped_wad[i].hnd)
    {
      CloseHandle(mapped_wad[i].hnd);
      mapped_wad[i].hnd=NULL;
    }
  }
  free(mapped_wad);
}

void W_InitCache(void)
{
  mapped_wad = calloc(numwadfiles,sizeof(mmap_info_t));
  memset(mapped_wad,0,sizeof(mmap_info_t)*numwadfiles);
  {
    int i;
    for (i=0; i<numlumps; i++)
    {
      int wad_index = (int)(lumpinfo[i].wadfile-wadfiles);
      if (!lumpinfo[i].wadfile)
        continue;
#ifdef RANGECHECK
      if ((wad_index<0)||((size_t)wad_index>=numwadfiles))
        I_Error("W_InitCache: wad_index out of range");
#endif
      if (!mapped_wad[wad_index].data)
      {
        mapped_wad[wad_index].hnd =
          (HANDLE)OpenFile(
            wadfiles[wad_index].name,
            &mapped_wad[wad_index].fileinfo,
            OF_READ
          );
        if (mapped_wad[wad_index].hnd==(HANDLE)HFILE_ERROR)
          I_Error("W_InitCache: OpenFile for memory mapping failed (LastError %i)",GetLastError());
        mapped_wad[wad_index].hnd_map =
          CreateFileMapping(
            mapped_wad[wad_index].hnd,
            NULL,
            PAGE_READONLY,
            0,
            0,
            NULL
          );
        if (mapped_wad[wad_index].hnd_map==NULL)
          I_Error("W_InitCache: CreateFileMapping for memory mapping failed (LastError %i)",GetLastError());
        mapped_wad[wad_index].data =
          MapViewOfFile(
            mapped_wad[wad_index].hnd_map,
            FILE_MAP_READ,
            0,
            0,
            0
          );
        if (mapped_wad[wad_index].hnd_map==NULL)
          I_Error("W_InitCache: MapViewOfFile for memory mapping failed (LastError %i)",GetLastError());
      }
    }
  }
}

const void* W_CacheLumpNum(int lump)
{
  int wad_index = (int)(lumpinfo[lump].wadfile-wadfiles);
#ifdef RANGECHECK
  if ((wad_index<0)||((size_t)wad_index>=numwadfiles))
    I_Error("W_CacheLumpNum: wad_index out of range");
#endif
  if (!lumpinfo[lump].wadfile)
    return NULL;
  return (void*)((unsigned char *)mapped_wad[wad_index].data+lumpinfo[lump].position);
}

#else

void ** mapped_wad;

void W_InitCache(void)
{
  int maxfd = 0;
  {
    int i;
    for (i=0; i<numlumps; i++)
      if (lumpinfo[i].wadfile)
        if (lumpinfo[i].wadfile->handle > maxfd) maxfd = lumpinfo[i].wadfile->handle;
  }
  mapped_wad = calloc(maxfd+1,sizeof *mapped_wad);
  {
    int i;
    for (i=0; i<numlumps; i++)
      if (lumpinfo[i].wadfile) {
        int fd = lumpinfo[i].wadfile->handle;
        if (!mapped_wad[fd])
          if ((mapped_wad[fd] = mmap(NULL,I_Filelength(fd),PROT_READ,MAP_SHARED,fd,0)) == MAP_FAILED) 
            I_Error("W_InitCache: failed to mmap");
      }
  }
}

void W_DoneCache(void)
{
  {
    int i;
    for (i=0; i<numlumps; i++)
      if (lumpinfo[i].wadfile) {
        int fd = lumpinfo[i].wadfile->handle;
        if (mapped_wad[fd]) {
          if (munmap(mapped_wad[fd],I_Filelength(fd))) 
            I_Error("W_DoneCache: failed to munmap");
          mapped_wad[fd] = NULL;
        }
      }
  }
  free(mapped_wad);
}

const void* W_CacheLumpNum(int lump)
{
  return (mapped_wad[lumpinfo[lump].wadfile->handle]+lumpinfo[lump].position);
}
#endif

void W_UnlockLumpNum(int lump) { }

