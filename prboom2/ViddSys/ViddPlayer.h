#ifndef VIDDPLAYER_H
#define VIDDPLAYER_H

#include "ViddController.h"

//---------------------------------------------------------------------------
// TViddPlayer
//---------------------------------------------------------------------------
class TViddPlayer : public TViddController {
private:
  TViddFileIn in;
  TVIDDTimeUnit firstFrame, lastFrame;
  std::vector<TViddElementTrack*> visStateTracks[VVS_MAXSTATES]; 
  int curSegmentIndex;
  void reset();

public:
  virtual bool open(const std::string &filename_);
  void close();

  bool beginSegment(int index);
  int getNumSegments();
  const TViddFileSegmentInfo &getSegment(int index);
  int getCurSegmentIndex() { return curSegmentIndex; }
  
  virtual TStringMap &getAttributes(TVIDDAttributeScope scope)  {
    return (scope == VAT_GLOBAL) ? in.getGlobalAttributes() : in.getSegment(curSegmentIndex).getLocalAttributes();
  }
  
  virtual void setFrame(TVIDDTimeUnit frame);  
  
  int getNumTracks(TVIDDVisibleState visibleState);
  TViddElementTrack *getTrack(TVIDDVisibleState visibleState, int index);  
  TViddElementTrack *getTrack(const TVIDDElementHandle &handle) {
    return TViddController::getTrack(handle, MODE_PLAY, false);
  }
  
  TVIDDTimeUnit getFirstFrame() const { return firstFrame; }
  TVIDDTimeUnit getLastFrame() const { return lastFrame; }
  
  int getNumTriggeredSounds();
  const TVIDDTriggeredSound &getTriggeredSound(int index);
  
  const std::string &getTriggeredEventSubject(const std::string &verb);
  virtual int getFileVersion() { return in.getVersion(); }
  
  TViddPlayer();
};

//---------------------------------------------------------------------------
extern TViddPlayer viddPlayer;
//---------------------------------------------------------------------------

#endif