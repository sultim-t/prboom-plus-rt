#ifndef VIDD_ANIMTRACK_H
#define VIDD_ANIMTRACK_H

#include <vector>
#include "ViddFile.h"

//---------------------------------------------------------------------------
// TAnimKey
//---------------------------------------------------------------------------
template <class T> class TAnimKey {
public:
  TVIDDTimeUnit frame;
  T value;

  bool operator ==(TAnimKey<T> key) {
    return (frame == key.frame && value == key.value);
  }
  
  TAnimKey(int frame_=0, const T &value_=T()) { frame = frame_; value = value_; }
};

//---------------------------------------------------------------------------
// TAnimTrack
//---------------------------------------------------------------------------
template <class T> class TAnimTrack {
protected:
  typedef TAnimKey<T> TTypedAnimKey;
  std::vector< TTypedAnimKey > keys; 
  T workValue;
  TVIDDTimeUnit prevFrame;
  std::vector<TVIDDTimeUnit> keyZoneBarrierFrames;
  
  //-------------------------------------------------------------------------    
  inline int getKeyZone(TVIDDTimeUnit frame) {
    for (int z=0; z<keyZoneBarrierFrames.size(); z++) {
      if (frame < keyZoneBarrierFrames[z]) return z;
    }
    return 0;
  }
  
  //-------------------------------------------------------------------------    
  inline bool getInSameKeyZone(const TTypedAnimKey &key1, const TTypedAnimKey &key2) {
    if (!keyZoneBarrierFrames.size()) return true;
    return (getKeyZone(key1.frame) == getKeyZone(key2.frame));
  }

  //-------------------------------------------------------------------------   
  template <class S> bool isDeltable(const S &val1) { return false; }
  bool isDeltable(const int &value) { return true; }
  bool isDeltable(const TVIDDFixedVector &value) { return true; }

  template <class S> S getDelta(const S &val1, const S &val2) { return val1; }
  int getDelta(const int &val1, const int &val2) { return val1 - val2; }
  TVIDDFixedVector getDelta(const TVIDDFixedVector &vec1, const TVIDDFixedVector &vec2) { return vec1 - vec2; }
  
  template <class S> S getAdded(const S &val1, const S &val2) { return val1; }
  int getAdded(const int &val1, const int &val2) { return val1 + val2; }
  TVIDDFixedVector getAdded(const TVIDDFixedVector &vec1, const TVIDDFixedVector &vec2) { return vec1 + vec2; }
  
  template <class S>
  S getLerped(TVIDDTimeUnit frame, TVIDDTimeUnit prevFrame, const S &prevValue, TVIDDTimeUnit nextFrame, const S &nextValue) {
    double t = (double)(frame - prevFrame) / (nextFrame - prevFrame); 
    return (nextValue - prevValue) * t + prevValue;
  }
  inline std::string getLerped(TVIDDTimeUnit frame, TVIDDTimeUnit prevFrame, const std::string &prevValue, TVIDDTimeUnit nextFrame, const std::string &nextValue) {
    return prevValue;
  }
  
public:
  //-------------------------------------------------------------------------  
  int getNumKeys() const { return keys.size(); }
  const TTypedAnimKey &getKeyFromIndex(int index) const { return keys[index]; }
  
  //-------------------------------------------------------------------------  
  TTypedAnimKey *getLastKey() {
    if (!keys.size()) return 0;
    return &keys[keys.size()-1]; 
  }

  //-------------------------------------------------------------------------  
  TVIDDTimeUnit getLowestFrame() {
    if (!keys.size()) return TVIDDTimeUnit_Highest;
    return keys[0].frame;
  }
  
  //-------------------------------------------------------------------------  
  TVIDDTimeUnit getHighestFrame() {
    TTypedAnimKey *lastKey = getLastKey();
    if (!lastKey) return TVIDDTimeUnit_Lowest;
    return lastKey->frame;
  }
  
  //-------------------------------------------------------------------------  
  int getMaxFrameDelta() {
    int maxDelta = 0;    
    for (int t=0; t<keys.size()-1; t++) {
      int delta = (keys[t+1].frame - keys[t].frame);
      if (delta > maxDelta) maxDelta = delta;
    }
    return maxDelta;
  }
  
  //-------------------------------------------------------------------------  
  void clearAllKeys() {
    keys.clear();
    keyZoneBarrierFrames.clear();
  }
 
  //-------------------------------------------------------------------------  
  TTypedAnimKey *addKey(TVIDDTimeUnit frame, const T &value, bool optimizedAdd=true) {
    TTypedAnimKey *lastKey = getLastKey();      
    
    if (!lastKey || frame > lastKey->frame) {
      // tacking the key on the end. this is optimized.
      if (optimizedAdd) {
        TVIDDTimeUnit savedPrevFrame = prevFrame;
        prevFrame = frame;
        
        if (lastKey) {
          if (lastKey->value == value) return 0;
          if (savedPrevFrame && lastKey->frame != savedPrevFrame) keys.push_back(TTypedAnimKey(savedPrevFrame, lastKey->value));
        }
      }
          
      keys.push_back(TTypedAnimKey(frame, value));
      return &keys[keys.size()-1];    
    }
    
    // inserting the key somewhere besides the end. this is potentially slow.
    for (int i=0; i<keys.size(); i++) {
      if (frame == keys[i].frame) {
        keys[i].value = value;
        return &keys[i];
      }
      if (frame < keys[i].frame) {
        std::vector< TTypedAnimKey >::iterator iter = keys.begin();
        iter += i;
        keys.insert(iter, TTypedAnimKey(frame, value));
        return &keys[i];
      }
    }

    keys.push_back(TTypedAnimKey(frame, value));
    return &keys[keys.size()-1];
  }
  
  //-------------------------------------------------------------------------  
  void insertKeyBarrier(TVIDDTimeUnit frame) {
    if (frame == getLowestFrame()) return;
    keyZoneBarrierFrames.push_back(frame);   
  }
  
  //-------------------------------------------------------------------------  
  TTypedAnimKey *getKey(TVIDDTimeUnit frame) {
    for (int k=0; k<keys.size(); k++) {
      if (keys[k].frame == frame) return &keys[k];    
    }
    return 0;
  }
  
  //-------------------------------------------------------------------------  
  T *getValue(TVIDDTimeUnit frame, TVIDDAnimSolver solver=VAS_STEP) {    
    if (!keys.size()) return 0;

    if (keys.size() == 1) {
      if (frame >= keys[0].frame) return &keys[0].value;
      return 0;
    }

    int lowIndex = 0;
    int highIndex = keys.size()-1;
    int prevIndex, nextIndex;
    
    // binary search for the key index that immediately
    // preceeds or equals the requested frame
    while (true) {
      prevIndex = (highIndex+lowIndex)/2;      
      
      if (frame >= keys[prevIndex].frame) {
        if (prevIndex == keys.size()-1) break;
        if (frame < keys[prevIndex+1].frame) break;
        if (prevIndex == highIndex-1) {
          lowIndex = highIndex;
          continue;
        }
        lowIndex = prevIndex;
      }
      else {
        highIndex = prevIndex;
      }
      if (highIndex <= lowIndex) return 0;    
    }
    nextIndex = prevIndex+1;
 
    if (solver == VAS_LINEAR && nextIndex < keys.size() && getInSameKeyZone(keys[prevIndex], keys[nextIndex])) {
      // lerp between prev and next frame values
      workValue = getLerped(frame, keys[prevIndex].frame, keys[prevIndex].value, keys[nextIndex].frame, keys[nextIndex].value);
      return &workValue;
    }
    return &keys[prevIndex].value;    
  }
  
  //-------------------------------------------------------------------------   
  void getKeyedValuesBetweenFrames(std::vector<T*> &vec, TVIDDTimeUnit firstFrame, TVIDDTimeUnit secondFrameInclusive) {
    vec.clear();
    if (firstFrame == secondFrameInclusive) {
      TTypedAnimKey *key = getKey(firstFrame);
      if (key) vec.push_back(&key->value);  
    }
    else if (firstFrame > secondFrameInclusive) {
      for (int i=keys.size()-1; i>=0; i--) {
        if (keys[i].frame >= firstFrame) continue;
        if (keys[i].frame < secondFrameInclusive) break;
        vec.push_back(&keys[i].value);
      }  
    }
    else {
      for (int i=0; i<keys.size(); i++) {
        if (keys[i].frame > secondFrameInclusive) break;
        if (keys[i].frame <= firstFrame) continue;
        vec.push_back(&keys[i].value);
      }
    }
  }

  //------------------------------------------------------------------------- 
  virtual void read(TViddFileIn &in) {
    clearAllKeys();
    int frame, numKeys, frameDelta;

    in.read(numKeys);
    in.read(frame);
    
    std::vector<int> frames;
    frames.push_back(frame);
    
    for (int i=0; i<numKeys-1; i++) {
      in.read(frameDelta);
      frame += frameDelta;
      frames.push_back(frame);
    }
    
    T value = T(), valueDelta;
    
    if (!isDeltable(value)) {
      for (int i=0; i<numKeys; i++) {
        in.read(value);
        keys.push_back( TTypedAnimKey(frames[i], value) );
      }
      return;
    }
    
    in.read(value);
    keys.push_back( TTypedAnimKey(frames[0], value) );
    
    for (i=1; i<numKeys; i++) {
      in.read(valueDelta);
      value = getAdded(value, valueDelta);
      keys.push_back( TTypedAnimKey(frames[i], value) );    
    }
  }

  //-------------------------------------------------------------------------   
  virtual void write(TViddFileOut &out) {
    // write out first key time
    out.write((int)keys.size());
    out.write(keys[0].frame);    
    
    for (int i=0; i<keys.size()-1; i++) {
      int delta = keys[i+1].frame - keys[i].frame;
      out.write(delta);    
    }
    
    if (!isDeltable(keys[0].value)) {
      for (int i=0; i<keys.size(); i++) {
        out.write(keys[i].value);    
      }
      return;
    }
    
    out.write(keys[0].value);
    for (i=1; i<keys.size(); i++) {
      out.write( getDelta(keys[i].value, keys[i-1].value) );    
    }
  }

  //------------------------------------------------------------------------- 
  TAnimTrack<T>() {
    prevFrame = 0;
  }
};

//---------------------------------------------------------------------------

#endif