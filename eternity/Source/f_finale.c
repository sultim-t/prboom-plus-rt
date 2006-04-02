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
//--------------------------------------------------------------------------
//
// DESCRIPTION:
//      Game completion, final screen animation.
//
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id: f_finale.c,v 1.16 1998/05/10 23:39:25 killough Exp $";

#include "z_zone.h"
#include "i_video.h"
#include "doomstat.h"
#include "d_event.h"
#include "v_video.h"
#include "w_wad.h"
#include "s_sound.h"
#include "sounds.h"
#include "dstrings.h"
#include "mn_engin.h"
#include "d_deh.h"  // Ty 03/22/98 - externalizations
#include "p_info.h"
#include "d_gi.h"
#include "c_io.h"
#include "f_finale.h"
#include "e_edf.h"

// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast
int finalestage;
int finalecount;

// defines for the end mission display text                     // phares

#define TEXTSPEED    3     // original value                    // phares
#define TEXTWAIT     250   // original value                    // phares
#define NEWTEXTSPEED 0.01  // new value                         // phares
#define NEWTEXTWAIT  1000  // new value                         // phares

void    F_StartCast (void);
void    F_CastTicker (void);
boolean F_CastResponder (event_t *ev);
void    F_CastDrawer (void);

void IN_checkForAccelerate(void);    // killough 3/28/98: used to
extern int acceleratestage;          // accelerate intermission screens
static int midstage;                 // whether we're in "mid-stage"

byte *DemonBuffer; // haleyjd 08/23/02

//
// F_StartFinale
//
void F_StartFinale(void)
{
   gameaction = ga_nothing;
   gamestate = GS_FINALE;
   automapactive = false;
   
   // killough 3/28/98: clear accelerative text flags
   acceleratestage = midstage = 0;

   // haleyjd 07/17/04: level-dependent initialization moved to LevelInfo

   S_ChangeMusicName(LevelInfo.interMusic, true);
   
   finalestage = 0;
   finalecount = 0;
}



boolean F_Responder (event_t *event)
{
   if(finalestage == 2)
      return F_CastResponder(event);
   
   // haleyjd: Heretic underwater hack for E2 end
   if(finalestage == 3 && event->type == ev_keydown)
   {
      // restore normal palette and kick out to title screen
      finalestage = 4;
      I_SetPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));
      return true;
   }
   
   return false;
}

// Get_TextSpeed() returns the value of the text display speed  // phares
// Rewritten to allow user-directed acceleration -- killough 3/28/98

static float Get_TextSpeed(void)
{
   return 
      (float)(midstage ? NEWTEXTSPEED : 
              (midstage=acceleratestage) ? 
               acceleratestage=0, NEWTEXTSPEED : TEXTSPEED);
}


//
// F_Ticker
//
// killough 3/28/98: almost totally rewritten, to use
// player-directed acceleration instead of constant delays.
// Now the player can accelerate the text display by using
// the fire/use keys while it is being printed. The delay
// automatically responds to the user, and gives enough
// time to read.
//
// killough 5/10/98: add back v1.9 demo compatibility
// haleyjd 10/12/01: reformatted, added cast call for any level
//
void F_Ticker(void)
{
   int i;

   if(!demo_compatibility)
   {
      // killough 3/28/98: check for acceleration
      IN_checkForAccelerate();
   }
   else if(gamemode == commercial && finalecount > 50)
   {  
      // check for skipping
      for(i = 0; i < MAXPLAYERS; ++i)
	 if(players[i].cmd.buttons)
	    goto next_level;      // go on to the next level
   }

   // advance animation
   finalecount++;
   
   if(finalestage == 2)
      F_CastTicker();

   if(!finalestage)
   {
      float speed = demo_compatibility ? TEXTSPEED : Get_TextSpeed();

      if(finalecount > strlen(LevelInfo.interText)*speed + // phares
	 (midstage ? NEWTEXTWAIT : TEXTWAIT) ||   // killough 2/28/98:
	 (midstage && acceleratestage))           // changed to allow acceleration
      {
	 if(gamemode != commercial) // Doom 1 / Ultimate Doom episode end
	 {                          // with enough time, it's automatic
            finalecount = 0;
            finalestage = 1;

            if(!(gamemode == hereticreg && 
                (gameepisode == 2 || gameepisode == 3)))
               wipegamestate = -1;     // force a wipe

            if(gameepisode == 3)
            {
               if(gamemode == hereticreg)
               {
                  DemonBuffer = Z_Malloc(128000, PU_LEVEL,
                                         (void **)(&DemonBuffer));
                  W_ReadLump(W_GetNumForName("FINAL2"), DemonBuffer);
                  W_ReadLump(W_GetNumForName("FINAL1"),
                             DemonBuffer+64000);
               }
               else
                  S_StartMusic(mus_bunny);
            }
	 }
	 else if(!demo_compatibility && midstage)
	 { 
	    // you must press a button to continue in Doom 2
	    // haleyjd: allow cast calls after arbitrary maps
	 next_level:
	    if(LevelInfo.endOfGame)
	       F_StartCast(); // cast of Doom 2 characters
	    else
	       gameaction = ga_worlddone;  // next level, e.g. MAP07
	 }
      }
   }
}

