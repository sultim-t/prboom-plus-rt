/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: c_runcmd.c,v 1.2 2001/07/09 14:21:52 proff_fs Exp $
 *
 *  PrBoom a Doom port merged with LxDoom and LSDLDoom
 *  based on BOOM, a modified and improved DOOM engine
 *  Copyright (C) 1999 by
 *  id Software, Chi Hoang, Lee Killough, Jim Flynn, Rand Phares, Ty Halderman
 *  Copyright (C) 1999-2000 by
 *  Jess Haas, Nicolas Kalkhof, Colin Phipps, Florian Schulze
 *  
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
 *  02111-1307, USA.
 *
 * DESCRIPTION:
 *
 * Command running functions
 *
 * Running typed commands, or network/linedef triggers. Sending commands over
 * the network. Calls handlers for some commands.
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

static const char
rcsid[] = "$Id: c_runcmd.c,v 1.2 2001/07/09 14:21:52 proff_fs Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "c_io.h"
#include "c_runcmd.h"
//#include "c_net.h"

#include "doomdef.h"
#include "doomstat.h"
//#include "mn_engin.h"
//#include "t_script.h"
#include "g_game.h"
#include "z_zone.h"
#include "lprintf.h"

/* proff - dummys */

mobj_t *t_trigger;
command_t *c_netcmds[20];
int cmdsrc = 0;           // the source of a network console command
#define CN_BROADCAST 128

void MN_ErrorMsg(char *s)
{
}

void C_SendCmd(int dest, int cmdnum, char *s,...)
{
}

/* proff - end of dummys */

static void C_EchoValue(command_t *command);
static void C_SetVariable(command_t *command);
static void C_RunAlias(alias_t *alias);
static int C_Sync(command_t *command);
static void C_ArgvtoArgs();
static boolean C_Strcmp(unsigned char *a, unsigned char *b);

///////////////////////////////////////////////////////////////////////////
//
// Parsing/Running Commands
//

int cmdtype;

static char cmdtokens[MAXTOKENS][MAXTOKENLENGTH];
static int numtokens;

command_t *c_command;
char c_argv[MAXTOKENS][MAXTOKENLENGTH];   // tokenised list of arguments
int c_argc;                               // number of arguments
char c_args[128];                         // raw list of arguments

        // break up the command into tokens
static void C_GetTokens(char *command)
{
  char *rover;
  boolean quotemark=0;

  rover = command;
  cmdtokens[0][0] = 0;
  numtokens = 1;

  while(*rover == ' ') rover++;
  if(!*rover)     // end of string already
    {
      numtokens = 0;
      return;
    }

  while(*rover)
    {
      if(*rover=='"')
	{
	  quotemark = !quotemark;
	  rover++;
	  continue;
	}
      if(*rover==' ' && !quotemark)   // end of token
	{
	  // only if the current one actually contains something
	  if(cmdtokens[numtokens-1][0])
	    {
	      numtokens++;
	      cmdtokens[numtokens-1][0] = 0;
	    }
	  rover++;
	  continue;
	}

      // add char to line
      sprintf(cmdtokens[numtokens-1], "%s%c",
	      cmdtokens[numtokens-1], *rover);
      rover++;
    }
}

//
// C_RunIndivTextCmd.
// called once a typed command line has been broken
// into individual commands (multiple commands on one
// line are allowed with the ';' character)

static void C_RunIndivTextCmd(char *cmdname)
{
  command_t *command;
  alias_t *alias;
  
  // cut off leading spaces
  while(*cmdname==' ') cmdname++;
  
  // break into tokens
  C_GetTokens(cmdname);
  
  // find the command being run from the first token.
  command = C_GetCmdForName(cmdtokens[0]);
  if(!numtokens) return; // no command
  if(!command)    // no _command_ called that
    {
      // alias?
      if((alias = C_GetAlias(cmdtokens[0])))
	{
	  // save the options into cmdoptions
	  cmdoptions = cmdname + strlen(cmdtokens[0]);
	  C_RunAlias(alias);
	}
      else         // no alias either
	C_Printf("unknown command: '%s'\n",cmdtokens[0]);
      return;
    }

  // run the command (buffer it)
  C_RunCommand(command, cmdname+strlen(cmdtokens[0]));
}

// check the flags of a command to see if it
// should be run or not

