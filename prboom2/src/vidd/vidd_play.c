#ifdef COMPILE_VIDD

#include "p_mobj.h"
#include "vidd.h"
#include "../ViddSys/ViddSys.h"
#include "i_main.h"
#include "lprintf.h"
#include "info.h"
#include "p_tick.h"
#include "d_think.h"
#include "g_game.h"
#include "doomdef.h"
#include "doomstat.h"
#include "p_spec.h"
#include "p_maputl.h"
#include "p_map.h"
#include "r_main.h"
#include "p_tick.h"
#include "s_sound.h"
#include "sounds.h"
#include "p_user.h"
#include "m_argv.h"
#include "p_setup.h"
#include "g_game.h"
#include "d_main.h"
#include "i_system.h"
#include "vidd_prop.h"

void P_SpawnPlayer(int n, const mapthing_t *mthing);

static int playInProgress = 0;
static int levelLoaded = 0;
static TQuickCodeBinding qcb;

static int getStateNum(void *state) {if (!state) return 0;return ((int)state - (int)&states[0]) / sizeof(state_t);}

//---------------------------------------------------------------------------
// VIDD_PLAY
//---------------------------------------------------------------------------
int VIDD_PLAY_inProgress() { return playInProgress; }

//---------------------------------------------------------------------------
void VIDD_PLAY_open(const char *filename) {
  if (!viddPlayer_open(filename)) VIDD_exitWithError("Error opening VIDD system.");
  playInProgress = true;
  VIDD_attachQuickCodes(&qcb, 1);
}

//---------------------------------------------------------------------------
void VIDD_PLAY_initDuringSecondParamCheck() {
}

//---------------------------------------------------------------------------
void VIDD_PLAY_doWorldDone() {
  if (!VIDD_PLAY_inProgress()) return;
  VIDD_MENU_doWorldDone(); 
  levelLoaded = 0;
}

//---------------------------------------------------------------------------
int VIDD_PLAY_getSoundPitch() {
  return VIDD_MENU_getSoundPitch();
}

//---------------------------------------------------------------------------
int VIDD_PLAY_fillIntProperty(
  const TVIDDElementHandle elementHandle, 
  const TVIDDElementProperty property,
  const TVIDDAnimSolver solver,
  int *value) {
  int prevValue = *value;
  *value = viddPlayer_getIntProp(elementHandle, property, solver);
  return (*value != prevValue);
}

//---------------------------------------------------------------------------
int VIDD_PLAY_fillShortProperty(
  const TVIDDElementHandle elementHandle, 
  const TVIDDElementProperty property,
  const TVIDDAnimSolver solver,
  short *value) {
  short prevValue = *value;
  *value = (short)viddPlayer_getIntProp(elementHandle, property, solver);
  return (*value != prevValue);
}

