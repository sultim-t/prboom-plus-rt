#ifndef VIDD_ELEMENTTRACK_H
#define VIDD_ELEMENTTRACK_H

#include "ViddSys.h"
#include "ViddBasic.h"
#include "ViddTypedPropertyTracks.h"
#include <string>
#include <fstream>
#include <map>

//---------------------------------------------------------------------------
// TViddElementTrack
//---------------------------------------------------------------------------
class TViddElementTrack {  
protected:
  enum TPropertyType {
    VPT_INT,
    VPT_FIXEDVECTOR,
    VPT_STRING,
    VPT_MAXTYPES
  };

  TVIDDElementHandle handle, prevHandle;
  std::vector<TViddTypedPropertyTracksBase*> typedPropertyTracks;  
  int curFrame, prevFrame;
  void *userData;
  TViddControllerMode mode;  
  
public:
  void *getUserData() const { return userData; }
  void setUserData(void *userData_) { userData = userData_; }
    
  const TVIDDElementHandle &getHandle() const { return handle; }
  const TVIDDElementHandle &getPrevHandle() const { return prevHandle; }
  
  void setFrame(int frame_);  
  void setHandle(const TVIDDElementHandle &handle_);
 
  TVIDDTimeUnit getLowestFrame(); 
  TVIDDTimeUnit getHighestFrame();
  
  void read(TViddFileIn &in);  
  void write(TViddFileOut &out);
  
  void setIntProperty(const TVIDDElementProperty prop, const int value);  
  int getIntProperty(const TVIDDElementProperty prop, const TVIDDAnimSolver solver=VAS_STEP);
  
  void setFixedVectorProperty(const TVIDDElementProperty prop, const TVIDDFixedVector &value);
  TVIDDFixedVector getFixedVectorProperty(const TVIDDElementProperty prop, const TVIDDAnimSolver solver=VAS_STEP);
  
  void setStringProperty(const TVIDDElementProperty prop, const std::string &value);
  const std::string &getStringProperty(const TVIDDElementProperty prop);
  
  TViddElementTrack(TViddControllerMode mode_, int firstFrame = 0);
  ~TViddElementTrack();
};

//---------------------------------------------------------------------------

#endif