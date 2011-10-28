#ifndef __SC_MAN__
#define __SC_MAN__

void SC_OpenLump(const char *name);
void SC_OpenLumpByNum(int lump);
void SC_Close(void);
dboolean SC_GetString(void);
void SC_MustGetString(void);
void SC_MustGetStringName(const char *name);
dboolean SC_GetNumber(void);
void SC_MustGetNumber(void);
void SC_UnGet(void);
dboolean SC_Check(void);
dboolean SC_Compare(const char *text);
int SC_MatchString(const char **strings);
int SC_MustMatchString(const char **strings);
void SC_ScriptError(const char *message);

extern char *sc_String;
extern int sc_Number;
extern int sc_Line;
extern dboolean sc_End;
extern dboolean sc_Crossed;
extern dboolean sc_FileScripts;

#endif // __SC_MAN__
