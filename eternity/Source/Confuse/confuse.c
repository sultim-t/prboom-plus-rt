/* Configuration file parser -*- tab-width: 4; -*-
 *
 * Copyright (c) 2002-2003, Martin Hedenfalk <mhe@home.se>
 * Modifications for Eternity Copyright (c) 2005 James Haley
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

#include <limits.h>
#include <errno.h>
#include <sys/types.h>

// haleyjd: Use zone memory and other assets such as wad/file io
// emulation, since this is now being made Eternity-specific by
// necessity.

#include "../z_zone.h"
#include "../d_io.h"
#include "../i_system.h"
#include "../w_wad.h"

#include "confuse.h"

extern DWFILE *currentFile; // haleyjd
extern char *mytext; // haleyjd
int mylex(cfg_t *cfg);
void lexer_init(void);
void lexer_reset(void);

// haleyjd 03/08/03: Modifications for Eternity
#define PACKAGE_VERSION "Eternity version"
#define PACKAGE_STRING  "libConfuse"

// haleyjd: removed ENABLE_NLS
#define _(str) str
#define N_(str) str

const char *confuse_version = PACKAGE_VERSION;
const char *confuse_copyright = PACKAGE_STRING" by Martin Hedenfalk <mhe@home.se>";
const char *confuse_author = "Martin Hedenfalk <mhe@home.se>";

// haleyjd: added data param
extern int cfg_lexer_include(cfg_t *cfg, const char *fname, int data);

#if defined(NDEBUG)
#define cfg_assert(test) ((void)0)
#else
#define cfg_assert(test) \
  ((void)((test)||(my_assert(#test, __FILE__, __LINE__),0)))
#endif

#define STATE_CONTINUE 0
#define STATE_EOF -1
#define STATE_ERROR 1

//
// my_assert
//
// haleyjd 04/03/03: Eternity cannot use assert, as it aborts the
// program, leaving everything messy. So this has replaced assert,
// and calls I_Error instead.
//
static void my_assert(const char *msg, const char *file, int line)
{
   I_Error("Assertion failed at %s line %d: %s\n", file, line, msg);
}

#ifndef HAVE_STRNDUP
static char *strndup(const char *s, size_t n)
{
   char *r;
   
   if(s == 0)
      return 0;
   
   r = (char *)malloc(n + 1);
   strncpy(r, s, n);
   r[n] = 0;
   return r;
}
#endif

cfg_opt_t *cfg_getopt(cfg_t *cfg, const char *name)
{
   int i;
   cfg_t *sec = cfg;
   
   cfg_assert(cfg && cfg->name && name);

   // haleyjd 07/11/03: from CVS, traverses subsections
   while(name && *name)
   {
      char *secname;
      size_t len = strcspn(name, "|");

      if(name[len] == 0) /* no more subsections */
         break;
      if(len)
      {
         secname = strndup(name, len);
         sec = cfg_getsec(sec, secname);
         if(sec == 0)
            cfg_error(cfg, _("no such option '%s'"), secname);
         free(secname);
         if(sec == 0)
            return 0;
      }
      name += len;
      name += strspn(name, "|");
   }
   
   for(i = 0; sec->opts[i].name; i++)
   {
      if(is_set(CFGF_NOCASE, sec->flags))
      {
         if(strcasecmp(sec->opts[i].name, name) == 0)
            return &sec->opts[i];
      } 
      else 
      {
         if(strcmp(sec->opts[i].name, name) == 0)
            return &sec->opts[i];
      }
   }
   cfg_error(cfg, _("no such option '%s'"), name);
   return 0;
}

const char *cfg_title(cfg_t *cfg)
{
   return cfg->title;
}

unsigned int cfg_size(cfg_t *cfg, const char *name)
{
   cfg_opt_t *opt = cfg_getopt(cfg, name);
   if(opt)
      return opt->nvalues;
   return 0;
}

signed long cfg_getnint(cfg_t *cfg, const char *name, unsigned int index)
{
   cfg_opt_t *opt = cfg_getopt(cfg, name);
   
   if(opt)
   {
      cfg_assert(opt->type == CFGT_INT);
      if(opt->nvalues == 0)
         return (signed long)opt->def;
      else
      {
         cfg_assert(index < opt->nvalues);
         return opt->values[index]->number;
      }
   }
   else
      return 0;
}