static boolean C_CheckFlags(command_t *command)
{
  char *errormsg;
  
  // check the flags
  errormsg = NULL;

  if((command->flags & cf_notnet) && (netgame && !demoplayback))
    errormsg = "not available in netgame";
  if((command->flags & cf_netonly) && !netgame && !demoplayback)
    errormsg = "only available in netgame";
  if((command->flags & cf_server) && consoleplayer && !demoplayback
     && cmdtype!=c_netcmd)
    errormsg = "for server only";
  if((command->flags & cf_level) && gamestate != GS_LEVEL)
    errormsg = "can be run in levels only";
  
  // net-sync critical variables are usually critical to
  // demo sync too
  if((command->flags & cf_netvar) && demoplayback)
    errormsg = "not during demo playback";
  
  if(errormsg)
    {
      C_Printf("%s: %s\n", command->name, errormsg);
      MN_ErrorMsg(errormsg);   // menu error
      return true;
    }

  return false;
}

// C_RunCommand.
// call with the command to run and the command-line options.
// buffers the commands which will be run later.

void C_RunCommand(command_t *command, char *options)
{
  // do not run straight away, we might be in the middle of rendering
  C_BufferCommand(cmdtype, command, options, cmdsrc);

  cmdtype = c_typed;  // reset to typed command as default
}

// actually run a command. Same as C_RunCommand only instant.

static void C_DoRunCommand(command_t *command, char *options)
{
  int i;

  C_GetTokens(options);
  
  memcpy(c_argv, cmdtokens, sizeof cmdtokens);
  c_argc = numtokens;
  c_command = command;
  
  // perform checks
  
  // check through the tokens for variable names
  for(i=0 ; i<c_argc ; i++)
    {
      if(c_argv[i][0]=='%' || c_argv[i][0]=='$') // variable
	{
	  command_t *variable;
	  
	  variable = C_GetCmdForName(c_argv[i]+1);
	  if(!variable || !variable->variable)
	    {
	      C_Printf("unknown variable '%s'\n",c_argv[i]+1);
	      // clear for next time
	      cmdtype = c_typed; cmdsrc = consoleplayer;
	      return;
	    }

	  strcpy(c_argv[i], c_argv[i][0]=='%' ?
		 C_VariableValue(variable->variable) :
		 C_VariableStringValue(variable->variable) );
	}
    }

  C_ArgvtoArgs();                 // build c_args

  // actually do this command
  switch(command->type)
    {
    case ct_command:
      // not to be run ?
      if(C_CheckFlags(command) ||
	 C_Sync(command))
	{
	  cmdtype = c_typed; cmdsrc = consoleplayer; 
	  return;
	}
      if(command->handler)
	command->handler();
      else
	C_Printf("error: no command handler for %s\n", command->name);
      break;
		
    case ct_constant:
      C_EchoValue(command);
      break;
      
    case ct_variable:
      C_SetVariable(command);
      break;

    default:
      C_Printf("unknown command type %i\n",command->type);
      break;
    }

  cmdtype = c_typed; cmdsrc = consoleplayer;   // clear for next time
}

// take all the argvs and put them all together in the args string

static void C_ArgvtoArgs()
{
  int i, n;

  for(i=0; i<c_argc; i++)
    {
      if(!c_argv[i][0])       // empty string
	{
	  for(n=i; n<c_argc-1; n++)
	    strcpy(c_argv[n],c_argv[n+1]);
	  c_argc--; i--;
	}
    }

  c_args[0] = 0;
  
  for(i=0 ; i<c_argc; i++)
    sprintf(c_args, "%s%s ", c_args, c_argv[i]);
}

// return a string of all the argvs linked together, but with each
// argv in quote marks "

static char *C_QuotedArgvToArgs()
{
  int i;
  static char returnvar[1024];

  returnvar[0] = 0;
  
  for(i=0 ; i<c_argc; i++)
    sprintf(returnvar, "%s\"%s\" ", returnvar, c_argv[i]);

  return returnvar;
}

// see if the command needs to be sent to other computers
// to maintain sync and do so if neccesary

static int C_Sync(command_t *command)
{
  if(command->flags & cf_netvar)
    {
      // dont get stuck repeatedly sending the same command
      if(cmdtype != c_netcmd)
	{                               // send to sync
	  C_SendCmd(CN_BROADCAST, command->netcmd,
		    "%s", C_QuotedArgvToArgs());
	  return true;
	}
    }
  
  return false;
}

