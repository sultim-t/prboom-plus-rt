// Emacs style mode select   -*- C++ -*-
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
//----------------------------------------------------------------------------
//
// DESCRIPTION:
//   
//   System-specific graphics code, along with some ill-placed
//   keyboard, mouse, and joystick code that really ought to have
//   its own module.
//
//-----------------------------------------------------------------------------

static const char rcsid[] = "$Id: i_video.c,v 1.12 1998/05/03 22:40:35 killough Exp $";

#include "../z_zone.h"  /* memory allocation wrappers -- killough */

#include <stdio.h>
#include <signal.h>

// Julian: 6/6/2001: replaced by local inclusion
#include "allegro.h"

#include <dpmi.h>
#include <sys/nearptr.h>
#include <sys/farptr.h>
#include <dos.h>
#include <go32.h>

#include "../doomstat.h"

#include "../am_map.h"
#include "../c_io.h"
#include "../c_runcmd.h"
#include "../d_main.h"
#include "../i_video.h"
#include "../m_argv.h"
#include "../m_bbox.h"
#include "../mn_engin.h"
#include "../r_draw.h"
#include "../st_stuff.h"
#include "../v_video.h"
#include "../w_wad.h"
#include "../z_zone.h"
#include "../d_gi.h"

///////////////////////////////////////////////////////////////////////////
//
// Input Code
//
//////////////////////////////////////////////////////////////////////////

extern void I_InitKeyboard();      // i_system.c

//
// Keyboard routines
// By Lee Killough
// Based only a little bit on Chi's v0.2 code
//

int I_ScanCode2DoomCode(int a)
{
   switch(a)
   {
   default:   return key_ascii_table[a]>8 ? key_ascii_table[a] : a+0x80;
   case 0x7b: return KEYD_PAUSE;
   case 0x0e: return KEYD_BACKSPACE;
   case 0x48: return KEYD_UPARROW;
   case 0x4d: return KEYD_RIGHTARROW;
   case 0x50: return KEYD_DOWNARROW;
   case 0x4b: return KEYD_LEFTARROW;
   case 0x38: return KEYD_LALT;
   case 0x79: return KEYD_RALT;
   case 0x1d:
   case 0x78: return KEYD_RCTRL;
   case 0x36:
   case 0x2a: return KEYD_RSHIFT;
   }
}

// Automatic caching inverter, so you don't need to maintain two tables.
// By Lee Killough

int I_DoomCode2ScanCode(int a)
{
   static int inverse[256], cache;
   
   for(; cache<256; ++cache)
      inverse[I_ScanCode2DoomCode(cache)] = cache;
   
   return inverse[a];
}


extern int usemouse;   // killough 10/98

/////////////////////////////////////////////////////////////////////////////
//
// JOYSTICK                                                  // phares 4/3/98
//
/////////////////////////////////////////////////////////////////////////////

extern int usejoystick;
extern int joystickpresent;

/* Julian: 6/6/2001 : Allegro compatibility
   Removed declarations that are of no use whatever version of allegro is used
   since allegro.h is included, but were causing conflicts on earlier allegro

   old code here :

        // Seems like the ints have been replaced by #defines
        extern int joy_x,joy_y;
        extern int joy_b1,joy_b2,joy_b3,joy_b4;

        // prototype has changed (returns int in earlier version)
        void poll_joystick(void);
*/

// I_JoystickEvents() gathers joystick data and creates an event_t for
// later processing by G_Responder().

