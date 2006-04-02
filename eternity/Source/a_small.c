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

#include "z_zone.h"
#include "doomtype.h"
#include "w_wad.h"
#include "v_misc.h"
#include "c_io.h"
#include "c_net.h"
#include "c_runcmd.h"
#include "p_spec.h"
#include "p_info.h"
#include "m_misc.h"
#include "a_small.h"
#include "hu_stuff.h"
#include "m_qstr.h"

//
// The Game Script
//
// This is a persistent AMX that is always around, and is used
// for globally available scripts, such as those called from
// mobj frames, etc.
//
void *gameScriptData = NULL;
boolean gameScriptLoaded = false;

SmallContext_t GameScript;
SmallContext_t *curGSContext;

//AMX gamescript;

//
// The Level Script
//
// This is a per-level AMX. Used for level-local scripts provided
// along with the map.
//
void *levelScriptData = NULL;
boolean levelScriptLoaded = false;

SmallContext_t LevelScript;
SmallContext_t *curLSContext;

//AMX levelscript;

//
// Initialization and Utilities
//

//
// Context Stuff:
//
// This is now necessary to resolve problems with non-reentrancy
// of the Small interpreter.  amx_Clone must be used in concert with
// copying back of the child data segment after execution. The
// contexts are kept in a tree, allowing any level of recursion to
// function properly.
//

//
// A_GetContextForAMX
//
// Small natives must call this to retrieve a pointer to the context
// for the AMX that is passed to them. It is stored using Small's
// user data facility.
//
SmallContext_t *A_GetContextForAMX(AMX *amx)
{
   void *context;

   amx_GetUserData(amx, AMX_USERTAG('C','N','T','X'), &context);

   return (SmallContext_t *)context;
}

//
// A_CreateChildContext
//
// This function allows reentrant calls to amx_Exec on the same vm
// by creating a cloned AMX. A_DestroyChildContext below must be
// called afterward to copy the clone's data segment back to the
// parent and then destroy the clone.
//
// Returns a pointer to the context that should be used for
// execution. It will return parent if a child context wasn't
// created.
//
SmallContext_t *A_CreateChildContext(SmallContext_t *parent, 
                                     SmallContext_t *child)
{
   long dataSize, sthpSize, totalSize;
   byte *data;

   // if the parent isn't running, we don't need to create a child context
   if(parent->invocationData.invokeType == SC_INVOKE_NONE)
      return parent;

#ifdef RANGECHECK
   // Big bug if this happens!
   if(parent->child)
      I_Error("A_CreateChildContext: child SmallContext displaced\n");
#endif

   memset(child, 0, sizeof(SmallContext_t));

   amx_MemInfo(&parent->smallAMX, NULL, &dataSize, &sthpSize);

   totalSize = dataSize + sthpSize;

   data = malloc(totalSize);

   if(amx_Clone(&child->smallAMX, &parent->smallAMX, data) != AMX_ERR_NONE)
      I_Error("A_CreateChildContext: internal Small error\n");

   // set child AMX to point back to its context
   amx_SetUserData(&child->smallAMX, 
                   AMX_USERTAG('C','N','T','X'), child);
   
   child->vm = parent->vm;

   // update the appropriate global context pointer
   switch(child->vm)
   {
   default:
   case SC_VM_LEVELSCRIPT:
      curLSContext = child;
      break;
   case SC_VM_GAMESCRIPT:
      curGSContext = child;
      break;
   }

   child->parent = parent;
   parent->child = child;

   // this context was created as a child context;
   // A_DestroyChildContext should destroy it.
   child->isChild = true;

   return child;
}

//
// A_DestroyChildContext
//
// Cleans up after A_CreateChildContext. This function will ignore any
// context that wasn't created as a child to begin with, and is thus
// safe to always call, even in non-reentrant situations.
//
void A_DestroyChildContext(SmallContext_t *context)
{
   sc_vm_e vmNum;
   byte *dest;
   long size;
   SmallContext_t *parent = context->parent;

   // if this isn't a context that was created as a child, we don't
   // need to do anything here.
   if(!context->isChild)
      return;

#ifdef RANGECHECK
   // A context is marked as a child but has no parent??
   if(!parent)
      I_Error("A_DestroyChildContext: child context with no parent!\n");
#endif

   // copy child data segment back into parent
   dest = A_GetAMXDataSegment(&parent->smallAMX, &size);
   memcpy(dest, context->smallAMX.data, size);

   // free the previously allocated child data segment
   free(context->smallAMX.data);

   parent->child = NULL;

   vmNum = parent->vm;

   // update the appropriate global context pointer
   switch(vmNum)
   {
   default:
   case SC_VM_LEVELSCRIPT:
      curLSContext = parent;
      break;
   case SC_VM_GAMESCRIPT:
      curGSContext = parent;
      break;
   }
}


//
// A_AMXProgramSize
//
// Returns the total amount of memory to allocate for an entire
// AMX script.
//
static size_t A_AMXProgramSize(char *lumpname)
{
   int lumpnum;
   AMX_HEADER hdr;

   lumpnum = W_CheckNumForName(lumpname);

   if(lumpnum == -1)
      return 0;

   W_ReadLumpHeader(lumpnum, (void *)(&hdr), sizeof(hdr));

   amx_Align16((unsigned short *)(&hdr.magic));
   amx_Align32((unsigned long  *)(&hdr.stp));

   return ((hdr.magic == AMX_MAGIC) ? (size_t)hdr.stp : 0);
}