signed long cfg_getint(cfg_t *cfg, const char *name)
{
   return cfg_getnint(cfg, name, 0);
}

double cfg_getnfloat(cfg_t *cfg, const char *name, unsigned int index)
{
   cfg_opt_t *opt = cfg_getopt(cfg, name);
   
   if(opt) 
   {
      cfg_assert(opt->type == CFGT_FLOAT);
      if(opt->nvalues == 0)
         return opt->fpdef;
      else
      {
         cfg_assert(index < opt->nvalues);
         return opt->values[index]->fpnumber;
      }
   }
   else
      return 0;
}

double cfg_getfloat(cfg_t *cfg, const char *name)
{
   return cfg_getnfloat(cfg, name, 0);
}

cfg_bool_t cfg_getnbool(cfg_t *cfg, const char *name, unsigned int index)
{
   cfg_opt_t *opt = cfg_getopt(cfg, name);
   
   if(opt)
   {
      cfg_assert(opt->type == CFGT_BOOL);
      if(opt->nvalues == 0)
         return (cfg_bool_t)opt->def;
      else
      {
         cfg_assert(index < opt->nvalues);
         return opt->values[index]->boolean;
      }
   }
   else
      return cfg_false;
}

cfg_bool_t cfg_getbool(cfg_t *cfg, const char *name)
{
   return cfg_getnbool(cfg, name, 0);
}

char *cfg_getnstr(cfg_t *cfg, const char *name, unsigned int index)
{
   cfg_opt_t *opt = cfg_getopt(cfg, name);
   
   if(opt)
   {
      cfg_assert(opt->type == CFGT_STR);
      if(opt->nvalues == 0) 
         return (char *)opt->def;
      else 
      {
         cfg_assert(index < opt->nvalues);
         return opt->values[index]->string;
      }
   }
   return 0;
}

char *cfg_getstr(cfg_t *cfg, const char *name)
{
   return cfg_getnstr(cfg, name, 0);
}

cfg_t *cfg_getnsec(cfg_t *cfg, const char *name, unsigned int index)
{
   cfg_opt_t *opt = cfg_getopt(cfg, name);
   
   if(opt) 
   {
      cfg_assert(opt->type == CFGT_SEC);
      if(!opt->values)
      {
         puts("opt->values is null!\n");
      }
      cfg_assert(opt->values);
      cfg_assert(index < opt->nvalues);
      return opt->values[index]->section;
   }
   return 0;
}

cfg_t *cfg_gettsec(cfg_t *cfg, const char *name, const char *title)
{
   unsigned int i, n;
   
   n = cfg_size(cfg, name);
   
   for(i = 0; i < n; i++) 
   {
      cfg_t *sec = cfg_getnsec(cfg, name, i);
      cfg_assert(sec && sec->title);
      
      if(is_set(CFGF_NOCASE, cfg->flags))
      {
         if(strcasecmp(title, sec->title) == 0)
            return sec;
      } 
      else 
      {
         if(strcmp(title, sec->title) == 0)
            return sec;
      }
   }
   return 0;
}

cfg_t *cfg_getsec(cfg_t *cfg, const char *name)
{
   return cfg_getnsec(cfg, name, 0);
}

static cfg_value_t *cfg_addval(cfg_opt_t *opt)
{
   opt->values = (cfg_value_t **)realloc(opt->values,
                                         (opt->nvalues+1) * sizeof(cfg_value_t *));
   if(!opt->values)
   {
      puts("opt->values is null!\n");
   }
   cfg_assert(opt->values);
   opt->values[opt->nvalues] = (cfg_value_t *)malloc(sizeof(cfg_value_t));
   memset(opt->values[opt->nvalues], 0, sizeof(cfg_value_t));
   return opt->values[opt->nvalues++];
}

static cfg_opt_t *cfg_dupopts(cfg_opt_t *opts)
{
   int n;
   cfg_opt_t *dupopts;
   
   for(n = 0; opts[n].name; n++) /* do nothing */ ;

   dupopts = (cfg_opt_t *)malloc(++n * sizeof(cfg_opt_t));
   memcpy(dupopts, opts, n * sizeof(cfg_opt_t));
   return dupopts;
}

int cfg_parse_boolean(const char *s)
{
   if(strcasecmp(s, "true") == 0
      || strcasecmp(s, "on") == 0
      || strcasecmp(s, "yes") == 0)
      return 1;
   else if(strcasecmp(s, "false") == 0
      || strcasecmp(s, "off") == 0
      || strcasecmp(s, "no") == 0)
      return 0;
   return -1;
}

