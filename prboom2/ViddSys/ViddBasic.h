#ifndef VIDD_BASIC_H
#define VIDD_BASIC_H

#include "ViddSys.h"
#include <map>
#include <string>

//---------------------------------------------------------------------------
typedef std::map<std::string, std::string> TStringMap;
typedef std::map<std::string, std::string> TStringMapIter;

//---------------------------------------------------------------------------
extern const TVIDDElementHandle nullHandle;
extern const std::string nullStr;

//---------------------------------------------------------------------------
std::string getTimeAsString(int milliseconds);
const char *getBZipError(int id);
void popupErrorMessageBox(const char *title, const char *message);

//---------------------------------------------------------------------------
inline bool operator == (const TVIDDElementHandle &h1, const TVIDDElementHandle &h2) {
  return ((h1.id == h2.id) && (h1.type == h2.type));// || (h1.id == 0 && h2.id == 0 && h1.type == VET_NONE && h2.type == VET_NONE);
}

//---------------------------------------------------------------------------
inline bool operator != (const TVIDDElementHandle &h1, const TVIDDElementHandle &h2) {
  return !(h1 == h2);
}

//---------------------------------------------------------------------------
inline bool operator < (const TVIDDElementHandle &h1, const TVIDDElementHandle &h2) {
  return (((unsigned __int64)h1.type<<32)|(h1.id)) < (((unsigned __int64)h2.type<<32)|(h2.id));
}

//---------------------------------------------------------------------------
inline bool operator == (const TVIDDTriggeredSound &s1, const TVIDDTriggeredSound &s2) {
  return (s1.soundId == s2.soundId && s1.sourceElementHandle == s2.sourceElementHandle);
}

//---------------------------------------------------------------------------
inline bool operator == (const TVIDDFixedVector &v1, const TVIDDFixedVector &v2) {
  return (v1.x == v2.x && v1.y == v2.y && v1.z == v2.z);
}

//---------------------------------------------------------------------------
enum TViddControllerMode {
  MODE_PLAY,
  MODE_RECORD
};

//---------------------------------------------------------------------------  
class TViddSysInfoNotifier {
private:
  TVIDDInfoCallback callback;
  
public:
  void setCallback(TVIDDInfoCallback callback_) { callback = callback_; }
  void print(const char *s, ...);
  TViddSysInfoNotifier() { callback = 0; }
};
extern TViddSysInfoNotifier infoNotifier;

//---------------------------------------------------------------------------  
inline TVIDDFixedVector operator - (const TVIDDFixedVector &vec1, const TVIDDFixedVector &vec2) {
  TVIDDFixedVector res;
  res.x = vec1.x - vec2.x;
  res.y = vec1.y - vec2.y;
  res.z = vec1.z - vec2.z;
  return res;
}

inline TVIDDFixedVector operator + (const TVIDDFixedVector &vec1, const TVIDDFixedVector &vec2) {
  TVIDDFixedVector res;
  res.x = vec1.x + vec2.x;
  res.y = vec1.y + vec2.y;
  res.z = vec1.z + vec2.z;
  return res;
}

inline TVIDDFixedVector operator * (const TVIDDFixedVector &vec, float t) {
  TVIDDFixedVector res;
  res.x = vec.x * t;
  res.y = vec.y * t;
  res.z = vec.z * t;
  return res;
}

//---------------------------------------------------------------------------


#endif