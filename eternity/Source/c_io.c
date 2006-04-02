// Emacs style mode select -*- C++ -*-
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
//--------------------------------------------------------------------------
//
// Console I/O
//
// Basic routines: outputting text to the console, main console functions:
//                 drawer, responder, ticker, init
//
// By Simon Howard
//
// NETCODE_FIXME -- CONSOLE_FIXME: Major changes needed in this module!
//
//-----------------------------------------------------------------------------

#include "z_zone.h"

#include "d_io.h" // SoM 3/14/2002: MSCV++
#include "c_io.h"
#include "c_runcmd.h"
#include "c_net.h"

#include "d_event.h"
#include "d_main.h"
#include "doomdef.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "hu_over.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "doomstat.h"
#include "w_wad.h"
#include "s_sound.h"
#include "d_gi.h"
#include "g_bind.h"
#include "a_small.h" // haleyjd

#define MESSAGES 384
// keep the last 32 typed commands
#define HISTORY 32

// SoM 2-4-04: ANYRES
#define C_SCREENHEIGHT v_height
#define C_SCREENWIDTH v_width

extern const char *shiftxform;
void Egg();

// the messages (what you see in the console window)
static unsigned char messages[MESSAGES][LINELENGTH];
static int message_pos = 0;   // position in the history (last line in window)
static int message_last = 0;  // the last message

// the command history(what you type in)
static unsigned char history[HISTORY][LINELENGTH];
static int history_last = 0;
static int history_current = 0;

static const char *inputprompt = FC_HI "$" FC_NORMAL;
// gee what is this for? :)
static const char *altprompt = FC_HI "#" FC_NORMAL;
int c_height=100;     // the height of the console
int c_speed=10;       // pixels/tic it moves
int current_target = 0;
int current_height = 0;
boolean c_showprompt;
static char *backdrop = NULL;
static char inputtext[INPUTLENGTH];
static char *input_point;      // left-most point you see of the command line

// for scrolling command line
static int pgup_down=0, pgdn_down=0;
int console_enabled = true;

// haleyjd 09/07/03: true logging capability
static FILE *console_log = NULL;

/////////////////////////////////////////////////////////////////////////
//
// Main Console functions
//
// ticker, responder, drawer, init etc.
//

static void C_InitBackdrop(void)
{
   patch_t *patch;
   const char *lumpname;
   //byte *oldscreen;
   VBuffer cback;
   
   switch(gamemode)
   {
   case commercial:
   case retail: 
      lumpname = "INTERPIC";
      break;
   case registered: 
      lumpname = "PFUB2";
      break;
   default: 
      lumpname = "TITLEPIC";
      break;
   }
   
   // allow for custom console background graphic
   if(W_CheckNumForName("CONSOLE") >= 0)
      lumpname = "CONSOLE";
   
   if(backdrop)
      Z_Free(backdrop);
   
   backdrop = Z_Malloc(C_SCREENHEIGHT*C_SCREENWIDTH, PU_STATIC, 0);

   // haleyjd 04/03/04: removed hack, setup VBuffer object
   memcpy(&cback, &vbscreen, sizeof(VBuffer));
   cback.data = backdrop;
   cback.width = cback.pitch = C_SCREENWIDTH;
   cback.height = C_SCREENHEIGHT;

   patch = W_CacheLumpName(lumpname, PU_CACHE);

   V_DrawPatch(0, 0, &cback, patch);
}

// input_point is the leftmost point of the inputtext which
// we see. This function is called every time the inputtext
// changes to decide where input_point should be.

//
// CONSOLE_FIXME: See how Quake 2 does this, it's much better.
//
static void C_UpdateInputPoint(void)
{
   for(input_point = inputtext;
       V_StringWidth(input_point) > SCREENWIDTH-20; input_point++);
}

// initialise the console

void C_Init(void)
{
   C_InitBackdrop();
   
   // sf: stupid american spellings =)
   C_NewAlias("color", "colour %opt");
   //C_NewAlias("centermsg", "centremsg %opt");
   
   C_AddCommands();
   C_UpdateInputPoint();
   
   // haleyjd
   G_InitKeyBindings();
}

// called every tic

