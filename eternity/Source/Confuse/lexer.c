/* Configuration file parser -*- tab-width: 4; -*-
 *
 * Copyright (c) 2002, Martin Hedenfalk <mhe@home.se>
 * Modifications for Eternity Copyright (c) 2003 James Haley
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

// haleyjd: Replacement lexer for libConfuse -- the flex lexer was
// not able to meet my needs and was in some respects too general.
// All of this code, with the exception of the include stuff, is
// brand spankin' new.

#if defined(DJGPP) || defined(LINUX)
#include <errno.h>
#endif

#include <ctype.h>

#include "../z_zone.h"
#include "../d_io.h"
#include "../w_wad.h"
#include "../m_qstr.h"

#include "confuse.h"

#define _(str) str
#define N_(str) str

DWFILE *currentFile; // haleyjd

// file include stack

#define MAX_INCLUDE_DEPTH 16

static struct cfginclude_s
{
   DWFILE *dwfile; // haleyjd
   char *filename;
   int lumpnum;    // haleyjd
   unsigned int line;
} include_stack[MAX_INCLUDE_DEPTH];

static int include_stack_ptr = 0;

char *mytext; // haleyjd: equivalent to yytext

// haleyjd 07/11/03: dynamic string buffer solution from 
// libConfuse v2.0; eliminates unsafe, overflowable array
// 02/12/04: replaced with generalized solution in m_qstr.c

static qstring_t qstring;

// state enumeration for lexer FSA

enum
{
   STATE_NONE,
   STATE_SLCOMMENT,
   STATE_MLCOMMENT,
   STATE_MLCOMMENTSTAR,
   STATE_SLASH,
   STATE_STRING,
   STATE_UNQUOTEDSTRING,
   STATE_PLUS,
};

void lexer_init(void)
{
   M_QStrCreate(&qstring);
}

void lexer_reset(void)
{
   // clear include stack
   memset(include_stack, 0, MAX_INCLUDE_DEPTH*sizeof(struct cfginclude_s));
   include_stack_ptr = 0;

   // reset lexer variables
   mytext = NULL;

   // free qstring buffer
   M_QStrFree(&qstring);
}

//
// lexer_error
//
// haleyjd 12/16/03:
// Calls cfg_error with a nice, uniform error message.
//
static void lexer_error(cfg_t *cfg, const char *msg)
{
   cfg_error(cfg, "lexer error: %s", msg);
}

//
// mylex
//
// Retrieves tokens from the current DWFILE for use by libConfuse
//
int mylex(cfg_t *cfg)
{
   int state = STATE_NONE;
   int stringtype = 0;
   char c;

include:
   while((c = D_Fgetc(currentFile)) != EOF)
   {
      // translate \r into \n
      if(c == '\r')
         c = '\n';

      switch(state)
      {
      case STATE_SLCOMMENT: // reading single-line comment
         // discard input up to next \n
         if(c == '\n')
         {
            cfg->line++;
            state = STATE_NONE;
         }
         continue;

      case STATE_MLCOMMENT: // reading multiline comment
         if(c == '\n')
            cfg->line++;
         // look for '*' to signal end of comment
         if(c == '*')
            state = STATE_MLCOMMENTSTAR;
         continue;

      case STATE_MLCOMMENTSTAR: // found star in multiline comment
         if(c == '\n')
            cfg->line++;
         // if found /, end comment, else go back to STATE_MLCOMMENT
         if(c == '/')
            state = STATE_NONE;
         else
            state = STATE_MLCOMMENT;
         continue;

      case STATE_SLASH: // read a / character
         if(c == '/') // begin single-line comment
         {
            state = STATE_SLCOMMENT;
            continue;
         }
         else if(c == '*') // begin multiline comment
         {
            state = STATE_MLCOMMENT;
            continue;
         }
         else
         {            
            lexer_error(cfg, "unexpected character after /");
            return 0;
         }

      case STATE_STRING: // reading a quoted string
         switch(c)
         {
         case '\t': // collapse hard tabs to spaces
            M_QStrPutc(&qstring, ' ');
            break;
         case '\f':
         case '\n': // free linebreak -- not allowed
            lexer_error(cfg, "unterminated string constant");
            return 0;
         case '\\': // escaped characters or line continuation
            {
               char s = D_Fgetc(currentFile);
               if(s == EOF)
               {                  
                  lexer_error(cfg, "EOF in string constant");
                  return 0;
               }
               else
               {
                  switch(s)
                  {
                  case '\n':
                     // line continuance for quoted strings!
                     // increment the line
                     cfg->line++;

                     // loop until a non-whitespace char is found
                     while(isspace((s = D_Fgetc(currentFile))));
                     
                     // better not be EOF!
                     if(s == EOF)
                     {
                        lexer_error(cfg, "EOF in string constant");
                        return 0;
                     }

                     // put back the last char
                     D_Ungetc(s, currentFile);
                     break;
                  case 'n':
                     M_QStrPutc(&qstring, '\n');
                     break;
                  case 't':
                     M_QStrPutc(&qstring, '\t');
                     break;
                  case 'a':
                     M_QStrPutc(&qstring, '\a');
                     break;
                  case 'b':
                     M_QStrPutc(&qstring, '\b');
                     break;
                  default:
                     M_QStrPutc(&qstring, s);
                     break;
                  }
                  continue;
               }
            }
            break;
         case '"':
            if(stringtype == 1) // double-quoted string, end it
            {
               mytext = M_QStrBuffer(&qstring);
               return CFGT_STR;
            }
            M_QStrPutc(&qstring, c);
            continue;
         case '\'':
            if(stringtype == 2) // single-quoted string, end it
            {               
               mytext = M_QStrBuffer(&qstring);
               return CFGT_STR;
            }
            M_QStrPutc(&qstring, c);
            continue;
         default:
            M_QStrPutc(&qstring, c);
            continue;
         }
         break;
         
      case STATE_UNQUOTEDSTRING: // unquoted string
         if(c == ' '  || c == '"'  || c == '\'' || c == '\t' || 
            c == '\n' || c == '\f' || c == '='  || c == '{'  || 
            c == '}'  || c == '('  || c == ')'  || c == '+'  || 
            c == ','  || c == '#'  || c == '/'  || c == ';')
         {
            // any special character ends an unquoted string
            D_Ungetc(c, currentFile); // put it back
            mytext = M_QStrBuffer(&qstring);
            return CFGT_STR;
         }
         else // normal characters
         {
            M_QStrPutc(&qstring, c);
            continue;
         }
         break;

      case STATE_PLUS: // read a + character
         if(c == '=')
         {
            mytext = "+=";
            return '+';
         }
         else
         {
            lexer_error(cfg, "unexpected character after +");
            return 0;
         }
         break;

      default:
         break;
      }

      // STATE_NONE:
      switch(c)
      {
      case ';':  // throw away optional semicolons
      case ' ':  // throw away whitespace
      case '\f':
      case '\t':
         continue;
      case '\n': // count and throw away line breaks
         cfg->line++;
         continue;
      case '#':
         state = STATE_SLCOMMENT;
         continue;
      case '/':
         state = STATE_SLASH;
         continue;
      case '{':
         mytext = "{";
         return '{';
      case '}':
         mytext = "}";
         return '}';
      case '(':
         mytext = "(";
         return '(';
      case ')':
         mytext = ")";
         return ')';
      case '=':
         mytext = "=";
         return '=';
      case '+':
         state = STATE_PLUS;
         continue;
      case ',':
         mytext = ",";
         return ',';
      case '"': // open double-quoted string
         M_QStrClear(&qstring);
         state = STATE_STRING;
         stringtype = 1;
         continue;
      case '\'': // open single-quoted string
         M_QStrClear(&qstring);
         state = STATE_STRING;
         stringtype = 2;
         continue;
      default:  // anything else is part of an unquoted string
         M_QStrClear(&qstring);
         M_QStrPutc(&qstring, c);
         state = STATE_UNQUOTEDSTRING;
         continue;
      }
   }

   if(state == STATE_STRING) // EOF in quoted string: not allowed
   {
      lexer_error(cfg, "EOF in string constant");
      return 0;
   }
   else if(state == STATE_UNQUOTEDSTRING)
   {
      // EOF after unquoted string -- return the string, next
      // call will return EOF      
      mytext = M_QStrBuffer(&qstring);
      return CFGT_STR;
   }
   else
   {
      // EOF handling -- check the include stack
      if(--include_stack_ptr < 0)
      {
         return EOF;
      }      
      else
      {
         // done with an include file
         D_Fclose(currentFile); // haleyjd
         free(currentFile);
         
         currentFile   = include_stack[include_stack_ptr].dwfile;
         free(cfg->filename);
         cfg->filename = include_stack[include_stack_ptr].filename;
         cfg->line     = include_stack[include_stack_ptr].line;
         cfg->lumpnum  = include_stack[include_stack_ptr].lumpnum;

         state = STATE_NONE; // make sure its not in an odd state
         goto include; // haleyjd: goto -- kill me now!
      }      
   }

   return EOF; // probably not reachable, but whatever
}

int cfg_lexer_include(cfg_t *cfg, const char *filename, int data)
{
   DWFILE *temp;
   char *xfilename;

   if(include_stack_ptr >= MAX_INCLUDE_DEPTH)
   {
      cfg_error(cfg, "Error: includes nested too deeply.");
      return 1;
   }

   // haleyjd
   include_stack[include_stack_ptr].dwfile   = currentFile;
   include_stack[include_stack_ptr].filename = cfg->filename;
   include_stack[include_stack_ptr].line     = cfg->line;
   include_stack[include_stack_ptr].lumpnum  = cfg->lumpnum;
   include_stack_ptr++;

   xfilename = cfg_tilde_expand(filename);

   // haleyjd: DWFILE handling
   temp = malloc(sizeof(DWFILE));

   // haleyjd 02/09/05: revised include handling for data vs file
   if(data >= 0)
      D_OpenLump(temp, data);
   else
      D_OpenFile(temp, xfilename, "r");

   currentFile = temp;

   if(!D_IsOpen(currentFile)) // haleyjd
   {
      cfg_error(cfg, "Error including file %s:\n%s", xfilename, 
                errno ? strerror(errno) : "unknown error"); // haleyjd
      free(temp); // haleyjd
      free(xfilename);
      return 1;
   }

   cfg->filename = xfilename;
   cfg->line = 1;
   cfg->lumpnum = data;

   return 0;
}

//
// cfg_lexer_source_type
//
// haleyjd 02/09/05: function to tell whether current source is
// data or a physical file. If physical file, it will return -1.
// If a lump, it returns the lump number.
//
int cfg_lexer_source_type(cfg_t *cfg)
{
   if(!currentFile)
   {
      cfg_error(cfg, "cfg_lexer_source_type: no file is open");
      return 0;
   }

   return cfg->lumpnum;
}

// EOF

