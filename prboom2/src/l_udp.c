/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: l_udp.c,v 1.1 2000/05/04 08:07:55 proff_fs Exp $
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
 *  Low level UDP network interface. This is shared between the server 
 *  and client, with SERVER defined for the former to select some extra
 *  functions. Handles socket creation, and packet send and receive.
 *
 *-----------------------------------------------------------------------------*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#if defined(__BEOS__)
#define PF_INET AF_INET
#define IPPORT_USERRESERVED 5000
#else
#include <arpa/inet.h>
#endif

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "protocol.h"
#include "i_network.h"
#include "lprintf.h"

/* cph -
 * Each client will either use the IPv4 socket or the IPv6 socket
 * Each server will use whichever or both that are available
 */
int v4socket = -1, v6socket = -1;

/* Statistics */
size_t sentbytes, recvdbytes;

//
// BindToLocalPort
//
static boolean BindToLocalPort( int s, int port, int family )
{
  switch (family) {
  case AF_INET:
    {
      struct sockaddr_in    address;
      
      memset (&address, 0, sizeof(address));
      address.sin_family = family;
      address.sin_addr.s_addr = INADDR_ANY;
      address.sin_port = htons(port);
      
      return (bind(s, (struct sockaddr *)&address, sizeof(address)) != -1);
    }
#if defined(HAVE_IPv6)
  case AF_INET6:
    {
      struct sockaddr_in6    address;
      
      memset (&address, 0, sizeof(address));
      address.sin6_family = family;
      address.sin6_addr = in6addr_any;
      address.sin6_port = htons(port);

      return (bind(s, (struct sockaddr *)&address, sizeof(address)) != -1);
    }
#endif
  default:
    return false;
  }
}

/*
 * I_SetupSocket
 * 
 * Sets the given socket non-blocking, binds to the given port, or first
 * avaibable if none is given
 */
void I_SetupSocket(int sock, int port, int family)
{
  if (port) {
    if (!BindToLocalPort(sock, port, family))
      I_Error("Failed to bind port %d", port);
  } else {
    port = IPPORT_RESERVED;
    while (!BindToLocalPort(sock, port, family))
      port++;
  }

  {  /* Set non-blocking */
    int oldflags = fcntl (sock, F_GETFL, 0);
    /* If reading the flags failed, return error indication now. */
    if (oldflags == -1)
      I_Error("fcntl(sock) failed?");

    oldflags |= O_NONBLOCK;
    fcntl (sock, F_SETFL, oldflags);
    /* Other possibilities - 
     * boolean        trueval = true;
     * ioctlsocket(sock, FIONBIO, (u_long *)&trueval);
     * setsockopt(sock, SOL_SOCKET, SO_NONBLOCK, &trueval, sizeof(trueval));
     * ioctl (recvsocket, FIONBIO, &trueval); 
     */
  }
}

/*
 * ChecksumPacket
 *
 * Returns the checksum of a given network packet 
 */
static byte ChecksumPacket(const packet_header_t* buffer, size_t len)
{
  const byte* p = (void*)buffer; 
  byte sum = 0;

  while (p++, --len)
    sum += *p;

  return sum;
}

struct sockaddr sentfrom;

size_t I_GetPacket(packet_header_t* buffer, size_t buflen)
{
  size_t sfsize = sizeof(sentfrom);
  int n = recvfrom(v4socket, buffer, buflen, 0, 
		   &sentfrom, &sfsize);
  if (n >= 0) recvdbytes += n;
  else if (errno != EWOULDBLOCK) perror("I_Getpacket:recvfrom");
  return ((n>0 && (buffer->checksum == ChecksumPacket(buffer, n))) ? n : 0);
}

void I_SendPacketTo(packet_header_t* packet, size_t len, struct sockaddr* to)
{
  packet->checksum = ChecksumPacket(packet, len);
  if (sendto(
#ifdef HAVE_IPv6
	     (to->sa_family == AF_INET6) ? v6socket : 
#endif
	     v4socket, 
	     packet, len, 0, to, sizeof *to)<0)
    perror("I_SendPacketTo: sendto");
  else sentbytes+=len;
}

