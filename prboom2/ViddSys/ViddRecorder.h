#ifndef VIDDRECORDER_H
#define VIDDRECORDER_H

#include "ViddController.h"

//---------------------------------------------------------------------------
// TViddRecorder
//---------------------------------------------------------------------------
class TViddRecorder : public TViddController {
private:
  TViddFileOut out;
  
  int curSegmentIndex;
  TViddFileSegmentInfo curSegmentInfo;
  void flushCurSegment();

  struct TAnnotation {
    int segmentIndex;
    float time;
    std::string text;
    TAnnotation(float time_, const std::string &text_) {
      time = time_;
      text = text_;
    }
  };
  typedef std::map<int, std::vector<TAnnotation> > TAnnotationMap;
  TAnnotationMap annotations;
  
  TViddFileSegmentInfo::TCompressMethod compressMethod;
  
public:
  virtual bool open(const std::string &paramFilename);
  void close();
  
  virtual TStringMap &getAttributes(TVIDDAttributeScope scope)  {
    return (scope == VAT_GLOBAL) ? out.getGlobalAttributes() : curSegmentInfo.getLocalAttributes();
  }
  
  void beginSegment(const std::string &segmentName);
  virtual void setFrame(TVIDDTimeUnit frame);
  
  void addEvent(const TVIDDTriggeredEvent &event);
  void addSound(const TVIDDTriggeredSound &sound);
  
  bool registerElementDestruction(const TVIDDElementHandle &handle);
  
  virtual TViddElementTrack *getTrack(const TVIDDElementHandle &handle, bool createOnFail=true) {
    return TViddController::getTrack(handle, MODE_RECORD, createOnFail);
  }
  virtual int getFileVersion() { return out.getVersion(); }
    
  TViddRecorder();
};
//---------------------------------------------------------------------------
extern TViddRecorder viddRecorder;
//---------------------------------------------------------------------------

#endif