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

// Invocation structure
//
// Holds some information pertinent to the way a script was
// started by the game engine. The informative fields of the
// structure are available as global values through native
// functions implemented below, provided the invocation type
// indicates they're valid at the time of call.
//
// Note no concurrency or consistency problems are created
// by this, unlike the problems with FraggleScript's built-in
// variables, since scripts cannot wait or be interrupted by
// savegames, and this information is only available at the
// time of call and not afterward for callbacks.
//
sc_invoke_t sc_invocation;

//
// The Game Script
//
// This is a persistent AMX that is always around, and is used
// for globally available scripts, such as those called from
// mobj frames, etc.
//
void *gameScriptData = NULL;
boolean gameScriptLoaded = false;

AMX gamescript;

//
// The Level Script
//
// This is a per-level AMX. Used for level-local scripts provided
// along with the map.
//
void *levelScriptData = NULL;
boolean levelScriptLoaded = false;

AMX levelscript;

//
// Initialization and Utilities
//

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

   memset(amx, 0, sizeof(*amx));

   return amx_Init(amx, memblock);
}

//
// A_AMXError
//
// Handles both internal AMX errors and implementation-defined
// errors caused by setup or native functions. Prints information
// to the console and makes a warning sound.
//
// 02/04/04: Rewritten to use new amx_StrError function
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
      errmsg = amx_StrError(err);

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
   amx_GetString(*dest, cellstr);

   return AMX_ERR_NONE;
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
void A_ClearInvocation(void)
{
   memset(&sc_invocation, 0, sizeof(sc_invoke_t));
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
   int index;

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
      Z_Free(gameScriptData);
      gameScriptData = NULL;
   }

   gameScriptData = Z_Malloc(memsize, PU_STATIC, NULL);

   error = A_AMXLoadProgram(&gamescript, "GAMESCR", gameScriptData);

   if(error != AMX_ERR_NONE)
   {
      A_AMXError(error);
      return;
   }

   // register native functions

   error = A_RegisterNatives(&gamescript);

   if(error != AMX_ERR_NONE)
   {
      A_AMXError(error);
      return;
   }

   // load was successful!
   gameScriptLoaded = true;

   // run the init script, if it exists
   if(amx_FindPublic(&gamescript, "OnInit", &index) == AMX_ERR_NONE)
   {
      sc_invocation.invokeType = SC_INVOKE_CALLBACK;
      A_ExecScriptV(&gamescript, index);
      A_ClearInvocation();
   }
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
   int index;

   levelScriptLoaded = false;

   // free any previously existing level script
   if(levelScriptData)
   {
      Z_Free(levelScriptData);
      levelScriptData = NULL;
   }

   if(!info_scripts) // current level has no scripts
      return;

   if(!(memsize = A_AMXProgramSize(info_scriptlump)))
   {
      A_AMXError(SC_ERR_READ | SC_ERR_MASK);
      return;
   }

   levelScriptData = Z_Malloc(memsize, PU_STATIC, NULL);

   error = A_AMXLoadProgram(&levelscript, info_scriptlump, 
                            levelScriptData);

   if(error != AMX_ERR_NONE)
   {
      A_AMXError(error);
      return;
   }

   // register native functions
   if((error = A_RegisterNatives(&levelscript)) != AMX_ERR_NONE)
   {
      A_AMXError(error);
      return;
   }

   // load was successful!
   levelScriptLoaded = true;

   // run the init script, if it exists
   if(amx_FindPublic(&levelscript, "OnInit", &index) == AMX_ERR_NONE)
   {
      sc_invocation.invokeType = SC_INVOKE_CALLBACK;
      A_ExecScriptV(&levelscript, index);
      A_ClearInvocation();
   }
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

//
// Callbacks
//

static sc_callback_t *currentcb;
static sc_callback_t callBackHead;

//
// A_AddCallback
//
// Schedules a callback for the given vm -- the indicated
// script will be called at the time indicated by its wait type
// and data. Callbacks are kept in a double-linked list.
//
int A_AddCallback(char *scrname, sc_vm_e vm, int waittype,
                  int waitdata)
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
      pvm = &gamescript;
      break;
   case SC_VM_LEVELSCRIPT:
      pvm = &levelscript;
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

   // insert callback at head of list (order is irrelevant)
   newCallback->prev = &callBackHead;
   newCallback->next = callBackHead.next;
   callBackHead.next->prev = newCallback;
   callBackHead.next = newCallback;
   
   return AMX_ERR_NONE;
}

