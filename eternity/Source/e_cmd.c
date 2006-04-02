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
// EDF- and ExtraData-Related Console Commands and Script Functions
//
// By James Haley
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "info.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "a_small.h"
#include "e_things.h"
#include "e_exdata.h"

//
// e_dumpthings
//
// Lists all the EDF thing type mnemonics along with their DeHackEd
// numbers and doomednums
//
CONSOLE_COMMAND(e_dumpthings, 0)
{
   int i;

   C_Printf("deh#\ted#\tname\n");

   for(i = 0; i < NUMMOBJTYPES; ++i)
   {
      C_Printf("%5d\t%5d\t%s\n", 
               mobjinfo[i].dehnum,
               mobjinfo[i].doomednum,
               mobjinfo[i].name);
   }
}

CONSOLE_COMMAND(e_thingtype, 0)
{
   int num;

   if(!c_argc)
   {
      C_Printf("usage: e_thingtype mnemonic\n");
      return;
   }

   num = E_ThingNumForName(c_argv[0]);

   if(num == NUMMOBJTYPES)
   {
      C_Printf("Thing type not found\n");
      return;
   }

   C_Printf(FC_HI "Data for Thing Type %s:\n"
            FC_ERROR "ID Data:\n"
            FC_HI "Next by name: " FC_NORMAL "%d\n"
            FC_HI "DeHackEd #: " FC_NORMAL "%d\n"
            FC_HI "Next by DeHackEd #: " FC_NORMAL "%d\n"
            FC_HI "DoomEd #: " FC_NORMAL "%d\n\n",
            mobjinfo[num].name, mobjinfo[num].namenext,
            mobjinfo[num].dehnum, mobjinfo[num].dehnext,
            mobjinfo[num].doomednum);

   C_Printf(FC_ERROR "State Data:\n"
            FC_HI "Spawn state: " FC_NORMAL "%d\n"
            FC_HI "See state: " FC_NORMAL "%d\n"
            FC_HI "Melee state: " FC_NORMAL "%d\n"
            FC_HI "Missile state: " FC_NORMAL "%d\n"
            FC_HI "Pain state: " FC_NORMAL "%d\n"
            FC_HI "Death state: " FC_NORMAL "%d\n"
            FC_HI "XDeath state: " FC_NORMAL "%d\n"
            FC_HI "Crash state: " FC_NORMAL "%d\n"
            FC_HI "Raise state: " FC_NORMAL "%d\n\n",
            mobjinfo[num].spawnstate, mobjinfo[num].seestate,
            mobjinfo[num].meleestate, mobjinfo[num].missilestate,
            mobjinfo[num].painstate, mobjinfo[num].deathstate,
            mobjinfo[num].xdeathstate, mobjinfo[num].crashstate,
            mobjinfo[num].raisestate);

   C_Printf(FC_ERROR "Sound data:\n"
            FC_HI "See sound: " FC_NORMAL "%d\n"
            FC_HI "Active sound: " FC_NORMAL "%d\n"
            FC_HI "Attack sound: " FC_NORMAL "%d\n"
            FC_HI "Pain sound: " FC_NORMAL "%d\n"
            FC_HI "Death sound: " FC_NORMAL "%d\n\n",
            mobjinfo[num].seesound, mobjinfo[num].activesound,
            mobjinfo[num].attacksound, mobjinfo[num].painsound,
            mobjinfo[num].deathsound);

   C_Printf(FC_ERROR "Metrics:\n"
            FC_HI "Spawnhealth: " FC_NORMAL "%d\n"
            FC_HI "Painchance: " FC_NORMAL "%d\n"
            FC_HI "Droptype: " FC_NORMAL "%d\n"
            FC_HI "Radius: " FC_NORMAL "%d\n"
            FC_HI "Height: " FC_NORMAL "%d\n"
            FC_HI "Mass: " FC_NORMAL "%d\n"
            FC_HI "Speed: " FC_NORMAL "%d\n"
            FC_HI "Reaction time: " FC_NORMAL "%d\n\n",
            mobjinfo[num].spawnhealth, mobjinfo[num].painchance,
            mobjinfo[num].droptype, mobjinfo[num].radius >> FRACBITS,
            mobjinfo[num].height >> FRACBITS, mobjinfo[num].mass,
            mobjinfo[num].speed, mobjinfo[num].reactiontime);

   C_Printf(FC_ERROR "Damage data:\n"
            FC_HI "Damage: " FC_NORMAL "%d\n"
            FC_HI "Damage special: " FC_NORMAL "%d\n"
            FC_HI "Means of Death: " FC_NORMAL "%d\n"
            FC_HI "Obituary: " FC_NORMAL "%s\n"
            FC_HI "Melee Obituary: " FC_NORMAL "%s\n\n",
            mobjinfo[num].damage, mobjinfo[num].dmgspecial,
            mobjinfo[num].mod, 
            mobjinfo[num].obituary ? mobjinfo[num].obituary : "none",
            mobjinfo[num].meleeobit ? mobjinfo[num].meleeobit : "none");

   C_Printf(FC_ERROR "Graphics data:\n"
            FC_HI "Skin sprite: " FC_NORMAL "%d\n"
            FC_HI "Blood color: " FC_NORMAL "%d\n"
            FC_HI "Particle FX: " FC_NORMAL "%xH\n"
            FC_HI "Translucency: " FC_NORMAL "%d\n\n",
            mobjinfo[num].altsprite, mobjinfo[num].bloodcolor,
            mobjinfo[num].colour, mobjinfo[num].particlefx,
            mobjinfo[num].translucency);

   C_Printf(FC_ERROR "Flags:\n"
            FC_HI "Flags 1: " FC_NORMAL "%xH\n"
            FC_HI "Flags 2: " FC_NORMAL "%xH\n"
            FC_HI "Flags 3: " FC_NORMAL "%xH\n",
            mobjinfo[num].flags, mobjinfo[num].flags2,
            mobjinfo[num].flags3);
}

