#include "stdafx.h"
#include "Viddfile.h"
#include "zlib/zlib.h"
#include "bzlib/bzlib.h"
#include <mmsystem.h>

//---------------------------------------------------------------------------
// TViddFile
//---------------------------------------------------------------------------
void TViddFile::open(const std::string &filename_, TOpenMode openMode) {
  filename = filename_;  
  reading = (openMode == OM_READ);
  
  file.open(
    filename.c_str(), 
    std::ios::binary | 
    (openMode == OM_READ ? std::ios_base::in : std::ios_base::out) |
    (openMode == OM_WRITEAPP ? std::ios_base::app : 0)
  );
  
  if (reading) {
    file.read((char*)&header, sizeof(header));
    if (
      header.viddStr[0] != 'V' ||
      header.viddStr[1] != 'I' ||
      header.viddStr[2] != 'D' ||
      header.viddStr[3] != 'D' ||
      (
        header.version != 100 &&
        header.version != 102 &&
        header.version != 110
      )
    ) {
      cleanupAndThrowError("Not a valid VIDD file.");
    }
  }
  else if (openMode == OM_WRITETRUNC) {
    header.viddStr[0] = 'V';
    header.viddStr[1] = 'I';
    header.viddStr[2] = 'D';
    header.viddStr[3] = 'D';
    header.version = 110;
    header.numSegments = 0;
    file.write((char*)&header, sizeof(header));
  }
  
  if (!file || !file.is_open()) cleanupAndThrowError("Unable to open file.");
  
  if (header.version == 100) {
    globalDataCompressMethod = TViddFileSegmentInfo::CZ_NONE;
  }  
  else {
    globalDataCompressMethod = TViddFileSegmentInfo::CZ_BZIP;
  }
}

//---------------------------------------------------------------------------
void TViddFile::close() {
  compressor.reset();   
  file.close(); 
}

//---------------------------------------------------------------------------
void TViddFile::cleanupAndThrowError(const char *error) {
  close();
  throw TViddFileException(filename, error);
}

//---------------------------------------------------------------------------
void TViddFile::read(void *data, int size) {
  if (compressor.mode == TCompressor::CM_READINGFROMCOMPRESSEDBUFFER) {
    if ((compressor.bufferPos + size) > compressor.bufferSize) {
      cleanupAndThrowError("Reading past compressed buffer.");
    }
    memcpy(data, &compressor.buffer[compressor.bufferPos], size);
    compressor.bufferPos += size;
    if (compressor.bufferPos == compressor.bufferSize) {
      // automatically pull out of compression mode
      endCompressedAccess();
    }
    return;
  }
  if (file.eof()) cleanupAndThrowError("Reading past EOF.");
  file.read((char*)data, size);
}

//---------------------------------------------------------------------------
void TViddFile::write(void *data, int size) {
  if (compressor.mode == TCompressor::CM_WRITINGTOCOMPRESSEDBUFFER) {
    if (size > (compressor.bufferSize - compressor.bufferPos)) {
      compressor.bufferSize += 1024000 + size;
      compressor.buffer = (unsigned char *)realloc(compressor.buffer, compressor.bufferSize);
    }
    memcpy( &compressor.buffer[compressor.bufferPos], data, size );
    compressor.bufferPos += size;
    return;
  }
  file.write((char*)data, size);
}

