// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2002 James Haley
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
// DESCRIPTION:
//
// Small Scripting Engine Interface
// 
// Functions needed to provide for game engine usage of the Small
// virtual machine (see amx.c)
//
// By James Haley
//
//----------------------------------------------------------------------------

#ifndef __A_SMALL_H__
#define __A_SMALL_H__

#include "p_mobj.h"
#include "amx.h"

// custom app-defined errors

#define SC_ERR_MASK 32768

typedef enum
{
   SC_ERR_READ,                        // lump read failure
   SC_ERR_GAMEMODE,                    // native not available in gm
   SC_ERR_INVOKE,                      // data invalid for inv. model
   SC_ERR_BADVM,                       // unknown vm type
   SC_ERR_BADWAIT,                     // unknown wait type
   SC_ERR_REENTRANT,                   // attempt at reentrant call
   SC_ERR_NUMERRS                      // maximum
} scripterr_e;

// invocation models

typedef enum
{
   SC_INVOKE_NONE,     // internal
   SC_INVOKE_CCMD,     // console command
   SC_INVOKE_THING,    // thing frame
   SC_INVOKE_PLAYER,   // player gun frame
   SC_INVOKE_LINE,     // linedef
   SC_INVOKE_TERRAIN,  // scripted TerrainType
   SC_INVOKE_CALLBACK, // scheduled callback
   SC_INVOKE_SPECIAL,  // started as a special
   SC_INVOKE_DIALOGUE, // started by dialogue
} scriptinvoke_e;

typedef struct sc_invoke_s
{
   scriptinvoke_e invokeType;

   boolean execd; // true if started by Exec* function

   // invocation data
   mobj_t *trigger;
   int playernum;
   int line_lid;
   int sector_sid;
} sc_invoke_t;

typedef enum
{
   SC_VM_GAMESCRIPT,
   SC_VM_LEVELSCRIPT,
} sc_vm_e;

// callbacks

typedef struct sc_callback_s
{
   int scriptNum;  // number of script to call (internal AMX #)
   sc_vm_e vm;     // vm to which this callback belongs

   enum
   {
      wt_none,          // not waiting
      wt_delay,         // wait for a set amount of time
      wt_tagwait,       // wait for sector to stop moving
      wt_numtypes,
   } wait_type;

   int wait_data;       // data used for waiting, varies by type

   struct sc_callback_s *next; // for linked list
   struct sc_callback_s *prev; // for linked list

} sc_callback_t;

int  A_GetSmallString(AMX *amx, char **dest, cell addr);
void A_ClearInvocation(void);
void A_InitGameScript(void);
void A_InitLevelScript(void);
void A_InitSmall(void);
void A_ExecuteCallbacks(void);
void A_RemoveCallbacks(int vm);
void A_RemoveCallback(sc_callback_t *callback);
int  A_AddCallback(char *scrname, sc_vm_e vm, 
                   int waittype, int waitdata);
cell A_ExecScriptV(AMX *amx, int fnNum);
cell A_ExecScriptByNum(AMX *amx, int number, int numparams, 
                       cell params[]);

extern boolean gameScriptLoaded;
extern boolean levelScriptLoaded;
extern AMX gamescript;
extern AMX levelscript;
extern sc_invoke_t sc_invocation;

#endif

// EOF

