// Emacs style mode select -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2003 James Haley
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
// Skin Viewer Widget for the Menu
//
//    Why? Because I can!
//
// haleyjd 04/20/03
//
//-----------------------------------------------------------------------------

#include "doomstat.h"
#include "doomtype.h"
#include "info.h"
#include "mn_engin.h"
#include "r_defs.h"
#include "r_draw.h"
#include "v_video.h"
#include "w_wad.h"
#include "p_pspr.h"
#include "p_skin.h"
#include "s_sound.h"
#include "sounds.h"
#include "m_random.h"
#include "d_gi.h"
#include "e_edf.h"

// player skin sprite states
enum
{
   SKV_WALKING,
   SKV_FIRING,
   SKV_PAIN,
   SKV_DEAD,
};

// static state variables

static state_t *skview_state = NULL;
static int skview_tics = 0;
static int skview_action = SKV_WALKING;
static int skview_rot = 0;
static boolean skview_halfspeed = false;
static int skview_typenum; // 07/12/03

// TODO: make sure this is a good sound for heretic
#define ATKSOUND \
  (gameModeInfo->flags & GIF_HERETIC ? sfx_gldhit : sfx_shotgn)

//
// MN_SkinSetState
//
// Sets the skin sprite to a state, and sets its tics appropriately.
// The tics will be set to 2x normal if half speed mode is active.
//
static void MN_SkinSetState(state_t *state)
{
   int tics;
   
   skview_state = state;
   
   tics = skview_state->tics;
   skview_tics = menutime + (skview_halfspeed ? 2*tics : tics);
}

//
// MN_SkinResponder
//
// The skin viewer widget responder function. Sorta long and
// hackish, but cool.
//
static boolean MN_SkinResponder(event_t *ev)
{
   // only interested in keydown events
   if(ev->type != ev_keydown)
      return false;

   switch(ev->data1)
   {
   case KEYD_BACKSPACE:
   case KEYD_ESCAPE:
      // kill the widget
      S_StartSound(NULL, gameModeInfo->menuSounds[MN_SND_DEACTIVATE]);
      current_menuwidget = NULL;
      break;
   case KEYD_LEFTARROW:
      // rotate sprite left
      if(skview_rot == 7)
         skview_rot = 0;
      else
         skview_rot++;
      break;
   case KEYD_RIGHTARROW:
      // rotate sprite right
      if(skview_rot == 0)
         skview_rot = 7;
      else
         skview_rot--;
      break;
   case KEYD_RCTRL:
      // attack!
      if(skview_action == SKV_WALKING)
      {
         S_StartSound(NULL, ATKSOUND);
         MN_SkinSetState(&states[E_SafeState(S_PLAY_ATK2)]);
         skview_action = SKV_FIRING;
      }
      break;
   case 'p':
   case 'P':
      // act hurt
      if(skview_action == SKV_WALKING)
      {
         S_StartSoundName(NULL,
            players[consoleplayer].skin->sounds[sk_plpain]);
         MN_SkinSetState(&states[mobjinfo[skview_typenum].painstate]);
         skview_action = SKV_PAIN;
      }
      break;
   case 'd':
   case 'D':
      // die normally
      if(skview_action != SKV_DEAD)
      {
         S_StartSoundName(NULL,
            (gamemode == shareware || M_Random() % 2) ? 
               players[consoleplayer].skin->sounds[sk_pldeth] :
               players[consoleplayer].skin->sounds[sk_pdiehi]);
         MN_SkinSetState(&states[mobjinfo[skview_typenum].deathstate]);
         skview_action = SKV_DEAD;
      }
      break;
   case 'x':
   case 'X':
      // gib
      if(skview_action != SKV_DEAD)
      {
         S_StartSoundName(NULL,
            players[consoleplayer].skin->sounds[sk_slop]);
         MN_SkinSetState(&states[mobjinfo[skview_typenum].xdeathstate]);
         skview_action = SKV_DEAD;
      }
      break;
   case 'h':
   case 'H':
      // toggle half-speed animation
      skview_halfspeed ^= true;
      break;
   case KEYD_SPACEBAR:
      // "respawn" the player if dead
      if(skview_action == SKV_DEAD)
      {
         S_StartSound(NULL, gameModeInfo->teleSound);
         MN_SkinSetState(&states[mobjinfo[skview_typenum].seestate]);
         skview_action = SKV_WALKING;
      }
      break;
   default:
      return false;
   }

   return true;
}

// defines for instruction string positions -- these cascade

