/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_video.c,v 1.37 2002/08/10 18:23:09 proff_fs Exp $
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
 *	DOOM graphics stuff for SDL
 *
 *-----------------------------------------------------------------------------
 */

static const char
rcsid[] = "$Id: i_video.c,v 1.37 2002/08/10 18:23:09 proff_fs Exp $";

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "SDL.h"

#include "i_system.h"
#include "m_argv.h"
#include "doomstat.h"
#include "doomdef.h"
#include "doomtype.h"
#include "v_video.h"
#include "r_draw.h"
#include "d_main.h"
#include "d_event.h"
#include "i_video.h"
#include "i_axes.h"
#include "z_zone.h"
#include "s_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "lprintf.h"
#include "c_runcmd.h"

#ifdef GL_DOOM
#include "gl_struct.h"

int gl_colorbuffer_bits=16;
int gl_depthbuffer_bits=16;

#endif

extern void M_QuitDOOM(int choice);

int use_doublebuffer = 1;
int use_fullscreen = 1;
static SDL_Surface *screen;

unsigned char* out_buffer = NULL;

//// TEMPORARY /////////
int enablejoystick = 1;

////////////////////////////////////////////////////////////////////////////
// Input code 
int             leds_always_off = 0; // Expected by m_misc, not relevant 

// Mouse handling
int     usemouse = 1;        // config file var
static int doubleclicktime[3]={0,0,0};
static int doubleclicks[3]={0,0,0};
static int eventtime;

/////////////////////////////////////////////////////////////////////////////////
// Keyboard handling

//
//  Translates the key currently in key
//

static int I_TranslateKey(SDL_keysym* key)
{
  int rc = 0;

  switch (key->sym) {
  case SDLK_LEFT:	rc = KEYD_LEFTARROW;	break;
  case SDLK_RIGHT:	rc = KEYD_RIGHTARROW;	break;
  case SDLK_DOWN:	rc = KEYD_DOWNARROW;	break;
  case SDLK_UP:		rc = KEYD_UPARROW;	break;
  case SDLK_ESCAPE:	rc = KEYD_ESCAPE;	break;
  case SDLK_RETURN:	rc = KEYD_ENTER;	break;
  case SDLK_TAB:	rc = KEYD_TAB;		break;
  case SDLK_F1:		rc = KEYD_F1;		break;
  case SDLK_F2:		rc = KEYD_F2;		break;
  case SDLK_F3:		rc = KEYD_F3;		break;
  case SDLK_F4:		rc = KEYD_F4;		break;
  case SDLK_F5:		rc = KEYD_F5;		break;
  case SDLK_F6:		rc = KEYD_F6;		break;
  case SDLK_F7:		rc = KEYD_F7;		break;
  case SDLK_F8:		rc = KEYD_F8;		break;
  case SDLK_F9:		rc = KEYD_F9;		break;
  case SDLK_F10:	rc = KEYD_F10;		break;
  case SDLK_F11:	rc = KEYD_F11;		break;
  case SDLK_F12:	rc = KEYD_F12;		break;
  case SDLK_BACKSPACE:	rc = KEYD_BACKSPACE;	break;
  case SDLK_DELETE:	rc = KEYD_DEL;	break;
  case SDLK_INSERT:	rc = KEYD_INSERT;	break;
  case SDLK_PAGEUP:	rc = KEYD_PAGEUP;	break;
  case SDLK_PAGEDOWN:	rc = KEYD_PAGEDOWN;	break;
  case SDLK_HOME:	rc = KEYD_HOME;	break;
  case SDLK_END:	rc = KEYD_END;	break;
  case SDLK_PAUSE:	rc = KEYD_PAUSE;	break;
  case SDLK_EQUALS:	rc = KEYD_EQUALS;	break;
  case SDLK_MINUS:	rc = KEYD_MINUS;	break;
  case SDLK_KP0:	rc = KEYD_KEYPAD0;	break;
  case SDLK_KP1:	rc = KEYD_KEYPAD1;	break;
  case SDLK_KP2:	rc = KEYD_KEYPAD2;	break;
  case SDLK_KP3:	rc = KEYD_KEYPAD3;	break;
  case SDLK_KP4:	rc = KEYD_KEYPAD4;	break;
  case SDLK_KP5:	rc = KEYD_KEYPAD5;	break;
  case SDLK_KP6:	rc = KEYD_KEYPAD6;	break;
  case SDLK_KP7:	rc = KEYD_KEYPAD7;	break;
  case SDLK_KP8:	rc = KEYD_KEYPAD8;	break;
  case SDLK_KP9:	rc = KEYD_KEYPAD9;	break;
  case SDLK_KP_PLUS:	rc = KEYD_KEYPADPLUS;	break;
  case SDLK_KP_MINUS:	rc = KEYD_KEYPADMINUS;	break;
  case SDLK_KP_DIVIDE:	rc = KEYD_KEYPADDIVIDE;	break;
  case SDLK_KP_MULTIPLY: rc = KEYD_KEYPADMULTIPLY; break;
  case SDLK_KP_ENTER:	rc = KEYD_KEYPADENTER;	break;
  case SDLK_KP_PERIOD:	rc = KEYD_KEYPADPERIOD;	break;
  case SDLK_LSHIFT:
  case SDLK_RSHIFT:	rc = KEYD_RSHIFT;	break;
  case SDLK_LCTRL:
  case SDLK_RCTRL:	rc = KEYD_RCTRL;	break;
  case SDLK_LALT:
  case SDLK_LMETA:
  case SDLK_RALT:
  case SDLK_RMETA:	rc = KEYD_RALT;		break;
  case SDLK_CAPSLOCK: rc = KEYD_CAPSLOCK; break;
  default:		rc = key->sym;		break;
  }

  return rc;

}

