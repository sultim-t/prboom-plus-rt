#ifdef COMPILE_VIDD

#include "vidd.h"
#include "../../ViddSys/ViddSys.h"
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
#include "math.h"
#include "vidd_util.h"
#include "am_map.h"

//---------------------------------------------------------------------------
typedef enum {
  MP_VCR,
  MP_SPEED,
  MP_POS,
  MP_SEGMENT,
  MP_SMOOTHING,
  MP_PHANTOM,
  MP_INTERMISSION,
  MP_ANNOTATIONS,
  MP_MAXCONTROLS
} TMenuStateActiveControl;

//---------------------------------------------------------------------------
typedef struct {
  float playbackSpeed;
  float maxPlaybackSpeed;
  float minPlaybackSpeed;
  
  TVIDDTimeUnit playbackTime;
  TVIDDTimeUnit lastTime;

  int interfaceDisplayCount;  
  int vcrSpeedupCount;
  int segmentChangeCount;
  
  TMenuStateActiveControl activeControl;
  
  int interpolationEnabled;
  int phantomActive;
  
  int intermissionsEnabled;
  int intermissionActive;
  int intermissionAdvanceCount;
  
  int highlightedSegment;
  
  char annotationText[VIDD_MAXSTRLEN];
  int annotationActive;
  int annotationsEnabled;
  
} TMenuState;

//---------------------------------------------------------------------------
static TMenuState menuState;
static menuFirstInit = 1;

//---------------------------------------------------------------------------
void VIDD_MENU_init() {
  menuState.lastTime = 0;
  menuState.playbackTime = viddPlayer_getFirstFrame()+1;  
  menuState.interfaceDisplayCount = 0;
  menuState.maxPlaybackSpeed = 5;
  menuState.minPlaybackSpeed = -3;
  menuState.highlightedSegment = viddPlayer_getCurSegmentIndex();
  menuState.segmentChangeCount = 0;
  menuState.intermissionActive = 0;
  menuState.intermissionAdvanceCount = 0;
  menuState.annotationActive = 0;

  if (!menuFirstInit) return;
    
  menuState.activeControl = MP_VCR;
  menuState.interpolationEnabled = 1;
  menuState.phantomActive = 0;
  menuState.intermissionsEnabled = 1;
  menuState.playbackSpeed = 1;
  menuState.annotationsEnabled = 1;
  
  menuFirstInit = 0;
}

//---------------------------------------------------------------------------
int VIDD_MENU_getAdvanceIntermission() {
  return (menuState.intermissionAdvanceCount > TICRATE*4);
}

