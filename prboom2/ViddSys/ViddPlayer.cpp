#include "stdafx.h"
#include "ViddPlayer.h"

TViddPlayer viddPlayer;

//---------------------------------------------------------------------------
// TViddPlayer
//---------------------------------------------------------------------------
TViddPlayer::TViddPlayer() : TViddController(MODE_PLAY) {
}

//---------------------------------------------------------------------------
bool TViddPlayer::open(const std::string &filename_) {
  try {
    TViddController::open(filename_);
    in.open(filename);
    beginSegment(0);
  }
  catch (TViddFileException &e) {
    popupErrorMessageBox("ERROR", e.getMsg().c_str());
    return false;
  }
  return true;
}

//---------------------------------------------------------------------------
void TViddPlayer::reset() {
  TViddController::reset(); 
  firstFrame = lastFrame = 0;
}

//---------------------------------------------------------------------------
bool TViddPlayer::beginSegment(int index) {
  if (index >= in.getNumSegments()) return false;
  
  reset();
  
  try {
    in.prepForSegmentRead(index);
    //std::ifstream in(filename.c_str(), std::ios::binary);
    
    int numTracks;
    std::string name;
    
    // read in event tracks
    in.read(numTracks);
    for (int t=0; t<numTracks; t++) {
      in.read(name);
      eventTracks[name].read(in);
    }
    
    // read in sound track
    soundTrack.read(in);
    
    // read in element tracks
    in.read(numTracks);
    for (t=0; t<numTracks; t++) {
      //TViddElementTrack *track = new TViddElementTrack(MODE_PLAY);
      elementTracks.push_back(new TViddElementTrack(MODE_PLAY));
      elementTracks[elementTracks.size()-1]->read(in);
    }
    
    firstFrame = in.getSegment(index).getFirstFrame();
    lastFrame = in.getSegment(index).getLastFrame();
    
    curSegmentIndex = index;
  }
  catch (TViddFileException e) {
    reset();
    popupErrorMessageBox("ERROR", e.getMsg().c_str());
    return false;
  }

  return true;
}

//---------------------------------------------------------------------------
int TViddPlayer::getNumSegments() {
  return in.getNumSegments();
}

//---------------------------------------------------------------------------
const TViddFileSegmentInfo &TViddPlayer::getSegment(int index) {
  return in.getSegment(index);
}

//---------------------------------------------------------------------------
void TViddPlayer::close() {
  in.close();
}

//---------------------------------------------------------------------------
void TViddPlayer::setFrame(TVIDDTimeUnit frame) {  
  if (frame > lastFrame) frame = lastFrame;
  if (frame < firstFrame) frame = firstFrame;
  
  TViddController::setFrame(frame);
  
  soundTrack.setFrame(frame);  
  for (int i=0; i<elementTracks.size(); i++) { elementTracks[i]->setFrame(frame); }
  for (i=0; i<VVS_MAXSTATES; i++) visStateTracks[i].clear();
  for (i=0; i<elementTracks.size(); i++) {
    if (elementTracks[i]->getHandle() != nullHandle && elementTracks[i]->getHandle() == elementTracks[i]->getPrevHandle()) {
      // alive
      visStateTracks[VVS_VISIBLE].push_back(elementTracks[i]);
      continue;
    }
    if (elementTracks[i]->getPrevHandle() != nullHandle) {
      // just died
      visStateTracks[VVS_JUSTDISAPPEARED].push_back(elementTracks[i]);
    }
    else if (elementTracks[i]->getHandle() != nullHandle) {
      // just born
      visStateTracks[VVS_JUSTAPPEARED].push_back(elementTracks[i]);
      visStateTracks[VVS_VISIBLE].push_back(elementTracks[i]);        
    }
  }
}

//---------------------------------------------------------------------------
int TViddPlayer::getNumTriggeredSounds() {
  return soundTrack.getNumEventsForCurFrame();
}

//---------------------------------------------------------------------------
const TVIDDTriggeredSound &TViddPlayer::getTriggeredSound(int index) {
  return soundTrack.getEventForCurFrame(index);
}
  
//---------------------------------------------------------------------------
const std::string &TViddPlayer::getTriggeredEventSubject(const std::string &verb) {
  TStringAnimTrackMapIter iter = eventTracks.find(verb);
  if (iter == eventTracks.end()) return nullStr;
  
  TStringAnimTrack *track = &iter->second;
  std::string *prev = track->getValue(prevFrame);  
  std::string *cur = track->getValue(curFrame);

  if (prev && cur && prevFrame != curFrame && prevFrame == firstFrame) return *prev;  
  if (!cur) return nullStr;
  if (prev && *prev == *cur) return nullStr; 
  
  return *cur;
}
  
//---------------------------------------------------------------------------
int TViddPlayer::getNumTracks(TVIDDVisibleState visibleState) {
  return visStateTracks[visibleState].size();
}

//---------------------------------------------------------------------------
TViddElementTrack *TViddPlayer::getTrack(TVIDDVisibleState visibleState, int index) {
  return (visStateTracks[visibleState])[index];
}

//---------------------------------------------------------------------------
/*
TViddElementTrack *TViddPlayer::getTrack(const TVIDDElementHandle &handle) {
  if (cachedTrack && cachedTrack->getHandle() == handle) return cachedTrack;
  for (int t=0; t<elementTracks.size(); t++) {
    if (elementTracks[t]->getHandle() != handle) continue;
    cachedTrack = elementTracks[t];
    return cachedTrack;
  }
  return 0;
}
*/