void C_Ticker(void)
{
   c_showprompt = true;
   
   if(gamestate != GS_CONSOLE)
   {
      // specific to half-screen version only
      
      if(current_height != current_target)
         redrawsbar = true;
      
      // move the console toward its target
      if(D_abs(current_height - current_target) >= c_speed)
      {
         current_height +=
            (current_target < current_height) ? -c_speed : c_speed;
      }
      else
      {
         current_height = current_target;
      }
   }
   else
   {
      // console gamestate: no moving consoles!
      current_target = current_height;
   }
   
   if(consoleactive)  // no scrolling thru messages when fullscreen
   {
      // scroll based on keys down
      message_pos += pgdn_down - pgup_down;
      
      // check we're in the area of valid messages        
      if(message_pos < 0)
         message_pos = 0;
      if(message_pos > message_last)
         message_pos = message_last;
   }
   
   //
   // NETCODE_FIXME -- CONSOLE_FIXME: Buffered command crap.
   // Needs complete rewrite.
   //
   
   C_RunBuffer(c_typed);   // run the delayed typed commands
   C_RunBuffer(c_menu);
}

//
// CONSOLE_FIXME: history needs to be more efficient. Use pointers 
// instead of copying strings back and forth.
//
static void C_AddToHistory(char *s)
{
   const char *t;
   const char *a_prompt;
   
   // display the command in console
   // hrmm wtf does this do? I dunno.
   if(gamestate == GS_LEVEL && !strcasecmp(players[0].name, "quasar"))
   {
      a_prompt = altprompt;
   }
   else
      a_prompt = inputprompt;
   
   C_Printf("%s%s\n", a_prompt, s);
   
   t = s;                  // check for nothing typed
   while(*t == ' ') t++;   // or just spaces
   if(!*t)
      return;
   
   // add it to the history
   // 6/8/99 maximum linelength to prevent segfaults
   strncpy(history[history_last], s, LINELENGTH);
   history_last++;
   
   // scroll the history if neccesary
   while(history_last >= HISTORY)
   {
      int i;
      
      // haleyjd 03/02/02: this loop went one past the end of history
      // and left possible garbage in the higher end of the array
      for(i = 0; i < HISTORY - 1; i++)
      {
         strcpy(history[i], history[i+1]);
      }
      history[HISTORY - 1][0] = '\0';
      
      history_last--;
   }

   history_current = history_last;
   history[history_last][0] = '\0';
}

// respond to keyboard input/events