// Null keyboard translation to satisfy m_misc.c
int I_DoomCode2ScanCode(int c)
{
  return c;
}

int I_ScanCode2DoomCode(int c)
{
  return c;
}

/////////////////////////////////////////////////////////////////////////////////
// Main input code

static void I_GetEvent(SDL_Event *Event)
{
  event_t event;

  switch (Event->type) {
  case SDL_KEYDOWN:
    event.type = ev_keydown;
    event.data1 = I_TranslateKey(&Event->key.keysym);
    D_PostEvent(&event);
    break;

  case SDL_KEYUP:
    event.type = ev_keyup;
    event.data1 = I_TranslateKey(&Event->key.keysym);
    D_PostEvent(&event);
    break;

  case SDL_MOUSEBUTTONDOWN:
    if (usemouse)
    {
      switch (Event->button.button)
      {
      case SDL_BUTTON_LEFT:
        event.type = ev_keydown;
        event.data1 = KEYD_MOUSE1;
        D_PostEvent(&event);
        if (doubleclicktime[0])
        {
          if ((eventtime-doubleclicktime[0])<20)
            doubleclicks[0]++;
          if (doubleclicks[0]==2)
          {
            event.type = ev_keydown;
            event.data1 = KEYD_MOUSED1;
            D_PostEvent(&event);
          }
        }
        else
        {
          doubleclicks[0]=1;
          doubleclicktime[0]=eventtime;
        }
        break;
      case SDL_BUTTON_RIGHT:
        event.type = ev_keydown;
        event.data1 = KEYD_MOUSE2;
        D_PostEvent(&event);
        if (doubleclicktime[1])
        {
          if ((eventtime-doubleclicktime[1])<20)
            doubleclicks[1]++;
          if (doubleclicks[1]==2)
          {
            event.type = ev_keydown;
            event.data1 = KEYD_MOUSED2;
            D_PostEvent(&event);
          }
        }
        else
        {
          doubleclicks[1]=1;
          doubleclicktime[1]=eventtime;
        }
        break;
      case SDL_BUTTON_MIDDLE:
        event.type = ev_keydown;
        event.data1 = KEYD_MOUSE3;
        D_PostEvent(&event);
        if (doubleclicktime[2])
        {
          if ((eventtime-doubleclicktime[2])<20)
            doubleclicks[2]++;
          if (doubleclicks[2]==2)
          {
            event.type = ev_keydown;
            event.data1 = KEYD_MOUSED3;
            D_PostEvent(&event);
          }
        }
        else
        {
          doubleclicks[2]=1;
          doubleclicktime[2]=eventtime;
        }
        break;
      }
    }
    break;

  case SDL_MOUSEBUTTONUP:
  if (usemouse)
  {
    switch (Event->button.button)
    {
    case SDL_BUTTON_LEFT:
      event.type = ev_keyup;
      event.data1 = KEYD_MOUSE1;
      D_PostEvent(&event);
      break;
    case SDL_BUTTON_RIGHT:
      event.type = ev_keyup;
      event.data1 = KEYD_MOUSE2;
      D_PostEvent(&event);
      break;
    case SDL_BUTTON_MIDDLE:
      event.type = ev_keyup;
      event.data1 = KEYD_MOUSE3;
      D_PostEvent(&event);
      break;
    }
  }
  break;

  case SDL_MOUSEMOTION:
  if (usemouse) {
    event.type = ev_mouse;
    event.data1 = 0;
    event.data2 = Event->motion.xrel << 5;
    event.data3 = -Event->motion.yrel << 5;
    D_PostEvent(&event);
  }
  break;

  case SDL_JOYBUTTONDOWN:
  case SDL_JOYBUTTONUP:
  if (enablejoystick) {
    if ((Event->jbutton.state) == SDL_PRESSED)
      event.type = ev_keydown;
    if ((Event->jbutton.state) == SDL_RELEASED)
      event.type = ev_keyup;
    switch (Event->jbutton.button)
    {
      case 0:
        event.data1 = KEYD_JOY1;
        break;
      case 1:
        event.data1 = KEYD_JOY2;
        break;
      case 2:
        event.data1 = KEYD_JOY3;
        break;
      case 3:
        event.data1 = KEYD_JOY4;
        break;
      case 4:
        event.data1 = KEYD_JOY5;
        break;
      case 5:
        event.data1 = KEYD_JOY6;
        break;
      case 6:
        event.data1 = KEYD_JOY7;
        break;
      case 7:
        event.data1 = KEYD_JOY8;
        break;
      case 8:
        event.data1 = KEYD_JOY9;
        break;
      case 9:
        event.data1 = KEYD_JOY10;
        break;
      case 10:
        event.data1 = KEYD_JOY11;
        break;
      case 11:
        event.data1 = KEYD_JOY12;
        break;
      case 12:
        event.data1 = KEYD_JOY13;
        break;
      case 13:
        event.data1 = KEYD_JOY14;
        break;
      case 14:
        event.data1 = KEYD_JOY15;
        break;
      case 15:
        event.data1 = KEYD_JOY16;
        break;
    }
    D_PostEvent(&event);
  }
  break;
        

  case SDL_JOYAXISMOTION:
  if (enablejoystick) {
    event.type = ev_axis;
    event.data1 = Event->jaxis.which;
    event.data2 = Event->jaxis.axis;
    event.data3 = (256 * Event->jaxis.value) / 32768;
    D_PostEvent(&event);
  }
  break;

  case SDL_QUIT:
    S_StartSound(NULL, sfx_swtchn);
    C_RunTextCmd("quit");

  default:
    break;
  }
}