//---------------------------------------------------------------------------
void TViddFile::beginCompressedAccess(TViddFileSegmentInfo::TCompressMethod method) {
  compressor.reset();
  compressor.method = method;
  
  if (reading) {    
    // read in the compressed size and the uncompressed size of the data
    unsigned int uncompressedSize, compressedSize;
    file.read((char*)&compressedSize, sizeof(compressedSize));
    file.read((char*)&uncompressedSize, sizeof(uncompressedSize));
    
    // read in the compressed data
    
    // sourceBuffer must use malloc since it may be assigned
    // to compressor.buffer, which utilizes free() and realloc()
    unsigned char *sourceBuffer = (unsigned char *)malloc(compressedSize);
    file.read((char*)sourceBuffer, compressedSize);
    
    // create the destination buffer to hold the uncompressed data    
    compressor.mode = TCompressor::CM_READINGFROMCOMPRESSEDBUFFER;
    compressor.bufferSize = uncompressedSize;
    compressor.bufferPos = 0;
    
    if (method == TViddFileSegmentInfo::CZ_NONE) {
      compressor.buffer = sourceBuffer;
    }
    else {
      // uncompress into the destination buffer          
      compressor.buffer = (unsigned char *)malloc(compressor.bufferSize);
      
      if (method == TViddFileSegmentInfo::CZ_BZIP) {
        // bzip uncompress
        int error = BZ2_bzBuffToBuffDecompress((char*)compressor.buffer, &compressor.bufferSize,
                    (char*)sourceBuffer, compressedSize, 0, 0);
        free(sourceBuffer);
        if (error != BZ_OK) cleanupAndThrowError(getBZipError(error));
      }

      if (method == TViddFileSegmentInfo::CZ_GZIP) {
        // gzip uncompress
        unsigned long bufferSize = compressor.bufferSize;
        int error = uncompress(compressor.buffer, &bufferSize, sourceBuffer, compressedSize);
        compressor.bufferSize = bufferSize;    
        free(sourceBuffer);
        if (error == Z_MEM_ERROR) cleanupAndThrowError("Z_MEM_ERROR");
        else if (error == Z_BUF_ERROR) cleanupAndThrowError("Z_BUF_ERROR");
        else if (error == Z_DATA_ERROR) cleanupAndThrowError("Z_DATA_ERROR");    
      }
    }  
  }
  else {
    // writing
    compressor.mode = TCompressor::CM_WRITINGTOCOMPRESSEDBUFFER;
    compressor.bufferSize = 1024;
    compressor.buffer = (unsigned char *)malloc(compressor.bufferSize);
    compressor.bufferPos = 0;
  }
}

//---------------------------------------------------------------------------
void TViddFile::endCompressedAccess() {
  if (reading) {
    compressor.reset();
  }
  else {
    unsigned int startTime = timeGetTime();
    
    // perform the compression in a memory buffer
    unsigned int destBufferSize;
    unsigned char *destBuffer;
 
    if (compressor.method == TViddFileSegmentInfo::CZ_NONE) {
      destBufferSize = compressor.bufferPos;
      destBuffer = compressor.buffer;    
    }
    else {
      destBufferSize = compressor.bufferPos*1.1 + 1024;
      destBuffer = new unsigned char[destBufferSize];
    
      if (compressor.method == TViddFileSegmentInfo::CZ_BZIP) {
        // bzip compress
        infoNotifier.print("Compressing data with bzip...\n");
        int error = BZ2_bzBuffToBuffCompress((char*)destBuffer, &destBufferSize, (char*)compressor.buffer, 
                    compressor.bufferPos, 9, 0, 30); //  blockSize100k, verbosity, workFactor 
        if (error != BZ_OK) {
          delete[] destBuffer;
          cleanupAndThrowError(getBZipError(error));
        }
      }    
      if (compressor.method == TViddFileSegmentInfo::CZ_GZIP) {
        // gzip compress
        infoNotifier.print("Compressing data with gzip...\n");
        unsigned long destBufferSizeAsULong = destBufferSize;
        int error = compress(destBuffer, &destBufferSizeAsULong, compressor.buffer, compressor.bufferPos);
        destBufferSize = destBufferSizeAsULong;
        if (error != Z_OK) {
          delete[] destBuffer;
          if (error == Z_MEM_ERROR) cleanupAndThrowError("Z_MEM_ERROR");
          else if (error == Z_BUF_ERROR) cleanupAndThrowError("Z_BUF_ERROR");
        }    
      }
      
      infoNotifier.print(
        "    Compression time: %s\n"
        "    Uncompressed size: %ik\n"
        "    Compressed size: %ik (%i%%)\n"
        "...Done compressing data\n",
        getTimeAsString(timeGetTime()-startTime).c_str(),
        compressor.bufferPos/1024,
        destBufferSize/1024,
        (int)(((float)destBufferSize/(float)compressor.bufferPos)*100)      
      );
    
    }
    // write out the compressed and uncompressed size of the data
    file.write((char*)&destBufferSize, sizeof(destBufferSize));
    file.write((char*)&compressor.bufferPos, sizeof(compressor.bufferPos));
        
    // write out the data
    file.write((char*)destBuffer, destBufferSize);
    
    if (compressor.method != TViddFileSegmentInfo::CZ_NONE) delete[] destBuffer;
    compressor.reset();
  }
}

