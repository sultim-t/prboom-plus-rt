#ifndef VIDDEVENTTRACK_H
#define VIDDEVENTTRACK_H

#include "ViddBasic.h"
#include "AnimTrack.h"
#include <vector>

//---------------------------------------------------------------------------
template <class T> class TViddEventTrack : public TAnimTrack< std::vector<T> > {
private:
  TVIDDTimeUnit prevFrame, curFrame;
  TViddControllerMode mode;
  std::vector<T*> crossedEvents;
  TAnimKey< std::vector<T> > *curKey;

public:
  void setFrame(int frame) {
    prevFrame = curFrame;
    curFrame = frame;
    
    if (mode == MODE_RECORD) {
      curKey = 0;    
      return;
    }
    
    crossedEvents.clear();    
    std::vector< std::vector<T> * > crossedValues;
    getKeyedValuesBetweenFrames(crossedValues, prevFrame, curFrame);
    
    for (int v=0; v<crossedValues.size(); v++) {
      std::vector<T> *vec = crossedValues[v];
      for (int e=0; e<vec->size(); e++) {
        crossedEvents.push_back( &( (*vec)[e] ) );
      }
    }
  }
  
  int getNumEventsTotal() {
    int numEvents = 0;
    for (int k=0; k<getNumKeys(); k++) {
      const TAnimKey< std::vector<T> > &key = getKeyFromIndex(k);
      numEvents += key.value.size();  
    }
    return numEvents;
  }
  
  int getNumEventsForCurFrame() { return crossedEvents.size(); }
  
  T &getEventForCurFrame(int index) { return *crossedEvents[index]; }
  
  void addEvent(const T &event) {
    if (!curKey) curKey = addKey(curFrame, std::vector<T>(), false);    
    curKey->value.push_back(event);
  }
  
  TViddEventTrack(TViddControllerMode mode_) {
    mode = mode_;
    prevFrame = curFrame = 0;
    setFrame(0);
  }
};

//---------------------------------------------------------------------------


#endif