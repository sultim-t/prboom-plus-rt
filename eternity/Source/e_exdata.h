// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2003 James Haley
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
//----------------------------------------------------------------------------
//
// ExtraData
//
// The be-all, end-all extension to the DOOM map format. Uses the
// libConfuse library like EDF.
// 
// ExtraData can extend mapthings, lines, and sectors with an
// arbitrary number of fields, with data provided in more or less
// any format. The use of a textual input language will forever
// remove any future problems caused by binary format limitations.
//
// By James Haley
//
//----------------------------------------------------------------------------

#ifndef E_EXDATA_H
#define E_EXDATA_H

#include "doomdata.h"

// defines

// the ExtraData control object has doomednum 5004
#define ED_CTRL_DOOMEDNUM 5004

// ExtraData mapthing structure

typedef struct mapthingext_s
{   
   // standard fields
   mapthing_t stdfields;

   // extended fields
   unsigned short tid;

   long args[5];

   // internal fields (used by ExtraData only)
   int recordnum;   
   int next;

} mapthingext_t;

void E_LoadExtraData(void);
mobj_t *E_SpawnMapThingExt(mapthing_t *mt);

#endif

// EOF

