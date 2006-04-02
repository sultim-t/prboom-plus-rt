// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
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
// DESCRIPTION:
//
//  Mid-level network code
//
// NETCODE_FIXME: Stubified during port to Windows. This is where
// mid-level networking code should go. Code here will need to act as
// a go-between for the code in d_net.c and code in an eventual Win32-
// specific (or general BSD) sockets module where UDP and possibly IPX
// are supported. Both zdoom and Quake 2 have sockets code that could
// easily be adapted to work for Eternity. Quake 2 is a preferred 
// reference source because of its GPL license.
//
//-----------------------------------------------------------------------------

#include "../z_zone.h"  /* memory allocation wrappers -- killough */

static const char
rcsid[] = "$Id: i_net.c,v 1.4 1998/05/16 09:41:03 jim Exp $";

#include "../doomstat.h"
#include "../i_system.h"
#include "../d_event.h"
#include "../d_net.h"
#include "../m_argv.h"

#include "../i_net.h"

void    NetSend (void);
boolean NetListen (void);

//
// NETWORKING
//

void    (*netget) (void);
void    (*netsend) (void);

//
// PacketSend
//
void PacketSend (void)
{
}


//
// PacketGet
//
void PacketGet (void)
{
}

//
// I_InitNetwork
//
void I_InitNetwork (void)
{
  // set up the singleplayer doomcom

  singleplayer.id = DOOMCOM_ID;
  singleplayer.numplayers = singleplayer.numnodes = 1;
  singleplayer.deathmatch = false;
  singleplayer.consoleplayer = 0;
  singleplayer.extratics=0;
  singleplayer.ticdup=1;
 
  // set up for network
                          
  // parse network game options,
  //  -net <consoleplayer> <host> <host> ...
  // single player game
  doomcom = &singleplayer;
  netgame = false;
  return;
}


void I_NetCmd (void)
{
}



//----------------------------------------------------------------------------
//
// $Log: i_net.c,v $
//
//
//----------------------------------------------------------------------------
