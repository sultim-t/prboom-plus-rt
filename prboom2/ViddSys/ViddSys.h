#ifndef VIDDSYS_H
#define VIDDSYS_H

#define VIDD_MAXSTRLEN 1024

/*---------------------------------------------------------------------------
  General overview of the ViddSys:
  
  The VIDD system records arbitrary "properties" for any number of "elements". An
  element would be any object that had a set of properties that may change over
  the course of a demo, such as an enemy mobj, the player, or a door sector. Each
  element gets it's own set of "animation" tracks to record the changes to its
  properties. When recording a VIDD, the TViddRecorder object creates new property 
  tracks for elements and updates their values at each frame (via the dll API defined
  in this file). Each VIDD is broken into "segments", where all element tracks
  are flushed (ie, when a new level loads). For playing back a VIDD, the 
  TViddPlayer object reads all elements and their property tracks back in, which
  can be queried through the dll API.  
  
  TViddController:
    base-class object responsible for managing the different elements and their
    property tracks and either reading or writing a VIDD file.
    
  TViddRecorder:
    TViddController used to create a VIDD file.
  
  TViddPlayer:
    TViddController used to read a VIDD file.
    
  TViddFile:
    I/O class that encapsulates reading the separate VIDD segments, generic VIDD 
    attributes, and string table, etc from a disk file. Also handles 
    compression/decompression transparently.
  
  TViddElementTrack:
    The main class that holds all the "tracks" of a VIDD element's properties.
    
  TViddTypedPropertyTracks:
    holds all property tracks of the same type for a VIDD element. ie. All elements
    have a set of <int> tracks, <std::string> tracks, and <TVIDDFixedVector> tracks
  
  TAnimTrack: 
    responsible for storing and fetching frame/value pairs of any type:
    (float, string, vector, etc)
*///-------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function and calling convention macros
//---------------------------------------------------------------------------
#if ((defined VIDDEXPORT) || (defined VIDDSYS_EXPORTS))
  #define VIDDIMPORTEXPORT dllexport
#else
  #define VIDDIMPORTEXPORT dllimport
#endif

//---------------------------------------------------------------------------
#ifdef __cplusplus
  #define VIDDAPI extern "C" _declspec(VIDDIMPORTEXPORT)
#else
  #define VIDDAPI _declspec(VIDDIMPORTEXPORT)
#endif

