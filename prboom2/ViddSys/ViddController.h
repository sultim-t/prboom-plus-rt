#ifndef VIDD_CONTROLLER_H
#define VIDD_CONTROLLER_H

#include "ViddSys.h"
#include "ViddElementTrack.h"
#include <string>
#include <fstream>
#include <hash_map>
#include "ViddEventTrack.h"

//---------------------------------------------------------------------------
typedef TAnimTrack<std::string> TStringAnimTrack;
typedef std::map<std::string, TStringAnimTrack > TStringAnimTrackMap;
typedef std::map<std::string, TStringAnimTrack >::iterator TStringAnimTrackMapIter;

//---------------------------------------------------------------------------
// TViddController
//---------------------------------------------------------------------------
class TViddController {
protected:
  std::string filename;  
  TVIDDTimeUnit prevFrame, curFrame;
  std::vector<int> nextTrackIndexes;
  int cachedTrackIndex;
  bool active;
  
  TStringMap staticProperties;
  
  std::vector<TViddElementTrack*> elementTracks;
  TStringAnimTrackMap eventTracks;
  TViddEventTrack<TVIDDTriggeredSound> soundTrack;
  
  virtual TViddElementTrack *getTrack(const TVIDDElementHandle &handle, TViddControllerMode mode, bool createOnFail=false);
  
  void reset();
  
public:
  bool isActive() const { return active; }
  
  virtual bool open(const std::string &filename_);
  virtual void close() = 0;
  
  virtual void setFrame(TVIDDTimeUnit frame);
  int getFrame() const;
  
  virtual TStringMap &getAttributes(TVIDDAttributeScope scope)  = 0;
  virtual int getFileVersion() = 0;

  TViddController(TViddControllerMode mode) : soundTrack(mode) { active = false; }
};

//---------------------------------------------------------------------------


#endif