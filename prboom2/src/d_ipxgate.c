#include <stdio.h>
#include <stdlib.h>
#include <netipx/ipx.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "protocol.h"

#define BACKUPTICS 12
#define NCMD_EXIT               0x80000000
#define NCMD_RETRANSMIT         0x40000000
#define NCMD_SETUP              0x20000000
#define NCMD_KILL               0x10000000      // kill game
#define NCMD_CHECKSUM           0x0fffffff

typedef struct
{
  short     gameid;                       // so multiple games can setup at once
  short     drone;
  short     nodesfound;
  short     nodeswanted;
} setupdata_t;

typedef struct
a
{
    // High bit is retransmit request.
    unsigned            checksum;
    // Only valid if NCMD_RETRANSMIT.
    byte                retransmitfrom;

    byte                starttic;
    byte                player; 
    byte                numtics;
    ticcmd_t            cmds[BACKUPTICS];
} doomdata_t;   
	    
typedef struct
{
  signed int tic;
  union altu {
    setupdata_t s;
    unsigned char data[100];
    doomdata_t d;
  } u;
} ipxpacket_t;

int nodes;

unsigned short port = 0x869b;

int ipx_socket(void) {
    int s = socket(PF_IPX,SOCK_DGRAM,0);
    struct sockaddr_ipx sa;
    if (s == -1) {
	    fprintf(stderr,"socket(PF_PIPX): %s\n",strerror(errno));
	    exit(-1);
    }
    memset(&sa,0,sizeof(sa));
    memset(sa.sipx_node,0xff,sizeof(sa.sipx_node));
    sa.sipx_port = htons(port);
    if (bind(s,(struct sockaddr*)&sa,sizeof(sa)) == -1) {
	    fprintf(stderr,"bind(%d): %s\n",port,strerror(errno));
	    exit(-1);
    }
    return s;
}

int udp_socket(const char* ip) {
  struct sockaddr_in sa;
  int s = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);

  if (s == -1) {
    fprintf(stderr,"socket(PF_INET): %s\n", strerror(errno));
    exit(-1);
  }
  sa.sin_family=PF_INET;
  inet_aton(ip,&sa.sin_addr);
  sa.sin_port = htons(5030);

  if (connect(s,(struct sockaddr*)&sa,sizeof sa) == -1) {
    fprintf(stderr,"connect(%s:%d): %s\n", ip, 5030, strerror(errno));
    exit(-1);
  }
  return s;
}

static byte ChecksumPacket(const packet_header_t* buffer, size_t len)
{
  const byte* p = (void*)buffer;
  byte sum = 0;

  if (len==0)
	  return 0;

  while (p++, --len)
	  sum += *p;

  return sum;
}

//
// Checksum
//
unsigned NetbufferChecksum (void* p, size_t l)
{
    unsigned            c;

    c = 0x1234567;

    l = l/4;
    for (int i=0 ; i<l ; i++)
        c += ((unsigned *)p)[i] * (i+1);

    return c & NCMD_CHECKSUM;
}


int ipxs, udps;

int consoleplayer;
int basetic;

int ExpandTics (int low, int maketic)
{
    int delta;
        
    delta = low - (maketic&0xff);
        
    if (delta >= -64 && delta <= 64)
        return (maketic&~0xff) + low;
    if (delta > 64)
        return (maketic&~0xff) - 256 + low;
    if (delta < -64)
        return (maketic&~0xff) + 256 + low;
    fprintf(stderr,"ExpandTics strange value %i at maketic %i\n",low,maketic);
    exit(-2);
}

void send_udp_packet(enum packet_type_e type, unsigned tic, void* data, size_t len) {
  packet_header_t* p = calloc(sizeof(packet_header_t)+len+1,1);
  p->tic = doom_htonl(basetic = tic); p->type = type;
  if (!data) {
    data = (void*)&consoleplayer; len = 1;
  }
  memcpy(((char*)p)+sizeof(*p),data,len);
  p->checksum = ChecksumPacket(p,sizeof(packet_header_t)+len);
  write(udps,p,sizeof(packet_header_t)+len+1);
}

int connected;
int ipxcounter;