static void I_JoystickEvents()
{
   event_t event;
   static int old_joy_b1, old_joy_b2, old_joy_b3, old_joy_b4;
   
   if(!joystickpresent || !usejoystick)
      return;
   poll_joystick(); // Reads the current joystick settings
   event.type = ev_joystick;
   event.data1 = 0;

   // read the button settings
   
   event.data1 =
      (joy_b1 ? 1 : 0) | (joy_b2 ? 2 : 0) |
      (joy_b3 ? 4 : 0) | (joy_b4 ? 8 : 0);

   // Read the x,y settings. Convert to -1 or 0 or +1.

   if(joy_x < 0)
      event.data2 = -1;
   else if(joy_x > 0)
      event.data2 = 1;
   else
      event.data2 = 0;

   if(joy_y < 0)
      event.data3 = -1;
   else if(joy_y > 0)
      event.data3 = 1;
   else
      event.data3 = 0;

   // post what you found
   
   D_PostEvent(&event);
   
   // build button events (make joystick buttons virtual keyboard keys
   // as originally suggested by lee killough in the boom suggestions file)

   if(joy_b1 != old_joy_b1)
   {
      event.type = joy_b1 ? ev_keydown : ev_keyup;
      event.data1 = KEYD_JOY1;
      D_PostEvent(&event);
      old_joy_b1 = joy_b1;
   }
   if(joy_b2 != old_joy_b2)
   {
      event.type = joy_b2 ? ev_keydown : ev_keyup;
      event.data1 = KEYD_JOY2;
      D_PostEvent(&event);
      old_joy_b2 = joy_b2;
   }
   if(joy_b3 != old_joy_b3)
   {
      event.type = joy_b3 ? ev_keydown : ev_keyup;
      event.data1 = KEYD_JOY3;
      D_PostEvent(&event);
      old_joy_b3 = joy_b3;
   }
   if(joy_b4 != old_joy_b4)
   {
      event.type = joy_b4 ? ev_keydown : ev_keyup;
      event.data1 = KEYD_JOY4;
      D_PostEvent(&event);
      old_joy_b4 = joy_b4;
   }
}


//
// I_StartFrame
//
void I_StartFrame(void)
{
   I_JoystickEvents(); // Obtain joystick data                 phares 4/3/98
}

/////////////////////////////////////////////////////////////////////////////
//
// END JOYSTICK                                              // phares 4/3/98
//
/////////////////////////////////////////////////////////////////////////////

// killough 3/22/98: rewritten to use interrupt-driven keyboard queue

// haleyjd 01/09/01: fixed problem that made the new mouse virtual key
// 		     code unreachable

static void I_GetEvent(void)
{
   event_t event;
   int tail;
   
   while((tail=keyboard_queue.tail) != keyboard_queue.head)
   {
      int k = keyboard_queue.queue[tail];
      keyboard_queue.tail = (tail+1) & (KQSIZE-1);
      event.type = k & 0x80 ? ev_keyup : ev_keydown;
      event.data1 = I_ScanCode2DoomCode(k & 0x7f);
      D_PostEvent(&event);
   }
   
   if(mousepresent != -1 && usemouse) // killough 10/98
   {
      static int lastbuttons;
      int xmickeys,ymickeys,buttons=mouse_b;
      
      get_mouse_mickeys(&xmickeys,&ymickeys);
      
      // haleyjd: buttons != lastbuttons still needs to be
      // here for allowing the original "buttons" variable, which
      // is still used for double-click mouse actions, to be updated
      // properly
      if(xmickeys || ymickeys || buttons != lastbuttons)
      {
	 event.data1 = buttons;
	 event.data3 = -ymickeys;
	 event.data2 = xmickeys;
	 event.type  = ev_mouse;
	 D_PostEvent(&event);
      }
      
      // haleyjd: generate virtual mouse key events as well
      if(buttons != lastbuttons)
      {
	 if((buttons & 1) != (lastbuttons & 1))
	 {
	    event.type = buttons & 1 ? ev_keydown : ev_keyup;
	    event.data1 = KEYD_MOUSE1;
            D_PostEvent(&event);
	 } 
         if((buttons & 2) != (lastbuttons & 2))
	 {
	    event.type = buttons & 2 ? ev_keydown : ev_keyup;
            event.data1 = KEYD_MOUSE2;
            D_PostEvent(&event);
	 }
         if((buttons & 4) != (lastbuttons & 4))
	 {
	    event.type = buttons & 4 ? ev_keydown : ev_keyup;
            event.data1 = KEYD_MOUSE3;
            D_PostEvent(&event);
	 }
         lastbuttons=buttons;
      }
  }
}

