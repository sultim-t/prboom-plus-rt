/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_video.c,v 1.4 2000/05/11 20:03:08 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Colin Phipps (cph@lxdoom.linuxgames.com), 
 *  Jess Haas (JessH@lbjhs.net)
 *  and Florian Schulze (florian.proff.schulze@gmx.net)
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
rcsid[] = "$Id: i_video.c,v 1.4 2000/05/11 20:03:08 proff_fs Exp $";

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "SDL.h"

#ifdef I386
void (*R_DrawColumn)(void);
void (*R_DrawTLColumn)(void);
#endif

#include "i_system.h"
#include "m_argv.h"
#include "doomstat.h"
#include "doomdef.h"
#include "doomtype.h"
#include "v_video.h"
#include "r_draw.h"
#include "d_main.h"
#include "d_event.h"
#include "v_video_trans.h"
#include "i_joy.h"
#include "i_video.h"
#include "z_zone.h"
#include "s_sound.h"
#include "sounds.h"
#include "w_wad.h"
#include "lprintf.h"
#ifdef GL_DOOM
#include "gl_struct.h"
#endif

extern void M_QuitDOOM(int choice);

int use_vsync = 0; // Included not to break m_misc, but not relevant to SDL
static SDL_Surface *screen;

// This is the pointer to the buffer to blit
pval     *      out_buffer = NULL;

// Common const strings
static const char lcase_lxdoom[] = { "lsdldoom" };
static const char ucase_lxdoom[] = { "LSDLDOOM" };

////////////////////////////////////////////////////////////////////////////
// Input code 
int             leds_always_off = 0; // Expected by m_misc, not relevant 

// Mouse handling
extern int     usemouse;        // config file var
static boolean grabMouse;       // internal var

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
  case SDLK_BACKSPACE:
  case SDLK_DELETE:	rc = KEYD_BACKSPACE;	break;
  case SDLK_PAUSE:	rc = KEYD_PAUSE;	break;
  case SDLK_EQUALS:	rc = KEYD_EQUALS;	break;
  case SDLK_MINUS:	rc = KEYD_MINUS;	break;
  case SDLK_KP0:	rc = KEYD_KEYPAD0;	break;
  case SDLK_KP1:	rc = KEYD_KEYPAD1;	break;
  case SDLK_KP2:	rc = KEYD_KEYPAD2;	break;
  case SDLK_KP3:	rc = KEYD_KEYPAD3;	break;
  case SDLK_KP4:	rc = KEYD_KEYPAD4;	break;
  case SDLK_KP5:	rc = KEYD_KEYPAD0;	break;
  case SDLK_KP6:	rc = KEYD_KEYPAD6;	break;
  case SDLK_KP7:	rc = KEYD_KEYPAD7;	break;
  case SDLK_KP8:	rc = KEYD_KEYPAD8;	break;
  case SDLK_KP9:	rc = KEYD_KEYPAD9;	break;
  case SDLK_KP_PLUS:	rc = KEYD_KEYPADPLUS;	break;
  case SDLK_KP_MINUS:	rc = KEYD_KEYPADMINUS;	break;
  case SDLK_KP_DIVIDE:	rc = KEYD_KEYPADDIVIDE;	break;
  case SDLK_KP_MULTIPLY: rc = KEYD_KEYPADMULTIPLY; break;
  case SDLK_KP_ENTER:	rc = KEYD_KEYPADENTER;	break;
  case SDLK_LSHIFT:
  case SDLK_RSHIFT:	rc = KEYD_RSHIFT;	break;
  case SDLK_LCTRL:
  case SDLK_RCTRL:	rc = KEYD_RCTRL;	break;
  case SDLK_LALT:
  case SDLK_LMETA:
  case SDLK_RALT:
  case SDLK_RMETA:	rc = KEYD_RALT;		break;
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
  {
    event.type = ev_keyup;
    event.data1 = I_TranslateKey(&Event->key.keysym);
    D_PostEvent(&event);
  }
  break;

  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP:
  if (usemouse)
  { Uint8 buttonstate;
    buttonstate = SDL_GetMouseState(NULL, NULL);
    event.type = ev_mouse;
    event.data1 = 0
      | (buttonstate & SDL_BUTTON(1) ? 1 : 0)
      | (buttonstate & SDL_BUTTON(2) ? 2 : 0)
      | (buttonstate & SDL_BUTTON(3) ? 4 : 0);
    event.data2 = event.data3 = 0;
    D_PostEvent(&event);
  }
  break;

  case SDL_MOUSEMOTION:
  /* Ignore mouse warp events */
  if (usemouse &&
      ((Event->motion.x != screen->w/2)||(Event->motion.y != screen->h/2)))
  {
    /* Warp the mouse back to the center */
    if (grabMouse) {
      if ( (Event->motion.x < ((screen->w/2)-(screen->w/4))) ||
           (Event->motion.x > ((screen->w/2)+(screen->w/4))) ||
           (Event->motion.y < ((screen->h/2)-(screen->h/4))) ||
           (Event->motion.y > ((screen->h/2)+(screen->h/4))) )
        SDL_WarpMouse((Uint16)(screen->w/2), (Uint16)(screen->h/2));
    }
    event.type = ev_mouse;
    event.data1 = 0
      | (Event->motion.state & SDL_BUTTON(1) ? 1 : 0)
      | (Event->motion.state & SDL_BUTTON(2) ? 2 : 0)
      | (Event->motion.state & SDL_BUTTON(3) ? 4 : 0);
    event.data2 = Event->motion.xrel << 2;
    event.data3 = -Event->motion.yrel << 2;
    D_PostEvent(&event);
  }
  break;

  case SDL_QUIT:
    S_StartSound(NULL, sfx_swtchn);
    M_QuitDOOM(0);

  default:
    break;
  }
}


