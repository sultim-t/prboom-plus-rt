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
 * Console commands
 *
 * basic console commands and variables for controlling
 * the console itself.
 *
 * By Simon Howard, added to PrBoom by Florian Schulze
 *
 *-----------------------------------------------------------------------------
 */

#include "c_io.h"
#include "c_runcmd.h"
#include "c_net.h"

#include "m_random.h"


                /************* aliases ***************/
CONSOLE_COMMAND(alias, 0)
{
  alias_t *alias;
  char *temp;

  // haleyjd 04/14/03: rewritten
  
  if(!c_argc)
    {
      // list em
      C_Printf(FC_HI"alias list:" FC_NORMAL "\n\n");

      alias = aliases.next;
      if(!alias)
      {
        C_Printf("(empty)\n");
      }
      else
      {
        while(alias)
        {
          C_Printf("\"%s\": \"%s\"\n", alias->name, alias->command);
          alias = alias->next;
        }
      }
      return;
    }

  if(c_argc == 1)  // only one, remove alias
    {
      C_RemoveAlias(c_argv[0]);
      return;
    }

  // find it or make a new one

  temp = c_args + strlen(c_argv[0]);
  while(*temp == ' ')
    temp++;

  C_NewAlias(c_argv[0], temp);
}

// %opt for aliases
CONST_STRING(cmdoptions);
CONSOLE_CONST(opt, cmdoptions);

// command list
CONSOLE_COMMAND(cmdlist, 0)
{
  command_t *current;
  int i;
  int charnum;

  // list each command from the hash chains

  //  5/8/99 change: use hash table and
  //  alphabetical order by first letter

  for(charnum=33; charnum < 'z'; charnum++) // go thru each char in alphabet
  {
    for(i=0; i<CMDCHAINS; i++)
    {
      for(current = cmdroots[i]; current; current = current->next)
      {
        if(current->name[0]==charnum && !(current->flags & cf_hidden))
        {
          C_Printf("%s\n", current->name);
        }
      }
    }
  }
}

// console height

CONSOLE_INT(c_height, c_height, NULL, 20, 200, NULL, 0) {}

// console speed

VARIABLE_INT(c_speed,   NULL,                   1, 200, NULL);
CONSOLE_VARIABLE(c_speed, c_speed, 0) {}

// echo string to console

CONSOLE_COMMAND(echo, 0)
{
  C_Puts(c_args);
}

// delay in console

CONSOLE_COMMAND(delay, 0)
{
  C_BufferDelay(cmdtype, c_argc ? atoi(c_argv[0]) : 1);
}

// flood the console with crap
// .. such a great and useful command

CONSOLE_COMMAND(flood, 0)
{
  int a;

  for(a=0; a<300; a++)
    C_Printf("%c\n", a%64 + 32);
}

        /******** add commands *******/

// command-adding functions in other modules

extern void Cheat_AddCommands(void);        // m_cheat.c
extern void     G_AddCommands(void);        // g_cmd.c
extern void    HU_AddCommands(void);        // hu_stuff.c
extern void     I_AddCommands(void);        // i_system.c
extern void   net_AddCommands(void);        // d_net.c
extern void     P_AddCommands(void);        // p_cmd.c
extern void     R_AddCommands(void);        // r_main.c
extern void     S_AddCommands(void);        // s_sound.c
extern void    ST_AddCommands(void);        // st_stuff.c
#ifdef FRAGGLE_SCRIPT
extern void     T_AddCommands(void);        // t_script.c
#endif
extern void     V_AddCommands(void);        // v_misc.c
extern void    MN_AddCommands(void);        // mn_menu.c
extern void    AM_AddCommands(void);        // am_map.c

extern void    PE_AddCommands(void);        // p_enemy.c -- haleyjd
extern void    G_Bind_AddCommands(void);    // g_bind.c  -- haleyjd
extern void    G_BindAxes_AddCommands(void);    // g_bindaxes.c
extern void    GL_AddCommands(void);        // gl_main.c

void C_AddCommands()
{
  C_AddCommand(c_height);
  C_AddCommand(c_speed);
  C_AddCommand(cmdlist);
  C_AddCommand(delay);
  C_AddCommand(alias);
  C_AddCommand(opt);
  C_AddCommand(echo);
  C_AddCommand(flood);

  // add commands in other modules
  Cheat_AddCommands();
  G_AddCommands();
  HU_AddCommands();
  I_AddCommands();
//  net_AddCommands();
  P_AddCommands();
  R_AddCommands();
  S_AddCommands();
  ST_AddCommands();
#ifdef FRAGGLE_SCRIPT
  T_AddCommands();
#endif
  V_AddCommands();
  MN_AddCommands();
  AM_AddCommands();
//  PE_AddCommands();  // haleyjd
  G_Bind_AddCommands();
  G_BindAxes_AddCommands();
#ifdef GL_DOOM
  GL_AddCommands();
#endif
}


