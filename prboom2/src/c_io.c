/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
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
 *
 * Console I/O
 *
 * Basic routines: outputting text to the console, main console functions:
 *                 drawer, responder, ticker, init
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "psnprntf.h"

#include "c_io.h"
#include "c_runcmd.h"
#include "c_net.h"

#include "d_event.h"
#include "d_main.h"
#include "doomdef.h"
#include "g_game.h"
#include "g_bind.h"
#include "g_bindaxes.h"
#include "hu_stuff.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"
#include "doomstat.h"
#include "w_wad.h"
#include "s_sound.h"
#include "sounds.h"

#define MESSAGES 384
// keep the last 32 typed commands
#define HISTORY 32

extern const char *shiftxform;

// the messages (what you see in the console window)
static unsigned char messages[MESSAGES][LINELENGTH];
static int message_pos=0;      // position in the history (last line in window)
static int message_last=0;     // the last message

// the command history(what you type in)
static unsigned char history[HISTORY][LINELENGTH];
static int history_last=0;
static int history_current=0;

static const char *inputprompt = FC_HI "$" FC_NORMAL;
int c_height=100;     // the height of the console
int c_speed=10;       // pixels/tic it moves
int current_target = 0;
int current_height = 0;
boolean c_showprompt;
static int backdrop_lumpnum = -1;
static char inputtext[INPUTLENGTH];
static char *input_point;      // left-most point you see of the command line

// for scrolling command line
static int pgup_down=0, pgdn_down=0;
int console_enabled = true;


/////////////////////////////////////////////////////////////////////////
//
// Main Console functions
//
// ticker, responder, drawer, init etc.
//

void C_InitBackdrop(void)
{
  const char *lumpname;
  TScreenVars oldscreen = {NULL,0,0};
  
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

  backdrop_lumpnum = W_GetNumForName(lumpname);
}

// input_point is the leftmost point of the inputtext which
// we see. This function is called every time the inputtext
// changes to decide where input_point should be.

static void C_UpdateInputPoint(void)
{
  for(input_point=inputtext;
      V_StringWidth(input_point) > 320-20; input_point++);
}

// initialise the console

void C_Init(void)
{
  // sf: stupid american spellings =)
  C_NewAlias("color", "colour %opt");
  C_NewAlias("centermsg", "centremsg %opt");
  
  C_AddCommands();
  C_UpdateInputPoint();

  G_InitKeyBindings();
  G_InitAxisBindings();
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
    if(D_abs(current_height-current_target)>=c_speed)
	    current_height += current_target<current_height ? -c_speed : c_speed;
    else
	    current_height = current_target;
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

  C_RunBuffer(c_typed);   // run the delayed typed commands
  C_RunBuffer(c_menu);
}

static void C_AddToHistory(char *s)
{
  char *t;
  
  // display the command in console
  C_Printf("%s%s\n", inputprompt, s);
  
  t = s;                  // check for nothing typed
  while(*t==' ') t++;     // or just spaces
  if(!*t) return; 
  
  // add it to the history
  // 6/8/99 maximum linelength to prevent segfaults
  // -3 for safety
  strncpy(history[history_last], s, LINELENGTH-3);
  history_last++;

  // scroll the history if neccesary
  while(history_last >= HISTORY)
    {
      int i;
      
      // haleyjd 03/02/02: this loop went one past the end of history
      // and left possible garbage in the higher end of the array
      for(i=0; i<HISTORY-1; i++)
	      strcpy(history[i], history[i+1]);
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
    return false;
  
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
        (history_current <= 0) ? 0 : history_current-1;
      
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
        ? history_last : history_current+1;

      // the last history is an empty string
      strcpy(inputtext, (history_current == history_last) ?
	     "" : (char*)history[history_current]);
      
      C_InitTab();            // reset tab-completion
      C_UpdateInputPoint();   // reset scrolling
      return true;
    }

  if(ev->data1 == KEYD_END)
    {
      message_pos = message_last;
      return true;
    }

  /////////////////////////////////////////
  // Normal Text Input
  //
  
  // backspace
  
  if(ev->data1 == KEYD_BACKSPACE)
    {
      if(strlen(inputtext) > 0)
	      inputtext[strlen(inputtext)-1] = '\0';
      
      C_InitTab();            // reset tab-completion
      C_UpdateInputPoint();   // reset scrolling
      return true;
    }

  // none of these, probably just a normal character

  ch = shiftdown ? shiftxform[ev->data1] : ev->data1; // shifted?

  // only care about valid characters
  // dont allow too many characters on one command line
  // sf 19/6 V_IsPrint
  
  if(V_IsPrint(ch) && strlen(inputtext) < INPUTLENGTH-3)
    {
      psnprintf(inputtext, INPUTLENGTH, "%s%c", inputtext, ch);
      
      C_InitTab();            // reset tab-completion
      C_UpdateInputPoint();   // reset scrolling
      return true;
    }
  
  return false;   // dont care about this event
}


// draw the console

void C_Drawer(void)
{
  int y;
  int count;
  static int oldscreenheight=0;
  
  if(!consoleactive)
    return;   // dont draw if not active

  if (backdrop_lumpnum < 0)
    C_InitBackdrop();

  // fullscreen console for fullscreen mode
  if (gamestate == GS_CONSOLE)
    current_height = 200;


  // draw backdrop
  V_DrawNumPatch(0, current_height-200, 0, backdrop_lumpnum, CR_DEFAULT, VPT_STRETCH);
  
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
    unsigned char tempstr[LINELENGTH];
      
    // if we are scrolled back, dont draw the input line
    if(message_pos == message_last)
	    psnprintf(tempstr, LINELENGTH, "%s%s_", inputprompt, input_point);
      
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

static void C_ScrollUp(void)
{
  if(message_last == message_pos)
    message_pos++;
  message_last++;

  if(message_last >= MESSAGES)       // past the end of the string
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
static void C_AddMessage(const char *s)
{
  const unsigned char *c;
  unsigned char *end;
  unsigned char linecolor = CR_RED + FC_BASEVALUE;

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
      S_StartSound(NULL, sfx_tink); // 'tink'!
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

#define MAX_MYCHARSPERLINE 45

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

    if(count == MAX_MYCHARSPERLINE)
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

/* C_Printf -
 * write some text 'printf' style to the console
 * the main function for I/O
 * cph 2001/07/22 - remove arbitrary limit, use malloc'd buffer instead
 *  and make format string parameter const char*
 */

void C_Printf(const char *s, ...)
{
  va_list args;
  char *t;
  
  // haleyjd: sanity check
  if(!s)
    return;
  
  // difficult to remove limit
  va_start(args, s);
#ifdef HAVE_VASPRINTF
  vasprintf(&t, s, args);
#else
  /* cph 2001/08/05 - since we use the libc vasprintf above, which uses the libc
   * malloc, we must force the libc malloc(3) here and free(3) below
   */
  t = (malloc)(2048);
  pvsnprintf(t,2047,s,args);
#endif
  va_end(args);

  C_AdjustLineBreaks(t); // haleyjd

  C_AddMessage(t);
  
  (free)(t);
}

// write a line of text to the console
// kind of redundant now, #defined as c_puts also

void C_Puts(const char *s)
{
  C_Printf("%s\n", s);
}

void C_Seperator()
{
  C_Puts("{|||||||||||||||||||||||||||||}\n");
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
  current_height = 200;
  current_target = 200;
  
  C_Update();
  S_StopMusic();                  // stop music if any
  S_StopSounds();                 // and sounds
  //G_StopDemo();                   // stop demo playing
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