//
// F_TextWrite
//
// This program displays the background and text at end-mission     // phares
// text time. It draws both repeatedly so that other displays,      //   |
// like the main menu, can be drawn over it dynamically and         //   V
// erased dynamically. The TEXTSPEED constant is changed into
// the Get_TextSpeed function so that the speed of writing the      //   ^
// text can be increased, and there's still time to read what's     //   |
// written.                                                         // phares
//
void F_TextWrite(void)
{
   int         w;         // killough 8/9/98: move variables below
   int         count;
   char*       ch;
   int         c;
   int         cx;
   int         cy;
   int         lumpnum;
   
   // haleyjd: get finale font metrics
   gitextmetric_t *fontmetrics = gameModeInfo->ftextinfo;

   // erase the entire screen to a tiled background
   
   // killough 11/98: the background-filling code was already in m_menu.c

   lumpnum = W_CheckNumForName(LevelInfo.backDrop);
   
   if(lumpnum == -1) // flat
      V_DrawBackground(LevelInfo.backDrop, &vbscreen);
   else
   {                     // normal picture
      patch_t *pic;
      
      pic = W_CacheLumpNum(lumpnum, PU_CACHE);
      V_DrawPatch(0, 0, &vbscreen, pic);
   }

   // draw some of the text onto the screen
   cx = fontmetrics->x;
   cy = fontmetrics->y;
   ch = LevelInfo.interText;
      
   count = (int)((finalecount - 10)/Get_TextSpeed()); // phares
   if(count < 0)
      count = 0;

   for(; count; count--)
   {
      if(!(c = *ch++))
         break;
      
      if(c == '\n')
      {
         cx = fontmetrics->x;
         cy += fontmetrics->cy;
         continue;
      }
      
      // haleyjd: added null pointer check
      c = toupper(c) - V_FONTSTART;

      if(c < 0 || c > V_FONTSIZE || !v_font[c])
      {
         cx += fontmetrics->space;
         continue;
      }
      
      w = SHORT(v_font[c]->width);
      if(cx + w > SCREENWIDTH)
         continue; // haleyjd: continue, not break

      V_DrawPatch(cx, cy, &vbscreen, v_font[c]);
      
      cx += w;
   }
}

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//

// haleyjd 07/05/03: modified to be dynamic through EDF

// define MAX_CASTORDER 18 /* Ty - hard coded for now */
// castinfo_t      castorder[MAX_CASTORDER]; 
int             max_castorder;
castinfo_t      *castorder; // Ty 03/22/98 - externalized and init moved into f_startcast()

int             castnum;
int             casttics;
state_t*        caststate;
boolean         castdeath;
int             castframes;
int             castonmelee;
boolean         castattacking;

extern  gamestate_t     wipegamestate;

// haleyjd 07/05/03: support old DEH names for the first
// 17 cast members, for compatibility purposes. Note that
// oldnames is an array of pointers-to-pointers, to support
// DeHackEd replacement properly.
#define OLDCASTMAX 17
static char **oldnames[OLDCASTMAX] =
{
   &s_CC_ZOMBIE,
   &s_CC_SHOTGUN,
   &s_CC_HEAVY,
   &s_CC_IMP,
   &s_CC_DEMON,
   &s_CC_LOST,
   &s_CC_CACO,
   &s_CC_HELL,
   &s_CC_BARON,
   &s_CC_ARACH,
   &s_CC_PAIN,
   &s_CC_REVEN,
   &s_CC_MANCU,
   &s_CC_ARCH,
   &s_CC_SPIDER,
   &s_CC_CYBER,
   &s_CC_HERO,
};