// execute a compound command (with or without ;'s)

void C_RunTextCmd(char *command)
{
  boolean quotemark=0;  // for " quote marks
  char *sub_command = NULL;
  char *rover;

  for(rover=command; *rover; rover++)
    {
      if(*rover=='\"')    // quotemark
	{
	  quotemark = !quotemark;
	  continue;
	}
      if(*rover==';' && !quotemark)  // command seperator and not in string 
	{
	  // found sub-command
	  // use recursion to run the subcommands

	  // left
	  // copy sub command, alloc slightly more than needed
	  sub_command = malloc(rover-command+3); 
	  strncpy(sub_command, command, rover-command);
	  sub_command[rover-command] = '\0';   // end string

	  C_RunTextCmd(sub_command);

	  // right
	  C_RunTextCmd(rover+1);

	  // leave to the other function calls (above) to run commands
	  free(sub_command);
	  return;
	}
    }
  
  // no sub-commands: just one
  // so run it

  C_RunIndivTextCmd(command);
}

// get the literal value of a variable (ie. "1" not "on")

char *C_VariableValue(variable_t *variable)
{
  static char value[128];
  
  if(!variable) return "";
  
  switch(variable->type)
    {
    case vt_int:
    case vt_toggle:
      sprintf(value, "%i", *(int*)variable->variable);
      break;
      
    case vt_string:
      sprintf(value, "%s", *(char**)variable->variable);
      break;

    default:
      sprintf(value, "(unknown)");
      break;
    }

  return value;
}

// get the string value (ie. "on" not "1")

char *C_VariableStringValue(variable_t *variable)
{
  static char value[128];

  if(!variable) return "";

  // does the variable have alternate 'defines' ?
  strcpy(value, variable->defines ?

	 // print the 'define' (string representing the value)
	 variable->defines[*(int*)variable->variable-variable->min] :

	 // otherwise print the literal value
	 C_VariableValue(variable) );

  return value;
}


// echo a value eg. ' "alwaysmlook" is "1" '

static void C_EchoValue(command_t *command)
{
  C_Printf("\"%s\" is \"%s\"\n", command->name,
	   C_VariableStringValue(command->variable) );
}

// is a string a number?

static boolean isnum(char *text)
{
  for(;*text; text++)
    if((*text>'9' || *text<'0') && *text!='-') return false;
  return true;
}

// take a string and see if it matches a define for a
// variable. Replace with the literal value if so.

static char* C_ValueForDefine(variable_t *variable, char *s)
{
  int count;
  static char returnstr[10];

  if(variable->defines)
    for(count = variable->min;count <= variable->max; count++)
      {
	if(!C_Strcmp(s, variable->defines[count-variable->min]))
	  {
	    sprintf(returnstr, "%i", count);
	    return returnstr;
	  }
      }
  
  // special hacks for menu

  if(variable->type == vt_int)    // int values only
    {
      if(!strcmp(s, "+"))     // increase value
	{
	  int value = *(int *)variable->variable + 1;
	  if(value > variable->max) value = variable->max;
	  sprintf(returnstr, "%i", value);
	  return returnstr;
	}
      if(!strcmp(s, "-"))     // decrease value
	{
	  int value = *(int *)variable->variable - 1;
	  if(value < variable->min) value = variable->min;
	  sprintf(returnstr, "%i", value);
	  return returnstr;
	}
      if(!strcmp(s, "/"))     // toggle value
	{
	  int value = *(int *)variable->variable + 1;
	  if(value > variable->max) value = variable->min; // wrap around
	  sprintf(returnstr, "%i", value);
	  return returnstr;
	}
      
      if(!isnum(s)) return NULL;
    }
  
  return s;
}
        // set a variable
