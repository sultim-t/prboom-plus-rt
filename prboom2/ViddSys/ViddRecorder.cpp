#include "stdafx.h"
#include <fstream>
#include <stdio.h>

#include <windows.h>

#include "ViddRecorder.h"
#include "XML\AdvXMLParser.h"

TViddRecorder viddRecorder;

//---------------------------------------------------------------------------
// TViddRecorder
//---------------------------------------------------------------------------
TViddRecorder::TViddRecorder() : TViddController(MODE_RECORD) {
}

//---------------------------------------------------------------------------
void setGlobalAttribute(TViddFileOut &out, const std::string &key, const std::string &value) {
  //MessageBox(0,value.c_str() , key.c_str(), MB_OK);  
  out.getGlobalAttributes()[key] = value;
}

//---------------------------------------------------------------------------
void recurseValues(TViddFileOut &out, const std::string &prefix, AdvXMLParser::Element &elem) {
  //  this will take an xml file structured as follows:
  //   
  //  <file>
  //   <person name="john">
  //      <fathersname>edward</fathersname>
  //    </person>
  //    <person>
  //      <name>jamie</name>
  //      <mother name="beth"/>
  //      <mother>
  //        <name>sue</name>
  //      </mother>
  //    </person>  
  //  </file>
  //
  //  .. and convert it to a std::map<std::string, std::string> like so:
  //
  //  KEY                         VALUE
  //  person_name           =>    john
  //  person_fathersname    =>    edward
  //  person1_name          =>    jamie
  //  person1_mother_name   =>    beth  
  //  person1_mother1_name  =>    sue  
  
  char buf[VIDD_MAXSTRLEN];
  int null = 0;
  
  for (int a=0;; a++) {
    AdvXMLParser::Attribute &attrib = elem.GetAttribute(a);
    if (&attrib == &AdvXMLParser::Attribute::null) break;
    setGlobalAttribute(out, prefix + attrib.GetName(), attrib.GetValue());
  }

  std::map<std::string, int> countMap;

  for (int e=0;;e++) {
    AdvXMLParser::Element &subelem = elem.GetElement(e);
    if (&subelem == &AdvXMLParser::Element::null) break; 
    
    int count = 0;
    std::map<std::string, int>::iterator iter = countMap.find(subelem.GetName());
    if (iter == countMap.end()) countMap[subelem.GetName()] = count;
    else count = iter->second;
    
    std::string key;
    
    if (count) _snprintf(buf, VIDD_MAXSTRLEN-1, "%s%s%i_", prefix.c_str(), subelem.GetName().c_str(), count);
    else _snprintf(buf, VIDD_MAXSTRLEN-1, "%s%s_", prefix.c_str(), subelem.GetName().c_str());
    
    buf[VIDD_MAXSTRLEN-1] = 0;
    key = buf;
    
    recurseValues(out, key, subelem);
    
    countMap[subelem.GetName()]++;
  }
  
  if (&elem.GetElement(null) != &AdvXMLParser::Element::null) return;  
  if (!prefix.length() || !elem.GetValue().length()) return;
  
  std::string key = prefix.substr(0, prefix.length()-1);
  std::string value =  elem.GetValue();
  
  setGlobalAttribute(out, key, value);
}

//---------------------------------------------------------------------------
bool TViddRecorder::open(const std::string &paramFilename) {
  AdvXMLParser::Document *doc = 0;
  curSegmentIndex = -1;
  
  try {
    // read the paramfile into a string
    std::ifstream paramFile(paramFilename.c_str(), std::ios_base::in);
    if (!paramFile.is_open() || !paramFile.good()) {
      throw TViddFileException(paramFilename, "Unable to open file."); 
    }
    
    std::string paramFileContents;

    while (!paramFile.eof()) {
      char buf[2] = { 0, 0 };
      paramFile.read(buf, 1);
      paramFileContents += buf;
    }
    
    // parse it as an .xml doc
    AdvXMLParser::Parser parser;
    doc = parser.Parse(paramFileContents.c_str(), paramFileContents.length());
    AdvXMLParser::Element &root = doc->GetRoot();

    if (root.GetName() != "vidd") {
      throw TViddFileException(paramFilename, "Expected root element named \"vidd\".");
    }
    
    // convert all elements/attributes/values in the xml into the globalAttributes
    // for the TViddFile
    recurseValues(out, "", root);
    
    // verify that there's a name for the vidd file being written
    filename = out.getGlobalAttributes()["name"];
    if (!filename.length()) throw TViddFileException(paramFilename, "You must specify a \"name\" attribute for the \"vidd\" node.");
    
    // get the compressMethod
    std::string str = out.getGlobalAttributes()["compress"];
    if (str == "" || str == "none") compressMethod = TViddFileSegmentInfo::CZ_NONE;
    else if (str == "bzip") compressMethod = TViddFileSegmentInfo::CZ_BZIP;
    else if (str == "gzip") compressMethod = TViddFileSegmentInfo::CZ_GZIP; 
    else throw TViddFileException(paramFilename, "\"compress\" attribute must be unspecified, \"none\", \"bzip\", or \"gzip\" only.");
      
    // read in the annotations
    std::string attName;
    int numLmps = 0;
    for (int e=0;;e++) {
      AdvXMLParser::Element &elem = root.GetElement(e);
      if (&elem == &AdvXMLParser::Element::null) break; 
      if (elem.GetName() != "annotation") continue;
      
      // read in annotation
      std::string segmentStr = elem.GetAttribute("segment").GetValue();
      std::string timeStr = elem.GetAttribute("time").GetValue();
      int segment = 0;
      float time = 0;
      if (segmentStr.length()) sscanf(segmentStr.c_str(), "%i", &segment);
      if (timeStr.length()) sscanf(timeStr.c_str(), "%f", &time);
      annotations[segment].push_back( TAnnotation(time, elem.GetValue()) );
    }

    TViddController::open(filename);
    out.open(filename);
  }
  catch (TViddFileException &e) {
    popupErrorMessageBox("VIDD Error", e.getMsg().c_str());
    return false;
  }
  catch (const AdvXMLParser::ParsingException &e) {
    std::string msg;
    char buf[VIDD_MAXSTRLEN];

    msg += "\"";
    msg += paramFilename;
    msg += "\"   (line ";
    msg += itoa(e.GetLine(), buf, 10);
    msg += ", column ";
    msg += itoa(e.GetColumn(), buf, 10);
    msg += ")\r\n\r\n";
    
    msg += e.GetErrorMessage();
    msg += "\r\n";
    popupErrorMessageBox("XML Parsing Error", msg.c_str());
    if (doc) delete doc;
    return false;
  }
  
  return true;
}

