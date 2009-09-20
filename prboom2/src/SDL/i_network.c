/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom: a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  Copyright 2005, 2006 by
 *  Florian Schulze, Colin Phipps, Neil Stevens, Andrey Budko
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
 *  Low level UDP network interface. This is shared between the server
 *  and client, with SERVER defined for the former to select some extra
 *  functions. Handles socket creation, and packet send and receive.
 *
 *-----------------------------------------------------------------------------*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#ifdef HAVE_NET

#include "SDL.h"
#include "SDL_net.h"

#include "protocol.h"
#include "i_network.h"
#include "lprintf.h"
//#include "doomstat.h"

/* cph -
 * Each client will either use the IPv4 socket or the IPv6 socket
 * Each server will use whichever or both that are available
 */
UDP_CHANNEL sentfrom;
IPaddress sentfrom_addr;
UDP_SOCKET udp_socket;

/* Statistics */
size_t sentbytes, recvdbytes;

UDP_PACKET *udp_packet;

/* I_ShutdownNetwork
 *
 * Shutdown the network code
 */
void I_ShutdownNetwork(void)
{
        SDLNet_FreePacket(udp_packet);
        SDLNet_Quit();
}

/* I_InitNetwork
 *
 * Sets up the network code
 */
void I_InitNetwork(void)
{
  SDLNet_Init();
  atexit(I_ShutdownNetwork);
  udp_packet = SDLNet_AllocPacket(10000);
}

UDP_PACKET *I_AllocPacket(int size)
{
  return(SDLNet_AllocPacket(size));
}

void I_FreePacket(UDP_PACKET *packet)
{
  SDLNet_FreePacket(packet);
}


/* cph - I_WaitForPacket - use select(2) via SDL_net's interface
 * No more I_uSleep loop kludge */

void I_WaitForPacket(int ms)
{
  SDLNet_SocketSet ss = SDLNet_AllocSocketSet(1);
  SDLNet_UDP_AddSocket(ss, udp_socket);
  SDLNet_CheckSockets(ss,ms);
  SDLNet_FreeSocketSet(ss);
#if (defined _WIN32 && !defined PRBOOM_SERVER)
  I_UpdateConsole();
#endif
}

/* I_ConnectToServer
 *
 * Connect to a server
 */
IPaddress serverIP;

int I_ConnectToServer(const char *serv)
{
  char server[500], *p;
  Uint16 port;

  /* Split serv into address and port */
  if (strlen(serv)>500) return 0;
  strcpy(server,serv);
  p = strchr(server, ':');
  if(p)
  {
    *p++ = '\0';
    port = atoi(p);
  }
  else
    port = 5030; /* Default server port */

  SDLNet_ResolveHost(&serverIP, server, port);
  if ( serverIP.host == INADDR_NONE )
    return -1;

  if (SDLNet_UDP_Bind(udp_socket, 0, &serverIP) == -1)
    return -1;

  return 0;
}

/* I_Disconnect
 *
 * Disconnect from server
 */
void I_Disconnect(void)
{
/*  int i;
  UDP_PACKET *packet;
  packet_header_t *pdata = (packet_header_t *)packet->data;
  packet = I_AllocPacket(sizeof(packet_header_t) + 1);

  packet->data[sizeof(packet_header_t)] = consoleplayer;
        pdata->type = PKT_QUIT; pdata->tic = gametic;

  for (i=0; i<4; i++) {
    I_SendPacket(packet);
    I_uSleep(10000);
    }
  I_FreePacket(packet);*/
  SDLNet_UDP_Unbind(udp_socket, 0);
}

/*
 * I_Socket
 *
 * Sets the given socket non-blocking, binds to the given port, or first
 * available if none is given
 */
UDP_SOCKET I_Socket(Uint16 port)
{
  if(port)
    return (SDLNet_UDP_Open(port));
  else {
    UDP_SOCKET sock;
    port = IPPORT_RESERVED;
    while( (sock = SDLNet_UDP_Open(port)) == NULL )
      port++;
    return sock;
  }
}

void I_CloseSocket(UDP_SOCKET sock)
{
  SDLNet_UDP_Close(sock);
}

UDP_CHANNEL I_RegisterPlayer(IPaddress *ipaddr)
{
  static int freechannel;
  return(SDLNet_UDP_Bind(udp_socket, freechannel++, ipaddr));
}

void I_UnRegisterPlayer(UDP_CHANNEL channel)
{
  SDLNet_UDP_Unbind(udp_socket, channel);
}

/*
 * ChecksumPacket
 *
 * Returns the checksum of a given network packet
 */
static byte ChecksumPacket(const packet_header_t* buffer, size_t len)
{
  const byte* p = (const void*)buffer;
  byte sum = 0;

  if (len==0)
    return 0;

  while (p++, --len)
    sum += *p;

  return sum;
}

size_t I_GetPacket(packet_header_t* buffer, size_t buflen)
{
  int checksum;
  size_t len;
  int status;

  status = SDLNet_UDP_Recv(udp_socket, udp_packet);
  len = udp_packet->len;
  if (buflen<len)
    len=buflen;
  if ( (status!=0) && (len>0) )
    memcpy(buffer, udp_packet->data, len);
  sentfrom=udp_packet->channel;
#ifndef SDL_NET_UDP_PACKET_SRC
  sentfrom_addr=udp_packet->address;
#else
  sentfrom_addr=udp_packet->src; /* cph - allow for old SDL_net library */
#endif
  checksum=buffer->checksum;
  buffer->checksum=0;
  if ( (status!=0) && (len>0)) {
    byte psum = ChecksumPacket(buffer, udp_packet->len);
/*    fprintf(stderr, "recvlen = %u, stolen = %u, csum = %u, psum = %u\n",
  udp_packet->len, len, checksum, psum); */
    if (psum == checksum) return len;
  }
  return 0;
}

void I_SendPacket(packet_header_t* packet, size_t len)
{
  packet->checksum = ChecksumPacket(packet, len);
  memcpy(udp_packet->data, packet, udp_packet->len = len);
  SDLNet_UDP_Send(udp_socket, 0, udp_packet);
}

void I_SendPacketTo(packet_header_t* packet, size_t len, UDP_CHANNEL *to)
{
  packet->checksum = ChecksumPacket(packet, len);
  memcpy(udp_packet->data, packet, udp_packet->len = len);
  SDLNet_UDP_Send(udp_socket, *to, udp_packet);
}

void I_PrintAddress(FILE* fp, UDP_CHANNEL *addr)
{
/*
  char *addy;
  Uint16 port;
  IPaddress *address;

  address = SDLNet_UDP_GetPeerAddress(udp_socket, player);

//FIXME: if it cant resolv it may freeze up
  addy = SDLNet_ResolveIP(address);
  port = address->port;

  if(addy != NULL)
      fprintf(fp, "%s:%d", addy, port);
  else
    fprintf(fp, "Error");
*/
}

#endif /* HAVE_NET */