//
// I_StartTic
//
void I_StartTic (void)
{
  SDL_Event Event;

  while ( SDL_PollEvent(&Event) )
    I_GetEvent(&Event);
  
  I_PollJoystick();
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
  // check if the user wants to grab the mouse (quite unnice)
  grabMouse = M_CheckParm("-nomouse") ? false : 
    usemouse ? true : false;

  I_InitJoystick();
}
/////////////////////////////////////////////////////////////////////////////

// I_SkipFrame
//
// Returns true if it thinks we can afford to skip this frame

// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static boolean I_SkipFrame(void)
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
    int            lump = W_GetNumForName("PLAYPAL");
    const byte *palette = W_CacheLumpNum(lump);
    register const byte *const gtable = gammatable[cachedgamma = usegamma];
    register int i;

    num_pals = W_LumpLength(lump) / (3*256);
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
  
    W_UnlockLumpNum(lump);
    num_pals/=256;
  }

#ifdef RANGECHECK
  if (pal >= num_pals) 
    I_Error("I_UploadNewPalette: Palette number out of range (%d>=%d)", 
	    pal, num_pals);
#endif
  
  // store the colors to the current display
  SDL_SetColors(SDL_GetVideoSurface(), colours+256*pal, 0, 256);
}

//////////////////////////////////////////////////////////////////////////////
// Graphics API

void I_ShutdownGraphics(void)
{
  fprintf(stderr, "I_ShutdownGraphics : ");

  // Free internal structures
  if (pixelvals != NULL) free(pixelvals);
  I_EndImageTranslation();
  fprintf(stderr, "\n");
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
void I_FinishUpdate (void)
{
  if (I_SkipFrame()) return;

#ifdef MONITOR_VISIBILITY
  if (!(SDL_GetAppState()&SDL_APPACTIVE)) {
    return;
  }
#endif
  
#ifndef GL_DOOM
  // scales the screen size before blitting it
  if (expand_buffer)
    (*I_ExpandImage)(out_buffer, screens[0]);
  
  // Update the display buffer (flipping video pages if supported)
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
  if (true_color) {
    int            lump = W_GetNumForName("PLAYPAL");
    const byte *palette = W_CacheLumpNum(lump);
    I_SetPaletteTranslation(palette + (3*256)*pal);
    W_UnlockLumpNum(lump);
  } else
    I_UploadNewPalette(pal);
}

// I_PreInitGraphics

void I_ShutdownSDL(void)
{
	SDL_Quit();
	return;
}

void I_PreInitGraphics(void)
{
  // Initialize SDL
  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
    I_Error("Could not initialize SDL [%s]", SDL_GetError());
  }
  
  atexit(I_ShutdownSDL);
}

