#include "stdafx.h"
#include "ViddBasic.h"
#include "ViddRecorder.h"
#include "ViddPlayer.h"

// this should be cased when (if) linux support is added. it's 
// currently only needed for the dll entry function
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//---------------------------------------------------------------------------
BOOL APIENTRY DllMain( 
  HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved
) {
  return TRUE;
}

//---------------------------------------------------------------------------
VIDDAPI int viddSys_getVersion() {
  if (viddRecorder.isActive()) return viddRecorder.getFileVersion();
  else if (viddPlayer.isActive()) return viddPlayer.getFileVersion();
  else return 0;
}

//---------------------------------------------------------------------------
VIDDAPI void viddSys_setInfoCallback(TVIDDInfoCallback callback) {
  infoNotifier.setCallback(callback);
}

//---------------------------------------------------------------------------
TStringMap *getActiveAttributes(TVIDDAttributeScope scope) {
  TStringMap *attributes = 0;
  if (viddRecorder.isActive()) return &viddRecorder.getAttributes(scope);
  else if (viddPlayer.isActive()) return &viddPlayer.getAttributes(scope);
  return 0;
}

//---------------------------------------------------------------------------
VIDDAPI const char *viddSys_getAttribute(TVIDDAttributeScope scope, const char *name) {
  static std::string str;
  
  TStringMap *attributes = getActiveAttributes(scope);
  if (!attributes) return "";
  
  TStringMap::iterator iter = attributes->find(std::string(name));
  if (iter == attributes->end()) str = "";
  else str = iter->second;

  return str.c_str();
}


//---------------------------------------------------------------------------
VIDDAPI int viddSys_getNumAttributes(TVIDDAttributeScope scope) {
  TStringMap *attributes = getActiveAttributes(scope);
  if (!attributes) return 0;  
  return attributes->size();
}

//---------------------------------------------------------------------------
VIDDAPI const char *viddSys_getAttributeName(TVIDDAttributeScope scope, int index) {
  static std::string str;
  
  TStringMap *attributes = getActiveAttributes(scope);
  if (!attributes) return "";

  TStringMap::iterator iter = attributes->begin();
  for (int i=0; i<index; i++) ++iter;
  str = iter->first;
  return str.c_str();
}

//---------------------------------------------------------------------------
VIDDAPI const char *viddSys_getAttributeValue(TVIDDAttributeScope scope, int index) {
  static std::string str;
  
  TStringMap *attributes = getActiveAttributes(scope);
  if (!attributes) return "";

  TStringMap::iterator iter = attributes->begin();
  for (int i=0; i<index; i++) ++iter;
  str = iter->second;
  return str.c_str();
}

//---------------------------------------------------------------------------
VIDDAPI void viddSys_setAttribute(TVIDDAttributeScope scope, const char *name, const char *value) {
  TStringMap *attributes = getActiveAttributes(scope);
  if (!attributes) return;

  (*attributes)[std::string(name)] = std::string(value);
}

//---------------------------------------------------------------------------
VIDDAPI void viddSys_copyAttributesFromGlobalToCurSegment(const char *attributeNamePrefix) {
  TStringMap *globalAttributes = getActiveAttributes(VAT_GLOBAL);
  if (!globalAttributes) return;
  TStringMap *localAttributes = getActiveAttributes(VAT_CURSEGMENT);
  if (!localAttributes) return;

  std::string prefix = attributeNamePrefix;
  
  TStringMap::iterator iter = globalAttributes->begin();
  for (;iter != globalAttributes->end(); ++iter) {
    const std::string &name = iter->first;
    if (prefix != name.substr(0, prefix.length())) continue;
    // copy this attribute to the local (segment) attributes without the prefix
    (*localAttributes)[ name.substr(prefix.length(), name.length()-prefix.length()) ] =
      iter->second;  
  }
}

//---------------------------------------------------------------------------
// record funcs
//---------------------------------------------------------------------------
VIDDAPI int viddRecorder_open(const char *paramFilename) {
  return (int)viddRecorder.open(std::string(paramFilename));
}