//---------------------------------------------------------------------------
void VIDD_PLAY_endFrame() {
  int numElements;
  TVIDDElementHandle handle, attackerHandle;
  TVIDDFixedVector vec;
  int i,t;
  mapthing_t mapthing;
  mobjtype_t mobjtype;
  mobj_t *mobj;
  player_t *player;
  void *userData;
  sector_t *sector;
  statenum_t statenum;
  int numSounds;
  TVIDDTriggeredSound sound;
  unsigned int bits;
  const char *levelname;
  side_t *side;

  // check for level load
  levelname = viddPlayer_getTriggeredEventSubject("loadlevel");
  if (strlen(levelname)) {
    int episode, map, skill;
    sscanf(levelname, "%i %i %i", &episode, &map, &skill);
    //if (episode != gameepisode || map != gamemap) {
      G_InitNew(skill, episode, map);
      gamestate = GS_LEVEL;
      advancedemo = false;
      levelLoaded = 1;
      
      // always create both players here; one for the demoguy
      // and one for the phantom setting.
      mapthing.x = 0;
      mapthing.y = 0;
      mapthing.angle = 0;    
      mapthing.type = 1; 
      playeringame[0] = true; 
      consoleplayer = 0;
      displayplayer = 0;
      P_SpawnPlayer(0, &mapthing);
      
      // spawn second player
      mapthing.x = 0;
      mapthing.y = 0;
      mapthing.angle = 0;    
      mapthing.type = 2; 
      playeringame[1] = true;
      P_SpawnPlayer(1, &mapthing);

      // make them both invisible
      VIDD_PLAY_setPlayerVisibility(0, 0);
      VIDD_PLAY_setPlayerVisibility(1, 0);
    //}
  }

  if (!levelLoaded) return;
  
  // figure out who to remove
  numElements = viddPlayer_getNumElements(VVS_JUSTDISAPPEARED);
  for (i=0; i<numElements; i++) {
    if (viddPlayer_getElementType_JustDisappeared(i) != qcb.TYPE_MOBJ) continue;
    mobj = (mobj_t*)viddPlayer_getElementUserData_JustDisappeared(i);
    viddPlayer_setElementUserData_JustDisappeared(i, 0);
    if (!mobj) continue;
    P_RemoveMobj(mobj);
  }
   
  // figure out who to spawn
  numElements = viddPlayer_getNumElements(VVS_JUSTAPPEARED);  
  for (i=0; i<numElements; i++) {
    handle = viddPlayer_getElementHandle_JustAppeared(i);
    
    vec = viddPlayer_getVectorProp(handle, qcb.MOBJ_POS, VIDD_MENU_getLerpSolver());
    mobjtype = (mobjtype_t)viddPlayer_getIntProp(handle, qcb.MOBJ_TYPE, VAS_STEP);      
  
    if (handle.type == qcb.TYPE_MOBJ) {
      mobj = P_SpawnMobj(vec.x, vec.y, vec.z, mobjtype);
      viddPlayer_setElementUserData(handle, (void*)mobj);
    }
    else if (handle.type == qcb.TYPE_PLAYER) {     
      player = &players[VIDD_MENU_getFirstUsablePlayerIndex()];
      playeringame[VIDD_MENU_getFirstUsablePlayerIndex()] = true;
      VIDD_PLAY_setPlayerVisibility(VIDD_MENU_getFirstUsablePlayerIndex(), 1);
      viddPlayer_setElementUserData(handle, 0);
      mobj = player->mo;
    }
    else continue;

    mobj->thinker.function = 0; 
    mobj->tics = -1; 
  }
 
  // update everybody's properties 
  numElements = viddPlayer_getNumElements(VVS_VISIBLE);
  for (i=0; i<numElements; i++) {
    handle = viddPlayer_getElementHandle_Visible(i);    
    userData = viddPlayer_getElementUserData(handle);
    
    if (handle.type == qcb.TYPE_PLAYER) {
      if ((int)userData < 0 || (int)userData >= MAXPLAYERS) continue;
      player = &players[VIDD_MENU_getFirstUsablePlayerIndex() + (int)userData];
      player->viewz = viddPlayer_getIntProp(handle, qcb.PLAYER_VIEWZ, VIDD_MENU_getLerpSolver());
      player->health = viddPlayer_getIntProp(handle, qcb.PLAYER_HEALTH, VAS_STEP);
      player->armorpoints = viddPlayer_getIntProp(handle, qcb.PLAYER_ARMORPOINTS, VAS_STEP);
      player->armortype = viddPlayer_getIntProp(handle, qcb.PLAYER_ARMORTYPE, VAS_STEP);
      player->backpack = viddPlayer_getIntProp(handle, qcb.PLAYER_BACKPACK, VAS_STEP);
      if (player->backpack) {
        player->maxammo[am_clip] = 400;
        player->maxammo[am_shell] = 100;
        player->maxammo[am_cell] = 600;
        player->maxammo[am_misl] = 100;
      }
      else {
        player->maxammo[am_clip] = 200;
        player->maxammo[am_shell] = 50;
        player->maxammo[am_cell] = 300;
        player->maxammo[am_misl] = 50;
      }
      bits = viddPlayer_getIntProp(handle, qcb.PLAYER_WEAPONS, VAS_STEP);
      for (t=0; t<NUMWEAPONS; t++) {
        // weaponowned is actually treated as an index into an array
        // that must be 0 or 1.. it's not a boolean in the proper sense
        // and 0xff doesn't just get treated as "true".
        player->weaponowned[t] = (bits & (1<<t)) ? 1 : 0;
      }
      bits = viddPlayer_getIntProp(handle, qcb.PLAYER_CARDS, VAS_STEP);
      for (t=0; t<NUMCARDS; t++) player->cards[t] =  (bits & (1<<t)) ? 1 : 0;
      player->powers[pw_ironfeet] = viddPlayer_getIntProp(handle, qcb.PLAYER_POWER_BIOSUIT, VAS_STEP);
      player->powers[pw_invulnerability] = viddPlayer_getIntProp(handle, qcb.PLAYER_POWER_INVULN, VAS_STEP);
      player->powers[pw_invisibility] = viddPlayer_getIntProp(handle, qcb.PLAYER_POWER_INVIS, VAS_STEP);
      player->powers[pw_allmap] = viddPlayer_getIntProp(handle, qcb.PLAYER_POWER_ALLMAP, VAS_STEP);
      player->powers[pw_infrared] = viddPlayer_getIntProp(handle, qcb.PLAYER_POWER_GOGGLES, VAS_STEP);
      player->powers[pw_strength] = viddPlayer_getIntProp(handle, qcb.PLAYER_POWER_BERSERK, VAS_STEP);
      player->killcount = viddPlayer_getIntProp(handle, qcb.PLAYER_KILLCOUNT, VAS_STEP);
      player->itemcount = viddPlayer_getIntProp(handle, qcb.PLAYER_ITEMCOUNT, VAS_STEP);
      player->secretcount = viddPlayer_getIntProp(handle, qcb.PLAYER_SECRETCOUNT, VAS_STEP);
      player->damagecount = viddPlayer_getIntProp(handle, qcb.PLAYER_DAMAGECOUNT, VAS_STEP);
      player->bonuscount = viddPlayer_getIntProp(handle, qcb.PLAYER_BONUSCOUNT, VAS_STEP);
      player->extralight = viddPlayer_getIntProp(handle, qcb.PLAYER_EXTRALIGHT, VAS_STEP);  
      player->fixedcolormap = viddPlayer_getIntProp(handle, qcb.PLAYER_FIXEDCOLORMAP, VAS_STEP); 
      player->colormap = viddPlayer_getIntProp(handle, qcb.PLAYER_COLORMAP, VAS_STEP);      
      statenum = viddPlayer_getIntProp(handle, qcb.PLAYER_SCREENSPRITE0_STATE, VAS_STEP);
      if (statenum) player->psprites[0].state = &states[statenum];
      else player->psprites[0].state = 0;
      player->psprites[0].sx = viddPlayer_getIntProp(handle, qcb.PLAYER_SCREENSPRITE0_SX, VIDD_MENU_getLerpSolver());
      player->psprites[0].sy = viddPlayer_getIntProp(handle, qcb.PLAYER_SCREENSPRITE0_SY, VIDD_MENU_getLerpSolver());
      statenum = viddPlayer_getIntProp(handle, qcb.PLAYER_SCREENSPRITE1_STATE, VAS_STEP);
      if (statenum) player->psprites[1].state = &states[statenum];
      else player->psprites[1].state = 0;
      player->psprites[1].sx = viddPlayer_getIntProp(handle, qcb.PLAYER_SCREENSPRITE1_SX, VIDD_MENU_getLerpSolver());
      player->psprites[1].sy = viddPlayer_getIntProp(handle, qcb.PLAYER_SCREENSPRITE1_SY, VIDD_MENU_getLerpSolver());
      player->ammo[am_clip] = viddPlayer_getIntProp(handle, qcb.PLAYER_AMMO_CLIP, VAS_STEP);
      player->ammo[am_shell] = viddPlayer_getIntProp(handle, qcb.PLAYER_AMMO_SHELL, VAS_STEP);
      player->ammo[am_misl] = viddPlayer_getIntProp(handle, qcb.PLAYER_AMMO_MISSLE, VAS_STEP);
      player->ammo[am_cell] = viddPlayer_getIntProp(handle, qcb.PLAYER_AMMO_CELL, VAS_STEP); 
      player->readyweapon = viddPlayer_getIntProp(handle, qcb.PLAYER_READYWEAPON, VAS_STEP); 
      player->message = viddPlayer_getStringProp(handle, qcb.PLAYER_MESSAGE);
      attackerHandle.id = viddPlayer_getIntProp(handle, qcb.PLAYER_ATTACKERID, VAS_STEP);
      attackerHandle.type = qcb.TYPE_MOBJ;
      player->attacker = (mobj_t*)viddPlayer_getElementUserData(attackerHandle);
      mobj = player->mo;
      if (!mobj) {
        // intermission
        continue;
      }
    }
    else if (handle.type == qcb.TYPE_MOBJ) {
      if (!userData) continue;
      mobj = (mobj_t*)userData;
    }
    else if (handle.type == qcb.TYPE_SECTOR) {
      sector = &sectors[handle.id];
      sector->floorpic = viddPlayer_getIntProp(handle, qcb.SECTOR_FLOORPIC, VAS_STEP);
      sector->ceilingpic = viddPlayer_getIntProp(handle, qcb.SECTOR_CEILINGPIC, VAS_STEP);   
      if (
          VIDD_PLAY_fillIntProperty(handle, qcb.SECTOR_FLOORHEIGHT, VIDD_MENU_getLerpSolver(), &sector->floorheight) ||
          VIDD_PLAY_fillIntProperty(handle, qcb.SECTOR_CEILINGHEIGHT, VIDD_MENU_getLerpSolver(), &sector->ceilingheight) ||
          VIDD_PLAY_fillShortProperty(handle, qcb.SECTOR_LIGHTLEVEL, VAS_STEP, &sector->lightlevel)
       ) {
         sector->floordata = sector->ceilingdata = sector->lightingdata = (void*)1;
      }
      else sector->floordata = sector->ceilingdata = sector->lightingdata = (void*)0;
      continue;
    }
    else if (handle.type == qcb.TYPE_SIDE) {
      side = &sides[handle.id];
      side->midtexture = viddPlayer_getIntProp(handle, qcb.SIDE_MIDTEX, VAS_STEP);
      side->bottomtexture = viddPlayer_getIntProp(handle, qcb.SIDE_BOTTOMTEX, VAS_STEP);
      side->toptexture = viddPlayer_getIntProp(handle, qcb.SIDE_TOPTEX, VAS_STEP);
      continue;  
    }
    else if (handle.type == qcb.TYPE_GLOBAL) {
      totalkills = viddPlayer_getIntProp(handle, qcb.GLOBAL_TOTALENEMIES, VAS_STEP);
      totalitems = viddPlayer_getIntProp(handle, qcb.GLOBAL_TOTALITEMS, VAS_STEP);
      totalsecret = viddPlayer_getIntProp(handle, qcb.GLOBAL_TOTALSECRETS, VAS_STEP);
      continue;
    }
    else continue;
   
    P_UnsetThingPosition(mobj);    
    vec = viddPlayer_getVectorProp(handle, qcb.MOBJ_POS, VIDD_MENU_getLerpSolver());
    mobj->x = (fixed_t)vec.x;
    mobj->y = (fixed_t)vec.y;
    mobj->z = (fixed_t)vec.z;
    P_SetThingPosition(mobj);
 
    mobj->angle = (angle_t)viddPlayer_getIntProp(handle, qcb.MOBJ_ANGLE, VIDD_MENU_getLerpSolver());
    mobj->sprite = (spritenum_t)viddPlayer_getIntProp(handle, qcb.MOBJ_SPRITE, VAS_STEP);
    mobj->type = (mobjtype_t)viddPlayer_getIntProp(handle, qcb.MOBJ_TYPE, VAS_STEP);    

    statenum = (statenum_t)viddPlayer_getIntProp(handle, qcb.MOBJ_STATE, VAS_STEP);
    if (statenum) {
      mobj->state = &states[statenum];
      mobj->frame = mobj->state->frame;
    }
    else mobj->state = 0;
  }
  
  // play sounds
  numSounds = viddPlayer_getNumTriggeredSounds();
  for (i=0; i<numSounds; i++) {
    sound = viddPlayer_getTriggeredSound(i);
    if (sound.sourceElementHandle.type == qcb.TYPE_SECTOR && sound.sourceElementHandle.id < (unsigned int)numsectors) {
      mobj = (mobj_t*)&sectors[sound.sourceElementHandle.id].soundorg;
      if (mobj) S_StartSound(mobj, sound.soundId);
    }
    else if (sound.sourceElementHandle.type == qcb.TYPE_MOBJ) {
      if (sound.sourceElementHandle.id) {
        mobj = (mobj_t*)viddPlayer_getElementUserData(sound.sourceElementHandle);
        if (mobj) S_StartSound(mobj, sound.soundId);
      }
    }
    else {
      S_StartSound(0, sound.soundId);
    }
  }
}

//---------------------------------------------------------------------------
void VIDD_PLAY_setPlayerVisibility(int playerIndex, int visible) {
  P_UnsetThingPosition(players[playerIndex].mo);
  if (visible) players[playerIndex].mo->flags &= ~MF_NOSECTOR;
  else players[playerIndex].mo->flags |= MF_NOSECTOR;    
  P_SetThingPosition(players[playerIndex].mo);  
}


#endif // #ifdef COMPILE_VIDD
