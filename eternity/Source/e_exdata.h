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

#ifndef E_EXDATA_H__
#define E_EXDATA_H__

#include "doomdata.h"

// defines

// the ExtraData control object has doomednum 5004
#define ED_CTRL_DOOMEDNUM 5004

// the ExtraData line special has number 270
#define ED_LINE_SPECIAL 270

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

// Extended line special flags
// Because these go into a new field used only by parameterized
// line specials, I don't face the issue of running out of line
// flags anytime soon. This provides another full word for
// future expansion.
enum
{
   EX_ML_CROSS   = 0x00000001, // crossable
   EX_ML_USE     = 0x00000002, // usable
   EX_ML_IMPACT  = 0x00000004, // shootable
   EX_ML_PUSH    = 0x00000008, // reserved for future use
   EX_ML_PLAYER  = 0x00000010, // enabled for players
   EX_ML_MONSTER = 0x00000020, // enabled for monsters
   EX_ML_MISSILE = 0x00000040, // enabled for missiles
   EX_ML_REPEAT  = 0x00000080, // can be used multiple times
   EX_ML_1SONLY  = 0x00000100, // only activated from first side
};

// ExtraData line structure

typedef struct maplinedefext_s
{
   // standard fields
   maplinedef_t stdfields;

   // extended fields
   long extflags;
   long args[5];

   // internal fields (used by ExtraData only)
   int recordnum;
   int next;

} maplinedefext_t;

// Globals

void E_LoadExtraData(void);
mobj_t *E_SpawnMapThingExt(mapthing_t *mt);
void E_LoadLineDefExt(line_t *line);
boolean E_IsParamSpecial(short special);
void E_GetEDMapThings(mapthingext_t **things, int *numthings);
void E_GetEDLines(maplinedefext_t **lines, int *numlines);

#endif

// EOF

