#ifndef VIDDTYPEDPROPERTYTRACKS_H
#define VIDDTYPEDPROPERTYTRACKS_H

#include "AnimTrack.h"

//---------------------------------------------------------------------------
// TViddTypedPropertyTracksBase
//---------------------------------------------------------------------------
class TViddTypedPropertyTracksBase {
public:
  virtual int getNumTracks() = 0;
  virtual void clear() = 0;
  virtual TVIDDTimeUnit getLowestFrame() = 0;
  virtual TVIDDTimeUnit getHighestFrame() = 0;
  virtual void insertKeyBarriers(TVIDDTimeUnit frame) = 0;
  virtual void read(TViddFileIn &in) = 0;  
  virtual void write(TViddFileOut &out) = 0;
};

//---------------------------------------------------------------------------
// TViddTypedPropertyTracks
//---------------------------------------------------------------------------
template <class T> class TViddTypedPropertyTracks : public TViddTypedPropertyTracksBase {
private:
  std::map <TVIDDElementProperty, TAnimTrack<T> > tracks;

public:
  virtual int getNumTracks() { return tracks.size(); } 
   
  virtual void clear() { tracks.clear(); }
  
  TAnimTrack<T> *getTrack(TVIDDElementProperty prop, bool createOnFail=true) {
    std::map<TVIDDElementProperty, TAnimTrack<T> >::iterator iter = tracks.find(prop);
    if (iter != tracks.end()) return &iter->second;
    if (!createOnFail) return 0;
    return &tracks[prop];
  }
  
  virtual TVIDDTimeUnit getLowestFrame() {
    TVIDDTimeUnit lowestFrame = TVIDDTimeUnit_Highest;    
    std::map<TVIDDElementProperty, TAnimTrack<T> >::iterator iter = tracks.begin(); 
    for (;iter != tracks.end(); iter++) {
      if (iter->second.getLowestFrame() > lowestFrame) continue;
      lowestFrame = iter->second.getLowestFrame();
    }
    return lowestFrame;    
  }

  virtual TVIDDTimeUnit getHighestFrame() {
    TVIDDTimeUnit highestFrame = TVIDDTimeUnit_Lowest;
    std::map<TVIDDElementProperty, TAnimTrack<T> >::iterator iter = tracks.begin(); 
    for (;iter != tracks.end(); iter++) {
      if (iter->second.getHighestFrame() < highestFrame) continue;
      highestFrame = iter->second.getHighestFrame();
    }
    return highestFrame;    
  }
  
  virtual void insertKeyBarriers(TVIDDTimeUnit frame) {
    std::map<TVIDDElementProperty, TAnimTrack<T> >::iterator iter = tracks.begin(); 
    for (;iter != tracks.end(); iter++) {
      iter->second.insertKeyBarrier(frame);
    }  
  }
  
  virtual void read(TViddFileIn &in) {
    tracks.clear();  
    unsigned short numTracks; 
    unsigned short property;
    in.read(numTracks);
    for (int t=0; t<numTracks; t++) {
      in.read(property);
      tracks[(TVIDDElementProperty)property].read(in);
    }    
  }
  
  virtual void write(TViddFileOut &out) {
    unsigned short numTracks = tracks.size();
    unsigned short property;
    out.write(numTracks);
    std::map<TVIDDElementProperty, TAnimTrack<T> >::iterator iter = tracks.begin(); 
    for (;iter != tracks.end(); iter++) {
      property = (unsigned short)iter->first;
      out.write(property);
      iter->second.write(out);
    }
  }
};

#endif