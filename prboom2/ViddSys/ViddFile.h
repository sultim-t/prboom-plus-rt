#ifndef VIDDFILE_H
#define VIDDFILE_H

#include <fstream>
#include "ViddBasic.h"

class TViddFileIn;
class TViddFileOut;

//---------------------------------------------------------------------------
#pragma pack(1)
struct TViddFileHeader {
  char viddStr[4]; // 'V','I','D','D'
  unsigned int version;
  unsigned short numSegments;
  unsigned int stringTablePos;
  unsigned int trailingDataPos;
};
#pragma pack()

//---------------------------------------------------------------------------
class TViddFileSegmentInfo {
public:
  enum TCompressMethod {
    CZ_NONE,
    CZ_BZIP,
    CZ_GZIP  
  };
  
protected: 
  std::string name;
  unsigned int dataStartPos;
  TVIDDTimeUnit firstFrame, lastFrame;
  TCompressMethod compressMethod;
  TStringMap localAttributes;
  
  void readHeader(TViddFileIn &in);
  void writeHeader(TViddFileOut &out);
    
  void readAttributes(TViddFileIn &in);
  void writeAttributes(TViddFileOut &out);
  
public:
  const std::string &getName() const { return name; }
  TCompressMethod getCompressMode() const { return compressMethod; }
  TVIDDTimeUnit getFirstFrame() const { return firstFrame; }
  TVIDDTimeUnit getLastFrame() const { return lastFrame; }
  TStringMap &getLocalAttributes() { return localAttributes; }

  void reset();
  
  friend class TViddRecorder;
  friend class TViddFileIn;
  friend class TViddFileOut;  
};

//---------------------------------------------------------------------------
class TViddFile {
private:
  struct TCompressor {
    enum TCompressionMode {
      CM_OFF,
      CM_READINGFROMCOMPRESSEDBUFFER,
      CM_WRITINGTOCOMPRESSEDBUFFER
    } mode;
    TViddFileSegmentInfo::TCompressMethod method;
    unsigned int bufferPos, bufferSize;
    unsigned char *buffer;
    void reset() {
      if (buffer) free(buffer);
      buffer = 0;
      bufferPos = 0;
      bufferSize = 0;
      mode = TCompressor::CM_OFF;
      method = TViddFileSegmentInfo::CZ_BZIP;
    }
    TCompressor() {
      buffer = 0;
      reset();
    }
  } compressor;
  
protected:
  std::string filename;
  std::fstream file;
  TViddFileHeader header;
  bool reading;
  TStringMap globalAttributes;
  TViddFileSegmentInfo::TCompressMethod globalDataCompressMethod;
  
  void cleanupAndThrowError(const char *error);
  
  enum TOpenMode {
    OM_READ,
    OM_WRITETRUNC,
    OM_WRITEAPP
  };
  
  void open(const std::string &filename_, TOpenMode openMode);

  void read(void *data, int size);
  void write(void *data, int size);
  
  void beginCompressedAccess(TViddFileSegmentInfo::TCompressMethod method);
  void endCompressedAccess();
  
  TViddFile() {}
  
public:  
  const std::string &getFilename() { return filename; }
  unsigned short getVersion();
  unsigned int getStreamPos();
  TStringMap &getGlobalAttributes() { return globalAttributes; }
  
  virtual void close();
};

//---------------------------------------------------------------------------
class TViddFileIn : public TViddFile {
private:
  std::vector<TViddFileSegmentInfo> segments;
  std::map<int, std::string> idToStringMap;
  const std::string &getString(int id);

public:
  int getNumSegments() const;
  TViddFileSegmentInfo &getSegment(int index);
  void prepForSegmentRead(int segmentIndex);

  void read(unsigned char &val) { read(&val, sizeof(val)); }
  void read(short &val) { read(&val, sizeof(val)); }
  void read(unsigned short &val) { read(&val, sizeof(val)); }
  void read(int &val) { read(&val, sizeof(val)); }
  void read(TVIDDFixedVector &val) { read(&val, sizeof(val)); }  
  void read(std::string &val);
  void read(std::vector<TVIDDTriggeredSound> &val);
  void read(TStringMap &val);
  void read(void *data, int size) { TViddFile::read(data, size); }
  
  void open(const std::string &filename_);
};

//---------------------------------------------------------------------------
class TViddFileOut : public TViddFile {

private:
  std::map<std::string, int> stringToIdMap;
  int getStringId(const std::string &str);

public:
  void beginSegmentWrite(TViddFileSegmentInfo &segment);
  void endSegmentWrite();

  void write(unsigned char val) { write(&val, sizeof(val)); }
  void write(short val) { write(&val, sizeof(val)); }
  void write(unsigned short val) { write(&val, sizeof(val)); }
  void write(int val) { write(&val, sizeof(val)); }
  void write(const TVIDDFixedVector &val) { write((void*)&val, sizeof(val)); }
  void write(const std::string &val);
  void write(const std::vector<TVIDDTriggeredSound> &val);
  void write(TStringMap &val);
  void write(void *data, int size) { TViddFile::write(data, size); }
  
  void open(const std::string &filename_, bool append=false);
  virtual void close();
};

//---------------------------------------------------------------------------
class TViddFileException {
  public:
  std::string filename;
  std::string error;
  const std::string getMsg() {
    return (filename + std::string(" : ") + error);    
  }
  TViddFileException(const std::string &filename_, const std::string &error_) {
    filename = filename_;
    error = error_;
  }
};

//---------------------------------------------------------------------------



#endif