//
// I_StartTic
//

void I_StartTic(void)
{
   I_GetEvent();
}

////////////////////////////////////////////////////////////////////////////
//
// Graphics Code
//
////////////////////////////////////////////////////////////////////////////

//
// I_UpdateNoBlit
//

void I_UpdateNoBlit(void)
{
}

// 1/25/98 killough: faster blit for Pentium, PPro and PII CPUs:
extern void ppro_blit(void *, size_t);
extern void pent_blit(void *, size_t);

extern void blast(void *destin, void *src);  // blits to VGA planar memory
extern void ppro_blast(void *destin, void *src);  // same but for PPro CPU

int use_vsync;     // killough 2/8/98: controls whether vsync is called
int page_flip;     // killough 8/15/98: enables page flipping
int hires;
int vesamode;
BITMAP *screens0_bitmap;
boolean noblit;

static boolean in_graphics_mode;
static boolean in_page_flip, in_highres, linear; // ANYRES
static int scroll_offset;
static unsigned long screen_base_addr;
static unsigned destscreen;

// OK, I can't exactly implement ANYRES in dos but I still need to retrofit it to interact with
// the anyres code correctly.
void I_FinishUpdate(void)
{
   if(noblit || !in_graphics_mode)
      return;

   if(in_page_flip)
   {
      if(!in_highres) // SoM: ANYRES
      {
         //
         // killough 8/15/98:
         //
         // 320x200 Wait-free page-flipping for flicker-free display
         //
         // blast it to the screen
         
         destscreen += 0x4000;             // Move address up one page
         destscreen &= 0xffff;             // Reduce address mod 4 pages
         
         // Pentium Pros and above need special consideration in the
         // planar multiplexing code, to avoid partial stalls. killough
         
         if(cpu_family >= 6)
            ppro_blast((byte *) __djgpp_conventional_base
                       + 0xa0000 + destscreen, *screens);
         else
            blast((byte *) __djgpp_conventional_base  // Other CPUs, e.g. 486
                  + 0xa0000 + destscreen, *screens);

         // page flip 
         outportw(0x3d4, destscreen | 0x0c); 
         
         return;
      }
      else       // hires hardware page-flipping (VBE 2.0)
         scroll_offset = scroll_offset ? 0 : 400;
   }
   else        // killough 8/15/98: no page flipping; use other methods
      if(use_vsync && !timingdemo)
         vsync(); // killough 2/7/98: use vsync() to prevent screen breaks.

   if(!linear)  // Banked mode is slower. But just in case it's needed...
      blit(screens0_bitmap, screen, 0, 0, 0, scroll_offset,
           SCREENWIDTH << hires, SCREENHEIGHT << hires);
   else
   {   // 1/16/98 killough: optimization based on CPU type
      int size =
         in_highres ? SCREENWIDTH*SCREENHEIGHT*4 : SCREENWIDTH*SCREENHEIGHT;
      byte *dascreen = screen_base_addr + screen->line[scroll_offset];
      if(cpu_family >= 6)     // PPro, PII
         ppro_blit(dascreen,size);
      else
         if(cpu_family >= 5)   // Pentium
            pent_blit(dascreen,size);
         else                   // Others
            memcpy(dascreen,*screens,size);
   }

   if(in_page_flip)  // hires hardware page-flipping (VBE 2.0)
      scroll_screen(0, scroll_offset);
}

//
// I_ReadScreen
//

void I_ReadScreen(byte *scr)
{
   int size = hires ? SCREENWIDTH*SCREENHEIGHT*4 : SCREENWIDTH*SCREENHEIGHT;
   
   // 1/18/98 killough: optimized based on CPU type:
   if(cpu_family >= 6)     // PPro or PII
      ppro_blit(scr,size);
   else
      if(cpu_family >= 5)   // Pentium
         pent_blit(scr,size);
      else                     // Others
         memcpy(scr,*screens,size);
}

