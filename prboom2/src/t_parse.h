/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
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
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#ifndef __PARSE_H__
#define __PARSE_H__

#include "m_fixed.h"
#include "p_mobj.h"     // for mobj_t

#define T_MAXTOKENS 64
#define TOKENLENGTH 128
 
typedef struct script_s script_t;
typedef struct svalue_s svalue_t;
typedef struct operator_s operator_t;

struct svalue_s
{
  int type;
  union
  {
    long i;
    char *s;
    char *labelptr; // goto() label
    fixed_t f;
    mobj_t *mobj;
  } value;
};

        // furthered to include support for svt_mobj
#define intvalue(v)                                               \
  ( (v).type == svt_string ? atoi((v).value.s) :                  \
    (v).type == svt_fixed ? ((v).value.f / FRACUNIT) :            \
    (v).type == svt_mobj ? -1 : (v).value.i )
  
#define fixedvalue(v)                                             \
  ( (v).type == svt_fixed ? (v).value.f :                         \
    (v).type == svt_string ? (atof((v).value.s) * FRACUNIT) :     \
    intvalue(v) * FRACUNIT )

char *stringvalue(svalue_t v);

#include "t_vari.h"
#include "t_prepro.h"

#define MAXSCRIPTS 128

struct script_s
{
  // script data
  
  char *data;
  int scriptnum;  // this script's number
  int len;
  
  // {} sections
  
  section_t *sections[SECTIONSLOTS];
  
  // variables:
  
  svariable_t *variables[VARIABLESLOTS];
  
  // ptr to the parent script
  // the parent script is the script above this level
  // eg. individual linetrigger scripts are children
  // of the levelscript, which is a child of the
  // global_script
  script_t *parent;

  // child scripts.
  // levelscript holds ptrs to all of the level's scripts
  // here.
  
  script_t *children[MAXSCRIPTS];
  
  mobj_t *trigger;        // object which triggered this script
};

struct operator_s
{
  char *string;
  svalue_t (*handler)(int, int, int); // left, mid, right
  int direction;
};

enum
{
  forward,
  backward
};

void run_script(script_t *script);
void continue_script(script_t *script, char *continue_point);
void parse_include(char *lumpname);
void run_statement();
void script_error(char *s, ...);

svalue_t evaluate_expression(int start, int stop);
int find_operator(int start, int stop, char *value);
int find_operator_backwards(int start, int stop, char *value);

/******* tokens **********/

typedef enum
{
  name,   // a name, eg 'count1' or 'frag'
  number,
  operator,
  string,
  unset,
  function          // function name
} tokentype_t;

enum    // brace types: where current_section is a { or }
{
  bracket_open,
  bracket_close
};

extern svalue_t nullvar;
extern int script_debug;

extern script_t *current_script;
extern mobj_t *trigger_obj;
extern int killscript;

extern char *tokens[T_MAXTOKENS];
extern tokentype_t tokentype[T_MAXTOKENS];
extern int num_tokens;
extern char *rover;     // current point reached in parsing
extern char *linestart; // start of the current expression

extern section_t *current_section;
extern section_t *prev_section;
extern int bracetype;

// the global_script is the root
// script and contains only built-in
// FraggleScript variables/functions

extern script_t global_script; 
extern script_t hub_script;

#endif
