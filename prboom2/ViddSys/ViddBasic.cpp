#include "stdafx.h"
#include "ViddBasic.h"
#include "bzlib/bzlib.h"

//---------------------------------------------------------------------------  
const TVIDDElementHandle nullHandle = { VET_NONE, 0 };
const std::string nullStr;
TViddSysInfoNotifier infoNotifier;

//---------------------------------------------------------------------------  
void popupErrorMessageBox(const char *title, const char *message) {
  infoNotifier.print("%s: %\n", title, message);
  
#if 0
  MessageBox(0, message, title, MB_ICONERROR | MB_OK);
#endif
}

//---------------------------------------------------------------------------  
void TViddSysInfoNotifier::print(const char *s, ...) {
  if (!callback) return;
  
  static char msg[VIDD_MAXSTRLEN];
  va_list v;
  va_start(v,s);
  vsprintf(msg,s,v);
  va_end(v);
  msg[VIDD_MAXSTRLEN-1] = 0;
  
  callback(msg);
}

//---------------------------------------------------------------------------
std::string getTimeAsString(int milliseconds) {
  static char buf[VIDD_MAXSTRLEN];
  int t, seconds, minutes, hours, frac;
  t = milliseconds/1000;
  seconds = t % 60;
  t = (t - seconds) / 60;
  minutes = t % 60;
  t = (t - minutes) / 60;
  hours = t;
  frac = (milliseconds%1000)/10;
  if (hours) {
    sprintf(buf, "%2i:%02i:%02i.%02i", hours, minutes, seconds, frac);  
  }
  else if (minutes) {
    sprintf(buf, "%2i:%02i.%02i", minutes, seconds, frac);  
  }
  else {
    sprintf(buf, "%2i.%02i", seconds, frac);  
  }
  return std::string(buf);
}

//---------------------------------------------------------------------------
const char *getBZipError(int id) {
  switch(id) { 
    case BZ_OK: return "BZ_OK";
    case BZ_RUN_OK: return "BZ_RUN_OK";
    case BZ_FLUSH_OK: return "BZ_FLUSH_OK";
    case BZ_FINISH_OK: return "BZ_FINISH_OK";
    case BZ_STREAM_END : return "BZ_STREAM_END";
    case BZ_SEQUENCE_ERROR: return "BZ_SEQUENCE_ERROR";
    case BZ_PARAM_ERROR: return "BZ_PARAM_ERROR";
    case BZ_MEM_ERROR: return "BZ_MEM_ERROR";
    case BZ_DATA_ERROR: return "BZ_DATA_ERROR";
    case BZ_DATA_ERROR_MAGIC: return "BZ_DATA_ERROR_MAGIC";
    case BZ_IO_ERROR: return "BZ_IO_ERROR";
    case BZ_UNEXPECTED_EOF: return "BZ_UNEXPECTED_EOF";
    case BZ_OUTBUFF_FULL: return "BZ_OUTBUFF_FULL";
    case BZ_CONFIG_ERROR : return "BZ_CONFIG_ERROR";
  }
  return "Unknown BZip Error";  
}