//---------------------------------------------------------------------------
VIDDAPI void viddRecorder_beginSegment(const char *name) {
  viddRecorder.beginSegment(std::string(name));
}
  
//---------------------------------------------------------------------------
VIDDAPI void viddRecorder_beginFrame(const TVIDDTimeUnit time) {
  viddRecorder.setFrame(time);
}
  
//---------------------------------------------------------------------------
VIDDAPI void viddRecorder_registerEvent(const TVIDDTriggeredEvent event) {
  viddRecorder.addEvent(event);
}
    
//---------------------------------------------------------------------------
VIDDAPI void viddRecorder_registerSound(
  const int soundId,
  const TVIDDElementHandle sourceElementHandle
) {
  TVIDDTriggeredSound sound;
  sound.soundId = soundId;
  sound.sourceElementHandle = sourceElementHandle;
  viddRecorder.addSound(sound);
}

//---------------------------------------------------------------------------
VIDDAPI void viddRecorder_setIntProp(const TVIDDElementHandle elementHandle,
  const TVIDDElementProperty property, const int value
) {
  TViddElementTrack *track = viddRecorder.getTrack(elementHandle);
  track->setIntProperty(property, value);
}

//---------------------------------------------------------------------------
VIDDAPI void viddRecorder_setVectorProp(const TVIDDElementHandle elementHandle,
  const TVIDDElementProperty property, const int x, const int y, const int z
) {
  TViddElementTrack *track = viddRecorder.getTrack(elementHandle);
  TVIDDFixedVector value;
  value.x = x; value.y = y; value.z = z;
  track->setFixedVectorProperty(property, value);
}

//---------------------------------------------------------------------------
VIDDAPI void viddRecorder_setStringProp(
  const TVIDDElementHandle elementHandle,
  const TVIDDElementProperty property,
  const char *value
) {
  TViddElementTrack *track = viddRecorder.getTrack(elementHandle);
  if (!value) value = "";
  track->setStringProperty(property, std::string(value));
}

//---------------------------------------------------------------------------
VIDDAPI int viddRecorder_getElementExists(
  const TVIDDElementHandle elementHandle
) {
  TViddElementTrack *track = viddRecorder.getTrack(elementHandle, false);
  if (!track) return 0;
  return 1;
}
    
//---------------------------------------------------------------------------
VIDDAPI int viddRecorder_registerElementDestruction(const TVIDDElementHandle elementHandle) {
  return viddRecorder.registerElementDestruction(elementHandle);
}

//---------------------------------------------------------------------------
VIDDAPI void viddRecorder_endFrame() {
}

//---------------------------------------------------------------------------
VIDDAPI void viddRecorder_close() {
  viddRecorder.close();
}

//---------------------------------------------------------------------------
// play funcs
//---------------------------------------------------------------------------
VIDDAPI int viddPlayer_open(const char *fromFilename) {
  return (int)viddPlayer.open(std::string(fromFilename));
}

//---------------------------------------------------------------------------
VIDDAPI int viddPlayer_getNumSegments() {
  return viddPlayer.getNumSegments();
}

//---------------------------------------------------------------------------
VIDDAPI TVIDDSegmentInfo viddPlayer_getSegmentInfo(int index) {
  TVIDDSegmentInfo info;
  info.name = viddPlayer.getSegment(index).getName().c_str();
  info.firstFrame = viddPlayer.getSegment(index).getFirstFrame();
  info.lastFrame = viddPlayer.getSegment(index).getLastFrame();
  return info;
}

//---------------------------------------------------------------------------
VIDDAPI int viddPlayer_getCurSegmentIndex() {
  return viddPlayer.getCurSegmentIndex();
}

//---------------------------------------------------------------------------
VIDDAPI int viddPlayer_loadSegment(int index) {
  return viddPlayer.beginSegment(index);
}
  
//---------------------------------------------------------------------------
VIDDAPI TVIDDTimeUnit viddPlayer_getFirstFrame() {
  return viddPlayer.getFirstFrame();
}

//---------------------------------------------------------------------------
VIDDAPI TVIDDTimeUnit viddPlayer_getLastFrame() {
  return viddPlayer.getLastFrame();
}
  
