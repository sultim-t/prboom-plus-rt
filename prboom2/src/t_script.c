/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
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
 *
 * scripting.
 *
 * delayed scripts, running scripts, console cmds etc in here
 * the interface between FraggleScript and the rest of the game
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#include "doomstat.h"
#include "c_io.h"
#include "c_net.h"
#include "c_runcmd.h"
#include "p_info.h"
#include "p_mobj.h"
#include "p_spec.h"
#include "w_wad.h"
#include "z_zone.h"

#include "t_script.h"
#include "t_parse.h"
#include "t_vari.h"
#include "t_func.h"

void clear_runningscripts();

//                  script tree:
//
//                     global_script
//                  /                 \.
//           hubscript                 thingscript
//          /         \                  /     \.
//    levelscript    [levelscript]    ... scripts ...
//     /      \          /      \.
//  ... scripts...   ... scripts ...
//

// the level script is just the stuff put in the wad,
// which the other scripts are derivatives of
script_t levelscript;

// the thing script is the THINGSCR wad lump which
// is used to define scripts for the dehacked
// startscript codepointer
script_t thingscript;

// the individual scripts
//script_t *scripts[MAXSCRIPTS];       // the scripts
mobj_t *t_trigger;

runningscript_t runningscripts;        // first in chain

//     T_Init()
//
//    called at program start

void T_Init()
{
  init_variables();
  init_functions();
}

//
// T_ClearScripts()
//
// called at level start, clears all scripts
//

void T_ClearScripts()
{
  int i;
  
  // stop runningscripts
  clear_runningscripts();

  // clear the levelscript
  levelscript.data = Z_Malloc(5, PU_LEVEL, 0);  // empty data
  levelscript.data[0] = '\0';
  
  levelscript.scriptnum = -1;
  levelscript.parent = &hub_script;

  // clear levelscript variables
  
  for(i=0; i<VARIABLESLOTS; i++)
    {
      levelscript.variables[i] = NULL;
    }
}

void T_LoadThingScript()
{
  const char *scriptlump;
  int lumpnum, lumplen;
  
  if(thingscript.data)
    Z_Free(thingscript.data);

  // load lump into thingscript.data

  // get lumpnum, lumplen
  
  lumpnum = W_CheckNumForName("THINGSCR");
  if(lumpnum == -1)
    return;
  
  lumplen = W_LumpLength(lumpnum);

  // alloc space for lump and copy lump data into it
  
  thingscript.data = Z_Malloc(lumplen+10, PU_STATIC, 0);
  scriptlump = W_CacheLumpNum(lumpnum);

  memcpy(thingscript.data, scriptlump, lumplen);

  W_UnlockLumpNum(lumpnum);

  // add '\0' to end of string

  thingscript.data[lumplen] = '\0';

  // preprocess script

  preprocess(&thingscript);

  // run script

  thingscript.trigger = players[0].mo;
  run_script(&thingscript);  
}

void T_PreprocessScripts()
{
  // run the levelscript first
  // get the other scripts
  
  // levelscript started by player 0 'superplayer'
  levelscript.trigger = players[0].mo;
  
  preprocess(&levelscript);
  run_script(&levelscript);

  // load and run the thing script

  T_LoadThingScript();
}

void T_RunScript(int n)
{
  script_t *script;
  
  if(n<0 || n>=MAXSCRIPTS) return;

  // use the level's child script script n
  script = levelscript.children[n];
  if(!script) return;
 
  script->trigger = t_trigger;    // save trigger in script
  
  run_script(script);
}

// T_RunThingScript:
// identical to T_RunScript but runs a script
// from the thingscript list rather than the
// levelscript list

void T_RunThingScript(int n)
{
  script_t *script;
  
  if(n<0 || n>=MAXSCRIPTS) return;

  // use the level's child script script n
  script = thingscript.children[n];
  if(!script) return;
 
  script->trigger = t_trigger;    // save trigger in script
  
  run_script(script);
}

// console scripting debugging commands

CONSOLE_COMMAND(t_dump, 0)
{
  script_t *script;
  
  if(!c_argc)
    {
      C_Printf("usage: T_DumpScript <scriptnum>\n");
      return;
    }

  if(!strcmp(c_argv[0], "global"))
    script = &levelscript;
  else
    script = levelscript.children[atoi(c_argv[0])];
  
  if(!script)
    {
      C_Printf("script '%s' not defined.\n", c_argv[0]);
      return;
    }
  
  C_Printf("%s\n", script->data);
}

CONSOLE_COMMAND(t_run, cf_level)
{
  int sn;
  
  if(!c_argc)
    {
      C_Printf("T_RunScript: run a script\n"
	       "usage: T_RunScript <script>\n");
      return;
    }
  
  sn = atoi(c_argv[0]);
  
  if(!levelscript.children[sn])
    {
      C_Printf("script not defined\n");
      return;
    }
  t_trigger = players[cmdsrc].mo;
  
  T_RunScript(sn);
}