//
// F_StartCast
//
// haleyjd 07/05/03: rewritten for EDF support
//
void F_StartCast(void)
{
   int i;

   // types are now set through EDF
   
   // if a cast name was left NULL by EDF, it means we're going to
   // use the old DeHackEd names
   for(i = 0; i < 17; i++)
   {
      if(!castorder[i].name)
         castorder[i].name = *(oldnames[i]); // array of ptr-to-ptrs (!)
   }

   wipegamestate = -1;         // force a screen wipe
   castnum = 0;
   caststate = &states[mobjinfo[castorder[castnum].type].seestate];
   casttics = caststate->tics;
   castdeath = false;
   finalestage = 2;    
   castframes = 0;
   castonmelee = 0;
   castattacking = false;
   S_ChangeMusicNum(mus_evil, true);
}

//
// F_CastTicker
//
void F_CastTicker(void)
{
   int st;
   int sfx;
   
   if(--casttics > 0)
      return;                 // not time to change state yet
              
   if(caststate->tics == -1 || caststate->nextstate == E_NullState())
   {
      // switch from deathstate to next monster
      castnum++;
      castdeath = false;
      if(castorder[castnum].name == NULL)
         castnum = 0;
      S_StartSound (NULL, mobjinfo[castorder[castnum].type].seesound);
      caststate = &states[mobjinfo[castorder[castnum].type].seestate];
      castframes = 0;
   }
   else
   {      
      // just advance to next state in animation

      // haleyjd: modified to use a field set through EDF
      //if(caststate == &states[S_PLAY_ATK1])
      //   goto stopattack;    // Oh, gross hack!

      int i;
      int statenum = mobjinfo[castorder[castnum].type].missilestate;

      if(caststate == &states[statenum] && castorder[castnum].stopattack)
         goto stopattack; // not quite as hackish as it used to be

      st = caststate->nextstate;
      caststate = &states[st];
      castframes++;

      // haleyjd: new sound event method -- each actor type
      // can define up to four sound events.
      
      // Search for a sound matching this state.
      sfx = 0;
      for(i = 0; i < 4; i++)
      {
         if(st == castorder[castnum].sounds[i].frame)
         {
            sfx = castorder[castnum].sounds[i].sound;
         }
      }
      
      S_StartSound(NULL, sfx);
   }
      
   if(castframes == 12)
   {
      int i, stnum;

      // go into attack frame
      castattacking = true;
      if(castonmelee)
         caststate=&states[(stnum = mobjinfo[castorder[castnum].type].meleestate)];
      else
         caststate=&states[(stnum = mobjinfo[castorder[castnum].type].missilestate)];
      castonmelee ^= 1;
      if(caststate == &states[E_NullState()])
      {
         if(castonmelee)
            caststate=
            &states[(stnum = mobjinfo[castorder[castnum].type].meleestate)];
         else
            caststate=
            &states[(stnum = mobjinfo[castorder[castnum].type].missilestate)];
      }

      // haleyjd 07/04/04: check for sounds matching the missile or
      // melee state
      if(!castorder[castnum].stopattack)
      {
         sfx = 0;
         for(i = 0; i < 4; i++)
         {
            if(stnum == castorder[castnum].sounds[i].frame)
            {
               sfx = castorder[castnum].sounds[i].sound;
            }
         }
         
         S_StartSound(NULL, sfx);
      }
   }
      
   if(castattacking)
   {
      if(castframes == 24
         ||  caststate == &states[mobjinfo[castorder[castnum].type].seestate] )
      {
         stopattack:
         castattacking = false;
         castframes = 0;
         caststate = &states[mobjinfo[castorder[castnum].type].seestate];
      }
   }
      
   casttics = caststate->tics;
   if(casttics == -1)
      casttics = 15;
}


//
// F_CastResponder
//

