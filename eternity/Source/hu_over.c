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
//--------------------------------------------------------------------------
//
// Heads up 'overlay' for fullscreen
//
// Rewritten and put in a seperate module(seems sensible)
//
// By Simon Howard
//
//----------------------------------------------------------------------------

#include "z_zone.h"
#include "doomdef.h"
#include "doomstat.h"
#include "c_runcmd.h"
#include "d_deh.h"
#include "d_event.h"
#include "g_game.h"
#include "hu_frags.h"
#include "hu_over.h"
#include "hu_stuff.h"
#include "p_info.h"
#include "p_map.h"
#include "p_setup.h"
#include "r_draw.h"
#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"
#include "d_gi.h"
#include "v_font.h"


// internal for other defines:

#define _wc_pammo(w)    ( weaponinfo[w].ammo == am_noammo ? 0 : \
                          players[displayplayer].ammo[weaponinfo[w].ammo])
#define _wc_mammo(w)    ( weaponinfo[w].ammo == am_noammo ? 0 : \
                          players[displayplayer].maxammo[weaponinfo[w].ammo])


#define weapcolour(w)   ( !_wc_mammo(w) ? *FC_GRAY :    \
        _wc_pammo(w) < ( (_wc_mammo(w) * ammo_red) / 100) ? *FC_RED :    \
        _wc_pammo(w) < ( (_wc_mammo(w) * ammo_yellow) / 100) ? *FC_GOLD : \
                          *FC_GREEN );

// the amount of ammo the displayplayer has left 

#define playerammo      \
        _wc_pammo(players[displayplayer].readyweapon)

// the maximum amount the player could have for current weapon

#define playermaxammo   \
        _wc_mammo(players[displayplayer].readyweapon)

// set up an overlay_t

#define setol(o,a,b) \
   { \
      overlay[o].x = (a); \
      overlay[o].y = (b); \
   }

#define HUDCOLOUR FC_GRAY

int hud_overlaystyle = 1;
int hud_enabled = 1;
int hud_hidestatus = 0;

///////////////////////////////////////////////////////////////////////
//
// Heads Up Font
//

// note to programmers from other ports: hu_font is the heads up font
// *not* the general doom font as it is in the original sources and
// most ports

#define HU_FONTSTART    '!'    /* the first font characters */
#define HU_FONTEND      (0x7f) /*jff 2/16/98 '_' the last font characters */

// Calculate # of glyphs in font.
#define HU_FONTSIZE     (HU_FONTEND - HU_FONTSTART + 1) 

static patch_t *hu_font[HU_FONTSIZE];
static boolean hu_fontloaded = false;

// haleyjd 01/14/05: new vfont object for HUD font

static vfont_t hud_font = 
{
   HU_FONTSTART, // first character
   HU_FONTEND,   // last character
   HU_FONTSIZE,  // size of font

   8,            // linebreak size
   4,            // space size
   0,            // char width delta
   8,            // max character height

   true,         // color enabled
   true,         // caps only
   false,        // no centering

   hu_font,      // patch array
};

//
// HU_LoadFont
//
// Loads the heads-up font. The naming scheme for the lumps is
// not very consistent, so this is relatively complicated.
//
void HU_LoadFont(void)
{
   int i, j;
   char lumpname[9];

   for(i = 0, j = HU_FONTSTART; i < HU_FONTSIZE; i++, j++)
   {
      lumpname[0] = 0;
      if((j >= '0' && j <= '9') || (j >= 'A' && j <= 'Z') )
         sprintf(lumpname, "DIG%c", j);
      if(j == 45 || j == 47 || j == 58 || j == 91 || j == 93)
         sprintf(lumpname, "DIG%i", j);
      if(j >=123 && j <= 127)
         sprintf(lumpname, "STBR%i", j);
      if(j=='_')
         strcpy(lumpname, "DIG45");
      if(j=='(')
         strcpy(lumpname, "DIG91");
      if(j==')')
         strcpy(lumpname, "DIG93");
      
      hu_font[i] = lumpname[0] ? W_CacheLumpName(lumpname, PU_STATIC) : NULL;
   }

   hu_fontloaded = true;
}

