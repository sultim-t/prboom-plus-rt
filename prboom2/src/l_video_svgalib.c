/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: l_video_svgalib.c,v 1.1 2000/05/04 08:08:16 proff_fs Exp $
 *
 *  SVGALib display code for LxDoom
 *  Copyright (C) 1999 by Colin Phipps, Gady Kozma
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
 *  Linux SVGALib screen output & input handling
 *-----------------------------------------------------------------------------*/

#ifndef lint
static const char 
rcsid[] = "$Id: l_video_svgalib.c,v 1.1 2000/05/04 08:08:16 proff_fs Exp $";
#endif /* lint */

//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vga.h>
#include <vgamouse.h>
#include <vgakeyboard.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif
#include "z_zone.h"
#include "i_video.h"
#include "i_system.h"
#include "doomtype.h"
#include "doomdef.h"
#include "doomstat.h"
#include "v_video.h"
#include "r_draw.h"
#include "d_event.h"
#include "d_main.h"
#include "m_argv.h"
#include "i_joy.h"
#include "i_main.h"
#include "w_wad.h"
#include "lprintf.h"

static enum { flip, flipped, blit } redraw_state;
static void (*blitfunc)(void* src, int dest, int w, int h, int pitch);
static enum { F_nomouse, F_mouse } mflag = F_nomouse;
int use_vsync; // Hmm...

static boolean initialised = false;

#ifdef I386
void (*R_DrawColumn)(void);
void (*R_DrawTLColumn)(void);
#endif

static event_t ev;

void I_StartTic(void)
{
  // Protect against calls too early
  if (!initialised) return;
  
  if (mflag != F_nomouse) 
    mouse_update();

  keyboard_update();

  I_PollJoystick();
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
}

static void I_PlainBlit(void* src, int dest, int w, int h, int pitch)
{
  vga_lockvc();
  {
    if (redraw_state != blit) // We probably chose blit to avoid paging
      vga_setpage(dest >> 16); // I.e each page is 64K
    if (pitch == SCREENWIDTH) {
      //unsigned char *vga;
      int loop;
      
      // You wouldn't believe the amount of tries of raw-writing in mode-X I 
      // did before I got to this trivial solution.
      // Of course an ideal solution would be to rewrite R_DrawColumn to 
      // work in mode-X... but let's leave something for somebody else, won't
      // we?
      
      for ( loop = 0 ; loop < h ; loop++ )
	vga_drawscanline(loop, ((unsigned char *)src) + loop * w);
    }
    else {
      // IS THIS EVER CALLED?????
      register byte* destptr = vga_getgraphmem();
      do {
        memcpy(destptr, src, w);
        src = (void*)((int)src + w);
        dest += pitch;
      } while (--h);
    }
  }
  vga_unlockvc();
}

void I_FinishUpdate(void)
{
  // CPhipps - protect against Boom sillyness
  if (!initialised) return;

  while (!vga_oktowrite()) {
    // While waiting keep scanning keyboard for possible abort sequence
    keyboard_update();

    usleep(10); // Give Linux a break
  }

  switch (redraw_state) {
  case blit:
    if (use_vsync) 
      vga_waitretrace();
    (*blitfunc)(screens[0], 0, SCREENWIDTH, SCREENHEIGHT, SCREENWIDTH);
    break;
  case flipped:
    (*blitfunc)(screens[0], 0, SCREENWIDTH, SCREENHEIGHT, SCREENWIDTH);
    if (use_vsync) 
      vga_waitretrace();
    vga_setdisplaystart(0);
    redraw_state = flip;
    break;
  case flip:
    (*blitfunc)(screens[0], 1<<16, 
		 SCREENWIDTH, SCREENHEIGHT, SCREENWIDTH);
    if (use_vsync) 
      vga_waitretrace();
    vga_setdisplaystart(1<<16);
    redraw_state = flipped;
    break;
  }
}

//
// I_UpdateNoBlit
//
void I_UpdateNoBlit (void)
{
  // what is this?
}

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
  memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

//
// I_SetPalette
//
void I_SetPalette (int pal)
{
  static int   *palette;
  static int    cachedgamma;
  static size_t num_pals;

  if (!palette || (cachedgamma != usegamma)) {
    int      lump = W_GetNumForName("PLAYPAL");
    const byte *playpal = W_CacheLumpNum(lump);
    register int *p;
    register int  count;
    register const unsigned char *const gtable = 
      gammatable[cachedgamma = usegamma];

    num_pals = W_LumpLength(lump) / (3*256);

    if (!palette)
      palette = Z_Malloc(256 * 3 * sizeof(int) * num_pals, PU_STATIC, NULL);

    p =  palette; count = num_pals * 256;

    do {
      // One RGB triple
      p[0] = gtable[(int)playpal[0]] >> 2;
      p[1] = gtable[(int)playpal[1]] >> 2;
      p[2] = gtable[(int)playpal[2]] >> 2;
      p+=3; playpal+=3;
    } while (--count);

    W_UnlockLumpNum(lump);
  }

  if (initialised && vga_oktowrite())
    vga_setpalvec(0, 256, palette + (256*3)*pal);
}

