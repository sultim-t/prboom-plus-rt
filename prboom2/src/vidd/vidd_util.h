#ifdef COMPILE_VIDD

#ifndef VIDD_UTIL_H
#define VIDD_UTIL_H

//---------------------------------------------------------------------------
#define COLOR_LIGHTRED '0'
#define COLOR_LIGHTYELLOW '1'
#define COLOR_WHITE '2'
#define COLOR_GREEN '3'
#define COLOR_GREY '4'
#define COLOR_YELLOW '5'
#define COLOR_RED '6'
#define COLOR_LIGHTBLUE '7'
#define COLOR_ORANGE '8'

//---------------------------------------------------------------------------
void appendString(char *target, const char *toAppend, int maxLen);
void copyString(char *dst, const char *src, int maxLen);
void appendStringEllipsied(char *target, const char *toEllipseAndAppend, int ellipseLen, int maxLen);
const char *getFloatAsString(float val);
void appendColoredString(char *target, const char *toAppend, char color, int maxLen);
const char *getProgressBar(int length, float percentage, int fill);
const char *getTimeAsString(int milliseconds, int includeFractionalSeconds);
void replaceColor(char *str, int maxLen, char replace, char with);
int getFileExists(const char *filename);
int getStateNum(void *state);
void setCurDirToModuleDir();
const char *getWordWrapped(const char *str, int lineLength);
int getStringAsInt(const char *str);
const char *getIntAsString(int val);

//---------------------------------------------------------------------------

#endif

#endif // #ifdef COMPILE_VIDD
