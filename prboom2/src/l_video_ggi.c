/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: l_video_ggi.c,v 1.1 2000/05/04 08:08:05 proff_fs Exp $
 *
 *  LxDoom, a Doom port for Linux/Unix
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *   and Colin Phipps
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
 *  GGI target code for LxDoom
 *-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include <ggi/ggi.h>

#include "z_zone.h"
#include "i_video.h"
#include "i_system.h"

#include "m_argv.h"
#include "doomtype.h"
#include "doomstat.h"
#include "v_video.h"
#include "d_event.h"
#include "d_main.h"
#include "r_draw.h"

#include "i_joy.h"
#include "l_video_trans.h"

static boolean initialised;
static ggi_visual_t screen;
static const ggi_pixelformat* pixelformat;
static int multiply = 1;
static pval* out_buffer;

#ifdef HIGHRES
#ifdef I386
void (*R_DrawColumn)(void);
void (*R_DrawTLColumn)(void);
#endif
#endif
static int ggi_screenwidth, ggi_screenheight;

// Mask of events that we are interested in
static const ggi_event_mask ev_mask = \
emKeyPress | emKeyRelease | emPtrRelative | emPtrButtonPress | \
emPtrButtonRelease;

// Unused vars to preserve in config file
int leds_always_off;
int use_vsync;

////////////////////////////////////////////////////////////////
// Input handling utility functions

// Null keyboard translation to satisfy m_misc.c
int I_DoomCode2ScanCode(int c)
{
  return c;
}

int I_ScanCode2DoomCode(int c)
{
  return c;
}

//
// I_GIITranslateKey
//
// Translate keys from LibGII
// Adapted from Linux Heretic v0.9.2

int I_GIITranslateKey(const ggi_key_event* key)
{
  int label = key->label, sym = key->sym;
  int rc=0;
  switch(label) {
  case GIIK_CtrlL:  case GIIK_CtrlR:  rc=KEYD_RCTRL; 	break;
  case GIIK_ShiftL: case GIIK_ShiftR: rc=KEYD_RSHIFT;	break;
  case GIIK_MetaL:  case GIIK_MetaR:
  case GIIK_AltL:   case GIIK_AltR:   rc=KEYD_RALT;	break;
  case GIIUC_BackSpace:   rc = KEYD_BACKSPACE;     break; 
  case GIIUC_Escape:      rc = KEYD_ESCAPE;        break; 
  case GIIK_Delete:       rc = KEYD_DEL;        break; 
  case GIIK_Insert:       rc = KEYD_INSERT;        break; 
  case GIIK_PageUp:       rc = KEYD_PAGEUP;        break; 
  case GIIK_PageDown:     rc = KEYD_PAGEDOWN;      break; 
  case GIIK_Home:         rc = KEYD_HOME;          break; 
  case GIIK_End:  rc = KEYD_END;           break; 
  case GIIUC_Tab:	              rc = KEYD_TAB;	break;
  case GIIK_Up:	                      rc = KEYD_UPARROW;	break;
  case GIIK_Down:	              rc = KEYD_DOWNARROW;	break;
  case GIIK_Left:	              rc = KEYD_LEFTARROW;	break;
  case GIIK_Right:                    rc = KEYD_RIGHTARROW;	break;
  case GIIK_Enter:                    rc = KEYD_ENTER;		break;
  case GIIK_F1:	rc = KEYD_F1;		break;
  case GIIK_F2:	rc = KEYD_F2;		break;
  case GIIK_F3:	rc = KEYD_F3;		break;
  case GIIK_F4:	rc = KEYD_F4;		break;
  case GIIK_F5:	rc = KEYD_F5;		break;
  case GIIK_F6:	rc = KEYD_F6;		break;
  case GIIK_F7:	rc = KEYD_F7;		break;
  case GIIK_F8:	rc = KEYD_F8;		break;
  case GIIK_F9:	rc = KEYD_F9;		break;
  case GIIK_F10:	rc = KEYD_F10;		break;
  case GIIK_F11:	rc = KEYD_F11;		break;
	case GIIK_F12:	rc = KEYD_F12;		break;
	case GIIK_Pause:rc = KEYD_PAUSE;		break;

	default:
	  if ((label > '0' && label < '9') || 
	      label == '.' || 
	      label == ',') { 
	      /* Must use label here, or it won't work when shift 
	         is down */ 
	    rc = label; 
	  } else if (sym < 256) { 		    
	      /* ASCII key - we want those */
	    rc = sym;
	      /* We want lowercase */
	    if (rc >='A' && rc <='Z') rc -= ('A' - 'a');
	    switch (sym) {
	      /* Some special cases */
	    case '+': rc = KEYD_EQUALS;	break;
	    case '-': rc = KEYD_MINUS; 
	    default:			break;
	    }
	  }
  }
  return rc;
}

