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
 * DESCRIPTION:
 *
 * 'Special' stuff
 *
 * if(), int statements, etc.
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include "c_io.h"
#include "z_zone.h"

#include "t_parse.h"
#include "t_spec.h"
#include "t_vari.h"

int find_operator(int start, int stop, char *value);

// ending brace found in parsing

void spec_brace()
{
  if(script_debug) C_Printf("brace\n");
  
  if(bracetype != bracket_close)  // only deal with closing } braces
    return;
  
  // if() requires nothing to be done
  if(current_section->type == st_if) return;
  
  // if a loop, jump back to the start of the loop
  if(current_section->type == st_loop)
    {
      rover = current_section->data.data_loop.loopstart;
      return;
    }
}

        // 'if' statement
void spec_if()
{
  int endtoken;
  svalue_t eval;
  
  if( (endtoken = find_operator(0, num_tokens-1, ")")) == -1)
    {
      script_error("parse error in if statement\n");
      return;
    }
  
  // 2 to skip past the 'if' and '('
  eval = evaluate_expression(2, endtoken-1);
  
  if(current_section && bracetype == bracket_open
     && endtoken == num_tokens-1)
    {
      // {} braces
      if(!intvalue(eval))       // skip to end of section
	rover = current_section->end+1;
    }
  else    // if() without {} braces
    if(intvalue(eval))
      {
	// nothing to do ?
	if(endtoken == num_tokens-1) return;
	evaluate_expression(endtoken+1, num_tokens-1);
      }
}

// while() loop

void spec_while()
{
  int endtoken;
  svalue_t eval;

  if(!current_section)
    {
      script_error("no {} section given for loop\n");
      return;
    }
  
  if( (endtoken = find_operator(0, num_tokens-1, ")")) == -1)
    {
      script_error("parse error in loop statement\n");
      return;
    }
  
  eval = evaluate_expression(2, endtoken-1);
  
  // skip if no longer valid
  if(!intvalue(eval)) rover = current_section->end+1;
}

void spec_for()                 // for() loop
{
  svalue_t eval;
  int start;
  int comma1, comma2;     // token numbers of the seperating commas
  
  if(!current_section)
    {
      script_error("need {} delimiters for for()\n");
      return;
    }
  
  // is a valid section
  
  start = 2;     // skip "for" and "(": start on third token(2)
  
  // find the seperating commas first
  
  if( (comma1 = find_operator(start,    num_tokens-1, ",")) == -1
      || (comma2 = find_operator(comma1+1, num_tokens-1, ",")) == -1)
    {
      script_error("incorrect arguments to if()\n");
      return;
    }
  
  // are we looping back from a previous loop?
  if(current_section == prev_section)
    {
      // do the loop 'action' (third argument)
      evaluate_expression(comma2+1, num_tokens-2);
      
      // check if we should run the loop again (second argument)
      eval = evaluate_expression(comma1+1, comma2-1);
      if(!intvalue(eval))
	{
	  // stop looping
	  rover = current_section->end + 1;
	}
    }
  else
    {
      // first time: starting the loop
      // just evaluate the starting expression (first arg)
      evaluate_expression(start, comma1-1);
    }
}

/**************************** Variable Creation ****************************/

int newvar_type;
script_t *newvar_script;

// called for each individual variable in a statement
//  newvar_type must be set

static void create_variable(int start, int stop)
{
  if(killscript) return;
  
  if(tokentype[start] != name)
    {
      script_error("invalid name for variable: '%s'\n",
		   tokens[start+1]);
      return;
    }
  
  // check if already exists, only checking
  // the current script
  if( variableforname(newvar_script, tokens[start]) )
    return;  // already one
  
  new_variable(newvar_script, tokens[start], newvar_type);
  
  if(stop != start) evaluate_expression(start, stop);
}

// divide a statement (without type prefix) into individual
// variables to be create them using create_variable

static void parse_var_line(int start)
{
  int starttoken = start, endtoken;
  
  while(1)
    {
      if(killscript) return;
      endtoken = find_operator(starttoken, num_tokens-1, ",");
      if(endtoken == -1) break;
      create_variable(starttoken, endtoken-1);
      starttoken = endtoken+1;  //start next after end of this one
    }
  // dont forget the last one
  create_variable(starttoken, num_tokens-1);
}

boolean spec_variable()
{
  int start = 0;

  newvar_type = -1;                 // init to -1
  newvar_script = current_script;   // use current script

  // check for 'hub' keyword to make a hub variable
  if(!strcmp(tokens[start], "hub"))
    {
      newvar_script = &hub_script;
      start++;  // skip first token
    }

  // now find variable type
  if(!strcmp(tokens[start], "const"))
    {
      newvar_type = svt_const;
      start++;
    }
  else if(!strcmp(tokens[start], "string"))
    {
      newvar_type = svt_string;
      start++;
    }
  else if(!strcmp(tokens[start], "int"))
    {
      newvar_type = svt_int;
      start++;
    }
  else if(!strcmp(tokens[start], "mobj"))
    {
      newvar_type = svt_mobj;
      start++;
    }
  else if(!strcmp(tokens[start], "float"))
    {
      newvar_type = svt_fixed;
      start++;
    }
  else if(!strcmp(tokens[start], "script"))     // check for script creation
    {
      spec_script();
      return true;       // used tokens
    }

  // other variable types could be added: eg float

  // are we creating a new variable?

  if(newvar_type != -1)
    {
      parse_var_line(start);
      return true;       // used tokens
    }

  return false; // not used: try normal parsing
}
