#ifdef COMPILE_VIDD

#ifndef VIDD_H
#define VIDD_H

//---------------------------------------------------------------------------
// SYSTEM
//---------------------------------------------------------------------------
void VIDD_checkStartupParamsFirst();
void VIDD_checkStartupParamsSecond();
int VIDD_inProgress();
int VIDD_getQuickVIDDInProgress();
void VIDD_close();
void VIDD_beginFrame();
void VIDD_endFrame();
int VIDD_handleKeyInput(int key, int down);
const char *VIDD_getIWad();
const char *VIDD_getNumberedAttrib(const char *preName, int index, const char *postName);
void VIDD_loadNecessaryWads();
void VIDD_exitWithError(const char *str, ...);
int VIDD_checkDemoStatus();
int VIDD_handleDraw();

//---------------------------------------------------------------------------
// REC
//---------------------------------------------------------------------------
int VIDD_REC_inProgress();
void VIDD_REC_open(const char *lmpname);
void VIDD_REC_openFromCommandLineParams();
void VIDD_REC_initDuringSecondParamCheck();
int VIDD_REC_checkDemoStatus();
void VIDD_REC_registerSound(int soundId, const mobj_t *origin);
void VIDD_REC_registerLevelLoad(int episode, int map, int skill);
void VIDD_REC_updatePlayerMessage(void *player, const char *msg);
void VIDD_REC_registerElementDestruction(void *ptr);
void VIDD_REC_endFrame();
void VIDD_REC_reset();
const char *VIDD_REC_getInfoMsg();

//---------------------------------------------------------------------------
// PLAY
//---------------------------------------------------------------------------
int VIDD_PLAY_inProgress();
void VIDD_PLAY_open(const char *filename);
void VIDD_PLAY_initDuringSecondParamCheck();
int VIDD_PLAY_getSoundPitch();
void VIDD_PLAY_endFrame();
void VIDD_PLAY_doWorldDone();
void VIDD_PLAY_setPlayerVisibility(int playerIndex, int visible);

//---------------------------------------------------------------------------
// MENU
//---------------------------------------------------------------------------
void VIDD_MENU_init();
void VIDD_MENU_beginFrame(unsigned int time);
void VIDD_MENU_endFrame();
int VIDD_MENU_getPlaybackTime();
int VIDD_MENU_getSoundPitch();
int VIDD_MENU_getLerpSolver();
int VIDD_MENU_handleKeyInput(int key, int down);
const char *VIDD_MENU_getInterfaceString();
int VIDD_MENU_getFirstUsablePlayerIndex();
void VIDD_MENU_doWorldDone();
const char *VIDD_MENU_getCurSegmentAttributesString();
int VIDD_MENU_getAdvanceIntermission();


//---------------------------------------------------------------------------

#endif

#endif // #ifdef COMPILE_VIDD