int C_Responder(event_t* ev)
{
   static int shiftdown;
   char ch;
   
   if(ev->data1 == KEYD_RSHIFT)
   {
      shiftdown = ev->type==ev_keydown;
      return consoleactive;   // eat if console active
   }
   if(ev->data1 == KEYD_PAGEUP)
   {
      pgup_down = ev->type==ev_keydown;
      return consoleactive;
   }
   if(ev->data1 == KEYD_PAGEDOWN)
   {
      pgdn_down = ev->type==ev_keydown;
      return consoleactive;
   }
  
   // only interested in keypresses
   if(ev->type != ev_keydown)
      return 0;
  
   //////////////////////////////////
   // Check for special keypresses
   //
   // detect activating of console etc.
   //
   
   // activate console?
   if(ev->data1 == KEYD_CONSOLE && console_enabled)
   {
      // set console
      current_target = current_target == c_height ? 0 : c_height;
      return true;
   }

   if(!consoleactive) 
      return false;
   
   // not til its stopped moving
   if(current_target < current_height) 
      return false;

   ///////////////////////////////////////
   // Console active commands
   //
   // keypresses only dealt with if console active
   //
   
   // tab-completion
   if(ev->data1 == KEYD_TAB)
   {
      // set inputtext to next or previous in
      // tab-completion list depending on whether
      // shift is being held down
      strcpy(inputtext, shiftdown ? C_NextTab(inputtext) :
             C_PrevTab(inputtext));
      
      C_UpdateInputPoint(); // reset scrolling
      return true;
    }
  
   // run command
   if(ev->data1 == KEYD_ENTER)
   {
      C_AddToHistory(inputtext);      // add to history
      
      if(!strcmp(inputtext, "r0x0rz delux0rz"))
         Egg(); //shh!
      
      // run the command
      cmdtype = c_typed;
      C_RunTextCmd(inputtext);
      
      C_InitTab();            // reset tab completion
      
      inputtext[0] = 0;       // clear inputtext now
      C_UpdateInputPoint();   // reset scrolling
      
      return true;
   }

   ////////////////////////////////
   // Command history
   //  
   
   // previous command
   
   if(ev->data1 == KEYD_UPARROW)
   {
      history_current =
         (history_current <= 0) ? 0 : history_current - 1;
      
      // read history from inputtext
      strcpy(inputtext, history[history_current]);
      
      C_InitTab();            // reset tab completion
      C_UpdateInputPoint();   // reset scrolling
      return true;
   }
  
  // next command
  
   if(ev->data1 == KEYD_DOWNARROW)
   {
      history_current = (history_current >= history_last) 
         ? history_last : history_current + 1;

      // the last history is an empty string
      strcpy(inputtext, (history_current == history_last) ?
             "" : (char*)history[history_current]);
      
      C_InitTab();            // reset tab-completion
      C_UpdateInputPoint();   // reset scrolling
      return true;
   }

   /////////////////////////////////////////
   // Normal Text Input
   //
   
   // backspace
   
   if(ev->data1 == KEYD_BACKSPACE)
   {
      if(strlen(inputtext) > 0)
         inputtext[strlen(inputtext) - 1] = '\0';
      
      C_InitTab();            // reset tab-completion
      C_UpdateInputPoint();   // reset scrolling
      return true;
   }

   // none of these, probably just a normal character
   
   ch = shiftdown ? shiftxform[ev->data1] : ev->data1; // shifted?

   // only care about valid characters
   // dont allow too many characters on one command line
   
   if(ch > 31 && ch < 127 && (strlen(inputtext) < INPUTLENGTH - 1))
   {
      psnprintf(inputtext, sizeof(inputtext), "%s%c", inputtext, ch);
      
      C_InitTab();            // reset tab-completion
      C_UpdateInputPoint();   // reset scrolling
      return true;
   }
   
   return false;   // dont care about this event
}


// draw the console

//
// CONSOLE_FIXME: Support other fonts. Break messages across lines during
// drawing instead of during message insertion? It should be possible
// although it would complicate the scrolling logic.
//

void C_Drawer(void)
{
   int y;
   int count;
   int real_height = (current_height * globalyscale) >> FRACBITS;
   static int oldscreenheight = 0;
   
   if(!consoleactive) 
      return;   // dont draw if not active

   // Check for change in screen res
   
   if(oldscreenheight != C_SCREENHEIGHT)
   {
      C_InitBackdrop();       // re-init to the new screen size
      oldscreenheight = C_SCREENHEIGHT;
   }

   // fullscreen console for fullscreen mode
   if(gamestate == GS_CONSOLE)
      current_height = SCREENHEIGHT;


   // draw backdrop

   memcpy(screens[0],
          backdrop + (C_SCREENHEIGHT-real_height)*C_SCREENWIDTH,
          real_height*C_SCREENWIDTH);

   //////////////////////////////////////////////////////////////////////
   // draw text messages
   
   // offset starting point up by 8 if we are showing input prompt
   
   y = current_height - ((c_showprompt && message_pos==message_last) ? 8 : 0);

   // start at our position in the message history
   count = message_pos;
        
   while(1)
   {
      // move up one line on the screen
      // back one line in the history
      y -= 8;
      
      if(--count < 0) break;    // end of message history?
      if(y < 0) break;        // past top of screen?
      
      // draw this line
      V_WriteText(messages[count], 0, y);
   }

   //////////////////////////////////
   // Draw input line
   //
  
   // input line on screen, not scrolled back in history?
   
   if(current_height > 8 && c_showprompt && message_pos == message_last)
   {
      const char *a_prompt;
      char tempstr[LINELENGTH];
      
      // if we are scrolled back, dont draw the input line
      if(message_pos == message_last)
      {
         if(gamestate == GS_LEVEL && !strcasecmp(players[0].name, "quasar"))
            a_prompt = altprompt;
         else
            a_prompt = inputprompt;

         psnprintf(tempstr, sizeof(tempstr), 
                   "%s%s_", a_prompt, input_point);
      }
      
      V_WriteText(tempstr, 0, current_height-8);
   }
}