//
// e_dumpitems
//
// As above, but filters for objects that have the MF_SPECIAL
// flag. This is useful in concert with the "give" command.
//
CONSOLE_COMMAND(e_dumpitems, 0)
{
   int i;

   C_Printf("deh#\ted#\tname\n");

   for(i = 0; i < NUMMOBJTYPES; ++i)
   {
      if(mobjinfo[i].flags & MF_SPECIAL)
      {
         C_Printf("%5d\t%5d\t%s\n",
                  mobjinfo[i].dehnum,
                  mobjinfo[i].doomednum,
                  mobjinfo[i].name);
      }
   }
}

CONSOLE_COMMAND(e_listmapthings, cf_level)
{
   mapthingext_t *things;
   int i, numthings;

   E_GetEDMapThings(&things, &numthings);

   if(!numthings)
   {
      C_Printf("No ExtraData mapthings defined\n");
      return;
   }

   C_Printf("rec\tnext\ttype\ttid\n");

   for(i = 0; i < numthings; ++i)
   {
      C_Printf("%5d\t%5d\t%5d\n", 
               things[i].recordnum, things[i].next, 
               things[i].stdfields.type, things[i].tid);
   }
}

CONSOLE_COMMAND(e_mapthing, cf_level)
{
   mapthingext_t *things;
   int i, numthings, recordnum;

   if(!c_argc)
   {
      C_Printf("usage: e_mapthing recordnum\n");
      return;
   }

   recordnum = atoi(c_argv[0]);

   E_GetEDMapThings(&things, &numthings);

   if(!numthings)
   {
      C_Printf("No ExtraData mapthings defined\n");
      return;
   }

   for(i = 0; i < numthings; ++i)
   {
      if(things[i].recordnum == recordnum)
      {
         C_Printf(FC_HI "Data for ED Mapthing %d:\n"
                  FC_HI "Next record: " FC_NORMAL "%d\n"
                  FC_HI "Type: " FC_NORMAL "%d\n"
                  FC_HI "Options: " FC_NORMAL "%d\n"
                  FC_HI "Tid: " FC_NORMAL "%d\n"
                  FC_HI "Args: " FC_NORMAL "%d, %d, %d, %d, %d\n",
                  things[i].recordnum,
                  things[i].next,
                  things[i].stdfields.type,
                  things[i].stdfields.options,
                  things[i].tid,
                  things[i].args[0], things[i].args[1],
                  things[i].args[2], things[i].args[3],
                  things[i].args[4]);
         return;
      }
   }
   C_Printf("Record not found\n");
}