//---------------------------------------------------------------------------
void VIDD_MENU_beginFrame(unsigned int time) {
  int lastPlaybackTime;
  int lastMenuTime;
  
  // rotate the lastMenuTime here so we can return from multiple places
  lastMenuTime = menuState.lastTime;
  menuState.lastTime = time;
  
  if (menuState.annotationActive) return;

  if (menuState.intermissionActive) {
    menuState.intermissionAdvanceCount++;
    return;
  }
  
  // check for annotations
  if (menuState.annotationsEnabled && strlen(viddPlayer_getTriggeredEventSubject("annotate"))) {
    menuState.annotationText[0] = 0;

    appendString(menuState.annotationText, 
      VIDD_MENU_getCurSegmentAttributesString(), VIDD_MAXSTRLEN);
 
    appendString(menuState.annotationText, "\n\n", VIDD_MAXSTRLEN);
    
    appendColoredString(
      menuState.annotationText, 
      viddPlayer_getTriggeredEventSubject("annotate"), 
      COLOR_WHITE, 
      VIDD_MAXSTRLEN
    );
 
    appendColoredString(
      menuState.annotationText,
      "\n\n[PRESS ANY KEY TO CONTINUE]",
      COLOR_GREEN, 
      VIDD_MAXSTRLEN
    );
    
    menuState.annotationActive = 1; 
    return;
  }

  // not annotating
  
  if (menuState.activeControl == MP_VCR) {
    // speed up the fastforward after a few seconds
    if (menuState.vcrSpeedupCount > 0 && --menuState.vcrSpeedupCount == 0) {
      menuState.playbackSpeed *= 2;    
      if (menuState.playbackSpeed < 5) menuState.vcrSpeedupCount = 4*TICRATE;
    }
  }
  if (menuState.activeControl == MP_SEGMENT) {
    // decrement the segmentchange count and load up a new segment when it triggers
    if (
      menuState.highlightedSegment != viddPlayer_getCurSegmentIndex() &&
      menuState.segmentChangeCount > 0 && 
      --menuState.segmentChangeCount == 0
    ) {
      // load the highlighted segment
      if (viddPlayer_loadSegment(menuState.highlightedSegment)) {
        // new segment successfully loaded
        VIDD_MENU_init();
      }
    }  
  }

  // advance playbacktime using playbackspeed
  lastPlaybackTime = menuState.playbackTime;
  
  if (lastMenuTime && !menuactive && !menuState.interfaceDisplayCount) 
    menuState.playbackTime += (int)((time - lastMenuTime) * (menuState.playbackSpeed));
  
  
  if (lastPlaybackTime == menuState.playbackTime) return;

  if (menuState.playbackTime > viddPlayer_getLastFrame()) {
    menuState.playbackTime = viddPlayer_getLastFrame();
    // load new segment or clip to last frame
    leveltime = viddPlayer_getLastFrame()/1000;
    G_ExitLevel();
    if (menuState.intermissionsEnabled) {
      menuState.intermissionActive = 1;
    }
    else {
      VIDD_MENU_doWorldDone();
    }
  }
  else if (menuState.playbackTime < viddPlayer_getFirstFrame()) {
    // clip to first frame
    menuState.playbackTime = viddPlayer_getFirstFrame();
  }
}

//---------------------------------------------------------------------------
void VIDD_MENU_doWorldDone() {
  idmusnum = -1;
  gameaction = ga_nothing;
  AM_clearMarks();
  
  if (viddPlayer_loadSegment(viddPlayer_getCurSegmentIndex()+1)) {
    // new segment successfully loaded
    VIDD_MENU_init();
  }
  else  {
    // at the last segment
    VIDD_close();
    exit(0);
  }
}

//---------------------------------------------------------------------------
void VIDD_MENU_endFrame() {
  int i;
  
  // put our interface string into the player's message so
  // it gets printed on the screen
  doom_printf("%s", VIDD_MENU_getInterfaceString());
  
  // decrement displaycount. when it's non-zero, the entire menu
  // is visible. when it's zero, only the active control is visible.
  if (menuState.interfaceDisplayCount > 0) menuState.interfaceDisplayCount--;
  
  if (!menuState.phantomActive) return;
  
  // force a few things for the phantom player
  players[0].cheats |= CF_NOCLIP | CF_GODMODE;
  players[0].cmd.buttons &= ~BT_USE;
  for (i=0; i<NUMWEAPONS; i++) players[0].weaponowned[i] = 1;
  for (i=0; i<NUMAMMO; i++) players[0].ammo[i] = players[0].maxammo[i];
  
  // process the think for the phantom player only
  P_PlayerThink(&players[0]);
}

//---------------------------------------------------------------------------
int VIDD_MENU_getPlaybackTime() {
  return menuState.playbackTime;
}

//---------------------------------------------------------------------------
int VIDD_MENU_getSoundPitch() {
  float t;  
  // this is supposed to be a config setting, but it defaults to off 
  // i think. let's force it on so they can hear the cool effects
  pitched_sounds = 1;  
  
  if (fabs(menuState.playbackSpeed) < 1) {
    // lower pitch
    return (int)(fabs(menuState.playbackSpeed) * 128);
  }  
  // higher pitch
  t = (float)(fabs(menuState.playbackSpeed)-1)/(menuState.maxPlaybackSpeed-1);
  return 128 + (int)(t*128);
}

//---------------------------------------------------------------------------
int VIDD_MENU_getLerpSolver() {
  return (menuState.interpolationEnabled ? VAS_LINEAR : VAS_STEP);
}