cfg_value_t *cfg_setopt(cfg_t *cfg, cfg_opt_t *opt, char *value)
{
   cfg_value_t *val = 0;
   int b;
   char *s;
   double f;
   long int i;
   char *endptr;
   
   cfg_assert(cfg && opt);
   
   if(opt->simple_value)
   {
      cfg_assert(opt->type != CFGT_SEC);
      val = (cfg_value_t *)opt->simple_value;
   } 
   else 
   {
      // haleyjd: code from 2.0: check for sections with same name

      if(opt->nvalues == 0 || is_set(CFGF_MULTI, opt->flags) ||
         is_set(CFGF_LIST, opt->flags))
      {
         val = 0;
         if(opt->type == CFGT_SEC && is_set(CFGF_TITLE, opt->flags))
         {
            unsigned int i;

            /* check if there is already a section with the same title */
            cfg_assert(value);
            for(i = 0; i < opt->nvalues; i++)
            {
               cfg_t *sec = opt->values[i]->section;
               if(is_set(CFGF_NOCASE, cfg->flags))
               {
                  if(strcasecmp(value, sec->title) == 0)
                     val = opt->values[i];
               }
               else
               {
                  if(strcmp(value, sec->title) == 0)
                     val = opt->values[i];
               }
            }
         }
         if(val == 0)
            val = cfg_addval(opt);
      }
      else
         val = opt->values[0];
   }
   
   switch(opt->type)
   {
   case CFGT_INT:
      if(opt->cb)
      {
         if((*opt->cb)(cfg, opt, value, &i) != 0)
            return 0;
         val->number = i;
      } 
      else 
      {
         val->number = strtol(value, &endptr, 0);
         if(*endptr != '\0')
         {
            cfg_error(cfg, _("invalid integer value for option '%s'"),
                      opt->name);
            return 0;
         }
         if(errno == ERANGE) 
         {
            cfg_error(cfg,
               _("integer value for option '%s' is out of range"),
               opt->name);
            return 0;
         }
      }
      break;
   case CFGT_FLOAT:
      if(opt->cb)
      {
         if((*opt->cb)(cfg, opt, value, &f) != 0)
            return 0;
         val->fpnumber = f;
      } 
      else 
      {
         val->fpnumber = strtod(value, &endptr);
         if(*endptr != '\0')
         {
            cfg_error(cfg,
               _("invalid floating point value for option '%s'"),
               opt->name);
            return 0;
         }
         if(errno == ERANGE)
         {
            cfg_error(cfg,
               _("floating point value for option '%s' is out of range"),
               opt->name);
            return 0;
         }
      }
      break;
   case CFGT_STR:
      if(val->string)
         free(val->string);
      if(opt->cb)
      {
         s = 0;
         if((*opt->cb)(cfg, opt, value, &s) != 0)
            return 0;
         val->string = strdup(s);
      } else
         val->string = strdup(value);
      break;
   case CFGT_SEC:
      // haleyjd 07/11/03: CVS bug fix for section overwrite mem. leak
      cfg_free(val->section);
      val->section = (cfg_t *)malloc(sizeof(cfg_t));
      cfg_assert(val->section);
      memset(val->section, 0, sizeof(cfg_t));
      val->section->name = strdup(opt->name);
      val->section->opts = cfg_dupopts(opt->subopts);
      val->section->flags = cfg->flags;
      val->section->flags |= CFGF_ALLOCATED;
      val->section->filename = cfg->filename;
      val->section->line = cfg->line;
      val->section->errfunc = cfg->errfunc;
      val->section->title = value;
      break;
   case CFGT_BOOL:
      if(opt->cb)
      {
         if((*opt->cb)(cfg, opt, value, &b) != 0)
            return 0;
      } 
      else
      {
         b = cfg_parse_boolean(value);
         if(b == -1)
         {
            cfg_error(cfg, _("invalid boolean value for option '%s'"),
               opt->name);
            return 0;
         }
      }
      val->boolean = (cfg_bool_t)b;
      break;
   default:
      cfg_error(cfg, "internal error in cfg_setopt(%s, %s)",
                opt->name, value);
      cfg_assert(0);
      break;
   }
   return val;
}