//
// A_AMXLoadProgram
//
// Loads a script from the requested lump and initializes the
// corresponding AMX structure.
//
static int A_AMXLoadProgram(AMX *amx, char *lumpname, void *memblock)
{
   int lumpnum;
   byte *lump;
   AMX_HEADER hdr;

   lumpnum = W_CheckNumForName(lumpname);

   if(lumpnum == -1)
   {
      return (SC_ERR_READ | SC_ERR_MASK);
   }
   
   W_ReadLumpHeader(lumpnum, (void *)(&hdr), sizeof(hdr));
   amx_Align32((unsigned long *)(&hdr.size));

   lump = W_CacheLumpNum(lumpnum, PU_CACHE);
   memcpy(memblock, lump, (size_t)(hdr.size));

   return amx_Init(amx, memblock);
}

// haleyjd 08/08/04: copied from Small 2.7.3 amxaux.c, unfortunate
// that this was removed from amx.c and put into a module with a ton
// of stuff I don't need... it's a waste of time and space here.

const char *AMXAPI aux_StrError(int errnum)
{
   static const char *messages[] = {
      /* AMX_ERR_NONE      */ "(none)",
      /* AMX_ERR_EXIT      */ "Forced exit",
      /* AMX_ERR_ASSERT    */ "Assertion failed",
      /* AMX_ERR_STACKERR  */ "Stack/heap collision (insufficient stack size)",
      /* AMX_ERR_BOUNDS    */ "Array index out of bounds",
      /* AMX_ERR_MEMACCESS */ "Invalid memory access",
      /* AMX_ERR_INVINSTR  */ "Invalid instruction",
      /* AMX_ERR_STACKLOW  */ "Stack underflow",
      /* AMX_ERR_HEAPLOW   */ "Heap underflow",
      /* AMX_ERR_CALLBACK  */ "No (valid) native function callback",
      /* AMX_ERR_NATIVE    */ "Native function failed",
      /* AMX_ERR_DIVIDE    */ "Divide by zero",
      /* AMX_ERR_SLEEP     */ "(sleep mode)",
      /* 13 */                "(reserved)",
      /* 14 */                "(reserved)",
      /* 15 */                "(reserved)",
      /* AMX_ERR_MEMORY    */ "Out of memory",
      /* AMX_ERR_FORMAT    */ "Invalid/unsupported P-code file format",
      /* AMX_ERR_VERSION   */ "File is for a newer version of the AMX",
      /* AMX_ERR_NOTFOUND  */ "File or function is not found",
      /* AMX_ERR_INDEX     */ "Invalid index parameter (bad entry point)",
      /* AMX_ERR_DEBUG     */ "Debugger cannot run",
      /* AMX_ERR_INIT      */ "AMX not initialized (or doubly initialized)",
      /* AMX_ERR_USERDATA  */ "Unable to set user data field (table full)",
      /* AMX_ERR_INIT_JIT  */ "Cannot initialize the JIT",
      /* AMX_ERR_PARAMS    */ "Parameter error",
      /* AMX_ERR_DOMAIN    */ "Domain error, expression result does not fit in range",
      /* AMX_ERR_GENERAL   */ "General error (unknown or unspecific error)",
   };
   if(errnum < 0 || errnum >= sizeof messages / sizeof messages[0])
      return "(unknown)";
   return messages[errnum];
}

//
// A_AMXError
//
// Handles both internal AMX errors and implementation-defined
// errors caused by setup or native functions. Prints information
// to the console and makes a warning sound.
//
static void A_AMXError(int err)
{
   const char *errmsg = NULL;
   static const char *msgs[SC_ERR_NUMERRS] =
   {
      "Script lump empty or not found",           // SC_ERR_READ
      "Bad gamemode for native function",         // SC_ERR_GAMEMODE
      "Bad invocation model for native function", // SC_ERR_INVOKE
      "Unknown or uninitialized VM",              // SC_ERR_BADVM
      "Unknown wait type",                        // SC_ERR_BADWAIT
      "Cannot make reentrant call",               // SC_ERR_REENTRANT
   };

   // some "errors" aren't really errors
   if(err == AMX_ERR_NONE || err == AMX_ERR_EXIT)
      return;

   if(err & SC_ERR_MASK)
   {
      err &= ~SC_ERR_MASK;
      
      if(err >= 0 && err < SC_ERR_NUMERRS)
         errmsg = msgs[err];
   }
   else
      errmsg = aux_StrError(err);

   if(!errmsg)
      errmsg = "Unknown error";

   C_Printf(FC_ERROR "Script error: %s\a\n", errmsg);
}

extern AMX_NATIVE_INFO    core_Natives[];
extern AMX_NATIVE_INFO cons_io_Natives[];
extern AMX_NATIVE_INFO   local_Natives[]; // actually in this module
extern AMX_NATIVE_INFO    ccmd_Natives[];
extern AMX_NATIVE_INFO   chase_Natives[];
extern AMX_NATIVE_INFO   panim_Natives[];
extern AMX_NATIVE_INFO  random_Natives[];
extern AMX_NATIVE_INFO    game_Natives[];
extern AMX_NATIVE_INFO   sound_Natives[];
extern AMX_NATIVE_INFO    user_Natives[];
extern AMX_NATIVE_INFO  pinter_Natives[];
extern AMX_NATIVE_INFO   fixed_Natives[];
extern AMX_NATIVE_INFO     edf_Natives[];
extern AMX_NATIVE_INFO    mobj_Natives[];
extern AMX_NATIVE_INFO  genlin_Natives[];
extern AMX_NATIVE_INFO   pspec_Natives[];
extern AMX_NATIVE_INFO hustuff_Natives[];
extern AMX_NATIVE_INFO    ptcl_Natives[];

