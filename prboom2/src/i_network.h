/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_network.h,v 1.2 2000/05/09 21:45:36 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Colin Phipps (cph@lxdoom.linuxgames.com), 
 *  Jess Haas (JessH@lbjhs.net)
 *  and Florian Schulze (florian.proff.schulze@gmx.net)
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
 *  Low level network interface. 
 *-----------------------------------------------------------------------------*/

void I_InitNetwork(const char* serv, int pn);
size_t I_GetPacket(packet_header_t* buffer, size_t buflen);
void I_SendPacket(packet_header_t* packet, size_t len);

#ifdef AF_INET
void I_SendPacketTo(packet_header_t* packet, size_t len, struct sockaddr* to);
void I_SetupSocket(int sock, int port, int family);
void I_PrintAddress(FILE* fp, struct sockaddr* addr);

extern struct sockaddr sentfrom;
extern int v4socket, v6socket;
#endif

extern size_t sentbytes, recvdbytes;