//
// I_StartTic
//
static int mouse_currently_grabbed;

void I_StartTic (void)
{
  SDL_Event Event;
  event_t event;

  {
    int should_be_grabbed = usemouse &&
	    !(paused || (gamestate != GS_LEVEL) || demoplayback); 

    if (mouse_currently_grabbed != should_be_grabbed)
      SDL_WM_GrabInput((mouse_currently_grabbed = should_be_grabbed) 
		      ? SDL_GRAB_ON : SDL_GRAB_OFF);
  }

  eventtime=I_GetTime_RealTime();
  if (((eventtime-doubleclicktime[0])>=20) || (doubleclicks[0]>=2))
  {
    event.type = ev_keyup;
    event.data1 = KEYD_MOUSED1;
    D_PostEvent(&event);
    doubleclicktime[0]=0;
    doubleclicks[0]=0;
  }
  if (((eventtime-doubleclicktime[1])>=20) || (doubleclicks[1]>=2))
  {
    event.type = ev_keyup;
    event.data1 = KEYD_MOUSED2;
    D_PostEvent(&event);
    doubleclicktime[1]=0;
    doubleclicks[1]=0;
  }
  if (((eventtime-doubleclicktime[2])>=20) || (doubleclicks[2]>=2))
  {
    event.type = ev_keyup;
    event.data1 = KEYD_MOUSED3;
    D_PostEvent(&event);
    doubleclicktime[2]=0;
    doubleclicks[2]=0;
  }

  while ( SDL_PollEvent(&Event) )
    I_GetEvent(&Event);
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
}