// updates the screen without actually waiting for d_display
// useful for functions that get input without using the gameloop
// eg. serial code

void C_Update(void)
{
   C_Drawer();
   I_FinishUpdate();
}

/////////////////////////////////////////////////////////////////////////
//
// I/O Functions
//

// scroll console up

//
// CONSOLE_FIXME: message buffer can be made more efficient.
//

static void C_ScrollUp(void)
{
   if(message_last == message_pos)
      message_pos++;   
   message_last++;

   if(message_last >= MESSAGES) // past the end of the string
   {
      // cut off the oldest 128 messages
      int i;
      
      // haleyjd 03/02/02: fixed code that assumed MESSAGES == 256
      for(i = 128; i < MESSAGES; i++)
         strcpy(messages[i - 128], messages[i]);
      
      message_last -= 128;      // move the message boundary

      // haleyjd 09/04/02: set message_pos to message_last
      // to avoid problems with console flooding
      message_pos = message_last;
   }
   
   messages[message_last][0] = '\0';  // new line is empty
}

// 
// C_AddMessage
//
// haleyjd:
// Add a message to the console.
// Replaced C_AddChar.
//
// CONSOLE_FIXME: A horrible mess, needs to be rethought entirely.
// See Quake 2's system for ideas.
//
static void C_AddMessage(const char *s)
{
   const unsigned char *c;
   unsigned char *end;
   unsigned char linecolor = gameModeInfo->colorNormal + 128;

   // haleyjd 09/04/02: set color to default at beginning
   if(V_StringWidth(messages[message_last]) > SCREENWIDTH-9 ||
      strlen(messages[message_last]) >= LINELENGTH - 1)
   {
      C_ScrollUp();
   }
   end = messages[message_last] + strlen(messages[message_last]);
   *end++ = linecolor;
   *end = '\0';

   for(c = (const unsigned char *)s; *c; c++)
   {
      // >= 128 for colours
      if(*c == '\t' || (*c > 31 && *c < 127) || *c >= 128)
      {
         if(*c >= 128)
            linecolor = *c;

         if(V_StringWidth(messages[message_last]) > SCREENWIDTH-9 ||
            strlen(messages[message_last]) >= LINELENGTH - 1)
         {
            // might possibly over-run, go onto next line
            C_ScrollUp();
            end = messages[message_last] + strlen(messages[message_last]);
            *end++ = linecolor; // keep current color on next line
            *end = '\0';
         }
         
         end = messages[message_last] + strlen(messages[message_last]);
         *end++ = *c;
         *end = '\0';
      }
      if(*c == '\a') // alert
      {
         S_StartSound(NULL, gameModeInfo->c_BellSound); // 'tink'!
      }
      if(*c == '\n')
      {
         C_ScrollUp();
         end = messages[message_last] + strlen(messages[message_last]);
         *end++ = linecolor; // keep current color on next line
         *end = '\0';
      }
   }
}

// haleyjd: this function attempts to break up formatted strings 
// into segments no more than a gamemode-dependent number of 
// characters long. It'll succeed as long as the string in question 
// doesn't contain that number of consecutive characters without a 
// space, tab, or line-break, so like, don't print stupidness 
// like that. Its a console, not a hex editor...
//
// CONSOLE_FIXME: See above, this is also a mess.
//
static void C_AdjustLineBreaks(char *str)
{
   int i, count, firstspace, lastspace, len;

   firstspace = -1;

   count = lastspace = 0;

   len = strlen(str);

   for(i = 0; i < len; ++i)
   {
      if(str[i] == ' ' || str[i] == '\t')
      {
         if(firstspace == -1)
            firstspace = i;

         lastspace = i;
      }
      
      if(str[i] == '\n')
         count = lastspace = 0;
      else
         count++;

      if(count == gameModeInfo->c_numCharsPerLine)
      {
         // 03/16/01: must add length since last space to new line
         count = i - (lastspace + 1);
         
         // replace last space with \n
         // if no last space, we're screwed
         if(lastspace)
         {
            if(lastspace == firstspace)
               firstspace = 0;
            str[lastspace] = '\n';
            lastspace = 0;
         }
      }
   }

   if(firstspace)
   {      
      // temporarily put a \0 in the first space
      char temp = str[firstspace];
      str[firstspace] = '\0';

      // if the first segment of the string doesn't fit on the 
      // current line, move the console up one line in advance

      if(V_StringWidth(str) + V_StringWidth(messages[message_last]) > SCREENWIDTH - 9
         || strlen(str) + strlen(messages[message_last]) >= LINELENGTH - 1)
      {
         C_ScrollUp();
      }

      // restore the string to normal
      str[firstspace] = temp;
   }
}