boolean F_CastResponder (event_t* ev)
{
  if (ev->type != ev_keydown)
    return false;
                
  if (castdeath)
    return true;                    // already in dying frames
                
  // go into death frame
  castdeath = true;
  caststate = &states[mobjinfo[castorder[castnum].type].deathstate];
  casttics = caststate->tics;
  castframes = 0;
  castattacking = false;
  if(mobjinfo[castorder[castnum].type].deathsound)
  {
    if(mobjinfo[castorder[castnum].type].dehnum == MT_PLAYER)
      S_StartSoundName(NULL, 
                       players[displayplayer].skin->sounds[sk_pldeth]);
    else
      S_StartSound(NULL, mobjinfo[castorder[castnum].type].deathsound);
  }
        
  return true;
}


void F_CastPrint (char* text)
{
  char*       ch;
  int         c;
  int         cx;
  int         w;
  int         width;
  
  // find width
  ch = text;
  width = 0;
      
  while (ch)
  {
    c = *ch++;
    if (!c)
      break;
    // haleyjd: added null pointer check
    c = toupper(c) - V_FONTSTART;
    if (c < 0 || c> V_FONTSIZE || !v_font[c])
    {
      width += 4;
      continue;
    }
            
    w = SHORT (v_font[c]->width);
    width += w;
  }
  
  // draw it
  cx = 160-width/2;
  ch = text;
  while (ch)
  {
    c = *ch++;
    if (!c)
      break;
    c = toupper(c) - V_FONTSTART;
    if (c < 0 || c> V_FONTSIZE)
    {
      cx += 4;
      continue;
    }
              
    w = SHORT (v_font[c]->width);
    V_DrawPatch(cx, 180, &vbscreen, v_font[c]);
    cx+=w;
  }
}


//
// F_CastDrawer
//

void F_CastDrawer (void)
{
  spritenum_t         altsprite;
  spritedef_t*        sprdef;
  spriteframe_t*      sprframe;
  int                 lump;
  boolean             flip;
  patch_t*            patch;
    
  // erase the entire screen to a background
  V_DrawPatch (0,0,&vbscreen, W_CacheLumpName (bgcastcall, PU_CACHE)); // Ty 03/30/98 bg texture extern

  F_CastPrint (castorder[castnum].name);
    
  // draw the current frame in the middle of the screen
  sprdef = sprites + caststate->sprite;

  // override for alternate monster sprite?
  if((altsprite = mobjinfo[castorder[castnum].type].altsprite) != NUMSPRITES)
     sprdef = &sprites[altsprite];
  
  // override for player skin?
  if(mobjinfo[castorder[castnum].type].dehnum == MT_PLAYER)
        sprdef = &sprites[players[displayplayer].skin->sprite];

  // haleyjd 08/15/02
  if(!(sprdef->spriteframes))
     return;

  sprframe = &sprdef->spriteframes[caststate->frame & FF_FRAMEMASK];
  lump = sprframe->lump[0];
  flip = (boolean)sprframe->flip[0];
                        
  patch = W_CacheLumpNum (lump+firstspritelump, PU_CACHE);
  if (flip)
    V_DrawPatchFlipped (160,170,&vbscreen,patch);
  else
    V_DrawPatch (160,170,&vbscreen,patch);
}


//
// F_DrawPatchCol
//

static void F_DrawPatchCol(int x, patch_t *patch, int col)
{
  const column_t *column = 
    (const column_t *)((byte *) patch + LONG(patch->columnofs[col]));

  // step through the posts in a column
  // SoM 2-4-04: ANYRES
  /*if (hires)
    while (column->topdelta != 0xff)
      {
	byte *desttop = screens[0] + x*2;
	const byte *source = (byte *) column + 3;
	byte *dest = desttop + column->topdelta*SCREENWIDTH*4;
	int count = column->length;
	for (;count--; dest += SCREENWIDTH*4)
	  dest[0] = dest[SCREENWIDTH*2] = dest[1] = dest[SCREENWIDTH*2+1] = 
	    *source++;
	column = (column_t *)(source+1);
      }
  else*/
   // SoM: ANYRES
   if(globalyscale > FRACUNIT)
   {
      byte *desttop = screens[0] + x;

      while (column->topdelta != 0xff)
      {
         register const byte *source = (byte *) column + 3;
         register byte *dest = desttop + realyarray[column->topdelta] * v_width;
         register int count = realyarray[column->length];
         register fixed_t frac;
         fixed_t step;

         frac = 0;
         step = globaliyscale;

         for (;count--; dest += v_width)
         {
            *dest = source[frac >> FRACBITS];
            frac += step;
         }

         column = (column_t *)(source + column->length + 1);
      }
   }
   else
   {
      while (column->topdelta != 0xff)
      {
         byte *desttop = screens[0] + x;
         const byte *source = (byte *) column + 3;
         byte *dest = desttop + column->topdelta*SCREENWIDTH;
         int count = column->length;
         for (;count--; dest += SCREENWIDTH)
            *dest = *source++;
         column = (column_t *)(source+1);
      }
   }
}