void cfg_free_value(cfg_opt_t *opt)
{
   unsigned int i;
   
   if(opt == 0)
      return;
   
   for(i = 0; i < opt->nvalues; i++)
   {
      if(opt->type == CFGT_STR)
         free(opt->values[i]->string);
      else if(opt->type == CFGT_SEC)
         cfg_free(opt->values[i]->section);
      free(opt->values[i]);
   }
   free(opt->values);
   opt->values = 0;
   opt->nvalues = 0;
}

cfg_errfunc_t cfg_set_error_function(cfg_t *cfg, cfg_errfunc_t errfunc)
{
   cfg_errfunc_t old;
   
   cfg_assert(cfg);   
   old = cfg->errfunc;
   cfg->errfunc = errfunc;
   return old;
}

void cfg_error(cfg_t *cfg, const char *fmt, ...)
{
   va_list ap;
   
   va_start(ap, fmt);
   
   if(cfg->errfunc)
   {
      (*cfg->errfunc)(cfg, fmt, ap);
   }
   else 
   {
      if(cfg && cfg->filename && cfg->line)
         fprintf(stderr, "%s:%d: ", cfg->filename, cfg->line);
      else if(cfg && cfg->filename)
         fprintf(stderr, "%s: ", cfg->filename);
      vfprintf(stderr, fmt, ap);
      fprintf(stderr, "\n");
   }
   
   va_end(ap);
}

static int call_function(cfg_t *cfg, cfg_opt_t *opt, cfg_opt_t *funcopt)
{
   int ret;
   const char **argv;
   unsigned int i;

   /* create a regular argv string vector and call
    * the registered function
    */

   argv = (const char **)malloc(funcopt->nvalues * sizeof(char *));

   for(i = 0; i < funcopt->nvalues; i++)
      argv[i] = funcopt->values[i]->string;
   ret = (*opt->func)(cfg, opt, funcopt->nvalues, argv);
   cfg_free_value(funcopt); // haleyjd: CVS fix
   free((char **)argv);
   return ret;
}