//---------------------------------------------------------------------------
void TViddRecorder::beginSegment(const std::string &segmentName) {
  flushCurSegment();
  curSegmentInfo.reset();
  curSegmentInfo.name = segmentName;
  curSegmentIndex++;
}

//---------------------------------------------------------------------------
void TViddRecorder::flushCurSegment() {
  if (!curSegmentInfo.name.length()) return;
  
  infoNotifier.print("Flushing segment \"%s\" to disk...\n", curSegmentInfo.name.c_str());
    
  curSegmentInfo.firstFrame = 0x7fffffff;
  curSegmentInfo.lastFrame = 0;
  for (int t=0; t<elementTracks.size(); t++) {
    TVIDDTimeUnit frame = elementTracks[t]->getLowestFrame();
    if (frame < curSegmentInfo.firstFrame) curSegmentInfo.firstFrame = frame;
    frame = elementTracks[t]->getHighestFrame();
    if (frame > curSegmentInfo.lastFrame) curSegmentInfo.lastFrame = frame;  
  }
  curSegmentInfo.compressMethod = compressMethod;
  out.beginSegmentWrite(curSegmentInfo);
  
  infoNotifier.print("    Frame range: %i to %i\n", curSegmentInfo.firstFrame, curSegmentInfo.lastFrame);
  
  // write out event tracks
  int totalEvents = 0;
  int numTracks = eventTracks.size();
  out.write(numTracks);
  for (TStringAnimTrackMapIter iter=eventTracks.begin(); iter!=eventTracks.end(); iter++) {
    out.write(iter->first);
    iter->second.write(out);
    totalEvents += iter->second.getNumKeys();
  }
  infoNotifier.print("    Generic events: %i\n", totalEvents);
  
  // write out sound track
  soundTrack.write(out);
  
  infoNotifier.print("    Sound events: %i\n", soundTrack.getNumEventsTotal());
  
  // write out element tracks
  numTracks = elementTracks.size();
  out.write(numTracks);
  for (t=0; t<numTracks; t++) {
    elementTracks[t]->write(out);    
  }
  
  infoNotifier.print("    Element tracks: %i\n", numTracks);
  
  out.endSegmentWrite();
  
  TViddController::reset();
  
  infoNotifier.print("...Done flushing segment\n", numTracks);
  
  curSegmentInfo.reset();
}
  
//---------------------------------------------------------------------------
void TViddRecorder::close() {
  flushCurSegment();
  out.close();
}

//---------------------------------------------------------------------------
void TViddRecorder::setFrame(TVIDDTimeUnit frame) {
  TViddController::setFrame(frame);
  soundTrack.setFrame(frame);
  for (int i=0; i<elementTracks.size(); i++) { elementTracks[i]->setFrame(frame); }
  
  // check for annotation crossing
  TAnnotationMap::iterator iter = annotations.find(curSegmentIndex);
  if (iter == annotations.end()) return;
  
  std::vector<TAnnotation> &segmentAnnots = iter->second;  
  TVIDDTimeUnit annotTime;
  
  for (i=0; i<segmentAnnots.size(); i++) {
    // how about we hardcode an arbitrary number? sound good?
    // there should be a viddSys_setFramesPerSecond(int fps) API func
    annotTime = segmentAnnots[i].time * 35 * 1000;
    if (annotTime > curFrame || annotTime < prevFrame) continue;
    // we've just crossed one
    TVIDDTriggeredEvent event;
    event.verb = "annotate";
    event.subject = segmentAnnots[i].text.c_str();
    addEvent(event);
  }
}

//---------------------------------------------------------------------------
void TViddRecorder::addEvent(const TVIDDTriggeredEvent &event) {
  eventTracks[std::string(event.verb)].addKey(curFrame, std::string(event.subject), prevFrame);
}

//---------------------------------------------------------------------------
void TViddRecorder::addSound(const TVIDDTriggeredSound &sound) {
  soundTrack.addEvent(sound);
}

//---------------------------------------------------------------------------
bool TViddRecorder::registerElementDestruction(const TVIDDElementHandle &handle) {
  TViddElementTrack *track = getTrack(handle, false);
  if (!track) return false;
  track->setHandle(nullHandle);
  return true;
}