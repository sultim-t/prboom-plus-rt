#ifdef COMPILE_VIDD

#include "psnprntf.h"

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
#include "math.h"
#include "vidd_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "v_video.h"
#include "i_video.h"
#include "c_io.h"

static int initted = 0;
static int quickVIDDInProgress = 0;
int VIDD_getQuickVIDDInProgress() { return quickVIDDInProgress; }

//---------------------------------------------------------------------------
void VIDD_infoCallback(const char *str) {
  lprintf(LO_INFO, "%s", str);
}

//---------------------------------------------------------------------------
void VIDD_init() {
  VIDD_MENU_init();
  viddSys_setInfoCallback(VIDD_infoCallback);  
  initted = 1;
}

//---------------------------------------------------------------------------
void VIDD_checkStartupParamsFirst() {
  int p, len;
  const char *paramStr, *extStr;
  char file[PATH_MAX+1];
  
  p = M_CheckParm("-viddrecord");
  if (p && p < myargc-1) {
    strcpy(file, myargv[p+1]);
    AddDefaultExtension(file,".xml");
    VIDD_REC_open(file);
    return;
  }

  p = M_CheckParm("-viddplay");  
  if (p && p < myargc-1) {
    strcpy(file, myargv[p+1]);
    AddDefaultExtension(file,".vidd");
    VIDD_PLAY_open(file);
    return;
  }
  
  if (M_CheckParm("-vidd")) {
    quickVIDDInProgress = 1;
    VIDD_REC_openFromCommandLineParams();
    return;
  }
  
  for (p=0; p<myargc; p++) {
    // check for autoplay of drag-n-drop .vidd or .xml
    paramStr = myargv[p];
    len = strlen(paramStr);
    if (len < 5) continue;
    extStr = &paramStr[len-5];
    if (!strcmp(extStr, ".vidd")) {
      // set the current directory to the dir
      // where the module exe resides since drag-drop
      // onto the .exe from explorer doesn't do this
      setCurDirToModuleDir();
      VIDD_PLAY_open(myargv[p]);
      break;
    }
    extStr = &paramStr[len-4];
    if (!strcmp(extStr, ".xml")) {
      setCurDirToModuleDir();
      VIDD_REC_open(myargv[p]);
      break;
    }    
  }
}

//---------------------------------------------------------------------------
void VIDD_checkStartupParamsSecond() {
  if (VIDD_PLAY_inProgress()) VIDD_PLAY_initDuringSecondParamCheck();
  else if (VIDD_REC_inProgress()) VIDD_REC_initDuringSecondParamCheck();
  else return;
  VIDD_loadNecessaryWads();
  
  // redirect stdout to a file we can read from another app
  freopen("vidd.log", "w", stdout);
}

//---------------------------------------------------------------------------
void VIDD_loadNecessaryWads() {
  // load up the custom wads
  const char *wadName;
  int i;
  for (i=1;;i++) {
    wadName = VIDD_getNumberedAttrib("pwad", i, "");
    if (!wadName[0]) break;
    D_AddFile(wadName, source_pwad);
    modifiedgame = true;
  } 
}

//---------------------------------------------------------------------------
void VIDD_exitWithError(const char *str, ...) {
  static char msg[VIDD_MAXSTRLEN];
  va_list v;
  va_start(v,str);
  vsprintf(msg,str,v);
  va_end(v);
  I_Error("VIDD ERROR: %s", msg);
  VIDD_close();
  exit(0);
}

//---------------------------------------------------------------------------
const char *VIDD_getIWad() {
  if (!(viddSys_getAttribute(VAT_GLOBAL, "iwad")[0])) {
    VIDD_exitWithError("\"iwad\" attribute missing from \"vidd\" node.");
  }    
  return viddSys_getAttribute(VAT_GLOBAL, "iwad");
}

//---------------------------------------------------------------------------
const char *VIDD_getNumberedAttrib(const char *preName, int index, const char *postName) {
  char attribName[VIDD_MAXSTRLEN];
  if (viddSys_getVersion() < 110) {
    // older versions always used the index
    psnprintf(attribName, VIDD_MAXSTRLEN-1, "%s%i%s", preName, index, postName);
  }
  else {
    // current versions ommit the number on the zero'th element
    if (index) psnprintf(attribName, VIDD_MAXSTRLEN-1, "%s%i%s", preName, index, postName);
    else psnprintf(attribName, VIDD_MAXSTRLEN-1, "%s%s", preName, postName);
  }
  attribName[VIDD_MAXSTRLEN] = 0;
  return viddSys_getAttribute(VAT_GLOBAL, attribName);
}

//---------------------------------------------------------------------------
int VIDD_inProgress() {
  return (VIDD_REC_inProgress() || VIDD_PLAY_inProgress());
}

//---------------------------------------------------------------------------
int VIDD_handleKeyInput(int key, int down) {
  if (VIDD_PLAY_inProgress()) return VIDD_MENU_handleKeyInput(key, down);
  if (VIDD_REC_inProgress() && down) {
    VIDD_close();
    exit(0);
  }
  return 0;
}

//---------------------------------------------------------------------------
void VIDD_beginFrame() {
  unsigned int time = ((gametic-basetic)*1000);
  
  if (!initted) VIDD_init();
  
  if (VIDD_REC_inProgress()) {
    viddRecorder_beginFrame(leveltime*1000);
  }
  else if (VIDD_PLAY_inProgress()) {
    nodrawers = false;  
    VIDD_MENU_beginFrame(time);
    viddPlayer_beginFrame(VIDD_MENU_getPlaybackTime());
  }
}

//---------------------------------------------------------------------------
void VIDD_endFrame() { 
  if (VIDD_REC_inProgress()) {
    VIDD_REC_endFrame();
  }
  else if (VIDD_PLAY_inProgress()) {
    VIDD_PLAY_endFrame(); 
    VIDD_MENU_endFrame();   
  }
}

//---------------------------------------------------------------------------
void VIDD_close() {
  if (VIDD_REC_inProgress()) viddRecorder_close();
  else if (VIDD_PLAY_inProgress()) viddPlayer_close();
}

//---------------------------------------------------------------------------
int VIDD_handleDraw() {
  const char *infoMsg;
  
  if (!VIDD_REC_inProgress()) return false;
  
  infoMsg = VIDD_REC_getInfoMsg();
  if (!infoMsg[0]) return true;

  V_FillRect(0, 0, 0, SCREENWIDTH, SCREENHEIGHT, 0);
  V_WriteText(infoMsg, 8, 8, 0);
  I_FinishUpdate();
  
  return true;
}

//---------------------------------------------------------------------------
int VIDD_checkDemoStatus() {
  if (VIDD_REC_inProgress()) {
    if (VIDD_REC_checkDemoStatus()) return true;

    // just stopped recording
    if (quickVIDDInProgress) {
      // finalize the quickVIDD
      
      VIDD_close();
      VIDD_REC_reset();
      VIDD_PLAY_open("./temp.vidd");      
      return true;
    }
    else {
      VIDD_close();
      exit(0);
    }
  }
  else return VIDD_PLAY_inProgress();
}

//---------------------------------------------------------------------------

#endif // #ifdef COMPILE_VIDD
