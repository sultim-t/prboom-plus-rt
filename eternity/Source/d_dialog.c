// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2001 James Haley
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
// Dialog Scripting System (DSS) Interpreter
//
// Reads lumps and drives character dialog. Savegame support is not
// necessary because saving will be disallowed during dialog and
// cinematics.
//
// By James Haley
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "d_main.h"
#include "doomdef.h"
#include "doomstat.h"
#include "doomtype.h"
#include "d_dialog.h"
#include "d_event.h"
#include "hu_stuff.h"
#include "v_video.h"
#include "w_wad.h"

// Defines
#define CLEARWAIT TICRATE
#define DONEWAIT  (3*TICRATE)
#define MAXDIALOGLINELEN 256
#define DIALOGNAMELEN 31

// Public globals
runningdialog_t *currentdialog = NULL;
boolean dlginit = false;

// Private globals
static int dlgcount = 0; // keeps track of time on current message
static boolean clearbox = false;  // whether to clear the old message
static char dlgmessage[MAXDIALOGLINELEN]; // the current message
static char dlgname[DIALOGNAMELEN];       // the current name
static patch_t *dlgpic = NULL;            // the current character pic
static patch_t *dlgbackground = NULL;

// renderer variables
static boolean bgtrans    = true;
static int     textcolor  = CR_RED;
static int     localcolor = -1;
static int     bgcolor    = CR_BLUE;  // blue by default

// Wait types for the dialog ticker
enum
{
   DWT_NONE,
   DWT_START,
   DWT_MESSAGE,
   DWT_CLEAR,
};

//
// DLG_Init
//
void DLG_Init(void)
{
   if(!dlginit)
   {
      // load background
      dlgbackground = (patch_t *)W_CacheLumpNum(W_GetNumForName("DLGBACK"), 
                                                PU_STATIC);
   }
   dlginit = true;
}

void DLG_Start(int playernum, const char *lumpname, const char *name, 
	       const char *picname)
{
   int lumpnum, picnum, numcmds;
   char startmessage[64];
   runningdialog_t *dialog;

   if(currentdialog || gamestate != GS_LEVEL)
   {
      // a dialog is already running or can't be started now
      return;
   }

   // set up the fields that the dialog will need
   lumpnum = W_GetNumForName(lumpname);
   picnum  = W_GetNumForName(picname);
   dlgpic  = (patch_t *)W_CacheLumpNum(picnum, PU_LEVEL);

   // clear out and then set dlgname
   memset(dlgname, 0, DIALOGNAMELEN);
   strncpy(dlgname, name, DIALOGNAMELEN);
   
   // load the dialog itself
   dialog = Z_Malloc(sizeof(runningdialog_t), PU_LEVEL, NULL);
   memset(dialog, 0, sizeof(runningdialog_t));
   strncpy(dialog->lump, lumpname, 8);
   
   dialog->cache = W_CacheLumpNum(lumpnum, PU_LEVEL);
   dialog->ip = (int *)(dialog->cache);
   
   numcmds = *(dialog->ip); // get numcmds to compute offset to strings
   dialog->ip += 1;         // increment the ip to set up for running

   // calculate offset to stringtable
   dialog->stringtable = (char *)(dialog->ip) + 2*numcmds*sizeof(int);

   dialog->playernum = playernum;
   
   // set currentdialog to the new dialog
   currentdialog = dialog;

   // only display startup message during a net game
   if(netgame)
   {
      psnprintf(startmessage, sizeof(startmessage), "%s speaks to %s",
                players[playernum].name, name);
      
      HU_CenterMessage(startmessage);
      
      // wait for HU message to clear (1.5 seconds)
      currentdialog->waittime = (3*TICRATE)/2;
      currentdialog->waittype = DWT_START;
   }
}

void DLG_Stop(void)
{
   // reset all globals
   // note that all allocated buffers were done so at the PU_LEVEL
   // cache level, and will be freed later by the zone allocator, to
   // avoid any headaches related to freeing them manually -- in
   // addition, this allows quick reuse of cached lumps :)

   currentdialog = NULL;
   dlgcount = 0;
   clearbox = false;
   dlgpic = NULL;
   bgtrans = true;
   textcolor = CR_RED;
   bgcolor = CR_BLUE;

   redrawborder = true;
}

// Miscellaneous property setting functions
// Some of these (ie SetBGColor) will only be useful with background
// graphics designed to take advantage of color range translation tables

