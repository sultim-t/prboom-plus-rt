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
//----------------------------------------------------------------------------

#include "../doomdef.h"
#include "../doomstat.h"
#include "../z_zone.h"

#include "ser_main.h"

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

extern void (*netdisconnect)();

// ser_port.c
void InitPort (void);
void ShutdownPort(void);
void jump_start(void);
int read_byte(void);
void write_byte(int c);

void Ser_Disconnect();

static boolean CheckForEsc();
static void ModemClear();
static void ModemCommand(char *str);

extern que_t inque, outque;

extern int uart;

int usemodem;
char startup[256], shutdown[256];

connect_t connectmode=CONNECT;
char phonenum[50];

extern doomcom_t *doomcom;
doomcom_t ser_doomcom;

int ser_active;

//
// write_buffer
//
static void write_buffer(char *buffer, unsigned int count)
{
   // if this would overrun the buffer, throw everything else out
   if(outque.head - outque.tail + count > QUESIZE)
      outque.tail = outque.head;
   
   while (count--)
      write_byte(*buffer++);
   
   if(INPUT(uart + LINE_STATUS_REGISTER) & 0x40)
      jump_start();
}


//
// Ser_Error
//
// For abnormal program terminations
//
static void Ser_Error (char *error, ...)
{
   va_list argptr;
   
   Ser_Disconnect();
   
   if(error)
   {
      // haleyjd: increased buffer size
      char tempstr[1024];
      memset(tempstr, 0, 1024);
      va_start(argptr,error);
      pvsnprintf(tempstr,sizeof(tempstr),error,argptr);
      va_end(argptr); // haleyjd: moved up
      usermsg("");
      usermsg(tempstr);
      usermsg("");

      MN_ErrorMsg(tempstr);
   }
   
   ser_active = false;
}

void Ser_Disconnect()
{
   doomcom = &singleplayer;
   
   ShutdownPort();
   
   if(!usemodem) return;
   
   usermsg("");
   usermsg("Dropping DTR.. ");
   usermsg("");
   
   OUTPUT(uart + MODEM_CONTROL_REGISTER,
	  INPUT(uart + MODEM_CONTROL_REGISTER) & ~MCR_DTR );
   delay (1250);
   OUTPUT(uart + MODEM_CONTROL_REGISTER,
	  INPUT(uart + MODEM_CONTROL_REGISTER) | MCR_DTR );
   
   // hang up modem
   ModemCommand("+++");
   delay (1250);
   ModemCommand(shutdown);
   delay (1250);
}


//
// Ser_ReadPacket
//

#define MAXPACKET 512
#define	FRAMECHAR 0x70

char packet[MAXPACKET];
int packetlen;
int inescape;
int newpacket;

static boolean Ser_ReadPacket(void)
{
   int c;
   
   // if the buffer has overflowed, throw everything out
   if(inque.head - inque.tail > QUESIZE - 4) // check for buffer overflow
   {
      inque.tail = inque.head;
      newpacket = true;
      return false;
   }
  
   if(newpacket)
   {
      packetlen = 0;
      newpacket = 0;
   }
   
   do
   {
      c = read_byte();
      if(c < 0)
	 return false; // haven't read a complete packet

      if(inescape)
      {
	 inescape = false;
	 if(c != FRAMECHAR)
	 {
	    newpacket = 1;
	    return true; // got a good packet
	 }
      }
      else if(c == FRAMECHAR)
      {
	 inescape = true;
	 continue; // don't know yet if it is a terminator
      }	           // or a literal FRAMECHAR
      
      if(packetlen >= MAXPACKET)
	 continue; // oversize packet

      packet[packetlen] = c;
      packetlen++;
   
   } while(1);
}

//
// Ser_WritePacket
//

// sf: made void *buffer not char *buffer

static void Ser_WritePacket(void *buffer, int len)
{
   int b;
   static char localbuffer[MAXPACKET*2+2];
   char *buf = buffer;
   
   b = 0;
   
   if(len > MAXPACKET)
      return;
   
   while (len--)
   {
      if (*buf == FRAMECHAR)
	 localbuffer[b++] = FRAMECHAR;	// escape it for literal
      localbuffer[b++] = *buf++;
   }
   
   localbuffer[b++] = FRAMECHAR;
   localbuffer[b++] = 0;
   
   write_buffer(localbuffer, b);
}