//
// A_RegisterNatives
//
// Registers all native functions with the given AMX.
// The native functions registered here are in various modules. 
// I feel this is preferable to putting all natives in one giant, 
// entangled module.
//
static int A_RegisterNatives(AMX *amx)
{
   // register each module's natives
   amx_Register(amx, cons_io_Natives, -1); // c_io   
   amx_Register(amx, local_Natives,   -1); // misc stuff in here
   amx_Register(amx, ccmd_Natives,    -1); // c_cmd
   amx_Register(amx, chase_Natives,   -1); // p_chase
   amx_Register(amx, panim_Natives,   -1); // p_anim
   amx_Register(amx, random_Natives,  -1); // m_random
   amx_Register(amx, game_Natives,    -1); // g_game
   amx_Register(amx, sound_Natives,   -1); // s_sound
   amx_Register(amx, user_Natives,    -1); // p_user
   amx_Register(amx, pinter_Natives,  -1); // p_inter
   amx_Register(amx, fixed_Natives,   -1); // a_fixed
   amx_Register(amx, edf_Natives,     -1); // e_cmd
   amx_Register(amx, mobj_Natives,    -1); // p_mobj
   amx_Register(amx, genlin_Natives,  -1); // p_genlin
   amx_Register(amx, pspec_Natives,   -1); // p_spec
   amx_Register(amx, hustuff_Natives, -1); // hu_stuff
   amx_Register(amx, ptcl_Natives,    -1); // p_partcl

   // finally, load the core functions
   return amx_Register(amx, core_Natives, -1); // amxcore.c
}

//
// A_GetSmallString
//
// Convenience method that does everything necessary to allocate
// and read a string from a Small VM address.
//
int A_GetSmallString(AMX *amx, char **dest, cell addr)
{
   cell *cellstr;
   int err, len;

   if((err = amx_GetAddr(amx, addr, &cellstr)) != AMX_ERR_NONE)
      return err;

   amx_StrLen(cellstr, &len);
   *dest = Z_Malloc(len + 1, PU_STATIC, NULL);
   amx_GetString(*dest, cellstr, 0);

   return AMX_ERR_NONE;
}

//
// A_GetAMXDataSegment
//
// Returns a pointer to the given AMX's data segment, and can
// optionally return the size of the segment in bytes in *size.
//
char *A_GetAMXDataSegment(AMX *amx, long *size)
{
   AMX_HEADER *hdr = (AMX_HEADER *)amx->base;

   if(size)
      amx_MemInfo(amx, NULL, size, NULL);

   return (amx->data != NULL) ? amx->data : amx->base + (int)hdr->dat;
}

//
// Invocation model functions
//

//
// A_ClearInvocation
//
// Resets the invocation structure to its default state.
// This must be done after any script execution.
//
void A_ClearInvocation(SmallContext_t *sc)
{
   memset(&sc->invocationData, 0, sizeof(sc_invoke_t));
}



//
// Optional Engine Callback Dispatchers
//
void A_OptScriptCallback(SmallContext_t *ctx, const char *cbname)
{
   int index;
   AMX *amx = &ctx->smallAMX;
   
   if(amx_FindPublic(amx, cbname, &index) == AMX_ERR_NONE)
   {
      ctx->invocationData.invokeType = SC_INVOKE_CALLBACK;
      A_ExecScriptV(amx, index);
      A_ClearInvocation(ctx);
   }
}

//
// A_InitGameScript
//
// Sets up the global gamescript VM.
//
void A_InitGameScript(void)
{
   int memsize;
   int error;
   AMX *amx = &GameScript.smallAMX;

   memset(&GameScript, 0, sizeof(SmallContext_t));

   // set VM type
   GameScript.vm = SC_VM_GAMESCRIPT;

   gameScriptLoaded = false;

   C_Printf(FC_HI "A_InitGameScript: " 
            FC_NORMAL "loading game scripts\n");

   // 07/19/03: go ahead and check if GAMESCR exists
   // before trying to load it; eliminates need for a 
   // dummy lump in eternity.wad
   
   if(W_CheckNumForName("GAMESCR") == -1)
      return;

   if(!(memsize = A_AMXProgramSize("GAMESCR")))
   {
      A_AMXError(SC_ERR_READ | SC_ERR_MASK);
      return;
   }

   // free any previously existing game script
   if(gameScriptData)
   {
      amx_Cleanup(amx);
      Z_Free(gameScriptData);
      gameScriptData = NULL;
   }

   gameScriptData = Z_Malloc(memsize, PU_STATIC, NULL);

   // load the GAMESCR lump
   error = A_AMXLoadProgram(amx, "GAMESCR", gameScriptData);

   if(error != AMX_ERR_NONE)
   {
      A_AMXError(error);
      return;
   }

   // register native functions
   if((error = A_RegisterNatives(amx)) != AMX_ERR_NONE)
   {
      A_AMXError(error);
      return;
   }

   // load was successful!
   gameScriptLoaded = true;

   curGSContext = &GameScript;

   // make the AMX point back to the GameScript context via the
   // Small UserData API
   amx_SetUserData(amx, AMX_USERTAG('C','N','T','X'), &GameScript);

   // run the init script, if it exists
   A_OptScriptCallback(&GameScript, "OnInit");
}

