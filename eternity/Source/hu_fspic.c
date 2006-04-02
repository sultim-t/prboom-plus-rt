// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
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
// Heads-up Graphics Support for FraggleScript
//
// By SoM, Revised by James Haley
//
//----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomtype.h"
#include "m_fixed.h"
#include "m_swap.h"
#include "z_zone.h"
#include "c_io.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_state.h"
#include "v_video.h"
#include "w_wad.h"
#include "hu_fspic.h"

fspic_t fspiclist = { NULL };
int lasthandle = -1;

//
// HU_ReInitFSPicList
//
// Called when a new level is started to reset global variables
//

void HU_ReInitFSPicList(void)
{
   // all fspics have already been freed by the zone allocator
   
   fspiclist.next = NULL;
   lasthandle = -1;
}
   
int HU_CreateFSPic(int lumpnum, int x, int y, boolean draw, boolean trans, fixed_t priority)
{
   fspic_t *rover, *prev, *newfsp;

   newfsp = Z_Malloc(sizeof(fspic_t), PU_LEVEL, NULL);
   
   prev = &fspiclist;
   
   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      if(rover->priority >= priority)
	 break;

      prev = rover;
   }

   newfsp->next = prev->next;
   prev->next = newfsp;

   newfsp->lumpnum = lumpnum;
   newfsp->x = x;
   newfsp->y = y;
   newfsp->draw = draw;
   newfsp->translucent = trans;
   newfsp->priority = priority;
   newfsp->handle = ++lasthandle;

   return newfsp->handle;
}

void HU_ModifyFSPic(int handle, int lumpnum, int x, int y)
{
   fspic_t *rover;

   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      if(rover->handle == handle)
      {
	 if(lumpnum != -1)
	    rover->lumpnum = lumpnum;
	 if(x != -1)
	    rover->x = x;
	 if(y != -1)
	    rover->y = y;

	 return;
      }
   }
}

void HU_ToggleFSPicVisible(int handle, boolean draw)
{
   fspic_t *rover;

   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      if(rover->handle == handle)
      {
	 rover->draw = draw;
	 return;
      }
   }
}

void HU_ToggleFSPicTL(int handle, boolean trans)
{
   fspic_t *rover;

   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      if(rover->handle == handle)
      {
	 rover->translucent = trans;
	 return;
      }
   }
}

void HU_DrawFSPics(void)
{
   fspic_t *rover;

   // short-circuit conditions for speed
   if(!fspiclist.next || viewcamera || automapactive)
      return;

   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      if(!rover->draw)
	 continue;
            
      rover->data = (patch_t *)W_CacheLumpNum(rover->lumpnum, PU_LEVEL);
      
      if(rover->x < 0 || 
	 rover->x + SHORT(rover->data->width) > SCREENWIDTH || 
	 rover->y < 0 || 
	 rover->y + SHORT(rover->data->height) > SCREENHEIGHT)
	 continue;

      if(rover->translucent)
	 V_DrawPatchTL(rover->x, rover->y, &vbscreen, rover->data, NULL, FTRANLEVEL);
      else
	 V_DrawPatch(rover->x, rover->y, &vbscreen, rover->data);
   }
}

int HU_GetFSPicHandle(int lumpnum, int x, int y)
{
   fspic_t *rover;

   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      if(rover->lumpnum == lumpnum && rover->x == x &&
	 rover->y == y)
      {
	 return rover->handle;
      }
   }

   return -1;
}

int HU_GetFSPicAttribute(int handle, const char *selector)
{
   fspic_t *rover;

   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      if(rover->handle == handle)
      {
	 if(!strcmp(selector, "lumpnum"))
	    return rover->lumpnum;
	 else if(!strcmp(selector, "x"))
	    return rover->x;
	 else if(!strcmp(selector, "y"))
	    return rover->y;
	 else if(!strcmp(selector, "draw"))
	    return rover->draw;
	 else if(!strcmp(selector, "trans"))
	    return rover->translucent;
      }
   }

   return -1;
}

fixed_t HU_GetFSPicPriority(int handle)
{
   fspic_t *rover;

   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      if(rover->handle == handle)
	 return rover->priority;
   }

   return -FRACUNIT;
}

void HU_SetFSPicPriority(int handle, fixed_t priority)
{
   fspic_t *rover, *targ, *prev = &fspiclist;

   if(!fspiclist.next)
      return;

   targ = NULL;

   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      if(rover->handle == handle)
      {
	 targ = rover;
	 break;
      }
      prev = rover;
   }

   if(!targ) return;

   prev->next = targ->next;

   targ->priority = priority;

   prev = &fspiclist;
   
   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      if(rover->priority >= priority)
	 break;

      prev = rover;
   }

   targ->next = prev->next;
   prev->next = targ;
}

fixed_t HU_GetLowestPriority(void)
{
   if(fspiclist.next)
      return fspiclist.next->priority;
   else
      return -FRACUNIT;
}

fixed_t HU_GetHighestPriority(void)
{
   fspic_t *rover, *prev = NULL;

   for(rover = fspiclist.next; rover; rover = rover->next)
   {
      prev = rover;
   }

   if(prev)
      return prev->priority;
   else
      return -FRACUNIT;
}

//
// HU_FSPicErase
//
// Yes, we might need to erase them sometimes :)
//
void HU_FSPicErase(void)
{
   fspic_t *rover;
   
   register int x, y, x2, y2;

   if(!fspiclist.next)
      return;

   for(rover = fspiclist.next; rover; rover = rover->next)
   {      
      if(!rover->draw || !rover->data)
         continue;

      x = rover->x;
      x2 = x + SHORT(rover->data->width);
      y = rover->y;
      y2 = y + SHORT(rover->data->height);

      if(x < 0 || x2 > SCREENWIDTH || y < 0 || y2 > SCREENHEIGHT)
         continue;

      R_VideoErase(x, y, SHORT(rover->data->width), y2 - y);
   }
}

// EOF