//
// A_RemoveCallback
//
// Unlinks and frees the indicated callback structure.
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
//
static boolean A_WaitFinished(sc_callback_t *callback)
{
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
// immediately removed. Callbacks should be reregistered by the
// callback script if continuous timer-style execution is desired.
//
void A_ExecuteCallbacks(void)
{
   AMX *vm;
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
         vm = &gamescript;
         break;
      case SC_VM_LEVELSCRIPT:
         vm = &levelscript;
         break;
      }

      if(A_WaitFinished(currentcb))
      {
         // execute the callback
         sc_invocation.invokeType = SC_INVOKE_CALLBACK;

         if((err = amx_Exec(vm, &retval, currentcb->scriptNum, 0))
                 != AMX_ERR_NONE)
         {
            A_AMXError(err);
         }

         A_ClearInvocation();

         // remove this callback from the list
         A_RemoveCallback(currentcb);
      }
   }
}

//
// A_InitSmall
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

   for(cb = callBackHead.next; cb != &callBackHead; cb = cb->next)
   {
      C_Printf("callback: %d %d %d\n", cb->scriptNum,
               cb->wait_type, cb->wait_data);
   }
}

//
// a_execv
//
// Executes a script with no arguments from the console
//
CONSOLE_COMMAND(a_execv, cf_notnet)
{
   AMX *vm;

   if(c_argc < 2)
   {
      C_Printf("usage: a_execv vm scriptname\n");
      return;
   }

   switch(atoi(c_argv[0]))
   {
   default:
   case SC_VM_GAMESCRIPT:
      if(!gameScriptLoaded)
      {
         C_Printf(FC_ERROR "game script not loaded\n");
         return;
      }
      vm = &gamescript;
      break;
   case SC_VM_LEVELSCRIPT:
      if(!levelScriptLoaded)
      {
         C_Printf(FC_ERROR "level script not loaded\n");
         return;
      }
      vm = &levelscript;
      break;
   }

   sc_invocation.invokeType = SC_INVOKE_CCMD;
   sc_invocation.playernum  = cmdsrc;
   if(gamemode == GS_LEVEL && players[cmdsrc].mo)
      sc_invocation.trigger = players[cmdsrc].mo;

   A_ExecScriptNameV(vm, c_argv[1]);

   A_ClearInvocation();
}

//
// a_execi
//
// Executes a script with any number of integer parameters
// from the console
//
CONSOLE_COMMAND(a_execi, cf_notnet)
{
   AMX *vm;
   cell *params;
   int i, argcount;

   if(c_argc < 3)
   {
      C_Printf(FC_ERROR "one or more parameters needed\n");
      return;
   }

   switch(atoi(c_argv[0]))
   {
   default:
   case SC_VM_GAMESCRIPT:
      if(!gameScriptLoaded)
      {
         C_Printf(FC_ERROR "game script not loaded\n");
         return;
      }
      vm = &gamescript;
      break;
   case SC_VM_LEVELSCRIPT:
      if(!levelScriptLoaded)
      {
         C_Printf(FC_ERROR "level script not loaded\n");
         return;
      }
      vm = &levelscript;
      break;
   }

   argcount = c_argc - 2;

   params = Z_Malloc(argcount * sizeof(cell), PU_STATIC, NULL);

   for(i = 2; i < c_argc; i++)
   {
      params[i-2] = (cell)(atoi(c_argv[i]));
   }

   sc_invocation.invokeType = SC_INVOKE_CCMD;
   sc_invocation.playernum  = cmdsrc;
   if(gamemode == GS_LEVEL)
      sc_invocation.trigger = players[cmdsrc].mo;

   A_ExecScriptNameI(vm, c_argv[1], argcount, params);

   A_ClearInvocation();

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
   return sc_invocation.invokeType;
}