//
// HU_WriteText
//
// sf: write a text line to x, y
// haleyjd 01/14/05: now uses vfont engine
//
void HU_WriteText(const char *s, int x, int y)
{
   if(hu_fontloaded)
      V_FontWriteText(&hud_font, s, x, y);
}

//
// HU_StringWidth
//
// Calculates the width in pixels of a string in heads-up font
// haleyjd 01/14/05: now uses vfont engine
//
int HU_StringWidth(const unsigned char *s)
{
   return V_FontStringWidth(&hud_font, s);
}

//
// HU_StringHeight
//
// Calculates the height in pixels of a string in heads-up font
// haleyjd 01/14/05: now uses vfont engine
//
int HU_StringHeight(const unsigned char *s)
{
   return V_FontStringHeight(&hud_font, s);
}

#define BARSIZE 15

// create a string containing the text 'bar' which graphically
// show %age of ammo/health/armor etc left

static void HU_TextBar(char *s, size_t len, int pct)
{
   if(pct > 100)
      pct = 100;
  
   // build the string, decide how many blocks
   while(pct)
   {
      int addchar = 0;
      
      if(pct >= BARSIZE)
      {
         addchar = 123;  // full pct: 4 blocks
         pct -= BARSIZE;
      }
      else
      {
         addchar = 127 - (pct*5)/BARSIZE;
         pct = 0;
      }
      psnprintf(s, len, "%s%c", s, addchar);
   }
}

/////////////////////////////////////////////////////////////////////////
//
// Drawer
//
// the actual drawer is the heart of the overlay
// code. It is split into individual functions,
// each of which draws a different part.

// the offset of percentage bars from the starting text
#define GAP 40

/////////////////////////
//
// draw health
//

void HU_DrawHealth(int x, int y)
{
   char tempstr[128];
   int fontcolour;
   
   memset(tempstr, 0, 128);

   HU_WriteText(HUDCOLOUR "Health", x, y);
   x += GAP;               // leave a gap between name and bar
   
   // decide on the colour first
   fontcolour =
      players[displayplayer].health < health_red ? *FC_RED :
      players[displayplayer].health < health_yellow ? *FC_GOLD :
      players[displayplayer].health <= health_green ? *FC_GREEN :
      *FC_BLUE;
  
   psnprintf(tempstr, sizeof(tempstr), "%c", fontcolour);

   // now make the actual bar
   HU_TextBar(tempstr, sizeof(tempstr), players[displayplayer].health);
   
   // append the percentage itself
   psnprintf(tempstr, sizeof(tempstr), 
             "%s %i", tempstr, players[displayplayer].health);
   
   // write it
   HU_WriteText(tempstr, x, y);
}

/////////////////////////////
//
// Draw Armour.
// very similar to drawhealth.
//

void HU_DrawArmor(int x, int y)
{
  char tempstr[128];
  int fontcolour;

  memset(tempstr, 0, 128);

  // title first
  HU_WriteText(HUDCOLOUR "Armor", x, y);
  x += GAP;              // leave a gap between name and bar
  
  // decide on colour
  fontcolour =
    players[displayplayer].armorpoints < armor_red ? *FC_RED :
    players[displayplayer].armorpoints < armor_yellow ? *FC_GOLD :
    players[displayplayer].armorpoints <= armor_green ? *FC_GREEN :
    *FC_BLUE;
  psnprintf(tempstr, sizeof(tempstr), "%c", fontcolour);
  
  // make the bar
  HU_TextBar(tempstr, sizeof(tempstr), 
             players[displayplayer].armorpoints);
  
  // append the percentage itself
  psnprintf(tempstr, sizeof(tempstr), "%s %i", tempstr,
	    players[displayplayer].armorpoints);
  
  HU_WriteText(tempstr, x, y);
}

