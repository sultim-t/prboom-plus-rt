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
//--------------------------------------------------------------------------
//
// Deathmatch Flags
//
// By James Haley
//
//---------------------------------------------------------------------------

#include "z_zone.h"
#include "g_dmflag.h"
#include "c_net.h"
#include "c_runcmd.h"
#include "c_io.h"

// The Deathmatch Flags variable
unsigned long dmflags;

// Defaults with which the game was started
// (Needed to preserve through demos et al.)
unsigned long default_dmflags;

//
// G_SetDefaultDMFlags
//
// Sets dmflags to the default setting for the given game type,
// for compatibility purposes. Propagates the value to
// default_dmflags if "setdefault" is true.
//
void G_SetDefaultDMFlags(int dmtype, boolean setdefault)
{
   if(GameType == gt_single)
      dmflags = DMD_SINGLE;
   else if(GameType == gt_coop)
      dmflags = DMD_COOP;
   else
   {
      switch(dmtype)
      {
      default:
      case 1:
         dmflags = DMD_DEATHMATCH;
         break;
      case 2:
         dmflags = DMD_DEATHMATCH2;
         break;
      case 3:
         dmflags = DMD_DEATHMATCH3;
         break;
      }
   }

   // optionally propagate value to default_dmflags
   if(setdefault)
      default_dmflags = dmflags;
}

VARIABLE_INT(dmflags, &default_dmflags, 0, D_MAXINT, NULL);
CONSOLE_NETVAR(dmflags, dmflags, cf_server, netcmd_dmflags) {}

// allows setting of default flags for a certain game mode

CONSOLE_COMMAND(defdmflags, cf_server)
{
   int mode;
   unsigned long flags = 0;
   char cmdbuf[64];
   const char *gm;

   if(!c_argc)
   {
      // no argument means set to default for current game type
      switch(GameType)
      {
      default:
      case gt_single:
         mode = 0;
         break;
      case gt_coop:
         mode = 1;
         break;
      case gt_dm:
         mode = 2;
         break;
      }
   }
   else
      mode = atoi(c_argv[0]);

   switch(mode)
   {
   default:
   case 0: // SP
      gm = "single player";
      break;
   case 1: // CO-OP
      gm = "coop";
      flags |= DMD_COOP;
      break;
   case 2: // normal dm
      gm = "deathmatch";
      flags |= DMD_DEATHMATCH;
      break;
   case 3: // altdeath
      gm = "altdeath";
      flags |= DMD_DEATHMATCH2;
      break;
   case 4: // trideath
      gm = "trideath";
      flags |= DMD_DEATHMATCH3;
      break;
   }

   psnprintf(cmdbuf, sizeof(cmdbuf), "dmflags %lu", flags);
   C_RunTextCmd(cmdbuf);

   C_Printf("dmflags set to default for %s\n", gm);
}

void G_DMAddCommands(void)
{
   C_AddCommand(dmflags);
   C_AddCommand(defdmflags);
}

// EOF