CONSOLE_COMMAND(e_listlinedefs, cf_level)
{
   maplinedefext_t *lines;
   int i, numlines;

   E_GetEDLines(&lines, &numlines);

   if(!numlines)
   {
      C_Printf("No ExtraData linedefs defined\n");
      return;
   }

   C_Printf("rec\tnext\tspec\ttag\n");

   for(i = 0; i < numlines; ++i)
   {
      C_Printf("%5d\t%5d\t%5d\t%5d\n", 
               lines[i].recordnum, lines[i].next, 
               lines[i].stdfields.special,
               lines[i].stdfields.tag);
   }
}

CONSOLE_COMMAND(e_linedef, cf_level)
{
   maplinedefext_t *lines;
   int i, numlines, recordnum;

   if(!c_argc)
   {
      C_Printf("usage: e_linedef recordnum\n");
      return;
   }

   recordnum = atoi(c_argv[0]);

   E_GetEDLines(&lines, &numlines);

   if(!numlines)
   {
      C_Printf("No ExtraData linedefs defined\n");
      return;
   }

   for(i = 0; i < numlines; ++i)
   {
      if(lines[i].recordnum == recordnum)
      {
         C_Printf(FC_HI "Data for ED Linedef %d:\n"
                  FC_HI "Next record: " FC_NORMAL "%d\n"
                  FC_HI "Special: " FC_NORMAL "%d\n"
                  FC_HI "Extflags: " FC_NORMAL "%d\n"
                  FC_HI "Tag: " FC_NORMAL "%d\n"
                  FC_HI "Args: " FC_NORMAL "%d, %d, %d, %d, %d\n",
                  lines[i].recordnum,
                  lines[i].next,
                  lines[i].stdfields.special,
                  lines[i].extflags,
                  lines[i].stdfields.tag,
                  lines[i].args[0], lines[i].args[1],
                  lines[i].args[2], lines[i].args[3],
                  lines[i].args[4]);
         return;
      }
   }
   C_Printf("Record not found\n");
}

//
// E_AddCommands
//
// Adds the commands to the command list
//
void E_AddCommands(void)
{
   C_AddCommand(e_dumpthings);
   C_AddCommand(e_thingtype);
   C_AddCommand(e_dumpitems);
   C_AddCommand(e_listmapthings);
   C_AddCommand(e_mapthing);
   C_AddCommand(e_listlinedefs);
   C_AddCommand(e_linedef);
}

//
// Script functions
//

static cell AMX_NATIVE_CALL sm_thingnumforname(AMX *amx, cell *params)
{
   char *buff;
   int num, err;

   if((err = A_GetSmallString(amx, &buff, params[1])) != AMX_ERR_NONE)
   {
      amx_RaiseError(amx, err);
      return 0;
   }

   num = E_SafeThingName(buff);

   Z_Free(buff);

   return num;
}

static cell AMX_NATIVE_CALL sm_thingnumfordehnum(AMX *amx, cell *params)
{
   return E_SafeThingType(params[1]);
}

static cell AMX_NATIVE_CALL sm_unknownthing(AMX *amx, cell *params)
{
   return UnknownThingType;
}

AMX_NATIVE_INFO edf_Natives[] =
{
   { "_ThingNumForName",   sm_thingnumforname },
   { "_ThingNumForDEHNum", sm_thingnumfordehnum },
   { "_ThingUnknown",      sm_unknownthing },
   { NULL,                NULL }
};

// EOF