//
// killough 10/98: init disk icon
//

int disk_icon;

static BITMAP *diskflash, *old_data;

static void I_InitDiskFlash(void)
{
   byte temp[32*32];
   
   if(diskflash)
   {
      destroy_bitmap(diskflash);
      destroy_bitmap(old_data);
   }
   
   //sf : disk is actually 16x15
   diskflash = create_bitmap_ex(8, 16<<hires, 15<<hires);
   old_data = create_bitmap_ex(8, 16<<hires, 15<<hires);
   
   V_GetBlock(0, 0, 0, 16, 15, temp);
   V_DrawPatchDirect(0, -1, &vbscreen, W_CacheLumpName(M_CheckParm("-cdrom") ?
                     "STCDROM" : "STDISK", PU_CACHE));
   V_GetBlock(0, 0, 0, 16, 15, diskflash->line[0]);
   V_DrawBlock(0, 0, &vbscreen, 16, 15, temp);
}

//
// killough 10/98: draw disk icon
//

void I_BeginRead(void)
{
   if(!disk_icon || !in_graphics_mode)
      return;
   
   blit(screen, old_data,
        (SCREENWIDTH-16) << hires,
        scroll_offset + ((SCREENHEIGHT-15)<<hires),
        0, 0, 16 << hires, 15 << hires);
   
   blit(diskflash, screen, 0, 0, (SCREENWIDTH-16) << hires,
        scroll_offset + ((SCREENHEIGHT-15)<<hires), 
        16 << hires, 15 << hires);
}

//
// killough 10/98: erase disk icon
//

void I_EndRead(void)
{
   if(!disk_icon || !in_graphics_mode)
      return;
   
   blit(old_data, screen, 0, 0, (SCREENWIDTH-16) << hires,
        scroll_offset + ((SCREENHEIGHT-15)<<hires), 
        16 << hires, 15 << hires);
}

void I_SetPalette(byte *palette)
{
   int i;
   
   if(!in_graphics_mode)             // killough 8/11/98
      return;
   
   if(!timingdemo)
      while(!(inportb(0x3da) & 8));

   outportb(0x3c8,0);
   for(i = 0; i < 256; ++i)
   {
      outportb(0x3c9, gammatable[usegamma][*palette++] >> 2);
      outportb(0x3c9, gammatable[usegamma][*palette++] >> 2);
      outportb(0x3c9, gammatable[usegamma][*palette++] >> 2);
   }
}

void I_ShutdownGraphics(void)
{
   if(in_graphics_mode)  // killough 10/98
   {
      clear(screen);
      
      // Turn off graphics mode
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      
      in_graphics_mode = false;
      in_textmode = true;
   }
}

#define BADVID "video mode not supported"

extern boolean setsizeneeded;

//
// killough 11/98: New routine, for setting hires and page flipping
//

// sf: now returns true if an error occurred

