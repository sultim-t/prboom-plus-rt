/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: protocol.h,v 1.1 2000/05/04 08:15:07 proff_fs Exp $
 *
 *  LxDoom, a Doom port for Linux/Unix
 *  New LxDoom network protocol, based partly on the old linuxdoom networking
 *  Copyright (C) 1999 by id Software, Colin Phipps
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
 *  Doom Network protocol packet definitions.
 *-----------------------------------------------------------------------------*/

#include "doomtype.h"
#include "d_ticcmd.h"
#include "m_swap.h"

enum packet_type_e { 
  PKT_INIT,    // initial packet to server
  PKT_SETUP,   // game information packet
  PKT_GO,      // game has started
  PKT_TICC,    // tics from client
  PKT_TICS,    // tics from server
  PKT_RETRANS, // Request for retransmission
  PKT_EXTRA,   // Extra info packet
  PKT_QUIT,    // Player quit game
  PKT_DOWN,    // Server downed
  PKT_WAD,     // Wad file request
};

typedef struct {
  byte checksum;       // Simple checksum of the entire packet
  byte type;           /* Type of packet */
  unsigned tic;        // Timestamp
} packet_header_t;

#ifndef GAME_OPTIONS_SIZE
// From g_game.h
#define GAME_OPTIONS_SIZE 64
#endif

struct setup_packet_s {
  byte players, yourplayer, skill, episode, level, deathmatch, complevel, ticdup, extratic;
  byte game_options[GAME_OPTIONS_SIZE];
  byte numwads;
  byte wadnames[1]; // Actually longer
};

// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static void GetTicSwap(ticcmd_t* dst, const ticcmd_t* src)
{
  *dst = *src;
  dst->angleturn = doom_ntohs(dst->angleturn);
  dst->consistancy = doom_ntohs(dst->consistancy);
}

/*
 * $Log: protocol.h,v $
 * Revision 1.1  2000/05/04 08:15:07  proff_fs
 * Initial revision
 *
 * Revision 1.7  2000/05/01 15:16:47  Proff
 * added __inline for VisualC
 *
 * Revision 1.6  2000/04/29 16:15:01  cph
 * Revert new netgame stuff
 *
 * Revision 1.5  2000/03/28 10:40:00  cph
 * Explicitely state 1 byte for packet type
 *
 * Revision 1.4  2000/03/28 08:47:48  cph
 * New free join/parting for network games
 *
 * Revision 1.3  1999/10/12 13:01:15  cphipps
 * Changed header to GPL
 *
 * Revision 1.2  1999/04/02 10:26:14  cphipps
 * Add new packet types for sending WAD info, and starting the game
 * Added info to startup packets about wad files
 *
 * Revision 1.1  1999/04/01 18:37:51  cphipps
 * Initial revision
 *
 */
