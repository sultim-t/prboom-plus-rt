/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: p_user.c,v 1.2 2000/05/09 21:45:39 proff_fs Exp $
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
 *      Player related stuff.
 *      Bobbing POV/weapon, movement.
 *      Pending weapon.
 *
 *-----------------------------------------------------------------------------*/

static const char
rcsid[] = "$Id: p_user.c,v 1.2 2000/05/09 21:45:39 proff_fs Exp $";

#include "doomstat.h"
#include "d_event.h"
#include "r_main.h"
#include "p_map.h"
#include "p_spec.h"
#include "p_user.h"

// Index of the special effects (INVUL inverse) map.

#define INVERSECOLORMAP 32

//
// Movement.
//

// 16 pixels of bob

#define MAXBOB  0x100000

boolean onground; // whether player is on ground or in air

//
// P_Thrust
// Moves the given origin along a given angle.
//

void P_Thrust(player_t* player,angle_t angle,fixed_t move)
  {
  angle >>= ANGLETOFINESHIFT;
  player->mo->momx += FixedMul(move,finecosine[angle]);
  player->mo->momy += FixedMul(move,finesine[angle]);
  }


//
// P_CalcHeight
// Calculate the walking / running height adjustment
//

void P_CalcHeight (player_t* player)
  {
  int     angle;
  fixed_t bob;

  // Regular movement bobbing
  // (needs to be calculated for gun swing
  // even if not on ground)
  // OPTIMIZE: tablify angle
  // Note: a LUT allows for effects
  //  like a ramp with low health.

  if (!demo_compatibility && !player_bobbing)               // phares 2/26/98
    player->bob = 0;                                        //   |
  else                                                      //   V
    {
    player->bob = FixedMul(player->mo->momx,player->mo->momx)
                + FixedMul(player->mo->momy,player->mo->momy);
    player->bob >>= 2;

    // phares 9/9/98: If player is standing on ice, reduce his bobbing.

    if (player->mo->friction > ORIG_FRICTION) // ice?
      {
      if (player->bob > (MAXBOB>>2))
        player->bob = MAXBOB>>2;
      }
    else                                                    //   ^
      if (player->bob > MAXBOB)                             //   |
        player->bob = MAXBOB;                               // phares 2/26/98
    }

  if ((player->cheats & CF_NOMOMENTUM) || !onground)
    {
    player->viewz = player->mo->z + VIEWHEIGHT;

    if (player->viewz > player->mo->ceilingz-4*FRACUNIT)
      player->viewz = player->mo->ceilingz-4*FRACUNIT;

// The following line was in the Id source and appears      // phares 2/25/98
// to be a bug. player->viewz is checked in a similar
// manner at a different exit below.

//  player->viewz = player->mo->z + player->viewheight;
    return;
    }

  angle = (FINEANGLES/20*leveltime)&FINEMASK;
  bob = FixedMul(player->bob/2,finesine[angle]);

  // move viewheight

  if (player->playerstate == PST_LIVE)
    {
    player->viewheight += player->deltaviewheight;

    if (player->viewheight > VIEWHEIGHT)
      {
      player->viewheight = VIEWHEIGHT;
      player->deltaviewheight = 0;
      }

    if (player->viewheight < VIEWHEIGHT/2)
      {
      player->viewheight = VIEWHEIGHT/2;
      if (player->deltaviewheight <= 0)
        player->deltaviewheight = 1;
      }

    if (player->deltaviewheight)
      {
      player->deltaviewheight += FRACUNIT/4;
      if (!player->deltaviewheight)
        player->deltaviewheight = 1;
      }
    }

  player->viewz = player->mo->z + player->viewheight + bob;

  if (player->viewz > player->mo->ceilingz-4*FRACUNIT)
    player->viewz = player->mo->ceilingz-4*FRACUNIT;
  }


//
// P_MovePlayer
//
// Adds momentum if the player is not in the air

void P_MovePlayer (player_t* player)
  {
  ticcmd_t* cmd;
  int       movefactor;       // movement factor                    // phares
  mobj_t*   thismo;           // local object                       // phares
  boolean   onobject = false; // on top of an object?               // phares

  cmd = &player->cmd;

  thismo = player->mo;                                              // phares
  thismo->angle += (cmd->angleturn<<16);                            //   |
                                                                    //   V
// Do not let the player control movement if not on ground.

  onground = (thismo->z <= thismo->floorz);
  if (onground || onobject)
    {
    movefactor = P_GetMoveFactor(thismo);
    if (cmd->forwardmove)
      P_Thrust(player,thismo->angle,cmd->forwardmove*movefactor);
    if (cmd->sidemove)
      P_Thrust(player,thismo->angle-ANG90,cmd->sidemove*movefactor);
    }
  if ((cmd->forwardmove || cmd->sidemove) &&
    (thismo->state == &states[S_PLAY]))                             //   ^
    P_SetMobjState(thismo,S_PLAY_RUN1);                             //   |
  }                                                                 // phares

#define ANG5 (ANG90/18)

//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//

