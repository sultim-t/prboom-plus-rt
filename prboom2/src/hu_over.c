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
 * Heads up 'overlay' for fullscreen
 *
 * Rewritten and put in a seperate module(seems sensible)
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>

#include "doomdef.h"
#include "doomstat.h"
#include "c_runcmd.h"
#include "d_deh.h"
#include "d_event.h"
#include "g_game.h"
#include "hu_frags.h"
#include "hu_over.h"
#include "hu_stuff.h"
#include "st_stuff.h"
//#include "p_info.h"
#include "p_map.h"
#include "p_setup.h"
#include "r_draw.h"
#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"


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

#define setol(o,a,b)          \
        {                     \
        overlay[o].x = (a);   \
        overlay[o].y = (b);   \
        }
#define HUDCOLOUR FC_GRAY

int hud_overlaystyle = 1;
int hud_enabled = 1;
int hud_hidestatus = 0;

///////////////////////////////////////////////////////////////////////
//
// Heads Up Font
//
// the utility code to draw the tiny heads-up
// font. Based on the code from v_video.c
// which draws the normal font.

// note to programmers from other ports: hu_font is the heads up font
// *not* the general doom font as it is in the original sources and
// most ports

patchnum_t hu_font[HU_FONTSIZE];

void HU_LoadFont()
{
  int i, j;
  char lumpname[10];

  memset(hu_font, 0, sizeof(hu_font));
  for(i=0, j=HU_FONTSTART; i<HU_FONTSIZE; i++, j++)
  {
    lumpname[0] = 0;
    if( (j>='0' && j<='9') || (j>='A' && j<='Z') )
	    sprintf(lumpname, "DIG%c", j);
    if(j==45 || j==47 || j==58 || j==91 || j==93)
	    sprintf(lumpname, "DIG%i", j);
    if(j>=123 && j<=127)
	    sprintf(lumpname, "STBR%i", j);
    if(j=='_') strcpy(lumpname, "DIG45");
    if(j=='(') strcpy(lumpname, "DIG91");
    if(j==')') strcpy(lumpname, "DIG93");
    
    if (lumpname[0])
      R_SetPatchNum(&hu_font[i], lumpname);
    else
      hu_font[i].lumpnum = -1;
  }
}

// sf: write a text line to x, y

void HU_WriteText(unsigned char *s, int x, int y)
{
  V_WriteTextFont(s, x, y, 0, hu_font);
}

void HU_WriteTextColoured(unsigned char *s, int colour, int x, int y)
{
  V_WriteTextFontColoured(s, colour, x, y, 0, hu_font);
}

// the width in pixels of a string in heads-up font

int HU_StringWidth(unsigned char *s)
{
  return V_StringWidthFont(s, 0, hu_font);
}

#define BARSIZE 15

// create a string containing the text 'bar' which graphically
// show %age of ammo/health/armor etc left

void HU_TextBar(unsigned char *s, int pct)
{
  if(pct > 100) pct = 100;
  
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

      s[strlen(s) + 1] = '\0';
      s[strlen(s)] = addchar;
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
  char tempstr[50];
  int fontcolour;
  
  HU_WriteText(HUDCOLOUR "Health", x, y);
  x += GAP;               // leave a gap between name and bar
  
  // decide on the colour first
  fontcolour =
    players[displayplayer].health < health_red ? *FC_RED :
    players[displayplayer].health < health_yellow ? *FC_GOLD :
    players[displayplayer].health <= health_green ? *FC_GREEN :
    *FC_BLUE;
  
  sprintf(tempstr, "%c", fontcolour);

  // now make the actual bar
  HU_TextBar(tempstr, players[displayplayer].health);

  // append the percentage itself
  sprintf(tempstr, "%s %i", tempstr, players[displayplayer].health);

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
  char tempstr[50];
  int fontcolour;
  
  // title first
  HU_WriteText(HUDCOLOUR "Armor", x, y);
  x += GAP;              // leave a gap between name and bar
  
  // decide on colour
  fontcolour =
    players[displayplayer].armorpoints < armor_red ? *FC_RED :
    players[displayplayer].armorpoints < armor_yellow ? *FC_GOLD :
    players[displayplayer].armorpoints <= armor_green ? *FC_GREEN :
    *FC_BLUE;
  sprintf(tempstr, "%c", fontcolour);
  
  // make the bar
  HU_TextBar(tempstr, players[displayplayer].armorpoints);
  
  // append the percentage itself
  sprintf(tempstr, "%s %i", tempstr,
	  players[displayplayer].armorpoints);
  
  HU_WriteText(tempstr, x, y);
}

////////////////////////////
//
// Drawing Ammo
//

void HU_DrawAmmo(int x, int y)
{
  char tempstr[50];
  int fontcolour;
  
  HU_WriteText(HUDCOLOUR "Ammo", x, y);
  x += GAP;
  
  fontcolour = weapcolour(players[displayplayer].readyweapon);
  sprintf(tempstr, "%c", fontcolour);
  
  if(playermaxammo)
    {
      HU_TextBar(tempstr, (100 * playerammo) / playermaxammo);
      sprintf(tempstr, "%s %i/%i", tempstr, playerammo, playermaxammo);
    }
  else    // fist or chainsaw
    strcat(tempstr, "N/A");
  
  HU_WriteText(tempstr, x, y);
}