//---------------------------------------------------------------------------
#ifdef __cplusplus
  extern "C" {
#endif

//---------------------------------------------------------------------------
// custom types
//---------------------------------------------------------------------------
typedef unsigned int TVIDDElementId;
typedef int TVIDDTimeUnit;

#define TVIDDTimeUnit_Highest 0x7fffffff
#define TVIDDTimeUnit_Lowest 0

//---------------------------------------------------------------------------
typedef enum {
  VET_NONE    = 0,
  VET_USER    = 100
} TVIDDElementType;

//---------------------------------------------------------------------------
typedef enum {
  VEP_NONE = 0,
  VEP_ELEMENT_TYPE,
  VEP_ELEMENT_ID,  
  VEP_USER = 100
} TVIDDElementProperty;

//---------------------------------------------------------------------------
typedef struct {
  TVIDDElementType type;
  TVIDDElementId id;
} TVIDDElementHandle;

//---------------------------------------------------------------------------
typedef enum {
  VAT_GLOBAL,
  VAT_CURSEGMENT
} TVIDDAttributeScope;

//---------------------------------------------------------------------------
typedef struct {
  int soundId;
  TVIDDElementHandle sourceElementHandle;
} TVIDDTriggeredSound;

//---------------------------------------------------------------------------
typedef struct {
  const char *verb;
  const char *subject;
} TVIDDTriggeredEvent;

//---------------------------------------------------------------------------
typedef enum {
  VVS_JUSTAPPEARED = 0,
  VVS_VISIBLE,
  VVS_JUSTDISAPPEARED,
  VVS_MAXSTATES
} TVIDDVisibleState;

//---------------------------------------------------------------------------
typedef struct {
  int x, y, z;
} TVIDDFixedVector;

//---------------------------------------------------------------------------
typedef enum {
  VAS_STEP,
  VAS_LINEAR,
  VAS_ANGLE
} TVIDDAnimSolver;

//---------------------------------------------------------------------------
typedef struct {
  const char *name;
  TVIDDTimeUnit firstFrame, lastFrame;
} TVIDDSegmentInfo;

//---------------------------------------------------------------------------
// info funcs
//---------------------------------------------------------------------------
VIDDAPI int viddSys_getVersion();
typedef void (*TVIDDInfoCallback)(const char *str);
VIDDAPI void viddSys_setInfoCallback(TVIDDInfoCallback callback);

//---------------------------------------------------------------------------
// General attribute functions for per-segment and per-vidd named attribs
//---------------------------------------------------------------------------
VIDDAPI const char *viddSys_getAttribute(TVIDDAttributeScope scope, const char *name);
VIDDAPI int viddSys_getNumAttributes(TVIDDAttributeScope scope);
VIDDAPI const char *viddSys_getAttributeName(TVIDDAttributeScope scope, int index);
VIDDAPI const char *viddSys_getAttributeValue(TVIDDAttributeScope scope, int index);
VIDDAPI void viddSys_setAttribute(TVIDDAttributeScope scope, const char *name, const char *value);
VIDDAPI void viddSys_copyAttributesFromGlobalToCurSegment(const char *attributeNamePrefix);

//---------------------------------------------------------------------------
// record funcs
//---------------------------------------------------------------------------
VIDDAPI int viddRecorder_open(const char *paramFilename);

  VIDDAPI void viddRecorder_beginSegment(const char *name);

  VIDDAPI void viddRecorder_beginFrame(const TVIDDTimeUnit time);
  
    VIDDAPI void viddRecorder_registerEvent(const TVIDDTriggeredEvent event);    
    VIDDAPI void viddRecorder_registerSound(const int soundId, const TVIDDElementHandle sourceElementHandle);
    
    VIDDAPI void viddRecorder_setIntProp(
      const TVIDDElementHandle elementHandle,
      const TVIDDElementProperty property,
      const int value
    );
    
    VIDDAPI void viddRecorder_setVectorProp(
      const TVIDDElementHandle elementHandle,
      const TVIDDElementProperty property, 
      const int x, const int y, const int z
    );
    
    VIDDAPI void viddRecorder_setStringProp(
      const TVIDDElementHandle elementHandle,
      const TVIDDElementProperty property,
      const char *value
    );
    
    VIDDAPI int viddRecorder_registerElementDestruction(
      const TVIDDElementHandle elementHandle
    );
    
    VIDDAPI int viddRecorder_getElementExists(
      const TVIDDElementHandle elementHandle
    );    

  VIDDAPI void viddRecorder_endFrame();

VIDDAPI void viddRecorder_close();

//---------------------------------------------------------------------------
// playback funcs
//---------------------------------------------------------------------------
VIDDAPI int viddPlayer_open(const char *fromFilename);
  
  VIDDAPI int viddPlayer_getNumSegments();
  VIDDAPI TVIDDSegmentInfo viddPlayer_getSegmentInfo(int index);
  VIDDAPI int viddPlayer_getCurSegmentIndex();
  VIDDAPI int viddPlayer_loadSegment(int index);
  
  VIDDAPI int viddPlayer_getNumCurSegmentAttribs();
  VIDDAPI const char *viddPlayer_getCurSegmentAttribName(int attribIndex);
  VIDDAPI const char *viddPlayer_getCurSegmentAttribValue(int attribIndex);
  VIDDAPI const char *viddPlayer_getCurSegmentAttrib(const char *attribName);
  
  VIDDAPI TVIDDTimeUnit viddPlayer_getFirstFrame();
  VIDDAPI TVIDDTimeUnit viddPlayer_getLastFrame();  

  VIDDAPI void viddPlayer_beginFrame(const TVIDDTimeUnit time);

    VIDDAPI const char *viddPlayer_getTriggeredEventSubject(const char *verb);
    
    VIDDAPI int viddPlayer_getNumTriggeredSounds();
    VIDDAPI const TVIDDTriggeredSound viddPlayer_getTriggeredSound(const int index);
    
    VIDDAPI int   viddPlayer_getNumElements(const TVIDDVisibleState state);
    
    VIDDAPI const TVIDDElementHandle viddPlayer_getElementHandle_JustAppeared(int index);
    VIDDAPI const TVIDDElementHandle viddPlayer_getElementHandle_Visible(int index);    
    
    VIDDAPI void                   *viddPlayer_getElementUserData_JustDisappeared(int index);
    VIDDAPI void                    viddPlayer_setElementUserData_JustDisappeared(int index, void *userData);
    VIDDAPI const TVIDDElementType  viddPlayer_getElementType_JustDisappeared(int index);

    VIDDAPI void viddPlayer_setElementUserData(const TVIDDElementHandle elementHandle, void *userData);
    VIDDAPI void *viddPlayer_getElementUserData(const TVIDDElementHandle elementHandle);

    VIDDAPI int viddPlayer_getIntProp(      
      const TVIDDElementHandle elementHandle, 
      const TVIDDElementProperty property,
      const TVIDDAnimSolver solver
    );
    
    VIDDAPI TVIDDFixedVector viddPlayer_getVectorProp(      
      const TVIDDElementHandle elementHandle, 
      const TVIDDElementProperty property,
      const TVIDDAnimSolver solver
    );
    
    VIDDAPI const char *viddPlayer_getStringProp(      
      const TVIDDElementHandle elementHandle, 
      const TVIDDElementProperty property
    );
  
  VIDDAPI void viddPlayer_endFrame();

VIDDAPI void viddPlayer_close();

//---------------------------------------------------------------------------
#ifdef __cplusplus
  }
#endif
//---------------------------------------------------------------------------

#endif