static int cfg_parse_internal(cfg_t *cfg, int level)
{
   int state = 0, next_state = 0;
   char *opttitle = 0;
   cfg_opt_t *opt = 0;
   cfg_value_t *val;
   cfg_bool_t append_value;
   cfg_opt_t funcopt = CFG_STR(0,0,0);
   cfg_bool_t found_func = cfg_false; // haleyjd
   
   while(1)
   {
      int tok = mylex(cfg); // haleyjd
      
      if(tok == 0)
      {
         /* lexer.l should have called cfg_error */
         return STATE_ERROR;
      }
      
      if(tok == EOF)
      {
         if(state != 0)
         {
            if(state == 10) // haleyjd: catch EOF while in lookfor state
               cfg_error(cfg, "missing closing conditional function");
            else
               cfg_error(cfg, _("premature end of file"));
            return STATE_ERROR;
         }
         return STATE_EOF;
      }

      switch(state)
      {
      case 0: /* expecting an option name */
         if(tok == '}')
         {
            if(level == 0)
            {
               cfg_error(cfg, _("unexpected closing brace"));
               return STATE_ERROR;
            }
            return STATE_EOF;
         }
         if(tok != CFGT_STR)
         {
            cfg_error(cfg, _("unexpected token '%s'"), mytext);
            return STATE_ERROR;
         }
         opt = cfg_getopt(cfg, mytext); // haleyjd
         if(opt == 0)
            return STATE_ERROR;
         if(opt->type == CFGT_SEC)
         {
            if(is_set(CFGF_TITLE, opt->flags))
               state = 6;
            else
               state = 5;
         } 
         else if(opt->type == CFGT_FUNC)
         {
            state = 7;
         } 
         else
            state = 1;
         break;
      case 1: /* expecting an equal sign or plus-equal sign */
         append_value = cfg_false;
         if(tok == '+')
         {
            if(!is_set(CFGF_LIST, opt->flags))
            {
               cfg_error(cfg,
                  _("attempt to append to non-list option %s"),
                  opt->name);
               return STATE_ERROR;
            }
            append_value = cfg_true;
         } 
         else if(tok != '=')
         {
            cfg_error(cfg, _("missing equal sign after option '%s'"),
               opt->name);
            return STATE_ERROR;
         }
         if(is_set(CFGF_LIST, opt->flags))
         {
            if(!append_value)
               cfg_free_value(opt);
            state = 3;
         }
         else
         {
            state = 2;
            next_state = 0;
         }
         break;
      case 2: /* expecting an option value */
         if(tok == '}' && is_set(CFGF_LIST, opt->flags))
         {
            state = 0;
            break;
         }
         
         if(tok != CFGT_STR)
         {
            cfg_error(cfg, _("unexpected token '%s'"), mytext);
            return STATE_ERROR;
         }
         
         if(cfg_setopt(cfg, opt, mytext) == 0)
            return STATE_ERROR;
         state = next_state;
         break;
      case 3: /* expecting an opening brace for a list option */
         if(tok != '{')
         {
            cfg_error(cfg, _("missing opening brace for option '%s'"),
               opt->name);
            return STATE_ERROR;
         }
         state = 2;
         next_state = 4;
         break;
         
      case 4: /* expecting a separator for a list option, or
               * closing (list) brace */
         if(tok == ',')
         {
            state = 2;
            next_state = 4;
         } 
         else if(tok == '}')
            state = 0;
         else
         {
            cfg_error(cfg, _("unexpected token '%s'"), mytext);
            return STATE_ERROR;
         }
         break;
      case 5: /* expecting an opening brace for a section */
         if(tok != '{')
         {
            cfg_error(cfg, _("missing opening brace for section '%s'"),
               opt->name);
            return STATE_ERROR;
         }
         
         val = cfg_setopt(cfg, opt, opttitle);
         opttitle = 0;
         if(!val)
            return STATE_ERROR;
         
         if(cfg_parse_internal(val->section, level+1) != STATE_EOF)
            return STATE_ERROR;
         cfg->line = val->section->line;
         state = 0;
         break;
      case 6: /* expecting a title for a section */
         if(tok != CFGT_STR)
         {
            cfg_error(cfg, _("missing title for section '%s'"),
                      opt->name);
            return STATE_ERROR;
         }
         else
            opttitle = strdup(mytext);
         state = 5;
         break;
      case 7: /* expecting a opening parenthesis for a function */
         if(tok != '(')
         {
            cfg_error(cfg, _("missing parenthesis for function '%s'"),
                      opt->name);
            return STATE_ERROR;
         }
         state = 8;
         break;
      case 8: /* expecting a function parameter or closing paren*/
         if(tok == ')')
         {
            int ret = call_function(cfg, opt, &funcopt);
            if(ret != 0)
               return STATE_ERROR;
            // haleyjd 09/30/05: check for LOOKFORFUNC flag
            if(is_set(CFGF_LOOKFORFUNC, cfg->flags))
            {
               found_func = cfg_false;
               state = 10; // go to new "lookfor" state
            }
            else
               state = 0;
         }
         else if(tok == CFGT_STR)
         {
            val = cfg_addval(&funcopt);
            val->string = strdup(mytext);
            state = 9;
         } 
         else 
         {
            cfg_error(cfg, _("syntax error in call of function '%s'"),
                      opt->name);
            return STATE_ERROR;
         }
         break;
      case 9: /* expecting a comma in a function or a closing paren */
         if(tok == ')')
         {
            int ret = call_function(cfg, opt, &funcopt);
            if(ret != 0)
               return STATE_ERROR;
            // haleyjd 01/14/04: check for LOOKFORFUNC flag
            if(is_set(CFGF_LOOKFORFUNC, cfg->flags))
            {
               found_func = cfg_false;
               state = 10; // go to new "lookfor" state
            }
            else
               state = 0;
         }
         else if(tok == ',')
         {
            state = 8;
         }
         else 
         {
            cfg_error(cfg, _("syntax error in call of function '%s'"),
                      opt->name);
            return STATE_ERROR;
         }
         break;
      case 10:
         // haleyjd 01/16/04: special state which looks for the
         // "lookfor" function, allowing ifdef-type stuff which
         // ignores everything until the function is encountered
         if(is_set(CFGF_NOCASE, cfg->flags))
         {
            if(!strcasecmp(mytext, cfg->lookfor))
               found_func = cfg_true;
         }
         else
         {
            if(!strcmp(mytext, cfg->lookfor))
               found_func = cfg_true;
         }
         if(found_func == cfg_true)
         {
            opt = cfg_getopt(cfg, mytext);
            if(opt == 0)
               return STATE_ERROR;
            if(opt->type == CFGT_FUNC)
               state = 7; // parse the function call
            else
            {
               cfg_error(cfg, "internal error");
               return STATE_ERROR;
            }
         }
         break;
      default:
         /* missing state, internal error, abort */
         cfg_assert(0);
      }
   }

   return STATE_EOF;
}

