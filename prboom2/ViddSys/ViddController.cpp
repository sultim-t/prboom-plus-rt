#include "stdafx.h"
#include "ViddController.h"

//---------------------------------------------------------------------------
// TViddController
//---------------------------------------------------------------------------
bool TViddController::open(const std::string &filename_) {
  filename = filename_;
  reset();
  active = true;
  return true;
}

//---------------------------------------------------------------------------
void TViddController::reset() {
  cachedTrackIndex = -1;
  prevFrame = curFrame = 0;
  eventTracks.clear();
  soundTrack.clearAllKeys();
  nextTrackIndexes.clear();
  elementTracks.clear();
}

//---------------------------------------------------------------------------
void TViddController::setFrame(TVIDDTimeUnit frame) { 
  prevFrame = curFrame; 
  curFrame = frame;
  cachedTrackIndex = -1;
}

//---------------------------------------------------------------------------
int TViddController::getFrame() const { return curFrame; }


//---------------------------------------------------------------------------
TViddElementTrack *TViddController::getTrack(const TVIDDElementHandle &handle, TViddControllerMode mode, bool createOnFail) {
  TViddElementTrack *track = 0;
  
  while (nextTrackIndexes.size() < elementTracks.size()) nextTrackIndexes.push_back(-1);

  if (cachedTrackIndex >= 0) {
    track = elementTracks[cachedTrackIndex];
    if (track->getHandle() == handle) return track;
    
    if (nextTrackIndexes[cachedTrackIndex] >= 0) {
      track = elementTracks[nextTrackIndexes[cachedTrackIndex]];
      if (track->getHandle() == handle) {
        cachedTrackIndex = nextTrackIndexes[cachedTrackIndex];
        return track;
      }
      nextTrackIndexes[cachedTrackIndex] = -1;
    }
  }

  int trackIndex = -1, freeTrackIndex = -1;
  for (int i=0; i<elementTracks.size(); i++) {
    if (
      createOnFail &&
      freeTrackIndex < 0 &&
      elementTracks[i]->getHandle() == nullHandle && 
      elementTracks[i]->getPrevHandle() == nullHandle
    ) {
      freeTrackIndex = i;
    }
    if (elementTracks[i]->getHandle() == handle) {
      trackIndex = i;
      break;
    }    
  }
  
  if (!createOnFail && trackIndex < 0) return 0;
  
  if (trackIndex >= 0) {
  }
  else if (freeTrackIndex >= 0) {
    trackIndex = freeTrackIndex;
  }
  else {
    elementTracks.push_back(new TViddElementTrack(mode, curFrame));
    nextTrackIndexes.push_back(-1);
    trackIndex = elementTracks.size()-1;
  } 
  
  if (cachedTrackIndex >= 0) nextTrackIndexes[cachedTrackIndex] = trackIndex; 
  cachedTrackIndex = trackIndex;
  
  track = elementTracks[trackIndex];
  if (track->getHandle() != handle) track->setHandle(handle);
  
  return track;
}