static void C_SetVariable(command_t *command)
{
  variable_t* variable;
  int size = 0;
  char *errormsg;
  char *temp;
  
  // cut off the leading spaces
  
  if(!c_argc)     // asking for value
    {
      C_EchoValue(command);
      return;
    }

  // change it?
  if(C_CheckFlags(command)) return;       // no

  // ok, set the value
  variable = command->variable;

  temp = C_ValueForDefine(variable, c_argv[0]);
  
  if(temp)
    strcpy(c_argv[0], temp);
  else
    {
      C_Printf("not a possible value for '%s'\n", command->name);
      return;
    }      

  switch(variable->type)
    {
    case vt_int:
      size = atoi(c_argv[0]);
      break;
      
    case vt_string:
      size = strlen(c_argv[0]);
      break;

    default:
      return;
    }

  // check the min/max sizes
  
  errormsg = NULL;
  if(size > variable->max)
    errormsg = "value too big";
  if(size < variable->min)
    errormsg = "value too small";
  if(errormsg)
    {
      MN_ErrorMsg(errormsg);
      C_Printf("%s\n",errormsg);
      return;
    }
                
  // netgame sync: send command to other nodes
  if(C_Sync(command)) return;
  
  // now set it
  // 5/8/99 set default value also
  // 16/9/99 cf_handlerset flag for variables set from
  // the handler instead
  
  if(!(command->flags & cf_handlerset))
    switch(variable->type)  // implicitly set the variable
      {
      case vt_int:
	*(int*)variable->variable = atoi(c_argv[0]);
	if(variable->v_default && cmdsrc==c_typed)  // default
	  *(int*)variable->v_default = atoi(c_argv[0]);
	break;
	
      case vt_string:
	free(*(char**)variable->variable);
	*(char**)variable->variable = strdup(c_argv[0]);
	if(variable->v_default && cmdsrc==c_typed)  // default
	  {
	    free(*(char**)variable->v_default);
	    *(char**)variable->v_default = strdup(c_argv[0]);
	  }
	break;
        
      default:
	return;
      }
  
  if(command->handler)          // run handler if there is one
    command->handler();
}

////////////////////////////////////////////////////////////////////////
//
// Tab Completion
//

static char origkey[100];
static boolean gotkey;
static command_t *tabs[128];
static int numtabs = 0;
static int thistab = -1;

// given a key (eg. "r_sw"), will look through all
// the commands in the hash chains and gather
// all the commands which begin with this into a
// list 'tabs'

void GetTabs(char *key)
{
  int i;
  int keylen;

  numtabs = 0;
  while(*key==' ') key++;

  strcpy(origkey, key);
  gotkey = true;

  if(!*key) return;
  
  keylen = strlen(key);

  // check each hash chain in turn

  for(i=0; i<CMDCHAINS; i++)
    {
      command_t* browser = cmdroots[i];

      // go through each link in this chain
      for(; browser; browser = browser->next)
	if(!(browser->flags & cf_hidden) && // ignore hidden ones
	   !strncmp(browser->name, key, keylen))
	  {
	    // found a new tab
	    tabs[numtabs] = browser;
	    numtabs++;
	  }
    }
}

// reset the tab list 

void C_InitTab()
{
  numtabs = 0;
  origkey[0] = '\0';
  gotkey = false;
  thistab = -1;
}

// called when tab pressed. get the next tab
// from the list

char *C_NextTab(char *key)
{
  static char returnstr[100];

  // get tabs if not done already
  if(!gotkey)
      GetTabs(key);
  
  // select next tab
  thistab = thistab == -1 ? 0 : thistab+1;  

  if(thistab >= numtabs)
    {
      thistab = -1;
      return origkey;
    }

  sprintf(returnstr,"%s ",tabs[thistab]->name);
  return returnstr;
}

// called when shift-tab pressed. get the
// previous tab from the lift

char *C_PrevTab(char *key)
{
  static char returnstr[100];

  // get tabs if neccesary
  if(!gotkey)
    {
      GetTabs(key);
    }
  
  // select prev.
  thistab = thistab == -1 ? numtabs - 1 : thistab - 1;
  
  // check invalid
  if(thistab < 0)
    {
      thistab = -1;
      return origkey;
    }
  
  sprintf(returnstr,"%s ",tabs[thistab]->name);
  return returnstr;
}

/////////////////////////////////////////////////////////////////////////
//
// Aliases
//

// fixed length array arrgh!
alias_t aliases[128];
char *cmdoptions;       // command line options for aliases

        // get an alias from a name
alias_t *C_GetAlias(char *name)
{
  alias_t *alias = aliases;

  // search all aliases

  while(alias->name)
    {
      if(!strcmp(name, alias->name))
	{
	  return alias;
	}
      alias++;
    }
  
  return NULL;
}