static boolean I_InitGraphicsMode(void)
{
   set_color_depth(8);     // killough 2/7/98: use allegro set_gfx_mode
   scroll_offset = 0;
   
   // haleyjd: We must call V_Init before Lee's Mode X setup code,
   // so we have to determine v_width and v_height here, rather than
   // below in the switch.

   v_width  = 320 << hires;
   v_height = 200 << hires;

   V_Init();
   
   
   // sf: reorganised to use v_mode
   
   switch(v_mode)
   {
      // mode 1: vga pageflipped, like original doom
      // by lee killough
      
   case 1:
      {
         // killough 8/15/98:
         //
         // Page flipping code for wait-free, flicker-free display.
         //
         // This is the method Doom originally used, and was removed in the
         // Linux source. It only works for 320x200. The VGA hardware is
         // switched to planar mode, which allows 256K of VGA RAM to be
         // addressable, and allows page flipping, split screen, and hardware
         // panning.
         
         destscreen = 0;
         
         //
         // VGA mode 13h
         //
         
         set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0);
         
         // 
         // turn off chain 4 and odd/even 
         // 
         outportb(0x3c4, 4); 
         outportb(0x3c5, (inportb(0x3c5) & ~8) | 4); 
         
         // 
         // turn off odd/even and set write mode 0 
         // 
         outportb(0x3ce, 5); 
         outportb(0x3cf, inportb(0x3cf) & ~0x13);
         
         // 
         // turn off chain 4 
         // 
         outportb(0x3ce, 6); 
         outportb(0x3cf, inportb(0x3cf) & ~2); 
         
         // 
         // clear the entire buffer space, because
         // int 10h only did 16 k / plane 
         // 
         outportw(0x3c4, 0xf02);
         blast(0xa0000 + (byte *) __djgpp_conventional_base, *screens);
         
         // Now we do most of this stuff again for Allegro's benefit :)
         // All that work above was just to clear the screen first.
         set_gfx_mode(GFX_MODEX, 320, 200, 320, 800);         
      }
      break;

      // 320x200 VESA
      // can be significantly faster than vga 320x200 
      
   case 2:
      if(set_gfx_mode(GFX_VESA2L, 320, 200, 320, 200))
      {
         I_SetMode(0);        // reset to mode 0
         MN_ErrorMsg(BADVID);
         return true;
      }
      break;
	
      // 3: hires mode 640x400
      
   case 3:
      if(set_gfx_mode(GFX_AUTODETECT, 640, 400, 0, 0))
      {
         I_SetMode(0);               // reset to vga low res
         MN_ErrorMsg(BADVID);
         return true;  // error
      }
      break;
      
      // 4: hires mode pageflipped 640x400
	
   case 4:
      if(set_gfx_mode(GFX_AUTODETECT, 640, 400, 640, 800))
      {
         // revert to normal non-pageflipped
         I_SetMode(3);
         MN_ErrorMsg(BADVID);
         return true;
      }
      else
         set_clip(screen, 0, 0, 640, 800);    // Allow full access      
      break;
      
      // 0: 320x200 plain ol' VGA
      // we place this at the end with default:
	
   default:
   case 0:
      if(set_gfx_mode(GFX_AUTODETECT, 320, 200, 320, 200))
      {
         // if we cant set this, we're screwed :)
         I_Error("couldn't get 320x200 vga!\n");
      }
      break;
   }

   if(v_mode != 1)
   {
      linear = is_linear_bitmap(screen);
      
      __dpmi_get_segment_base_address(screen->seg, &screen_base_addr);
      screen_base_addr -= __djgpp_base_address;
   }

   MN_ErrorMsg("");       // clear any error messages
   
   in_graphics_mode = true;
   in_textmode = false;
   in_page_flip = page_flip;
   in_highres = hires;
   
   setsizeneeded = true;
   
   I_InitDiskFlash();        // Initialize disk icon
   I_SetPalette(W_CacheLumpName("PLAYPAL",PU_CACHE));
   
   return false;
}

static void I_ResetScreen(void)
{
   // Switch out of old graphics mode
   
   I_ShutdownGraphics();
   
   // Switch to new graphics mode
   // check for errors -- we may be setting to a different mode instead
  
   if(I_InitGraphicsMode())
      return;
   
   // reset other modules
   
   if(automapactive)
      AM_Start();             // Reset automap dimensions
   
   ST_Start();               // Reset palette
   
   if(gamestate == GS_INTERMISSION)
   {
      gameModeInfo->interfuncs->DrawBackground();
      V_CopyRect(0, 0, 1, SCREENWIDTH, SCREENHEIGHT, 0, 0, 0);
   }
   
   Z_CheckHeap();
}