////////////////////////////
//
// Drawing Ammo
//

void HU_DrawAmmo(int x, int y)
{
   char tempstr[128];
   int fontcolour;
   
   memset(tempstr, 0, 128);
   
   HU_WriteText(HUDCOLOUR "Ammo", x, y);
   x += GAP;
   
   fontcolour = weapcolour(players[displayplayer].readyweapon);
   psnprintf(tempstr, sizeof(tempstr), "%c", fontcolour);
   
   if(playermaxammo)
   {
      HU_TextBar(tempstr, sizeof(tempstr), 
                 (100 * playerammo) / playermaxammo);
      psnprintf(tempstr, sizeof(tempstr), "%s %i/%i", 
                tempstr, playerammo, playermaxammo);
   }
   else    // fist or chainsaw
      psnprintf(tempstr, sizeof(tempstr), "%sN/A", tempstr);
   
   HU_WriteText(tempstr, x, y);
}

//////////////////////////////
//
// Weapons List
//

void HU_DrawWeapons(int x, int y)
{
   char tempstr[128];
   int i;
   int fontcolour;
   
   memset(tempstr, 0, 128);
   
   HU_WriteText(HUDCOLOUR "Weapons", x, y);    // draw then leave a gap
   x += GAP;
  
   for(i=0; i<NUMWEAPONS; i++)
   {
      if(players[displayplayer].weaponowned[i])
      {
         // got it
         fontcolour = weapcolour(i);
         psnprintf(tempstr, sizeof(tempstr), "%s%c%i ", tempstr,
                   fontcolour, i+1);
      }
   }
   
   HU_WriteText(tempstr, x, y);    // draw it
}

////////////////////////////////
//
// Draw the keys
//

extern patch_t *keys[NUMCARDS+3];

void HU_DrawKeys(int x, int y)
{
  int i;
  
  HU_WriteText(HUDCOLOUR "Keys", x, y);    // draw then leave a gap
  x += GAP;
  
  for(i=0; i<NUMCARDS; i++)
    {
      if(players[displayplayer].cards[i])
	{
	  // got that key
	  V_DrawPatch(x, y, &vbscreen, keys[i]);
	  x += 11;
	}
    }
}

///////////////////////////////
//
// Draw the Frags

void HU_DrawFrag(int x, int y)
{
  char tempstr[64];

  memset(tempstr, 0, 64);

  HU_WriteText(HUDCOLOUR "Frags", x, y);    // draw then leave a gap
  x += GAP;
  
  sprintf(tempstr, HUDCOLOUR "%i", players[displayplayer].totalfrags);
  HU_WriteText(tempstr, x, y);        
}

///////////////////////////////////////
//
// draw the status (number of kills etc)
//

void HU_DrawStatus(int x, int y)
{
  char tempstr[128];

  memset(tempstr, 0, 128);

  HU_WriteText(HUDCOLOUR "Status", x, y); // draw, leave a gap
  x += GAP;
  
  psnprintf(tempstr, sizeof(tempstr),
	    FC_RED "K" FC_GREEN " %i/%i "
	    FC_RED "I" FC_GREEN " %i/%i "
	    FC_RED "S" FC_GREEN " %i/%i ",
	    players[displayplayer].killcount, totalkills,
	    players[displayplayer].itemcount, totalitems,
	    players[displayplayer].secretcount, totalsecret
	   );
  
  HU_WriteText(tempstr, x, y);
}

// all overlay modules
overlay_t overlay[NUMOVERLAY];

void HU_ToggleHUD(void)
{
   hud_enabled = !hud_enabled;
}

void HU_DisableHUD(void)
{
   hud_enabled = false;
}