//---------------------------------------------------------------------------
void enablePhantomMode(int enable) {
  // enable/disable phantom mode 
  if (enable) {
    // turn phantom mode on
    menuState.phantomActive = 1; 
    // enable the second player as the demoguy and take
    // over the first player
    playeringame[1] = true;
    VIDD_PLAY_setPlayerVisibility(0,1);
    VIDD_PLAY_setPlayerVisibility(1,1);
    
    // turn on his thinker so movement works
    players[0].mo->thinker.function = P_MobjThinker;

    // plant him on the floor
    players[0].mo->floorz   = players[0].mo->subsector->sector->floorheight;
    players[0].mo->ceilingz = players[0].mo->subsector->sector->ceilingheight;
    players[0].mo->z = players[0].mo->floorz;
    players[0].mo->dropoffz = players[0].mo->floorz;
  }
  else {
    // turn phantom off
    menuState.phantomActive = 0;
    playeringame[1] = false;
    players[0].cheats = 0;
    players[0].mo->thinker.function = 0;
    VIDD_PLAY_setPlayerVisibility(0,1);
    VIDD_PLAY_setPlayerVisibility(1,0);
  }
}

//---------------------------------------------------------------------------
int VIDD_MENU_handleKeyInput(int key, int down) {
  if (menuState.annotationActive && down) {
    menuState.annotationActive = 0;
    return 1;
  }
  
  if (menuState.phantomActive) {
    // when phantom is active, only the escape key is monitored
    if (key == KEYD_ESCAPE) {
      enablePhantomMode(0);
      return 1;    
    }
    else if (down && menuState.interfaceDisplayCount) {
      // any other key, while the menu is up, dismisses the menu
      menuState.interfaceDisplayCount = 0;
      return 1;
    }
    return 0;  
  }
  
  if (key == KEYD_LEFTARROW || key == KEYD_RIGHTARROW) {
    int dir = (key == KEYD_LEFTARROW ? -1 : 1);
    if (menuState.activeControl == MP_VCR) {
      // the vcr control is the only one to handle key ups
      if (down) {
        menuState.vcrSpeedupCount = 2*TICRATE;
        menuState.playbackSpeed = (float)dir*2;
      }
      else {
        menuState.vcrSpeedupCount = 0;
        menuState.playbackSpeed = 1.0;      
      }    
    }
    else if (!down) return 0;
    
    if (menuState.activeControl == MP_SPEED) {
      // adjust and clip the playbackspeed
      menuState.playbackSpeed += dir * 0.1f;
      if (menuState.playbackSpeed > menuState.maxPlaybackSpeed) menuState.playbackSpeed = menuState.maxPlaybackSpeed;
      else if (menuState.playbackSpeed < menuState.minPlaybackSpeed) menuState.playbackSpeed = menuState.minPlaybackSpeed;
    }
    else if (menuState.activeControl == MP_POS) {
      // adjust and clip the segment position
      float t = (float)(menuState.playbackTime - viddPlayer_getFirstFrame()) / (float)(viddPlayer_getLastFrame() - viddPlayer_getFirstFrame());
      t = ((int)(t*10)+dir) / 10.0f;
      menuState.playbackTime = viddPlayer_getFirstFrame() + (int)(t*(viddPlayer_getLastFrame()-viddPlayer_getFirstFrame()));
      if (menuState.playbackTime > viddPlayer_getLastFrame()) menuState.playbackTime = viddPlayer_getLastFrame();
      else if (menuState.playbackTime < viddPlayer_getFirstFrame()) menuState.playbackTime = viddPlayer_getFirstFrame();      
    }
    else if (menuState.activeControl == MP_SMOOTHING) {
      // enable/disable smoothing
      menuState.interpolationEnabled = (dir < 0) ? 1 : 0;
    }
    else if (menuState.activeControl == MP_PHANTOM) {
      // enable/disable phantom mode 
      if (dir < 0 && !menuState.phantomActive) {
        enablePhantomMode(1);
      }
      else if (dir > 0 && menuState.phantomActive) {
        enablePhantomMode(0);
      }
    }
    else if (menuState.activeControl == MP_INTERMISSION) {
      // enable/disable intermissions
      menuState.intermissionsEnabled = (dir < 0) ? 1 : 0;
    }
    else if (menuState.activeControl == MP_ANNOTATIONS) {
      // enable/disable annotations
      menuState.annotationsEnabled = (dir < 0) ? 1 : 0;
    }
    else if (menuState.activeControl == MP_SEGMENT) {
      menuState.highlightedSegment += dir;
      if (menuState.highlightedSegment >= viddPlayer_getNumSegments()) menuState.highlightedSegment = viddPlayer_getNumSegments()-1;
      else if (menuState.highlightedSegment < 0) menuState.highlightedSegment = 0;
      menuState.segmentChangeCount = 2*TICRATE;    
    }
  }
  // change the active control
  else if (down && key == KEYD_UPARROW) {
    if (--menuState.activeControl < 0) menuState.activeControl = 0;
    menuState.interfaceDisplayCount = TICRATE*4;
  }
  else if (down && key == KEYD_DOWNARROW) {
    if (++menuState.activeControl >= MP_MAXCONTROLS) menuState.activeControl = MP_MAXCONTROLS - 1;
    menuState.interfaceDisplayCount = TICRATE*4; 
  }
  // dismiss the menu
  else if (down && menuState.interfaceDisplayCount) {
    // any other key, while the menu is up, dismisses the menu
    menuState.interfaceDisplayCount = 0;
    // force a segmentchange if one is pending
    menuState.segmentChangeCount = 1;
  }  
  else return 0;
  
  // return 1 to signify we handled the input and the
  // game should ignore the keypress
  return 1;
}

