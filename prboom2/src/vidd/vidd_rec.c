#ifdef COMPILE_VIDD

#include "vidd.h"
#include "../ViddSys/ViddSys.h"
#include "p_mobj.h"
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
#include "hu_lib.h"
#include "vidd_util.h"
#include "vidd_prop.h"
#include <stdio.h>
#include "i_sound.h"

static int recInProgress = 0;
static int lmpNum = -1;
static TQuickCodeBinding qcb;
static char curLmpName[VIDD_MAXSTRLEN];
static int levelStartTime = 0;
static int nextInfoPrintTime = 0;
static char infoMsg[VIDD_MAXSTRLEN];

static int getStateNum(void *state) {if (!state) return 0;return ((int)state - (int)&states[0]) / sizeof(state_t);}

//---------------------------------------------------------------------------
// VIDD_REC
//---------------------------------------------------------------------------
void VIDD_REC_reset() {
  recInProgress = 0;
  lmpNum = -1;
  levelStartTime = 0;
  nextInfoPrintTime = 0;
  singledemo = false;
  singletics = false;
	timingdemo = false;
	demoplayback = false;
  //nomusicparm = nosfxparm = false;
	//I_InitSound();
}

//---------------------------------------------------------------------------
int VIDD_REC_inProgress() { return recInProgress; }

//---------------------------------------------------------------------------
void VIDD_REC_open(const char *name) {
  if (!viddRecorder_open(name)) VIDD_exitWithError("Error opening VIDD system.");
  recInProgress = true;
  VIDD_attachQuickCodes(&qcb, 0);
}

//---------------------------------------------------------------------------
void VIDD_REC_openFromCommandLineParams() {\
  int p;
  char line[VIDD_MAXSTRLEN];
  const char *headerText = 
    "<vidd\n"
    "  name = \"temp.vidd\"\n"
    "  compress = \"gzip\"\n"
  ;
  const char *preLmpText = ">\n\n<lmp file=\"";
  const char *footerText =
    "\">\n"
    "</lmp>\n\n"
    "</vidd>\n"
  ;
  const char *iwadName;
  const char *tempXMLFilename = "./temp.xml";
  FILE *file;
  
  setCurDirToModuleDir();
  
  // write out the temp xml file
  file = fopen(tempXMLFilename, "w+t");

  // write out the header text (never changes)
  fwrite( headerText, sizeof(char), strlen(headerText), file );

  // write iwad
  iwadName = D_FindIWADFile();
  line[0] = 0;
  appendString(line, "  iwad = \"", VIDD_MAXSTRLEN);
  appendString(line, iwadName, VIDD_MAXSTRLEN);
  appendString(line, "\"\n", VIDD_MAXSTRLEN);
  fwrite( line, sizeof(char), strlen(line), file );
  
  // write pwads  
  if (p = M_CheckParm("-file")) {
    int i = 1;
    
    // clear it. NOT LEGAL!
    ((char*)myargv[p])[0] = 0;
      
    while (++p != myargc && *myargv[p] != '-') {
      line[0] = 0;
      appendString(line, "  pwad", VIDD_MAXSTRLEN);
      appendString(line, getIntAsString(i), VIDD_MAXSTRLEN);
      appendString(line, " = ", VIDD_MAXSTRLEN);
      appendString(line, strdup(myargv[p]), VIDD_MAXSTRLEN);
      appendString(line, "\"\n", VIDD_MAXSTRLEN);
            
      fwrite( line, sizeof(char), strlen(line), file );
      
      // clear it. NOT LEGAL!
      ((char*)myargv[p])[0] = 0;

      i++;
    }
  }
  
  // write preLmpText (never changes)
  fwrite( preLmpText, sizeof(char), strlen(preLmpText), file );
  
  // write lmp
  if (p = M_CheckParm("-playdemo")) {
    char lmpName[PATH_MAX+1];
    strcpy(lmpName, myargv[p+1]);
    AddDefaultExtension(lmpName, ".lmp");
    fwrite( lmpName, sizeof(char), strlen(lmpName), file );
    // clear it. NOT LEGAL!
    ((char*)myargv[p])[0] = 0;
    ((char*)myargv[p+1])[0] = 0;    
  }
  
  // write footer
  fwrite( footerText, sizeof(char), strlen(footerText), file );
  
  fclose(file);
  
  VIDD_REC_open(tempXMLFilename);
}