void ipx_receive(int s) {
  ipxpacket_t buf;
  int rc;
  struct sockaddr from;
  size_t sl = sizeof(from);
  rc = recvfrom(s,&buf,sizeof buf,0,&from,&sl);
  if (rc == -1) {
    fprintf(stderr,"read(ipx): %s\n", strerror(errno));
    exit(-2);
  }
  if (rc > 0) {
    if (buf.tic == -1) {
      // Setup packet
      if (!connected++) {
        connect(s,&from,sl);
	send_udp_packet(PKT_INIT,0,NULL,0);
      }
    } else {
      if (buf.u.d.checksum & NCMD_SETUP) {
	printf("setup packet, dropped\n");
      } else if (buf.u.d.checksum & NCMD_EXIT) {
	      send_udp_packet(PKT_QUIT,buf.u.d.starttic,NULL,0);
	      exit(0);
      } else if ((buf.u.d.checksum & NCMD_CHECKSUM) == buf.u.d.checksum) {
	      // No flags, normal game packet
	      char outbuf[100];
	      int tics;
	      outbuf[0] = tics = buf.u.d.numtics;
	      outbuf[1] = buf.u.d.player;
              for (int i=0; i< tics; i++)
		TicToRaw(outbuf+2+i*sizeof(ticcmd_t),&buf.u.d.cmds[i]);
	      send_udp_packet(PKT_TICC, ExpandTics(buf.u.d.starttic, basetic), outbuf, 2+tics*sizeof(ticcmd_t));
      }
    }
  }
}

void udp_receive(int s) {
  size_t len = 1024;
  packet_header_t *p = malloc(len);
  int rc;
  
  rc = read(s,p,len);
  if (rc < 0) {
    fprintf(stderr,"read(udp): %s\n", strerror(errno));
    exit(-2);
  }
  if (rc > 0) {
    switch (p->type) {
	    case PKT_SETUP:
		    {
			    struct setup_packet_s *sinfo = (void*)(p+1);
			    consoleplayer = sinfo->yourplayer;
			    send_udp_packet(PKT_GO,0,NULL,0);
			    write(ipxs,"\xff\xff\xff\xff\x00\x00\x00\x00\x02\x00\x02\x00\x00\x00\x00\x00",16);
		    }
		    break;
	    case PKT_GO:
		    {
			    ipxpacket_t pkt;
			    memset(&pkt,0,sizeof(pkt));
			    pkt.tic = ipxcounter++;
			    pkt.u.d.player = consoleplayer^1;
			    pkt.u.d.starttic = 0;
			    pkt.u.d.numtics = 0;
			    pkt.u.d.retransmitfrom = 0;
			    pkt.u.d.checksum = NetbufferChecksum(&pkt.u.d.retransmitfrom, 4);
			    write(ipxs,&pkt,16);
		    }
		    break;
	    case PKT_TICS:
		    {
			    ipxpacket_t pkt;
			    int tic = doom_ntohl(p->tic);
			    byte *pp = (void*)(p+1);
			    int tics = *pp++;
			    memset(&pkt,0,sizeof(pkt));
			    size_t len;

			    pkt.tic = ipxcounter++;
			    pkt.u.d.starttic = tic;
			    pkt.u.d.player = (consoleplayer == 0 ? 1 : 0);
			    pkt.u.d.numtics = tics;

			    for (int t=0; t<tics; t++) {
			      int players = *pp++;
			      for (int i=0; i<players; i++) {
				if (*pp++ == pkt.u.d.player)
				  RawToTic(&pkt.u.d.cmds[t],pp);
				pp += sizeof(ticcmd_t);
			      }
			    }
			    pkt.u.d.retransmitfrom = 0;
			    len = 12+tics*sizeof(ticcmd_t);
			    len = (len+7)&0xff8; // round up to next 16
			    pkt.u.d.checksum = NetbufferChecksum(&pkt.u.d.retransmitfrom, len-8);
			    write(ipxs,&pkt,len);
		    }
    }
  }
}

void loop(int ipxs, int udps) {
    for (;;) {
      struct timeval wt = {60,0};
      fd_set fds;
      int rc;

      FD_ZERO(&fds);
      FD_SET(ipxs,&fds);
      FD_SET(udps,&fds);
      rc = select(ipxs+udps,&fds,NULL,NULL,&wt);
      if (rc < 0) {
	fprintf(stderr,"select: %s\n", strerror(errno));
	exit(-2);
      }
      if (rc>0) {
	if (FD_ISSET(ipxs,&fds))
	  ipx_receive(ipxs);
	if (FD_ISSET(udps,&fds))
	  udp_receive(udps);
      }
    }
}

int main(int argc, char**argv) {
    ipxs = ipx_socket();
    udps = udp_socket(argv[1]);
    loop(ipxs,udps);
    return 0;
}

