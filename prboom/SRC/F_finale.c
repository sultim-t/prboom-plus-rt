// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: F_finale.c,v 1.1 2000/04/09 18:18:13 proff_fs Exp $
//
//  PRBOOM/GLBOOM (C) Florian 'Proff' Schulze (florian.proff.schulze@gmx.net)
//  based on
//  BOOM, a modified and improved DOOM engine
//  Copyright (C) 1999 by
//  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
//  02111-1307, USA.
//
// DESCRIPTION:
//      Game completion, final screen animation.
//
//-----------------------------------------------------------------------------


static const char
rcsid[] = "$Id: F_finale.c,v 1.1 2000/04/09 18:18:13 proff_fs Exp $";

#include "doomstat.h"
#include "d_event.h"
#include "v_video.h"
#include "w_wad.h"
#include "s_sound.h"
#include "sounds.h"
#include "dstrings.h"
#include "m_menu.h"
#include "d_deh.h"  // Ty 03/22/98 - externalizations

// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast
int finalestage;
int finalecount;

// defines for the end mission display text                     // phares

#define TEXTSPEED    3     // original value                    // phares
#define TEXTWAIT     250   // original value                    // phares
#define NEWTEXTSPEED 0.01  // new value                         // phares
#define NEWTEXTWAIT  1000  // new value                         // phares

char*   finaletext;
char*   finaleflat;

void    F_StartCast (void);
void    F_CastTicker (void);
H_boolean F_CastResponder (event_t *ev);
void    F_CastDrawer (void);

void WI_checkForAccelerate(void);    // killough 3/28/98: used to
extern int acceleratestage;          // accelerate intermission screens
static int midstage;                 // whether we're in "mid-stage"

//
// F_StartFinale
//
void F_StartFinale (void)
{
  gameaction = ga_nothing;
  gamestate = GS_FINALE;
  viewactive = false;
  automapactive = false;

  // killough 3/28/98: clear accelerative text flags
  acceleratestage = midstage = 0;

  // Okay - IWAD dependend stuff.
  // This has been changed severly, and
  //  some stuff might have changed in the process.
  switch ( gamemode )
  {
    // DOOM 1 - E1, E3 or E4, but each nine missions
    case shareware:
    case registered:
    case retail:
    {
      S_ChangeMusic(mus_victor, true);
      
      switch (gameepisode)
      {
        case 1:
             finaleflat = bgflatE1; // Ty 03/30/98 - new externalized bg flats
             finaletext = s_E1TEXT; // Ty 03/23/98 - Was e1text variable.
             break;
        case 2:
             finaleflat = bgflatE2;
             finaletext = s_E2TEXT; // Ty 03/23/98 - Same stuff for each 
             break;
        case 3:
             finaleflat = bgflatE3;
             finaletext = s_E3TEXT;
             break;
        case 4:
             finaleflat = bgflatE4;
             finaletext = s_E4TEXT;
             break;
        default:
             // Ouch.
             break;
      }
      break;
    }
    
    // DOOM II and missions packs with E1, M34
    case commercial:
    {
      S_ChangeMusic(mus_read_m, true);

      // Ty 08/27/98 - added the gamemission logic
      switch (gamemap)
      {
        case 6:
             finaleflat = bgflat06;
             finaletext = (gamemission==pack_tnt)  ? s_T1TEXT :
                          (gamemission==pack_plut) ? s_P1TEXT : s_C1TEXT;
             break;
        case 11:
             finaleflat = bgflat11;
             finaletext = (gamemission==pack_tnt)  ? s_T2TEXT :
                          (gamemission==pack_plut) ? s_P2TEXT : s_C2TEXT;
             break;
        case 20:
             finaleflat = bgflat20;
             finaletext = (gamemission==pack_tnt)  ? s_T3TEXT :
                          (gamemission==pack_plut) ? s_P3TEXT : s_C3TEXT;
             break;
        case 30:
             finaleflat = bgflat30;
             finaletext = (gamemission==pack_tnt)  ? s_T4TEXT :
                          (gamemission==pack_plut) ? s_P4TEXT : s_C4TEXT;
             break;
        case 15:
             finaleflat = bgflat15;
             finaletext = (gamemission==pack_tnt)  ? s_T5TEXT :
                          (gamemission==pack_plut) ? s_P5TEXT : s_C5TEXT;
             break;
        case 31:
             finaleflat = bgflat31;
             finaletext = (gamemission==pack_tnt)  ? s_T6TEXT :
                          (gamemission==pack_plut) ? s_P6TEXT : s_C6TEXT;
             break;
        default:
             // Ouch.
             break;
      }
      break;
      // Ty 08/27/98 - end gamemission logic
    } 

    // Indeterminate.
    default:  // Ty 03/30/98 - not externalized
         S_ChangeMusic(mus_read_m, true);
         finaleflat = "F_SKY1"; // Not used anywhere else.
         finaletext = s_C1TEXT;  // FIXME - other text, music?
         break;
  }
  
  finalestage = 0;
  finalecount = 0;
}