/************************
         PAUSING SCRIPTS
 ************************/

runningscript_t *freelist=NULL;      // maintain a freelist for speed

runningscript_t *new_runningscript()
{
  // check the freelist
  if(freelist)
    {
      runningscript_t *returnv=freelist;
      freelist = freelist->next;
      return returnv;
    }
  
  // alloc static: can be used in other levels too
  return Z_Malloc(sizeof(runningscript_t), PU_STATIC, 0);
}

static void free_runningscript(runningscript_t *runscr)
{
  // add to freelist
  runscr->next = freelist;
  freelist = runscr;
}

static boolean wait_finished(runningscript_t *script)
{
  switch(script->wait_type)
    {
    case wt_none: return true;        // uh? hehe
    case wt_scriptwait:               // waiting for script to finish
      {
	runningscript_t *current;
	for(current = runningscripts.next; current; current = current->next)
	  {
	    if(current == script) continue;  // ignore this script
	    if(current->script->scriptnum == script->wait_data)
	      return false;        // script still running
	  }
	return true;        // can continue now
      }

    case wt_delay:                          // just count down
      {
	return --script->wait_data <= 0;
      }
    
    case wt_tagwait:
      {
	int secnum = -1;

	while ((secnum = P_FindSectorFromTag(script->wait_data, secnum)) >= 0)
	  {
	    sector_t *sec = &sectors[secnum];
	    if(sec->floordata || sec->ceilingdata || sec->lightingdata)
	      return false;        // not finished
	  }
	return true;
      }

    default: return true;
    }

  return false;
}

void T_DelayedScripts()
{
  runningscript_t *current, *next;
  int i;

  if(!info_scripts) return;       // no level scripts
  
  current = runningscripts.next;
  
  while(current)
    {
      if(wait_finished(current))
	{
	  // copy out the script variables from the
	  // runningscript_t

	  for(i=0; i<VARIABLESLOTS; i++)
	    current->script->variables[i] = current->variables[i];
	  current->script->trigger = current->trigger; // copy trigger
	  
	  // continue the script

	  continue_script(current->script, current->savepoint);
	  
	  // unhook from chain and free

	  current->prev->next = current->next;
	  if(current->next) current->next->prev = current->prev;
	  next = current->next;   // save before freeing
	  free_runningscript(current);
	}
      else
	next = current->next;
      current = next;   // continue to next in chain
    }
                
}

static runningscript_t *T_SaveCurrentScript()
{
  runningscript_t *runscr;
  int i;

  runscr = new_runningscript();
  runscr->script = current_script;
  runscr->savepoint = rover;

  // leave to other functions to set wait_type: default to wt_none
  runscr->wait_type = wt_none;

  // hook into chain at start
  
  runscr->next = runningscripts.next;
  runscr->prev = &runningscripts;
  runscr->prev->next = runscr;
  if(runscr->next)
    runscr->next->prev = runscr;
  
  // save the script variables 
  for(i=0; i<VARIABLESLOTS; i++)
    {
      runscr->variables[i] = current_script->variables[i];
      
      // remove all the variables from the script variable list
      // to prevent them being removed when the script stops

      while(current_script->variables[i] &&
	    current_script->variables[i]->type != svt_label)
	current_script->variables[i] =
	  current_script->variables[i]->next;
    }
  runscr->trigger = current_script->trigger;      // save trigger
  
  killscript = true;      // stop the script

  return runscr;
}

// script function

void SF_Wait()
{
  runningscript_t *runscr;

  if(t_argc != 1)
    {
      script_error("incorrect arguments to function\n");
      return;
    }

  runscr = T_SaveCurrentScript();

  runscr->wait_type = wt_delay;
  runscr->wait_data = (intvalue(t_argv[0]) * 35) / 100;
}

// wait for sector with particular tag to stop moving
void SF_TagWait()
{
  runningscript_t *runscr;

  if(t_argc != 1)
    {
      script_error("incorrect arguments to function\n");
      return;
    }

  runscr = T_SaveCurrentScript();

  runscr->wait_type = wt_tagwait;
  runscr->wait_data = intvalue(t_argv[0]);
}

// wait for a script to finish
void SF_ScriptWait()
{
  runningscript_t *runscr;

  if(t_argc != 1)
    {
      script_error("incorrect arguments to function\n");
      return;
    }

  runscr = T_SaveCurrentScript();

  runscr->wait_type = wt_scriptwait;
  runscr->wait_data = intvalue(t_argv[0]);
}

extern mobj_t *trigger_obj;           // in t_func.c

