/* Emacs style mode select   -*- C++ -*- 
 *-----------------------------------------------------------------------------
 *
 * $Id: c_cmd.c,v 1.7 2002/11/15 17:20:27 proff_fs Exp $
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
//#include "c_net.h"

#include "m_random.h"

//extern const char *cmdoptions;
                /************* aliases ***************/
CONSOLE_COMMAND(alias, 0)
{
  alias_t *alias;
  char *temp;
  
  if(!c_argc)
    {
      // list em
      C_Printf(FC_GRAY"alias list:" FC_RED "\n\n");
      alias = aliases;
      while(alias->name)
	{
	  C_Printf("\"%s\": \"%s\"\n", alias->name,
		   alias->command);
	  alias++;
	}
      if(alias==aliases) C_Printf("(empty)\n");
      return;
    }
  
  if(c_argc == 1)  // only one, remove alias
    {
      C_RemoveAlias(c_argv[0]);
      return;
    }

  // find it or make a new one
  
  temp = c_args + strlen(c_argv[0]);
  while(*temp == ' ') temp++;
  
  C_NewAlias(c_argv[0], temp);
}

// %opt for aliases
CONST_STRING(cmdoptions);
CONSOLE_CONST(opt, cmdoptions);

// command list
CONSOLE_COMMAND(cmdlist, 0)
{
  int numonline = 0;
  command_t *current;
  int i;
  int charnum;
  
  // list each command from the hash chains
  
  //  5/8/99 change: use hash table and 
  //  alphabetical order by first letter

  for(charnum=33; charnum < 'z'; charnum++) // go thru each char in alphabet
    for(i=0; i<CMDCHAINS; i++)
      for(current = cmdroots[i]; current; current = current->next)
	{
	  if(current->name[0]==charnum && !(current->flags & cf_hidden))
	    {
	      C_Printf("%s ", current->name);
	      numonline++;
	      if(numonline >= 3)
		{
		  numonline = 0;
		  C_Printf("\n");
		}
	    }
	}
  C_Printf("\n");
}

// console height

//VARIABLE_INT(c_height,  NULL,                   20, 200, NULL);
//CONSOLE_VARIABLE(c_height, c_height, 0) {}

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

extern void Cheat_AddCommands();        // m_cheat.c
extern void     G_AddCommands();        // g_cmd.c
extern void    HU_AddCommands();        // hu_stuff.c
//extern void     I_AddCommands();        // i_system.c
//extern void   net_AddCommands();        // d_net.c
//extern void     P_AddCommands();        // p_cmd.c
extern void     R_AddCommands();        // r_main.c
extern void     S_AddCommands();        // s_sound.c
extern void    ST_AddCommands();        // st_stuff.c
//extern void     T_AddCommands();        // t_script.c
//extern void     V_AddCommands();        // v_misc.c
extern void    MN_AddCommands();        // mn_menu.c
extern void    AM_AddCommands();        // am_color.c

//extern void    PE_AddCommands();        // p_enemy.c -- haleyjd
extern void    G_Bind_AddCommands();    // g_bind.c  -- haleyjd
extern void    G_BindAxes_AddCommands();    // g_bindaxes.c
extern void    P_Chase_AddCommands();   // p_chase.c  -- proff (until P_AddCommands is used)

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
//  I_AddCommands();
//  net_AddCommands();
//  P_AddCommands();
  R_AddCommands();
  S_AddCommands();
  ST_AddCommands();
//  T_AddCommands();
//  V_AddCommands();
  MN_AddCommands();
  AM_AddCommands();
//  PE_AddCommands();  // haleyjd
  G_Bind_AddCommands();
  G_BindAxes_AddCommands();
  P_Chase_AddCommands();
}

