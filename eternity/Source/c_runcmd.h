// Emacs style mode select -*- C++ -*-
//----------------------------------------------------------------------------
//
// Copyright(C) 2000 James Haley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------

#ifndef __C_RUNCMD_H__
#define __C_RUNCMD_H__

#include "d_io.h"

// NETCODE_FIXME -- CONSOLE_FIXME -- CONFIG_FIXME: Commands and 
// variables need tweaks and extensions to fully support archiving in
// the configuration and possibly in savegames, and also telling what
// commands and variables are sync-critical. The main addition needed
// is the support of defaults for ALL archived console variables and
// a way to set a variable's value without changing its default.

typedef struct command_s command_t;
typedef struct variable_s variable_t;

/******************************** #defines ********************************/

#define MAXTOKENS 32
#define MAXTOKENLENGTH 64
#define CMDCHAINS 16

// zdoom _inspired_:

// create a new console command, eg.
//        CONSOLE_COMMAND(say_something, cf_notnet)
//        {
//              C_Printf("hello!\n");
//        }

#define CONSOLE_COMMAND(name, flags)                    \
        void Handler_ ## name(void);                    \
        command_t Cmd_ ## name = { # name, ct_command,  \
                       flags, NULL, Handler_ ## name,   \
                       0 };                             \
        void Handler_ ## name(void)

		 
// console variable. you must define the range of values etc. for
//      the variable using the other macros below.
//      You must also provide a handler function even
//      if it is just {}

#define CONSOLE_VARIABLE(name, variable, flags)                      \
        void Handler_ ## name(void);                                 \
        command_t Cmd_ ## name = { # name, ct_variable,              \
                        flags, &var_ ## variable, Handler_ ## name,  \
                        0 };                                         \
        void Handler_ ## name(void)

// Same as CONSOLE_COMMAND, but sync-ed across network. When
//      this command is executed, it is run on all computers.
//      You must assign your variable a unique netgame
//      variable (list in c_net.h)

#define CONSOLE_NETCMD(name, flags, netcmd)             \
        void Handler_ ## name(void);                    \
        command_t Cmd_ ## name = { # name, ct_command,  \
                       (flags) | cf_netvar, NULL,       \
                       Handler_ ## name, netcmd };      \
        void Handler_ ## name()

// As for CONSOLE_VARIABLE, but for net, see above

#define CONSOLE_NETVAR(name, variable, flags, netcmd)               \
        void Handler_ ## name(void);                                \
        command_t Cmd_ ## name = { # name, ct_variable,             \
                        cf_netvar | (flags), &var_ ## variable,     \
                        Handler_ ## name, netcmd };                 \
        void Handler_ ## name(void)

// Create a constant. You must declare the variable holding
//      the constant using the variable macros below.

#define CONSOLE_CONST(name, variable)                           \
        command_t Cmd_ ## name = { # name, ct_constant, 0,      \
                &var_ ## variable, NULL, 0 };

        /*********** variable macros *************/

// Each console variable has a corresponding C variable.
// It also has a variable_t which contains data such as,
// a pointer to the variable, the range of values it can
// take, etc. These macros allow you to define variable_t's
// more easily.

// basic VARIABLE macro. You must specify all the data needed

#define VARIABLE(name, defaultvar, type, min, max, strings)  \
        variable_t var_ ## name = { &name, defaultvar,       \
                        type, min, max, strings};

// simpler macro for int. You do not need to specify the type

#define VARIABLE_INT(name, defaultvar, min, max, strings)    \
        variable_t var_ ## name = { &name, defaultvar,       \
                        vt_int, min, max, strings};

// Simplified to create strings: 'max' is the maximum string length

#define VARIABLE_STRING(name, defaultvar, max)               \
        variable_t var_ ## name = { &name, defaultvar,       \
                        vt_string, 0, max, NULL};

// Boolean. Note that although the name here is boolean, the
// actual type is int.

#define VARIABLE_BOOLEAN(name, defaultvar, strings)          \
        variable_t var_ ## name = { &name, defaultvar,       \
                        vt_int, 0, 1, strings };