unsigned short I_GIITranslateButtons(const ggi_pbutton_event* ev)
{
  return ev->button;
}

////////////////////////////////////////////////////////////////
// API

//
// I_StartFrame
//
void I_StartFrame (void)
{
  // If we reuse the blitting buffer in the next rendering, 
  // make sure it is reusable now
  if (!expand_buffer)
    ggiFlush(screen);
}

void I_StartTic(void)
{
  struct timeval nowait = { 0, 0 };

  if (!initialised) return;

  while (ggiEventPoll(screen, ev_mask, &nowait)!=evNothing) {
    // There is a desirable event
    ggi_event ggi_ev;
    event_t doom_ev;
    int m_x = 0, m_y = 0; // Mouse motion
    static unsigned short buttons; // Buttons remembered
    boolean mouse_event = false; // Do we need a mouse event?

    ggiEventRead(screen, &ggi_ev, ev_mask);

    switch(ggi_ev.any.type) {
    case evKeyPress:
      doom_ev.type = ev_keydown;
      if ((doom_ev.data1= I_GIITranslateKey(&ggi_ev.key)) >0 )
	D_PostEvent(&doom_ev);
      break;
    case evKeyRelease:
      doom_ev.type = ev_keyup;
      if ((doom_ev.data1 = I_GIITranslateKey(&ggi_ev.key) > 0))
	D_PostEvent(&doom_ev);
      break;
    case evPtrRelative:
      m_x += ggi_ev.pmove.x;
      m_y += ggi_ev.pmove.y;
      mouse_event = true;
      break;
    case evPtrButtonPress:
      buttons |= I_GIITranslateButtons(&ggi_ev.pbutton);
      break;
    case evPtrButtonRelease:
      buttons &= ~I_GIITranslateButtons(&ggi_ev.pbutton);
      break;
    default:
      I_Error("I_StartTic: Bad GGI event type");
    }

    if (mouse_event) {
      doom_ev.type = ev_mouse;
      doom_ev.data1 = buttons;
      doom_ev.data2 = m_x;
      doom_ev.data3 = m_y;
      D_PostEvent(&doom_ev);
    }
  }
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
  // Finish up any output
  ggiFlush(screen);
}