// create a new alias, or use one that already exists

alias_t *C_NewAlias(unsigned char *aliasname, unsigned char *command)
{
  alias_t *alias;

  alias = aliases;

  while(alias->name)
    {
      if(!strcmp(alias->name, c_argv[0]))
	{
	  free(alias->name);
	  free(alias->command);
	  break;
	}
      alias++;
    }

  alias->name = strdup(aliasname);
  alias->command = strdup(command);
  
  return alias;
}

// remove an alias

void C_RemoveAlias(unsigned char *aliasname)
{
  alias_t *alias;

  alias = C_GetAlias(aliasname);
  if(!alias)
    {
      C_Printf("unknown alias \"%s\"\n", aliasname);
      return;
    }
    
  // free alias data
  free(alias->name); free(alias->command);
  
  // move all further aliases back
  while(alias->name)
    {
      memcpy(alias, alias+1, sizeof(alias_t));
      alias++;
    }
}

// run an alias

void C_RunAlias(alias_t *alias)
{
  // store command line for use in macro
  while(*cmdoptions==' ') cmdoptions++;
  C_RunTextCmd(alias->command);   // run the command
}

//////////////////////////////////////////////////////////////////////
//
// Command Bufferring
//

// new ticcmds can be built at any time including during the
// rendering process. The commands need to be buffered
// and run by the tickers instead of directly from the
// responders
//
// a seperate buffered command list is kept for each type
// of command (typed, script, etc)

// 15/11/99 cleaned up: now uses linked lists of bufferedcmd's
//              rather than a static array: no limit on
//              buffered commands and nicer code

typedef struct bufferedcmd_s bufferedcmd;

struct bufferedcmd_s
{
  command_t *command;     // the command
  char *options;          // command line options
  int cmdsrc;             // source player
  bufferedcmd *next;      // next in list
};

typedef struct
{
  // NULL once list empty
  bufferedcmd *cmdbuffer;
  int timer;      // tic timer to temporarily freeze executing of cmds
} cmdbuffer;

cmdbuffer buffers[C_CMDTYPES];

void C_RunBufferedCommand(bufferedcmd *bufcmd)
{
  // run command
  // restore variables
  
  cmdsrc = bufcmd->cmdsrc;
  t_trigger = players[cmdsrc].mo;
  C_DoRunCommand(bufcmd->command, bufcmd->options);
}

void C_BufferCommand(int cmtype, command_t *command, char *options,
                        int cmdsrc)
{
  bufferedcmd *bufcmd;
  bufferedcmd *newbuf;

  // create bufferedcmd
  newbuf = malloc(sizeof(bufferedcmd));

  // add to new bufferedcmd
  newbuf->command = command;
  newbuf->options = strdup(options);
  newbuf->cmdsrc = cmdsrc;
  newbuf->next = NULL;            // is always at end of chain

  // no need to be buffered: run it now
  if(!(command->flags & cf_buffered) && buffers[cmtype].timer == 0)
    {
      cmdtype = cmtype;
      C_RunBufferedCommand(newbuf);
      
      free(newbuf->options);
      free(newbuf);
      return;
    }

  // add to list now

  // select according to cmdtype
  bufcmd = buffers[cmtype].cmdbuffer;

  // list empty?
  if(!bufcmd)              // hook in
    buffers[cmtype].cmdbuffer = newbuf;
  else
    {
      // go to end of list
      for(; bufcmd->next; bufcmd = bufcmd->next);
      
      // point last to newbuf
      bufcmd->next = newbuf;
    }
}

void C_RunBuffer(int cmtype)
{
  bufferedcmd *bufcmd, *next;
  
  bufcmd = buffers[cmtype].cmdbuffer;
  
  while(bufcmd)
    {
      // check for delay timer
      if(buffers[cmtype].timer)
	{
	  buffers[cmtype].timer--;
	  break;
	}

      cmdtype = cmtype;
      C_RunBufferedCommand(bufcmd);
      
      // save next before freeing
      
      next = bufcmd->next;
      
      // free bufferedcmd
      
      free(bufcmd->options);
      free(bufcmd);

      // go to next in list
      bufcmd = buffers[cmtype].cmdbuffer = next;
    }

}