int I_ScanCode2DoomCode(int c)
{
  return c;
}

int I_DoomCode2ScanCode(int c)
{
  return c;
}

static unsigned short scancode2doomcode[256];

// Proff - added __inline for VisualC
#ifdef _MSC_VER
__inline
#else
inline
#endif
static void I_InitKBTransTable(void)
{
  { // First do rows of standard keys
#define KEYROWS 5
    const unsigned char keyrow[KEYROWS][20]= { { "1234567890" }, { "qwertyuiop" }, 
				      { "asdfghjkl;'" }, { "\\zxcvbnm,./`" }, 
   { KEYD_F1, KEYD_F2, KEYD_F3, KEYD_F4, KEYD_F5, 
   KEYD_F6, KEYD_F7, KEYD_F8, KEYD_F9, KEYD_F10, 0} };
    const int scancode4keyrow[KEYROWS] = { SCANCODE_1, SCANCODE_Q,
					   SCANCODE_A, SCANCODE_BACKSLASH,
					   SCANCODE_F1 };
    int i = KEYROWS;
    while (i--) {
      int j = strlen(keyrow[i]);
      while (j--) {
	scancode2doomcode[(scancode4keyrow[i] + j) & 0xff] = 
	  keyrow[i][j];
      }
    }
#undef KEYROWS
  }
  // Now the rest
  {
#define SETKEY(A,B) scancode2doomcode[SCANCODE_ ## A] = KEYD_ ## B
    SETKEY(BREAK_ALTERNATIVE,PAUSE); // BREAK_ALTERNATIVE is the pause key
    // (note that BREAK is printscreen) - rain    
    SETKEY(CURSORRIGHT,RIGHTARROW);
    SETKEY(CURSORUP,UPARROW);
    SETKEY(CURSORDOWN,DOWNARROW);
    SETKEY(CURSORLEFT,LEFTARROW);
    SETKEY(CURSORBLOCKRIGHT,RIGHTARROW);
    SETKEY(CURSORBLOCKUP,UPARROW);
    SETKEY(CURSORBLOCKDOWN,DOWNARROW);
    SETKEY(CURSORBLOCKLEFT,LEFTARROW);
    SETKEY(EQUAL,EQUALS);
    SETKEY(SPACE,SPACEBAR);
    SETKEY(LEFTALT,LALT);
#ifdef LCTRL
    SETKEY(LEFTCONTROL,LCTRL);
#else
    SETKEY(LEFTCONTROL,RCTRL);
#endif
#ifdef LSHIFT
    SETKEY(LEFTSHIFT,LSHIFT);
#else
    SETKEY(LEFTSHIFT,RSHIFT);
#endif
    SETKEY(RIGHTALT,RALT);
    SETKEY(RIGHTCONTROL,RCTRL);
    SETKEY(RIGHTSHIFT,RSHIFT);
    SETKEY(KEYPADENTER,ENTER);
#undef SETKEY
#define SETKEY(A) scancode2doomcode[SCANCODE_ ## A] = KEYD_ ## A
    SETKEY(ESCAPE);
    SETKEY(TAB);
    SETKEY(BACKSPACE);
    SETKEY(MINUS);
    SETKEY(INSERT);
    SETKEY(HOME);
    SETKEY(END);
    SETKEY(PAGEUP);
    SETKEY(PAGEDOWN);
    SETKEY(BACKSPACE);
    SETKEY(F11);
    SETKEY(F12);
    SETKEY(ENTER);
    SETKEY(CAPSLOCK);
    SETKEY(NUMLOCK);
    SETKEY(SCROLLLOCK);
    SETKEY(KEYPAD0);
    SETKEY(KEYPAD1);
    SETKEY(KEYPAD2);
    SETKEY(KEYPAD3);
    SETKEY(KEYPAD4);
    SETKEY(KEYPAD5);
    SETKEY(KEYPAD6);
    SETKEY(KEYPAD7);
    SETKEY(KEYPAD8);
    SETKEY(KEYPAD9);
    SETKEY(KEYPADPERIOD);
    SETKEY(KEYPADMULTIPLY);
    SETKEY(KEYPADDIVIDE);
    SETKEY(KEYPADPLUS);
    SETKEY(KEYPADMINUS);
  #undef SETKEY
  }
}

