#include "stdafx.h"
#include "ViddElementTrack.h"
//---------------------------------------------------------------------------
// TViddElementTrack
//---------------------------------------------------------------------------
TViddElementTrack::TViddElementTrack(TViddControllerMode mode_, int firstFrame) {
  
  typedPropertyTracks.resize(VPT_MAXTYPES);
  //typedPropertyTracks[VPT_BYTE] = new TViddTypedPropertyTracks<unsigned char>();
  //typedPropertyTracks[VPT_SHORT] = new TViddTypedPropertyTracks<short>();  
  typedPropertyTracks[VPT_INT] = new TViddTypedPropertyTracks<int>();
  typedPropertyTracks[VPT_FIXEDVECTOR] = new TViddTypedPropertyTracks<TVIDDFixedVector>();
  typedPropertyTracks[VPT_STRING] = new TViddTypedPropertyTracks<std::string>();
  
  mode = mode_;
  handle = prevHandle = nullHandle;
  curFrame = prevFrame = 0;
  userData = 0;
  setFrame(firstFrame);   
} 

//---------------------------------------------------------------------------
TViddElementTrack::~TViddElementTrack() {
  /*
  for (int t=0; t<typedPropertyTracks.size(); t++) {
    delete typedPropertyTracks[t];
  }
  typedPropertyTracks.clear();
  */
}

//---------------------------------------------------------------------------
void TViddElementTrack::setFrame(int frame_) {
  prevFrame = curFrame;
  curFrame = frame_;
  prevHandle = handle; 
  if (mode != MODE_PLAY) return;
  handle.type = (TVIDDElementType)getIntProperty(VEP_ELEMENT_TYPE);
  handle.id = (TVIDDElementId)getIntProperty(VEP_ELEMENT_ID);
}

//---------------------------------------------------------------------------
void TViddElementTrack::setHandle(const TVIDDElementHandle &handle_) {
  if (mode != MODE_RECORD || handle == handle_) return;
  
  handle = handle_;
  // terminate the previous properties
  for (int t=0; t<typedPropertyTracks.size(); t++) {
    typedPropertyTracks[t]->insertKeyBarriers(curFrame);
  }  
  setIntProperty(VEP_ELEMENT_TYPE, (int)handle.type);
  setIntProperty(VEP_ELEMENT_ID, (int)handle.id);
}

//---------------------------------------------------------------------------
TVIDDTimeUnit TViddElementTrack::getLowestFrame() {
  TVIDDTimeUnit lowestFrame = TVIDDTimeUnit_Highest, frame;
  for (int t=0; t<typedPropertyTracks.size(); t++) {
    frame = typedPropertyTracks[t]->getLowestFrame();
    if (frame < lowestFrame) lowestFrame = frame;  
  }
  return lowestFrame;
}
  
//---------------------------------------------------------------------------
TVIDDTimeUnit TViddElementTrack::getHighestFrame() {
  TVIDDTimeUnit highestFrame = TVIDDTimeUnit_Lowest, frame;
  for (int t=0; t<typedPropertyTracks.size(); t++) {
    frame = typedPropertyTracks[t]->getHighestFrame();
    if (frame > highestFrame) highestFrame = frame;  
  }
  return highestFrame;   
}

//---------------------------------------------------------------------------
void TViddElementTrack::read(TViddFileIn &in) {
  for (int t=0; t<typedPropertyTracks.size(); t++) {
    typedPropertyTracks[t]->read(in); 
  }
  
  TAnimTrack<unsigned char> *idTrack = ((TViddTypedPropertyTracks<unsigned char>*)typedPropertyTracks[VPT_INT])->getTrack(VEP_ELEMENT_ID);
  for (int i=0; i<idTrack->getNumKeys(); i++) {
    const TAnimKey<unsigned char> &key = idTrack->getKeyFromIndex(i);
    for (int t=0; t<typedPropertyTracks.size(); t++) {
      typedPropertyTracks[t]->insertKeyBarriers(key.frame);
    }
  }
}

//---------------------------------------------------------------------------
void TViddElementTrack::write(TViddFileOut &out) {
  for (int t=0; t<typedPropertyTracks.size(); t++) {
    typedPropertyTracks[t]->write(out); 
  }   
}

//---------------------------------------------------------------------------
// Property Set/Get
//---------------------------------------------------------------------------
#define ADDKEY(datatype) \
  TAnimTrack<datatype> *track = ((TViddTypedPropertyTracks<datatype>*)typedPropertyTracks[type])->getTrack(prop, true); \
  track->addKey(curFrame, (datatype)value); \
  return;
  
//---------------------------------------------------------------------------
void TViddElementTrack::setIntProperty(const TVIDDElementProperty prop, const int value) {
  const TPropertyType type = VPT_INT;
  ADDKEY(int)
}    

//---------------------------------------------------------------------------
void TViddElementTrack::setFixedVectorProperty(const TVIDDElementProperty prop, const TVIDDFixedVector &value) {
  const TPropertyType type = VPT_FIXEDVECTOR;
  ADDKEY(TVIDDFixedVector)
}

//---------------------------------------------------------------------------
void TViddElementTrack::setStringProperty(const TVIDDElementProperty prop, const std::string &value) {
  const TPropertyType type = VPT_STRING;
  ADDKEY(std::string)
}

//---------------------------------------------------------------------------     
#define RETURNVALUE(datatype, def) \
  TAnimTrack<datatype> *track = ((TViddTypedPropertyTracks<datatype>*)typedPropertyTracks[type])->getTrack(prop, false); \
  if (!track) return def; \
  datatype *val = track->getValue(curFrame, solver); \
  if (!val) return def; \
  return *val;

//---------------------------------------------------------------------------  
int TViddElementTrack::getIntProperty(const TVIDDElementProperty prop, const TVIDDAnimSolver solver) {
  const TPropertyType type = VPT_INT;
  RETURNVALUE(int, 0)
}

//---------------------------------------------------------------------------  
TVIDDFixedVector TViddElementTrack::getFixedVectorProperty(const TVIDDElementProperty prop, const TVIDDAnimSolver solver) {
  static const TVIDDFixedVector defaultVec = {0,0,0};
  const TPropertyType type = VPT_FIXEDVECTOR;
  RETURNVALUE(TVIDDFixedVector, defaultVec)
}

//---------------------------------------------------------------------------  
const std::string &TViddElementTrack::getStringProperty(const TVIDDElementProperty prop) {
  const TPropertyType type = VPT_STRING;
  static const TVIDDAnimSolver solver = VAS_STEP;
  static const std::string defaultStr;
  RETURNVALUE(std::string, defaultStr)
}