//
// A_InitLevelScript
//
// Sets up the levelscript VM for the current level.
//
void A_InitLevelScript(void)
{
   int memsize;
   int error;
   AMX *amx = &LevelScript.smallAMX;

   memset(&LevelScript, 0, sizeof(SmallContext_t));

   // set VM type
   LevelScript.vm = SC_VM_LEVELSCRIPT;

   levelScriptLoaded = false;

   // free any previously existing level script
   if(levelScriptData)
   {
      amx_Cleanup(amx);
      Z_Free(levelScriptData);
      levelScriptData = NULL;
   }

   if(!LevelInfo.hasScripts) // current level has no scripts
      return;

   if(!(memsize = A_AMXProgramSize(LevelInfo.scriptLump)))
   {
      A_AMXError(SC_ERR_READ | SC_ERR_MASK);
      return;
   }

   // allocate the script -- note PU_STATIC since we free it ourselves
   levelScriptData = Z_Malloc(memsize, PU_STATIC, NULL);

   // load the script
   error = A_AMXLoadProgram(amx, LevelInfo.scriptLump, levelScriptData);

   if(error != AMX_ERR_NONE)
   {
      A_AMXError(error);
      return;
   }

   // register native functions
   if((error = A_RegisterNatives(amx)) != AMX_ERR_NONE)
   {
      A_AMXError(error);
      return;
   }

   // load was successful!
   levelScriptLoaded = true;

   curLSContext = &LevelScript;

   // make the AMX point back to the LevelScript context via the
   // Small UserData API
   amx_SetUserData(amx, AMX_USERTAG('C','N','T','X'), &LevelScript);

   // run the OnInit script, if it exists and we are NOT loading
   // a saved game. The state created by OnInit should still exist 
   // in a saved game, so running the script again would be unexpected.

   if(gameaction != ga_loadgame)
      A_OptScriptCallback(&LevelScript, "OnInit");
}

//
// Executing Scripts
//

//
// A_ExecScriptV
//
// Execute a script by internal AMX number, passing no params.
//
cell A_ExecScriptV(AMX *amx, int fnNum)
{
   int err;
   cell retval;

   if((err = amx_Exec(amx, &retval, fnNum, 0)) != AMX_ERR_NONE)
   {
      A_AMXError(err);
   }

   return retval;
}

//
// A_ExecScriptNameV
//
// Execute a script, passing no parameters
//
cell A_ExecScriptNameV(AMX *amx, char *fn)
{
   int err;
   int index;
   cell retval;

   if((err = amx_FindPublic(amx, fn, &index)) != AMX_ERR_NONE)
   {
      A_AMXError(err);
      return 0;
   }

   if((err = amx_Exec(amx, &retval, index, 0)) != AMX_ERR_NONE)
   {
      A_AMXError(err);
   }

   return retval;
}

//
// A_ExecScriptNameI
//
// Execute a script, passing a given number of integer parameters
//
cell A_ExecScriptNameI(AMX *amx, char *fn, 
                       int numparams, cell params[])
{
   int err;
   int index;
   cell retval;

   if((err = amx_FindPublic(amx, fn, &index)) != AMX_ERR_NONE)
   {
      A_AMXError(err);
      return 0;
   }

   if((err = amx_Execv(amx, &retval, index, numparams, params)) 
         != AMX_ERR_NONE)
   {
      A_AMXError(err);
   }

   return retval;
}

//
// A_ExecScriptByNum
//
// Implements call by number, for use from linedefs and frames.
// The number given is translated into "Script<number>", which
// must match the name of a public function in the given AMX.
//
cell A_ExecScriptByNum(AMX *amx, int number, int numparams, 
                       cell params[])
{
   char scriptname[64];

   psnprintf(scriptname, sizeof(scriptname), 
             "%s%d", "Script", number);

   return A_ExecScriptNameI(amx, scriptname, numparams, params);
}

cell A_ExecScriptByNumV(AMX *amx, int number)
{
   char scriptname[64];

   psnprintf(scriptname, sizeof(scriptname), 
             "%s%d", "Script", number);

   return A_ExecScriptNameV(amx, scriptname);
}

//
// Callbacks
//

static sc_callback_t *currentcb;
static sc_callback_t callBackHead;

//
// A_GetCallbackList
//
// This is needed by the save/load game code, so it can save
// callbacks, and then clear out and restore the old callback
// list on game load. (You could make callBackHead global, but
// I don't like introducing more globals, there are too many
// as it is ;)
//
sc_callback_t *A_GetCallbackList(void) 
{ 
   return &callBackHead; 
}

//
// A_LinkCallback
//
// Puts a pre-built callback into the callback list.
//
void A_LinkCallback(sc_callback_t *callback)
{
   // insert callback at head of list (order is irrelevant)
   callback->prev = &callBackHead;
   callback->next = callBackHead.next;
   callBackHead.next->prev = callback;
   callBackHead.next = callback;
}

//
// A_AddCallback
//
// Schedules a callback for the given vm -- the indicated
// script will be called at the time indicated by its wait type
// and data. Callbacks are kept in a double-linked list.
//
int A_AddCallback(char *scrname, sc_vm_e vm, int waittype,
                  int waitdata, int waitflags)
{
   AMX *pvm;
   sc_callback_t *newCallback = NULL;
   int err, scnum;

   // check wait type for validity
   if(waittype < 0 || waittype >= wt_numtypes)
   {
      return (SC_ERR_BADWAIT | SC_ERR_MASK);
   }

   // check the vm, get a pointer
   switch(vm)
   {
   case SC_VM_GAMESCRIPT:
      pvm = &GameScript.smallAMX;
      break;
   case SC_VM_LEVELSCRIPT:
      pvm = &LevelScript.smallAMX;
      break;
   default:
      return (SC_ERR_BADVM | SC_ERR_MASK);
   }

   // get script num for name
   if((err = amx_FindPublic(pvm, scrname, &scnum)) != AMX_ERR_NONE)
   {
      return err;
   }
   
   newCallback = Z_Malloc(sizeof(sc_callback_t), PU_STATIC, 0);

   newCallback->vm = vm;
   newCallback->scriptNum = scnum;
   newCallback->wait_type = waittype;
   newCallback->wait_data = waitdata;
   newCallback->flags     = waitflags;

   // link callback into list
   A_LinkCallback(newCallback);
   
   return AMX_ERR_NONE;
}