void HU_OverlaySetup(void)
{
   int i;
   
   // setup the drawers
   overlay[ol_health].drawer = HU_DrawHealth;
   overlay[ol_ammo].drawer   = HU_DrawAmmo;
   overlay[ol_weap].drawer   = HU_DrawWeapons;
   overlay[ol_armor].drawer  = HU_DrawArmor;
   overlay[ol_key].drawer    = HU_DrawKeys;
   overlay[ol_frag].drawer   = HU_DrawFrag;
   overlay[ol_status].drawer = HU_DrawStatus;

   // now decide where to put all the widgets
   
   for(i = 0; i < NUMOVERLAY; ++i)
      overlay[i].x = 1;       // turn em all on

   // turn off status if we aren't using it
   if(hud_hidestatus)
      overlay[ol_status].x = -1;

   // turn off frag counter or key display,
   // according to if we're in a deathmatch game or not
   if(GameType == gt_dm)
      overlay[ol_key].x = -1;
   else
      overlay[ol_frag].x = -1;

   // now build according to style
   
   switch(hud_overlaystyle)
   {      
   case 0: // 0: 'off'
   case 4: // 4: 'graphical' -- haleyjd 01/11/05: this is handled by status bar
      for(i = 0; i < NUMOVERLAY; ++i)
      {
         setol(i, -1, -1); // turn it off
      }
      break;
      
   case 1: // 1:'bottom left' style
      {
         int y = SCREENHEIGHT - 8;
         
         for(i = NUMOVERLAY - 1; i >= 0; --i)
         {
            if(overlay[i].x != -1)
            {
               setol(i, 0, y);
               y -= 8;
            }
         }
      }
      break;
      
   case 2: // 2: all at bottom of screen
      {
         int x = 0, y = 192;
         
         for(i = 0; i < NUMOVERLAY; ++i)
         {
            if(overlay[i].x != -1)
            {
               // haleyjd: swap health & armor, keep rest the same
               int idx = i;
               if(idx == ol_health)
                  idx = ol_armor;
               else if(idx == ol_armor)
                  idx = ol_health;

               setol(idx, x, y);
               x += 160;
               if(x >= 300)
               {
                  x = 0; 
                  y -=8;
               }
            }
         }
      }
      break;

   case 3: // 3: similar to boom 'distributed' style
      setol(ol_health, SCREENWIDTH-138, 0);
      setol(ol_armor, SCREENWIDTH-138, 8);
      setol(ol_weap, SCREENWIDTH-138, 184);
      setol(ol_ammo, SCREENWIDTH-138, 192);
      if(GameType == gt_dm)  // if dm, put frags in place of keys
      {
         setol(ol_frag, 0, 192);
      }
      else
      {
         setol(ol_key, 0, 192);
      }
      if(!hud_hidestatus)
         setol(ol_status, 0, 184);
      break;
   }
}

////////////////////////////////////////////////////////////////////////
//
// heart of the overlay really.
// draw the overlay, deciding which bits to draw and where

void HU_OverlayDraw(void)
{
   int i;
   
   // SoM 2-4-04: ANYRES
   if(viewheight != v_height || automapactive || !hud_enabled) 
      return;  // fullscreen only
  
   HU_OverlaySetup();
   
   for(i = 0; i < NUMOVERLAY; ++i)
   {
      if(overlay[i].x != -1)
         overlay[i].drawer(overlay[i].x, overlay[i].y);
   }
}

char *str_style[] =
{
   "off",
   "boom style",
   "flat",
   "distributed",
   "graphical",   // haleyjd 01/11/05
};

VARIABLE_INT(hud_overlaystyle,  NULL,   0, 4,    str_style);
CONSOLE_VARIABLE(hu_overlay, hud_overlaystyle, 0) {}

VARIABLE_BOOLEAN(hud_hidestatus, NULL, yesno);
CONSOLE_VARIABLE(hu_hidesecrets, hud_hidestatus, 0) {}

void HU_OverAddCommands()
{
  C_AddCommand(hu_overlay);
  C_AddCommand(hu_hidesecrets);
}

// EOF
