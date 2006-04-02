// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
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
// sersetup.c
//
// converted so that it is now inside the exe
// this is a hacked up piece of shit
//     
//   (haleyjd: tell me about it...)
//
// By Simon Howard, based on id sersetup source
//
// NETCODE_FIXME: Obsolete module from DOS. Should be eliminated.
//
//----------------------------------------------------------------------------

#include "../doomdef.h"
#include "../doomstat.h"

//#include "ser_main.h"

#include "../c_io.h"
#include "../c_runcmd.h"
#include "../c_net.h"
#include "../d_event.h"
#include "../d_main.h"
#include "../d_net.h"
#include "../g_game.h"
#include "../m_random.h"
#include "../i_video.h"
#include "../mn_engin.h"


/***************************
        CONSOLE COMMANDS
 ***************************/

int comport;

VARIABLE_INT(comport, NULL, 1, 4, NULL);

CONSOLE_COMMAND(nullmodem, cf_notnet | cf_buffered)
{
}

CONSOLE_COMMAND(dial, cf_notnet | cf_buffered)
{
}

CONSOLE_COMMAND(answer, cf_notnet | cf_buffered)
{
}

CONSOLE_VARIABLE(com, comport, 0) {}

void Ser_AddCommands(void)
{
   C_AddCommand(nullmodem);
   C_AddCommand(dial);
   C_AddCommand(com);
   C_AddCommand(answer);
}

// EOF
