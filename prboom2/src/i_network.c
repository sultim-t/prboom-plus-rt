/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_network.c,v 1.3 2000/11/12 14:59:29 cph Exp $
 *
 *  New UDP networking code for LxDoom, based in part on 
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
 *  UDP network client stuff. Mainly the code to set up sockets and 
 * contact the server.
 *
 *-----------------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>

#if defined(__BEOS__)
#define PF_INET AF_INET
#define IPPORT_USERRESERVED 5000
#else
#include <arpa/inet.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "protocol.h"
#include "i_network.h"
#include "lprintf.h"

/* GetInAddr
 * Return a struct sockaddr from a given host[:port] string */
static int GetInAddr(const char* host, struct sockaddr *s_addr)
{
  char hostname[128], *p;
  struct sockaddr_in* addr = (void*)s_addr;

  addr->sin_family = AF_INET;
  if (strlen(host)>127) return 0;
  // Alternative port support
  strcpy(hostname, host); p = strchr(hostname, ':');
  if (p) {
    *p++=0; addr->sin_port = doom_htons(atoi(p));
  } else addr->sin_port = doom_htons(5030 /* Default server port */);

  if (isalpha(hostname[0])) {
    struct hostent*    hostentry;  
    if (!(hostentry = gethostbyname(hostname))) return 0;
    addr->sin_addr.s_addr = *(unsigned long int*)hostentry->h_addr;
  } else {
#ifdef HAVE_INET_ATON
    // dotted-quad ip address
    if (!inet_aton(hostname, &addr->sin_addr))
#endif
      return 0;
  }
  return 1;
}

/* The address of the server */
struct sockaddr sendtoaddr;

void I_SendPacket(packet_header_t* packet, size_t len)
{
  I_SendPacketTo(packet, len, &sendtoaddr);
}

void I_InitNetwork(const char* serv, int pn)
{
  struct { packet_header_t head; short pn; char myaddr[200]; } initpacket;

  // Get local & remote network addresses
  if (!GetInAddr(serv, &sendtoaddr)) 
    I_Error("I_InitNetwork: Unable to locate server");
  if (gethostname(initpacket.myaddr, 200)<0) 
    strcpy(initpacket.myaddr, "too.long");

  { /* Set up our socket */
    switch (sendtoaddr.sa_family) {
    case AF_INET:
      v4socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
      I_SetupSocket(v4socket, 0, AF_INET);
      break;
    default:
      I_Error("I_InetNetwork: Unable to construct socket matching protocol family of server");
    }
  }
  // Send init packet
  initpacket.pn = doom_htons(pn);
  initpacket.head.type = PKT_INIT; initpacket.head.tic = 0;
  I_SendPacket(&initpacket.head, sizeof(initpacket));
}

