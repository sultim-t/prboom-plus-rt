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
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __T_SCRIPT_H__
#define __T_SCRIPT_H__

typedef struct runningscript_s runningscript_t;

#include "p_mobj.h"
#include "t_parse.h"

struct runningscript_s
{
  script_t *script;
  
  // where we are
  char *savepoint;

  enum
  {
    wt_none,        // not waiting
    wt_delay,       // wait for a set amount of time
    wt_tagwait,     // wait for sector to stop moving
    wt_scriptwait,  // wait for script to finish
  } wait_type;
  int wait_data;  // data for wait: tagnum, counter, script number etc
	
  // saved variables
  svariable_t *variables[VARIABLESLOTS];
  
  runningscript_t *prev, *next;  // for chain
  mobj_t *trigger;
};

void T_Init();
void T_ClearScripts();
void T_RunScript(int n);
void T_RunThingScript(int);
void T_PreprocessScripts();
void T_DelayedScripts();
mobj_t *MobjForSvalue(svalue_t svalue);

        // console commands
void T_Dump();
void T_ConsRun();

extern script_t levelscript;
//extern script_t *scripts[MAXSCRIPTS];       // the scripts
extern mobj_t *t_trigger;

#endif