//---------------------------------------------------------------------------
unsigned short TViddFile::getVersion() { return header.version; }

//---------------------------------------------------------------------------
unsigned int TViddFile::getStreamPos() {
  return file.tellp();
}

//---------------------------------------------------------------------------
// TViddFileIn
//---------------------------------------------------------------------------
void TViddFileIn::open(const std::string &filename_) {
  TViddFile::open(filename_, OM_READ);
  
  // read in the string table absolutely first
  file.seekg(header.stringTablePos, std::ios::beg);
  beginCompressedAccess(globalDataCompressMethod);
  int numStrings;
  read(numStrings);
  for (int s=0; s<numStrings; s++) {
    unsigned short len;
    std::string val;
    read(len);
    char letter;
    for (unsigned short c=0; c<len; c++) {
      read(&letter, sizeof(letter));
      val += letter;
    }
    idToStringMap[s] = val;
  }  
  endCompressedAccess();
  
  // read in the trailing data
  file.seekg(header.trailingDataPos, std::ios::beg);
  beginCompressedAccess(globalDataCompressMethod);
  // global attributes
  read(globalAttributes);
  endCompressedAccess();
  
  // reset back to the beginning
  file.seekg(sizeof(header), std::ios::beg);

  // skip through the file, reading in segment headers
  unsigned int dataSize;
  for (s=0; s<header.numSegments; s++) {
    TViddFileSegmentInfo segment;
    segment.readHeader(*this);
    segments.push_back(segment);
    
    // read in the size of the compressed data so we can skip it
    read((char*)&dataSize, sizeof(dataSize));
    file.seekg(dataSize+sizeof(unsigned int), std::ios::cur);  
  }
  
  if (!segments.size()) cleanupAndThrowError("No segments found.");
  
  // reset to the first segment
  prepForSegmentRead(0);
}

//---------------------------------------------------------------------------
int TViddFileIn::getNumSegments() const {
  return segments.size(); 
}

//---------------------------------------------------------------------------
TViddFileSegmentInfo &TViddFileIn::getSegment(int index) {
  return segments[index];
}
  
//---------------------------------------------------------------------------
void TViddFileIn::prepForSegmentRead(int segmentIndex) {
  file.seekg(segments[segmentIndex].dataStartPos, std::ios::beg);
  beginCompressedAccess(segments[segmentIndex].compressMethod);
}

//---------------------------------------------------------------------------
const std::string &TViddFileIn::getString(int id) {
  std::map<int, std::string>::iterator iter = idToStringMap.find(id);
  if (iter == idToStringMap.end()) return nullStr;
  return iter->second;
}

//---------------------------------------------------------------------------
void TViddFileIn::read(std::string &val) {
  int stringId;
  read(stringId);
  val = getString(stringId);
}

//---------------------------------------------------------------------------
void TViddFileIn::read(std::vector<TVIDDTriggeredSound> &val) {
  val.clear();
  unsigned short size;
  read(size);
  TVIDDTriggeredSound sound;
  for (unsigned short s=0; s<size; s++) {
    read(&sound, sizeof(sound));
    val.push_back(sound);
  }
}

//---------------------------------------------------------------------------
void TViddFileIn::read(TStringMap &val) {
  val.clear();
  unsigned short size;
  read(size);
  std::string key, value;
  for (unsigned short s=0; s<size; s++) {
    read(key);
    read(value);
    val[key] = value;  
  }
}

