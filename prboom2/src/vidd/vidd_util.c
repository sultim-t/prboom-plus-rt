#ifdef COMPILE_VIDD

#include "vidd_util.h"
#include "../ViddSys/ViddSys.h"
#include <stdio.h>
#include <string.h>

//---------------------------------------------------------------------------
int getStringAsInt(const char *str) {
  int val = 0;
  if (!str || !str[0]) return 0;
  sscanf(str, "%i", &val);
  return val;
}

//---------------------------------------------------------------------------
const char *getIntAsString(int val) {
  static char str[VIDD_MAXSTRLEN];
  _snprintf(str, VIDD_MAXSTRLEN-1, "%i", val);
  return str;
}

//---------------------------------------------------------------------------
void appendString(char *target, const char *toAppend, int maxLen) {
  if (!toAppend) return;
  strncat(target, toAppend, maxLen);
  target[maxLen-1] = 0;
}

//---------------------------------------------------------------------------
const char *getWordWrapped(const char *str, int lineLength) {
  static char wrapped[VIDD_MAXSTRLEN];
  
  int charCount = 0;
  const char *getStep = str;
  char *putStep = wrapped;

  while (*getStep) {
    if (*getStep == '\n') charCount = 0;
    else charCount++;
    
    if (charCount >= lineLength) {
      while (*getStep != ' ' && getStep != str) {
        getStep--;
        putStep--;
      }
      getStep++;
      *putStep++ = '\n';
      charCount = 0;
    }
    *putStep++ = *getStep++;
  }
  
  *putStep = 0;
  
  return wrapped;
}

//---------------------------------------------------------------------------
void appendStringEllipsied(char *target, const char *toEllipseAndAppend, int ellipseLen, int maxLen) {
  static char str[VIDD_MAXSTRLEN];
  //static char ep[VIDD_MAXSTRLEN];
  if (ellipseLen < 3) ellipseLen = 3;
  
  strncpy(str, toEllipseAndAppend, VIDD_MAXSTRLEN);
  
  if (strlen(str) > (size_t)ellipseLen) {
    str[ellipseLen-3] = '.';
    str[ellipseLen-2] = '.';
    str[ellipseLen-1] =  0;  
  }
  
  appendString(target, str, maxLen);
}

//---------------------------------------------------------------------------
const char *getFloatAsString(float val) {
  static char str[VIDD_MAXSTRLEN];
  sprintf(str, "% 4.2f", val);
  return str;
}

//---------------------------------------------------------------------------
void copyString(char *dst, const char *src, int maxLen) {
  strncpy(dst, src, maxLen-1);
  dst[maxLen-1] = 0;
}

//---------------------------------------------------------------------------
void appendColoredString(char *target, const char *toAppend, char color, int maxLen) {
  char colorStr[2];
  colorStr[0] = color;
  colorStr[1] = 0;
  appendString(target, "\x1b", maxLen);
  appendString(target, colorStr, maxLen);
  appendString(target, toAppend, maxLen);
}

//---------------------------------------------------------------------------
const char *getProgressBar(int length, float percentage, int fill) {
  int b = 0, c = 0;
  static char bar[VIDD_MAXSTRLEN];
  int transitionSpot = (int)(length*percentage);
  
  bar[b++] = '\x1b';
  bar[b++] = fill ? COLOR_GREEN : COLOR_GREY;
  
  for (c=0; c<length && c<VIDD_MAXSTRLEN-1; c++) {
    if (!fill) {
      if (c == transitionSpot) { 
        bar[b++] = '\x1b';
        bar[b++] = COLOR_GREEN;
        bar[b++] = '[';
      }
      else if (c == transitionSpot+1) {
        bar[b++] = ']';
        bar[b++] = '\x1b';
        bar[b++] = COLOR_GREY;
      }
      else bar[b++] = '=';
    }
    else {
      if (c == transitionSpot) {
        bar[b++] = '\x1b';
        bar[b++] = COLOR_GREY;       
      }
      bar[b++] = '=';
    }
  }
  bar[b] = 0;
  return bar;
}

//---------------------------------------------------------------------------
const char *getTimeAsString(int milliseconds, int includeFractionalSeconds) {
  static char buf[VIDD_MAXSTRLEN];
  int t, seconds, minutes, hours, frac;
  t = milliseconds/1000;
  seconds = t % 60;
  t = (t - seconds) / 60;
  minutes = t % 60;
  t = (t - minutes) / 60;
  hours = t;
  frac = (milliseconds%1000)/10;
  
  if (includeFractionalSeconds) {
    if (hours)        sprintf(buf, "%2i:%02i:%02i.%02i", hours, minutes, seconds, frac);
    else if (minutes) sprintf(buf, "%2i:%02i.%02i", minutes, seconds, frac);  
    else              sprintf(buf, ":%2i.%02i", seconds, frac);
  }
  else {
    if (hours)        sprintf(buf, "%2i:%02i:%02i", hours, minutes, seconds);
    else if (minutes) sprintf(buf, "%2i:%02i", minutes, seconds);  
    else              sprintf(buf, ":%2i", seconds);  
  }  
  
  
  return buf;
}

//---------------------------------------------------------------------------
void replaceColor(char *str, int maxLen, char replace, char with) {
  int c;  
  for (c=0; str[c] && str[c+1] && c<maxLen-1; c++) {
    if (str[c] != '\x1b') continue;
    if (str[c+1] != replace) continue;
    str[c+1] = with;  
  }
}

//-----------------------------------------------------------------------------
int getFileExists(const char *filename) {
  FILE *fd = 0;
  fd = fopen(filename, "r");
  if (!fd) return 0;
  fclose(fd);
  return 1;
}

//---------------------------------------------------------------------------
// OS-Dependent functions
//---------------------------------------------------------------------------
#ifdef _WIN32
  #include <windows.h> // GetModuleFileName, _splitpath, SetCurrentDirectory
#endif

void setCurDirToModuleDir() {
#ifdef _WIN32
  // this is only needed for Win32 drag and drop support. explorer 
  // doesn't set the working directory to that of the .exe that gets 
  // dropped upon; this being a problem for prboom
  char moduleFilename[MAX_PATH+1];
  char drive[MAX_PATH+1];
  char dir[MAX_PATH+1];
  char fname[MAX_PATH+1];
  char ext[MAX_PATH+1];  

  GetModuleFileName(GetModuleHandle(0), moduleFilename, MAX_PATH);
  moduleFilename[MAX_PATH] = 0;

  _splitpath(moduleFilename, drive, dir, fname, ext);

  strcat(drive, dir);
  SetCurrentDirectory(drive);
#endif
}

#endif // #ifdef COMPILE_VIDD