//
// Ser_NetISR
//
void Ser_NetISR(void)
{
   if(ser_doomcom.command == CMD_SEND)
   {
      Ser_WritePacket((char *)&ser_doomcom.data, 
	              ser_doomcom.datalength);
   }
   else if(ser_doomcom.command == CMD_GET)
   {
      if(Ser_ReadPacket() && packetlen <= sizeof(ser_doomcom.data))
      {
	 ser_doomcom.remotenode = 1;
	 ser_doomcom.datalength = packetlen;
	 memcpy(&ser_doomcom.data, &packet, packetlen);
      }
      else
	 ser_doomcom.remotenode = -1;
   }
}

//
// Ser_Connect
//
// Figures out who is player 0 and 1
//

typedef struct
{
   enum
   {
      pt_join,
      pt_accept
   } packet_type;
   
   short remote_number;

} setuppacket_t;

// sf: simplified connection routines/selection of player numbers
// the player who ran connect first is server

// when we enter connect, we send out a packet. If there is another
// player already waiting they detect it and send a response to
// start the game. If there is not yet a player waiting then the
// opposite will happen: when the other player joins we will detect
// their join packet.

static void Ser_Connect(void)
{
   int local_number;       // assign us a random number as our own
   setuppacket_t send;     // sending packet

   usermsg("");
   usermsg("Looking for another player");
   usermsg("press escape to abort.");
   usermsg("");
   
   // get local number: 0-65536
   // 2 computers unlikely to generate the same number

   // shuffle random seed to make sure
   G_ScrambleRand();
   
   local_number = M_Random() * 256 + M_Random();
   
   // send a packet and wait for a response.
   // If there is a response, theres already another SMMU waiting
   
   send.packet_type = pt_join;
   send.remote_number = local_number;
   Ser_WritePacket(&send, sizeof(send));
   
   // wait for a response from the other SMMU
   // or a call from another SMMU joining
   while(1)
   {
      if(CheckForEsc())
      {
	 Ser_Error("connection aborted.");
	 return;
      }

      while(Ser_ReadPacket())
      {
	 setuppacket_t recv;
	 
	 // check it is a setuppacket
	 if(packetlen != sizeof(setuppacket_t))
	 {
	    Ser_Error("weird length packet received");
	    return;
	 }
	 // copy it into recv
	 memcpy(&recv, packet, packetlen);
	 
	 // modem echo?
	 if(recv.remote_number == local_number)
	 {
	    Ser_Error("packet echo (from a modem?)");
	    return;
	 }
	 
	 // other SMMU joining
	 // therefore we started first
	 if(recv.packet_type == pt_join)
	 {
	    ser_doomcom.consoleplayer = 0;    // we are server
	    
	    // send a response
	    send.packet_type = pt_accept;
	    send.remote_number = local_number;
	    Ser_WritePacket(&send, sizeof(send));
	    
	    // start game
	    while(Ser_ReadPacket());     // flush
	    
	    return;
	 }
	 else if(recv.packet_type == pt_accept)
	 {
	    // response from other computer
	    // therefore we have joined a server
	    
	    ser_doomcom.consoleplayer = 1;      // second player
	    
	    // no response needed to server
	    // start game	    
	    while(Ser_ReadPacket());      // flush
	    
	    return;
	 }
	 else
	 {
	    Ser_Error("unexpected packet type received");
	    return;
	 }
      }
   }
}

//
// ModemCommand
//
static void ModemCommand(char *str)
{
  if(!ser_active) return;         // aborted
  
  usermsg("%s",str);
  write_buffer(str, strlen(str));
  write_buffer("\r", 1);
}

//
// ModemClear
//
// Clear out modem commands from previous attempts to connect
//
static void ModemClear()
{
   while(read_byte() != -1);
   
   packetlen = newpacket = inescape = 0; // clear it
}


//
// ModemResponse
//
// Waits for OK, RING, Ser_Connect, etc
//

char response[80];

static void ModemResponse(char *resp)
{
   int c;
   int respptr;
   
   if(!ser_active) return; // it has been aborted
   
   do
   {
      respptr=0;
      do
      {
	 if(CheckForEsc())
	 {
	    Ser_Error("modem response aborted.");
	    return;
	 }
	 c = read_byte ();
	 if (c == -1)
	    continue;
	 
	 if(c == '\n' || respptr == 79)
	 {
	    response[respptr] = 0;
	    usermsg(FC_GOLD"%s",response);
	    break;
	 }
	 
	 if (c >= ' ')
	 {
	    response[respptr] = c;
	    respptr++;
	 }

      } while(1);
   
   } while(strncmp(response, resp, strlen(resp)));
}


//
// ReadLine
//
static void ReadLine(FILE *f, char *dest)
{
   int c;
   
   do
   {
      c = fgetc(f);
      
      if(c == EOF)
      {
	 usermsg("EOF in modem.cfg");
	 return;
      }
      
      if(c == '\r' || c == '\n')
	 break;
      
      *dest++ = c;
   
   } while(1);
   
   *dest = 0;
}