void DLG_SetBGTrans(boolean trans)
{
   bgtrans = trans;
}

void DLG_SetTextColor(int color)
{
   if(!(color < 0 || color >= CR_LIMIT))
      textcolor = color;
}

void DLG_SetBGColor(int color)
{
   if(!(color < 0 || color >= CR_LIMIT))
      bgcolor = color;
}

// Dialog command handler functions
// These are fed their argument retrieved from the binary

static void handler_string(int arg)
{
   int i;
   char *msg;

   i = 0;
   msg = currentdialog->stringtable + arg;

   memset(dlgmessage, 0, MAXDIALOGLINELEN);

   while(*msg && i < MAXDIALOGLINELEN)
      dlgmessage[i++] = *msg++;

   currentdialog->waittime = -1; // suspend indefinitely for drawer
   currentdialog->waittype = DWT_MESSAGE;
   dlgcount = 0;                 // reset dlgcount for new message
   localcolor = -1;              // reset renderer's local string color
}

static void handler_name(int arg)
{
   int i;
   char *msg;

   i = 0;
   msg = currentdialog->stringtable + arg;

   memset(dlgname, 0, DIALOGNAMELEN);

   while(*msg && i < DIALOGNAMELEN)
      dlgname[i++] = *msg++;
}

static void handler_pic(int arg)
{
   int lumpnum;
   char lumpname[9];
   
   strncpy(lumpname, currentdialog->stringtable + arg, 8);
   lumpname[8] = '\0';
   
   lumpnum = W_GetNumForName(lumpname);
   dlgpic = (patch_t *)W_CacheLumpNum(lumpnum, PU_LEVEL);
}

static void handler_callback(int arg)
{
}

// although characters 0x80 through 0x89 are supported directly
// for color changes, this can be difficult to use -- this supports
// changing the global color but can't change the color for only one 
// word -- extended characters will still be necessary for that
static void handler_textcolor(int arg)
{
   DLG_SetTextColor(arg);
}

static void handler_setbgtrans(int arg)
{
   DLG_SetBGTrans(!!arg); // make boolean, 0 or 1
}

static void handler_setbgcolor(int arg)
{
   DLG_SetBGColor(arg);
}

// The dialog command structure.
// Mostly for convenience and encapsulation.

struct dialogcmd_s
{
   enum dctype_e
   {
      KW_STRING,
      KW_SETNAME,
      KW_SETPIC,
      KW_CALLBACK,
      KW_TEXTCOLOR,
      KW_BGTRANS,
      KW_BGCOLOR,
      NUMDIALOGKEYWORDS,
   } type; // type of command

   void (*handler)(int arg); // handler for this type

} dialogcmds[NUMDIALOGKEYWORDS] = {

   {KW_STRING,	  handler_string},
   {KW_SETNAME,	  handler_name},
   {KW_SETPIC,	  handler_pic},
   {KW_CALLBACK,  handler_callback},
   {KW_TEXTCOLOR, handler_textcolor},
   {KW_BGTRANS,   handler_setbgtrans},
   {KW_BGCOLOR,   handler_setbgcolor},
};

//
// DLG_Ticker
//
// The heart of the DSS, this is a mini virtual machine.
// Called by G_Ticker when currentdialog is not NULL;
// May wait on DLG_Drawer to finish drawing the current message
// before running more dialog commands.
//
void DLG_Ticker(void)
{
   if((menuactive && !demoplayback && !netgame) || paused)
      return; // don't run dialog sequences when game is stopped

   // shouldn't happen, but failsafes never killed anybody
   if(gameaction == ga_worlddone || gamestate != GS_LEVEL)
   {
      DLG_Stop();
      return;
   }

   dlgcount++; // always increment the time counter

   if(currentdialog->waittime > 0)
   {
      currentdialog->waittime--;
      
      // finite wait period is over
      if(!currentdialog->waittime)
      {
	 int wt = currentdialog->waittype;

	 currentdialog->waittype = DWT_NONE;

	 if(wt == DWT_MESSAGE || wt == DWT_CLEAR)
	 {
	    if(clearbox) // we are through waiting for the box to clear
	    {
	       clearbox = false;
	    }
	    else         // we need to wait for the box to be cleared
	    {
	       clearbox = true;
	       currentdialog->waittime = CLEARWAIT;
	       currentdialog->waittype = DWT_CLEAR;
	    }
	 }	 
      }
      return;
   }
   else if(currentdialog->waittime < 0) // waiting for message to render
   {
      return;
   }

   // Main dialog loop -- run until another message is found
   do
   {
      int cmd, arg;

      if(currentdialog->ip >= (int *)(currentdialog->stringtable))
      {
	 // end of dialog
	 DLG_Stop();
	 return;
      }

      cmd = LONG(*(currentdialog->ip));
      if(cmd < 0 || cmd >= NUMDIALOGKEYWORDS)
      {
	 // invalid op code
	 DLG_Stop();
	 return;
      }
      currentdialog->ip += 1;

      arg = LONG(*(currentdialog->ip));
      currentdialog->ip += 1;

      // call appropriate handler with retrieved argument
      // if this is a message command, waittime will be set to -1
      // and this loop will exit
      dialogcmds[cmd].handler(arg);

   } while(!currentdialog->waittime);
}