//---------------------------------------------------------------------------
void VIDD_REC_initDuringSecondParamCheck() {
  int i;
  const char *lmpName;
  
  //nodrawers = true;
  //nomusicparm = nosfxparm = true;
  lmpNum = -1;
  
  // load up the lmps
  for (i=0;;i++) {
    lmpName = VIDD_getNumberedAttrib("lmp", i, "_file");
    if (!lmpName[0]) break;
    D_AddFile(lmpName, source_lmp);
  }
  
  if (i != 0) return;
  
  // couldn't load any demos
  VIDD_exitWithError("No lmp files to queue.");
}

//---------------------------------------------------------------------------

  
//---------------------------------------------------------------------------
int VIDD_REC_checkDemoStatus() {
  
  if (!VIDD_REC_inProgress()) return false;

  lmpNum++;
  
  // load the next lmp if possible
  copyString(curLmpName, VIDD_getNumberedAttrib("lmp", lmpNum, "_file"), VIDD_MAXSTRLEN);
  
  if (!curLmpName[0]) {
    return false;
  }  
  
  // que up the demo
  G_DeferedPlayDemo(curLmpName);
  singledemo = false;
  singletics = true;
	timingdemo = true;
	demoplayback = true;
	
	return true;
}  

//---------------------------------------------------------------------------
void VIDD_REC_registerSound(int soundId, void *origin) {
  mobj_t *mobj;
  sector_t *sector;
  int i;
  TVIDDElementHandle handle;
  
  if (!origin) {
    // sourceless sound
    handle.type = VET_NONE;
    handle.id = 0;
    viddRecorder_registerSound(soundId, handle);
    return;
  }
  
  mobj = (mobj_t*)origin;
  handle.type = qcb.TYPE_MOBJ;
  handle.id = (TVIDDElementId)mobj;
  
  if (!viddRecorder_getElementExists(handle)) {
    // see if it's a sector making the noise
    for (i=0, sector=sectors; i<numsectors; i++, sector++) {
      if (origin != (void*)&sector->soundorg) continue;
      handle.type = qcb.TYPE_SECTOR;
      handle.id = (TVIDDElementId)i;
      break;
    }    
  }
  viddRecorder_registerSound(soundId, handle);
}

//---------------------------------------------------------------------------
void VIDD_REC_registerLevelLoad(int episode, int map, int skill) {
  TVIDDTriggeredEvent event;
  static char eventStr[VIDD_MAXSTRLEN];
  static char segmentName[VIDD_MAXSTRLEN];
  char *skillStr;
  
  // build the verb and subject string to describe this loadlevel event
  skillStr = skill == 0 ? "0" : skill == 1 ? "1" : skill == 2 ? "2" : skill == 3 ? "uv" : "nm";
  snprintf(eventStr, VIDD_MAXSTRLEN-1, "%i %i %i", episode, map, skill);
  if (!strcmp(VIDD_getIWad(), "doom2.wad")) snprintf(segmentName, VIDD_MAXSTRLEN-1, "M%i%s", map, skillStr);
  else snprintf(segmentName, VIDD_MAXSTRLEN-1, "E%iM%i%s", episode, map, skillStr);
  
  event.verb = "loadlevel";
  event.subject = eventStr;
  
  viddRecorder_beginSegment(segmentName);
  viddRecorder_registerEvent(event);  
  
  // set some default info stuff
  {
  char attributePrefix[VIDD_MAXSTRLEN];
  if (!lmpNum && viddSys_getVersion() >= 110) sprintf(attributePrefix, "lmp_");
  else sprintf(attributePrefix, "lmp%i_", lmpNum);
  viddSys_copyAttributesFromGlobalToCurSegment(attributePrefix);
  }
  
  levelStartTime = gametic;
}

//---------------------------------------------------------------------------
void VIDD_REC_registerElementDestruction(void *ptr) {
  TVIDDElementHandle handle = { qcb.TYPE_MOBJ, (TVIDDElementId)ptr };
  viddRecorder_registerElementDestruction(handle);
}

//---------------------------------------------------------------------------
void VIDD_REC_updatePlayerMessage(void *player, const char *msg) {
  // called from the hud code
  TVIDDElementHandle handle;  
  handle.type = qcb.TYPE_PLAYER;
  handle.id = (TVIDDElementId)player;
  viddRecorder_setStringProp(handle, qcb.PLAYER_MESSAGE, msg);
}