void I_InitGraphics(void)
{
   static int firsttime = true;
   
   if (!firsttime)
      return;
   
   firsttime = false;
   
   check_cpu();    // 1/16/98 killough -- sets cpu_family based on CPU
   
#ifndef RANGECHECK
   asm("fninit");  // 1/16/98 killough -- prevents FPU exceptions
#endif

   timer_simulate_retrace(0);
   
   if(nodrawers) // killough 3/2/98: possibly avoid gfx mode
      return;
   
   // init keyboard
   I_InitKeyboard();

   //
   // enter graphics mode
   //

   atexit(I_ShutdownGraphics);
   
   signal(SIGINT, SIG_IGN);  // ignore CTRL-C in graphics mode
   
   in_page_flip = page_flip;
   
   V_ResetMode();
   
   Z_CheckHeap();
}

// the list of video modes is stored here in i_video.c
// the console commands to change them are in v_misc.c,
// so that all the platform-specific stuff is in here.
// v_misc.c does not care about the format of the videomode_t,
// all it asks is that it contains a text value 'description'
// which describes the mode
        
videomode_t videomodes[]=
{
   // hires, pageflip, vesa, description
   {0, 0, 0, "320x200 VGA"},
   {0, 1, 0, "320x200 VGA (pageflipped)"},
   {0, 0, 1, "320x200 VESA"},
   {1, 0, 0, "640x400"},
   {1, 1, 0, "640x400 (pageflipped)"},
   {0, 0, 0,  NULL}  // last one has NULL description
};

void I_SetMode(int i)
{
   static int firsttime = true;    // the first time to set mode
   
   v_mode = i;
   
   hires = videomodes[i].hires;
   page_flip = videomodes[i].pageflip;
   vesamode = videomodes[i].vesa;
   
   if(firsttime)
      I_InitGraphicsMode();
   else
      I_ResetScreen();
   
   firsttime = false;
}        

/************************
        CONSOLE COMMANDS
 ************************/

VARIABLE_BOOLEAN(use_vsync, NULL,  yesno);
VARIABLE_BOOLEAN(disk_icon, NULL,  onoff);

CONSOLE_VARIABLE(v_diskicon, disk_icon, 0) {}
CONSOLE_VARIABLE(v_retrace, use_vsync, 0)
{
   V_ResetMode();
}

VARIABLE_INT(usemouse, NULL,            0, 1, yesno);
VARIABLE_INT(usejoystick, NULL,         0, 1, yesno);

CONSOLE_VARIABLE(use_mouse, usemouse, 0) {}
CONSOLE_VARIABLE(use_joystick, usejoystick, 0) {}

void I_Video_AddCommands(void)
{
   C_AddCommand(use_mouse);
   C_AddCommand(use_joystick);
   
   C_AddCommand(v_diskicon);
   C_AddCommand(v_retrace);
}

//----------------------------------------------------------------------------
//
// $Log: i_video.c,v $
// Revision 1.12  1998/05/03  22:40:35  killough
// beautification
//
// Revision 1.11  1998/04/05  00:50:53  phares
// Joystick support, Main Menu re-ordering
//
// Revision 1.10  1998/03/23  03:16:10  killough
// Change to use interrupt-driver keyboard IO
//
// Revision 1.9  1998/03/09  07:13:35  killough
// Allow CTRL-BRK during game init
//
// Revision 1.8  1998/03/02  11:32:22  killough
// Add pentium blit case, make -nodraw work totally
//
// Revision 1.7  1998/02/23  04:29:09  killough
// BLIT tuning
//
// Revision 1.6  1998/02/09  03:01:20  killough
// Add vsync for flicker-free blits
//
// Revision 1.5  1998/02/03  01:33:01  stan
// Moved __djgpp_nearptr_enable() call from I_video.c to i_main.c
//
// Revision 1.4  1998/02/02  13:33:30  killough
// Add support for -noblit
//
// Revision 1.3  1998/01/26  19:23:31  phares
// First rev with no ^Ms
//
// Revision 1.2  1998/01/26  05:59:14  killough
// New PPro blit routine
//
// Revision 1.1.1.1  1998/01/19  14:02:50  rand
// Lee's Jan 19 sources
//
//----------------------------------------------------------------------------