// CPhipps -
// I_SetRes
// Sets the screen resolution, possibly using the supplied guide

void I_SetRes(unsigned int width, unsigned int height)
{
#ifdef HIGHRES
  SCREENWIDTH = 
#endif
    (width+3) & ~3;

#ifdef HIGHRES
  SCREENHEIGHT = 
#endif
    (height+3) & ~3;

#ifndef GL_DOOM
  if (SCREENWIDTH == 320) {
    R_DrawColumn = R_DrawColumn_Normal;
    R_DrawTLColumn = R_DrawTLColumn_Normal;
  } else {
    R_DrawColumn = R_DrawColumn_HighRes;
    R_DrawTLColumn = R_DrawTLColumn_HighRes;
  }
#endif
  lprintf(LO_INFO,"I_SetRes: Using resolution %dx%d\n", SCREENWIDTH, SCREENHEIGHT);
}

void I_InitGraphics(void)
{
  int           w, h;
  int		n;
  Uint32        init_flags;
  
  {  
    static int		firsttime=1;
    
    if (!firsttime) {
      return;
    }
    firsttime = 0;
  }

  { // Check for screen enlargement
    char str[3] = { '-', 0, 0 };

    for (n=1; n<4; n++) {
      str[1] = n + '0';
      if (M_CheckParm(str)) multiply = n;
    }
  }
  
  w = SCREENWIDTH * multiply;
  h = SCREENHEIGHT * multiply;
  
  // Initialize SDL with this graphics mode
#ifdef GL_DOOM
  init_flags = SDL_OPENGL;
#else
  init_flags = SDL_SWSURFACE|SDL_HWPALETTE;
#endif
  if ( M_CheckParm("-fullscreen") ) {
    init_flags |= SDL_FULLSCREEN;
  }
#ifdef GL_DOOM
  SDL_GL_SetAttribute( SDL_GL_BUFFER_SIZE, 16 );
  SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
  screen = SDL_SetVideoMode(w, h, 16, init_flags);
#else
  if(SDL_VideoModeOK(w, h, 8, init_flags) == 8) { 
    screen = SDL_SetVideoMode(w, h, 8, init_flags);
  } else {
    screen = SDL_SetVideoMode(w, h, 0, init_flags);
  }
#endif
  if(screen == NULL || !I_QueryImageTranslation()) {
    I_Error("Couldn't set %dx%d video mode [%s]", w, h, SDL_GetError());
  }
  dest_bpp = screen->format->BitsPerPixel;
  SDL_WM_SetCaption(lcase_lxdoom, ucase_lxdoom);

  I_InitImageTranslation();
  if (true_color) {
    // Set up colour shifts
    I_SetColourShift(screen->format->Rmask, &redshift);
    I_SetColourShift(screen->format->Gmask, &greenshift);
    I_SetColourShift(screen->format->Bmask, &blueshift);
  }

  lprintf(LO_INFO,"I_InitGraphics:");
  lprintf(LO_INFO, " SDL %d bpp %s, scale x%d\n", dest_bpp,
	  screen->format->palette ? "PseudoColor" : "TrueColor", multiply);

  // Get the info needed to render to the display
  out_buffer = (pval *)screen->pixels;
  if (!expand_buffer) {
    // Render directly into SDL display memory
    Z_Free(screens[0]);
    screens[0] = (unsigned char *) (screen->pixels); 
  }

  atexit(I_ShutdownGraphics);

  // Initialize the input system
  I_InitInputs();

#ifdef GL_DOOM
  gld_Init(SCREENWIDTH, SCREENHEIGHT);
#endif
  // Hide pointer while over this window
  SDL_ShowCursor(0);
}