void SF_StartScript()
{
  runningscript_t *runscr;
  script_t *script;
  int i, snum;
  
  if(t_argc != 1)
    {
      script_error("incorrect arguments to function\n");
      return;
    }

  snum = intvalue(t_argv[0]);
  
  script = levelscript.children[snum];
  
  if(!script)
    {
      script_error("script %i not defined\n", snum);
    }
  
  runscr = new_runningscript();
  runscr->script = script;
  runscr->savepoint = script->data; // start at beginning
  runscr->wait_type = wt_none;      // start straight away

  // hook into chain at start
  
  runscr->next = runningscripts.next;
  runscr->prev = &runningscripts;
  runscr->prev->next = runscr;
  if(runscr->next)
    runscr->next->prev = runscr;
  
  // save the script variables 
  for(i=0; i<VARIABLESLOTS; i++)
    {
      runscr->variables[i] = script->variables[i];
      
      // in case we are starting another current_script:
      // remove all the variables from the script variable list
      // we only start with the basic labels
      while(runscr->variables[i] &&
	    runscr->variables[i]->type != svt_label)
	runscr->variables[i] =
	  runscr->variables[i]->next;
    }
  // copy trigger
  runscr->trigger = current_script->trigger;
}

void SF_ScriptRunning()
{
  runningscript_t *current;
  int snum;

  if(t_argc < 1)
    {
      script_error("not enough arguments to function\n");
      return;
    }

  snum = intvalue(t_argv[0]);
  
  for(current=runningscripts.next; current; current=current->next)
    {
      if(current->script->scriptnum == snum)
	{
	  // script found so return
	  t_return.type = svt_int;
	  t_return.value.i = 1;
	  return;
	}
    }

  // script not found
  t_return.type = svt_int;
  t_return.value.i = 0;
}

// running scripts

CONSOLE_COMMAND(t_running, 0)
{
  runningscript_t *current;
  
  current = runningscripts.next;
  
  C_Printf(FC_GRAY "running scripts\n" FC_RED);
  
  if(!current)
    C_Printf("no running scripts.\n");
  
  while(current)
    {
      C_Printf("%i:", current->script->scriptnum);
      switch(current->wait_type)
	{
	case wt_none:
	  C_Printf("waiting for nothing?\n");
	  break;
	case wt_delay:
	  C_Printf("delay %i tics\n", current->wait_data);
	  break;
	case wt_tagwait:
	  C_Printf("waiting for tag %i\n", current->wait_data);
	  break;
	case wt_scriptwait:
	  C_Printf("waiting for script %i\n", current->wait_data);
	  break;
	default:
	  C_Printf("unknown wait type \n");
	  break;
	}
      current = current->next;
    }
}

void clear_runningscripts()
{
  runningscript_t *runscr, *next;
  
  runscr = runningscripts.next;
  
  // free the whole chain
  while(runscr)
    {
      next = runscr->next;
      free_runningscript(runscr);
      runscr = next;
    }
  runningscripts.next = NULL;
}

mobj_t *MobjForSvalue(svalue_t svalue)
{
  int intval ;
  
  if(svalue.type == svt_mobj)
    return svalue.value.mobj;
  
  // this requires some creativity. We use the intvalue
  // as the thing number of a thing in the level.
  
  intval = intvalue(svalue);        
  
  if(intval < 0 || intval >= numthings)
    { script_error("no levelthing %i\n", intval); return NULL;}
  
  return spawnedthings[intval];
}


/*********************
            ADD SCRIPT
 *********************/

// when the level is first loaded, all the
// scripts are simply stored in the levelscript.
// before the level starts, this script is
// preprocessed and run like any other. This allows
// the individual scripts to be derived from the
// levelscript. When the interpreter detects the
// 'script' keyword this function is called

void spec_script()
{
  int scriptnum;
  int datasize;
  script_t *script;
  
  if(!current_section)
    {
      script_error("need seperators for script\n");
      return;
    }
  
  // presume that the first token is "script"
  
  if(num_tokens < 2)
    {
      script_error("need script number\n");
      return;
    }

  scriptnum = intvalue(evaluate_expression(1, num_tokens-1));
  
  if(scriptnum < 0)
    {
      script_error("invalid script number\n");
      return;
    }
  
  script = Z_Malloc(sizeof(script_t), PU_LEVEL, 0);

  // add to scripts list of parent
  current_script->children[scriptnum] = script;
  
  // copy script data
  // workout script size: -2 to ignore { and }
  datasize = current_section->end - current_section->start - 2;

  // alloc extra 10 for safety
  script->data = Z_Malloc(datasize+10, PU_LEVEL, 0);
 
  // copy from parent script (levelscript) 
  // ignore first char which is {
  memcpy(script->data, current_section->start+1, datasize);

  // tack on a 0 to end the string
  script->data[datasize] = '\0';
  
  script->scriptnum = scriptnum;
  script->parent = current_script; // remember parent
  
  // preprocess the script now
  preprocess(script);
    
  // restore current_script: usefully stored in new script
  current_script = script->parent;

  // rover may also be changed, but is changed below anyway
  
  // we dont want to run the script, only add it
  // jump past the script in parsing
  
  rover = current_section->end + 1;
}

/****** scripting command list *******/

void T_AddCommands()
{
  C_AddCommand(t_dump);
  C_AddCommand(t_run);
  C_AddCommand(t_running);
}
