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
 * Hubs.
 *
 * Hubs are where there is more than one level, and links between them:
 * you can freely travel between all the levels in a hub and when you
 * return, the level should be exactly as it previously was.
 * As in Quake2/Half life/Hexen etc.
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include "doomstat.h"
#include "c_io.h"
#include "g_game.h"
#include "p_maputl.h"
#include "p_setup.h"
#include "p_spec.h"
#include "r_main.h"
#include "t_vari.h"
#include "z_zone.h"

void P_SavePlayerPosition(player_t *player, int sectag);
void P_RestorePlayerPosition(void);

#define MAXHUBLEVELS 128

typedef struct hublevel_s hublevel_t;
struct hublevel_s
{
  char levelname[8];
  char *tmpfile;        // temporary file holding the saved level
};

extern char *gamemapname;

// sf: set when we are changing to
//  another level in the hub
boolean hub_changelevel = false;  

hublevel_t hub_levels[MAXHUBLEVELS];
int num_hub_levels;

// sf: my own tmpnam (djgpp one doesn't work as i want it)

char *temp_hubfile(void)
{
  static int tmpfilenum = 0;
  char *new_tmpfilename;

  new_tmpfilename = malloc(10);

  sprintf(new_tmpfilename, "smmu%i.tmp", tmpfilenum++);

  return new_tmpfilename;  
}

void P_ClearHubs(void)
{
  int i;
  
  for(i=0; i<num_hub_levels; i++)
    if(hub_levels[i].tmpfile)
      remove(hub_levels[i].tmpfile);

  num_hub_levels = 0;

  // clear the hub_script
  T_ClearHubScript();
}

// seperate function: ensure that atexit is not set twice

void P_ClearHubsAtExit(void)
{
  static boolean atexit_set = false;

  if(atexit_set) return;   // already set

  atexit(P_ClearHubs);

  atexit_set = true;
}

void P_InitHubs(void)
{
  num_hub_levels = 0;

  P_ClearHubsAtExit();    // set P_ClearHubs to be called at exit
}

static hublevel_t *HublevelForName(char *name)
{
  int i;

  for(i=0; i<num_hub_levels; i++)
    {
      if(!strncasecmp(name, hub_levels[i].levelname, 8))
	return &hub_levels[i];
    }

  return NULL;  // not found
}

static hublevel_t *AddHublevel(char *levelname)
{
  strncpy(hub_levels[num_hub_levels].levelname, levelname, 8);
  hub_levels[num_hub_levels].tmpfile = NULL;

  return &hub_levels[num_hub_levels++];
}

// save the current level in the hub

static void SaveHubLevel(void)
{
  hublevel_t *hublevel;

  hublevel = HublevelForName(levelmapname);

  // create new hublevel if not been there yet
  if(!hublevel)
    hublevel = AddHublevel(levelmapname);

  // allocate a temp. filename for save
  if(!hublevel->tmpfile)
    hublevel->tmpfile = temp_hubfile();

  // G_SaveCurrentLevel(hublevel->tmpfile, "smmu hubs"); FIXME
}

extern void G_DoLoadLevel(void);                  // g_game.c

static void LoadHubLevel(char *levelname)
{
  hublevel_t *hublevel;

  hublevel = HublevelForName(levelname);

  if(!hublevel)
    {
      // load level normally
      gamemapname = strdup(levelname);
      G_DoLoadLevel();
    }
  else
    {
      // found saved level: reload
      //G_LoadSavedLevel(hublevel->tmpfile); FIXME
      hub_changelevel = true;
    }

  P_RestorePlayerPosition();
  
  wipegamestate = gamestate;
}

//
// G_LoadHubLevel
//
// sf: ga_loadhublevel is used instead of ga_loadlevel when
// we are loading a level into the hub for the first
// time.
//

static char new_hubmap[9];      // name of level to change to

void P_DoChangeHubLevel(void)
{
  hub_changelevel = true;

  V_SetLoading(0, "loading");
  
  SaveHubLevel();
  LoadHubLevel(new_hubmap);
}

void P_ChangeHubLevel(char *levelname)
{
  gameaction = ga_loadhublevel;
  strncpy(new_hubmap, levelname, 8);
}

void P_HubReborn(void)
{
  // restore player from savegame created when
  // we entered the level.
  // we do _not_ use hub_changelevel as we want to
  // restore all the data that was saved.

  hublevel_t *hublevel;

  hub_changelevel = false; // restore _all_ data
  
  hublevel = HublevelForName(levelmapname);

  if(!hublevel)
    {
      // load level normally
      G_DoLoadLevel();
    }
  else
    {
      // found saved level: reload
      //G_LoadSavedLevel(hublevel->tmpfile); FIXME
      hub_changelevel = true;
    }
}

void P_DumpHubs(void)
{
  int i;
  char tempbuf[10];

  for(i=0; i<num_hub_levels; i++)
    {
      strncpy(tempbuf, hub_levels[i].levelname, 8);
    }
}

static fixed_t          save_xoffset;
static fixed_t          save_yoffset;
static fixed_t          save_viewzoffset;
static mobj_t           save_mobj;
static int              save_sectag;
static player_t *       save_player;
static pspdef_t         save_psprites[NUMPSPRITES];

// save a player's position relative to a particular sector
void P_SavePlayerPosition(player_t *player, int sectag)
{
  sector_t *sec;
  int secnum;

  save_player = player;

  // save psprites whatever happens

  memcpy(save_psprites, player->psprites, sizeof(player->psprites));

  // save sector x,y offset

  save_sectag = sectag;

  if((secnum = P_FindSectorFromTag(sectag, -1)) < 0)
    {
      // invalid: sector not found
      save_sectag = -1;
      return;
    }
  
  sec = &sectors[secnum];

  // use soundorg x and y as 'centre' of sector

  save_xoffset = player->mo->x - sec->soundorg.x;
  save_yoffset = player->mo->y - sec->soundorg.y;

  // save viewheight

  save_viewzoffset = player->viewz
    - R_PointInSubsector(player->mo->x, player->mo->y)->sector->floorheight;  

  // save mobj so we can restore various bits of data

  memcpy(&save_mobj, player->mo, sizeof(mobj_t));
}

// restore the players position -- sector must be the same shape
void P_RestorePlayerPosition(void)
{
  sector_t *sec;
  int secnum;

  // we always save and restore the psprites

  memcpy(save_player->psprites, save_psprites, sizeof(save_player->psprites));

  // restore player position from x,y offset

  if(save_sectag == -1) return;      // no sector relativeness

  if((secnum = P_FindSectorFromTag(save_sectag, -1)) < 0)
    {
      // invalid: sector not found
      return;
    }
  
  sec = &sectors[secnum];

  // restore position

  P_UnsetThingPosition(save_player->mo);

  save_player->mo->x = sec->soundorg.x + save_xoffset;
  save_player->mo->y = sec->soundorg.y + save_yoffset;

  // restore various other things
  save_player->mo->angle = save_mobj.angle;
  save_player->mo->momx = save_mobj.momx;    // keep momentum
  save_player->mo->momy = save_mobj.momy;
  P_SetThingPosition(save_player->mo);

  // restore viewz

  save_player->viewz =
    R_PointInSubsector(save_player->mo->x,
		       save_player->mo->y)->sector->floorheight
    + save_viewzoffset;

  SaveHubLevel();
}