static void I_KBHandler(int scancode, int press)
{
  ev.type = (press == KEY_EVENTPRESS) ? ev_keydown : ev_keyup;
  if (scancode < 256) {
    ev.data1= scancode2doomcode[scancode];
    
    D_PostEvent(&ev);
  }
}

static void I_MouseEventHandler(int button, int dx, int dy, 
				int dz, int rdx, int rdy, int rdz)
{
  ev.type = ev_mouse;
  ev.data1= ((button & MOUSE_LEFTBUTTON) ? 1 : 0)
    | ((button & MOUSE_MIDDLEBUTTON) ? 2 : 0)
    | ((button & MOUSE_RIGHTBUTTON)  ? 4 : 0);
  ev.data2 = dx << 2; ev.data3 = -(dy << 2);

  D_PostEvent(&ev);
}

static int mode = G320x200x256;

#ifdef HIGHRES
static void I_FindClosestResolution(unsigned width, unsigned height)
{
  int                 loop, vlmn, area, ideal_area = width * height,
    closest_area = 0;
  vga_modeinfo        *mi;
  
  if (stored_euid != -1) seteuid(stored_euid);
  // I find the resolution with the area closest to width * height.
  vlmn = vga_lastmodenumber();
  for ( loop = 1; loop <= vlmn; loop++ ) {
    if ( vga_hasmode(loop) == 0 ) // not supported
      continue;
    
    mi = vga_getmodeinfo(loop);
    if ( mi -> bytesperpixel != 1 &&
	 mi -> bytesperpixel != 0 ) // planar modes - seem to work ok now.
      continue;
    
    if ( mi -> colors != 256 )
      continue;
    
    
    area = mi -> width * mi -> height;
    
    if ( closest_area == 0 ||
	 D_abs(closest_area - ideal_area) > D_abs(area - ideal_area) ||
	 (D_abs(closest_area - ideal_area) == D_abs(area - ideal_area) &&
	  D_abs(SCREENWIDTH - width) > D_abs(mi -> width - width)) ) {
      closest_area = area;
      SCREENWIDTH = mi -> width;
      SCREENHEIGHT = mi -> height;
      mode = loop;
    }
  }
  if ( SCREENWIDTH != width || SCREENHEIGHT != height )
    fprintf(stderr, "Closest resolution found: %dx%d\n",
	    SCREENWIDTH, SCREENHEIGHT);
  if (stored_euid != -1) seteuid(getuid());
}
#endif

void I_ShutdownGraphics(void)
{
  if (mflag != F_nomouse)
    vga_setmousesupport(0);

  keyboard_close();
  vga_setmode(TEXT);

  system("stty sane");
}

void I_PreInitGraphics(void)
{
  // Initialise hi-res vars
#ifdef HIGHRES
  SCREENWIDTH = 320; SCREENHEIGHT = 200;
#ifdef I386
  R_DrawColumn = R_DrawColumn_Normal;
  R_DrawTLColumn = R_DrawTLColumn_Normal;
#endif
#endif

  if (!M_CheckParm("-safety"))
    return;
  
  if (stored_euid != -1) seteuid(stored_euid);
  vga_safety_fork(I_ShutdownGraphics);
  vga_init();
  if (stored_euid != -1) seteuid(getuid());    
  initialised = true;
}

void I_SetRes(unsigned int width, unsigned int height)
{
#ifdef HIGHRES
  I_FindClosestResolution(width, height);
#endif

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
  const vga_modeinfo * pinfo;

  I_InitKBTransTable();
  fprintf(stderr, "I_InitGraphics: ");

  if (!initialised) { 
    if (stored_euid != -1) seteuid(stored_euid);
    vga_init();
    if (stored_euid != -1) seteuid(getuid());    
    initialised = true;
  }
 
  atexit(I_ShutdownGraphics); // svgalib does signal handlers itself
  
  // Start RAW keyboard handling
  keyboard_init();
  
  // cph - By request, this is added, but since I worry about the lack of an escape 
  //  sequence, you have to explicitely ask for it
  // Disables CTRL-C checking, because CTRL is the standard fire
  // key and there are lots of interesting key combinations using
  // C with your left hand.
  if (M_CheckParm("-ctrlc")) keyboard_translatekeys(DONT_CATCH_CTRLC);

  keyboard_seteventhandler(I_KBHandler);
  
  // Start mouse handling
  if (!M_CheckParm("-nomouse")) {
    vga_setmousesupport(1); 
    mflag = F_mouse;
  }
  
  if (!vga_hasmode(mode))
    I_Error("SVGALib reports video mode not available.");

  // Set default non-accelerated graphics funcs
  blitfunc = I_PlainBlit;
  redraw_state = blit; // Default

  if (!M_CheckParm("-noaccel")) {
    pinfo = vga_getmodeinfo(mode);
    
    if (pinfo->haveblit & HAVE_IMAGEBLIT) {
      blitfunc = vga_imageblt;
    }
    
    if (pinfo->flags & HAVE_RWPAGE ) {
      if (pinfo->flags & EXT_INFO_AVAILABLE) {
	if (pinfo->memory >= 2 * (1<<16)) {
	  redraw_state = flip;
	} else
	  if (devparm)
	    fprintf(stderr, "Insufficient memory for video paging. ");
      } else 
	if (devparm)
	  fprintf(stderr, "SVGALib did not return video memory size. ");
    } else 
      if (devparm)
	fprintf(stderr, "Video memory paging not available. "); 
  }

  fprintf(stderr, "Using:");

  if (redraw_state != blit)
    fprintf(stderr, " paging");

  if (use_vsync)
    fprintf(stderr, " retrace");

  if (blitfunc == vga_imageblt)
    fprintf(stderr, " accel");

  putc('\n', stderr);

  if (vga_setmode(mode))
    I_Error("Failed to set video mode 320x200x256");

  if (mflag == F_mouse)
    mouse_seteventhandler((__mouse_handler)I_MouseEventHandler);

  I_InitJoystick();
}

