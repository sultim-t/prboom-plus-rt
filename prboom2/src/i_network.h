/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_network.h,v 1.1 2000/05/04 08:02:58 proff_fs Exp $
 *
 *  New low level networking code for LxDoom, based in part on 
 *  the original linuxdoom networking
 *  Copyright (C) 1993-1996 by id Software
 *  Copyright (C) 1999-2000 by Colin Phipps
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

/*
 * $Log: i_network.h,v $
 * Revision 1.1  2000/05/04 08:02:58  proff_fs
 * Initial revision
 *
 * Revision 1.6  2000/04/03 17:06:10  cph
 * Split client specific stuff from l_udp.c to new l_network.c
 * Move server specific stuff from l_udp.c to d_server.c
 * Update copyright notices
 * Restructure ready for IPv6 support
 * Use fcntl instead of ioctl to set socket non-blocking
 *
 * Revision 1.5  2000/03/28 10:43:21  cph
 * Pass wanted player number in init packet
 *
 * Revision 1.4  1999/10/12 13:00:57  cphipps
 * Changed header to GPL, converted C++ comments to C
 *
 * Revision 1.3  1999/09/05 10:48:30  cphipps
 * Added sentfrom address so server can work out client addresses
 * when they connect.
 *
 * Revision 1.2  1999/04/01 09:38:09  cphipps
 * Add variables holding stats
 *
 * Revision 1.1  1999/03/29 11:55:09  cphipps
 * Initial revision
 *
 */