//---------------------------------------------------------------------------
void VIDD_REC_endFrame() {  
  int i = 0, t;
  thinker_t *thinker;
  mobj_t *mobj;
  TVIDDElementHandle eh;
  sector_t *sector;
  player_t *player;
  int bits;
  side_t *side;

  if (lmpNum < 0) {
    // run the initial check to start the first demo,
    // which must happen after everything's been initialized.
    VIDD_REC_checkDemoStatus();
    return;
  }
  
  // update the player's properties  
  eh.type = qcb.TYPE_PLAYER;
  player = &players[consoleplayer];
  eh.id = (TVIDDElementId)player;
  
  if (!player->mo) {
    // intermission
    viddRecorder_endFrame();    
    return;
  }
  
  viddRecorder_setVectorProp(eh, qcb.MOBJ_POS, player->mo->x, player->mo->y, player->mo->z);
  viddRecorder_setIntProp(eh, qcb.MOBJ_ANGLE, player->mo->angle);
  viddRecorder_setIntProp(eh, qcb.MOBJ_SPRITE, player->mo->sprite);
  viddRecorder_setIntProp(eh, qcb.MOBJ_STATE, getStateNum(player->mo->state));
  viddRecorder_setIntProp(eh, qcb.PLAYER_VIEWZ, player->viewz);
  viddRecorder_setIntProp(eh, qcb.PLAYER_HEALTH, player->health);
  viddRecorder_setIntProp(eh, qcb.PLAYER_ARMORPOINTS, player->armorpoints);
  viddRecorder_setIntProp(eh, qcb.PLAYER_ARMORTYPE, player->armortype);
  viddRecorder_setIntProp(eh, qcb.PLAYER_BACKPACK, player->backpack);
  bits = 0;
  for (t=0; t<NUMWEAPONS; t++) if (player->weaponowned[t]) bits |= (1<<t);
  viddRecorder_setIntProp(eh, qcb.PLAYER_WEAPONS, bits);
  bits = 0;
  for (t=0; t<NUMCARDS; t++) if (player->cards[t]) bits |= (1<<t);
  viddRecorder_setIntProp(eh, qcb.PLAYER_CARDS, bits);
  viddRecorder_setIntProp(eh, qcb.PLAYER_POWER_BIOSUIT, player->powers[pw_ironfeet]);
  viddRecorder_setIntProp(eh, qcb.PLAYER_POWER_INVULN, player->powers[pw_invulnerability]);
  viddRecorder_setIntProp(eh, qcb.PLAYER_POWER_INVIS, player->powers[pw_invisibility]);
  viddRecorder_setIntProp(eh, qcb.PLAYER_POWER_ALLMAP, player->powers[pw_allmap]);
  viddRecorder_setIntProp(eh, qcb.PLAYER_POWER_GOGGLES, player->powers[pw_infrared]);
  viddRecorder_setIntProp(eh, qcb.PLAYER_POWER_BERSERK, player->powers[pw_strength]);
  viddRecorder_setIntProp(eh, qcb.PLAYER_KILLCOUNT, player->killcount);
  viddRecorder_setIntProp(eh, qcb.PLAYER_ITEMCOUNT, player->itemcount);
  viddRecorder_setIntProp(eh, qcb.PLAYER_SECRETCOUNT, player->secretcount);
  viddRecorder_setIntProp(eh, qcb.PLAYER_DAMAGECOUNT, player->damagecount);
  viddRecorder_setIntProp(eh, qcb.PLAYER_BONUSCOUNT, player->bonuscount);
  viddRecorder_setIntProp(eh, qcb.PLAYER_EXTRALIGHT, player->extralight);  
  viddRecorder_setIntProp(eh, qcb.PLAYER_FIXEDCOLORMAP, player->fixedcolormap); 
  viddRecorder_setIntProp(eh, qcb.PLAYER_COLORMAP, player->colormap);
  viddRecorder_setIntProp(eh, qcb.PLAYER_SCREENSPRITE0_STATE, getStateNum(player->psprites[0].state));
  viddRecorder_setIntProp(eh, qcb.PLAYER_SCREENSPRITE0_SX, player->psprites[0].sx);
  viddRecorder_setIntProp(eh, qcb.PLAYER_SCREENSPRITE0_SY, player->psprites[0].sy);
  viddRecorder_setIntProp(eh, qcb.PLAYER_SCREENSPRITE1_STATE, getStateNum(player->psprites[1].state));
  viddRecorder_setIntProp(eh, qcb.PLAYER_SCREENSPRITE1_SX, player->psprites[1].sx);
  viddRecorder_setIntProp(eh, qcb.PLAYER_SCREENSPRITE1_SY, player->psprites[1].sy);
  viddRecorder_setIntProp(eh, qcb.PLAYER_ATTACKERID, (int)player->attacker);
  viddRecorder_setIntProp(eh, qcb.PLAYER_AMMO_CLIP, player->ammo[am_clip]);
  viddRecorder_setIntProp(eh, qcb.PLAYER_AMMO_SHELL, player->ammo[am_shell]);
  viddRecorder_setIntProp(eh, qcb.PLAYER_AMMO_MISSLE, player->ammo[am_misl]);
  viddRecorder_setIntProp(eh, qcb.PLAYER_AMMO_CELL, player->ammo[am_cell]);
  viddRecorder_setIntProp(eh, qcb.PLAYER_READYWEAPON, player->readyweapon);
  
  // update thinker/mobj properties
  thinker = 0;
  i=0;
  while ((thinker = P_NextThinker(thinker,th_all)) != NULL) {
       
    if (!thinker) break;    
    if (thinker->function != P_MobjThinker) continue;   
    
    eh.type = qcb.TYPE_MOBJ;
    eh.id = (TVIDDElementId)thinker;      
      
    mobj = (mobj_t*)thinker;
    if (!mobj->state || mobj->state == &states[0]) {
      VIDD_REC_registerElementDestruction(thinker);
      continue;
    }
    
    viddRecorder_setVectorProp(eh, qcb.MOBJ_POS, mobj->x, mobj->y, mobj->z);
    viddRecorder_setIntProp(eh, qcb.MOBJ_ANGLE, mobj->angle);
    viddRecorder_setIntProp(eh, qcb.MOBJ_TYPE, mobj->type);    
    viddRecorder_setIntProp(eh, qcb.MOBJ_SPRITE, mobj->sprite);      
    viddRecorder_setIntProp(eh, qcb.MOBJ_STATE, getStateNum(mobj->state));
    
    i++;
  }  
  
  // update sectors
  eh.type = qcb.TYPE_SECTOR; 
  for (i=0, sector=sectors; i<numsectors; i++, sector++){
    eh.id = (TVIDDElementId)i;
    viddRecorder_setIntProp(eh, qcb.SECTOR_FLOORHEIGHT, sector->floorheight);
    viddRecorder_setIntProp(eh, qcb.SECTOR_CEILINGHEIGHT, sector->ceilingheight);
    viddRecorder_setIntProp(eh, qcb.SECTOR_LIGHTLEVEL, sector->lightlevel);
    viddRecorder_setIntProp(eh, qcb.SECTOR_FLOORPIC, sector->floorpic);
    viddRecorder_setIntProp(eh, qcb.SECTOR_CEILINGPIC, sector->ceilingpic);
  }
  
  // update sides
  eh.type = qcb.TYPE_SIDE;
  for (i=0, side=sides; i<numsides; i++, side++) {
    if (!side->special) continue;
    
    eh.id = (TVIDDElementId)i;
    viddRecorder_setIntProp(eh, qcb.SIDE_TOPTEX, side->toptexture);
    viddRecorder_setIntProp(eh, qcb.SIDE_BOTTOMTEX, side->bottomtexture);
    viddRecorder_setIntProp(eh, qcb.SIDE_MIDTEX, side->midtexture);  
  }

  // update globals
  eh.type = qcb.TYPE_GLOBAL;
  eh.id = 0;
  viddRecorder_setIntProp(eh, qcb.GLOBAL_TOTALENEMIES, totalkills);
  viddRecorder_setIntProp(eh, qcb.GLOBAL_TOTALSECRETS, totalsecret);
  viddRecorder_setIntProp(eh, qcb.GLOBAL_TOTALITEMS, totalitems);
      
  viddRecorder_endFrame();
  
  infoMsg[0] = 0;    
  
  if (gametic >= nextInfoPrintTime) {
    // print info to console
    char segmentTime[VIDD_MAXSTRLEN];
    char totalTime[VIDD_MAXSTRLEN];
    const char *msg =
      "---------------------------------------------------\n"
      "                                 VIDD\n"      
      "     Version Independent Doom Demo System\n"
      "---------------------------------------------------\n"
      "\n"
      "Record in progress:\n"
      "\n"
      "  \"%s\"    %s\n"
      "                                                  %s\n"
      "                                                (%s)\n"
      "\n"
      "\n"
      " Playback will begin when complete.\n"
      "\n"
      "---------------------------------------------------\n"
      "                   PRESS ANY KEY TO EXIT\n"
      "---------------------------------------------------\n"      
    ;
    
    copyString(segmentTime, getTimeAsString((gametic-levelStartTime)*1000/TICRATE, false), VIDD_MAXSTRLEN);
    copyString(totalTime, getTimeAsString(gametic*1000/TICRATE, false), VIDD_MAXSTRLEN);
    
    snprintf(infoMsg, VIDD_MAXSTRLEN, msg, curLmpName, viddSys_getCurSegmentName(), segmentTime, totalTime);
    
    nextInfoPrintTime = gametic + 10*TICRATE;    
  }
}

//---------------------------------------------------------------------------
const char *VIDD_REC_getInfoMsg() { return infoMsg; }

//---------------------------------------------------------------------------


#endif // #ifdef COMPILE_VIDD