//
// A_RemoveCallback
//
// Unlinks and frees the indicated callback structure.
// This is safe to call in a loop that uses currentcb to track
// its position (this works just like the thinker list code).
//
void A_RemoveCallback(sc_callback_t *callback)
{
   // modify currentcb
   currentcb = callback->prev;
   callback->next->prev = callback->prev;
   callback->prev->next = callback->next;

   Z_Free(callback);
}

//
// A_RemoveCallbacks
//
// Removes all callbacks matching the vm class, or all callbacks if
// vm is equal to -1.
//
void A_RemoveCallbacks(int vm)
{
   for(currentcb = callBackHead.next; 
       currentcb != &callBackHead; 
       currentcb = currentcb->next)
   {
      if(vm == -1 || vm == currentcb->vm)
         A_RemoveCallback(currentcb);
   }
}

//
// A_WaitFinished
//
// Returns true if the given callback is ready to execute.
// This code is actually the only thing that has made its
// way here from FraggleScript :)
//
static boolean A_WaitFinished(sc_callback_t *callback)
{
   // check pause flags

   if((callback->flags & SCBF_PAUSABLE && paused) ||
      (callback->flags & SCBF_MPAUSE && 
       menuactive && !demoplayback && !netgame))
      return false;

   switch(callback->wait_type)
   {
   default:
   case wt_none:
      return true;
   case wt_delay:
      return (--callback->wait_data <= 0);
   case wt_tagwait:
      {
         int secnum = -1;
         
         while((secnum = P_FindSectorFromTag(callback->wait_data, 
                                             secnum)) >= 0)
         {
            sector_t *sec = &sectors[secnum];
            if(sec->floordata || sec->ceilingdata || sec->lightingdata)
               return false;        // not finished
         }
      }
      return true;
   }
}

//
// A_ExecuteCallbacks
//
// Runs down the double-linked list of callbacks and checks each
// to see if it is ready to execute. If a callback executes, it is
// immediately removed. Callbacks should be re-registered by the
// callback script if continuous timer-style execution is desired.
//
void A_ExecuteCallbacks(void)
{
   AMX *vm;
   SmallContext_t *context;
   int err;
   cell retval;

   for(currentcb = callBackHead.next;
       currentcb != &callBackHead;
       currentcb = currentcb->next)
   {
      switch(currentcb->vm)
      {
      default:
      case SC_VM_GAMESCRIPT:
         context = &GameScript;
         vm = &GameScript.smallAMX;
         break;
      case SC_VM_LEVELSCRIPT:
         context = &LevelScript;
         vm = &LevelScript.smallAMX;
         break;
      }

      if(A_WaitFinished(currentcb))
      {
         // execute the callback
         context->invocationData.invokeType = SC_INVOKE_CALLBACK;

         // this cannot be called recursively, no need to clone
         if((err = amx_Exec(vm, &retval, currentcb->scriptNum, 0))
                 != AMX_ERR_NONE)
         {
            A_AMXError(err);
         }

         A_ClearInvocation(context);

         // remove this callback from the list
         A_RemoveCallback(currentcb);
      }
   }
}

//
// A_InitSmall
//
// Called at program startup. Sets up basic stuff needed for Small.
//
void A_InitSmall(void)
{
   // initialize the double-linked callback list
   callBackHead.next = callBackHead.prev = &callBackHead;
}

//
// Console Commands
//

//
// a_running
//
// Prints a list of running callback data to the console
//
CONSOLE_COMMAND(a_running, 0)
{
   sc_callback_t *cb;
   int i = 0;

   for(cb = callBackHead.next; cb != &callBackHead; cb = cb->next)
   {
      C_Printf("callback %d: scr#:%d  wttype:%d wtdata:%d\n", 
               i, cb->scriptNum, cb->wait_type, cb->wait_data);
      ++i;
   }
}

//
// a_execv
//
// Executes a script with no arguments from the console
//
CONSOLE_COMMAND(a_execv, cf_notnet)
{
   SmallContext_t *context;
   sc_vm_e vmNum;
   AMX *vm;

   if(c_argc < 2)
   {
      C_Printf("usage: a_execv vm scriptname\n");
      return;
   }

   vmNum = atoi(c_argv[0]);

   switch(vmNum)
   {
   default:
   case SC_VM_GAMESCRIPT:
      if(!gameScriptLoaded)
      {
         C_Printf(FC_ERROR "game script not loaded\n");
         return;
      }
      context = &GameScript;
      vm = &GameScript.smallAMX;
      break;
   case SC_VM_LEVELSCRIPT:
      if(!levelScriptLoaded)
      {
         C_Printf(FC_ERROR "level script not loaded\n");
         return;
      }
      context = &LevelScript;
      vm = &LevelScript.smallAMX;
      break;
   }

   context->invocationData.invokeType = SC_INVOKE_CCMD;
   context->invocationData.playernum  = cmdsrc;
   if(gamestate == GS_LEVEL && players[cmdsrc].mo)
      context->invocationData.trigger = players[cmdsrc].mo;

   A_ExecScriptNameV(vm, c_argv[1]);

   A_ClearInvocation(context);
}

