/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: i_video.c,v 1.1 2000/09/20 09:34:42 figgi Exp $
 *
 *  X11 display code for LxDoom. Based on the original linuxdoom i_video.c
 *  Copyright (C) 1993-1996 by id Software
 *  Copyright (C) 1999-2000 by Colin Phipps (cph@lxdoom.linuxgames.com)
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
 *	DOOM graphics stuff for X11, UNIX.
 *
 *-----------------------------------------------------------------------------
 */

static const char
rcsid[] = "$Id: i_video.c,v 1.1 2000/09/20 09:34:42 figgi Exp $";

#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <signal.h>

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

/* uncomment to force DGA off */
/* #undef HAVE_LIBXXF86DGA */

#ifdef HAVE_LIBXEXT
// CPhipps - if available, use MITShm X extension

#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
// Had to dig up XShm.c for this one.
// It is in the libXext, but not in the XFree86 headers.
int XShmGetEventBase( Display* dpy ); // problems with g++?
int XShmQueryExtension(Display* dpy); // CP added
#endif

#ifdef HAVE_LIBXXF86DGA
#include <X11/extensions/xf86dga.h>
#endif

#ifdef I386
void (*R_DrawColumn)(void);
void (*R_DrawTLColumn)(void);
#endif

void M_QuitDOOM(int choice);

int use_vsync = 0; // Included not to break m_misc, but not relevant to X
static Display*	X_display=NULL;
static int      X_screen;
static Window   X_mainWindow;
static Colormap	X_cmap;
static Visual*	X_visual;
static GC	X_gc;
static XEvent	X_event;
static XImage*	image=NULL;
static int      X_width;
static int      X_height;
static Atom     X_deletewin;

// This is the pointer to the buffer to blit
pval     *      out_buffer = NULL;

#define INITIALISED (X_display != NULL)

#ifdef HAVE_LIBXXF86DGA
// xf86 DGA extension
static boolean          doDga;
static int		DGA_real_width, DGA_pagelen, DGA_memlen;
static int		DGA_width, DGA_height;
#endif

#ifdef HAVE_LIBXEXT
// MIT SHared Memory extension.
static boolean          doShm;
static XShmSegmentInfo	X_shminfo;
static int              X_shmeventtype;
#endif

// Common const strings
static const char lcase_lxdoom[] = { "lxdoom" };
static const char ucase_lxdoom[] = { "LXDOOM" };

////////////////////////////////////////////////////////////////////////////
// Input code 

// POLL_POINTER means do a XQueryPointer once pre frame, to measure 
//  mouse motion. The alternative is an XGrabPointer which instructs 
//  XFree86 to send events when the pointer moves - however X seems to
//  buffer these events, making it unusable
#define POLL_POINTER

// AGGR_FOCUS is short for 'aggressive focusing'. That means that
// whenever the  mouse is grabbed, grab the focus too. The alternative 
// is passive focusing, which queries if we have the input focus and 
// sets the mouse grab state to match.
// AGGR_FOCUS is more likely to be asynchronous, but could make it
// harder to  get focus away from lxdoom in some window managers.
// #define AGGR_FOCUS

// Event handling
static event_t  event; // CPhipps - static because several subs need it 
                       // so save having several scattered around
static boolean  shmFinished = true; // Are we awaiting an MITShm completion?
static int      vis_flag = VisibilityUnobscured; // Is lxdoom visible?

extern int      X_opt; // Options to change the way we use X
// it's a bitfield, currently 1 = no MitSHM, 2 = pretend 24bpp is 32bpp
int             X_bpp; // bpp that we tell X

// Mouse handling
extern int     usemouse;        // config file var
int            use_fullscreen;  /* cph - config file dummy */
static boolean grabMouse;       // internal var
static boolean grabbed = false; // Is the mouse currently grabbed

// Little struct to keep things together
typedef struct {
  int x, y;
} mouse_t;

static mouse_t	lastmouse; // Mouse status last time
static mouse_t	newmouse;  // Mouse status just queried

#ifndef POLL_POINTER
static int      buttons;
#endif

/////////////////////////////////////////////////////////////////////////////////
// Keyboard handling

//
//  Translates the key currently in X_event
//