//---------------------------------------------------------------------------
// TViddFileOut
//---------------------------------------------------------------------------
void TViddFileOut::open(const std::string &filename_, bool append) {
  TViddFile::open(filename_, append ? OM_WRITEAPP : OM_WRITETRUNC);
}

//---------------------------------------------------------------------------
void TViddFileOut::close() {
  // write out the trailing data
  header.trailingDataPos = file.tellp();
  beginCompressedAccess(globalDataCompressMethod);  
  // general attributes
  write(globalAttributes);  
  endCompressedAccess();
  
  // write out the string table. this must be the absolute last
  // thing to be written
  header.stringTablePos = file.tellp();
  beginCompressedAccess(globalDataCompressMethod);  
  int numStrings = stringToIdMap.size();
  write(numStrings);  
  std::map<int, std::string> idToStringMap;
  std::map<std::string, int>::iterator iter = stringToIdMap.begin();
  for (;iter != stringToIdMap.end(); ++iter) {
    idToStringMap[iter->second] = iter->first;
  }
  for (int i=0; i<idToStringMap.size(); i++) {
    std::string &val = idToStringMap[i];
    unsigned short len = val.length();
    write(len);
    write((void*)val.c_str(), len);
  }
  endCompressedAccess();
  
  // reset back to the beginning
  file.seekp(sizeof(header), std::ios::beg);
  
  // go back to the start and re-write the header so numSegments
  // and trailingDataPos have the correct values
  file.seekp(0, std::ios::beg);
  file.write((char*)&header, sizeof(header));
  file.seekp(0, std::ios::end);
  
  // close it up
  TViddFile::close();
}

//---------------------------------------------------------------------------
void TViddFileOut::beginSegmentWrite(TViddFileSegmentInfo &segment) {
  segment.writeHeader(*this);
  beginCompressedAccess(segment.compressMethod);
}

//---------------------------------------------------------------------------
void TViddFileOut::endSegmentWrite() {
  endCompressedAccess();
  header.numSegments++;
}

//---------------------------------------------------------------------------
int TViddFileOut::getStringId(const std::string &str) {
  std::map<std::string, int>::iterator iter = stringToIdMap.find(str);
  if (iter != stringToIdMap.end()) return iter->second;
  int id = stringToIdMap.size();
  stringToIdMap[str] = id;
  return id;
}
  
//---------------------------------------------------------------------------
void TViddFileOut::write(const std::string &val) {
  write(getStringId(val));
}

//---------------------------------------------------------------------------
void TViddFileOut::write(const std::vector<TVIDDTriggeredSound> &val) {
  unsigned short size = val.size();
  write(size);
  for (unsigned short s=0; s<size; s++) {
    write((void*)&val[s], sizeof(val[s])); 
  }
}
  
//---------------------------------------------------------------------------
void TViddFileOut::write(TStringMap &val) {
  unsigned short size = val.size();
  write(size);
  TStringMap::iterator iter = val.begin();
  for (; iter != val.end(); ++iter) {
    write(iter->first);
    write(iter->second);
  }
} 

//---------------------------------------------------------------------------
// TViddFileSegmentInfo
//---------------------------------------------------------------------------
void TViddFileSegmentInfo::readHeader(TViddFileIn &in) {
  in.read(name);
  int cm; in.read(cm);
  compressMethod = (TCompressMethod)cm;
  in.read(firstFrame);
  in.read(lastFrame);
  in.read(localAttributes);
  dataStartPos = in.getStreamPos();
}

//---------------------------------------------------------------------------
void TViddFileSegmentInfo::writeHeader(TViddFileOut &out) {
  out.write(name);
  out.write((int)compressMethod);
  out.write(firstFrame);
  out.write(lastFrame);
  out.write(localAttributes);
}

//---------------------------------------------------------------------------
void TViddFileSegmentInfo::reset() {
  name = "";
  compressMethod = CZ_NONE;
  firstFrame = 0;
  lastFrame = 0;
  localAttributes.clear();
}