H_boolean F_Responder (event_t *event)
{
  if (finalestage == 2)
    return F_CastResponder (event);
        
  return false;
}

// Get_TextSpeed() returns the value of the text display speed  // phares
// Rewritten to allow user-directed acceleration -- killough 3/28/98

static float Get_TextSpeed(void)
{
  return (float)(midstage ? NEWTEXTSPEED : (midstage=acceleratestage) ? 
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
//

void F_Ticker(void)
{
  int i;
  if (!demo_compatibility)
    WI_checkForAccelerate();  // killough 3/28/98: check for acceleration
  else
    if (gamemode == commercial && finalecount > 50) // check for skipping
      for (i=0; i<MAXPLAYERS; i++)
        if (players[i].cmd.buttons)
          goto next_level;      // go on to the next level

  // advance animation
  finalecount++;
 
  if (finalestage == 2)
    F_CastTicker();

  if (!finalestage)
    {
      float speed = demo_compatibility ? TEXTSPEED : Get_TextSpeed();
      if (finalecount > strlen(finaletext)*speed +  // phares
          (midstage ? NEWTEXTWAIT : TEXTWAIT) ||  // killough 2/28/98:
          (midstage && acceleratestage))       // changed to allow acceleration
        if (gamemode != commercial)       // Doom 1 / Ultimate Doom episode end
          {                               // with enough time, it's automatic
            finalecount = 0;
            finalestage = 1;
            wipegamestate = -1;         // force a wipe
            if (gameepisode == 3)
              S_StartMusic(mus_bunny);
          }
        else   // you must press a button to continue in Doom 2
          if (!demo_compatibility && midstage)
            {
            next_level:
              if (gamemap == 30)
                F_StartCast();              // cast of Doom 2 characters
              else
                gameaction = ga_worlddone;  // next level, e.g. MAP07
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

#include "hu_stuff.h"
extern patchnum_t hu_font[HU_FONTSIZE];


void F_TextWrite (void)
{
  int         w;
  int         count;
  char*       ch;
  int         c;
  int         cx;
  int         cy;
  
  M_DrawBackground(finaleflat, screens[0]); // Draw background

  // draw some of the text onto the screen
  cx = 10;
  cy = 10;
  ch = finaletext;
      
  count = (int)((finalecount - 10)/Get_TextSpeed());                 // phares
  if (count < 0)
    count = 0;

  for ( ; count ; count-- )
  {
    c = *ch++;
    if (!c)
      break;
    if (c == '\n')
    {
      cx = 10;
      cy += 11;
      continue;
    }
              
    c = toupper(c) - HU_FONTSTART;
    if (c < 0 || c> HU_FONTSIZE)
    {
      cx += 4;
      continue;
    }
              
    w = SHORT (hu_font[c].width);
    if (cx+w > SCREENWIDTH)
      break;
    V_DrawPatchStretchedFromNum(cx, cy, 0, hu_font[c].lumpnum);
    cx+=w;
  }
}

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//
typedef struct
{
  char       *name;
  mobjtype_t  type;
} castinfo_t;

#define MAX_CASTORDER 18 /* Ty - hard coded for now */
castinfo_t      castorder[MAX_CASTORDER]; // Ty 03/22/98 - externalized and init moved into f_startcast()

int             castnum;
int             casttics;
state_t*        caststate;
H_boolean         castdeath;
int             castframes;
int             castonmelee;
H_boolean         castattacking;


//
// F_StartCast
//
extern  gamestate_t     wipegamestate;

void F_StartCast (void)
{
  // Ty 03/23/98 - clumsy but time is of the essence
  castorder[0].name = s_CC_ZOMBIE,  castorder[0].type = MT_POSSESSED;
  castorder[1].name = s_CC_SHOTGUN, castorder[1].type = MT_SHOTGUY;
  castorder[2].name = s_CC_HEAVY,   castorder[2].type = MT_CHAINGUY;
  castorder[3].name = s_CC_IMP,     castorder[3].type = MT_TROOP;
  castorder[4].name = s_CC_DEMON,   castorder[4].type = MT_SERGEANT;
  castorder[5].name = s_CC_LOST,    castorder[5].type = MT_SKULL;
  castorder[6].name = s_CC_CACO,    castorder[6].type = MT_HEAD;
  castorder[7].name = s_CC_HELL,    castorder[7].type = MT_KNIGHT;
  castorder[8].name = s_CC_BARON,   castorder[8].type = MT_BRUISER;
  castorder[9].name = s_CC_ARACH,   castorder[9].type = MT_BABY;
  castorder[10].name = s_CC_PAIN,   castorder[10].type = MT_PAIN;
  castorder[11].name = s_CC_REVEN,  castorder[11].type = MT_UNDEAD;
  castorder[12].name = s_CC_MANCU,  castorder[12].type = MT_FATSO;
  castorder[13].name = s_CC_ARCH,   castorder[13].type = MT_VILE;
  castorder[14].name = s_CC_SPIDER, castorder[14].type = MT_SPIDER;
  castorder[15].name = s_CC_CYBER,  castorder[15].type = MT_CYBORG;
  castorder[16].name = s_CC_HERO,   castorder[16].type = MT_PLAYER;
  castorder[17].name = NULL,        castorder[17].type = 0;

  wipegamestate = -1;         // force a screen wipe
  castnum = 0;
  caststate = &states[mobjinfo[castorder[castnum].type].seestate];
  casttics = caststate->tics;
  castdeath = false;
  finalestage = 2;    
  castframes = 0;
  castonmelee = 0;
  castattacking = false;
  S_ChangeMusic(mus_evil, true);
}


//
// F_CastTicker
//
void F_CastTicker (void)
{
  int st;
  int sfx;
      
  if (--casttics > 0)
    return;                 // not time to change state yet
              
  if (caststate->tics == -1 || caststate->nextstate == S_NULL)
  {
    // switch from deathstate to next monster
    castnum++;
    castdeath = false;
    if (castorder[castnum].name == NULL)
      castnum = 0;
    if (mobjinfo[castorder[castnum].type].seesound)
      S_StartSound (NULL, mobjinfo[castorder[castnum].type].seesound);
    caststate = &states[mobjinfo[castorder[castnum].type].seestate];
    castframes = 0;
  }
  else
  {
    // just advance to next state in animation
    if (caststate == &states[S_PLAY_ATK1])
      goto stopattack;    // Oh, gross hack!
    st = caststate->nextstate;
    caststate = &states[st];
    castframes++;
      
    // sound hacks....
    switch (st)
    {
      case S_PLAY_ATK1:     sfx = sfx_dshtgn; break;
      case S_POSS_ATK2:     sfx = sfx_pistol; break;
      case S_SPOS_ATK2:     sfx = sfx_shotgn; break;
      case S_VILE_ATK2:     sfx = sfx_vilatk; break;
      case S_SKEL_FIST2:    sfx = sfx_skeswg; break;
      case S_SKEL_FIST4:    sfx = sfx_skepch; break;
      case S_SKEL_MISS2:    sfx = sfx_skeatk; break;
      case S_FATT_ATK8:
      case S_FATT_ATK5:
      case S_FATT_ATK2:     sfx = sfx_firsht; break;
      case S_CPOS_ATK2:
      case S_CPOS_ATK3:
      case S_CPOS_ATK4:     sfx = sfx_shotgn; break;
      case S_TROO_ATK3:     sfx = sfx_claw; break;
      case S_SARG_ATK2:     sfx = sfx_sgtatk; break;
      case S_BOSS_ATK2:
      case S_BOS2_ATK2:
      case S_HEAD_ATK2:     sfx = sfx_firsht; break;
      case S_SKULL_ATK2:    sfx = sfx_sklatk; break;
      case S_SPID_ATK2:
      case S_SPID_ATK3:     sfx = sfx_shotgn; break;
      case S_BSPI_ATK2:     sfx = sfx_plasma; break;
      case S_CYBER_ATK2:
      case S_CYBER_ATK4:
      case S_CYBER_ATK6:    sfx = sfx_rlaunc; break;
      case S_PAIN_ATK3:     sfx = sfx_sklatk; break;
      default: sfx = 0; break;
    }
            
    if (sfx)
      S_StartSound (NULL, sfx);
  }
      
  if (castframes == 12)
  {
    // go into attack frame
    castattacking = true;
    if (castonmelee)
      caststate=&states[mobjinfo[castorder[castnum].type].meleestate];
    else
      caststate=&states[mobjinfo[castorder[castnum].type].missilestate];
    castonmelee ^= 1;
    if (caststate == &states[S_NULL])
    {
      if (castonmelee)
        caststate=
          &states[mobjinfo[castorder[castnum].type].meleestate];
      else
        caststate=
          &states[mobjinfo[castorder[castnum].type].missilestate];
    }
  }
      
  if (castattacking)
  {
    if (castframes == 24
       ||  caststate == &states[mobjinfo[castorder[castnum].type].seestate] )
    {
      stopattack:
      castattacking = false;
      castframes = 0;
      caststate = &states[mobjinfo[castorder[castnum].type].seestate];
    }
  }
      
  casttics = caststate->tics;
  if (casttics == -1)
      casttics = 15;
}


//
// F_CastResponder
//

H_boolean F_CastResponder (event_t* ev)
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
  if (mobjinfo[castorder[castnum].type].deathsound)
    S_StartSound (NULL, mobjinfo[castorder[castnum].type].deathsound);
        
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
    c = toupper(c) - HU_FONTSTART;
    if (c < 0 || c> HU_FONTSIZE)
    {
      width += 4;
      continue;
    }
            
    w = SHORT (hu_font[c].width);
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
    c = toupper(c) - HU_FONTSTART;
    if (c < 0 || c> HU_FONTSIZE)
    {
      cx += 4;
      continue;
    }
              
    w = SHORT (hu_font[c].width);
    V_DrawPatchStretchedFromNum(cx, 180, 0, hu_font[c].lumpnum);
    cx+=w;
  }
}


//
// F_CastDrawer
//

void F_CastDrawer (void)
{
  spritedef_t*        sprdef;
  spriteframe_t*      sprframe;
  int                 lump;
  H_boolean           flip;
    
  // erase the entire screen to a background
  V_DrawPatchStretchedFromName(0,0,0, bgcastcall); // Ty 03/30/98 bg texture extern

  F_CastPrint (castorder[castnum].name);
    
  // draw the current frame in the middle of the screen
  sprdef = &sprites[caststate->sprite];
  sprframe = &sprdef->spriteframes[ caststate->frame & FF_FRAMEMASK];
  lump = sprframe->lump[0];
  flip = (H_boolean)sprframe->flip[0];
                        
  if (flip)
    V_DrawPatchFlipStretchedFromNum(160,170,0,lump+firstspritelump);
  else
    V_DrawPatchStretchedFromNum(160,170,0,lump+firstspritelump);
}

//
// F_DrawPatchCol
//
static void F_DrawPatchCol(int x, patch_t* patch, int col)
{
  column_t*   column;
  byte*       source;
  byte*       dest;
  byte*       desttop;
  int         count;
        
  column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
  desttop = screens[0]+x;

  // step through the posts in a column
  while (column->topdelta != 0xff )
  {
    source = (byte *)column + 3;
    dest = desttop + column->topdelta*SCREENWIDTH;
    count = column->length;
                
    while (count--)
    {
      *dest = *source++;
      dest += SCREENWIDTH;
    }
    column = (column_t *)(  (byte *)column + column->length + 4 );
  }
}

// proff 09/21/98: Added for high-res
static void F_DrawPatchColStretched(int x, patch_t* patch, int col)
{
  column_t*   column;
  byte*       source;
  byte*       dest;
  byte*       desttop;
  int         count;
  int         srccol;
  int DY;
  int DYI;
  
  if (SCREENHEIGHT==200)
  {
    F_DrawPatchCol(x,patch,col);
    return;
  }

  column = (column_t *)((byte *)patch + LONG(patch->columnofs[col]));
  desttop = screens[0]+x;

  DY  = (SCREENHEIGHT<<16) / 200;
  DYI = (200<<16)          / SCREENHEIGHT;

  // step through the posts in a column
  while (column->topdelta != 0xff )
  {
    source = (byte *)column + 3;
    dest = desttop + ( ( column->topdelta * DY ) >> 16 )*SCREENWIDTH;
    count = ( column->length * DY ) >> 16;
 	  srccol = 0x8000;
                
    while (count--)
    {
      *dest  =  source[srccol>>16];
      dest  += SCREENWIDTH;
      srccol+=  DYI;
    }
    column = (column_t *)(  (byte *)column + column->length + 4 );
  }
}


//
// F_BunnyScroll
//
void F_BunnyScroll (void)
{
// proff 11/99: use OpenGL
#ifdef GL_DOOM
    int         scrolled;

    scrolled = 320 - (finalecount-230)/2;
    if (scrolled > 320)
      scrolled = 320;
    if (scrolled < 0)
      scrolled = 0;

    V_DrawPatchStretchedFromName(320-scrolled-1,0,0,"PFUB1");
    V_DrawPatchStretchedFromName(-scrolled,0,0,"PFUB2");
#else
    int         scrolled;
    int         x;
    patch_t*    p1;
    patch_t*    p2;
    char        name[10];
    int         stage;
    static int  laststage;
    // proff 09/21/98: Added for high-res
    int         DXI;
    int         SX;
  
    p1 = W_CacheLumpName ("PFUB2", PU_LEVEL);
    p2 = W_CacheLumpName ("PFUB1", PU_LEVEL);

    scrolled = 320 - (finalecount-230)/2;
    if (scrolled > 320)
      scrolled = 320;
    if (scrolled < 0)
      scrolled = 0;

    // proff 09/21/98: Added for high-res
    DXI=(320<<16)/SCREENWIDTH;
    SX=0;
    for ( x=0 ; x<SCREENWIDTH ; x++,SX+=DXI)
    {
      if ((SX>>16)+scrolled < 320)
        F_DrawPatchColStretched (x, p1, (SX>>16)+scrolled);
      else
        F_DrawPatchColStretched (x, p2, (SX>>16)+scrolled - 320);           
    }

    // proff: Free the allocated lumps
    Z_ChangeTag(p2,PU_CACHE);
    Z_ChangeTag(p1,PU_CACHE);
      
  if (finalecount < 1130)
    return;
  if (finalecount < 1180)
  {
    V_DrawPatchStretchedFromName((320-13*8)/2,(200-8*8)/2,0,"END0");
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
      
  sprintf (name,"END%i",stage);
  V_DrawPatchStretchedFromName((320-13*8)/2,(200-8*8)/2,0,name);
#endif //GL_DOOM
}


//
// F_Drawer
//
void F_Drawer (void)
{
  if (finalestage == 2)
  {
    F_CastDrawer ();
    return;
  }

  if (!finalestage)
    F_TextWrite ();
  else
  {
    switch (gameepisode)
    {
      case 1:
           if ( gamemode == retail )
             V_DrawPatchStretchedFromName(0,0,0,"CREDIT");
           else
             V_DrawPatchStretchedFromName(0,0,0,"HELP2");
           break;
      case 2:
           V_DrawPatchStretchedFromName(0,0,0,"VICTORY2");
           break;
      case 3:
           F_BunnyScroll ();
           break;
      case 4:
           V_DrawPatchStretchedFromName(0,0,0,"ENDPIC");
           break;
    }
  }
}




//----------------------------------------------------------------------------
//
// $Log: F_finale.c,v $
// Revision 1.1  2000/04/09 18:18:13  proff_fs
// Initial revision
//
// Revision 1.17  1998/08/29  23:00:55  thldrmn
// Gamemission fixes for TNT and Plutonia
//
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