void I_PrintAddress(FILE* fp, struct sockaddr* addr)
{
#ifdef HAVE_INET_NTOP
  char buf[1000];
  char sep;
  short port;
  
  switch(addr->sa_family) {
  case AF_INET:
    {
      struct sockaddr_in *p = (void*)addr;

      inet_ntop(AF_INET, &p->sin_addr, buf, sizeof(buf));
      port = ntohs(p->sin_port);
      sep = ':';
    }
    break;
#ifdef HAVE_IPv6
  case AF_INET6:
    {
      struct sockaddr_in6 *p = (void*)addr;

      inet_ntop(AF_INET, &p->sin6_addr, buf, sizeof(buf));
      sep=';'; /* FIXME */
      port = ntohs(p->sin6_port);
    }
    break;
#endif
  default:
    strcpy(buf, "unrecognised socket family");
    sep = ' ';
    port = addr->sa_family;
  }
  fprintf(fp, "%s:%d", buf, port);
#else
  fprintf(fp, "(system lacks inet_ntop)");
#endif
}

/*
 * $Log: l_udp.c,v $
 * Revision 1.1  2000/05/04 08:07:55  proff_fs
 * Initial revision
 *
 * Revision 1.16  2000/05/01 17:50:35  Proff
 * made changes to compile with VisualC and SDL
 *
 * Revision 1.15  2000/04/23 20:48:21  cph
 * More IPv6 work
 *
 * Revision 1.14  2000/04/03 21:47:39  cph
 * Better detection fo IPv6
 * Minor header file corrections
 *
 * Revision 1.13  2000/04/03 17:06:10  cph
 * Split client specific stuff from l_udp.c to new l_network.c
 * Move server specific stuff from l_udp.c to d_server.c
 * Update copyright notices
 * Restructure ready for IPv6 support
 * Use fcntl instead of ioctl to set socket non-blocking
 *
 * Revision 1.12  2000/03/28 10:42:29  cph
 * Send and receive using the same socket
 * Do not transmit port with init packet, server determines it
 * Client searches for a free port instead of needing -port
 * Transmit player number wanted to server and act on it
 * Remove more diagnostics
 *
 * Revision 1.11  2000/01/26 08:44:24  cphipps
 * Support filio.h so Solaris can get the non-blocking socket ioctl
 *
 * Revision 1.10  1999/12/26 17:46:18  cphipps
 * Always set socket to non-blocking, using an ioctl if nothing better
 *
 * Revision 1.9  1999/12/18 15:48:10  cphipps
 * Use autoconf script to detect inet_aton
 *
 * Revision 1.8  1999/10/31 16:25:22  cphipps
 * Change i_system.h include to lprintf.h, where I_Error is now
 *
 * Revision 1.7  1999/10/12 13:01:11  cphipps
 * Changed header to GPL
 *
 * Revision 1.6  1999/09/05 13:57:38  cphipps
 * Added pointer cast for libc5
 *
 * Revision 1.5  1999/09/05 10:49:52  cphipps
 * I_GetPacket stores the address that sent the packet, used
 * by server during startup
 * Init packet no longer contains the full machine address, instead
 * just the port, and the hostname just for display
 * Failure to lookup local hostname is no longer a fatal error
 *
 * Revision 1.4  1999/04/01 09:46:05  cphipps
 * Sort out default port numbers
 * Add accounting (count sent/rec'ved bytes)
 *
 * Revision 1.3  1999/03/30 06:52:49  cphipps
 * Add cast to sendto(2) parameter to satisfy fussy libc5 headers
 * Fix endianness of port send in PKT_INIT packet by client
 *
 * Revision 1.2  1999/03/29 12:11:38  cphipps
 * Fix endianness of port numbers
 *
 * Revision 1.1  1999/03/29 11:55:30  cphipps
 * Initial revision
 *
 */