//
// sm_getCcmdSrc
//
// Implements GetCcmdSrc()
// *** valid only when invocation type is SC_INVOKE_CCMD ***
//
static cell AMX_NATIVE_CALL sm_getCcmdSrc(AMX *amx, cell *params)
{
   if(sc_invocation.invokeType != SC_INVOKE_CCMD)
   {
      amx_RaiseError(amx, SC_ERR_INVOKE | SC_ERR_MASK);
      return 0;
   }

   return sc_invocation.playernum + 1;
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
   switch(sc_invocation.invokeType)
   {
   case SC_INVOKE_CCMD:
   case SC_INVOKE_PLAYER:
   case SC_INVOKE_DIALOGUE:
      return sc_invocation.playernum + 1;
   case SC_INVOKE_THING:
   case SC_INVOKE_LINE:
   case SC_INVOKE_TERRAIN:
      if(sc_invocation.trigger && sc_invocation.trigger->player)
         return (cell)((sc_invocation.trigger->player) - players) + 1;
      else
         return -1;
   default:
      break;
   }

   amx_RaiseError(amx, SC_ERR_INVOKE | SC_ERR_MASK);
   return 0;
}

//
// sm_itoa
//
// Implements itoa(num, string[], base, packed)
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
      return 0;
   }

   amx_SetString(cstr, smitoabuf, packed);

   return 0;
}

//
// sm_setcallback
//
// Implements SetCallback(scrname[], waittype, waitdata)
//
static cell AMX_NATIVE_CALL sm_setcallback(AMX *amx, cell *params)
{
   int waittype;
   int waitdata;
   sc_vm_e vm;
   char *scrname;
   int err;

   if(amx == &levelscript)
      vm = SC_VM_LEVELSCRIPT;
   else
      vm = SC_VM_GAMESCRIPT;

   // get script name
   if((err = A_GetSmallString(amx, &scrname, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return 0;
   }

   waittype = (int)params[2];
   waitdata = (int)params[3];

   if((err = A_AddCallback(scrname, vm, waittype, waitdata))
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
static cell exec_across(AMX *amx, cell *params)
{
   char *scrname;
   int err, numparams, retval;
   cell *scrparams;

   // check sc_invocation to avoid double reentrancy
   if(sc_invocation.execd)
   {
      amx_RaiseError(amx, SC_ERR_REENTRANT | SC_ERR_MASK);
      return 0;
   }

   // get script name
   if((err = A_GetSmallString(amx, &scrname, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return 0;
   }

   // count of parameters to send to function (subtract one for fn)
   numparams = (params[0] / sizeof(cell)) - 1;
   
   if(numparams > 0)
   {
      scrparams = Z_Malloc(numparams * sizeof(cell), PU_STATIC, NULL);
      memcpy(scrparams, params + 1, numparams * sizeof(cell));

      sc_invocation.execd = true;
      retval = A_ExecScriptNameI(amx, scrname, numparams, scrparams);
      sc_invocation.execd = false;
      
      Z_Free(scrparams);
   }
   else
   {
      sc_invocation.execd = true;
      retval = A_ExecScriptNameV(amx, scrname);
      sc_invocation.execd = false;
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
   // cannot call from within the gamescript (no point anyways...)
   if(amx == &gamescript)
   {
      amx_RaiseError(amx, SC_ERR_REENTRANT | SC_ERR_MASK);
      return 0;
   }

   // game script must be loaded
   if(!gameScriptLoaded)
   {
      amx_RaiseError(amx, SC_ERR_BADVM | SC_ERR_MASK);
      return 0;
   }

   return exec_across(amx, params);
}

//
// sm_execls
//
// Implements ExecLS(fnname[], ...)
// Allows game scripts to call level scripts
//
static cell AMX_NATIVE_CALL sm_execls(AMX *amx, cell *params)
{
   // cannot call from within the levelscript (no point anyways...)
   if(amx == &levelscript)
   {
      amx_RaiseError(amx, SC_ERR_REENTRANT | SC_ERR_MASK);
      return 0;
   }

   // level script must be loaded
   if(!levelScriptLoaded)
   {
      amx_RaiseError(amx, SC_ERR_BADVM | SC_ERR_MASK);
      return 0;
   }

   return exec_across(amx, params);
}

AMX_NATIVE_INFO local_Natives[] =
{
   { "GetInvokeType", sm_invoketype },
   { "GetCcmdSrc", sm_getCcmdSrc },
   { "GetPlayerSrc", sm_getPlayerSrc },
   { "itoa", sm_itoa },
   { "SetCallback", sm_setcallback },
   { "ExecGS", sm_execgs },
   { "ExecLS", sm_execls },
   { NULL, NULL }
};

// EOF