cfg_t *cfg_init(cfg_opt_t *opts, cfg_flag_t flags)
{
   cfg_t *cfg;
   
   cfg = (cfg_t *)malloc(sizeof(cfg_t));
   cfg_assert(cfg);
   memset(cfg, 0, sizeof(cfg_t));

   cfg->name = "root";
   cfg->opts = opts;
   cfg->flags = flags;
   cfg->filename = 0;
   cfg->line = 0;
   cfg->lumpnum = -1; // haleyjd
   cfg->errfunc = 0;
   cfg->lookfor = NULL; // haleyjd

   // haleyjd: removed ENABLE_NLS

   return cfg;
}

char *cfg_tilde_expand(const char *filename)
{   
   // haleyjd 03/08/03: removed tilde expansion for eternity
   return strdup(filename);
}

int cfg_parse(cfg_t *cfg, const char *filename)
{
   DWFILE dwfile; // haleyjd
   int ret; // haleyjd

   cfg_assert(cfg && filename);
 
   // haleyjd 07/11/03: CVS fix
   if(cfg->filename)
      free(cfg->filename);

   cfg->filename = cfg_tilde_expand(filename);
   if(cfg->filename == 0)
   {
      cfg_error(cfg, _("%s: can't expand home directory"), filename);
      return CFG_FILE_ERROR;
   }

   cfg->line = 1;

   cfg->lumpnum = -1; // haleyjd 07/20/05

   D_OpenFile(&dwfile, filename, "r");
   
   if(!D_IsOpen(&dwfile))
      return CFG_FILE_ERROR;

   // haleyjd: initialize the lexer
   lexer_init();

   currentFile = &dwfile;
   ret = cfg_parse_internal(cfg, 0);

   // haleyjd: wow, should probably close the file huh?
   D_Fclose(&dwfile);

   // haleyjd: reset the lexer state
   lexer_reset();

   if(ret == STATE_ERROR)
      return CFG_PARSE_ERROR;

   return CFG_SUCCESS;
}

// 
// cfg_parselump
// 
// haleyjd 04/03/03: allow input from WAD lumps
//
int cfg_parselump(cfg_t *cfg, const char *lumpname)
{
   DWFILE dwfile; // haleyjd
   int ret; // haleyjd

   cfg_assert(cfg && lumpname);

   // haleyjd 07/11/03: CVS fix
   if(cfg->filename)
      free(cfg->filename);
   
   cfg->filename = cfg_tilde_expand(lumpname);
   if(cfg->filename == 0)
   {
      cfg_error(cfg, _("%s: can't expand home directory"), lumpname);
      return CFG_FILE_ERROR;
   }

   cfg->line = 1;

   cfg->lumpnum = W_GetNumForName(cfg->filename); // haleyjd 07/20/05

   D_OpenLump(&dwfile, cfg->lumpnum);
   
   if(!D_IsOpen(&dwfile))
      return CFG_FILE_ERROR;

   // haleyjd 02/28/05: woops, forgot this!
   lexer_init();
   
   currentFile = &dwfile;
   ret = cfg_parse_internal(cfg, 0);

   // haleyjd: wow, should probably close the file huh?
   D_Fclose(&dwfile);

   // reset the lexer state
   lexer_reset();

   if(ret == STATE_ERROR)
      return CFG_PARSE_ERROR;
   
   return CFG_SUCCESS;
}

void cfg_free(cfg_t *cfg)
{
   int i;
   
   if(cfg == 0)
      return;
   
   for(i = 0; cfg->opts[i].name; ++i)
      cfg_free_value(&cfg->opts[i]);

   if(is_set(CFGF_ALLOCATED, cfg->flags))
   {
      free(cfg->name);
      free(cfg->opts);
      free(cfg->title);
   }
   else
      free(cfg->filename);
   
   free(cfg);
}

int cfg_include(cfg_t *cfg, cfg_opt_t *opt, int argc, const char **argv)
{
   if(argc != 1)      
   {
      cfg_error(cfg, _("wrong number of arguments to cfg_include()"));
      return 1;
   }
   
   return cfg_lexer_include(cfg, argv[0], -1); // haleyjd
}

// EOF