//
// I_InitInputs
//

static void I_InitInputs(void)
{
  I_InitAxes();
}
/////////////////////////////////////////////////////////////////////////////

// I_SkipFrame
//
// Returns true if it thinks we can afford to skip this frame

inline static boolean I_SkipFrame(void)
{
  static int frameno;

  frameno++;
  switch (gamestate) {
  case GS_LEVEL:
    if (!paused)
      return false;
  default:
    // Skip odd frames
    return (frameno & 1) ? true : false;
  }
}

///////////////////////////////////////////////////////////
// Palette stuff.
//
static void I_UploadNewPalette(int pal)
{
  // This is used to replace the current 256 colour cmap with a new one
  // Used by 256 colour PseudoColor modes

  // Array of SDL_Color structs used for setting the 256-colour palette 
  static SDL_Color* colours;
  static int cachedgamma;
  static size_t num_pals;

#ifdef GL_DOOM
  return;
#endif
  if ((colours == NULL) || (cachedgamma != usegamma)) {
    int pplump = W_GetNumForName("PLAYPAL");
    int gtlump = (W_CheckNumForName)("GAMMATBL",ns_prboom);
    register const byte * palette = W_CacheLumpNum(pplump);
    register const byte * const gtable = (const byte *)W_CacheLumpNum(gtlump) + 256*(cachedgamma = usegamma);
    register int i;

    num_pals = W_LumpLength(pplump) / (3*256);
    num_pals *= 256;

    if (!colours) {
      // First call - allocate and prepare colour array
      colours = malloc(sizeof(*colours)*num_pals);
    }

    // set the colormap entries
    for (i=0 ; (size_t)i<num_pals ; i++) {
      colours[i].r = gtable[palette[0]];
      colours[i].g = gtable[palette[1]];
      colours[i].b = gtable[palette[2]];
      palette += 3;
    }
  
    W_UnlockLumpNum(pplump);
    W_UnlockLumpNum(gtlump);
    num_pals/=256;
  }

#ifdef RANGECHECK
  if ((size_t)pal >= num_pals) 
    I_Error("I_UploadNewPalette: Palette number out of range (%d>=%d)", 
	    pal, num_pals);
#endif
  
  // store the colors to the current display
  SDL_SetPalette(SDL_GetVideoSurface(),SDL_PHYSPAL,colours+256*pal, 0, 256);
}

//////////////////////////////////////////////////////////////////////////////
// Graphics API

void I_ShutdownGraphics(void)
{
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
}

//
// I_FinishUpdate
//
static int newpal = 0;
#define NO_PALETTE_CHANGE 1000