void P_DeathThink (player_t* player)
  {
  angle_t angle;
  angle_t delta;

  P_MovePsprites (player);

  // fall to the ground

  if (player->viewheight > 6*FRACUNIT)
    player->viewheight -= FRACUNIT;

  if (player->viewheight < 6*FRACUNIT)
    player->viewheight = 6*FRACUNIT;

  player->deltaviewheight = 0;
  onground = (player->mo->z <= player->mo->floorz);
  P_CalcHeight (player);

  if (player->attacker && player->attacker != player->mo)
    {
    angle = R_PointToAngle2 (player->mo->x,
                             player->mo->y,
                             player->attacker->x,
                             player->attacker->y);

    delta = angle - player->mo->angle;

    if (delta < ANG5 || delta > (unsigned)-ANG5)
      {
      // Looking at killer,
      //  so fade damage flash down.

      player->mo->angle = angle;

      if (player->damagecount)
        player->damagecount--;
      }
    else if (delta < ANG180)
      player->mo->angle += ANG5;
    else
      player->mo->angle -= ANG5;
    }
  else if (player->damagecount)
    player->damagecount--;

  if (player->cmd.buttons & BT_USE)
    player->playerstate = PST_REBORN;
  }


//
// P_PlayerThink
//

void P_PlayerThink (player_t* player)
  {
  ticcmd_t*    cmd;
  weapontype_t newweapon;

  // killough 2/8/98, 3/21/98:
  if (player->cheats & CF_NOCLIP)
    player->mo->flags |= MF_NOCLIP;
  else
    player->mo->flags &= ~MF_NOCLIP;

  // chain saw run forward

  cmd = &player->cmd;
  if (player->mo->flags & MF_JUSTATTACKED)
    {
    cmd->angleturn = 0;
    cmd->forwardmove = 0xc800/512;
    cmd->sidemove = 0;
    player->mo->flags &= ~MF_JUSTATTACKED;
    }

  if (player->playerstate == PST_DEAD)
    {
    P_DeathThink (player);
    return;
    }

  // Move around.
  // Reactiontime is used to prevent movement
  //  for a bit after a teleport.

  if (player->mo->reactiontime)
    player->mo->reactiontime--;
  else
    P_MovePlayer (player);

  P_CalcHeight (player); // Determines view height and bobbing

  // Determine if there's anything about the sector you're in that's
  // going to affect you, like painful floors.

  if (player->mo->subsector->sector->special)
    P_PlayerInSpecialSector (player);

  // Check for weapon change.

  if (cmd->buttons & BT_CHANGE)
    {
    // The actual changing of the weapon is done
    //  when the weapon psprite can do it
    //  (read: not in the middle of an attack).

    newweapon = (cmd->buttons & BT_WEAPONMASK)>>BT_WEAPONSHIFT;

    // killough 3/22/98: For demo compatibility we must perform the fist
    // and SSG weapons switches here, rather than in G_BuildTiccmd(). For
    // other games which rely on user preferences, we must use the latter.

    if (demo_compatibility)
      { // compatibility mode -- required for old demos -- killough
      if (newweapon == wp_fist && player->weaponowned[wp_chainsaw] &&
        (player->readyweapon != wp_chainsaw ||
         !player->powers[pw_strength]))
        newweapon = wp_chainsaw;
      if (gamemode == commercial &&
        newweapon == wp_shotgun &&
        player->weaponowned[wp_supershotgun] &&
        player->readyweapon != wp_supershotgun)
        newweapon = wp_supershotgun;
      }

    // killough 2/8/98, 3/22/98 -- end of weapon selection changes

    if (player->weaponowned[newweapon] && newweapon != player->readyweapon)

      // Do not go to plasma or BFG in shareware,
      //  even if cheated.

      if ((newweapon != wp_plasma && newweapon != wp_bfg)
          || (gamemode != shareware) )
        player->pendingweapon = newweapon;
    }

  // check for use

  if (cmd->buttons & BT_USE)
    {
    if (!player->usedown)
      {
      P_UseLines (player);
      player->usedown = true;
      }
    }
  else
    player->usedown = false;

  // cycle psprites

  P_MovePsprites (player);

  // Counters, time dependent power ups.

  // Strength counts up to diminish fade.

  if (player->powers[pw_strength])
    player->powers[pw_strength]++;

  // killough 1/98: Make idbeholdx toggle:

  if (player->powers[pw_invulnerability] > 0) // killough
    player->powers[pw_invulnerability]--;

  if (player->powers[pw_invisibility] > 0)    // killough
    if (! --player->powers[pw_invisibility] )
      player->mo->flags &= ~MF_SHADOW;

  if (player->powers[pw_infrared] > 0)        // killough
    player->powers[pw_infrared]--;

  if (player->powers[pw_ironfeet] > 0)        // killough
    player->powers[pw_ironfeet]--;

  if (player->damagecount)
    player->damagecount--;

  if (player->bonuscount)
    player->bonuscount--;

  // Handling colormaps.
  // killough 3/20/98: reformat to terse C syntax

  player->fixedcolormap = player->powers[pw_invulnerability] > 4*32 ||
    player->powers[pw_invulnerability] & 8 ? INVERSECOLORMAP :
    player->powers[pw_infrared] > 4*32 || player->powers[pw_infrared] & 8;
  }