void C_RunBuffers()
{
  int i;

  // run all buffers

  for(i=0; i<C_CMDTYPES; i++)
    C_RunBuffer(i);
}

void C_BufferDelay(int cmdtype, int delay)
{
  buffers[cmdtype].timer += delay;
}

void C_ClearBuffer(int cmdtype)
{
  buffers[cmdtype].timer = 0;                     // clear timer
  buffers[cmdtype].cmdbuffer = NULL;              // empty 
}

        // compare regardless of font colour
boolean C_Strcmp(unsigned char *a, unsigned char *b)
{
  while(*a || *b)
    {
      // remove colour dependency
      if(*a >= 128)   // skip colour
	{
	  a++; continue;
	}
      if(*b >= 128)
	{
	  b++; continue;
	}
      // regardless of case also
      if(toupper(*a) != toupper(*b))
	return true;
      a++; b++;
    }

  return false;       // no difference in them
}

//////////////////////////////////////////////////////////////////
//
// Command hashing
//
// Faster look up of console commands

command_t *cmdroots[CMDCHAINS];

       // the hash key
#define CmdHashKey(s)                                     \
   (( (s)[0] + (s)[0] ? (s)[1] + (s)[1] ? (s)[2] +        \
                 (s)[2] ? (s)[3] + (s)[3] ? (s)[4]        \
                        : 0 : 0 : 0 : 0 ) % 16)

void (C_AddCommand)(command_t *command)
{
  int hash;
  
  hash = CmdHashKey(command->name);
  
  command->next = cmdroots[hash]; // hook it in at the start of
  cmdroots[hash] = command;       // the table

  // save the netcmd link
  if(command->flags & cf_netvar && command->netcmd == 0)
    C_Printf("C_AddCommand: cf_netvar without a netcmd (%s)\n", command->name);

  c_netcmds[command->netcmd] = command;
}

// add a list of commands terminated by one of type ct_end

void C_AddCommandList(command_t *list)
{
  for(;list->type != ct_end; list++)
    (C_AddCommand)(list);
}

// get a command from a string if possible
        
command_t *C_GetCmdForName(char *cmdname)
{
  command_t *current;
  int hash;

  while(*cmdname==' ') cmdname++;

  // start hashing
  
  hash = CmdHashKey(cmdname);
  
  current = cmdroots[hash];
  while(current)
    {
      if(!strcasecmp(cmdname, current->name))
	return current;
      current = current->next;        // try next in chain
    }
  
  return NULL;
}

/*
  haleyjd: Command Scripts
  From SMMU v3.30, these allow running command scripts from either
  files or buffered lumps -- very cool IMHO.
*/
  
void C_RunScript(char *script)
{
  char buffer[128] = "";
  char *rover;
  
  if(!script)
    return;
  
  for(rover = script; *rover; rover++)
  {
      if(*rover == '\n')      // end of line - run command
      {
	  //	  C_Puts(buffer);
	  
	  if(buffer[0] == '#' || buffer[0] == ';' ||
	     (buffer[0] == '/' && buffer[1] == '/'))
	  {
	      // comment
	  }
	  else
	  {
	      cmdtype = c_script;
	      C_RunTextCmd(buffer);
	      C_RunBuffer(c_script);   // force to run now
	  }
	  
	  // clear buffer for next line
	  buffer[0] = '\0';
      }
      else if(isprint(*rover))
      {
	 // haleyjd: we need overflow protection here, or a segv
	 // is possible
	 if(strlen(buffer) == 127)
	 {
	    // should have had a line break by now but we haven't.
	    // we'll assume the line is garbage and go on to the
	    // next one
	    buffer[0] = '\0';
	    continue;
	 }
	 	 
	 // add to end of buffer

	 buffer[strlen(buffer) + 1] = '\0';
	 buffer[strlen(buffer)] = *rover;
      }
  }
  
}

extern int M_ReadFile(const char*, byte**);

void C_RunScriptFromFile(char *filename)
{
  char *filedata;

  if(M_ReadFile(filename, (byte **)&filedata) <= 0) // haleyjd: ptr cast
    C_Printf("couldn't exec '%s'\n", filename);
  else
  {
     C_Printf("executing %s\n", filename);
     C_RunScript(filedata);
  }
}