//
// a_execi
//
// Executes a script with any number of integer parameters
// from the console
//
CONSOLE_COMMAND(a_execi, cf_notnet)
{
   SmallContext_t *context;
   AMX *vm;
   cell *params;
   sc_vm_e vmNum;
   int i, argcount;

   if(c_argc < 3)
   {
      C_Printf(FC_ERROR "one or more parameters needed\n");
      return;
   }

   vmNum = atoi(c_argv[0]);

   switch(vmNum)
   {
   default:
   case SC_VM_GAMESCRIPT:
      if(!gameScriptLoaded)
      {
         C_Printf(FC_ERROR "game script not loaded\n");
         return;
      }
      context = &GameScript;
      vm = &GameScript.smallAMX;
      break;
   case SC_VM_LEVELSCRIPT:
      if(!levelScriptLoaded)
      {
         C_Printf(FC_ERROR "level script not loaded\n");
         return;
      }
      context = &LevelScript;
      vm = &LevelScript.smallAMX;
      break;
   }

   argcount = c_argc - 2;

   params = Z_Malloc(argcount * sizeof(cell), PU_STATIC, NULL);

   for(i = 2; i < c_argc; i++)
   {
      params[i-2] = (cell)(atoi(c_argv[i]));
   }

   context->invocationData.invokeType = SC_INVOKE_CCMD;
   context->invocationData.playernum  = cmdsrc;
   if(gamestate == GS_LEVEL && players[cmdsrc].mo)
      context->invocationData.trigger = players[cmdsrc].mo;

   A_ExecScriptNameI(vm, c_argv[1], argcount, params);

   A_ClearInvocation(context);

   Z_Free(params);
}

void A_AddCommands(void)
{
   C_AddCommand(a_running);
   C_AddCommand(a_execv);
   C_AddCommand(a_execi);
}

// local natives -- these classify as core functionality

//
// sm_invoketype
//
// Implements GetInvokeType()
//
static cell AMX_NATIVE_CALL sm_invoketype(AMX *amx, cell *params)
{
   return A_GetContextForAMX(amx)->invocationData.invokeType;
}

//
// sm_getCcmdSrc
//
// Implements GetCcmdSrc()
// *** valid only when invocation type is SC_INVOKE_CCMD ***
//
static cell AMX_NATIVE_CALL sm_getCcmdSrc(AMX *amx, cell *params)
{
   SmallContext_t *context = A_GetContextForAMX(amx);
   sc_invoke_t *invocation = &context->invocationData;

   if(invocation->invokeType != SC_INVOKE_CCMD)
   {
      amx_RaiseError(amx, SC_ERR_INVOKE | SC_ERR_MASK);
      return -1;
   }

   return invocation->playernum + 1;
}

//
// sm_getPlayerSrc
//
// Implements GetCcmdSrc()
// *** valid when invocation type is CCMD, PLAYER, DIALOGUE,
//     THING, LINE, or TERRAIN ***
//
static cell AMX_NATIVE_CALL sm_getPlayerSrc(AMX *amx, cell *params)
{
   SmallContext_t *context = A_GetContextForAMX(amx);
   sc_invoke_t *invocation = &context->invocationData;

   switch(invocation->invokeType)
   {
   case SC_INVOKE_CCMD:
   case SC_INVOKE_PLAYER:
   case SC_INVOKE_DIALOGUE:
      return invocation->playernum + 1;
   case SC_INVOKE_THING:
   case SC_INVOKE_LINE:
   case SC_INVOKE_TERRAIN:
      if(invocation->trigger && invocation->trigger->player)
         return (cell)((invocation->trigger->player) - players) + 1;
      else
         return -1;
   default:
      break;
   }

   amx_RaiseError(amx, SC_ERR_INVOKE | SC_ERR_MASK);
   return -1;
}

//
// sm_getThingSrc
//
// Returns the true TID of the trigger object, if it has one, otherwise
// returns TID_TRIGGER.
// If there is no trigger object, the function returns zero.  
//
static cell AMX_NATIVE_CALL sm_getThingSrc(AMX *amx, cell *params)
{
   SmallContext_t *context = A_GetContextForAMX(amx);
   mobj_t *mo;

   if((mo = context->invocationData.trigger))
   {
      if(mo->player) // is a player?
         return -((mo->player - players) + 1); // return -1 through -4

      // return true tid if object has one, else TID_TRIGGER
      return mo->tid != 0 ? mo->tid : -10;
   }

   return 0; // no trigger object
}