// -------------------------------------------------------------
// $Log: l_video_svgalib.c,v $
// Revision 1.1  2000/05/04 08:08:16  proff_fs
// Initial revision
//
// Revision 1.27  2000/05/01 17:50:35  Proff
// made changes to compile with VisualC and SDL
//
// Revision 1.26  2000/05/01 15:16:47  Proff
// added __inline for VisualC
//
// Revision 1.25  2000/05/01 14:37:33  Proff
// changed abs to D_abs
//
// Revision 1.24  2000/04/09 13:23:24  cph
// Remove PCX screenshot support
// Remove DOS-specific config file options
// Add level_precache option
//
// Revision 1.23  2000/03/16 13:27:28  cph
// Clean up uid stuff
//
// Revision 1.22  2000/01/25 21:33:22  cphipps
// Fix security in case of being setuid
//
// Revision 1.21  1999/10/27 18:35:51  cphipps
// Made W_CacheLump* return a const pointer
//
// Revision 1.20  1999/10/18 13:44:47  cphipps
// Fix signedness of key translation setup strings
//
// Revision 1.19  1999/10/12 13:01:11  cphipps
// Changed header to GPL
//
// Revision 1.18  1999/10/02 12:14:56  cphipps
// Fix compile error when compiled without HIGHRES
//
// Revision 1.17  1999/06/17 09:19:15  cphipps
// Remove refresh rate dots
//
// Revision 1.16  1999/05/14 08:06:36  cphipps
// Added ability to disable ctrl-c checking if desired
// Added numeric keypad codes
// Fix gamma correction 0 problems
//
// Revision 1.15  1999/03/13 09:28:41  cphipps
// Hires for the SVGALib version, thanks to patch from Gady Kozma
//
// Revision 1.14  1999/03/13 09:02:21  cphipps
// Minor tweaks to I_SetPalette
//
// Revision 1.13  1999/02/13 11:09:10  cphipps
// Stop compiler complaining of non-int array indexing
//
// Revision 1.12  1999/02/02 09:20:35  cphipps
// Changes to I_SetPalette for new faster palette handling
//
// Revision 1.11  1999/01/25 22:44:09  cphipps
// Fix the previous keycodes fix so it no longer overruns it's buffer
// Remove rogue fputc
//
// Revision 1.10  1999/01/03 16:40:22  cphipps
// Fix parameters to I_SetPalette
//
// Revision 1.9  1999/01/01 18:07:59  cphipps
// Import fixes from rain:
// Fix ,./\`'[] and PAUSE keys
//
// Revision 1.8  1999/01/01 10:35:39  cphipps
// Make I_SetPalette take a const*
//
// Revision 1.7  1998/12/29 13:27:59  cphipps
// Fix bad pointer cast in I_PlainBlit
// Fix blitting in I_PlainBlit
//
// Revision 1.6  1998/12/28 21:12:33  cphipps
// Fix gamma table const'ness
//
// Revision 1.5  1998/12/18 20:02:25  cphipps
// Fixed so compiles when I386 is not defined
//
// Revision 1.4  1998/12/18 19:09:02  cphipps
// Changed for modified high-res support
//
// Revision 1.3  1998/11/21 12:41:48  cphipps
// Compiles for DosDoom now, not working yet though
//
// Revision 1.2  1998/11/21 12:07:56  cphipps
// Working SVGALib lxdoom with hi-res changes done
// No hi-res supported with svgalib yet though
//
// Revision 1.1  1998/10/29 11:45:33  cphipps
// Initial revision
//