//
// F_BunnyScroll
//
void F_BunnyScroll (void)
{
  int         scrolled;
  int         x;
  patch_t*    p1;
  patch_t*    p2;
  char        name[10];
  int         stage;
  static int  laststage;
              
  p1 = W_CacheLumpName ("PFUB2", PU_LEVEL);
  p2 = W_CacheLumpName ("PFUB1", PU_LEVEL);

  V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
      
  scrolled = 320 - (finalecount-230)/2;
  if (scrolled > 320)
      scrolled = 320;
  if (scrolled < 0)
      scrolled = 0;
              
   // ANYRES
  for ( x = 0 ; x < v_width ; x++)
  {
    int scaledx = (x * globaliyscale) >> FRACBITS;

    if (scaledx+scrolled < 320)
      F_DrawPatchCol (x, p1, scaledx+scrolled);
    else
      F_DrawPatchCol (x, p2, scaledx+scrolled - 320);           
  }
      
  if (finalecount < 1130)
    return;
  if (finalecount < 1180)
  {
    V_DrawPatch ((SCREENWIDTH-13*8)/2,
                 (SCREENHEIGHT-8*8)/2,&vbscreen, 
                 W_CacheLumpName ("END0",PU_CACHE));
    laststage = 0;
    return;
  }
      
  stage = (finalecount-1180) / 5;
  if (stage > 6)
    stage = 6;
  if (stage > laststage)
  {
    S_StartSound (NULL, sfx_pistol);
    laststage = stage;
  }
      
  sprintf(name,"END%i", stage);
  V_DrawPatch ((SCREENWIDTH-13*8)/2, 
               (SCREENHEIGHT-8*8)/2,&vbscreen, 
               W_CacheLumpName (name,PU_CACHE));
}

// haleyjd: heretic e2 ending -- sort of hackish
void F_DrawUnderwater(void)
{
   switch(finalestage)
   {
   case 1:
      C_InstaPopup(); // put away console if down
      
      {
         byte *palette;

         palette = W_CacheLumpName("E2PAL", PU_CACHE);
         I_SetPalette(palette);

         V_DrawBlock(0,0,&vbscreen,SCREENWIDTH,SCREENHEIGHT,
                     W_CacheLumpName("E2END", PU_CACHE));
         finalestage = 3;
      }
      // fall through
   case 3:
      console_enabled = false; // let console key fall through
      paused = false;
      menuactive = false;
      break;
   
   case 4:
      console_enabled = true;
      V_DrawBlock(0,0,&vbscreen,SCREENWIDTH,SCREENHEIGHT,
                  W_CacheLumpName("TITLE", PU_CACHE));
      break;
   }
}

// haleyjd: Heretic episode 3 demon scroller
void F_DemonScroll(void)
{
   static int yval = 0;
   static int nextscroll = 0;

   // show first screen for a while
   if(finalecount < 70)
   {
      V_DrawBlock(0,0,&vbscreen,SCREENWIDTH,SCREENHEIGHT,DemonBuffer+64000);
      nextscroll = finalecount;
      yval = 0;
      return;
   }

   if(yval < 64000)
   {
      // scroll up one line at a time until only the top screen
      // shows
      V_DrawBlock(0,0,&vbscreen,SCREENWIDTH,SCREENHEIGHT,
                  DemonBuffer + 64000 - yval);
      
      if(finalecount >= nextscroll)
      {
         yval += 320; // move up one line
         nextscroll = finalecount + 3; // don't scroll too fast
      }
   }
   else
   {
      // finished scrolling
      V_DrawBlock(0,0,&vbscreen,SCREENWIDTH,SCREENHEIGHT,DemonBuffer);
   }
}