// basic variable_t creators for constants.

#define CONST_INT(name)                                      \
        variable_t var_ ## name = { &name, NULL,             \
                        vt_int, -1, -1, NULL};

#define CONST_STRING(name)                                   \
        variable_t var_ ## name = { &name, NULL,             \
                        vt_string, -1, -1, NULL};


#define C_AddCommand(c)  (C_AddCommand)(&Cmd_ ## c) 

/********************************* ENUMS **********************************/

enum    // cmdtype values
{
  c_typed,        // typed at console
  c_menu,
  c_netcmd,
  c_script,	  // haleyjd: started by command script
  C_CMDTYPES
};

enum    // command type
{
  ct_command,
  ct_variable,
  ct_constant,
  ct_end
};

enum    // command flag
{
  cf_notnet       =1,     // not in netgames
  cf_netonly      =2,     // only in netgames
  cf_server       =4,     // server only 
  cf_handlerset   =8,     // if set, the handler sets the variable,
                          // not c_runcmd.c itself
  cf_netvar       =16,    // sync with other pcs
  cf_level        =32,    // only works in levels
  cf_hidden       =64,    // hidden in cmdlist
  cf_buffered     =128,   // buffer command: wait til all screen
                          // rendered before running command
};

//
// CONSOLE_FIXME: Implement float and toggle (boolean) ASAP.
//

enum    // variable type
{
  vt_int,                // normal integer 
  vt_float,              // decimal               NOT IMPLEMENTED
  vt_string,             // string
  vt_toggle              // on/off value          NOT IMPLEMENTED
};

/******************************** STRUCTS ********************************/

struct variable_s
{
  // NB: for strings, this is char ** not char *
  void *variable;
  void *v_default;         // the default 
  int type;       // vt_?? variable type: int, string
  int min;        // minimum value or string length
  int max;        // maximum value/length
  char **defines;  // strings representing the value: eg "on" not "1"
};

struct command_s
{
  char *name;
  int type;               // ct_?? command type
  int flags;              // cf_??
  variable_t *variable;
  void (*handler)(void);  // handler
  int netcmd;     // network command number
  command_t *next;        // for hashing
};

typedef struct alias_s
{
  char *name;
  char *command;
  
  struct alias_s *next; // haleyjd 04/14/03

} alias_t;

/************************** PROTOTYPES/EXTERNS ****************************/

/***** command running ****/

extern command_t *c_command;
extern int cmdtype;
extern char c_argv[MAXTOKENS][MAXTOKENLENGTH];
extern int c_argc;
extern char c_args[128];

void C_RunCommand(command_t *command, const char *options);
void C_RunTextCmd(const char *cmdname);

const char *C_VariableValue(variable_t *command);
const char *C_VariableStringValue(variable_t *command);

// haleyjd: the SMMU v3.30 script-running functions
// (with my fixes :P)

void C_RunScript(DWFILE *);
void C_RunScriptFromFile(const char *filename);

/**** tab completion ****/

void C_InitTab();
char *C_NextTab(char *key);
char *C_PrevTab(char *key);

/**** aliases ****/

extern alias_t aliases; // haleyjd 04/14/03: changed to linked list
extern char *cmdoptions;

alias_t *C_NewAlias(unsigned char *aliasname, unsigned char *command);
void C_RemoveAlias(unsigned char *aliasname);
alias_t *C_GetAlias(char *name);

/**** command buffers ****/

void C_BufferCommand(int cmdtype, command_t *command,
                     const char *options, int cmdsrc);
void C_RunBuffers();
void C_RunBuffer(int cmtype);
void C_BufferDelay(int, int);
void C_ClearBuffer(int);

/**** hashing ****/

extern command_t *cmdroots[CMDCHAINS];   // the commands in hash chains

void (C_AddCommand)(command_t *command);
void C_AddCommandList(command_t *list);
void C_AddCommands();
command_t *C_GetCmdForName(char *cmdname);

/***** define strings for variables *****/

extern char *yesno[];
extern char *onoff[];
extern char *colournames[];
extern char *textcolours[];
extern char *skills[];

#endif

// EOF
