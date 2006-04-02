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

// VM types

typedef enum
{
   SC_VM_GAMESCRIPT,
   SC_VM_LEVELSCRIPT,
   SC_VM_END, // a special end marker (used by savegame code)
} sc_vm_e;

//
// Invocation Data
//
// This is one of the core structures used to interact with the Small
// scripting engine. This structure holds data about how a script was
// started, only while that script is running. Every SmallContext
// contains its own invocation data. This is one thing that FraggleScript
// REALLY needed.
//
typedef struct sc_invoke_s
{
   scriptinvoke_e invokeType; // invocation type for native functions

   // invocation data
   mobj_t *trigger; // thing that started script -- get with TID_TRIGGER
   int playernum;   // # of player that started script
   int line_lid;    // TODO: LID of line that started script
   int sector_sid;  // TODO: SID of sector that started script
} sc_invoke_t;

// callback flags

enum scb_flags
{
   SCBF_PAUSABLE = 0x00000001, // callback will wait if game pauses
   SCBF_MPAUSE   = 0x00000002, // callback will wait if menus up in non-netgame
};

// callbacks

typedef struct sc_callback_s
{
   char vm;     // vm to which this callback belongs
   int scriptNum;  // number of script to call (internal AMX #)

   enum
   {
      wt_none,          // not waiting
      wt_delay,         // wait for a set amount of time
      wt_tagwait,       // wait for sector to stop moving
      wt_numtypes,
   } wait_type;

   int wait_data;       // data used for waiting, varies by type

   int flags;           // 05/24/04: flags for special behaviors

   struct sc_callback_s *next; // for linked list
   struct sc_callback_s *prev; // for linked list

} sc_callback_t;

//
// SmallContext
//
// haleyjd 06/01/04: Because of reentrancy issues, most script
// starting must now use a SmallContext. This structure and its
// helper functions in a_small.c make amx_Exec appear to behave
// like a purely reentrant function to the rest of the game engine.
// Actually, it's a big and sort of gross hack necessitated by a 
// limitation in Small -- the code and execution context are still 
// married, even with amx_Clone (it cannot allow a shared memory 
// space; this structure has to copy back data to the parent context).
//
// Note that the AMX is also given a pointer back to its SmallContext
// container, so that native functions can retrieve the context data.
// This is done using the Small USERDATA facility and the helper
// function A_GetContextForAMX.
//
typedef struct SmallContext_s
{
   AMX smallAMX;                  // The Small AMX for this context
   sc_invoke_t invocationData;    // invocation data for this context
   struct SmallContext_s *parent; // parent context, if any
   struct SmallContext_s *child;  // child context, if any
   boolean isChild;               // true if created as a child
   sc_vm_e vm;                    // vm this context is or is a child of   
} SmallContext_t;

SmallContext_t *A_GetContextForAMX(AMX *);
SmallContext_t *A_CreateChildContext(SmallContext_t *, SmallContext_t *);
void A_DestroyChildContext(SmallContext_t *);

int  A_GetSmallString(AMX *amx, char **dest, cell addr);
char *A_GetAMXDataSegment(AMX *amx, long *size);
void A_ClearInvocation(SmallContext_t *);
void A_InitGameScript(void);
void A_InitLevelScript(void);
void A_InitSmall(void);
sc_callback_t *A_GetCallbackList(void);
void A_LinkCallback(sc_callback_t *);
void A_ExecuteCallbacks(void);
void A_RemoveCallbacks(int vm);
void A_RemoveCallback(sc_callback_t *callback);
int  A_AddCallback(char *scrname, sc_vm_e vm, 
                   int waittype, int waitdata, int waitflags);
cell A_ExecScriptV(AMX *amx, int fnNum);
cell A_ExecScriptByNum(AMX *amx, int number, int numparams, 
                       cell params[]);
cell A_ExecScriptByNumV(AMX *amx, int number);

extern boolean gameScriptLoaded;
extern boolean levelScriptLoaded;
extern SmallContext_t GameScript;
extern SmallContext_t *curGSContext;
extern SmallContext_t LevelScript;
extern SmallContext_t *curLSContext;

// haleyjd 07/06/04: FINE put it here!
mobj_t *P_FindMobjFromTID(int tid, mobj_t *rover, struct SmallContext_s *context);

#endif

// EOF