static int I_XTranslateKey(XEvent* pEvent)
{

  int rc;
  
  switch(rc = XKeycodeToKeysym(X_display, pEvent->xkey.keycode, 0))
    {
#ifdef NETWINDER
    /* CPhipps - Jamie's hacks to work with 
                     NetWinder X server. */
    case 65430:     rc = KEYD_LEFTARROW;    break;
    case 65432:     rc = KEYD_RIGHTARROW;   break;
    case 65433:     rc = KEYD_DOWNARROW;    break;
    case 65431:     rc = KEYD_UPARROW;      break;
#endif	
    case XK_Left:	rc = KEYD_LEFTARROW;	break;
    case XK_Right:	rc = KEYD_RIGHTARROW;	break;
    case XK_Down:	rc = KEYD_DOWNARROW;	break;
    case XK_Up:	rc = KEYD_UPARROW;	break;
    case XK_Escape:	rc = KEYD_ESCAPE;	break;
    case XK_Return:	rc = KEYD_ENTER;	break;
    case XK_Tab:	rc = KEYD_TAB;		break;
    case XK_F1:	rc = KEYD_F1;		break;
    case XK_F2:	rc = KEYD_F2;		break;
    case XK_F3:	rc = KEYD_F3;		break;
    case XK_F4:	rc = KEYD_F4;		break;
    case XK_F5:	rc = KEYD_F5;		break;
    case XK_F6:	rc = KEYD_F6;		break;
    case XK_F7:	rc = KEYD_F7;		break;
    case XK_F8:	rc = KEYD_F8;		break;
    case XK_F9:	rc = KEYD_F9;		break;
    case XK_F10:	rc = KEYD_F10;		break;
    case XK_F11:	rc = KEYD_F11;		break;
    case XK_F12:	rc = KEYD_F12;		break;
	
    case XK_BackSpace:
    case XK_Delete:	rc = KEYD_BACKSPACE;	break;
      
    case XK_Pause:	rc = KEYD_PAUSE;	break;

    case XK_equal:	rc = KEYD_EQUALS;	break;
      
    case XK_minus:	rc = KEYD_MINUS;	break;
      
    case XK_Shift_L:
    case XK_Shift_R:
      rc = KEYD_RSHIFT;
      break;
      
    case XK_Control_L:
    case XK_Control_R:
      rc = KEYD_RCTRL;
      break;
      
    case XK_Alt_L:
    case XK_Meta_L:
    case XK_Alt_R:
    case XK_Meta_R:
      rc = KEYD_RALT;
      break;

    case XK_KP_0:
    case XK_KP_Insert:
      rc = KEYD_KEYPAD0; break;
    case XK_KP_1:
    case XK_KP_End:
      rc = KEYD_KEYPAD1; break;
    case XK_KP_2:
    case XK_KP_Down:
      rc = KEYD_KEYPAD2; break;
    case XK_KP_3:
    case XK_KP_Page_Down:
      rc = KEYD_KEYPAD3; break;
    case XK_KP_4:
    case XK_KP_Left:
      rc = KEYD_KEYPAD4; break;
    case XK_KP_5:
      rc = KEYD_KEYPAD0; break;
    case XK_KP_6:
    case XK_KP_Right:
      rc = KEYD_KEYPAD6; break;
    case XK_KP_7:
    case XK_KP_Home:
      rc = KEYD_KEYPAD7; break;
    case XK_KP_8:
    case XK_KP_Up:
      rc = KEYD_KEYPAD8; break;
    case XK_KP_9:
    case XK_KP_Page_Up:
      rc = KEYD_KEYPAD9; break;
    case XK_KP_Add:
      rc = KEYD_KEYPADPLUS; break;
    case XK_KP_Subtract:
      rc = KEYD_KEYPADMINUS; break;
    case XK_KP_Divide:
      rc = KEYD_KEYPADDIVIDE; break;
    case XK_KP_Multiply:
      rc = KEYD_KEYPADMULTIPLY; break;
    case XK_KP_Enter:
      rc = KEYD_KEYPADENTER; break;
      
    default:
      if (rc >= XK_space && rc <= XK_asciitilde)
	rc = rc - XK_space + ' ';
      if (rc >= 'A' && rc <= 'Z')
	rc = rc - 'A' + 'a';
      break;
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

////////////////////////////////////////////////////////////////////////////////
// Mouse handling

static boolean I_QueryPointer(mouse_t* p, unsigned int* pmask)
{
  int          rootx, rooty;
  Window       root, child;
  mouse_t      altmouse;
  unsigned int altmask;

  // If NULL parameter, use a dummy instead
  if (p == NULL) 
    p = &altmouse;
  if (pmask == NULL)
    pmask = &altmask;

  return (XQueryPointer(X_display, X_mainWindow, &root, &child, 
			&rootx, &rooty, &(p->x), &(p->y), pmask)
	  ? true : false);
}

static void I_VerifyPointerGrabState(void)
{
#ifndef AGGR_FOCUS
  boolean focused; // Are we the focused window?

  {
    Window fwin = None;
    int revert_to = 0;

    XGetInputFocus(X_display, &fwin, &revert_to);

    // WARNING: non-portable?
    focused = (fwin == X_mainWindow) ? true : false;
  }
#endif

  if (!grabMouse) return;

  // CPhipps - do not grab the mouse if paused
  // or not playing a level or the window is obscured
  // or during demo playback
  // DO grab in menus, because needed for key bindings screen
  if (paused || (gamestate != GS_LEVEL) || 
      (vis_flag != VisibilityUnobscured)
#ifndef AGGR_FOCUS
      || !focused
#endif
      || demoplayback) {

    if (grabbed) {
      // Release the mouse
      XUngrabPointer(X_display, CurrentTime);

      // Post a Doom event saying there is no movement and no buttons
      event.type = ev_mouse;
      event.data1 = event.data2 = event.data3 = 0;
    }

    grabbed = false;

  } else {

#ifdef AGGR_FOCUS
    XSetInputFocus(X_display, X_mainWindow, RevertToParent, CurrentTime);
#endif

    // grabs the pointer so it is restricted to this window
    if (!grabbed && (vis_flag == VisibilityUnobscured)) {
      XGrabPointer(X_display, X_mainWindow, True,
#ifndef POLL_POINTER
	   ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
#else
		   0,
#endif
		   GrabModeAsync, GrabModeAsync,
		   X_mainWindow, None, CurrentTime);

      I_QueryPointer(&newmouse, NULL);
    }
    
    grabbed = true;
    /* Warp the pointer back to the middle of the window
     * or it could end up stuck on an edge 
     * Only warping if needed */
    if (D_abs(newmouse.x - X_width/2) > (X_width/2 - 32) || 
	    D_abs(newmouse.y - X_height/2) > (X_height/2 - 20)) 
    {
      /*mead allow larger deltas by preserving pre-warp portion.*/
      lastmouse.x = (X_width/2 -  (newmouse.x - lastmouse.x));
      lastmouse.y = (X_height/2 - (newmouse.y - lastmouse.y));

      XWarpPointer( X_display, None, X_mainWindow,
		    0, 0, 0, 0, X_width/2, X_height/2);
    } else {
      lastmouse = newmouse;
    }
  } 
}

/////////////////////////////////////////////////////////////////////////////////
// Main input code

static void I_GetEvent(void)
{
  // CPhipps - make this local
  XEvent	X_event;

  // put event-grabbing stuff in here
  XNextEvent(X_display, &X_event);
  switch (X_event.type) {
  case KeyPress:
    event.type = ev_keydown;
    event.data1 = I_XTranslateKey(&X_event);
    D_PostEvent(&event);
    // fprintf(stderr, "k");
    break;
  case KeyRelease:
    event.type = ev_keyup;
    event.data1 = I_XTranslateKey(&X_event);
    D_PostEvent(&event);
    // fprintf(stderr, "ku");
    break;
#ifndef POLL_POINTER
  case ButtonPress:
    event.data2 = event.data3 = 0;
    event.type = ev_mouse;
    buttons = event.data1 =
      (X_event.xbutton.state & Button1Mask ? 1 : 0)
      | (X_event.xbutton.state & Button2Mask ? 2 : 0)
      | (X_event.xbutton.state & Button3Mask ? 4 : 0)
      | (X_event.xbutton.button == Button1 ? 1 : 0)
      | (X_event.xbutton.button == Button2 ? 2 : 0)
      | (X_event.xbutton.button == Button3 ? 4 : 0);
    D_PostEvent(&event);
    break;
  case ButtonRelease:
    event.data2 = event.data3 = 0;
    event.type = ev_mouse;
    event.data1 =
      (X_event.xbutton.state & Button1Mask ? 1 : 0)
      | (X_event.xbutton.state & Button2Mask ? 2 : 0)
      | (X_event.xbutton.state & Button3Mask ? 4 : 0);
    // suggest parentheses around arithmetic in operand of |
    buttons = event.data1 =
      event.data1
      ^ (X_event.xbutton.button == Button1 ? 1 : 0)
      ^ (X_event.xbutton.button == Button2 ? 2 : 0)
      ^ (X_event.xbutton.button == Button3 ? 4 : 0);
    D_PostEvent(&event);
    break;
  case MotionNotify:
    event.type = ev_mouse;
    buttons =
      (X_event.xmotion.state & Button1Mask ? 1 : 0)
      | (X_event.xmotion.state & Button2Mask ? 2 : 0)
      | (X_event.xmotion.state & Button3Mask ? 4 : 0);
    newmousex = X_event.xmotion.x;
    newmousey = X_event.xmotion.y;
    
    break;
#endif	
#ifdef MONITOR_VISIBILITY
  case VisibilityNotify:
    vis_flag = X_event.xvisibility.state;
    break;
#endif
  case ClientMessage:
    // CPhipps - allow WM quit
    if (X_event.xclient.data.l[0] == X_deletewin) {
      S_StartSound(NULL, sfx_swtchn);
      M_QuitDOOM(0);
    }
    break;
  case Expose:
  case ConfigureNotify:
    break;
    
  default:
#ifdef HAVE_LIBXEXT
    if (doShm && X_event.type == X_shmeventtype) shmFinished = true;
#endif
    break;
  }
}

#ifdef HAVE_LIBXEXT
//
// I_WaitShm
//

static void I_WaitShm(void)
{
  while (!shmFinished) {
    if (XPending(X_display))
      I_GetEvent();
    else
      usleep(10000);
  }
}
#endif

//
// I_StartTic
//
void I_StartTic (void)
{
  
  if (!X_display)
    return;

#ifndef POLL_POINTER
  newmouse = lastmouse;
#endif  

  while (XPending(X_display))
    I_GetEvent();
  
  if (grabbed) {
#ifndef POLL_POINTER
    if (newmouse.y != lastmouse.y || newmouse.x != lastmouse.x) {
      event.type = ev_mouse;
      event.data1 = buttons;
#else
    unsigned int mask;
    if (I_QueryPointer(&newmouse, &mask)) {
      // It only reaches here if pointer is on our screen
      event.data1 = ((mask & Button1Mask) ? 1 : 0)
	| ((mask & Button2Mask) ? 2 : 0)
	| ((mask & Button3Mask) ? 4 : 0);
#endif
      event.type  = ev_mouse;
      event.data2 = (newmouse.x - lastmouse.x) << 5; /*mead: make lxdoom move */
      event.data3 = (lastmouse.y - newmouse.y) << 5; /* more like lsdoom */

      D_PostEvent(&event);
    }
  }

  I_VerifyPointerGrabState();
  I_PollJoystick();
}

//
// I_StartFrame
//
void I_StartFrame (void)
{
}

//
// I_XInitInputs
//

static void I_XInitInputs(void)
{
  // Make sure we have focus
  XSetInputFocus(X_display, X_mainWindow, RevertToParent, CurrentTime);

  // check if the user wants to grab the mouse (quite unnice)
  grabMouse = M_CheckParm("-nomouse") ? false : 
    usemouse ? true : false;

  I_VerifyPointerGrabState();
  
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

  // Array of XColor structs used for setting the 256-colour palette 
  static XColor* colours;
  static int cachedgamma;
  static size_t num_pals;

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
      for (i=0 ; i<num_pals ; i++) {
	colours[i].pixel = i & 0xff;
	colours[i].flags = DoRed|DoGreen|DoBlue;
      }
    }

    // set the X colormap entries
    for (i=0 ; i<num_pals ; i++) {
      register int c;
      c = gtable[palette[0]];
      colours[i].red = (c<<8) + c;
      c = gtable[palette[1]];
      colours[i].green = (c<<8) + c;
      c = gtable[palette[2]];
      colours[i].blue = (c<<8) + c;
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
  
  // store the colors to the current colormap
  XStoreColors(X_display, X_cmap, colours + 256*pal, 256);

#ifdef HAVE_LIBXXF86DGA
  /* install DGA colormap */
  if(doDga)
     XF86DGAInstallColormap(X_display, X_screen, X_cmap);
#endif

}

/////////////////////////////////////////////////////////////////////////
// Internal helper functions

static Cursor I_XCreateNullCursor( Display* display, Window root )
{
  Pixmap cursormask;
  XGCValues xgc;
  GC gc;
  XColor dummycolour;
  Cursor cursor;
  
  cursormask = XCreatePixmap(display, root, 1, 1, 1);
  // NOTE: depth in the above call is 1 because the image is monochrone, 
  // i.e a mask
  xgc.function = GXclear;
  gc =  XCreateGC(display, cursormask, GCFunction, &xgc);
  XFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
  dummycolour.pixel = 0;
  dummycolour.red = 0;
  dummycolour.flags = 04;
  cursor = XCreatePixmapCursor(display, cursormask, cursormask,
			       &dummycolour,&dummycolour, 0,0);
  XFreePixmap(display,cursormask);
  XFreeGC(display,gc);
  return cursor;
}

#ifdef HAVE_LIBXEXT
//
// This function is probably redundant,
//  if XShmDetach works properly.
// ddt never detached the XShm memory,
//  thus there might have been stale
//  handles accumulating.
//
static void I_XShmGrabSharedMemory(size_t size)
{
  int			key = ('d'<<24) | ('o'<<16) | ('o'<<8) | 'm';
  struct shmid_ds	shminfo;
  int			minsize = 320*200;
  int			id;
  int			rc;
  // UNUSED int done=0;
  int			pollution=5;
  
  // try to use what was here before
  do {
    id = shmget((key_t) key, minsize, 0777); // just get the id
    if (id != -1) {
      rc=shmctl(id, IPC_STAT, &shminfo); // get stats on it
      if (!rc) {
	if (shminfo.shm_nattch)	{
	  fprintf(stderr, "User %d appears to be running "
		  "DOOM.  Is that wise?\n", shminfo.shm_cpid);
	  key++;
	} else {
	  if (getuid() == shminfo.shm_perm.cuid) {
	    rc = shmctl(id, IPC_RMID, 0);
	    if (!rc)
	      fprintf(stderr,
		      "Was able to kill my old shared memory\n");
	    else
	      I_Error("Was NOT able to kill my old shared memory");
	    
	    id = shmget((key_t)key, size, IPC_CREAT|0777);
	    if (id==-1)
	      I_Error("Could not get shared memory");
	    
	    rc=shmctl(id, IPC_STAT, &shminfo);
	    
	    break;
	    
	  }
	  if (size >= shminfo.shm_segsz) {
	    fprintf(stderr,
		    "will use %d's stale shared memory\n",
		    shminfo.shm_cpid);
	    break;
	  } else {
	    fprintf(stderr,
		    "warning: can't use stale "
		    "shared memory belonging to id %d, "
		    "key=0x%x\n",
		    shminfo.shm_cpid, key);
	    key++;
	  }
	}
      }else {
	I_Error("could not get stats on key=%d", key);
      }
    } else {
      id = shmget((key_t)key, size, IPC_CREAT|0777);
      if (id==-1) {
	extern int errno;
	fprintf(stderr, "errno=%d\n", errno);
	I_Error("Could not get any shared memory");
      }
      break;
    }
  } while (--pollution);
  
  if (!pollution) {
    I_Error("Sorry, system too polluted with stale shared memory segments.\n");
  }	
  
  X_shminfo.shmid = id;
  
  // attach to the shared memory segment
  image->data = X_shminfo.shmaddr = shmat(id, 0, 0);
  
  //  fprintf(stderr, "shared memory id=%d, addr=0x%x\n", id, (int) (image->data));
}
#endif

//////////////////////////////////////////////////////////////////
// Mode querying

//
// I_FindMode
//
// Scans the table above, finding a supported mode
//
static void I_PrintMode(const char* s) {
  fprintf(stderr, "%s %d bpp %s, scale x%d\n", s, BITSPP, 
	  true_color ? "TrueColor" : "PseudoColor", multiply);
}

static Visual* I_FindMode(int screen)
{
  XVisualInfo	X_visualinfo;
  int i;
  const int nice_depths[]={ 8, 16, 15, 32, 24, 0}; // 0 terminated

  XPixmapFormatValues *pfv;
  int npfv;

  fprintf(stderr, "I_FindMode: ");
  for (i=0; nice_depths[i]!=0; i++) {
    dest_bpp = nice_depths[i];
    if (I_QueryImageTranslation()) {
      if (XMatchVisualInfo(X_display, screen, dest_bpp, 
			   true_color ? TrueColor : PseudoColor, 
			   &X_visualinfo))
	break;
      if (devparm) I_PrintMode("no visual like:");
    } else 
      if (devparm) I_PrintMode("no translation to:");
  }
  if (nice_depths[i] == 0)
    I_Error("Unable to find supported visual settings");

  X_bpp = dest_bpp;
  if ((dest_bpp == 24) && (X_opt & 2)) dest_bpp = 32; // kludge for 24bpp

  /* find out whether depth 24 is sparse or packed */
  pfv=XListPixmapFormats(X_display,&npfv);
  if(pfv) {
     for(i=0;i<npfv;i++)
	if(pfv[i].depth==dest_bpp) break;
     if(i<npfv) {
	dest_bpp=pfv[i].bits_per_pixel;
	fprintf(stderr,"(Depth %d / %d BPP) ",X_bpp,dest_bpp);
     }
     else 
	fprintf(stderr,"(Depth %d: no pixfmt?) ",dest_bpp);
     XFree(pfv);
  }

  I_InitImageTranslation();

  if (true_color) {
    // Set up colour shifts
    I_SetColourShift(X_visualinfo.red_mask, &redshift);
    I_SetColourShift(X_visualinfo.green_mask, &greenshift);
    I_SetColourShift(X_visualinfo.blue_mask, &blueshift);
  }

  I_PrintMode("using");

  return X_visualinfo.visual;
}

//////////////////////////////////////////////////////////////////////////////
// Graphics API

void I_ShutdownGraphics(void)
{
  fprintf(stderr, "I_ShutdownGraphics : ");

  // Free internal structures
  if (pixelvals != NULL) free(pixelvals);
  I_EndImageTranslation();

#ifdef HAVE_LIBXXF86DGA
  if (doDga) {
     XF86DGADirectVideo(X_display, X_screen, 0);
  }
  else
#endif

#ifdef HAVE_LIBXEXT
  if (doShm) {
    // Detach from X server
    if (!XShmDetach(X_display, &X_shminfo))
      I_Error("XShmDetach() failed in I_ShutdownGraphics()");
    else
      fprintf(stderr, "Released XShm shared memory.\n\r");

    // Release shared memory.
    shmdt(X_shminfo.shmaddr);
    shmctl(X_shminfo.shmid, IPC_RMID, 0);
    
    // Paranoia.
    image->data = NULL;
  } else
#endif
  {
    if (multiply != 1) {
      free(image->data);
    }
    image->data = NULL; // Prevent server trying to free image data
    XDestroyImage(image);
    fprintf(stderr, "Released XImage memory\n");
  }
  XDestroyWindow(X_display, X_mainWindow);
  XCloseDisplay(X_display);
  X_display = NULL;
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
  // Protect against calls before I_InitGraphics
  if (!INITIALISED || I_SkipFrame()) return;

#ifdef MONITOR_VISIBILITY
  if (vis_flag == VisibilityFullyObscured) {
    return;
  }
#endif
  
#ifdef HAVE_LIBXEXT
  if (doShm && expand_buffer)
    I_WaitShm();
#endif
  
  // scales the screen size before blitting it
  if (expand_buffer)
    (*I_ExpandImage)(out_buffer, screens[0]);
  
#ifdef HAVE_LIBXXF86DGA
  if (doDga) {
    int i;
     for(i=0;i<X_height;i++)
	   memcpy(((char*)out_buffer)+i*DGA_real_width,
		  ((char*)(screens[0]))+i*X_width,
		  X_width);

  }
  else
#endif

#ifdef HAVE_LIBXEXT    
  if (doShm) {
    if (!XShmPutImage(	X_display, X_mainWindow,
			X_gc, image,
			0, 0, 0, 0,
			X_width, X_height, True))
      I_Error("XShmPutImage() failed\n");
    
    // If we will reuse this buffer in next rendering, wait for it to finish
    shmFinished = false;
    if (!expand_buffer) 
      I_WaitShm();
  } else
#endif
  {
    // draw the image
    XPutImage(	X_display, X_mainWindow,
		X_gc, image,
		0, 0, 0, 0,
		X_width, X_height );
    
    // sync up with server
    XSync(X_display, False);
  }
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
  if (!INITIALISED) return;
  if (true_color) {
    int            lump = W_GetNumForName("PLAYPAL");
    const byte *palette = W_CacheLumpNum(lump);
    I_SetPaletteTranslation(palette + (3*256)*pal);
    W_UnlockLumpNum(lump);
  } else
    I_UploadNewPalette(pal);
}

// I_PreInitGraphics

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
    X_width = (width+3) & ~3;

#ifdef HIGHRES
  SCREENHEIGHT = 
#endif
    X_height = (height+3) & ~3;

#ifdef I386
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
  const char*	displayname = NULL;
  int		n;
  int		pnum;
  int		x=0;
  int		y=0;
  
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
  
  X_width = SCREENWIDTH * multiply;
  X_height = SCREENHEIGHT * multiply;
  
  // check for command-line geometry
  if ( (pnum=M_CheckParm("-geom")) ? 1 : (pnum=M_CheckParm("-geometry"))) 
   if (pnum+1 < myargc) { // check parm given
    // warning: char format, different type arg
    char	xsign=' ';
    char	ysign=' ';
    const char* pg = myargv[pnum+1];
    
    pg += strcspn(pg, "+-");
    
    // warning: char format, different type arg 3,5
    n = sscanf(pg, "%c%d%c%d", &xsign, &x, &ysign, &y);
    
    if (n==2)
      x = y = 0;
    else if (n==4) {
      if (xsign == '-')
	x = -x;
      if (ysign == '-')
	y = -y;
    } else
      fprintf(stderr, "I_InitGraphics: bad -geom offsets \"%s\"\n", pg);
  }
  
  // check for command-line display name
  if ( (pnum=M_CheckParm("-disp")) ) // suggest parentheses around assignment
    displayname = myargv[pnum+1];
  // CPhipps - support more standard -display param
  if ( (pnum=M_CheckParm("-display")) ) // ditto
    displayname = myargv[pnum+1];
  // CPhipps - and -displayname as for xterm
  if ( (pnum=M_CheckParm("-displayname")) ) // ditto
    displayname = myargv[pnum+1];

  // open the display
  if (!(X_display = XOpenDisplay(displayname))) {
    if (displayname)
      I_Error("Could not open display [%s]", displayname);
    else
      I_Error("Could not open display (DISPLAY=[%s])", getenv("DISPLAY"));
  }

  // use the default visual 
  X_screen = DefaultScreen(X_display);

  X_deletewin = XInternAtom(X_display, "WM_DELETE_WINDOW", False);

  // check for the MITSHM extension
  // even if it's available, make sure it's a local connection
  lprintf(LO_INFO,"I_InitGraphics:");

#ifdef HAVE_LIBXXF86DGA
  if ((doDga = (M_CheckParm("-dga") && XF86DGAQueryExtension(X_display, &n, &n)) )) {
    lprintf(LO_INFO,"I_InitGraphics: found DGA extension\n");
    if (M_CheckParm("-noaccel") || (X_opt & 1)) doDga = false;
    else lprintf(LO_INFO, "(using DGA)");
  }
  else 
#endif
#ifdef HAVE_LIBXEXT
  if ((doShm = XShmQueryExtension(X_display))) {

    if (!displayname) displayname = getenv("DISPLAY");
    
    if (displayname) {
      // CPhipps - don't modify the original string, please.
      //         - oops my code here was totally wrong
      // I have no idea exactly what should go here.

      if (M_CheckParm("-noaccel") || (X_opt & 1)) doShm = false;
      else lprintf(LO_INFO, "(using MITShm)");
    }
  }
  else 
#endif 
    if (devparm) 
      fprintf(stderr, "No MITShm extension in server");

  if (devparm)
    fputc('\n', stderr);

  if ((X_visual = I_FindMode(X_screen)) == NULL) 
    I_Error("Unsupported visual");
  
  // create the colormap
  X_cmap = XCreateColormap(X_display, RootWindow(X_display, X_screen), 
			   X_visual, true_color ? AllocNone : AllocAll);

#ifdef HAVE_LIBXXF86DGA
  /* setup for DGA */
  if(doDga) {
     char *fb;
     X_mainWindow=RootWindow(X_display, X_screen);
     XF86DGAGetViewPortSize(X_display, X_screen, &DGA_width, &DGA_height);
     XF86DGAGetVideo(X_display, X_screen, &fb,
		     &DGA_real_width, &DGA_pagelen, &DGA_memlen);

     fprintf(stderr,
	     "I_InitGraphics: DGA reports %dx%d scan=%d page=%d mem=%d\n",
	     DGA_width,DGA_height,DGA_real_width,DGA_pagelen,DGA_memlen);

     out_buffer=(void*)fb;

     XF86DGADirectVideo(X_display, X_screen, XF86DGADirectGraphics);
     XF86DGASetVidPage(X_display, X_screen, 0);
     XF86DGAForkApp(X_screen);
     XSelectInput(X_display, X_mainWindow, KeyPressMask | KeyReleaseMask);
     fprintf(stderr,"DGA framebuffer @%p\n",fb);
     memset(fb,0,102400);
  }
#endif

#ifdef HAVE_LIBXXF86DGA
  if(!doDga)
#endif

  { // setup attributes for main window
    unsigned long	attribmask;
    XSetWindowAttributes attribs;

    attribmask = CWEventMask | CWColormap | CWBorderPixel;
    attribs.event_mask = KeyPressMask | KeyReleaseMask
#ifdef MONITOR_VISIBILITY
      | VisibilityChangeMask
#endif
      | ExposureMask;
    
    attribs.colormap = X_cmap;
    attribs.border_pixel = 0;
    
    // create the main window
    X_mainWindow = XCreateWindow( X_display,
				  RootWindow(X_display, X_screen),
				  x, y,
				  X_width, X_height,
				  0, // borderwidth
				  X_bpp, // depth
				  InputOutput, X_visual,
				  attribmask, &attribs );
  }
  
#ifdef HAVE_LIBXXF86DGA
  if(!doDga)
#endif

  {
    XClassHint c_hint;
    XTextProperty x_name;
    XSizeHints s_hint;
    XWMHints wm_hint;
    char lcase_buf[10], ucase_buf[10];
    char* str;

    strcpy(str = lcase_buf, lcase_lxdoom);
    XStringListToTextProperty(&str, 1, &x_name);

    s_hint.flags = PSize | PMinSize | PMaxSize;
    s_hint.min_width = s_hint.max_width = s_hint.base_width = X_width;
    s_hint.min_height = s_hint.max_height = s_hint.base_height = X_height;

    wm_hint.input = True;
    wm_hint.window_group = X_mainWindow;
    wm_hint.flags = InputHint | WindowGroupHint;

    strcpy(c_hint.res_name = lcase_buf, lcase_lxdoom);
    strcpy(c_hint.res_class = ucase_buf, ucase_lxdoom);
    
    XSetWMProtocols(X_display, X_mainWindow, &X_deletewin, 1);
    
    XSetWMProperties(X_display, X_mainWindow, &x_name, &x_name, 
		     (char**)myargv, myargc, &s_hint, &wm_hint, &c_hint);

    XFlush(X_display);
  }

#ifdef HAVE_LIBXXF86DGA
  if(!doDga)
#endif
  // Hide pointer while over this window
  XDefineCursor(X_display, X_mainWindow,
		I_XCreateNullCursor( X_display, X_mainWindow ) );
  
  { // create the GC
    XGCValues		xgcvalues;
    int		valuemask;
    
    valuemask = GCGraphicsExposures;
    xgcvalues.graphics_exposures = False;
    X_gc = XCreateGC(X_display, X_mainWindow, valuemask, &xgcvalues);
  }
  
#ifdef HAVE_LIBXXF86DGA
  if(!doDga)
#endif
  {
     // name the window
     XStoreName(X_display, X_mainWindow, "lxdoom");
  
     // map the window
     XMapWindow(X_display, X_mainWindow);
  }  

  // wait until it is OK to draw
#ifdef HAVE_LIBXXF86DGA
  if(!doDga)
#endif
  do {
    XNextEvent(X_display, &X_event);
  } while (!(X_event.type == Expose && !X_event.xexpose.count));

#ifdef HAVE_LIBXXF86DGA
  if(doDga) {}
  else
#endif

#ifdef HAVE_LIBXEXT
  if (doShm) {
    
    X_shmeventtype = XShmGetEventBase(X_display) + ShmCompletion;
    
    // create the image
    image = XShmCreateImage( X_display, X_visual,
			     X_bpp, ZPixmap,
			     0, &X_shminfo,
			     X_width, X_height );
    
    I_XShmGrabSharedMemory(image->bytes_per_line * image->height);
    
    if (!(out_buffer = (pval*)image->data)) {
      perror("");
      I_Error("shmat() failed in I_InitGraphics()");
    }
    
    // get the X server to attach to it
    if (!XShmAttach(X_display, &X_shminfo))
      I_Error("XShmAttach() failed in InitGraphics()");
    
    if (!expand_buffer) {
      // Render directly into MitSHM memory
      Z_Free(screens[0]);
      screens[0] = (unsigned char *) (image->data); 
    }
  } else
#endif 
  {
    if (!expand_buffer) {
      // Very efficient, render directly into image
      out_buffer = (pval*)screens[0];
    } else {
      // Will have to enlarge from rendering buffer
      //  into image buffer
      out_buffer = (pval*)malloc (X_width * X_height * BYTESPP);
    }

    image=XCreateImage( X_display, X_visual,
			X_bpp, ZPixmap,
			0, (char*)out_buffer,
			X_width, X_height,
			X_bpp, 0);

    if (!image) 
      I_Error("XCreateImage: failed to create image %dx%d %d bpp", 
	      X_width, X_height, X_bpp);
  }
  
  atexit(I_ShutdownGraphics);

  I_XInitInputs();
}