void I_FinishUpdate (void)
{
  if (I_SkipFrame()) return;

#ifdef MONITOR_VISIBILITY
  if (!(SDL_GetAppState()&SDL_APPACTIVE)) {
    return;
  }
#endif
  
#ifndef GL_DOOM
  if (screen->pixels != screens[0])
  {
    if (SDL_MUSTLOCK(screen))
    {
      int h;
      int w;
      char *src;
      char *dest;

      if (SDL_LockSurface(screen) >= 0)
      {
        //dest=(char *)(screen->pixels)+(screen->clip_rect.y*screen->pitch)+screen->clip_rect.x;
        dest=(char *)screen->pixels;
        src=screens[0];
        w=(screen->clip_rect.w>SCREENWIDTH)?(SCREENWIDTH):(screen->clip_rect.w);
        h=(screen->clip_rect.h>SCREENHEIGHT)?(SCREENHEIGHT):(screen->clip_rect.h);
        for (; h>0; h--)
        {
          memcpy(dest,src,w);
          dest+=screen->pitch;
          src+=SCREENWIDTH;
        }
        SDL_UnlockSurface(screen);
      }
    }
  }
  /* Update the display buffer (flipping video pages if supported)
   * If we need to change palette, that implicitely does a flip */
  if (newpal != NO_PALETTE_CHANGE) { 
    I_UploadNewPalette(newpal);
    newpal = NO_PALETTE_CHANGE;
  }
    SDL_Flip(screen);
#else
  // proff 04/05/2000: swap OpenGL buffers
  gld_Finish();
#endif
}

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
  memcpy(scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

//
// I_SetPalette
//
void I_SetPalette (int pal)
{
  newpal = pal;
}


void I_ShutdownSDL(void)
{
	SDL_Quit();
	return;
}

// I_PreInitGraphics

void I_PreInitGraphics(void)
{
  // Initialize SDL
  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
    I_Error("Could not initialize SDL [%s]", SDL_GetError());
  }
  
  atexit(I_ShutdownSDL);
  
  I_InitInputs();
}

// CPhipps -
// I_SetRes
// Sets the screen resolution, possibly using the supplied guide

void I_SetRes(unsigned int width, unsigned int height)
{
  SCREENWIDTH = (width+3) & ~3;
  SCREENHEIGHT = (height+3) & ~3;

  lprintf(LO_INFO,"I_SetRes: Using resolution %dx%d\n", SCREENWIDTH, SCREENHEIGHT);
}

void I_InitGraphics(void)
{
  char          titlebuffer[2048];
  static int		firsttime=1;
  
  if (firsttime)
  {  
    firsttime = 0;

    atexit(I_ShutdownGraphics);
    lprintf(LO_INFO, "I_InitGraphics: %dx%d\n", SCREENWIDTH, SCREENHEIGHT);

    out_buffer=screens[0];

    /* Set the video mode */
    I_UpdateVideoMode();

    /* Setup the window title */
    strcpy(titlebuffer,PACKAGE);
    strcat(titlebuffer," ");
    strcat(titlebuffer,VERSION);
    SDL_WM_SetCaption(titlebuffer, titlebuffer);
  }
}

void I_UpdateVideoMode(void)
{
  unsigned int w, h;
  int init_flags;

  lprintf(LO_INFO, "I_UpdateVideoMode: %dx%d (%s)\n", SCREENWIDTH, SCREENHEIGHT, use_fullscreen ? "fullscreen" : "nofullscreen");

  w = SCREENWIDTH;
  h = SCREENHEIGHT;
  
  // Initialize SDL with this graphics mode
#ifdef GL_DOOM
  init_flags = SDL_OPENGL;
#else
  if (use_doublebuffer)
    init_flags = SDL_DOUBLEBUF;
  else
    init_flags = SDL_SWSURFACE;
#ifndef _DEBUG
  init_flags |= SDL_HWPALETTE;
#endif
#endif
  if ( use_fullscreen )
    init_flags |= SDL_FULLSCREEN;

#ifdef GL_DOOM
  SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 0 );
  SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 0 );
  SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 0 );
  SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 0 );
  SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 0 );
  SDL_GL_SetAttribute( SDL_GL_ACCUM_RED_SIZE, 0 );
  SDL_GL_SetAttribute( SDL_GL_ACCUM_GREEN_SIZE, 0 );
  SDL_GL_SetAttribute( SDL_GL_ACCUM_BLUE_SIZE, 0 );
  SDL_GL_SetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, 0 );
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
  SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, gl_colorbuffer_bits );
  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, gl_depthbuffer_bits );
  screen = SDL_SetVideoMode(w, h, gl_colorbuffer_bits, init_flags);