static void C_AppendToLog(const char *text);

//
// C_Printf
//
// Write some text 'printf' style to the console.
// The main function for I/O.
//
// CONSOLE_FIXME: As the two above, this needs to be adjusted to
// work better with any new console message buffering system that
// is designed.
//
void C_Printf(const char *s, ...)
{
   char tempstr[1024];
   va_list args;

   // haleyjd: sanity check
   if(!s)
      return;
   
   va_start(args, s);
   pvsnprintf(tempstr, sizeof(tempstr), s, args);
   va_end(args);

   // haleyjd: write this message to the log if one is open
   C_AppendToLog(tempstr); 
   
   C_AdjustLineBreaks(tempstr); // haleyjd
   
   C_AddMessage(tempstr);
}

// haleyjd 01/24/03: got rid of C_WriteText, added real C_Puts from
// prboom

void C_Puts(const char *s)
{
   C_Printf("%s\n", s);
}


void C_Separator(void)
{
   C_Puts("{|||||||||||||||||||||||||||||}");
}

//
// C_StripColorChars
//
// haleyjd 09/07/03: Abstracted out of the below function, this
// code copies from src to dest, skipping any characters greater
// than 128 in value. This prevents console logs and dumps from
// having a bunch of meaningless extended ASCII in them.
//
static void C_StripColorChars(const unsigned char *src, 
                              unsigned char *dest, 
                              int len)
{
   register int srcidx = 0, destidx = 0;

   for(; srcidx < len; ++srcidx)
   {
      if(src[srcidx] < 128)
      {
         dest[destidx] = src[srcidx];
         ++destidx;
      }
   }
}

// 
// C_DumpMessages
//
// haleyjd 03/01/02: now you can dump the console to file :)
//
void C_DumpMessages(const char *filename)
{
   int i, len;
   FILE *outfile;
   unsigned char tmpmessage[LINELENGTH];

   memset(tmpmessage, 0, LINELENGTH);

   if(!(outfile = fopen(filename, "a+")))
   {
      C_Printf(FC_ERROR"Could not append console buffer to file %s\n", 
               filename);
      return;
   }

   for(i = 0; i < message_last; i++)
   {
      // strip color codes from strings
      memset(tmpmessage, 0, LINELENGTH);
      len = strlen(messages[i]);

      C_StripColorChars(messages[i], tmpmessage, len);

      fprintf(outfile, "%s\n", tmpmessage);
   }

   fclose(outfile);

   C_Printf("Console buffer appended to file %s\n", filename);
}

//
// C_OpenConsoleLog
//
// haleyjd 09/07/03: true console logging
//
void C_OpenConsoleLog(const char *filename)
{
   // don't do anything if a log is already open
   if(console_log)
      return;

   // open file in append mode
   if(!(console_log = fopen(filename, "a+")))
   {
      C_Printf(FC_ERROR"Couldn't open file %s for console logging\n", 
               filename);
   }
   else
      C_Printf("Opened file %s for console logging\n", filename);
}

//
// C_CloseConsoleLog
//
// haleyjd 09/07/03: true console logging
//
void C_CloseConsoleLog(void)
{
   if(console_log)
      fclose(console_log);

   console_log = NULL;
}

//
// C_AppendToLog
//
// haleyjd 09/07/03: true console logging
//
static void C_AppendToLog(const char *text)
{
   // only if console logging is enabled
   if(console_log)
   {
      int len;
      unsigned char tmpmessage[1024];
      const unsigned char *src = (const unsigned char *)text;

      memset(tmpmessage, 0, 1024);
      len = strlen(text);

      C_StripColorChars(src, tmpmessage, len);

      fprintf(console_log, "%s", tmpmessage);
      fflush(console_log);
   }
}

///////////////////////////////////////////////////////////////////
//
// Console activation
//