//---------------------------------------------------------------------------
int VIDD_MENU_getFirstUsablePlayerIndex() {
  return (menuState.phantomActive ? 1 : 0);
}

//---------------------------------------------------------------------------
typedef struct  {
  const char *text;
  int id;
} TMenuControlInfo;

//---------------------------------------------------------------------------
const char *VIDD_MENU_getCurSegmentAttributesString() {
  static char str[VIDD_MAXSTRLEN];
  int numAttribs;
  int a;
  str[0] = 0;
  numAttribs = viddSys_getNumAttributes(VAT_CURSEGMENT);
  for (a=0; a<numAttribs; a++) {
    appendColoredString(str, viddSys_getAttributeName(VAT_CURSEGMENT, a), COLOR_WHITE, VIDD_MAXSTRLEN);
    appendColoredString(str, ":\t", COLOR_WHITE, VIDD_MAXSTRLEN);
    appendColoredString(str, viddSys_getAttributeValue(VAT_CURSEGMENT, a), COLOR_GREEN, VIDD_MAXSTRLEN);
    appendColoredString(str, "\n", COLOR_WHITE, VIDD_MAXSTRLEN);
  }
  return str;
}

//---------------------------------------------------------------------------
const char *VIDD_MENU_getInterfaceString() {
  // the VIDD menu is simply a string that gets printed using the 
  // doom_printf function (which just sets player->message). this is 
  // where that string gets built
  static char str[VIDD_MAXSTRLEN];
  static char timeStr[VIDD_MAXSTRLEN];
  char line[VIDD_MAXSTRLEN];
  #define numLines 11
  float t; 
  int i, n;
  const TMenuControlInfo lines[numLines] = {
    { "\x1b""4"" VIDD PLAYBACK MENU",                         -1 },
    { "\x1b""4""________________________________________",    -1 },
    { "\x1b""2""   VIDD\t",                                   MP_VCR },
    { "\x1b""2""   SPEED\t",                                  MP_SPEED },
    { "\x1b""2""   POS\t",                                    MP_POS },
    { "\x1b""2""   SEGMENT\t",                                MP_SEGMENT },
    { "\x1b""2""   SMOOTH\t",                                 MP_SMOOTHING },
    { "\x1b""2""   HGWELLS\t",                                MP_PHANTOM },
    { "\x1b""2""   INTERMS\t",                                MP_INTERMISSION },
    { "\x1b""2""   ANNOTS\t",                                 MP_ANNOTATIONS },
    { "\x1b""4""________________________________________\n",  -1 }
  };
  str[0] = 0;

  if (menuState.annotationActive) return getWordWrapped(menuState.annotationText, 40);
    
  for (i=0; i<numLines; i++) {
    // only consider the active control when displaycount is zero
    if (!menuState.interfaceDisplayCount && lines[i].id != menuState.activeControl) continue;
        
    if (lines[i].id < 0) {
      // just a spacer/static line
      appendString(str, lines[i].text, VIDD_MAXSTRLEN);
      appendString(str, "\n", VIDD_MAXSTRLEN);
      continue;
    }
    
    strncpy(line, lines[i].text, VIDD_MAXSTRLEN);
    line[VIDD_MAXSTRLEN-1] = 0;
    
    if (lines[i].id == MP_VCR) {
      // vcr
      if (menuState.playbackSpeed < -3) n = 0;
      else if (menuState.playbackSpeed < 0) n = 1;
      else if (menuState.playbackSpeed < 2) n = 2;
      else if (menuState.playbackSpeed < 3) n = 3;
      else n = 4;
      appendColoredString(line, "<<< ", n==0?COLOR_GREEN:COLOR_GREY, VIDD_MAXSTRLEN);
      appendColoredString(line, "<< ", n==1?COLOR_GREEN:COLOR_GREY, VIDD_MAXSTRLEN);
      appendColoredString(line, "> ", n==2?COLOR_GREEN:COLOR_GREY, VIDD_MAXSTRLEN);
      appendColoredString(line, ">> ", n==3?COLOR_GREEN:COLOR_GREY, VIDD_MAXSTRLEN);
      appendColoredString(line, ">>>   ", n==4?COLOR_GREEN:COLOR_GREY, VIDD_MAXSTRLEN);
      appendStringEllipsied(line, viddPlayer_getSegmentInfo(menuState.highlightedSegment).name, 8, VIDD_MAXSTRLEN);
      appendColoredString(line, "   ", COLOR_GREY, VIDD_MAXSTRLEN);
      {
      int curTime, totalTime;
      int s;
      int numSpaces;
      totalTime = (viddPlayer_getLastFrame()-viddPlayer_getFirstFrame()) / TICRATE;
      curTime = (menuState.playbackTime-viddPlayer_getFirstFrame()) / TICRATE; 
      numSpaces = strlen(getTimeAsString(totalTime, false));
      numSpaces -= strlen(getTimeAsString(curTime, false));
      for (s=0; s<numSpaces; s++) appendString(line, " ", VIDD_MAXSTRLEN);
     
      appendColoredString(line, getTimeAsString(curTime, false), COLOR_GREY, VIDD_MAXSTRLEN);
      appendColoredString(line, " / ", COLOR_GREY, VIDD_MAXSTRLEN);
      appendColoredString(line, getTimeAsString(totalTime, false), COLOR_GREY, VIDD_MAXSTRLEN);
      }
    }
    else if (lines[i].id == MP_SPEED) {
      // speed
      t = (float)(menuState.playbackSpeed - menuState.minPlaybackSpeed) / (menuState.maxPlaybackSpeed-menuState.minPlaybackSpeed);
      appendString(line, getProgressBar(36, t, 0), VIDD_MAXSTRLEN);
      appendString(line, "  ", VIDD_MAXSTRLEN); 
      appendString(line, getFloatAsString(menuState.playbackSpeed), VIDD_MAXSTRLEN);
    }
    else if (lines[i].id == MP_POS) {
      // position
      t = (float)(menuState.playbackTime - viddPlayer_getFirstFrame()) / (float)(viddPlayer_getLastFrame() - viddPlayer_getFirstFrame());
      appendString(line, getProgressBar(36, t, 1), VIDD_MAXSTRLEN);
      appendString(line, " ", VIDD_MAXSTRLEN); 
      appendString(line, getTimeAsString((menuState.playbackTime - viddPlayer_getFirstFrame()) / TICRATE, true), VIDD_MAXSTRLEN);
    }
    else if (lines[i].id == MP_SMOOTHING) {
      // smoothing
      appendColoredString(line, "ON     ", menuState.interpolationEnabled ? COLOR_GREEN : COLOR_GREY, VIDD_MAXSTRLEN); 
      appendColoredString(line, "OFF", menuState.interpolationEnabled ? COLOR_GREY : COLOR_GREEN, VIDD_MAXSTRLEN); 
    }
    else if (lines[i].id == MP_INTERMISSION) {
      // intermissions
      appendColoredString(line, "ON     ", menuState.intermissionsEnabled ? COLOR_GREEN : COLOR_GREY, VIDD_MAXSTRLEN); 
      appendColoredString(line, "OFF", menuState.intermissionsEnabled ? COLOR_GREY : COLOR_GREEN, VIDD_MAXSTRLEN); 
    }
    else if (lines[i].id == MP_SEGMENT) {
      // segments
      // gather up the four segment names around the currently playing segment
      int firstVisibleSegment, lastVisibleSegment;
      firstVisibleSegment = menuState.highlightedSegment - 1;
      if (firstVisibleSegment < 0) firstVisibleSegment = 0;      
      lastVisibleSegment = firstVisibleSegment + 4;
      if (lastVisibleSegment > viddPlayer_getNumSegments()) lastVisibleSegment = viddPlayer_getNumSegments();
      
      // tack them onto the string using ellipses so they don't overdraw the line
      for (n=firstVisibleSegment; n<lastVisibleSegment; n++) {
        appendColoredString(line, "", n==menuState.highlightedSegment ? COLOR_GREEN : COLOR_GREY, VIDD_MAXSTRLEN);
        appendStringEllipsied(line, viddPlayer_getSegmentInfo(n).name, 7, VIDD_MAXSTRLEN);
        if (n != lastVisibleSegment-1) appendColoredString(line, " - ", COLOR_GREY, VIDD_MAXSTRLEN);
      }   
    }
    else if (lines[i].id == MP_PHANTOM) {
      // phantom
      if (menuState.phantomActive) {
        appendColoredString(line, "[PRESS ESCAPE TO DISABLE]", COLOR_GREEN, VIDD_MAXSTRLEN);      
      }
      else {
        appendColoredString(line, "ON     ", menuState.phantomActive ? COLOR_GREEN : COLOR_GREY, VIDD_MAXSTRLEN); 
        appendColoredString(line, "OFF", menuState.phantomActive ? COLOR_GREY : COLOR_GREEN, VIDD_MAXSTRLEN);
      }      
    }
    else if (lines[i].id == MP_ANNOTATIONS) {
      // annotations
      appendColoredString(line, "ON     ", menuState.annotationsEnabled ? COLOR_GREEN : COLOR_GREY, VIDD_MAXSTRLEN); 
      appendColoredString(line, "OFF", menuState.annotationsEnabled ? COLOR_GREY : COLOR_GREEN, VIDD_MAXSTRLEN); 
    }
    
    if (menuState.activeControl == lines[i].id) {
      // if the currently drawing control is also the active control,
      // change its colors to signify focus
      replaceColor(line, VIDD_MAXSTRLEN, COLOR_WHITE, COLOR_LIGHTRED);
      replaceColor(line, VIDD_MAXSTRLEN, COLOR_GREEN, COLOR_LIGHTRED);
      replaceColor(line, VIDD_MAXSTRLEN, COLOR_GREY, COLOR_WHITE);
    }
    
    appendString(str, line, VIDD_MAXSTRLEN);
    appendString(str, "\n", VIDD_MAXSTRLEN);
  }
  
  if (menuState.interfaceDisplayCount) {
    // add the cursegment attributes string
    appendString(str, VIDD_MENU_getCurSegmentAttributesString(), VIDD_MAXSTRLEN);
  }
  
  if (players[displayplayer].message) {
    // tack on the previous player->message
    appendColoredString(str, players[consoleplayer].message, COLOR_RED, VIDD_MAXSTRLEN);
    appendString(str, "\n", VIDD_MAXSTRLEN);
  }
  
  return str;
}

#endif // #ifdef COMPILE_VIDD