//---------------------------------------------------------------------------
VIDDAPI void viddPlayer_beginFrame(const TVIDDTimeUnit time) {
  viddPlayer.setFrame(time);
}

//---------------------------------------------------------------------------
VIDDAPI const char *viddPlayer_getTriggeredEventSubject(const char *verb) {
  return viddPlayer.getTriggeredEventSubject(std::string(verb)).c_str();
}

//---------------------------------------------------------------------------
VIDDAPI int viddPlayer_getNumTriggeredSounds() {
  return viddPlayer.getNumTriggeredSounds();
}

//---------------------------------------------------------------------------
VIDDAPI const TVIDDTriggeredSound viddPlayer_getTriggeredSound(const int index) {
  return viddPlayer.getTriggeredSound(index);
}

//---------------------------------------------------------------------------
VIDDAPI int   viddPlayer_getNumElements(const TVIDDVisibleState state) {
  return viddPlayer.getNumTracks(state);
}

//---------------------------------------------------------------------------
VIDDAPI const TVIDDElementHandle viddPlayer_getElementHandle_JustAppeared(int index) {
  return viddPlayer.getTrack(VVS_JUSTAPPEARED, index)->getHandle();  
}

//---------------------------------------------------------------------------
VIDDAPI const TVIDDElementHandle viddPlayer_getElementHandle_Visible(int index) {    
   return viddPlayer.getTrack(VVS_VISIBLE, index)->getHandle();  
}

//---------------------------------------------------------------------------
VIDDAPI void *viddPlayer_getElementUserData_JustDisappeared(int index) {
  return viddPlayer.getTrack(VVS_JUSTDISAPPEARED, index)->getUserData();
}

//---------------------------------------------------------------------------
VIDDAPI const TVIDDElementType viddPlayer_getElementType_JustDisappeared(int index) {
  return viddPlayer.getTrack(VVS_JUSTDISAPPEARED, index)->getPrevHandle().type;
}

//---------------------------------------------------------------------------
VIDDAPI void viddPlayer_setElementUserData_JustDisappeared(int index, void *userData) {
  viddPlayer.getTrack(VVS_JUSTDISAPPEARED, index)->setUserData(userData);
}

//---------------------------------------------------------------------------
VIDDAPI void viddPlayer_setElementUserData(const TVIDDElementHandle elementHandle, void *userData) {
  TViddElementTrack *track = viddPlayer.getTrack(elementHandle);
  if (!track) return;
  track->setUserData(userData);
}

//---------------------------------------------------------------------------
VIDDAPI void *viddPlayer_getElementUserData(const TVIDDElementHandle elementHandle) {
  TViddElementTrack *track = viddPlayer.getTrack(elementHandle);
  if (!track) return 0;
  return track->getUserData();
} 

//---------------------------------------------------------------------------
VIDDAPI int viddPlayer_getIntProp(
  const TVIDDElementHandle elementHandle, 
  const TVIDDElementProperty property,
  const TVIDDAnimSolver solver
) {
  TViddElementTrack *track = viddPlayer.getTrack(elementHandle);
  if (!track) return 0;
  return track->getIntProperty(property, solver);
}

//---------------------------------------------------------------------------
VIDDAPI TVIDDFixedVector viddPlayer_getVectorProp(const TVIDDElementHandle elementHandle, const TVIDDElementProperty property, const TVIDDAnimSolver solver) {
  TViddElementTrack *track = viddPlayer.getTrack(elementHandle);
  static const TVIDDFixedVector defaultVec = {0,0,0};
  if (!track) return defaultVec;
  return track->getFixedVectorProperty(property, solver);
}

//---------------------------------------------------------------------------
VIDDAPI const char *viddPlayer_getStringProp(      
  const TVIDDElementHandle elementHandle, 
  const TVIDDElementProperty property
) {
  TViddElementTrack *track = viddPlayer.getTrack(elementHandle);
  if (!track) return 0;
  return track->getStringProperty(property).c_str();
}
  
//---------------------------------------------------------------------------
VIDDAPI void viddPlayer_endFrame() {
}

//---------------------------------------------------------------------------
VIDDAPI void viddPlayer_close() {
  viddPlayer.close();
}