#else
  screen = SDL_SetVideoMode(w, h, 8, init_flags);
#endif

  if (screen == NULL) {
    I_Error("Couldn't set %dx%d video mode [%s]", w, h, SDL_GetError());
  }

  mouse_currently_grabbed = false;

  // Get the info needed to render to the display
  if (!SDL_MUSTLOCK(screen))
  {
    if (out_buffer)
      free(out_buffer);
    out_buffer=NULL;
    screens[0] = (unsigned char *) (screen->pixels);
  }
  else
  {
    if (!out_buffer)
      free(out_buffer);
    out_buffer = calloc(SCREENWIDTH*SCREENHEIGHT, 1);
    screens[0] = out_buffer;
  }

  // Hide pointer while over this window
  SDL_ShowCursor(0);

  R_InitBuffer(w,h);

#ifdef GL_DOOM
  {
  int temp;
  lprintf(LO_INFO,"    SDL OpenGL PixelFormat:\n");
  SDL_GL_GetAttribute( SDL_GL_RED_SIZE, &temp );
  lprintf(LO_INFO,"    SDL_GL_RED_SIZE: %i\n",temp);
  SDL_GL_GetAttribute( SDL_GL_GREEN_SIZE, &temp );
  lprintf(LO_INFO,"    SDL_GL_GREEN_SIZE: %i\n",temp);
  SDL_GL_GetAttribute( SDL_GL_BLUE_SIZE, &temp );
  lprintf(LO_INFO,"    SDL_GL_BLUE_SIZE: %i\n",temp);
  SDL_GL_GetAttribute( SDL_GL_STENCIL_SIZE, &temp );
  lprintf(LO_INFO,"    SDL_GL_STENCIL_SIZE: %i\n",temp);
  SDL_GL_GetAttribute( SDL_GL_ACCUM_RED_SIZE, &temp );
  lprintf(LO_INFO,"    SDL_GL_ACCUM_RED_SIZE: %i\n",temp);
  SDL_GL_GetAttribute( SDL_GL_ACCUM_GREEN_SIZE, &temp );
  lprintf(LO_INFO,"    SDL_GL_ACCUM_GREEN_SIZE: %i\n",temp);
  SDL_GL_GetAttribute( SDL_GL_ACCUM_BLUE_SIZE, &temp );
  lprintf(LO_INFO,"    SDL_GL_ACCUM_BLUE_SIZE: %i\n",temp);
  SDL_GL_GetAttribute( SDL_GL_ACCUM_ALPHA_SIZE, &temp );
  lprintf(LO_INFO,"    SDL_GL_ACCUM_ALPHA_SIZE: %i\n",temp);
  SDL_GL_GetAttribute( SDL_GL_DOUBLEBUFFER, &temp );
  lprintf(LO_INFO,"    SDL_GL_DOUBLEBUFFER: %i\n",temp);
  SDL_GL_GetAttribute( SDL_GL_BUFFER_SIZE, &temp );
  lprintf(LO_INFO,"    SDL_GL_BUFFER_SIZE: %i\n",temp);
  SDL_GL_GetAttribute( SDL_GL_DEPTH_SIZE, &temp );
  lprintf(LO_INFO,"    SDL_GL_DEPTH_SIZE: %i\n",temp);
  gld_Init(SCREENWIDTH, SCREENHEIGHT);
  }
#endif
}