// put smmu into console mode

void C_SetConsole(void)
{
   gamestate = GS_CONSOLE;
   gameaction = ga_nothing;
   current_height = SCREENHEIGHT;
   current_target = SCREENHEIGHT;
   
   S_StopMusic();                  // stop music if any
   S_StopSounds();                 // and sounds
   G_StopDemo();                   // stop demo playing
}

// make the console go up

void C_Popup(void)
{
   current_target = 0;
}

// make the console disappear

void C_InstaPopup(void)
{
   current_target = current_height = 0;
}

//
// Small native functions
//

//
// sm_c_print
//
// Implements ConsolePrint(const string[], ...)
//
// Prints preformatted messages to the console
//
static cell AMX_NATIVE_CALL sm_c_print(AMX *amx, cell *params)
{
   int i, j;
   int err;
   int len;
   int totallen = 0;
   cell *cstr;
   char **msgs;
   char *msg;

   int numparams = (int)(params[0] / sizeof(cell));

   // create a string table
   msgs = Z_Malloc(numparams * sizeof(char *), PU_STATIC, NULL);

   memset(msgs, 0, numparams * sizeof(char *));

   for(i = 1; i <= numparams; i++)
   {      
      // translate reference parameter to physical address
      if((err = amx_GetAddr(amx, params[i], &cstr)) != AMX_ERR_NONE)
      {
         amx_RaiseError(amx, err);

         // clean up allocations before exit
         for(j = 0; j < numparams; j++)
         {
            if(msgs[j])
               Z_Free(msgs[j]);
         }
         Z_Free(msgs);

         return 0;
      }

      // get length of string
      amx_StrLen(cstr, &len);

      msgs[i-1] = Z_Malloc(len + 1, PU_STATIC, NULL);

      // convert from small string to C string
      amx_GetString(msgs[i-1], cstr, 0);
   }

   // calculate total strlen
   for(i = 0; i < numparams; i++)
      totallen += (int)strlen(msgs[i]);
  
   // create complete message
   msg = Z_Malloc(totallen + 1, PU_STATIC, NULL);

   memset(msg, 0, totallen + 1);

   for(i = 0; i < numparams; i++)
      strcat(msg, msgs[i]);

   C_Printf(msg);

   // free all allocations (fun!)
   Z_Free(msg);                    // complete message
   for(i = 0; i < numparams; i++)  // individual strings
      Z_Free(msgs[i]);   
   Z_Free(msgs);                   // string table

   return 0;
}

//
// sm_consolehr
//
// Implements ConsoleHR()
//
// Prints a <hr> style separator bar to the console --
// this is possible through ConsolePrint as well, but doesn't
// look pretty on the user end.
//
static cell AMX_NATIVE_CALL sm_consolehr(AMX *amx, cell *params)
{
   C_Separator();

   return 0;
}

static cell AMX_NATIVE_CALL sm_consolebeep(AMX *amx, cell *params)
{
   C_Printf("\a");

   return 0;
}

AMX_NATIVE_INFO cons_io_Natives[] =
{
   { "_ConsolePrint", sm_c_print },
   { "_ConsoleHR",    sm_consolehr },
   { "_ConsoleBeep",  sm_consolebeep },
   { NULL, NULL }
};

//
// CONSOLE_FIXME: Should probably either disambiguate this easter
// egg or just get rid of it. It tiles fraggle's head (the graphics
// are stored in an array named "egg" that is in another file) over
// the console background. It's kind of cute so I'd like to keep it
// around.
//

#define E extern
#define U unsigned
#define C char
#define I int
#define V void
#define F for
#define Z FC_BROWN
#define C_W C_SCREENWIDTH
#define C_H C_SCREENHEIGHT
#define s0 screens[0]
#define WT HU_WriteText
#define bd backdrop

V Egg(V){C *os;I x,y;E U C egg[];F(x=0;x<C_W;x++)F(y=0;y<C_H
;y++){U C *s=egg+((y%44)*42)+(x%42);if(*s!=247)bd[y*C_W+x]=*
s;}os=s0;s0=bd;WT(Z"my hair looks much too\n dark in this p"
"ic.\noh well, have fun!\n      -- fraggle",160,168);s0=os;}

// EOF