//
// F_DoomDrawer
//
// Drawer function for DOOM gamemode finales.
//
static void F_DoomDrawer(void)
{
   switch(gameepisode)
   {
   case 1:
      if(gamemode == retail)
         V_DrawPatch(0,0,&vbscreen,W_CacheLumpName("CREDIT",PU_CACHE));
      else
         V_DrawPatch(0,0,&vbscreen,W_CacheLumpName("HELP2",PU_CACHE));
      break;
   case 2:
      V_DrawPatch(0,0,&vbscreen,W_CacheLumpName("VICTORY2",PU_CACHE));
      break;
   case 3:
      F_BunnyScroll();
      break;
   case 4:
      V_DrawPatch(0,0,&vbscreen,W_CacheLumpName("ENDPIC",PU_CACHE));
      break;
   }
}

//
// F_HticDrawer
//
// Drawer function for Heretic finales.
//
static void F_HticDrawer(void)
{
   switch(gameepisode)
   {
   case 1:
      if(gamemode == hereticsw)
         V_DrawBlock(0,0,&vbscreen,SCREENWIDTH,SCREENHEIGHT,
                     W_CacheLumpName("ORDER", PU_CACHE));
      else
         V_DrawBlock(0,0,&vbscreen,SCREENWIDTH,SCREENHEIGHT,
                     W_CacheLumpName("CREDIT", PU_CACHE));
      break;
   case 2:
      F_DrawUnderwater();
      break;
   case 3:
      F_DemonScroll();
      break;
   default: // episodes 4 and 5 fall in this catagory
      V_DrawBlock(0,0,&vbscreen,SCREENWIDTH,SCREENHEIGHT,
                  W_CacheLumpName("CREDIT", PU_CACHE));
      break;
   }
}

typedef void (*fdrawer_t)(void);

static fdrawer_t FDrawers[NumGameModeTypes] =
{
   F_DoomDrawer,
   F_HticDrawer
};

//
// F_Drawer
//
// Main finale drawing routine.
// Either runs a text mode finale, draws the DOOM II cast, or calls
// the current gamemode's drawer function.
//
void F_Drawer(void)
{
   switch(finalestage)
   {
   case 2:
      F_CastDrawer();
      break;
   case 0:
      F_TextWrite();
      break;
   default:
      (FDrawers[gameModeInfo->type])();
      break;
   }
}

//----------------------------------------------------------------------------
//
// $Log: f_finale.c,v $
// Revision 1.16  1998/05/10  23:39:25  killough
// Restore v1.9 demo sync on text intermission screens
//
// Revision 1.15  1998/05/04  21:34:30  thldrmn
// commenting and reformatting
//
// Revision 1.14  1998/05/03  23:25:05  killough
// Fix #includes at the top, nothing else
//
// Revision 1.13  1998/04/19  01:17:18  killough
// Tidy up last fix's code
//
// Revision 1.12  1998/04/17  15:14:10  killough
// Fix showstopper flat bug
//
// Revision 1.11  1998/03/31  16:19:25  killough
// Fix minor merge glitch
//
// Revision 1.10  1998/03/31  11:41:21  jim
// Fix merge glitch in f_finale.c
//
// Revision 1.9  1998/03/31  00:37:56  jim
// Ty's finale.c fixes
//
// Revision 1.8  1998/03/28  17:51:33  killough
// Allow use/fire to accelerate teletype messages
//
// Revision 1.7  1998/02/05  12:15:06  phares
// cleaned up comments
//
// Revision 1.6  1998/02/02  13:43:30  killough
// Relax endgame message speed to demo_compatibility
//
// Revision 1.5  1998/01/31  01:47:39  phares
// Removed textspeed and textwait externs
//
// Revision 1.4  1998/01/30  18:48:18  phares
// Changed textspeed and textwait to functions
//
// Revision 1.3  1998/01/30  16:08:56  phares
// Faster end-mission text display
//
// Revision 1.2  1998/01/26  19:23:14  phares
// First rev with no ^Ms
//
// Revision 1.1.1.1  1998/01/19  14:02:54  rand
// Lee's Jan 19 sources
//
//
//----------------------------------------------------------------------------