// Renderer-related defines

#define TEXTWIDTH     (SCREENWIDTH - 8)
#define TEXTHEIGHT    64
#define DIALOGPIC_X   8
#define DIALOGPIC_Y   8
#define DIALOGNAME_X  (DIALOGPIC_X + 64 + 1)
#define DIALOGNAME_Y  DIALOGPIC_Y
#define DIALOGYSTEP   9
#define DIALOGTEXT_X  DIALOGNAME_X
#define DIALOGTEXT_Y  (DIALOGNAME_Y + DIALOGYSTEP)
#define DLGTEXTSPEED  3

void DLG_WriteText(void);

void DLG_Drawer(void)
{
   redrawborder = true;

   if((menuactive && !demoplayback && !netgame) || paused)
      return; // don't run dialog sequences when game is stopped

   // not ready to start drawing yet
   if(currentdialog->waittime && currentdialog->waittype == DWT_START)
      return;

   // draw background
   if(bgtrans)
      V_DrawPatchTL(0, 0, &vbscreen, dlgbackground, colrngs[bgcolor], FTRANLEVEL);
   else
      V_DrawPatchTranslated(0, 0, &vbscreen, dlgbackground, colrngs[bgcolor], false);

   // draw character picture
   V_DrawPatch(DIALOGPIC_X, DIALOGPIC_Y, &vbscreen, dlgpic);

   // draw character name
   V_WriteText(dlgname, DIALOGNAME_X, DIALOGNAME_Y);

   if(!clearbox && currentdialog->waittype != DWT_NONE)
   {
      DLG_WriteText();
   }
}

//
// DLG_WriteText
//
// A simplified version of F_TextWrite from f_finale.c, but
// specialized to support drawing within the dialog box bounds and
// with extra support for colored text
//
void DLG_WriteText(void)
{
   int w, count, c, cx, cy;
   unsigned char *ch;  // 08/25/01: needs to be unsigned!
   char *outr;

   cx = DIALOGTEXT_X;
   cy = DIALOGTEXT_Y;
   ch = (unsigned char *)dlgmessage;

   count = (dlgcount - 10) / DLGTEXTSPEED;
   if(count < 0)
      count = 0;

   for(; count; count--)
   {
      c = *ch++;
      if(!c)
      {
	 // start the ticker counting down again
	 // when it reaches 0, it'll set clearbox and the drawer
	 // will stop calling this function
	 if(currentdialog->waittime == -1)
	    currentdialog->waittime = DONEWAIT;
	 break;
      }

      if(c == '\n')
      {
	 cx = DIALOGTEXT_X;
	 cy += DIALOGYSTEP;
	 continue;
      }

      // support \x80 through \x89 for localized color changes
      if(c >= 128)
      {
	 int colnum = c - 128;
	 
	 if(!(colnum < 0 || colnum >= CR_LIMIT))
	    localcolor = colnum;

	 continue;
      }

      c = toupper(c) - V_FONTSTART;
      if(c < 0 || c >= V_FONTSIZE || !v_font[c])
      {
	 cx += 4;
	 continue;
      }

      w = SHORT(v_font[c]->width);
      if(cx + w > TEXTWIDTH)
	 continue; // keep looking in case of \n later
      if(cy > TEXTHEIGHT)
	 continue; // still need to get to end of message anyways

      outr = (localcolor == -1) ? colrngs[textcolor] 
	                        : colrngs[localcolor];

      V_DrawPatchTranslated(cx, cy, &vbscreen, v_font[c], outr, 0);
      cx += w;
   }
   
   // local color needs to be reset at the end of each drawing pass
   localcolor = -1;  
}

// EOF