//
// InitModem
//
static void InitModem(void)
{
   ModemCommand(startup);
   ModemResponse("OK");
}

//
// Ser_Init
//
// Read cfg from modem.cfg
//
void Ser_Init(void)
{
   FILE *f;
   
   f = fopen("modem.cfg", "r");
   
   if(!f)
   {
      usermsg("Couldn't read MODEM.CFG");
      strcpy(startup, "atz");
      strcpy(shutdown, "at z h0");
      return;
   }
   
   ReadLine(f, startup);
   ReadLine(f, shutdown);
   fclose(f);
}


//
// Dial
//
static void Dial(void)
{
   static char cmd[80];

   memset(cmd, 0, 80);
   
   usemodem = true;
   InitModem();
   
   if(!ser_active) return; // aborted
   
   usermsg ("");
   usermsg ("Dialing...");
   usermsg ("");
   
   sprintf(cmd, "ATDT%s", phonenum);
   
   ModemCommand(cmd);
   ModemResponse("CONNECT");
   
   if(!ser_active) return; // aborted
   
   if(strncmp(response+8, "9600", 4))
      Ser_Error("The Connection MUST be made at 9600\n"
                "baud, no Error correction, no compression!\n"
		"Check your modem initialization string!");
   
   ser_doomcom.consoleplayer = 1;
}


//
// Answer
//
static void Answer(void)
{
   usemodem = true;
   InitModem();
   
   if(!ser_active) return;         // aborted
   
   usermsg("");
   usermsg("Waiting for ring...");
   usermsg("");
  
   ModemResponse("RING");
   ModemCommand("ATA");
   ModemResponse("CONNECT");
   
   ser_doomcom.consoleplayer = 0;
}

extern void (*netget)(void);
extern void (*netsend)(void);

//
// Ser_Start
//
static void Ser_Start(void)
{
   c_showprompt = false;
   C_SetConsole();
   
   ser_active = true;
   usemodem = false; // default usemodem to false
   
   //
   // set network characteristics
   //
   
   ser_doomcom.id = DOOMCOM_ID;
   ser_doomcom.ticdup = 1;
   ser_doomcom.extratics = 0;
   ser_doomcom.numnodes = 2;
   ser_doomcom.numplayers = 2;
   ser_doomcom.drone = 0;
   
   doomcom = &ser_doomcom;
   
   usermsg("");
   C_Separator();
   usermsg(FC_HI "smmu serial mode");
   usermsg("");
   
   //
   // establish communications
   //
   
   InitPort();
   ModemClear();
   
   usermsg("hit escape to abort");
   
   switch(connectmode)
   {
   case ANSWER:
      Answer();
      break;
   
   case DIAL:
      Dial();
      break;
   
   default:
      break;
   }
   
   if(!ser_active) return; // aborted
   
   Ser_Connect();
   
   if(!ser_active) return; // aborted
   
   netdisconnect = Ser_Disconnect;
   netget = Ser_NetISR;
   netsend = Ser_NetISR;
   netgame = true;
   
   ResetNet();
   
   D_InitNetGame();
   
   ResetNet();
   
   C_SendNetData();
   
   if(!netgame) // aborted
   {
      Ser_Disconnect();
      ResetNet();
      return;
   }
   
   ResetNet();
   MN_ClearMenus(); // clear menus now connected
}

extern event_t events[MAXEVENTS];
extern int eventhead, eventtail;

static boolean CheckForEsc(void)
{
   event_t *ev;
   boolean escape = false;
   
   I_StartTic();
   
   for(; eventtail != eventhead; eventtail = (eventtail+1) & (MAXEVENTS-1))
   {
      ev = events + eventtail;
      if((ev->type == ev_keydown) && (ev->data1 == KEYD_ESCAPE))
	 escape = true;
   }
   return escape;
}

/***************************
        CONSOLE COMMANDS
 ***************************/

VARIABLE_INT(comport, NULL, 1, 4, NULL);

CONSOLE_COMMAND(nullmodem, cf_notnet | cf_buffered)
{
   connectmode = CONNECT;
   Ser_Start();
}

CONSOLE_COMMAND(dial, cf_notnet | cf_buffered)
{
   connectmode = DIAL;
   strcpy(phonenum, c_args);
   Ser_Start();
}

CONSOLE_COMMAND(answer, cf_notnet | cf_buffered)
{
   connectmode = ANSWER;
   Ser_Start();
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