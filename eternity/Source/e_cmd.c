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
// EDF-Related Console Commands and Script Functions
//
// By James Haley
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "info.h"
#include "c_io.h"
#include "c_runcmd.h"
#include "a_small.h"
#include "e_edf.h"

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

//
// E_AddCommands
//
// Adds the commands to the command list
//
void E_AddCommands(void)
{
   C_AddCommand(e_dumpthings);
   C_AddCommand(e_dumpitems);
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
   return E_UnknownThing();
}

AMX_NATIVE_INFO edf_Natives[] =
{
   { "ThingNumForName",   sm_thingnumforname },
   { "ThingNumForDEHNum", sm_thingnumfordehnum },
   { "ThingUnknown",      sm_unknownthing },
   { NULL,                NULL }
};

// EOF