//
// sm_itoa
//
// Implements _Itoa(num, string[], base, packed)
//
static cell AMX_NATIVE_CALL sm_itoa(AMX *amx, cell *params)
{
   int err;
   int num;
   int base;
   int packed;
   cell *cstr;
   char smitoabuf[33];

   num    = (int)params[1];
   base   = (int)params[3];
   packed = (int)params[4];

   M_Itoa(num, smitoabuf, base);

   // translate reference parameter to physical address
   if((err = amx_GetAddr(amx, params[2], &cstr)) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   amx_SetString(cstr, smitoabuf, packed, 0);

   return 0;
}

//
// sm_setcallback
//
// Implements _SetCallback(scrname[], waittype, waitdata, flags)
//
static cell AMX_NATIVE_CALL sm_setcallback(AMX *amx, cell *params)
{
   int waittype;
   int waitdata;
   int waitflags;
   sc_vm_e vm;
   char *scrname;
   int err;
   SmallContext_t *context = A_GetContextForAMX(amx);

   vm = context->vm;

   // get script name
   if((err = A_GetSmallString(amx, &scrname, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return -1;
   }

   waittype  = (int)params[2];
   waitdata  = (int)params[3];
   waitflags = (int)params[4];

   if((err = A_AddCallback(scrname, vm, waittype, waitdata, waitflags))
           != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
   }

   Z_Free(scrname);

   return 0;
}

//
// exec_across
//
// Used by the native functions below to invoke a script in
// another AMX.
//
static cell exec_across(AMX *origAMX, cell *params, SmallContext_t *execContext)
{
   char *scrname;
   int err, numparams, retval;
   cell *scrparams;

   // get script name
   if((err = A_GetSmallString(origAMX, &scrname, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(origAMX, err);
      return 0;
   }

   // count of parameters to send to function (subtract one for fn)
   numparams = (params[0] / sizeof(cell)) - 1;
   
   if(numparams > 0)
   {
      scrparams = Z_Malloc(numparams * sizeof(cell), PU_STATIC, NULL);
      memcpy(scrparams, params + 1, numparams * sizeof(cell));

      retval = A_ExecScriptNameI(&execContext->smallAMX, scrname, 
                                 numparams, scrparams);
      
      Z_Free(scrparams);
   }
   else
   {
      retval = A_ExecScriptNameV(&execContext->smallAMX, scrname);
   }

   Z_Free(scrname);

   return retval;
}

//
// sm_execgs
//
// Implements ExecGS(fnname[], ...)
// Allows level scripts to call game scripts
//
static cell AMX_NATIVE_CALL sm_execgs(AMX *amx, cell *params)
{
   SmallContext_t *LScontext = A_GetContextForAMX(amx);
   SmallContext_t *GScontext = curGSContext;
   SmallContext_t *useContext;
   SmallContext_t newGSContext;
   cell retval;

   // cannot call from within the gamescript
   if(LScontext->vm == SC_VM_GAMESCRIPT)
   {
      amx_RaiseError(amx, SC_ERR_REENTRANT | SC_ERR_MASK);
      return -1;
   }

   // game script must be loaded
   if(!gameScriptLoaded)
   {
      amx_RaiseError(amx, SC_ERR_BADVM | SC_ERR_MASK);
      return -1;
   }

   // possibly create a child context of the Gamescript
   useContext = A_CreateChildContext(GScontext, &newGSContext);

   // copy the Levelscript's invocation data to the current context
   memcpy(&useContext->invocationData, &LScontext->invocationData,
          sizeof(sc_invoke_t));

   // execute the script
   retval = exec_across(amx, params, useContext);

   // clear the invocation data
   A_ClearInvocation(useContext);

   // destroy any child context that might have been created
   A_DestroyChildContext(useContext);

   // return the script's return value
   return retval;
}

//
// sm_execls
//
// Implements ExecLS(fnname[], ...)
// Allows game scripts to call level scripts
//
static cell AMX_NATIVE_CALL sm_execls(AMX *amx, cell *params)
{
   SmallContext_t *GScontext = A_GetContextForAMX(amx);
   SmallContext_t *LScontext = curLSContext;
   SmallContext_t *useContext;
   SmallContext_t newLSContext;
   int retval;

   // cannot call from within the levelscript
   if(GScontext->vm == SC_VM_LEVELSCRIPT)
   {
      amx_RaiseError(amx, SC_ERR_REENTRANT | SC_ERR_MASK);
      return -1;
   }

   // level script must be loaded
   if(!levelScriptLoaded)
   {
      amx_RaiseError(amx, SC_ERR_BADVM | SC_ERR_MASK);
      return -1;
   }

   // possibly create a child context of the Levelscript
   useContext = A_CreateChildContext(LScontext, &newLSContext);

   // copy the Gamescript's invocation data to the current context
   memcpy(&useContext->invocationData, &GScontext->invocationData,
          sizeof(sc_invoke_t));

   // execute the script
   retval = exec_across(amx, params, useContext);

   // clear the invocation data
   A_ClearInvocation(useContext);

   // destroy any child context that might have been created
   A_DestroyChildContext(useContext);

   // return the script's return value
   return retval;
}

static cell A_GetPublicVarValue(AMX *execAMX, AMX *varAMX, cell nameaddr)
{
   int err;
   char *varname;
   cell varaddr;
   cell *physvaraddr;

   // get name of public variable from the executing AMX
   if((err = A_GetSmallString(execAMX, &varname, nameaddr)) != AMX_ERR_NONE)
   {
      amx_RaiseError(execAMX, err);
      return -1;
   }

   // find public variable in the var AMX
   if((err = amx_FindPubVar(varAMX, varname, &varaddr)) != AMX_ERR_NONE)
   {
      amx_RaiseError(execAMX, err);
      Z_Free(varname);
      return -1;
   }

   // done with varname
   Z_Free(varname);

   // resolve AMX address to physical address
   if((err = amx_GetAddr(varAMX, varaddr, &physvaraddr)) != AMX_ERR_NONE)
   {
      amx_RaiseError(execAMX, err);
      return -1;
   }

   // return value of variable
   return *physvaraddr;
}

static cell A_SetPublicVarValue(AMX *execAMX, AMX *varAMX, cell nameaddr, cell value)
{
   int err;
   char *varname;
   cell varaddr;
   cell *physvaraddr;

   // get name of public variable from the executing AMX
   if((err = A_GetSmallString(execAMX, &varname, nameaddr)) != AMX_ERR_NONE)
   {
      amx_RaiseError(execAMX, err);
      return -1;
   }

   // find public variable in the var AMX
   if((err = amx_FindPubVar(varAMX, varname, &varaddr)) != AMX_ERR_NONE)
   {
      amx_RaiseError(execAMX, err);
      Z_Free(varname);
      return -1;
   }
   
   // done with varname
   Z_Free(varname);

   // resolve AMX address to physical address
   if((err = amx_GetAddr(varAMX, varaddr, &physvaraddr)) != AMX_ERR_NONE)
   {
      amx_RaiseError(execAMX, err);
      return -1;
   }   

   // set value of variable
   return (*physvaraddr = value);
}

static cell AMX_NATIVE_CALL sm_getlevelvar(AMX *amx, cell *params)
{
   AMX *lsamx;

   if(!levelScriptLoaded)
   {
      amx_RaiseError(amx, SC_ERR_BADVM | SC_ERR_MASK);
      return -1;
   }

   // get AMX containing variable
   lsamx = &(curLSContext->smallAMX);

   return A_GetPublicVarValue(amx, lsamx, params[1]);
}

static cell AMX_NATIVE_CALL sm_setlevelvar(AMX *amx, cell *params)
{
   AMX *lsamx;

   if(!levelScriptLoaded)
   {
      amx_RaiseError(amx, SC_ERR_BADVM | SC_ERR_MASK);
      return -1;
   }

   // get AMX containing variable
   lsamx = &(curLSContext->smallAMX);

   return A_SetPublicVarValue(amx, lsamx, params[1], params[2]);
}

static cell AMX_NATIVE_CALL sm_getgamevar(AMX *amx, cell *params)
{
   AMX *gsamx;

   if(!gameScriptLoaded)
   {
      amx_RaiseError(amx, SC_ERR_BADVM | SC_ERR_MASK);
      return -1;
   }
   
   // get AMX containing variable
   gsamx = &(curGSContext->smallAMX);

   return A_GetPublicVarValue(amx, gsamx, params[1]);
}

static cell AMX_NATIVE_CALL sm_setgamevar(AMX *amx, cell *params)
{
   AMX *gsamx;

   if(!gameScriptLoaded)
   {
      amx_RaiseError(amx, SC_ERR_BADVM | SC_ERR_MASK);
      return -1;
   }

   // get AMX containing variable
   gsamx = &(curGSContext->smallAMX);

   return A_SetPublicVarValue(amx, gsamx, params[1], params[2]);
}

//
// Small printf stuff
//

static qstring_t small_qstr;

static int printstring(AMX *amx,cell *cstr, cell *params, int num);

static int dochar(AMX *amx, char ch, cell param)
{
   cell *cptr;
   char intbuffer[33];
   
   switch(ch)
   {
   case '%':
      M_QStrPutc(&small_qstr, ch);
      return 0;
   case 'c':
      amx_GetAddr(amx, param, &cptr);
      M_QStrPutc(&small_qstr, (char)*cptr);
      return 1;
   case 'd':
      amx_GetAddr(amx, param, &cptr);
      M_Itoa((int)*cptr, intbuffer, 10);
      M_QStrCat(&small_qstr, intbuffer);
      return 1;
   case 's':
      amx_GetAddr(amx, param, &cptr);
      printstring(amx, cptr, NULL, 0);
      return 1;
   // TODO: fixed-point number support
   default:
      break;
   } /* switch */

   /* error in the string format, try to repair */
   M_QStrPutc(&small_qstr, ch);
   return 0;
}

static int printstring(AMX *amx, cell *cstr, cell *params, int num)
{
   int i;
   int informat = 0, paramidx = 0;
   
   /* check whether this is a packed string */
   if((ucell)*cstr > UCHAR_MAX)
   {
      int j = sizeof(cell) - sizeof(char);
      char c;

      /* the string is packed */
      i = 0;
      for(;;)
      {
         c = (char)((ucell)cstr[i] >> 8 * j);
         if(c == 0)
            break;

         if(informat)
         {
            paramidx += dochar(amx, c, params[paramidx]);
            informat=0;
         } 
         else if(params != NULL && c == '%')
         {
            informat = 1;
         } 
         else
         {
            M_QStrPutc(&small_qstr, c);
         } /* if */
         if(j == 0)
            i++;
         j = (j + sizeof(cell) - sizeof(char)) % sizeof(cell);
      } /* for */
   } 
   else 
   {
      /* the string is unpacked */
      for(i = 0; cstr[i] != 0; ++i)
      {
         if(informat)
         {
            paramidx += dochar(amx, (char)cstr[i], params[paramidx]);
            informat = 0;
         } 
         else if(params != NULL && (int)cstr[i] == '%')
         {
            if(paramidx < num)
               informat = 1;
            else
               amx_RaiseError(amx, AMX_ERR_NATIVE);
         } 
         else
         {
            M_QStrPutc(&small_qstr, (char)cstr[i]);
         } /* if */
      } /* for */
   } /* if */

   return paramidx;
}

static cell AMX_NATIVE_CALL sm_printf(AMX *amx, cell *params)
{
   static boolean first = true;
   int destination;
   cell *cstr;
   const char *msg;

   // first time: initialize the qstring
   if(first)
   {
      M_QStrCreate(&small_qstr);
      first = false;
   }

   // get destination
   destination = (int)params[1];
   
   // print the formatted string into the qstring
   amx_GetAddr(amx, params[2], &cstr);
   printstring(amx, cstr, params+3, (int)(params[0] / sizeof(cell)) - 2);

   // now, display the qstring appropriately
   msg = M_QStrBuffer(&small_qstr);

   switch(destination)
   {
   default:
   case 0: // console
      C_Printf(msg);
      break;
   case 1: // message
      doom_printf(msg);
      break;
   case 2: // center message
      HU_CenterMessage(msg);
      break;
   }

   // clear the qstring
   M_QStrClear(&small_qstr);

   return 0;
}

AMX_NATIVE_INFO local_Natives[] =
{
   { "_GetInvokeType", sm_invoketype },
   { "_GetCcmdSrc",    sm_getCcmdSrc },
   { "_GetPlayerSrc",  sm_getPlayerSrc },
   { "_GetThingSrc",   sm_getThingSrc },
   { "_Itoa",          sm_itoa },
   { "_SetCallback",   sm_setcallback },
   { "_ExecGS",        sm_execgs },
   { "_ExecLS",        sm_execls },
   { "_GetLevelVar",   sm_getlevelvar },
   { "_SetLevelVar",   sm_setlevelvar },
   { "_GetGameVar",    sm_getgamevar },
   { "_SetGameVar",    sm_setgamevar },
   { "_Printf",        sm_printf },
   { NULL, NULL }
};

// EOF