//
// I_FinishUpdate
//
void I_FinishUpdate (void)
{
  
  int		i;
  
  // Protect against calls before I_InitGraphics
  if (!initialised) return;

  // draws little dots on the bottom of the screen
  if (devparm) {
    static int	lasttic;
    int		tics;
    
    i = I_GetTime();
    tics = i - lasttic;
    lasttic = i;
    if (tics > 20) tics = 20;
    
    for (i=0 ; i<tics*2 ; i+=2)
      screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0xff;
    for ( ; i<20*2 ; i+=2)
      screens[0][ (SCREENHEIGHT-1)*SCREENWIDTH + i] = 0x0;
  }
  
  // scales the screen size before blitting it
  if (expand_buffer)
    (*I_ExpandImage)(out_buffer, screens[0]);

  // Blit it
  ggiPutBox(screen, 0, 0, ggi_screenwidth, 
	    ggi_screenheight, out_buffer);
}

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
  memcpy(scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

void I_SetPalette(const byte* palette)
{
  if (!initialised) return;

  if (!true_color) {
    ggi_color ggi_pal[256];
    ggi_color* p;
    int i;

    // PseudoColor mode, so set the palette

    for (i=0, p=ggi_pal; i<256; i++) {
      // Translate palette entry to 16 bits per component
      p->r = palette[0] << 8;
      p->g = palette[1] << 8;
      p->b = palette[2] << 8;
      
      p++; palette+=3;
    }

    ggiSetPalette(screen, 0, 256, ggi_pal);
  } else {
    // TrueColor mode, so rewrite conversion table

    I_SetPaletteTranslation(palette);
  }
}

void I_ShutdownGraphics(void)
{
  if (!initialised) return;

  ggiRemoveFlags(screen, GGIFLAG_ASYNC);

  if (expand_buffer) 
    free(out_buffer);

  ggiExit(); initialised = false;
}

void I_PreInitGraphics(void)
{
}

// CPhipps -
// I_SetRes
// Sets the screen resolution, possibly using the supplied guide

void I_SetRes(unsigned int width, unsigned int height)
{
#ifdef HIGHRES
  SCREENWIDTH = 
#endif
    ggi_screenwidth = width;

#ifdef HIGHRES
  SCREENHEIGHT = 
#endif
    ggi_screenheight = height;

#ifdef I386
  if (SCREENWIDTH == 320) {
    R_DrawColumn = R_DrawColumn_Normal;
    R_DrawTLColumn = R_DrawTLColumn_Normal;
  } else {
    R_DrawColumn = R_DrawColumn_HighRes;
    R_DrawTLColumn = R_DrawTLColumn_HighRes;
  }
#endif
}

void I_InitGraphics(void)
{
  ggi_mode mode;

  if (initialised) return;
  
  fprintf(stderr, "I_InitGraphics:");

  if (ggiInit())
    I_Error("Failed to initialise GGI\n");

  if (!(screen = ggiOpen(NULL))) // Open default visual
    I_Error("Failed to get default visual\n");
  
  initialised = true; atexit(I_ShutdownGraphics);

  { // Check for screen enlargement
    char str[3] = { '-', 0, 0 };
    int n;

    for (n=1; n<4; n++) {
      str[1] = n + '0';
      if (M_CheckParm(str)) multiply = n;
    }
  }

  // Video mode negotiation

  mode.frames = 2; // We'd like page flipping
  mode.visible.x = mode.virt.x = ggi_screenwidth = SCREENWIDTH * multiply;
  mode.visible.y = mode.virt.y = ggi_screenheight = SCREENHEIGHT * multiply;
  mode.size.x    = mode.size.y = GGI_AUTO;
  mode.dpp.x     = mode.dpp.y  = 1; // Graphics mode
  mode.graphtype = GT_8BIT; // 8 bit PseudoColor please

  ggiCheckMode(screen, &mode); // Try it

  if ((mode.visible.x < SCREENWIDTH) || 
      (mode.visible.y < SCREENHEIGHT)) 
    I_Error("Insufficient screen size available");

  if (mode.graphtype & (GT_TEXT | GT_GREYSCALE)) 
    I_Error("Unsupported video mode suggested\n");

  if ((dest_bpp = GT_DEPTH(mode.graphtype)) < 8)
    I_Error("Need 8 bpp depth minimum (%d found)", 
	    GT_DEPTH(mode.graphtype));

  // Clamp multiply value to window size

  if ((ggi_screenwidth > mode.visible.x) || 
      (ggi_screenheight > mode.visible.y)) {
    multiply = mode.visible.x / SCREENWIDTH;
    if (multiply * SCREENHEIGHT > mode.visible.y)
      multiply = mode.visible.y / SCREENHEIGHT;

    fprintf(stderr, "multiply level reduced to %d", multiply);
    ggi_screenwidth = SCREENWIDTH * multiply;
    ggi_screenheight = SCREENHEIGHT * multiply;  
  }

  // We have a large enough, deep enough, colour mode, so
  // now check that we can convert out image to that format

  if (!I_QueryImageTranslation())
    I_Error("Unable to use convert to mode %ux%u %d bpp", 
	    mode.visible.x, mode.visible.y, dest_bpp);

  // Sanity check

  if ((true_color && !(mode.graphtype & GT_TRUECOLOR))
      || (!true_color && (mode.graphtype & GT_TRUECOLOR)))
    I_Error("Disagreed with libggi over TrueColor");

  // Go for it

  fprintf(stderr, " using mode %ux%u %d bpp, scale x%d, %s\n", 
	  mode.visible.x, mode.visible.y, 
	  GT_DEPTH(mode.graphtype), multiply, 
	  mode.graphtype & GT_TRUECOLOR ? 
	  "TrueColor" : "PseudoColor");

  if (ggiSetMode(screen, &mode))
    I_Error("Failed to set mode");

  pixelformat = ggiGetPixelFormat(screen);

  if (true_color) {
    // Calculate TrueColor shifts

    I_SetColourShift(pixelformat->red_mask, &redshift);
    I_SetColourShift(pixelformat->green_mask, &greenshift);
    I_SetColourShift(pixelformat->blue_mask, &blueshift);
  }

  // Allocate enlarement buffer if needed
  if (expand_buffer)
    out_buffer = malloc(ggi_screenheight*ggi_screenwidth*BYTESPP);
  else
    out_buffer = (pval*)screens[0];

  // Go asynchronous
  ggiAddFlags(screen, GGIFLAG_ASYNC);

  // Mask events
  ggiSetEventMask(screen, ev_mask);

  I_InitJoystick();
}

/*
 * $Log: l_video_ggi.c,v $
 * Revision 1.1  2000/05/04 08:08:05  proff_fs
 * Initial revision
 *
 * Revision 1.2  1999/10/12 13:01:11  cphipps
 * Changed header to GPL
 *
 * Revision 1.1  1999/01/26 09:27:08  cphipps
 * Initial revision
 *
 */