//////////////////////////////
//
// Weapons List
//

void HU_DrawWeapons(int x, int y)
{
  char tempstr[50] = "";
  int i;
  int fontcolour;
  
  HU_WriteText(HUDCOLOUR "Weapons", x, y);    // draw then leave a gap
  x += GAP;
  
  for(i=0; i<NUMWEAPONS; i++)
    {
      if(players[displayplayer].weaponowned[i])
	{
	  // got it
	  fontcolour = weapcolour(i);
	  sprintf(tempstr, "%s%c%i ", tempstr,
		  fontcolour, i+1);
	}
    }

  HU_WriteText(tempstr, x, y);    // draw it
}

////////////////////////////////
//
// Draw the keys
//

extern patchnum_t keys[NUMCARDS+3];

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
	  V_DrawNumPatch(x, y, 0, keys[i].lumpnum, CR_DEFAULT, VPT_STRETCH);
	  x += 11;
	}
    }
}

///////////////////////////////
//
// Draw the Frags

void HU_DrawFrag(int x, int y)
{
  char tempstr[20];
  
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
  char tempstr[50];
  
  HU_WriteText(HUDCOLOUR "Status", x, y); // draw, leave a gap
  x += GAP;
  
  
  sprintf(tempstr,
	  FC_RED "K" FC_GREEN " %i "
	  FC_RED "M" FC_GREEN " %i "
	  FC_RED "I" FC_GREEN " %i/%i "
	  FC_RED "S" FC_GREEN " %i/%i ",
	  players[displayplayer].killcount, totallive,
	  players[displayplayer].itemcount, totalitems,
	  players[displayplayer].secretcount, totalsecret
	  );
  
  HU_WriteText(tempstr, x, y);
}

// all overlay modules
overlay_t overlay[NUMOVERLAY];

// toggle the overlay style

void HU_OverlayStyle()
{
  hud_enabled = true;
  hud_overlaystyle = (hud_overlaystyle+1) % 4;
}

void HU_ToggleHUD()
{
  hud_enabled = !hud_enabled;
}

void HU_OverlaySetup()
{
  int i;
  
  // setup the drawers
  overlay[ol_health].drawer = HU_DrawHealth;
  overlay[ol_ammo].drawer = HU_DrawAmmo;
  overlay[ol_weap].drawer = HU_DrawWeapons;
  overlay[ol_armor].drawer = HU_DrawArmor;
  overlay[ol_key].drawer = HU_DrawKeys;
  overlay[ol_frag].drawer = HU_DrawFrag;
  overlay[ol_status].drawer = HU_DrawStatus;

  // now decide where to put all the widgets

  for(i=0; i<NUMOVERLAY; i++)
    overlay[i].x = 1;       // turn em all on

  // turn off status if we aren't using it
  
  if(hud_hidestatus) overlay[ol_status].x = -1;

  // turn off frag counter or key display,
  // according to if we're in a deathmatch game or not
  
  if(deathmatch)
    overlay[ol_key].x = -1;
  else
    overlay[ol_frag].x = -1;

  // now build according to style

  switch(hud_overlaystyle)
    {
      // 0: off
      
      case 0:
	for(i=0; i<NUMOVERLAY; i++)
	  {
	    setol(i, -1, -1);       // turn it off
	  }
	break;

      // 1:'bottom left' style
	
      case 1:
	{
	  int y = 200-8;
	  
	  for(i=NUMOVERLAY-1; i >= 0; i--)
	    {
	      if(overlay[i].x != -1)
		{
		  setol(i, 0, y);
		  y -= 8;
		}
	    }
	  break;
	}

      // 2: all at bottom of screen
      
      case 2:
	{
	  int x = 0, y = 192;
	  
	  for(i=0; i<NUMOVERLAY; i++)
	    {
	      if(overlay[i].x != -1)
		{
		  setol(i, x, y);
		  x += 160;
		  if(x >= 300)
		    {
		      x = 0; y -=8;
		    }
		}
	    }
	  break;
	}

      // 3: similar to boom 'distributed' style

      case 3:
	setol(ol_health, 320-138, 0);
	setol(ol_armor, 320-138, 8);
	setol(ol_weap, 320-138, 184);
	setol(ol_ammo, 320-138, 192);
	if(deathmatch)  // if dm, put frags in place of keys
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

void HU_OverlayDraw()
{
  int i;
  
  if (viewheight != SCREENHEIGHT) return;  // fullscreen only
  if (automapmode & am_active) return;
  if (!hud_enabled) return;
  
  HU_OverlaySetup();
  
  for(i=0; i<NUMOVERLAY; i++)
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
};

VARIABLE_INT(hud_overlaystyle,  NULL,   0, 3,    str_style);
CONSOLE_VARIABLE(hu_overlay, hud_overlaystyle, 0) {}

VARIABLE_BOOLEAN(hud_hidestatus, NULL, yesno);
CONSOLE_VARIABLE(hu_hidesecrets, hud_hidestatus, 0) {}

void HU_OverAddCommands()
{
  C_AddCommand(hu_overlay);
  C_AddCommand(hu_hidesecrets);
}