#define EXIT_Y  (SCREENHEIGHT - gameModeInfo->vtextinfo->cy - 1)
#define RESP_Y  (EXIT_Y - gameModeInfo->vtextinfo->cy)
#define A_Y     (RESP_Y - gameModeInfo->vtextinfo->cy)
#define LR_Y    (A_Y - gameModeInfo->vtextinfo->cy)
#define INSTR_Y (LR_Y - gameModeInfo->vtextinfo->cy)

//
// MN_SkinInstructions
//
// Draws some v_font strings to the screen to tell the user how
// to operate this beast.
//
static void MN_SkinInstructions(void)
{
   char *msg = FC_GOLD "skin viewer";
  
   // draw a title at the top, too
   V_WriteText(msg, 160 - V_StringWidth(msg)/2, 8);

   V_WriteText("instructions:", 4, INSTR_Y);
   V_WriteText(FC_GRAY "<-" FC_RED " = rotate left,  "
               FC_GRAY "->" FC_RED " = rotate right", 4, LR_Y);
   V_WriteText(FC_GRAY "ctrl" FC_RED " = fire,  "
               FC_GRAY "p" FC_RED " = pain,  "
               FC_GRAY "d" FC_RED " = die,  "
               FC_GRAY "x" FC_RED " = gib", 4, A_Y);
   V_WriteText(FC_GRAY "space" FC_RED " = respawn,  "
               FC_GRAY "h" FC_RED " = half-speed", 4, RESP_Y);
   V_WriteText(FC_GRAY "escape or backspace" FC_RED " = exit", 4, EXIT_Y);
}

//
// MN_SkinDrawer
//
// The skin viewer widget drawer function. Basically implements a
// small state machine and sprite drawer all in one function. Since
// state transition timing is done through the drawer and not a ticker,
// it's not absolutely precise, but its good enough.
//
static void MN_SkinDrawer(void)
{
   spritedef_t *sprdef;
   spriteframe_t *sprframe;
   int lump;
   boolean flip;
   patch_t *patch;

   // draw the normal menu background
   V_DrawBackground(gameModeInfo->menuBackground, &vbscreen);

   // draw instructions and title
   MN_SkinInstructions();

   // do state transitions
   if(skview_tics != -1 && menutime >= skview_tics)
   {
      // EDF FIXME: frames need fix
      // hack states: these need special nextstate handling so
      // that the player will start walking again afterward
      if(skview_state == &states[E_SafeState(S_PLAY_ATK1)] ||
         skview_state == &states[E_SafeState(S_PLAY_PAIN2)])
      {
         MN_SkinSetState(&states[mobjinfo[skview_typenum].seestate]);
         skview_action = SKV_WALKING;
      }
      else
      {
         // normal state transition
         MN_SkinSetState(&states[skview_state->nextstate]);

         // if the state has -1 tics, reset skview_tics
         if(skview_state->tics == -1)
            skview_tics = -1;
      }
   }

   // get the player skin sprite definition
   sprdef = &sprites[players[consoleplayer].skin->sprite];
   if(!(sprdef->spriteframes))
      return;

   // get the current frame, using the skin state and rotation vars
   sprframe = &sprdef->spriteframes[skview_state->frame&FF_FRAMEMASK];
   if(sprframe->rotate)
   {
      lump = sprframe->lump[skview_rot];
      flip = (boolean)sprframe->flip[skview_rot];
   }
   else
   {
      lump = sprframe->lump[0];
      flip = (boolean)sprframe->flip[0];
   }

   // cache the sprite patch -- watch out for "firstspritelump"!
   patch = W_CacheLumpNum(lump+firstspritelump, PU_CACHE);

   // draw the sprite, with color translation and proper flipping
   // 01/12/04: changed translation handling
   V_DrawPatchTranslated(160, 120, &vbscreen, patch,
      players[consoleplayer].colormap ?
        (char *)translationtables[(players[consoleplayer].colormap - 1)] :
        NULL, 
      flip);
}

// the skinviewer menu widget

menuwidget_t skinviewer = { MN_SkinDrawer, MN_SkinResponder, true };

//
// MN_InitSkinViewer
//
// Called by the skinviewer console command, this function resets
// all the skview internal state variables to their defaults, and
// activates the skinviewer menu widget.
//
void MN_InitSkinViewer(void)
{
   // reset all state variables
   skview_action = SKV_WALKING;
   skview_rot = 0;
   skview_halfspeed = false;
   skview_typenum = E_GetThingNumForDEHNum(MT_PLAYER);

   MN_SkinSetState(&states[mobjinfo[skview_typenum].seestate]);

   // set the widget
   current_menuwidget = &skinviewer;
}

// EOF

